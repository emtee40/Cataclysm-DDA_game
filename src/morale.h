#ifndef MORALE_H
#define MORALE_H

#include "json.h"
#include <string>
#include "calendar.h"

struct itype;
class player;

enum morale_type : int {
    MORALE_NULL = 0,
    MORALE_FOOD_GOOD,
    MORALE_FOOD_HOT,
    MORALE_MUSIC,
    MORALE_HONEY,
    MORALE_GAME,
    MORALE_MARLOSS,
    MORALE_MUTAGEN,
    MORALE_FEELING_GOOD,
    MORALE_SUPPORT,
    MORALE_PHOTOS,

    MORALE_CRAVING_NICOTINE,
    MORALE_CRAVING_CAFFEINE,
    MORALE_CRAVING_ALCOHOL,
    MORALE_CRAVING_OPIATE,
    MORALE_CRAVING_SPEED,
    MORALE_CRAVING_COCAINE,
    MORALE_CRAVING_CRACK,
    MORALE_CRAVING_MUTAGEN,
    MORALE_CRAVING_DIAZEPAM,
    MORALE_CRAVING_MARLOSS,

    MORALE_FOOD_BAD,
    MORALE_CANNIBAL,
    MORALE_VEGETARIAN,
    MORALE_MEATARIAN,
    MORALE_ANTIFRUIT,
    MORALE_LACTOSE,
    MORALE_ANTIJUNK,
    MORALE_ANTIWHEAT,
    MORALE_NO_DIGEST,
    MORALE_WET,
    MORALE_DRIED_OFF,
    MORALE_COLD,
    MORALE_HOT,
    MORALE_FEELING_BAD,
    MORALE_KILLED_INNOCENT,
    MORALE_KILLED_FRIEND,
    MORALE_KILLED_MONSTER,
    MORALE_MUTILATE_CORPSE,
    MORALE_MUTAGEN_ELF,
    MORALE_MUTAGEN_CHIMERA,
    MORALE_MUTAGEN_MUTATION,

    MORALE_MOODSWING,
    MORALE_BOOK,
    MORALE_COMFY,

    MORALE_SCREAM,

    MORALE_PERM_MASOCHIST,
    MORALE_PERM_HOARDER,
    MORALE_PERM_FANCY,
    MORALE_PERM_OPTIMIST,
    MORALE_PERM_BADTEMPER,
    MORALE_PERM_CONSTRAINED,
    MORALE_GAME_FOUND_KITTEN,

    MORALE_HAIRCUT,
    MORALE_SHAVE,

    NUM_MORALE_TYPES
};

// Morale multiplier
struct morale_mult {
    morale_mult(): good( 1.0 ), bad( 1.0 ) {}
    morale_mult( double good, double bad ): good( good ), bad( bad ) {}
    morale_mult( double both ): good( both ), bad( both ) {}

    double good;    // For good morale
    double bad;     // For bad morale

    morale_mult operator * ( const morale_mult &rhs ) const {
        return morale_mult( *this ) *= rhs;
    }

    morale_mult &operator *= ( const morale_mult &rhs ) {
        good *= rhs.good;
        bad *= rhs.bad;
        return *this;
    }
};

inline double operator * ( double morale, const morale_mult &mult )
{
    return morale * ( ( morale >= 0.0 ) ? mult.good : mult.bad );
}

inline double operator * ( const morale_mult &mult, double morale )
{
    return morale * mult;
}

inline double operator *= ( double &morale, const morale_mult &mult )
{
    morale = morale * mult;
    return morale;
}

inline int operator *= ( int &morale, const morale_mult &mult )
{
    morale = morale * mult;
    return morale;
}

// Commonly used morale multipliers
namespace morale_mults
{
// Optimistic characters focus on the good things in life,
// and downplay the bad things.
static const morale_mult optimistic( 1.25, 0.75 );
// Again, those grouchy Bad-Tempered folks always focus on the negative.
// They can't handle positive things as well.  They're No Fun.  D:
static const morale_mult badtemper( 0.75, 1.25 );
// Prozac reduces overall negative morale by 75%.
static const morale_mult prozac( 1.0, 0.25 );
}

class morale_point : public JsonSerializer, public JsonDeserializer
{
    public:
        morale_point(
            morale_type type = MORALE_NULL,
            const itype *item_type = nullptr,
            int bonus = 0,
            int duration = MINUTES( 6 ),
            int decay_start = MINUTES( 3 ),
            int age = 0 ):
            type( type ),
            item_type( item_type ),
            bonus( bonus ),
            duration( duration ),
            decay_start( decay_start ),
            age( age ) {};

        using JsonDeserializer::deserialize;
        void deserialize( JsonIn &jsin ) override;
        using JsonSerializer::serialize;
        void serialize( JsonOut &json ) const override;

        std::string get_name() const;
        int get_net_bonus() const;
        int get_net_bonus( const morale_mult &mult ) const;
        bool is_expired() const;

        morale_type get_type() const {
            return type;
        }

        const itype *get_item_type() const {
            return item_type;
        }

        void add( int new_bonus, int new_max_bonus, int new_duration, int new_decay_start, bool new_cap );
        void proceed( int ticks = 1 );

    private:
        morale_type type;
        const itype *item_type;

        int bonus;
        int duration;
        int decay_start;
        int age;

        /**
         * Returns either new_time or remaining time (which one is greater).
         * Only returns new time if same_sign is true
         */
        int pick_time( int cur_time, int new_time, bool same_sign ) const;
};

class player_morale
{
    public:
        player_morale() = default;
        player_morale( player_morale && ) = default;
        player_morale( const player_morale & ) = default;
        player_morale &operator =( player_morale && ) = default;
        player_morale &operator =( const player_morale & ) = default;

        /** Returns overall morale level */
        int get_level( const player *p ) const;
        /** Adds morale to existing or creates one */
        void add( morale_type type, int bonus, int max_bonus = 0, int duration = 60,
                  int decay_start = 30, bool capped = false, const itype *item_type = nullptr );
        /** Returns bonus from specified morale */
        int has( morale_type type, const itype *item_type = nullptr ) const;
        /** Removes specified morale */
        void remove( morale_type type, const itype *item_type = nullptr );
        /** Ticks down morale counters and removes them */
        void update( const player *p, const int ticks = 1 );
        /** Displays morale screen */
        void display( const player *p );
        /** Clears up all morale points */
        void clear();
        /** Invalidates morale level to recalculate it on demand */
        void invalidate();
    protected:
        friend class player;

        void store( JsonOut &jsout ) const;
        void load( JsonObject &jsin );
    private:
        std::vector<morale_point> points;
        // Mutability is required for lazy initialization
        mutable int level;
        mutable bool level_is_valid;

        /** Returns current traits multiplier for morale */
        morale_mult get_traits_mult( const player *p ) const;
        /** Returns current effects multiplier for morale */
        morale_mult get_effects_mult( const player *p ) const;
        /** Returns penalty if inventory's not full */
        int get_hoarder_penalty( const player *p ) const;
        /** Returns bonus for stylish stuff */
        int get_stylish_bonus( const player *p ) const;
        /** Returns bonus for masochists etc */
        int get_pain_bonus( const player *p ) const;
        /** Ensures persistent morale effects are up-to-date */
        void apply_persistent( const player *p );
};

#endif
