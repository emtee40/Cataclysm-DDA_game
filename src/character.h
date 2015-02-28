#ifndef CHARACTER_H
#define CHARACTER_H

#include "creature.h"
#include "action.h"
#include "inventory.h"

#include <map>
#include <unordered_map>

class Character : public Creature
{
    public:
        virtual ~Character() override;

        field_id bloodType() const;
        field_id gibType() const;
        virtual bool is_warm() const override;
        virtual const std::string &symbol() const override;
        
        /** Processes effects which may prevent the Character from moving (bear traps, crushed, etc.).
         *  Returns false if movement is stopped. */
        virtual bool move_effects();
        /** Performs any Character-specific modifications to the arguments before passing to Creature::add_effect(). */
        virtual void add_effect(efftype_id eff_id, int dur, body_part bp = num_bp, bool permanent = false,
                                int intensity = 0);
        
        /** Recalculates HP after a change to max strength */
        void recalc_hp();
        /** Modifies the player's sight values
         *  Must be called when any of the following change:
         *  This must be called when any of the following change:
         * - effects
         * - bionics
         * - mutations
         * - underwater
         * - clothes
         */
        void recalc_sight_limits();
        
        // --------------- Mutation Stuff ---------------
        // In newcharacter.cpp
        /** Returns the id of a random starting trait that costs >= 0 points */
        std::string random_good_trait();
        /** Returns the id of a random starting trait that costs < 0 points */
        std::string random_bad_trait();
        
        // In mutation.cpp
    public:
        /** Returns true if the player has the entered mutation. */
        bool has_mut(const muttype_id &flag) const;
        /** Returns true if the Character has the given mutation and it is active. */
        bool has_active_mutation(const muttype_id &flag) const;
        /** Returns true if the player has the entered mutation as a trait. */
        bool is_trait(const muttype_id &flag) const;
        /** Adds a new mutation if the player doesn't already have it. Performs no other
         *  checks on mutation validity. If message == true and Character is the player,
         *  displays a message about gaining the mutation. Calls mutation_effect().
         *  Returns true if the mutation was added successfully. */
        bool add_mutation(const muttype_id &flag, bool message = true);
        /** Calls add mutation with the given arguments. If the new mutation is
         *  successfully added, adds the mutation id to my_traits as well. */
        void add_trait(const muttype_id &flag, bool message = false);
        /** Removes the entered mutation if the player has it, calling mutation_loss_effect().
         *  Performs no other checks on mutation removal validity. Calls add_mutation() on any
         *  traits that are no longer suppressed by conflicting mutations after mutation removal.
         *  Displays a mutation loss message if message == true. */
        void remove_mutation(const muttype_id &flag, bool message = true);
        /** Calls add_mutation() on the first flag and remove_mutation() on the second.
         *  Performs no checks on mutation addition/removal validity. Prints a message
         *  about the first mutation turning into the second. */
        void replace_mutation(const muttype_id &flag1, const muttype_id &flag2);
        
    protected:
        /** Applies stat mods to character. active = true when mods are caused by activating or
         *  deactivating a mutation, false when mods are caused by adding or removing the mutation. */
        void apply_mods(const std::string &mut_id, bool add_remove, bool active);
        /** Handles mutation gain effects. (Destruction of armor, etc.) */
        void mutation_effect(const muttype_id &flag);
        /** Handles mutation loss effects. */
        void mutation_loss_effect(const muttype_id &flag);
        /** Activates a mutation if possible. Calls mutation.activate() to determine activation
         *  effects and costs before applying mutation activation costs. */
        void activate_mutation(const muttype_id &flag);
        /** Deactivates a mutation if possible. Calls mutation.deactivate(). */
        void deactivate_mutation(const muttype_id &flag);
        /** Calls mutation.process() on all mutations in my_mutations. Determines if any
         *  any stats are low enough to cause forceful deactivation of active mutations
         *  and applies any active mutation automatic reactivation costs. */
        void process_mutations()
        /** Returns true if the player currently has a threshold mutation. */
        bool crossed_threshold() const;
        /** Returns the id of the current highest strength mutation category. */
        std::string get_highest_category() const;
        /** Returns a random dream message of the given category and strength. */
        std::string get_category_dream(const std::string &cat, int strength) const;
        /** The basic mutation function. This determines a mutation to upgrade or downgrade
         *  and calls either mutate_towards() or downgrade_mutation() as needed. */
        void mutate();
        /** The basic purification function. This attempts to add a missing unsuppressed trait
         *  back to the player's mutation list. If there are no valid traits, it will attempt
         *  to call downgrade_mutation() on a random mutation. */
        void purify();
        /** This attempts to logically pick a mutation within the given category and calls
         *  mutate_towards() on the chosen mutation. */
        void mutate_category(const std::string &cat);
        
    private:
        /** Returns a vector of all mutations that either replace the given mutation
         *  or have it as a prerequisite. */
        std::vector<muttype_id> get_dependant_muts(const muttype_id &flag) const;
        /** Attempts to perform a logical downgrade on the entered mutation, either removing
         *  or replacing the mutation with a lower level replacement. Will only downgrade traits
         *  if trait_force == true. */
        void downgrade_mutation(const muttype_id &flag, bool trait_force);
        /** Reevaluates the current mutation category levels. Should be called any time
         *  that mutations are added or removed. Calls set_cat_level_rec(). */
        void set_cat_levels();
        /** A recursive function called by set_cat_levels() to apply the current
         *  category strength effects of the entered mutation. */
        void set_cat_level_rec(const muttype_id &flag);
        /** Returns true if the player has a mutation which is eventually replaced by the
         *  entered one. Else returns false. Recursively calls itself. */
        bool has_lower_mut(const muttype_id &flag) const;
        /** Returns true if the player has a mutation which eventually replaces the
         *  entered one. Else returns false. Recursively calls itself. */
        bool has_higher_mut(const muttype_id &flag) const;
        /** Returns true if the player has a mutation which the entered mutation cancels, or
         *  if the player has any mutation which cancels the entered mutation.
         *  Else returns false. */
        bool has_conflicting_mut(const muttype_id &flag) const;
        /** Returns false if the Character already has the mutation, has a higher mutation,
         *  or if the mutation doesn't match the current force_good & force_bad setting.
         *  Else will return true. */
        bool mutation_ok(const muttype_id &flag, bool force_good, bool force_bad) const;
        /** A helper function used by mutate_towards(). Returns false if the entered vector
         *  is empty or if the player has one of the listed mutation ids. Else returns true. */
        bool mut_vect_check(std::vector<muttype_id> &current_vector) const;
        /** Logically attempts to mutate towards a given mutation. Recursively calls itself
         *  before calling add_mutation(), downgrade_mutation(), or replace_mutation() as needed. */
        void mutate_towards(muttype_id &flag);

        // --------------- Bionic Stuff ---------------
    public:
        /** Returns true if the player has the entered bionic id */
        bool has_bionic(const bionic_id &b) const;
        /** Returns true if the player has the entered bionic id and it is powered on */
        bool has_active_bionic(const bionic_id &b) const;
        
        // --------------- Generic Item Stuff ---------------

        struct has_mission_item_filter {
            int mission_id;
            bool operator()(const item &it) {
                return it.mission_id == mission_id;
            }
        };

        // -2 position is 0 worn index, -3 position is 1 worn index, etc
        static int worn_position_to_index(int position)
        {
            return -2 - position;
        }
        
        /**
         * Test whether an item in the possession of this player match a
         * certain filter.
         * The items might be inside other items (containers / quiver / etc.),
         * the filter is recursively applied to all item contents.
         * If this returns true, the vector returned by @ref items_with
         * (with the same filter) will be non-empty.
         * @param filter some object that when invoked with the () operator
         * returns true for item that should checked for.
         * @return Returns true when at least one item matches the filter,
         * if no item matches the filter it returns false.
         */
        template<typename T>
        bool has_item_with(T filter) const
        {
            if( inv.has_item_with( filter ) ) {
                return true;
            }
            if( !weapon.is_null() && inventory::has_item_with_recursive( weapon, filter ) ) {
                return true;
            }
            for( auto &w : worn ) {
                if( inventory::has_item_with_recursive( w, filter ) ) {
                    return true;
                }
            }
            return false;
        }
        /**
         * Gather all items that match a certain filter.
         * The returned vector contains pointers to items in the possession
         * of this player (can be weapon, worn items or inventory).
         * The items might be inside other items (containers / quiver / etc.),
         * the filter is recursively applied to all item contents.
         * The items should not be changed directly, the pointers can be used
         * with @ref i_rem, @ref reduce_charges. The pointers are *not* suitable
         * for @ref get_item_position because the returned index can only
         * refer to items directly in the inventory (e.g. -1 means the weapon,
         * there is no index for the content of the weapon).
         * @param filter some object that when invoked with the () operator
         * returns true for item that should be returned.
         */
        template<typename T>
        std::vector<const item *> items_with(T filter) const
        {
            auto result = inv.items_with( filter );
            if( !weapon.is_null() ) {
                inventory::items_with_recursive( result, weapon, filter );
            }
            for( auto &w : worn ) {
                inventory::items_with_recursive( result, w, filter );
            }
            return result;
        }
        /**
         * Removes the items that match the given filter.
         * The returned items are a copy of the removed item.
         * If no item has been removed, an empty list will be returned.
         */
        template<typename T>
        std::list<item> remove_items_with( T filter )
        {
            // player usually interacts with items in the inventory the most (?)
            std::list<item> result = inv.remove_items_with( filter );
            for( auto &article : worn ) {
                if( filter( article ) ) {
                    result.push_back( article );
                } else {
                    result.splice( result.begin(), article.remove_items_with( filter ) );
                }
            }
            worn.erase( std::remove_if( worn.begin(), worn.end(), filter ), worn.end() );
            if( !weapon.is_null() ) {
                if( filter( weapon ) ) {
                    result.push_back( remove_weapon() );
                } else {
                    result.splice( result.begin(), weapon.remove_items_with( filter ) );
                }
            }
            return result;
        }
        
        item &i_add(item it);
        /**
         * Remove a specific item from player possession. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param pos The item position of the item to be removed. The item *must*
         * exists, use @ref has_item to check this.
         * @return A copy of the removed item.
         */
        item i_rem(int pos);
        /**
         * Remove a specific item from player possession. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param it A pointer to the item to be removed. The item *must* exists
         * in the players possession (one can use @ref has_item to check for this).
         * @return A copy of the removed item.
         */
        item i_rem(const item *it);
        void i_rem_keep_contents( int pos );
        /** Sets invlet and adds to inventory if possible, drops otherwise, returns true if either succeeded.
         *  An optional qty can be provided (and will perform better than separate calls). */
        bool i_add_or_drop(item &it, int qty = 1);
        
        /** Only use for UI things. Returns all invlets that are currently used in
         * the player inventory, the weapon slot and the worn items. */
        std::set<char> allocated_invlets() const;
        
        /**
         * Whether the player carries an active item of the given item type.
         */
        bool has_active_item(const itype_id &id) const;
        item remove_weapon();
        void remove_mission_items(int mission_id);
        
        int weight_carried() const;
        int volume_carried() const;
        int weight_capacity() const;
        int volume_capacity() const;
        bool can_pickVolume(int volume) const;
        bool can_pickWeight(int weight, bool safe = true) const;
        
        bool has_artifact_with(const art_effect_passive effect) const;
        
        // --------------- Clothing Stuff ---------------
        /** Returns true if the player is wearing the item. */
        bool is_wearing(const itype_id &it) const;
        /** Returns true if the player is wearing the item on the given body_part. */
        bool is_wearing_on_bp(const itype_id &it, body_part bp) const;
        /** Returns true if the player is wearing an item with the given flag. */
        bool worn_with_flag( std::string flag ) const;
        
        // --------------- Skill Stuff ---------------
        SkillLevel &skillLevel(const Skill* _skill);
        SkillLevel &skillLevel(std::string ident);

        /** for serialization */
        SkillLevel get_skill_level(const Skill* _skill) const;
        SkillLevel get_skill_level(const std::string &ident) const;
        
        // --------------- Other Stuff ---------------
        

        /** return the calendar::turn the character expired */
        int get_turn_died() const
        {
            return turn_died;
        }
        /** set the turn the turn the character died if not already done */
        void set_turn_died(int turn)
        {
            turn_died = (turn_died != -1) ? turn : turn_died;
        }
        
        /** Calls Creature::normalize()
         *  nulls out the player's weapon
         *  Should only be called through player::normalize(), not on it's own!
         */
        virtual void normalize();
        virtual void die(Creature *nkiller);
        
        /** Resets stats, and applies effects in an idempotent manner */
        virtual void reset_stats();
        
        /** Returns true if the player has some form of night vision */
        bool has_nv();
        
        // In newcharacter.cpp
        void empty_skills();
        /** Returns a random name from NAMES_* */
        void pick_name();
        /** Returns the set "my_traits" */
        std::vector<std::string> get_traits() const;
        /** Returns the set "my_mutations" */
        std::vector<std::string> get_mutations() const;
        /** Empties the trait list */
        void empty_traits();
        void add_traits();
        
        // --------------- Values ---------------
        std::string name;
        bool male;
        
        std::vector<item> worn;
        std::array<int, num_hp_parts> hp_cur, hp_max;
        bool nv_cached;
        
        inventory inv;
        std::map<char, itype_id> assigned_invlet;
        itype_id last_item;
        item weapon;
        item ret_null; // Null item, sometimes returns by weapon() etc

    protected:
        Character();
        Character(const Character &) = default;
        Character(Character &&) = default;
        Character &operator=(const Character &) = default;
        Character &operator=(Character &&) = default;

        void store(JsonOut &jsout) const;
        void load(JsonObject &jsin);
        
        // --------------- Values ---------------
        std::map<const Skill*, SkillLevel> _skills;

        std::unordered_map<muttype_id, mutation> my_mutations;
        std::unordered_set<muttype_id> my_traits;
        std::map<std::string, int> mutation_category_level;
        
        std::vector<bionic> my_bionics;
        
        bool underwater;
        
        int sight_max;
        int sight_boost;
        int sight_boost_cap;

        // turn the character expired, if -1 it has not been set yet.
        int turn_died = -1;
};

#endif
