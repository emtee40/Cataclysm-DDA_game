#if defined(TILES)

#include "pixel_minimap.h"

#include <assert.h>
#include <stdlib.h>
#include <memory>
#include <set>
#include <vector>
#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <iterator>
#include <utility>

#include "avatar.h"
#include "coordinate_conversions.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "sdl_utils.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "creature.h"
#include "game_constants.h"
#include "int_id.h"
#include "lightmap.h"
#include "math_defines.h"
#include "optional.h"

extern void set_displaybuffer_rendertarget();

namespace
{

const point tiles_range = { ( MAPSIZE - 2 ) *SEEX, ( MAPSIZE - 2 ) *SEEY };

point get_pixel_size( const point &tile_size, pixel_minimap_mode mode )
{
    switch( mode ) {
        case pixel_minimap_mode::solid:
            return tile_size;

        case pixel_minimap_mode::squares:
            return { std::max( tile_size.x - 1, 1 ), std::max( tile_size.y - 1, 1 ) };

        case pixel_minimap_mode::dots:
            return { 1, 1 };
    }

    return {};
}

/// Returns a number in range [0..1]. The range lasts for @param phase_length_ms (milliseconds).
float get_animation_phase( int phase_length_ms )
{
    if( phase_length_ms == 0 ) {
        return 0.0f;
    }

    return std::fmod<float>( SDL_GetTicks(), phase_length_ms ) / phase_length_ms;
}

//creates the texture that individual minimap updates are drawn to
//later, the main texture is drawn to the display buffer
//the surface is needed to determine the color format needed by the texture
SDL_Texture_Ptr create_cache_texture( const SDL_Renderer_Ptr &renderer, int tile_width,
                                      int tile_height )
{
    return CreateTexture( renderer,
                          SDL_PIXELFORMAT_ARGB8888,
                          SDL_TEXTUREACCESS_TARGET,
                          tile_width,
                          tile_height );
}

SDL_Color get_map_color_at( const tripoint &p )
{
    const auto &m = g->m;

    if( const auto vp = m.veh_at( p ) ) {
        return curses_color_to_SDL( vp->vehicle().part_color( vp->part_index() ) );
    }

    if( const auto furn_id = m.furn( p ) ) {
        return curses_color_to_SDL( furn_id->color() );
    }

    return curses_color_to_SDL( m.ter( p )->color() );
}

SDL_Color get_critter_color( Creature *critter, int flicker, int mixture )
{
    auto result = curses_color_to_SDL( critter->symbol_color() );

    if( const auto m = dynamic_cast<monster *>( critter ) ) {
        //faction status (attacking or tracking) determines if red highlights get applied to creature
        const auto matt = m->attitude( &g->u );

        if( MATT_ATTACK == matt || MATT_FOLLOW == matt ) {
            const auto red_pixel = SDL_Color{ 0xFF, 0x0, 0x0, 0xFF };
            result = adjust_color_brightness( mix_colors( result, red_pixel, mixture ), flicker );
        }
    }

    return result;
}

} // namespace

// a texture pool to avoid recreating textures every time player changes their view
// at most 142 out of 144 textures can be in use due to regular player movement
//  (moving from submap corner to new corner) with MAPSIZE = 11
// textures are dumped when the player moves more than one submap in one update
//  (teleporting, z-level change) to prevent running out of the remaining pool
struct pixel_minimap::shared_texture_pool {
    std::vector<SDL_Texture_Ptr> texture_pool;
    std::set<int> active_index;
    std::vector<int> inactive_index;

    shared_texture_pool() {
        texture_pool.resize( ( MAPSIZE + 1 ) * ( MAPSIZE + 1 ) );
        for( int i = 0; i < static_cast<int>( texture_pool.size() ); i++ ) {
            inactive_index.push_back( i );
        }
    }

    //reserves a texture from the inactive group and returns tracking info
    SDL_Texture_Ptr request_tex( int &i ) {
        if( inactive_index.empty() ) {
            //shouldn't be happening, but minimap will just be default color instead of crashing
            return nullptr;
        }
        const int index = inactive_index.back();
        inactive_index.pop_back();
        active_index.insert( index );
        i = index;
        return std::move( texture_pool[index] );
    }

    //releases the provided texture back into the inactive pool to be used again
    //called automatically in the submap cache destructor
    void release_tex( int i, SDL_Texture_Ptr ptr ) {
        const auto it = active_index.find( i );
        if( it == active_index.end() ) {
            return;
        }
        inactive_index.push_back( i );
        active_index.erase( i );
        texture_pool[i] = std::move( ptr );
    }
};

struct pixel_minimap::submap_cache {
    //the color stored for each submap tile
    std::array<SDL_Color, SEEX *SEEY> minimap_colors = {};
    //checks if the submap has been looked at by the minimap routine
    bool touched;
    //the texture updates are drawn to
    SDL_Texture_Ptr minimap_tex;
    //the submap being handled
    int texture_index;
    //the list of updates to apply to the texture
    //reduces render target switching to once per submap
    std::vector<point> update_list;
    //flag used to indicate that the texture needs to be cleared before first use
    bool ready;
    shared_texture_pool &pool;

    //reserve the SEEX * SEEY submap tiles
    submap_cache( shared_texture_pool &pool );
    submap_cache( submap_cache && );
    //handle the release of the borrowed texture
    ~submap_cache();
};

pixel_minimap::pixel_minimap( const SDL_Renderer_Ptr &renderer ) :
    renderer( renderer ),
    type( pixel_minimap_type::ortho ),
    cached_center_sm( tripoint_min ),
    screen_rect{ 0, 0, 0, 0 }
{
}

pixel_minimap::~pixel_minimap() = default;

void pixel_minimap::set_type( pixel_minimap_type type )
{
    this->type = type;
    reset();
}

void pixel_minimap::set_settings( const pixel_minimap_settings &settings )
{
    this->settings = settings;
    reset();
}

void pixel_minimap::prepare_cache_for_updates( const tripoint &center )
{
    const auto new_center_sm = g->m.get_abs_sub() + ms_to_sm_copy( center );
    const auto center_sm_diff = cached_center_sm - new_center_sm;

    //invalidate the cache if the game shifted more than one submap in the last update, or if z-level changed.
    if( std::abs( center_sm_diff.x ) > 1 ||
        std::abs( center_sm_diff.y ) > 1 ||
        std::abs( center_sm_diff.z ) > 0 ) {
        cache.clear();
    } else {
        for( auto &mcp : cache ) {
            mcp.second.touched = false;
        }
    }

    cached_center_sm = new_center_sm;
}

//deletes the mapping of unused submap caches from the main map
//the touched flag prevents deletion
void pixel_minimap::clear_unused_cache()
{
    for( auto it = cache.begin(); it != cache.end(); ) {
        it = it->second.touched ? std::next( it ) : cache.erase( it );
    }
}

//draws individual updates to the submap cache texture
//the render target will be set back to display_buffer after all submaps are updated
void pixel_minimap::flush_cache_updates()
{
    for( auto &mcp : cache ) {
        if( mcp.second.update_list.empty() ) {
            continue;
        }

        SetRenderTarget( renderer, mcp.second.minimap_tex );

        if( !mcp.second.ready ) {
            mcp.second.ready = true;

            SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0x00 );
            RenderClear( renderer );

            for( int y = 0; y < SEEY; ++y ) {
                for( int x = 0; x < SEEX; ++x ) {
                    const auto tile_pos = projector->get_tile_pos( { x, y }, { SEEX, SEEY } );
                    const auto tile_size = projector->get_tile_size();

                    const auto rect = SDL_Rect{ tile_pos.x, tile_pos.y, tile_size.x, tile_size.y };

                    render_fill_rect( renderer, rect, 0x00, 0x00, 0x00 );
                }
            }
        }

        for( const point &p : mcp.second.update_list ) {
            const auto tile_pos = projector->get_tile_pos( p, { SEEX, SEEY } );
            const auto tile_color = mcp.second.minimap_colors[p.y * SEEX + p.x];

            if( pixel_size.x == 1 && pixel_size.y == 1 ) {
                SetRenderDrawColor( renderer, tile_color.r, tile_color.g, tile_color.b, tile_color.a );
                RenderDrawPoint( renderer, tile_pos.x, tile_pos.y );
            } else {
                const auto rect = SDL_Rect{ tile_pos.x, tile_pos.y, pixel_size.x, pixel_size.y };
                render_fill_rect( renderer, rect, tile_color.r, tile_color.g, tile_color.b );
            }
        }

        mcp.second.update_list.clear();
    }
}

void pixel_minimap::update_cache_at( const tripoint &sm_pos )
{
    const auto &access_cache = g->m.access_cache( sm_pos.z );
    const bool nv_goggle = g->u.get_vision_modes()[NV_GOGGLES];

    auto &cache_item = get_cache_at( g->m.get_abs_sub() + sm_pos );
    const auto ms_pos = sm_to_ms_copy( sm_pos );

    cache_item.touched = true;

    for( int y = 0; y < SEEY; ++y ) {
        for( int x = 0; x < SEEX; ++x ) {
            const auto p = ms_pos + tripoint{ x, y, 0 };
            const auto lighting = access_cache.visibility_cache[p.x][p.y];

            SDL_Color color;

            if( lighting == LL_BLANK || lighting == LL_DARK ) {
                color = { 0x00, 0x00, 0x00, 0xFF };    // TODO: Map memory?
            } else {
                color = get_map_color_at( p );

                //color terrain according to lighting conditions
                if( nv_goggle ) {
                    if( lighting == LL_LOW ) {
                        color = color_pixel_nightvision( color );
                    } else if( lighting != LL_DARK && lighting != LL_BLANK ) {
                        color = color_pixel_overexposed( color );
                    }
                } else if( lighting == LL_LOW ) {
                    color = color_pixel_grayscale( color );
                }

                color = adjust_color_brightness( color, settings.brightness );
            }

            SDL_Color &current_color = cache_item.minimap_colors[y * SEEX + x];

            if( current_color != color ) {
                current_color = color;
                cache_item.update_list.push_back( { x, y } );
            }
        }
    }
}

pixel_minimap::submap_cache &pixel_minimap::get_cache_at( const tripoint &abs_sm_pos )
{
    auto it = cache.find( abs_sm_pos );

    if( it == cache.end() ) {
        it = cache.emplace( abs_sm_pos, *tex_pool ).first;
    }

    return it->second;
}

void pixel_minimap::process_cache( const tripoint &center )
{
    prepare_cache_for_updates( center );

    for( int y = 0; y < MAPSIZE; ++y ) {
        for( int x = 0; x < MAPSIZE; ++x ) {
            update_cache_at( { x, y, center.z } );
        }
    }

    flush_cache_updates();
    clear_unused_cache();
}

pixel_minimap::submap_cache::submap_cache( shared_texture_pool &pool ) : ready( false ),
    pool( pool )
{
    minimap_tex = pool.request_tex( texture_index );
}

pixel_minimap::submap_cache::~submap_cache()
{
    pool.release_tex( texture_index, std::move( minimap_tex ) );
}

pixel_minimap::submap_cache::submap_cache( submap_cache && ) = default;

void pixel_minimap::set_screen_rect( const SDL_Rect &screen_rect )
{
    if( this->screen_rect == screen_rect && main_tex && tex_pool && projector ) {
        return;
    }

    this->screen_rect = screen_rect;

    projector = create_projector( screen_rect );
    pixel_size = get_pixel_size( projector->get_tile_size(), settings.mode );

    const auto size_on_screen = projector->get_tiles_size( tiles_range );

    if( settings.scale_to_fit ) {
        main_tex_clip_rect = SDL_Rect{ 0, 0, size_on_screen.x, size_on_screen.y };
        screen_clip_rect = fit_rect_inside(
                               SDL_Rect{ 0, 0, size_on_screen.x, size_on_screen.y },
                               screen_rect
                           );

        SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );
        main_tex = create_cache_texture( renderer, size_on_screen.x, size_on_screen.y );
        SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "0" );


    } else {
        const int dx = ( size_on_screen.x - screen_rect.w ) / 2;
        const int dy = ( size_on_screen.y - screen_rect.h ) / 2;

        main_tex_clip_rect = SDL_Rect{
            std::max( dx, 0 ),
            std::max( dy, 0 ),
            size_on_screen.x - 2 * std::max( dx, 0 ),
            size_on_screen.y - 2 * std::max( dy, 0 )
        };

        screen_clip_rect = SDL_Rect{
            screen_rect.x - std::min( dx, 0 ),
            screen_rect.y - std::min( dy, 0 ),
            main_tex_clip_rect.w,
            main_tex_clip_rect.h
        };

        main_tex = create_cache_texture( renderer, size_on_screen.x, size_on_screen.y );
    }

    cache.clear();

    tex_pool = std::make_unique<shared_texture_pool>();

    const auto chunk_size = projector->get_tiles_size( { SEEX, SEEY } );

    for( auto &elem : tex_pool->texture_pool ) {
        elem = create_cache_texture( renderer, chunk_size.x, chunk_size.y );
        SetTextureBlendMode( elem, SDL_BLENDMODE_BLEND );
    }
}

void pixel_minimap::reset()
{
    projector.reset();
    cache.clear();
    main_tex.reset();
    tex_pool.reset();
}

void pixel_minimap::render( const tripoint &center )
{
    SetRenderTarget( renderer, main_tex );

    SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0x00 );
    RenderClear( renderer );

    render_cache( center );
    render_critters( center );

    //set display buffer to main screen
    set_displaybuffer_rendertarget();
    //paint intermediate texture to screen
    RenderCopy( renderer, main_tex, &main_tex_clip_rect, &screen_clip_rect );
}

void pixel_minimap::render_cache( const tripoint &center )
{
    const auto sm_center = g->m.get_abs_sub() + ms_to_sm_copy( center );
    const auto sm_offset = tripoint{
        tiles_range.x / SEEX / 2,
        tiles_range.y / SEEY / 2, 0
    };

    auto ms_offset = center.xy();
    ms_to_sm_remain( ms_offset );
    ms_offset = point{ SEEX / 2, SEEY / 2 } - ms_offset;

    for( const auto &elem : cache ) {
        if( !elem.second.touched ) {
            continue;   // What you gonna do with all that junk?
        }

        const auto rel_pos = elem.first - sm_center;

        if( std::abs( rel_pos.x ) > sm_offset.x + 1 ||
            std::abs( rel_pos.y ) > sm_offset.y + 1 ||
            rel_pos.z != 0 ) {
            continue;
        }

        const auto sm_pos = rel_pos + sm_offset;
        const auto ms_pos = sm_to_ms_copy( sm_pos ) + ms_offset;

        const auto rect = projector->get_chunk_rect( { ms_pos.x, ms_pos.y }, { SEEX, SEEY } );

        RenderCopy( renderer, elem.second.minimap_tex, nullptr, &rect );
    }
}

void pixel_minimap::render_critters( const tripoint &center )
{
    //handles the enemy faction red highlights
    //this value should be divisible by 200
    const int indicator_length = settings.blink_interval * 200; //default is 2000 ms, 2 seconds

    int flicker = 100;
    int mixture = 0;

    if( indicator_length > 0 ) {
        const float t = get_animation_phase( 2 * indicator_length );
        const float s = std::sin( 2 * M_PI * t );

        flicker = lerp_clamped( 25, 100, std::abs( s ) );
        mixture = lerp_clamped( 0, 100, std::max( s, 0.0f ) );
    }

    const auto &access_cache = g->m.access_cache( center.z );

    const int start_x = center.x - tiles_range.x / 2;
    const int start_y = center.y - tiles_range.y / 2;

    for( int y = 0; y < tiles_range.y; y++ ) {
        for( int x = 0; x < tiles_range.x; x++ ) {
            const auto p = tripoint{ start_x + x, start_y + y, center.z };
            const auto lighting = access_cache.visibility_cache[p.x][p.y];

            if( lighting == LL_DARK || lighting == LL_BLANK ) {
                continue;
            }

            const auto critter = g->critter_at( p, true );

            if( critter == nullptr ) {
                continue;
            }

            if( critter != &g->u && !g->u.sees( *critter ) ) {
                continue;
            }

            const auto critter_pos = projector->get_tile_pos( { x, y }, tiles_range );
            const auto critter_rect = SDL_Rect{
                critter_pos.x,
                critter_pos.y,
                projector->get_tile_size().x,
                projector->get_tile_size().y };
            const auto critter_color = get_critter_color( critter, flicker, mixture );

            draw_beacon( critter_rect, critter_color );
        }
    }
}

//the main call for drawing the pixel minimap to the screen
void pixel_minimap::draw( const SDL_Rect &screen_rect, const tripoint &center )
{
    if( !g ) {
        return;
    }

    if( screen_rect.w <= 0 || screen_rect.h <= 0 ) {
        return;
    }

    set_screen_rect( screen_rect );
    process_cache( center );
    render( center );
}

void pixel_minimap::draw_beacon( const SDL_Rect &rect, const SDL_Color &color )
{
    for( int x = -rect.w, x_max = rect.w; x <= x_max; ++x ) {
        for( int y = -rect.h + std::abs( x ), y_max = rect.h - std::abs( x ); y <= y_max; ++y ) {
            const int divisor = 2 * ( std::abs( y ) == rect.h - std::abs( x ) ? 1 : 0 ) + 1;

            SetRenderDrawColor( renderer, color.r / divisor, color.g / divisor, color.b / divisor, 0xFF );
            RenderDrawPoint( renderer, rect.x + x, rect.y + y );
        }
    }
}

std::unique_ptr<pixel_minimap_projector> pixel_minimap::create_projector(
    const SDL_Rect &max_screen_rect )
const
{
    switch( type ) {
        case pixel_minimap_type::ortho:
            return std::unique_ptr<pixel_minimap_projector> {
                new pixel_minimap_ortho_projector( tiles_range, max_screen_rect, settings.square_pixels )
            };

        case pixel_minimap_type::iso:
            return std::unique_ptr<pixel_minimap_projector> {
                new pixel_minimap_iso_projector( tiles_range, max_screen_rect, settings.square_pixels )
            };
    }

    return nullptr;
}

#endif // SDL_TILES
