#include "magic.h"

#include <set>

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "creature.h"
#include "enums.h"
#include "game.h"
#include "item.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "player.h"
#include "projectile.h"
#include "type_id.h"

static tripoint random_point( int min_distance, int max_distance, const tripoint &player_pos )
{
    const int angle = rng( 0, 360 );
    const int dist = rng( min_distance, max_distance );
    const int x = round( dist * cos( angle ) );
    const int y = round( dist * sin( angle ) );
    return tripoint( x + player_pos.x, y + player_pos.y, player_pos.z );
}

void spell_effect::teleport( int min_distance, int max_distance )
{
    if( min_distance > max_distance || min_distance < 0 || max_distance < 0 ) {
        debugmsg( "ERROR: Teleport argument(s) invalid" );
        return;
    }
    const tripoint player_pos = g->u.pos();
    tripoint target;
    // limit the loop just in case it's impossble to find a valid point in the range
    int tries = 0;
    do {
        target = random_point( min_distance, max_distance, player_pos );
        tries++;
    } while( g->m.impassable( target ) && tries < 20 );
    if( tries == 20 ) {
        add_msg( m_bad, _( "Unable to find a valid target for teleport." ) );
        return;
    }
    g->place_player( target );
}

void spell_effect::pain_split()
{
    player &p = g->u;
    add_msg( m_info, _( "Your injuries even out." ) );
    int num_limbs = 0; // number of limbs effected (broken don't count)
    int total_hp = 0; // total hp among limbs

    for( const int &part : p.hp_cur ) {
        if( part != 0 ) {
            num_limbs++;
            total_hp += part;
        }
    }
    for( int &part : p.hp_cur ) {
        const int hp_each = total_hp / num_limbs;
        if( part != 0 ) {
            part = hp_each;
        }
    }
}

void spell_effect::move_earth( const tripoint &target )
{
    ter_id ter_here = g->m.ter( target );

    std::set<ter_id> empty_air = { t_hole };
    std::set<ter_id> deep_pit = { t_pit, t_slope_down };
    std::set<ter_id> shallow_pit = { t_pit_corpsed, t_pit_covered, t_pit_glass, t_pit_glass_covered, t_pit_shallow, t_pit_spiked, t_pit_spiked_covered, t_rootcellar };
    std::set<ter_id> soft_dirt = { t_grave, t_dirt, t_sand, t_clay, t_dirtmound, t_grass, t_grass_long, t_grass_tall, t_grass_golf, t_grass_dead, t_grass_white, t_dirtfloor, t_fungus_floor_in, t_fungus_floor_sup, t_fungus_floor_out, t_sandbox };
    // rock: can still be dug through with patience, converts to sand upon completion
    std::set<ter_id> hard_dirt = { t_pavement, t_pavement_y, t_sidewalk, t_concrete, t_thconc_floor, t_thconc_floor_olight, t_strconc_floor, t_floor, t_floor_waxed, t_carpet_red, t_carpet_yellow, t_carpet_purple, t_carpet_green, t_linoleum_white, t_linoleum_gray, t_slope_up, t_rock_red, t_rock_green, t_rock_blue, t_floor_red, t_floor_green, t_floor_blue, t_pavement_bg_dp, t_pavement_y_bg_dp, t_sidewalk_bg_dp };

    if( empty_air.count( ter_here ) == 1 ) {
        add_msg( m_bad, _( "All the dust in the air here falls to the ground." ) );
    } else if( deep_pit.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_hole );
        add_msg( _( "The pit has deepened further." ) );
    } else if( shallow_pit.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_pit );
        add_msg( _( "More debris shifts out of the pit." ) );
    } else if( soft_dirt.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_pit_shallow );
        add_msg( _( "The earth moves out of the way for you." ) );
    } else if( hard_dirt.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_sand );
        add_msg( _( "The rocks here are ground into sand." ) );
    } else {
        add_msg( m_bad, _( "The earth here does not listen to your command to move." ) );
    }
}

static bool in_spell_aoe( const tripoint &start, const tripoint &end, const int &radius,
                          const bool ignore_walls )
{
    if( rl_dist( start, end ) > radius ) {
        return false;
    }
    if( ignore_walls ) {
        return true;
    }
    const std::vector<tripoint> trajectory = line_to( start, end );
    for( const tripoint &pt : trajectory ) {
        if( g->m.impassable( pt ) ) {
            return false;
        }
    }
    return true;
}

std::set<tripoint> spell_effect::spell_effect_blast( const spell &, const tripoint &,
        const tripoint &target, const int aoe_radius, const bool ignore_walls )
{
    std::set<tripoint> targets;
    // TODO: Make this breadth-first
    for( int x = target.x - aoe_radius; x <= target.x + aoe_radius; x++ ) {
        for( int y = target.y - aoe_radius; y <= target.y + aoe_radius; y++ ) {
            const tripoint potential_target( x, y, target.z );
            if( in_spell_aoe( target, potential_target, aoe_radius, ignore_walls ) ) {
                targets.emplace( potential_target );
            }
        }
    }
    return targets;
}

std::set<tripoint> spell_effect::spell_effect_cone( const spell &sp, const tripoint &source,
        const tripoint &target, const int aoe_radius, const bool ignore_walls )
{
    std::set<tripoint> targets;
    // cones go all the way to end (if they don't hit an obstacle)
    const int range = sp.range() + 1;
    const int initial_angle = coord_to_angle( source, target );
    std::set<tripoint> end_points;
    for( int angle = initial_angle - floor( aoe_radius / 2.0 );
         angle <= initial_angle + ceil( aoe_radius / 2.0 ); angle++ ) {
        tripoint potential;
        calc_ray_end( angle, range, source, potential );
        end_points.emplace( potential );
    }
    for( const tripoint &ep : end_points ) {
        std::vector<tripoint> trajectory = line_to( source, ep );
        for( const tripoint &tp : trajectory ) {
            if( ignore_walls || g->m.passable( tp ) ) {
                targets.emplace( tp );
            } else {
                break;
            }
        }
    }
    // we don't want to hit ourselves in the blast!
    targets.erase( source );
    return targets;
}

struct matrix22_i {
    int x0, y0, x1, y1;
};

struct ray {
    point delta;
    point origin;
    int steps;
};

static int get_octant( const point &p )
{
    int ax = p.x * ( p.x > 0 ? 1 : -1 );
    int ay = p.y * ( p.y > 0 ? 1 : -1 );

    /*
    A = ax >= ay
    B = ax == p.x
    C = ay == p.y
    A | B | C | RES
    ---------------
    T | T | T | 0
    T | T | F | 7
    T | F | T | 3
    T | F | F | 4
    F | T | T | 1
    F | T | F | 6
    F | F | T | 2
    F | F | F | 5
    */
    int A = ( ax >= ay ) << 2;
    int B = ( ax == p.x ) << 1;
    int C = ( ay == p.y ) << 0;

    int n = A | B | C;

    static const int results[8] = {
        5,
        2,
        6,
        1,
        4,
        3,
        7,
        0
    };
    return results[n];
}

static const matrix22_i &matrix_to( int to_octant )
{
    static const matrix22_i matrices[8] = {
        { 1, 0, 0, 1 },
        { 0, 1, 1, 0 },
        { 0, -1, 1, 0 },
        {-1, 0, 0, 1 },
        {-1, 0, 0, -1 },
        { 0, -1, -1, 0 },
        { 0, 1, -1, 0 },
        { 1, 0, 0, -1 }
    };
    return matrices[ to_octant ];
}
static const matrix22_i &matrix_from( int octant )
{
    static const int matrices[8] = {
        0, 1, 6, 3, 4, 5, 2, 7
    };
    return matrix_to( matrices[octant] );
}
static point xform( const matrix22_i &mat, const point &p )
{
    return {
        p.x *mat.x0 + p.y * mat.x1,
        p.x *mat.y0 + p.y *mat.y1
    };
}

static point ray_at( const ray &r, int step )
{
    if( r.steps == 0 || step == 0 ) {
        return r.origin;
    }

    point result = r.origin;

    int mul_steps = ( step > 0 ? ( step / r.steps ) : ( ( step - r.steps + 1 ) / r.steps ) );
    int mod_steps = step - mul_steps * r.steps;

    result.x += mul_steps * r.delta.x;
    result.y += mul_steps * r.delta.y;

    // Figure out what grid point we are along the line
    int last_dy = 0, leftover = 0;
    for( int i = 0; i < mod_steps; ++i ) {
        leftover += r.delta.y;
        if( leftover >= r.delta.x ) {
            ++last_dy;
            leftover -= r.delta.x;
        }
    }

    result.x += mod_steps;
    result.y += last_dy;

    return result;
}
std::set<tripoint> spell_effect::spell_effect_line( const spell &, const tripoint &source,
        const tripoint &target, const int aoe_radius, const bool ignore_walls )
{
    const tripoint delta3 = target - source;
    const point delta( delta3.x, delta3.y );
    // Get tile distance between source and target, if zero exit early to prevent
    // an infinite loop later.
    const int distance = square_dist( source.x, source.y, target.x, target.y );
    if( distance == 0 ) {
        return std::set<tripoint>();
    }

    // Octant used to determine matrix transforms to and from octant zero,
    // defined as octant between y=0, y=x, and x>0
    int octant = get_octant( delta );
    const matrix22_i &to_zero = matrix_to( octant );
    const matrix22_i &from_zero = matrix_from( octant );
    // Transform 'delta' direction vector to octant zero
    const point ray_diff = xform( to_zero, delta );
    ray o_ray{ ray_diff, point( 0, 0 ), ray_diff.x };

    struct ray_set {
        ray r;
        int s;
    };

    // We will have a ray extending from the source, and 'aoe_radius' rays
    // extending from the relative left and right of the source.
    std::vector<ray_set> rays;
    rays.reserve( aoe_radius + 1 );
    // calculate line perpendicular to direction we're targetting
    point perp( -ray_diff.y, ray_diff.x );
    // Orientation of point C relative to line AB
    auto side_of = []( const point & a, const point & b, const point & c ) {
        int cross = ( ( b.x - a.x ) * ( c.y - a.y ) - ( b.y - a.y ) * ( c.x - a.x ) );
        return ( cross > 0 ) - ( cross < 0 );
    };
    // side that the target is on
    int side = side_of( point( 0, 0 ), perp, ray_diff );
    int nside = side * -1; // side opposite the target

    int left = aoe_radius / 2;
    int right = aoe_radius - left; // effectively ceil( aoe_radius / 2.0 )

    // Due to matrix transformations, odd numbered origin octants require
    // swapping the left and right leg-lengths
    if( octant % 2 == 1 ) {
        std::swap( left, right );
    }

    auto test = []( bool ignore_walls, const tripoint & src, const point & p ) -> bool {
        return ignore_walls || g->m.passable( src + tripoint( p, 0 ) );
    };

    // Build left leg from just beyond origin to extant
    for( int i = -1; i >= -left; --i ) {
        // translate origin ray towards left side
        ray_set rs;
        rs.r = o_ray;
        rs.r.origin.y = i;
        rs.s = 0;
        // walk translated ray forward while not on the same side as target, nor coincident
        // with perpendicular ray
        while( nside == side_of( point( 0, 0 ), perp, ray_at( rs.r, rs.s ) ) ) {
            ++rs.s;
        }
        // Test if the point is blocking. If it is blocking do not add it to the ray set,
        // also stop processing this leg.
        // Transform the ray point from octant zero into originating octant
        if( !test( ignore_walls, source, xform( from_zero, ray_at( rs.r, rs.s ) ) ) ) {
            break;
        }
        rays.push_back( rs );
    }
    // add ray pointing from origin to delta
    rays.push_back( { o_ray, 0 } );
    // repeat building process for right leg
    for( int i = 1; i <= right; ++i ) {
        ray_set rs;
        rs.r = o_ray;
        rs.r.origin.y = i;
        rs.s = -1;

        // Walk translated ray backward while on the same side as target, or coincident
        // with perpendicular ray
        while( nside != side_of( point( 0, 0 ), perp, ray_at( rs.r, rs.s ) ) ) {
            --rs.s;
        }
        // move back to coincident or on same side
        ++rs.s;
        // Test if the point is blocking. If it is blocking do not add it to the ray set,
        // also stop processing this leg.
        // Transform the ray point from octant zero into originating octant
        if( !test( ignore_walls, source, xform( from_zero, ray_at( rs.r, rs.s ) ) ) ) {
            break;
        }
        rays.push_back( rs );
    }

    std::set<tripoint> result;
    for( const ray_set &rs : rays ) {
        // ensure that the ray's origin is able to be used
        if( test( ignore_walls, source, xform( from_zero, ray_at( rs.r, rs.s ) ) ) ) {
            // put first point immediately as it has already been tested
            result.emplace( source + tripoint( xform( from_zero, ray_at( rs.r, rs.s ) ), 0 ) );

            // iterate over remaining points
            for( int i = 1; i <= distance; ++i ) {
                point p = xform( from_zero, ray_at( rs.r, rs.s + i ) );
                // test point, add if able to continue, otherwise stop processing this ray.
                if( test( ignore_walls, source, p ) ) {
                    result.emplace( source + tripoint( p, 0 ) );
                } else {
                    break;
                }
            }
        }
    }
    // remove source point and return
    result.erase( source );
    return result;
}
// spells do not reduce in damage the further away from the epicenter the targets are
// rather they do their full damage in the entire area of effect
static std::set<tripoint> spell_effect_area( const spell &sp, const tripoint &source,
        const tripoint &target,
        std::function<std::set<tripoint>( const spell &, const tripoint &, const tripoint &, const int, const bool )>
        aoe_func, bool ignore_walls = false )
{
    std::set<tripoint> targets = { target }; // initialize with epicenter
    if( sp.aoe() <= 1 ) {
        return targets;
    }

    const int aoe_radius = sp.aoe();
    targets = aoe_func( sp, source, target, aoe_radius, ignore_walls );

    for( const tripoint &p : targets ) {
        if( !sp.is_valid_target( p ) ) {
            targets.erase( p );
        }
    }

    // Draw the explosion
    std::map<tripoint, nc_color> explosion_colors;
    for( auto &pt : targets ) {
        explosion_colors[pt] = sp.damage_type_color();
    }

    explosion_handler::draw_custom_explosion( g->u.pos(), explosion_colors );
    return targets;
}

static void add_effect_to_target( const tripoint &target, const spell &sp )
{
    const int dur_moves = sp.duration();
    const time_duration dur_td = 1_turns * dur_moves / 100;

    Creature *const critter = g->critter_at<Creature>( target );
    Character *const guy = g->critter_at<Character>( target );
    efftype_id spell_effect( sp.effect_data() );
    bool bodypart_effected = false;

    if( guy ) {
        for( const body_part bp : all_body_parts ) {
            if( sp.bp_is_affected( bp ) ) {
                guy->add_effect( spell_effect, dur_td, bp, sp.has_flag( spell_flag::PERMANENT ) );
                bodypart_effected = true;
            }
        }
    }
    if( !bodypart_effected ) {
        critter->add_effect( spell_effect, dur_td, num_bp );
    }
}

static void damage_targets( const spell &sp, const std::set<tripoint> &targets )
{
    for( const tripoint &target : targets ) {
        if( !sp.is_valid_target( target ) ) {
            continue;
        }
        sp.make_sound( target );
        Creature *const cr = g->critter_at<Creature>( target );
        if( !cr ) {
            continue;
        }

        projectile bolt;
        bolt.impact = sp.get_damage_instance();
        bolt.proj_effects.emplace( "magic" );

        dealt_projectile_attack atk;
        atk.end_point = target;
        atk.hit_critter = cr;
        atk.proj = bolt;
        if( !sp.effect_data().empty() ) {
            add_effect_to_target( target, sp );
        }
        if( sp.damage() > 0 ) {
            cr->deal_projectile_attack( &g->u, atk, true );
        } else if( sp.damage() < 0 ) {
            sp.heal( target );
            add_msg( m_good, _( "%s wounds are closing up!" ), cr->disp_name( true ) );
        }
    }
}

void spell_effect::projectile_attack( const spell &sp, const tripoint &source,
                                      const tripoint &target )
{
    std::vector<tripoint> trajectory = line_to( source, target );
    for( std::vector<tripoint>::iterator iter = trajectory.begin(); iter != trajectory.end(); iter++ ) {
        if( g->m.impassable( *iter ) ) {
            if( iter != trajectory.begin() ) {
                target_attack( sp, source, *( iter - 1 ) );
            } else {
                target_attack( sp, source, *iter );
            }
            return;
        }
    }
    target_attack( sp, source, trajectory.back() );
}

void spell_effect::target_attack( const spell &sp, const tripoint &source,
                                  const tripoint &epicenter )
{
    damage_targets( sp, spell_effect_area( sp, source, epicenter, spell_effect_blast,
                                           sp.has_flag( spell_flag::IGNORE_WALLS ) ) );
}

void spell_effect::cone_attack( const spell &sp, const tripoint &source, const tripoint &target )
{
    damage_targets( sp, spell_effect_area( sp, source, target, spell_effect_cone,
                                           sp.has_flag( spell_flag::IGNORE_WALLS ) ) );
}

void spell_effect::line_attack( const spell &sp, const tripoint &source, const tripoint &target )
{
    damage_targets( sp, spell_effect_area( sp, source, target, spell_effect_line,
                                           sp.has_flag( spell_flag::IGNORE_WALLS ) ) );
}

void spell_effect::spawn_ethereal_item( spell &sp )
{
    item granted( sp.effect_data(), calendar::turn );
    if( !granted.is_comestible() && !( sp.has_flag( spell_flag::PERMANENT ) && sp.is_max_level() ) ) {
        granted.set_var( "ethereal", to_turns<int>( sp.duration_turns() ) );
        granted.set_flag( "ETHEREAL_ITEM" );
    }
    if( granted.count_by_charges() && sp.damage() > 0 ) {
        granted.charges = sp.damage();
    }
    if( g->u.can_wear( granted ).success() ) {
        granted.set_flag( "FIT" );
        g->u.wear_item( granted, false );
    } else if( !g->u.is_armed() ) {
        g->u.weapon = granted;
    } else {
        g->u.i_add( granted );
    }
    if( !granted.count_by_charges() ) {
        for( int i = 1; i < sp.damage(); i++ ) {
            g->u.i_add( granted );
        }
    }
}

void spell_effect::recover_energy( spell &sp, const tripoint &target )
{
    // this spell is not appropriate for healing
    const int healing = sp.damage();
    const std::string energy_source = sp.effect_data();
    // TODO: Change to Character
    // current limitation is that Character does not have stamina or power_level members
    player *p = g->critter_at<player>( target );

    if( energy_source == "MANA" ) {
        p->magic.mod_mana( *p, healing );
    } else if( energy_source == "STAMINA" ) {
        p->mod_stat( "stamina", healing );
    } else if( energy_source == "FATIGUE" ) {
        // fatigue is backwards
        p->mod_fatigue( -healing );
    } else if( energy_source == "BIONIC" ) {
        if( healing > 0 ) {
            p->power_level = std::min( p->max_power_level, p->power_level + healing );
        } else {
            p->mod_stat( "stamina", healing );
        }
    } else if( energy_source == "PAIN" ) {
        // pain is backwards
        p->mod_pain_noresist( -healing );
    } else if( energy_source == "HEALTH" ) {
        p->mod_healthy( healing );
    } else {
        debugmsg( "Invalid effect_str %s for spell %s", energy_source, sp.name() );
    }
}

static bool is_summon_friendly( const spell &sp )
{
    const bool hostile = sp.has_flag( spell_flag::HOSTILE_SUMMON );
    bool friendly = !hostile;
    if( sp.has_flag( spell_flag::HOSTILE_50 ) ) {
        friendly = friendly && rng( 0, 1000 ) < 500;
    }
    return friendly;
}

static bool add_summoned_mon( const mtype_id &id, const tripoint &pos, const time_duration &time,
                              const spell &sp )
{
    const bool permanent = sp.has_flag( spell_flag::PERMANENT );
    monster spawned_mon( id, pos );
    if( is_summon_friendly( sp ) ) {
        spawned_mon.friendly = INT_MAX;
    } else {
        spawned_mon.friendly = 0;
    }
    if( !permanent ) {
        spawned_mon.set_summon_time( time );
    }
    spawned_mon.no_extra_death_drops = true;
    return g->add_zombie( spawned_mon );
}

void spell_effect::spawn_summoned_monster( spell &sp, const tripoint &source,
        const tripoint &target )
{
    const mtype_id mon_id( sp.effect_data() );
    std::set<tripoint> area = spell_effect_area( sp, source, target, spell_effect_blast );
    // this should never be negative, but this'll keep problems from happening
    size_t num_mons = abs( sp.damage() );
    const time_duration summon_time = sp.duration_turns();
    while( num_mons > 0 && area.size() > 0 ) {
        const size_t mon_spot = rng( 0, area.size() - 1 );
        auto iter = area.begin();
        std::advance( iter, mon_spot );
        if( add_summoned_mon( mon_id, *iter, summon_time, sp ) ) {
            num_mons--;
            sp.make_sound( *iter );
        } else {
            add_msg( m_bad, "failed to place monster" );
        }
        // whether or not we succeed in spawning a monster, we don't want to try this tripoint again
        area.erase( iter );
    }
}

void spell_effect::translocate( spell &sp, const tripoint &source, const tripoint &target,
                                teleporter_list &tp_list )
{
    tp_list.translocate( spell_effect_area( sp, source, target, spell_effect_blast, true ) );
}
