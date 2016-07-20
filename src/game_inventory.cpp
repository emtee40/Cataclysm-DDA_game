#include "game.h"

#include "cata_utility.h"
#include "inventory_ui.h"
#include "player.h"
#include "item.h"
#include "itype.h"

item_location inv_internal( player &p, const inventory_selector_preset &preset,
                            const std::string &title, int radius,
                            const std::string &none_message )
{
    p.inv.restack( &p );
    p.inv.sort();

    inventory_pick_selector inv_s( p, preset );

    inv_s.set_title( title );
    inv_s.set_display_stats( false );

    inv_s.add_character_items( p );
    inv_s.add_nearby_items( radius );

    if( inv_s.empty() ) {
        const std::string msg = ( none_message.empty() ) ? _( "You don't have the necessary item at hand." )
                                : none_message;
        popup( msg, PF_GET_KEY );
        return item_location();
    }

    return std::move( inv_s.execute() );
}

void game::interactive_inv()
{
    static const std::set<int> allowed_selections = { { ' ', '.', 'q', '=', '\n', KEY_LEFT, KEY_ESCAPE } };

    u.inv.restack( &u );
    u.inv.sort();

    inventory_pick_selector inv_s( u );

    inv_s.add_character_items( u );
    inv_s.set_title( _( "INVENTORY" ) );

    int res;
    do {
        inv_s.set_hint( string_format(
                            _( "Item hotkeys assigned: <color_ltgray>%d</color>/<color_ltgray>%d</color>" ),
                            u.allocated_invlets().size(), inv_chars.size() - u.allocated_invlets().size() ) );
        const item_location &location = inv_s.execute();
        if( location == item_location::nowhere ) {
            break;
        }
        refresh_all();
        res = inventory_item_menu( u.get_item_position( location.get_item() ) );
    } while( allowed_selections.count( res ) != 0 );
}

int game::inv_for_filter( const std::string &title, const item_filter &filter,
                          const std::string &none_message )
{
    return u.get_item_position( inv_map_splice( filter, title, -1, none_message ).get_item() );
}

int game::inv_for_filter( const std::string &title, const item_location_filter &filter,
                          const std::string &none_message )
{
    return u.get_item_position( inv_map_splice( filter, title, -1, none_message ).get_item() );
}

int game::inv_for_all( const std::string &title, const std::string &none_message )
{
    const std::string msg = ( none_message.empty() ) ? _( "Your inventory is empty." ) : none_message;
    return u.get_item_position( inv_internal( u, inventory_selector_preset(),
                                title, -1, none_message ).get_item() );
}

int game::inv_for_activatables( const player &p, const std::string &title )
{
    return inv_for_filter( title, [ &p ]( const item & it ) {
        return p.rate_action_use( it ) != HINT_CANT;
    }, _( "You don't have any items you can use." ) );
}

int game::inv_for_flag( const std::string &flag, const std::string &title )
{
    return inv_for_filter( title, [ &flag ]( const item & it ) {
        return it.has_flag( flag );
    } );
}

int game::inv_for_id( const itype_id &id, const std::string &title )
{
    return inv_for_filter( title, [ &id ]( const item & it ) {
        return it.typeId() == id;
    }, string_format( _( "You don't have a %s." ), item::nname( id ).c_str() ) );
}

int game::inv_for_tools_powered_by( const ammotype &battery_id, const std::string &title )
{
    return inv_for_filter( title, [ &battery_id ]( const item & it ) {
        return it.is_tool() && it.ammo_type() == battery_id;
    }, string_format( _( "You don't have %s-powered tools." ), ammo_name( battery_id ).c_str() ) );
}

int game::inv_for_equipped( const std::string &title )
{
    return inv_for_filter( title, [ this ]( const item & it ) {
        return u.is_worn( it );
    }, _( "You don't wear anything." ) );
}

int game::inv_for_unequipped( const std::string &title )
{
    return inv_for_filter( title, [ this ]( const item & it ) {
        return it.is_armor() && !u.is_worn( it );
    }, _( "You don't have any items to wear." ) );
}

item_location game::inv_map_splice( const item_filter &filter, const std::string &title, int radius,
                                    const std::string &none_message )
{
    return inv_map_splice( [ &filter ]( const item_location & location ) {
        return filter( *location.get_item() );
    }, title, radius, none_message );
}

item_location game::inv_map_splice( const item_location_filter &filter, const std::string &title,
                                    int radius,
                                    const std::string &none_message )
{
    return inv_internal( u, inventory_filter_preset( filter ),
                         title, radius, none_message );
}

class liquid_inventory_preset: public inventory_selector_preset
{
    public:
        liquid_inventory_preset( const item &liquid ) : liquid( liquid ) {
            append_cell( [ this ]( const inventory_entry & entry ) {
                if( is_same_container( entry.get_item() ) ) {
                    return std::string( _( "is the source" ) );
                }

                std::string err;
                const int rem_capacity = get_rem_capacity( entry.get_location(), &err );
                const int total_capacity = entry.get_item().get_total_capacity_for_liquid( this->liquid );

                if( !err.empty() ) {
                    return err;
                }

                return ( entry.get_available_count() > 1 )
                       ? string_format( "%d x %d/%d", entry.get_available_count(), rem_capacity, total_capacity )
                       : string_format( "%d/%d", rem_capacity, total_capacity );
            }, _( "AVAILABLE" ) );
        }

        bool is_shown( const item &it ) const override {
            return it.is_reloadable_with( liquid.typeId() ) || it.is_container();
        }

        bool is_enabled( const item_location &location ) const override {
            return !is_same_container( *location.get_item() ) && get_rem_capacity( location ) > 0;
        }

        int get_rank( const item_location &location ) const override {
            return -get_rem_capacity( location );
        }

    protected:
        bool is_same_container( const item &it ) const {
            return it.is_container() && &it.contents.front() == &liquid;
        }

        int get_rem_capacity( const item_location &location, std::string *err = nullptr ) const {
            if( location.where() == item_location::type::character ) {
                Character *character = dynamic_cast<Character *>( g->critter_at( location.position() ) );
                if( character == nullptr ) {
                    debugmsg( "Supplied an invalid location: no character found." );
                    return 0;
                }
                return location->get_remaining_capacity_for_liquid( liquid, *character, err );
            } else {
                const bool allow_buckets = location.where() == item_location::type::map;
                return location->get_remaining_capacity_for_liquid( liquid, allow_buckets, err );
            }
        }

    private:
        const item &liquid;
};

item *game::inv_map_for_liquid( const item &liquid, const std::string &title, int radius )
{
    return inv_internal( u, liquid_inventory_preset( liquid ), title, radius,
                         string_format( _( "You don't have a suitable container for carrying %s." ),
                                        liquid.tname().c_str() ) ).get_item();
}

class comestible_inventory_preset: public inventory_selector_preset
{
    public:
        comestible_inventory_preset( const player &p ) : p( p ) {
            append_cell( [ this ]( const item & it ) {
                return to_string( this->p.nutrition_for( get_food_type( it ) ) );
            }, _( "NUTRITION" ) );

            append_cell( [ this ]( const item & it ) {
                return to_string( get_comestible( it ).quench );
            }, _( "QUENCH" ) );

            append_cell( [ this ]( const item & it ) {
                return to_string( get_comestible( it ).fun );
            }, _( "  FUN" ) );
        }

        bool is_shown( const item &it ) const override {
            return it.made_of( SOLID ) && get_food_type( it ) != nullptr;
        }

        int get_rank( const item &it ) const override {
            const auto food_type = get_food_type( it );
            const auto &comestible = get_comestible( food_type );
            const int value = p.nutrition_for( food_type ) + comestible.quench + comestible.fun;

            return -value;
        }

    protected:
        // @todo This idiom is quite frequent. De-duplicate.
        const itype *get_food_type( const item &it ) const {
            if( it.is_food( &p ) ) {
                return it.type;
            } else if( it.is_food_container( &p ) && it.contents.front().is_food( &p ) ) {
                return it.contents.front().type;
            } else {
                return nullptr;
            }
        }
        // This function is for robustness. It can prevent potential crashes.
        const islot_comestible &get_comestible( const itype *food_type ) const {
            if( food_type == nullptr || food_type->comestible == nullptr ) {
                static islot_comestible dummy;
                debugmsg( "Supplied an invalid comestible." );
                return dummy;
            }
            return *food_type->comestible;
        }

        const islot_comestible &get_comestible( const item &it ) const {
            return get_comestible( get_food_type( it ) );
        }

    private:
        const player &p;
};

item_location game::inv_for_comestibles( const std::string &title )
{
    return inv_internal( u, comestible_inventory_preset( u ),
                         title, 1, _( "You have nothing to consume." ) );
}

class gunmod_inventory_preset: public inventory_selector_preset
{
    public:
        gunmod_inventory_preset( const item &gunmod ) : gunmod( gunmod ) {
            append_cell( [ this ]( const item & it ) {
                std::string err;
                it.gunmod_compatible( this->gunmod, &err );
                return err.empty() ? string_format( "<color_ltgreen>%s</color>", _( "yes" ) ) : err;
            }, _( "MODIFIABLE" ) );
            // @todo Display rolls
        }

        bool is_shown( const item &it ) const override {
            return it.is_gun() && !it.is_gunmod();
        }

        bool is_enabled( const item &it ) const override {
            return it.gunmod_compatible( gunmod );
        }

    private:
        const item &gunmod;
};

item_location game::inv_for_gunmod( const item &gunmod, const std::string &title )
{
    return inv_internal( u, gunmod_inventory_preset( gunmod ),
                         title, -1, _( "You don't have any guns." ) );
}

class drop_inventory_preset: public inventory_selector_preset
{
    public:
        drop_inventory_preset( const player &p ) : p( p ) {
            append_cell( [ this ]( const item & it ) {
                return to_string( it.volume() );
            }, _( "VOLUME" ) );

            append_cell( [ this ]( const item & it ) {
                return string_format( "%.1f", convert_weight( it.weight() ) );
            }, string_format( _( "WEIGHT(%s)" ), to_upper_case( weight_units() ).c_str() ) );

            append_cell( [ this ]( const item & it ) {
                return to_string( it.get_storage() ); //std::to_string
            }, _( "STORAGE" ) );
        }

        bool is_enabled( const item &it ) const override {
            return p.can_unwield( it, false );
        }

        int get_rank( const item &it ) const override {
            return -it.volume();
        }

    private:
        const player &p;
};

std::list<std::pair<int, int>> game::multidrop()
{
    u.inv.restack( &u );
    u.inv.sort();

    const drop_inventory_preset preset( u );
    inventory_drop_selector inv_s( u, preset );

    inv_s.add_character_items( u );
    inv_s.set_title( _( "MULTIDROP" ) );
    inv_s.set_hint( "To drop x items, type a number before selecting." );

    if( inv_s.empty() ) {
        popup( std::string( _( "You have nothing to drop." ) ), PF_GET_KEY );
        return std::list<std::pair<int, int> >();
    }
    return inv_s.execute();
}

void game::compare( const tripoint &offset )
{
    u.inv.restack( &u );
    u.inv.sort();

    inventory_compare_selector inv_s( u );

    inv_s.add_character_items( u );
    inv_s.set_title( _( "COMPARE" ) );
    inv_s.set_hint( "Select two items to compare them." );

    if( offset != tripoint_min ) {
        inv_s.add_map_items( u.pos() + offset );
        inv_s.add_vehicle_items( u.pos() + offset );
    } else {
        inv_s.add_nearby_items();
    }

    if( inv_s.empty() ) {
        popup( std::string( _( "There are no items to compare." ) ), PF_GET_KEY );
        return;
    }

    do {
        const auto to_compare = inv_s.execute();

        if( to_compare.first == nullptr || to_compare.second == nullptr ) {
            break;
        }

        std::vector<iteminfo> vItemLastCh, vItemCh;
        std::string sItemLastCh, sItemCh, sItemLastTn, sItemTn;

        to_compare.first->info( true, vItemCh );
        sItemCh = to_compare.first->tname();
        sItemTn = to_compare.first->type_name();

        to_compare.second->info( true, vItemLastCh );
        sItemLastCh = to_compare.second->tname();
        sItemLastTn = to_compare.second->type_name();

        int iScrollPos = 0;
        int iScrollPosLast = 0;
        int ch = ( int ) ' ';

        do {
            draw_item_info( 0, ( TERMX - VIEW_OFFSET_X * 2 ) / 2, 0, TERMY - VIEW_OFFSET_Y * 2,
                            sItemLastCh, sItemLastTn, vItemLastCh, vItemCh, iScrollPosLast, true ); //without getch(
            ch = draw_item_info( ( TERMX - VIEW_OFFSET_X * 2 ) / 2, ( TERMX - VIEW_OFFSET_X * 2 ) / 2,
                                 0, TERMY - VIEW_OFFSET_Y * 2, sItemCh, sItemTn, vItemCh, vItemLastCh, iScrollPos );

            if( ch == KEY_PPAGE ) {
                iScrollPos--;
                iScrollPosLast--;
            } else if( ch == KEY_NPAGE ) {
                iScrollPos++;
                iScrollPosLast++;
            }
        } while( ch == KEY_PPAGE || ch == KEY_NPAGE );
    } while( true );
}
