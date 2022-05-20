#include "npctrade_utils.h"

#include <list>
#include <map>

#include "calendar.h"
#include "clzones.h"
#include "npc.h"
#include "rng.h"
#include "vehicle.h"
#include "vpart_position.h"

static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );

namespace
{
using consume_cache = std::map<itype_id, int>;
using consume_queue = std::vector<item_location>;
using dest_t = std::vector<tripoint_abs_ms>;

void _consume_item( item_location elem, consume_queue &consumed, consume_cache &cache, npc &guy,
                    int amount )
{
    if( !elem->is_owned_by( guy ) ) {
        return;
    }
    std::list<item *> const contents = elem->all_items_top( item_pocket::pocket_type::CONTAINER );
    if( contents.empty() ) {
        auto it = cache.find( elem->typeId() );
        if( it == cache.end() ) {
            it = cache.emplace( elem->typeId(), amount ).first;
        }
        if( it->second != 0 ) {
            consumed.push_back( elem );
            it->second--;
        }
    } else {
        for( item *it : contents ) {
            _consume_item( item_location( elem, it ), consumed, cache, guy, amount );
        }
    }
}

dest_t _get_shuffled_point_set( std::unordered_set<tripoint_abs_ms> const &set )
{
    dest_t ret;
    std::copy( set.begin(), set.end(), std::back_inserter( ret ) );
    std::shuffle( ret.begin(), ret.end(), rng_get_engine() );
    return ret;
}

bool _to_map( item const &it, map &here, tripoint const &dpoint_here )
{
    bool leftover = true;
    if( here.can_put_items_ter_furn( dpoint_here ) and
        here.free_volume( dpoint_here ) >= it.volume() ) {
        here.add_item_or_charges( dpoint_here, it, false );
        leftover = false;
    }

    return leftover;
}

bool _to_veh( item const &it, cata::optional<vpart_reference> const vp )
{
    bool leftover = true;
    int const part = static_cast<int>( vp->part_index() );
    if( vp->vehicle().free_volume( part ) >= it.volume() ) {
        vp->vehicle().add_item( part, it );
        leftover = false;
    }
    return leftover;
}

} // namespace

void add_fallback_zone( npc &guy )
{
    zone_manager &zmgr = zone_manager::get_manager();
    tripoint_abs_ms const loc = guy.get_location();
    faction_id const &fac_id = guy.get_fac_id();

    if( !zmgr.has_near( zone_type_LOOT_UNSORTED, loc, PICKUP_RANGE, fac_id ) ) {
        zmgr.add( fallback_name, zone_type_LOOT_UNSORTED, fac_id, false,
                  true, loc.raw() + tripoint_north_west, loc.raw() + tripoint_south_east );
        DebugLog( DebugLevel::D_WARNING, DebugClass::D_GAME )
                << "Added a fallack loot zone for NPC trader " << guy.name;
    }
}

std::list<item> distribute_items_to_npc_zones( std::list<item> &items, npc &guy )
{
    zone_manager &zmgr = zone_manager::get_manager();
    map &here = get_map();
    tripoint_abs_ms const loc_abs = guy.get_location();
    faction_id const &fac_id = guy.get_fac_id();

    std::list<item> leftovers;
    dest_t const fallback = _get_shuffled_point_set(
                                zmgr.get_near( zone_type_LOOT_UNSORTED, loc_abs, PICKUP_RANGE, nullptr, fac_id ) );
    for( item const &it : items ) {
        zone_type_id const zid =
            zmgr.get_near_zone_type_for_item( it, loc_abs, PICKUP_RANGE, fac_id );

        dest_t dest = zid.is_valid() ? _get_shuffled_point_set( zmgr.get_near(
                          zid, loc_abs, PICKUP_RANGE, &it, fac_id ) )
                      : dest_t();
        std::copy( fallback.begin(), fallback.end(), std::back_inserter( dest ) );

        bool leftover = true;
        for( tripoint_abs_ms const &dpoint : dest ) {
            tripoint const dpoint_here = here.getlocal( dpoint );
            cata::optional<vpart_reference> const vp =
                here.veh_at( dpoint_here ).part_with_feature( "CARGO", false );
            if( vp and vp->vehicle().get_owner() == fac_id ) {
                leftover = _to_veh( it, vp );
            } else {
                leftover = _to_map( it, here, dpoint_here );
            }
            if( !leftover ) {
                break;
            }
        }
        if( leftover ) {
            leftovers.emplace_back( it );
        }
    }

    return leftovers;
}

void consume_items_in_zones( npc &guy, time_duration const &elapsed )
{
    std::unordered_set<tripoint> const src = zone_manager::get_manager().get_point_set_loot(
                guy.get_location(), PICKUP_RANGE, guy.get_fac_id() );

    consume_cache cache;
    map &here = get_map();
    int constexpr rate = 5; // FIXME: jsonize

    for( tripoint const &pt : src ) {
        consume_queue consumed;
        std::list<item_location> stack =
        here.items_with( pt, [&guy]( item const & it ) {
            return it.is_owned_by( guy );
        } );
        for( item_location &elem : stack ) {
            _consume_item( elem, consumed, cache, guy, rate * to_days<int>( elapsed ) );
        }
        for( item_location &it : consumed ) {
            it.remove_item();
        }
    }
}
