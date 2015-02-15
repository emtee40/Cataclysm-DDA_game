#ifndef MUTATION_H
#define MUTATION_H

#include "pldata.h"
#include "json.h"
#include "enums.h" // tripoint
#include "ui.h"
#include <vector>
#include <map>
#include <unordered_map>

class mutation_type
{
        friend void load_mutation_type(JsonObject &jo);
        friend class mutation;
    public:
        // --------------- Constructors ---------------
        mutation_type();
        mutation_type(const mutation_type &rhs);
    private:
        // --------------- Values ---------------
        /** Mutation id. */
        muttype_id id;
        /** Mutation in-game name. */
        std::string name;
        /** Mutation in-game description. */
        std::string description;
        /** Point cost at character creation. */
        int points;
        /** How visible the mutation is. */
        int visibility;
        /** How ugly the mutation is compared to base human standards. */
        int ugliness;
        /** False for mutations that can't be obtained through normal means. */
        bool valid;
        /** Can the mutation be purified? */
        bool purifiable;
        /** Is the mutation a threshold mutation? */
        bool threshold;
        /** Is the mutation a profession based mutation? */
        bool profession;
        /** Prerequisites, only 1 is needed. */
        std::vector<std::string> prereqs;
        /** Secondary prerequisites, only 1 is needed. */
        std::vector<std::string> prereqs2;
        /** Threshold prerequisite, only 1 is needed. */
        std::vector<std::string> threshreq;
        /** Mutations that this mutation removes. */
        std::vector<std::string> cancels;
        /** Mutations that this mutation can mutate into. */
        std::vector<std::string> replacements;
        /** Mutations that this mutation makes more likely to occur. */
        std::vector<std::string> additions;
        /** What categories does this mutation fall into? */
        std::vector<std::string> category;
        /** Mutation wet protection effect. */
        std::map<std::string, mutation_wet> protection;
        /** Mutation stat modifiers, key pair order is <active: bool, mod type: "STR"> */
        std::unordered_map<std::pair<bool, std::string>, int> mods;
        /** Martial art styles that can be chosen on character generation with this mutation.
         *  Used mainly by profession mutations. */
        std::vector<std::string> initial_ma_styles;
}

class mutation : public JsonSerializer, public JsonDeserializer
{
    public:
    private:
        // --------------- Values ---------------
}


struct dream;
struct mutation_branch;

extern std::vector<dream> dreams;
extern std::map<std::string, std::vector<std::string> > mutations_category;
extern std::map<std::string, mutation_branch> mutation_data;
typedef std::pair<body_part, tripoint> mutation_wet;

struct dream {
    std::vector<std::string> messages; // The messages that the dream will give
    std::string category; // The category that will trigger the dream
    int strength; // The category strength required for the dream

    dream()
    {
        category = "";
        strength = 0;
    }
};

struct mutation_branch {
    bool valid; // True if this is a valid mutation (False for "unavailable from generic mutagen")
    bool purifiable; // True if Purifier can remove it (False for *Special* mutations)
    bool threshold; // True if it's a threshold itself, and shouldn't be obtained *easily* (False by default)
    bool profession; // True if this is a trait associated with professional training/experience, so profession/quest ONLY
    std::vector<std::string> prereqs; // Prerequisites; Only one is required
    std::vector<std::string> prereqs2; // Prerequisites; need one from here too
    std::vector<std::string> threshreq; // Prerequisites; dedicated slot to needing thresholds
    std::vector<std::string> cancels; // Mutations that conflict with this one
    std::vector<std::string> replacements; // Mutations that replace this one
    std::vector<std::string> additions; // Mutations that add to this one
    std::vector<std::string> category; // Mutation Categories
    std::map<std::string, mutation_wet> protection; // Mutation wet effects
    /** Key pair is <active: bool, mod type: "STR"> */
    std::unordered_map<std::pair<bool, std::string>, int> mods; // Mutation stat mods
    std::vector<std::string> initial_ma_styles; // Martial art styles that can be chosen upon character generation

    mutation_branch()
    {
        valid = false;
    };
};

extern std::vector<std::string> faulty_traits;
extern std::vector<std::string> unpowered_traits;
void mut_draw_exam_window(WINDOW *win, int border_line, bool examination);
void reset_mutations();

void init_mutation_parts();
void load_mutation(JsonObject &jsobj);
void load_dream(JsonObject &jsobj);

#endif
