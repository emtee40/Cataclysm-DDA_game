#include "morale.h"

#include "cata_utility.h"
#include "debug.h"
#include "itype.h"
#include "output.h"
#include "bodypart.h"
#include "player.h"
#include "catacharset.h"

#include <stdlib.h>
#include <algorithm>

const efftype_id effect_took_prozac( "took_prozac" );
const efftype_id effect_took_xanax( "took_xanax" );

namespace
{
static const std::string item_name_placeholder = "%i"; // Used to address an item name

const std::string &get_morale_data( const morale_type id )
{
    static const std::array<std::string, NUM_MORALE_TYPES> morale_data = { {
            { "This is a bug (player.cpp:moraledata)" },
            { _( "Enjoyed %i" ) },
            { _( "Enjoyed a hot meal" ) },
            { _( "Music" ) },
            { _( "Enjoyed honey" ) },
            { _( "Played Video Game" ) },
            { _( "Marloss Bliss" ) },
            { _( "Mutagenic Anticipation" ) },
            { _( "Good Feeling" ) },
            { _( "Supported" ) },
            { _( "Looked at photos" ) },

            { _( "Nicotine Craving" ) },
            { _( "Caffeine Craving" ) },
            { _( "Alcohol Craving" ) },
            { _( "Opiate Craving" ) },
            { _( "Speed Craving" ) },
            { _( "Cocaine Craving" ) },
            { _( "Crack Cocaine Craving" ) },
            { _( "Mutagen Craving" ) },
            { _( "Diazepam Craving" ) },
            { _( "Marloss Craving" ) },

            { _( "Disliked %i" ) },
            { _( "Ate Human Flesh" ) },
            { _( "Ate Meat" ) },
            { _( "Ate Vegetables" ) },
            { _( "Ate Fruit" ) },
            { _( "Lactose Intolerance" ) },
            { _( "Ate Junk Food" ) },
            { _( "Wheat Allergy" ) },
            { _( "Ate Indigestible Food" ) },
            { _( "Wet" ) },
            { _( "Dried Off" ) },
            { _( "Cold" ) },
            { _( "Hot" ) },
            { _( "Bad Feeling" ) },
            { _( "Killed Innocent" ) },
            { _( "Killed Friend" ) },
            { _( "Guilty about Killing" ) },
            { _( "Guilty about Mutilating Corpse" ) },
            { _( "Fey Mutation" ) },
            { _( "Chimerical Mutation" ) },
            { _( "Mutation" ) },

            { _( "Moodswing" ) },
            { _( "Read %i" ) },
            { _( "Got comfy" ) },

            { _( "Heard Disturbing Scream" ) },

            { _( "Masochism" ) },
            { _( "Hoarder" ) },
            { _( "Stylish" ) },
            { _( "Optimist" ) },
            { _( "Bad Tempered" ) },
            //~ You really don't like wearing the Uncomfy Gear
            { _( "Uncomfy Gear" ) },
            { _( "Found kitten <3" ) },

            { _( "Got a Haircut" ) },
            { _( "Freshly Shaven" ) },
        }
    };

    if( static_cast<size_t>( id ) >= morale_data.size() ) {
        debugmsg( "invalid morale type: %d", id );
        return morale_data[0];
    }

    return morale_data[id];
}
} // namespace

std::string morale_point::get_name() const
{
    std::string name = get_morale_data( type );

    if( item_type != nullptr ) {
        name = string_replace( name, item_name_placeholder, item_type->nname( 1 ) );
    } else if( name.find( item_name_placeholder ) != std::string::npos ) {
        debugmsg( "%s(): Morale #%d (%s) requires item_type to be specified.", __FUNCTION__, type,
                  name.c_str() );
    }

    return name;
}

int morale_point::get_net_bonus() const
{
    return bonus * ( ( age > decay_start ) ? logarithmic_range( decay_start, duration, age ) : 1 );
}

int morale_point::get_net_bonus( const morale_mult &mult ) const
{
    return get_net_bonus() * mult;
}

bool morale_point::is_expired() const
{
    return age >= duration; // Will show zero morale bonus
}

void morale_point::add( int new_bonus, int new_max_bonus, int new_duration, int new_decay_start,
                        bool new_cap )
{
    if( new_cap ) {
        duration = new_duration;
        decay_start = new_decay_start;
    } else {
        bool same_sign = ( bonus > 0 ) == ( new_max_bonus > 0 );

        duration = pick_time( duration, new_duration, same_sign );
        decay_start = pick_time( decay_start, new_decay_start, same_sign );
    }

    bonus = get_net_bonus() + new_bonus;

    if( abs( bonus ) > abs( new_max_bonus ) && ( new_max_bonus != 0 || new_cap ) ) {
        bonus = new_max_bonus;
    }

    age = 0; // Brand new. The assignment should stay below.
}

int morale_point::pick_time( int current_time, int new_time, bool same_sign ) const
{
    const int remaining_time = current_time - age;
    return ( remaining_time <= new_time && same_sign ) ? new_time : remaining_time;
}

void morale_point::proceed( int ticks )
{
    if( ticks < 0 ) {
        debugmsg( "%s(): Called with negative ticks %d.", __FUNCTION__, ticks );
        return;
    }

    age += ticks;
}

void player_morale::add( morale_type type, int bonus, int max_bonus,
                         int duration, int decay_start,
                         bool capped, const itype *item_type )
{
    // Search for a matching morale entry.
    for( auto &m : points ) {
        if( m.get_type() == type && m.get_item_type() == item_type ) {
            const int prev_bonus = m.get_net_bonus();

            m.add( bonus, max_bonus, duration, decay_start, capped );
            if( m.get_net_bonus() != prev_bonus ) {
                invalidate();
            }
            return;
        }
    }

    morale_point new_morale( type, item_type, bonus, duration, decay_start );

    if( !new_morale.is_expired() ) {
        points.push_back( new_morale );
        invalidate();
    }
}

int player_morale::has( morale_type type, const itype *item_type ) const
{
    for( auto &m : points ) {
        if( m.get_type() == type && ( item_type == nullptr || m.get_item_type() == item_type ) ) {
            return m.get_net_bonus();
        }
    }
    return 0;
}

void player_morale::remove( morale_type type, const itype *item_type )
{
    for( size_t i = 0; i < points.size(); ++i ) {
        if( points[i].get_type() == type && points[i].get_item_type() == item_type ) {
            if( points[i].get_net_bonus() > 0 ) {
                invalidate();
            }
            points.erase( points.begin() + i );
            break;
        }
    }
}

morale_mult player_morale::get_traits_mult( const player *p ) const
{
    morale_mult ret;

    if( p->has_trait( "OPTIMISTIC" ) ) {
        ret *= morale_mults::optimistic;
    }

    if( p->has_trait( "BADTEMPER" ) ) {
        ret *= morale_mults::badtemper;
    }

    return ret;
}

morale_mult player_morale::get_effects_mult( const player *p ) const
{
    morale_mult ret;

    //TODO: Maybe add something here to cheer you up as well?
    if( p->has_effect( effect_took_prozac ) ) {
        ret *= morale_mults::prozac;
    }

    return ret;
}

int player_morale::get_level( const player *p ) const
{
    if( !level_is_valid ) {
        const morale_mult mult = get_traits_mult( p );

        level = 0;
        for( auto &m : points ) {
            level += m.get_net_bonus( mult );
        }

        level *= get_effects_mult( p );
        level_is_valid = true;
    }

    return level;
}

void player_morale::update( const player *p, const int ticks )
{
    const auto proceed = [ ticks ]( morale_point & m ) {
        m.proceed( ticks );
    };
    const auto is_expired = []( const morale_point & m ) -> bool { return m.is_expired(); };

    std::for_each( points.begin(), points.end(), proceed );
    const auto new_end = std::remove_if( points.begin(), points.end(), is_expired );
    points.erase( new_end, points.end() );
    // Apply persistent morale effects
    apply_persistent( p );
    // Invalidate level to recalculate it on demand
    invalidate();
}

void player_morale::display( const player *p )
{
    // Create and draw the window itself.
    WINDOW *w = newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                        ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                        ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );
    draw_border( w );

    // Figure out how wide the name column needs to be.
    int name_column_width = 18;
    for( auto &i : points ) {
        int length = utf8_width( i.get_name() );
        if( length > name_column_width ) {
            name_column_width = length;
        }
    }

    // If it's too wide, truncate.
    if( name_column_width > 72 ) {
        name_column_width = 72;
    }

    // Start printing the number right after the name column.
    // We'll right-justify it later.
    int number_pos = name_column_width + 1;

    // Header
    mvwprintz( w, 1,  1, c_white, _( "Morale Modifiers:" ) );
    mvwprintz( w, 2,  1, c_ltgray, _( "Name" ) );
    mvwprintz( w, 2, name_column_width + 2, c_ltgray, _( "Value" ) );

    // Ensure the player's persistent morale effects are up-to-date.
    apply_persistent( p );

    const morale_mult mult = get_traits_mult( p );
    // Print out the morale entries.
    for( size_t i = 0; i < points.size(); i++ ) {
        std::string name = points[i].get_name();
        int bonus = points[i].get_net_bonus( mult );

        // Print out the name.
        trim_and_print( w, i + 3,  1, name_column_width, ( bonus < 0 ? c_red : c_green ), name.c_str() );

        // Print out the number, right-justified.
        mvwprintz( w, i + 3, number_pos, ( bonus < 0 ? c_red : c_green ),
                   "% 6d", bonus );
    }

    // Print out the total morale, right-justified.
    int mor = get_level( p );
    mvwprintz( w, 20, 1, ( mor < 0 ? c_red : c_green ), _( "Total:" ) );
    mvwprintz( w, 20, number_pos, ( mor < 0 ? c_red : c_green ), "% 6d", mor );

    // Print out the focus gain rate, right-justified.
    double gain = ( p->calc_focus_equilibrium() - p->focus_pool ) / 100.0;
    mvwprintz( w, 22, 1, ( gain < 0 ? c_red : c_green ), _( "Focus gain:" ) );
    mvwprintz( w, 22, number_pos - 3, ( gain < 0 ? c_red : c_green ), _( "%6.2f per minute" ), gain );

    // Make sure the changes are shown.
    wrefresh( w );

    // Wait for any keystroke.
    getch();

    // Close the window.
    werase( w );
    delwin( w );
}

void player_morale::clear()
{
    points.clear();
    invalidate();
}

void player_morale::invalidate()
{
    level_is_valid = false;
}

int player_morale::get_hoarder_penalty( const player *p ) const
{
    int pen = int( ( p->volume_capacity() - p->volume_carried() ) / 2 );

    if( pen > 70 ) {
        pen = 70;
    } else if( pen < 0 ) {
        pen = 0;
    }
    if( p->has_effect( effect_took_xanax ) ) {
        pen = int( pen / 7 );
    } else if( p->has_effect( effect_took_prozac ) ) {
        pen = int( pen / 2 );
    }
    return pen;
}


int player_morale::get_stylish_bonus( const player *p ) const
{
    int bonus = 0;
    std::bitset<num_bp> covered; // body parts covered

    for( auto &elem : p->worn ) {
        const bool basic_flag = elem.has_flag( "FANCY" );
        const bool bonus_flag = elem.has_flag( "SUPER_FANCY" );

        if( basic_flag || bonus_flag ) {
            std::bitset<num_bp> covered_by_elem = elem.get_covered_body_parts();

            if( bonus_flag ) {
                bonus += 2;
            } else if( ( covered & covered_by_elem ).none() ) {
                // bonus += 1;
            }

            covered |= covered_by_elem;
        }
    }

    if( covered.test( bp_torso ) ) {
        bonus += 6;
    }
    if( covered.test( bp_leg_l ) || covered.test( bp_leg_r ) ) {
        bonus += 2;
    }
    if( covered.test( bp_foot_l ) || covered.test( bp_foot_r ) ) {
        bonus += 1;
    }
    if( covered.test( bp_hand_l ) || covered.test( bp_hand_r ) ) {
        bonus += 1;
    }
    if( covered.test( bp_head ) ) {
        bonus += 3;
    }
    if( covered.test( bp_eyes ) ) {
        bonus += 2;
    }
    if( covered.test( bp_arm_l ) || covered.test( bp_arm_r ) ) {
        bonus += 1;
    }
    if( covered.test( bp_mouth ) ) {
        bonus += 2;
    }

    if( bonus > 20 ) {
        bonus = 20;
    }

    return bonus;
}

int player_morale::get_pain_bonus( const player *p ) const
{
    int bonus = p->pain / 2.5;
    // Advanced masochists really get a morale bonus from pain.
    // (It's not capped.)
    if( p->has_trait( "MASOCHIST" ) && ( bonus > 25 ) ) {
        bonus = 25;
    }
    if( p->has_effect( effect_took_prozac ) ) {
        bonus = int( bonus / 3 );
    }

    return bonus;
}

void player_morale::apply_persistent( const player *p )
{
    // Hoarders get a morale penalty if they're not carrying a full inventory.
    if( p->has_trait( "HOARDER" ) ) {
        const int pen = get_hoarder_penalty( p );
        if( pen > 0 ) {
            add( MORALE_PERM_HOARDER, -pen, -pen, 5, 5, true );
        }
    }

    // The stylish get a morale bonus for each body part covered in an item
    // with the FANCY or SUPER_FANCY tag.
    if( p->has_trait( "STYLISH" ) ) {
        const int bonus = get_stylish_bonus( p );
        if( bonus > 0 ) {
            add( MORALE_PERM_FANCY, bonus, bonus, 5, 5, true );
        }
    }

    // Floral folks really don't like having their flowers covered.
    if( p->has_trait( "FLOWERS" ) && p->wearing_something_on( bp_head ) ) {
        add( MORALE_PERM_CONSTRAINED, -10, -10, 5, 5, true );
    }

    // The same applies to rooters and their feet; however, they don't take
    // too many problems from no-footgear.
    double shoe_factor = p->footwear_factor();
    if( ( p->has_trait( "ROOTS" ) || p->has_trait( "ROOTS2" ) || p->has_trait( "ROOTS3" ) ) &&
        shoe_factor ) {
        add( MORALE_PERM_CONSTRAINED, -10 * shoe_factor, -10 * shoe_factor, 5, 5, true );
    }

    // Masochists get a morale bonus from pain.
    if( p->has_trait( "MASOCHIST" ) || p->has_trait( "MASOCHIST_MED" ) ||
        p->has_trait( "CENOBITE" ) ) {
        const int bonus = get_pain_bonus( p );
        if( bonus > 0 ) {
            add( MORALE_PERM_MASOCHIST, bonus, bonus, 5, 5, true );
        }
    }

    // Optimist gives a base +4 to morale.
    // The +25% boost from optimist also applies here, for a net of +5.
    if( p->has_trait( "OPTIMISTIC" ) ) {
        add( MORALE_PERM_OPTIMIST, 4, 4, 5, 5, true );
    }

    // And Bad Temper works just the same way.  But in reverse.  ):
    if( p->has_trait( "BADTEMPER" ) ) {
        add( MORALE_PERM_BADTEMPER, -4, -4, 5, 5, true );
    }
}
