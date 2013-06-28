#include "player.h"
#include "profession.h"
#include "item_factory.h"
#include "output.h"
#include "rng.h"
#include "keypress.h"
#include "game.h"
#include "options.h"
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <fstream>
#include <sstream>

// Colors used in this file: (Most else defaults to c_ltgray)
#define COL_STAT_ACT		c_ltred    // Selected stat
#define COL_TR_GOOD		c_green    // Good trait descriptive text
#define COL_TR_GOOD_OFF_ACT		c_ltgray  // A toggled-off good trait
#define COL_TR_GOOD_ON_ACT		c_ltgreen    // A toggled-on good trait
#define COL_TR_GOOD_OFF_PAS		c_dkgray  // A toggled-off good trait
#define COL_TR_GOOD_ON_PAS		c_green    // A toggled-on good trait
#define COL_TR_BAD		c_red      // Bad trait descriptive text
#define COL_TR_BAD_OFF_ACT		c_ltgray    // A toggled-off bad trait
#define COL_TR_BAD_ON_ACT		c_red      // A toggled-on bad trait
#define COL_TR_BAD_OFF_PAS		c_dkgray    // A toggled-off bad trait
#define COL_TR_BAD_ON_PAS		c_ltred      // A toggled-on bad trait
#define COL_SKILL_USED		c_green    // A skill with at least one point

#define HIGH_STAT 14 // The point after which stats cost double

#define NEWCHAR_TAB_MAX 4 // The ID of the rightmost tab

void draw_tabs(WINDOW* w, std::string sTab);

int set_stats(WINDOW* w, game* g, player *u, int &points);
int set_traits(WINDOW* w, game* g, player *u, int &points, int max_trait_points);
int set_profession(WINDOW* w, game* g, player *u, int &points);
int set_skills(WINDOW* w, game* g, player *u, int &points);
int set_description(WINDOW* w, game* g, player *u, int &points);

int random_skill();

int calc_HP(int strength, bool tough);

void save_template(player *u);

bool player::create(game *g, character_type type, std::string tempname)
{
 weapon = item(g->itypes["null"], 0);

 g->u.prof = profession::generic();

 WINDOW* w = newwin(25, 80, (TERMY > 25) ? (TERMY-25)/2 : 0, (TERMX > 80) ? (TERMX-80)/2 : 0);

 int tab = 0, points = 38, max_trait_points = 12;
 if (type != PLTYPE_CUSTOM) {
  switch (type) {
   case PLTYPE_RANDOM: {
    str_max = rng(6, 12);
    dex_max = rng(6, 12);
    int_max = rng(6, 12);
    per_max = rng(6, 12);
    points = points - str_max - dex_max - int_max - per_max;
    if (str_max > HIGH_STAT)
     points -= (str_max - HIGH_STAT);
    if (dex_max > HIGH_STAT)
     points -= (dex_max - HIGH_STAT);
    if (int_max > HIGH_STAT)
     points -= (int_max - HIGH_STAT);
    if (per_max > HIGH_STAT)
     points -= (per_max - HIGH_STAT);

    int num_gtraits = 0, num_btraits = 0, rn, tries;
    while (points < 0 || rng(-3, 20) > points) {
     if (num_btraits < max_trait_points && one_in(3)) {
      tries = 0;
      do {
       rn = random_bad_trait();
       tries++;
      } while ((has_trait(rn) ||
              num_btraits - traits[rn].points > max_trait_points) && tries < 5);
      if (tries < 5) {
       toggle_trait(rn);
       points -= traits[rn].points;
       num_btraits -= traits[rn].points;
      }
     } else {
      switch (rng(1, 4)) {
       case 1: if (str_max > 5) { str_max--; points++; } break;
       case 2: if (dex_max > 5) { dex_max--; points++; } break;
       case 3: if (int_max > 5) { int_max--; points++; } break;
       case 4: if (per_max > 5) { per_max--; points++; } break;
      }
     }
    }
    while (points > 0) {
     switch (rng((num_gtraits < max_trait_points ? 1 : 5), 9)) {
     case 1:
     case 2:
     case 3:
     case 4:
      rn = random_good_trait();
      if (!has_trait(rn) && points >= traits[rn].points &&
          num_gtraits + traits[rn].points <= max_trait_points) {
       toggle_trait(rn);
       points -= traits[rn].points;
       num_gtraits += traits[rn].points;
      }
      break;
     case 5:
      switch (rng(1, 4)) {
       case 1: if (str_max < HIGH_STAT) { str_max++; points--; } break;
       case 2: if (dex_max < HIGH_STAT) { dex_max++; points--; } break;
       case 3: if (int_max < HIGH_STAT) { int_max++; points--; } break;
       case 4: if (per_max < HIGH_STAT) { per_max++; points--; } break;
      }
      break;
     case 6:
     case 7:
     case 8:
     case 9:
      rn = random_skill();

      Skill *aSkill = Skill::skill(rn);
      int level = skillLevel(aSkill);

      if (level < points) {
        points -= level + 1;
        skillLevel(aSkill).level(level + 2);
      }
      break;
     }
    }
   } break;
   case PLTYPE_TEMPLATE: {
    std::ifstream fin;
    std::stringstream filename;
    filename << "data/" << tempname << ".template";
    fin.open(filename.str().c_str());
    if (!fin.is_open()) {
     debugmsg("Couldn't open %s!", filename.str().c_str());
     return false;
    }
    std::string(data);
    getline(fin, data);
    load_info(g, data);
    points = 0;
   } break;
  }
  tab = NEWCHAR_TAB_MAX;
 } else
  points = OPTIONS[OPT_INITIAL_POINTS];
  max_trait_points = OPTIONS[OPT_MAX_TRAIT_POINTS];

 do {
  werase(w);
  wrefresh(w);
  switch (tab) {
   case 0: tab += set_stats      (w, g, this, points); break;
   case 1: tab += set_traits     (w, g, this, points, max_trait_points); break;
   case 2: tab += set_profession (w, g, this, points); break;
   case 3: tab += set_skills     (w, g, this, points); break;
   case 4: tab += set_description(w, g, this, points); break;
  }
 } while (tab >= 0 && tab <= NEWCHAR_TAB_MAX);
 delwin(w);

 if (tab < 0)
  return false;

 // Character is finalized.  Now just set up HP, &c
 for (int i = 0; i < num_hp_parts; i++) {
  hp_max[i] = calc_HP(str_max, has_trait(PF_TOUGH));
  hp_cur[i] = hp_max[i];
 }
 if (has_trait(PF_HARDCORE)) {
  for (int i = 0; i < num_hp_parts; i++) {
   hp_max[i] = int(hp_max[i] * .25);
   hp_cur[i] = hp_max[i];
  }
 } if (has_trait(PF_GLASSJAW)) {
  hp_max[hp_head] = int(hp_max[hp_head] * .80);
  hp_cur[hp_head] = hp_max[hp_head];
 }
 if (has_trait(PF_SMELLY))
  scent = 800;
 if (has_trait(PF_ANDROID)) {
  bionic_id first_bio;
  do {
   first_bio = g->random_good_bionic();
  } while (bionics[first_bio]->power_cost > 10);
  add_bionic(first_bio);
  add_bionic(bionic_id(power_source_bionics[rng(0,power_source_bionics.size()-1)]));	// Power Source
  max_power_level = 10;
  power_level = 10;
 }

 if (has_trait(PF_MARTIAL_ARTS)) {
  itype_id ma_type;
  do {
   int choice = menu(false, "Pick your style:",
                     "Karate", "Judo", "Aikido", "Tai Chi", "Taekwondo", NULL);
   if (choice == 1)
    ma_type = "style_karate";
   if (choice == 2)
    ma_type = "style_judo";
   if (choice == 3)
    ma_type = "style_aikido";
   if (choice == 4)
    ma_type = "style_tai_chi";
   if (choice == 5)
    ma_type = "style_taekwondo";
   item tmpitem = item(g->itypes[ma_type], 0);
   full_screen_popup(tmpitem.info(true).c_str());
  } while (!query_yn("Use this style?"));
  styles.push_back(ma_type);
 }

    if (has_trait(PF_MARTIAL_ARTS2)) {
  itype_id ma_type;
  do {
   int choice = menu(false, "Pick your style:",
                     "Capoeira", "Krav Maga", "Muay Thai", "Ninjutsu", "Zui Quan", NULL);
   if (choice == 1)
    ma_type = "style_capoeira";
   if (choice == 2)
    ma_type = "style_krav_maga";
   if (choice == 3)
    ma_type = "style_muay_thai";
   if (choice == 4)
    ma_type = "style_ninjutsu";
   if (choice == 5)
    ma_type = "style_zui_quan";
   item tmpitem = item(g->itypes[ma_type], 0);
   full_screen_popup(tmpitem.info(true).c_str());
  } while (!query_yn("Use this style?"));
  styles.push_back(ma_type);
 }
 if (has_trait(PF_MARTIAL_ARTS3)) {
  itype_id ma_type;
  do {
   int choice = menu(false, "Pick your style:",
                     "Tiger", "Crane", "Leopard", "Snake", "Dragon", NULL);
   if (choice == 1)
    ma_type = "style_tiger";
   if (choice == 2)
    ma_type = "style_crane";
   if (choice == 3)
    ma_type = "style_leopard";
   if (choice == 4)
    ma_type = "style_snake";
   if (choice == 5)
    ma_type = "style_dragon";
   item tmpitem = item(g->itypes[ma_type], 0);
   full_screen_popup(tmpitem.info(true).c_str());
  } while (!query_yn("Use this style?"));
  styles.push_back(ma_type);
 }
 if (has_trait(PF_MARTIAL_ARTS4)) {
  itype_id ma_type;
  do {
   int choice = menu(false, "Pick your style:",
                     "Centipede", "Viper", "Scorpion", "Lizard", "Toad", NULL);
   if (choice == 1)
    ma_type = "style_centipede";
   if (choice == 2)
    ma_type = "style_venom_snake";
   if (choice == 3)
    ma_type = "style_scorpion";
   if (choice == 4)
    ma_type = "style_lizard";
   if (choice == 5)
    ma_type = "style_toad";
   item tmpitem = item(g->itypes[ma_type], 0);
   full_screen_popup(tmpitem.info(true).c_str());
  } while (!query_yn("Use this style?"));
  styles.push_back(ma_type);
 }
 ret_null = item(g->itypes["null"], 0);
 if (!styles.empty())
  weapon = item(g->itypes[ styles[0] ], 0, ':');
 else
  weapon   = item(g->itypes["null"], 0);

 item tmp; //gets used several times

 std::vector<std::string> prof_items = g->u.prof->items();
 for (std::vector<std::string>::const_iterator iter = prof_items.begin(); iter != prof_items.end(); ++iter) {
  item tmp = item(item_controller->find_template(*iter), 0, 'a' + worn.size());
  if (tmp.is_armor()) {
   if (tmp.has_flag("VARSIZE"))
    tmp.item_tags.insert("FIT");
   worn.push_back(tmp);
  } else {
   inv.push_back(tmp);
  }
 }

 std::vector<addiction> prof_addictions = g->u.prof->addictions();
 for (std::vector<addiction>::const_iterator iter = prof_addictions.begin(); iter != prof_addictions.end(); ++iter)
 {
     g->u.addictions.push_back(*iter);
 }

 std::vector<std::string> prof_mutations = g->u.prof->mutations();
 for (std::vector<std::string>::const_iterator iter = prof_mutations.begin(); iter != prof_mutations.end(); ++iter)
 {
     if(*iter == "PF_SKIN_ROUGH")
    {
        g->u.mutate_towards(g,PF_SKIN_ROUGH,true);
    }
    if(*iter == "PF_NIGHTVISION2")
    {
        g->u.mutate_towards(g,PF_NIGHTVISION2,true);
    }
    if(*iter == "PF_NIGHTVISION3")
    {
        g->u.mutate_towards(g,PF_NIGHTVISION3,true);
    }
    if(*iter == "PF_INFRARED")
    {
        g->u.mutate_towards(g,PF_INFRARED,true);
    }
    if(*iter == "PF_FASTHEALER2")
    {
        g->u.mutate_towards(g,PF_FASTHEALER2,true);
    }
    if(*iter == "PF_REGEN")
    {
        g->u.mutate_towards(g,PF_REGEN,true);
    }
    if(*iter == "PF_FANGS")
    {
        g->u.mutate_towards(g,PF_FANGS,true);
    }
    if(*iter == "PF_MEMBRANE")
    {
        g->u.mutate_towards(g,PF_MEMBRANE,true);
    }
    if(*iter == "PF_GILLS")
    {
        g->u.mutate_towards(g,PF_GILLS,true);
    }
    if(*iter == "PF_SCALES")
    {
        g->u.mutate_towards(g,PF_SCALES,true);
    }
    if(*iter == "PF_THICK_SCALES")
    {
        g->u.mutate_towards(g,PF_THICK_SCALES,true);
    }
    if(*iter == "PF_SLEEK_SCALES")
    {
        g->u.mutate_towards(g,PF_SLEEK_SCALES,true);
    }
    if(*iter == "PF_LIGHT_BONES")
    {
        g->u.mutate_towards(g,PF_LIGHT_BONES,true);
    }
    if(*iter == "PF_FEATHERS")
    {
        g->u.mutate_towards(g,PF_FEATHERS,true);
    }
    if(*iter == "PF_LIGHTFUR")
    {
        g->u.mutate_towards(g,PF_LIGHTFUR,true);
    }
    if(*iter == "PF_FUR")
    {
        g->u.mutate_towards(g,PF_FUR,true);
    }
     if(*iter == "PF_CHITIN")
    {
        g->u.mutate_towards(g,PF_CHITIN,true);
    }
    if(*iter == "PF_CHITIN2")
    {
        g->u.mutate_towards(g,PF_CHITIN2,true);
    }
    if(*iter == "PF_CHITIN3")
    {
        g->u.mutate_towards(g,PF_CHITIN3,true);
    }
    if(*iter == "PF_SPINES")
    {
        g->u.mutate_towards(g,PF_SPINES,true);
    }
    if(*iter == "PF_QUILLS")
    {
        g->u.mutate_towards(g,PF_QUILLS,true);
    }
    if(*iter == "PF_PLANTSKIN")
    {
        g->u.mutate_towards(g,PF_PLANTSKIN,true);
    }
    if(*iter == "PF_BARK")
    {
        g->u.mutate_towards(g,PF_BARK,true);
    }
    if(*iter == "PF_THORNS")
    {
        g->u.mutate_towards(g,PF_THORNS,true);
    }
    if(*iter == "PF_LEAVES")
    {
        g->u.mutate_towards(g,PF_LEAVES,true);
    }
    if(*iter == "PF_NAILS")
    {
        g->u.mutate_towards(g,PF_NAILS,true);
    }
    if(*iter == "PF_CLAWS")
    {
        g->u.mutate_towards(g,PF_CLAWS,true);
    }
    if(*iter == "PF_TALONS")
    {
        g->u.mutate_towards(g,PF_TALONS,true);
    }
    if(*iter == "PF_RADIOGENIC")
    {
        g->u.mutate_towards(g,PF_RADIOGENIC,true);
    }
    if(*iter == "PF_MARLOSS")
    {
        g->u.mutate_towards(g,PF_MARLOSS,true);
    }
    if(*iter == "PF_PHEROMONE_INSECT")
    {
        g->u.mutate_towards(g,PF_PHEROMONE_INSECT,true);
    }
    if(*iter == "PF_PHEROMONE_MAMMAL")
    {
        g->u.mutate_towards(g,PF_PHEROMONE_MAMMAL,true);
    }
    if(*iter == "PF_DISIMMUNE")
    {
        g->u.mutate_towards(g,PF_DISIMMUNE,true);
    }
    if(*iter == "PF_POISONOUS")
    {
        g->u.mutate_towards(g,PF_POISONOUS,true);
    }
    if(*iter == "PF_SLIME_HANDS")
    {
        g->u.mutate_towards(g,PF_SLIME_HANDS,true);
    }
    if(*iter == "PF_COMPOUND_EYES")
    {
        g->u.mutate_towards(g,PF_COMPOUND_EYES,true);
    }
    if(*iter == "PF_PADDED_FEET")
    {
        g->u.mutate_towards(g,PF_PADDED_FEET,true);
    }
    if(*iter == "PF_HOOVES")
    {
        g->u.mutate_towards(g,PF_HOOVES,true);
    }
    if(*iter == "PF_SAPROVORE")
    {
        g->u.mutate_towards(g,PF_SAPROVORE,true);
    }
    if(*iter == "PF_RUMINANT")
    {
        g->u.mutate_towards(g,PF_RUMINANT,true);
    }
    if(*iter == "PF_HORNS")
    {
        g->u.mutate_towards(g,PF_HORNS,true);
    }
    if(*iter == "PF_HORNS_CURLED")
    {
        g->u.mutate_towards(g,PF_HORNS_CURLED,true);
    }
    if(*iter == "PF_HORNS_POINTED")
    {
        g->u.mutate_towards(g,PF_HORNS_POINTED,true);
    }
    if(*iter == "PF_ANTENNAE")
    {
        g->u.mutate_towards(g,PF_ANTENNAE,true);
    }
    if(*iter == "PF_FLEET2")
    {
        g->u.mutate_towards(g,PF_FLEET2,true);
    }
    if(*iter == "PF_TAIL_STUB")
    {
        g->u.mutate_towards(g,PF_TAIL_STUB,true);
    }
    if(*iter == "PF_TAIL_FIN")
    {
        g->u.mutate_towards(g,PF_TAIL_FIN,true);
    }
    if(*iter == "PF_TAIL_LONG")
    {
        g->u.mutate_towards(g,PF_TAIL_LONG,true);
    }
    if(*iter == "PF_TAIL_FLUFFY")
    {
        g->u.mutate_towards(g,PF_TAIL_FLUFFY,true);
    }
    if(*iter == "PF_TAIL_STING")
    {
        g->u.mutate_towards(g,PF_TAIL_STING,true);
    }
    if(*iter == "PF_TAIL_CLUB")
    {
        g->u.mutate_towards(g,PF_TAIL_CLUB,true);
    }
    if(*iter == "PF_PAINREC1")
    {
        g->u.mutate_towards(g,PF_PAINREC1,true);
    }
    if(*iter == "PF_PAINREC2")
    {
        g->u.mutate_towards(g,PF_PAINREC2,true);
    }
    if(*iter == "PF_PAINREC3")
    {
        g->u.mutate_towards(g,PF_PAINREC3,true);
    }
    if(*iter == "PF_WINGS_BIRD")
    {
        g->u.mutate_towards(g,PF_WINGS_BIRD,true);
    }
    if(*iter == "PF_WINGS_INSECT")
    {
        g->u.mutate_towards(g,PF_WINGS_INSECT,true);
    }
    if(*iter == "PF_MOUTH_TENTACLES")
    {
        g->u.mutate_towards(g,PF_MOUTH_TENTACLES,true);
    }
    if(*iter == "PF_MANDIBLES")
    {
        g->u.mutate_towards(g,PF_MANDIBLES,true);
    }
    if(*iter == "PF_CANINE_EARS")
    {
        g->u.mutate_towards(g,PF_CANINE_EARS,true);
    }
    if(*iter == "PF_WEB_WALKER")
    {
        g->u.mutate_towards(g,PF_WEB_WEAVER,true);
    }
    if(*iter == "PF_WHISKERS")
    {
        g->u.mutate_towards(g,PF_WHISKERS,true);
    }
    if(*iter == "PF_STR_UP")
    {
        g->u.mutate_towards(g,PF_STR_UP,true);
    }
    if(*iter == "PF_STR_UP_2")
    {
        g->u.mutate_towards(g,PF_STR_UP_2,true);
    }
    if(*iter == "PF_STR_UP_3")
    {
        g->u.mutate_towards(g,PF_STR_UP_3,true);
    }
    if(*iter == "PF_STR_UP_4")
    {
        g->u.mutate_towards(g,PF_STR_UP_4,true);
    }
    if(*iter == "PF_DEX_UP")
    {
        g->u.mutate_towards(g,PF_DEX_UP,true);
    }
    if(*iter == "PF_DEX_UP_2")
    {
        g->u.mutate_towards(g,PF_DEX_UP_2,true);
    }
    if(*iter == "PF_DEX_UP_3")
    {
        g->u.mutate_towards(g,PF_DEX_UP_3,true);
    }
    if(*iter == "PF_DEX_UP_4")
    {
        g->u.mutate_towards(g,PF_DEX_UP_4,true);
    }
    if(*iter == "PF_INT_UP")
    {
        g->u.mutate_towards(g,PF_INT_UP,true);
    }
    if(*iter == "PF_INT_UP_2")
    {
        g->u.mutate_towards(g,PF_INT_UP_2,true);
    }
    if(*iter == "PF_INT_UP_3")
    {
        g->u.mutate_towards(g,PF_INT_UP_3,true);
    }
    if(*iter == "PF_INT_UP_4")
    {
        g->u.mutate_towards(g,PF_INT_UP_4,true);
    }
    if(*iter == "PF_PER_UP")
    {
        g->u.mutate_towards(g,PF_PER_UP,true);
    }
    if(*iter == "PF_PER_UP_2")
    {
        g->u.mutate_towards(g,PF_PER_UP_2,true);
    }
    if(*iter == "PF_PER_UP_3")
    {
        g->u.mutate_towards(g,PF_PER_UP_3,true);
    }
    if(*iter == "PF_PER_UP_4")
    {
        g->u.mutate_towards(g,PF_PER_UP_4,true);
    }
    if(*iter == "PF_HEADBUMPS")
    {
        g->u.mutate_towards(g,PF_HEADBUMPS,true);
    }
    if(*iter == "PF_ANTLERS")
    {
        g->u.mutate_towards(g,PF_ANTLERS,true);
    }
    if(*iter == "PF_SLIT_NOSTRILS")
    {
        g->u.mutate_towards(g,PF_SLIT_NOSTRILS,true);
    }
    if(*iter == "PF_FORKED_TONGUE")
    {
        g->u.mutate_towards(g,PF_FORKED_TONGUE,true);
    }
    if(*iter == "PF_EYEBULGE")
    {
        g->u.mutate_towards(g,PF_EYEBULGE,true);
    }
    if(*iter == "PF_MOUTH_FLAPS")
    {
        g->u.mutate_towards(g,PF_MOUTH_FLAPS,true);
    }
    if(*iter == "PF_WINGS_STUB")
    {
        g->u.mutate_towards(g,PF_WINGS_STUB,true);
    }
    if(*iter == "PF_WINGS_BAT")
    {
        g->u.mutate_towards(g,PF_WINGS_BAT,true);
    }
    if(*iter == "PF_PALE")
    {
        g->u.mutate_towards(g,PF_PALE,true);
    }
    if(*iter == "PF_SPOTS")
    {
        g->u.mutate_towards(g,PF_SPOTS,true);
    }
    if(*iter == "PF_SMELLY2")
    {
        g->u.mutate_towards(g,PF_SMELLY2,true);
    }
    if(*iter == "PF_DEFORMED")
    {
        g->u.mutate_towards(g,PF_DEFORMED,true);
    }
    if(*iter == "PF_DEFORMED2")
    {
        g->u.mutate_towards(g,PF_DEFORMED2,true);
    }
    if(*iter == "PF_DEFORMED3")
    {
        g->u.mutate_towards(g,PF_DEFORMED3,true);
    }
    if(*iter == "PF_BEAUTIFUL")
    {
        g->u.mutate_towards(g,PF_BEAUTIFUL,true);
    }
    if(*iter == "PF_BEAUTIFUL2")
    {
        g->u.mutate_towards(g,PF_BEAUTIFUL2,true);
    }
    if(*iter == "PF_BEAUTIFUL3")
    {
        g->u.mutate_towards(g,PF_BEAUTIFUL3,true);
    }
    if(*iter == "PF_HOLLOW_BONES")
    {
        g->u.mutate_towards(g,PF_HOLLOW_BONES,true);
    }
    if(*iter == "PF_NAUSEA")
    {
        g->u.mutate_towards(g,PF_NAUSEA,true);
    }
    if(*iter == "PF_VOMITOUS")
    {
        g->u.mutate_towards(g,PF_VOMITOUS,true);
    }
    if(*iter == "PF_HUNGER")
    {
        g->u.mutate_towards(g,PF_HUNGER,true);
    }
    if(*iter == "PF_THIRST")
    {
        g->u.mutate_towards(g,PF_THIRST,true);
    }
    if(*iter == "PF_ROT1")
    {
        g->u.mutate_towards(g,PF_ROT1,true);
    }
    if(*iter == "PF_ROT2")
    {
        g->u.mutate_towards(g,PF_ROT2,true);
    }
    if(*iter == "PF_ROT3")
    {
        g->u.mutate_towards(g,PF_ROT3,true);
    }
    if(*iter == "PF_ALBINO")
    {
        g->u.mutate_towards(g,PF_ALBINO,true);
    }
    if(*iter == "PF_SORES")
    {
        g->u.mutate_towards(g,PF_SORES,true);
    }
    if(*iter == "PF_TROGLO")
    {
        g->u.mutate_towards(g,PF_TROGLO,true);
    }
    if(*iter == "PF_TROGLO2")
    {
        g->u.mutate_towards(g,PF_TROGLO2,true);
    }
    if(*iter == "PF_TROGLO3")
    {
        g->u.mutate_towards(g,PF_TROGLO3,true);
    }
    if(*iter == "PF_WEBBED")
    {
        g->u.mutate_towards(g,PF_WEBBED,true);
    }
    if(*iter == "PF_BEAK")
    {
        g->u.mutate_towards(g,PF_BEAK,true);
    }
    if(*iter == "PF_UNSTABLE")
    {
        g->u.mutate_towards(g,PF_UNSTABLE,true);
    }
    if(*iter == "PF_RADIOACTIVE1")
    {
        g->u.mutate_towards(g,PF_RADIOACTIVE1,true);
    }
    if(*iter == "PF_RADIOACTIVE2")
    {
        g->u.mutate_towards(g,PF_RADIOACTIVE2,true);
    }
    if(*iter == "PF_RADIOACTIVE3")
    {
        g->u.mutate_towards(g,PF_RADIOACTIVE3,true);
    }
    if(*iter == "PF_SLIMY")
    {
        g->u.mutate_towards(g,PF_SLIMY,true);
    }
    if(*iter == "PF_HERBIVORE")
    {
        g->u.mutate_towards(g,PF_HERBIVORE,true);
    }
    if(*iter == "PF_CARNIVORE")
    {
        g->u.mutate_towards(g,PF_CARNIVORE,true);
    }
    if(*iter == "PF_PONDEROUS1")
    {
        g->u.mutate_towards(g,PF_PONDEROUS1,true);
    }
    if(*iter == "PF_PONDEROUS2")
    {
        g->u.mutate_towards(g,PF_PONDEROUS2,true);
    }
    if(*iter == "PF_PONDEROUS3")
    {
        g->u.mutate_towards(g,PF_PONDEROUS3,true);
    }
    if(*iter == "PF_SUNLIGHT_DEPENDENT")
    {
        g->u.mutate_towards(g,PF_SUNLIGHT_DEPENDENT,true);
    }
    if(*iter == "PF_COLDBLOOD")
    {
        g->u.mutate_towards(g,PF_COLDBLOOD,true);
    }
    if(*iter == "PF_COLDBLOOD2")
    {
        g->u.mutate_towards(g,PF_COLDBLOOD2,true);
    }
    if(*iter == "PF_COLDBLOOD3")
    {
        g->u.mutate_towards(g,PF_COLDBLOOD3,true);
    }
    if(*iter == "PF_GROWL")
    {
        g->u.mutate_towards(g,PF_GROWL,true);
    }
    if(*iter == "PF_SNARL")
    {
        g->u.mutate_towards(g,PF_SNARL,true);
    }
    if(*iter == "PF_SHOUT1")
    {
        g->u.mutate_towards(g,PF_SHOUT1,true);
    }
    if(*iter == "PF_SHOUT2")
    {
        g->u.mutate_towards(g,PF_SHOUT2,true);
    }
    if(*iter == "PF_SHOUT3")
    {
        g->u.mutate_towards(g,PF_SHOUT3,true);
    }
    if(*iter == "PF_ARM_TENTACLES")
    {
        g->u.mutate_towards(g,PF_ARM_TENTACLES,true);
    }
    if(*iter == "PF_ARM_TENTACLES_4")
    {
        g->u.mutate_towards(g,PF_ARM_TENTACLES_4,true);
    }
    if(*iter == "PF_ARM_TENTACLES_8")
    {
        g->u.mutate_towards(g,PF_ARM_TENTACLES_8,true);
    }
    if(*iter == "PF_SHELL")
    {
        g->u.mutate_towards(g,PF_SHELL,true);
    }
    if(*iter == "PF_LEG_TENTACLES")
    {
        g->u.mutate_towards(g,PF_LEG_TENTACLES,true);
    }
 }

 // Grab the skills from the profession, if there are any
 profession::StartingSkillList prof_skills = g->u.prof->skills();
 for (profession::StartingSkillList::const_iterator iter = prof_skills.begin();
      iter != prof_skills.end(); ++iter)
 {
     assert(Skill::skill(iter->first));
     if (Skill::skill(iter->first))
     {
        g->u.boost_skill_level(iter->first, iter->second);
     }
 }

// The near-sighted get to start with glasses.
 if (has_trait(PF_MYOPIC) && !has_trait(PF_HYPEROPIC)) {
  tmp = item(g->itypes["glasses_eye"], 0, 'a' + worn.size());
  worn.push_back(tmp);
 }
// And the far-sighted get to start with reading glasses.
 if (has_trait(PF_HYPEROPIC) && !has_trait(PF_MYOPIC)) {
  tmp = item(g->itypes["glasses_reading"], 0, 'a' + worn.size());
  worn.push_back(tmp);
 }

 if (has_trait(PF_HYPEROPIC) && has_trait(PF_MYOPIC))
 {
     tmp = item(g->itypes["glasses_bifocal"], 0, 'a' + worn.size());
     worn.push_back(tmp);
 }

// Likewise, the asthmatic start with their medication.
 if (has_trait(PF_ASTHMA)) {
  tmp = item(g->itypes["inhaler"], 0, 'a' + worn.size());
  inv.push_back(tmp);
 }
// Basic starter gear, added independently of profession.
 tmp = item(g->itypes["pockknife"], 0,'a' + worn.size());
  inv.push_back(tmp);
 tmp = item(g->itypes["matches"], 0,'a' + worn.size());
  inv.push_back(tmp);
// make sure we have no mutations
 //for (int i = 0; i < PF_MAX2; i++)
 // if (!has_base_trait(i))
//	my_mutations[i] = false;
 return true;
}

void draw_tabs(WINDOW* w, std::string sTab)
{
 for (int i = 1; i < 79; i++) {
  mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 4, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 24, i, c_ltgray, LINE_OXOX);

  if (i > 2 && i < 24) {
   mvwputch(w, i, 0, c_ltgray, LINE_XOXO);
   mvwputch(w, i, 79, c_ltgray, LINE_XOXO);
  }
 }

 draw_tab(w, 7, "STATS", (sTab == "STATS") ? true : false);
 draw_tab(w, 18, "TRAITS", (sTab == "TRAITS") ? true : false);
 draw_tab(w, 30, "PROFESSION", (sTab == "PROFESSION") ? true : false);
 draw_tab(w, 46, "SKILLS", (sTab == "SKILLS") ? true : false);
 draw_tab(w, 58, "DESCRIPTION", (sTab == "DESCRIPTION") ? true : false);

 mvwputch(w, 2,  0, c_ltgray, LINE_OXXO); // |^
 mvwputch(w, 2, 79, c_ltgray, LINE_OOXX); // ^|

 mvwputch(w, 4, 0, c_ltgray, LINE_XXXO); // |-
 mvwputch(w, 4, 79, c_ltgray, LINE_XOXX); // -|

 mvwputch(w, 24, 0, c_ltgray, LINE_XXOO); // |_
 mvwputch(w, 24, 79, c_ltgray, LINE_XOOX); // _|
}

int set_stats(WINDOW* w, game* g, player *u, int &points)
{
 unsigned char sel = 1;
 char ch;

 draw_tabs(w, "STATS");

 mvwprintz(w, 11, 2, c_ltgray, "j/k, 8/2, or arrows select");
 mvwprintz(w, 12, 2, c_ltgray, " a statistic.");
 mvwprintz(w, 13, 2, c_ltgray, "l, 6, or right arrow");
 mvwprintz(w, 14, 2, c_ltgray, " increases the statistic.");
 mvwprintz(w, 15, 2, c_ltgray, "h, 4, or left arrow");
 mvwprintz(w, 16, 2, c_ltgray, " decreases the statistic.");
 mvwprintz(w, 18, 2, c_ltgray, "> Takes you to the next tab.");
 mvwprintz(w, 19, 2, c_ltgray, "< Returns you to the main menu.");

 do {
  mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);
  switch (sel) {
  case 1:
   if (u->str_max >= HIGH_STAT)
    mvwprintz(w, 3, 33, c_ltred, "Increasing Str further costs 2 points.");
   else
    mvwprintz(w, 3, 33, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   mvwprintz(w, 6,  2, COL_STAT_ACT, "Strength:     %d  ", u->str_max);
   mvwprintz(w, 7,  2, c_ltgray,     "Dexterity:    %d  ", u->dex_max);
   mvwprintz(w, 8,  2, c_ltgray,     "Intelligence: %d  ", u->int_max);
   mvwprintz(w, 9,  2, c_ltgray,     "Perception:   %d  ", u->per_max);
   mvwprintz(w, 6, 33, COL_STAT_ACT, "Base HP: %d                                 ",
             calc_HP(u->str_max, u->has_trait(PF_TOUGH)));
   mvwprintz(w, 7, 33, COL_STAT_ACT, "Carry weight: %d lbs                        ",
             u->weight_capacity(false) / 4);
   mvwprintz(w, 8, 33, COL_STAT_ACT, "Melee damage: %d                            ",
             u->base_damage(false));
   mvwprintz(w,10, 33, COL_STAT_ACT, "Strength also makes you more resistant to   ");
   mvwprintz(w,11, 33, COL_STAT_ACT, "many diseases and poisons, and makes actions");
   mvwprintz(w,12, 33, COL_STAT_ACT, "which require brute force more effective.   ");
   break;

  case 2:
   if (u->dex_max >= HIGH_STAT)
    mvwprintz(w, 3, 33, c_ltred, "Increasing Dex further costs 2 points.");
   else
    mvwprintz(w, 3, 33, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   mvwprintz(w, 6,  2, c_ltgray,     "Strength:     %d  ", u->str_max);
   mvwprintz(w, 7,  2, COL_STAT_ACT, "Dexterity:    %d  ", u->dex_max);
   mvwprintz(w, 8,  2, c_ltgray,     "Intelligence: %d  ", u->int_max);
   mvwprintz(w, 9,  2, c_ltgray,     "Perception:   %d  ", u->per_max);
   mvwprintz(w, 6, 33, COL_STAT_ACT, "Melee to-hit bonus: +%d                      ",
             u->base_to_hit(false));
   mvwprintz(w, 7, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w, 7, 33, COL_STAT_ACT, "Ranged %s: %s%d",
             (u->ranged_dex_mod(false) <= 0 ? "bonus" : "penalty"),
             (u->ranged_dex_mod(false) <= 0 ? "+" : "-"),
             abs(u->ranged_dex_mod(false)));
   mvwprintz(w, 8, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w, 8, 33, COL_STAT_ACT, "Throwing %s: %s%d",
             (u->throw_dex_mod(false) <= 0 ? "bonus" : "penalty"),
             (u->throw_dex_mod(false) <= 0 ? "+" : "-"),
             abs(u->throw_dex_mod(false)));
   mvwprintz(w, 9, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w,10, 33, COL_STAT_ACT, "Dexterity also enhances many actions which  ");
   mvwprintz(w,11, 33, COL_STAT_ACT, "require finesse.                            ");
   mvwprintz(w,12, 33, COL_STAT_ACT, "                                            ");
   break;

  case 3:
   if (u->int_max >= HIGH_STAT)
    mvwprintz(w, 3, 33, c_ltred, "Increasing Int further costs 2 points.");
   else
    mvwprintz(w, 3, 33, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   mvwprintz(w, 6,  2, c_ltgray,     "Strength:     %d  ", u->str_max);
   mvwprintz(w, 7,  2, c_ltgray,     "Dexterity:    %d  ", u->dex_max);
   mvwprintz(w, 8,  2, COL_STAT_ACT, "Intelligence: %d  ", u->int_max);
   mvwprintz(w, 9,  2, c_ltgray,     "Perception:   %d  ", u->per_max);

   mvwprintz(w, 6, 33, COL_STAT_ACT, "Read times: %d%%%%                              ",
             u->read_speed(false));
   mvwprintz(w, 7, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w, 8, 33, COL_STAT_ACT, "Intelligence is also used when crafting,    ");
   mvwprintz(w, 9, 33, COL_STAT_ACT, "installing bionics, and interacting with    ");
   mvwprintz(w,10, 33, COL_STAT_ACT, "NPCs.                                       ");
   mvwprintz(w,11, 33, COL_STAT_ACT, "                                            ");
   break;

  case 4:
   if (u->per_max >= HIGH_STAT)
    mvwprintz(w, 3, 33, c_ltred, "Increasing Per further costs 2 points.");
   else
    mvwprintz(w, 3, 33, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   mvwprintz(w, 6,  2, c_ltgray,     "Strength:     %d  ", u->str_max);
   mvwprintz(w, 7,  2, c_ltgray,     "Dexterity:    %d  ", u->dex_max);
   mvwprintz(w, 8,  2, c_ltgray,     "Intelligence: %d  ", u->int_max);
   mvwprintz(w, 9,  2, COL_STAT_ACT, "Perception:   %d  ", u->per_max);
   mvwprintz(w, 6, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w, 6, 33, COL_STAT_ACT, "Ranged %s: %s%d",
             (u->ranged_per_mod(false) <= 0 ? "bonus" : "penalty"),
             (u->ranged_per_mod(false) <= 0 ? "+" : "-"),
             abs(u->ranged_per_mod(false)));
   mvwprintz(w, 7, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w, 8, 33, COL_STAT_ACT, "Perception is also used for detecting       ");
   mvwprintz(w, 9, 33, COL_STAT_ACT, "traps and other things of interest.         ");
   mvwprintz(w,10, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w,11, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w,12, 33, COL_STAT_ACT, "                                            ");
   break;
  }

  wrefresh(w);
  ch = input();
  if (ch == 'j' && sel < 4)
   sel++;
  if (ch == 'k' && sel > 1)
   sel--;
  if (ch == 'h') {
   if (sel == 1 && u->str_max > 4) {
    if (u->str_max > HIGH_STAT)
     points++;
    u->str_max--;
    points++;
   } else if (sel == 2 && u->dex_max > 4) {
    if (u->dex_max > HIGH_STAT)
     points++;
    u->dex_max--;
    points++;
   } else if (sel == 3 && u->int_max > 4) {
    if (u->int_max > HIGH_STAT)
     points++;
    u->int_max--;
    points++;
   } else if (sel == 4 && u->per_max > 4) {
    if (u->per_max > HIGH_STAT)
     points++;
    u->per_max--;
    points++;
   }
  }
  if (ch == 'l' && points > 0) {
   if (sel == 1 && u->str_max < 20 && (u->str_max < HIGH_STAT || points > 1)) {
    points--;
    if (u->str_max >= HIGH_STAT)
     points--;
    u->str_max++;
   } else if (sel == 2 && u->dex_max < 20 &&
              (u->dex_max < HIGH_STAT || points > 1)) {
    points--;
    if (u->dex_max >= HIGH_STAT)
     points--;
    u->dex_max++;
   } else if (sel == 3 && u->int_max < 20 &&
              (u->int_max < HIGH_STAT || points > 1)) {
    points--;
    if (u->int_max >= HIGH_STAT)
     points--;
    u->int_max++;
   } else if (sel == 4 && u->per_max < 20 &&
              (u->per_max < HIGH_STAT || points > 1)) {
    points--;
    if (u->per_max >= HIGH_STAT)
     points--;
    u->per_max++;
   }
  }
  if (ch == '<' && query_yn("Return to main menu?"))
   return -1;
  if (ch == '>')
   return 1;
 } while (true);
}

int set_traits(WINDOW* w, game* g, player *u, int &points, int max_trait_points)
{
 draw_tabs(w, "TRAITS");

 WINDOW* w_description = newwin(3, 78, 21 + getbegy(w), 1 + getbegx(w));
// Track how many good / bad POINTS we have; cap both at MAX_TRAIT_POINTS
 int num_good = 0, num_bad = 0;
 for (int i = 0; i < PF_SPLIT; i++) {
  if (u->has_trait(i))
   num_good += traits[i].points;
 }
 for (int i = PF_SPLIT + 1; i < PF_MAX; i++) {
  if (u->has_trait(i))
   num_bad += abs(traits[i].points);
 }

 for (int i = 0; i < 16; i++) {//preparation: draw disadvantages list
  mvwprintz(w, 5 + i, 40, c_dkgray, "\
                                     ");
  if (u->has_trait(PF_SPLIT + 1 + i))//highlight disadvantages
   mvwprintz(w, 5 + i, 40, COL_TR_BAD_ON_PAS, traits[PF_SPLIT + 1 + i].name.c_str());
  else
   mvwprintz(w, 5 + i, 40, COL_TR_BAD_OFF_PAS, traits[PF_SPLIT + 1 + i].name.c_str());
 }
 mvwprintz(w,11,32, c_ltgray, "h   l");
 mvwprintz(w,12,32, c_ltgray, "<   >");
 mvwprintz(w,13,32, c_ltgray, "4   6");
 mvwprintz(w,15,32, c_ltgray, "Space");
 mvwprintz(w,16,31, c_ltgray,"Toggles");

 int cur_adv = 1, cur_dis = PF_SPLIT + 1, cur_trait, traitmin, traitmax, xoff;
 nc_color col_on_act, col_off_act, col_on_pas, col_off_pas, hi_on, hi_off;
 bool using_adv = true;	// True if we're selecting advantages, false if we're
			// selecting disadvantages

 do {
  mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);
  mvwprintz(w,  3,18, c_ltgreen, "%s%d/%d", (num_good < 10 ? " " : ""),
                                 num_good, max_trait_points);
  mvwprintz(w,  3,25, c_ltred, "%s%d/%d", (num_bad < 10 ? " " : ""),
                               num_bad, max_trait_points);
// Clear the bottom of the screen.
  mvwprintz(w_description, 0, 0, c_ltgray, "                                                                             ");
  mvwprintz(w_description, 1, 0, c_ltgray, "                                                                             ");
  mvwprintz(w_description, 2, 0, c_ltgray, "                                                                             ");
  if (using_adv) {//Traits costs note and traits description
   col_on_act  = COL_TR_GOOD_ON_ACT;
   col_off_act = COL_TR_GOOD_OFF_ACT;
   col_on_pas  = COL_TR_GOOD_ON_PAS;
   col_off_pas = COL_TR_GOOD_OFF_PAS;
   hi_on   = hilite(col_on_act);
   hi_off  = hilite(col_off_act);
   xoff = 2;
   cur_trait = cur_adv;
   traitmin = 1;
   traitmax = PF_SPLIT;
   mvwprintz(w,  3, 33, c_ltgray, "                                              ");
   mvwprintz(w,  3, 33, COL_TR_GOOD, "%s costs %d points",
             traits[cur_adv].name.c_str(), traits[cur_adv].points);
   mvwprintz(w_description, 0, 0, COL_TR_GOOD, "%s", traits[cur_adv].description.c_str());
  } else {
   col_on_act  = COL_TR_BAD_ON_ACT;
   col_off_act = COL_TR_BAD_OFF_ACT;
   col_on_pas  = COL_TR_BAD_ON_PAS;
   col_off_pas = COL_TR_BAD_OFF_PAS;
   hi_on   = hilite(col_on_act);
   hi_off  = hilite(col_off_act);
   xoff = 40;
   cur_trait = cur_dis;
   traitmin = PF_SPLIT + 1;
   traitmax = PF_MAX;
   mvwprintz(w,  3, 33, c_ltgray, "                                              ");
   mvwprintz(w,  3, 33, COL_TR_BAD, "%s earns %d points",
             traits[cur_dis].name.c_str(), traits[cur_dis].points * -1);
   mvwprintz(w_description, 0, 0, COL_TR_BAD, "%s", traits[cur_dis].description.c_str());
  }

  if (cur_trait <= traitmin + 7) {//draw list
   for (int i = traitmin; i < traitmin + 16; i++) {
    mvwprintz(w, 5 + i - traitmin, xoff, c_ltgray, "\
                                      ");	// Clear the line
    if (i == cur_trait) {
     if (u->has_trait(i))
      mvwprintz(w, 5 + i - traitmin, xoff, hi_on, traits[i].name.c_str());
     else
      mvwprintz(w, 5 + i - traitmin, xoff, hi_off, traits[i].name.c_str());
    } else {
     if (u->has_trait(i))
      mvwprintz(w, 5 + i - traitmin, xoff, col_on_act, traits[i].name.c_str());
     else
      mvwprintz(w, 5 + i - traitmin, xoff, col_off_act, traits[i].name.c_str());
    }
   }
  } else if (cur_trait >= traitmax - 9) {
   for (int i = traitmax - 16; i < traitmax; i++) {
    mvwprintz(w, 21 + i - traitmax, xoff, c_ltgray, "\
                                      ");	// Clear the line
    if (i == cur_trait) {
     if (u->has_trait(i))
      mvwprintz(w, 21 + i - traitmax, xoff, hi_on, traits[i].name.c_str());
     else
      mvwprintz(w, 21 + i - traitmax, xoff, hi_off, traits[i].name.c_str());
    } else {
     if (u->has_trait(i))
      mvwprintz(w, 21 + i - traitmax, xoff, col_on_act, traits[i].name.c_str());
     else
      mvwprintz(w, 21 + i - traitmax, xoff, col_off_act, traits[i].name.c_str());
    }
   }
  } else {
   for (int i = cur_trait - 7; i < cur_trait + 9; i++) {
    mvwprintz(w, 12 + i - cur_trait, xoff, c_ltgray, "\
                                     ");	// Clear the line
    if (i == cur_trait) {
     if (u->has_trait(i))
      mvwprintz(w, 12 + i - cur_trait, xoff, hi_on, traits[i].name.c_str());
     else
      mvwprintz(w, 12 + i - cur_trait, xoff, hi_off, traits[i].name.c_str());
    } else {
     if (u->has_trait(i))
      mvwprintz(w, 12 + i - cur_trait, xoff, col_on_act, traits[i].name.c_str());
     else
      mvwprintz(w, 12 + i - cur_trait, xoff, col_off_act, traits[i].name.c_str());
    }
   }
  }

  wrefresh(w);
  wrefresh(w_description);
  switch (input()) {
   case 'h':
   case 'l':
   case '\t':
    if (cur_trait <= traitmin + 7) {//draw list
     for (int i = traitmin; i < traitmin + 16; i++) {
      mvwprintz(w, 5 + i - traitmin, xoff, c_ltgray, "\
                                     ");	// Clear the line
      if (u->has_trait(i))
       mvwprintz(w, 5 + i - traitmin, xoff, col_on_pas, traits[i].name.c_str());
      else
       mvwprintz(w, 5 + i - traitmin, xoff, col_off_pas, traits[i].name.c_str());
     }
    } else if (cur_trait >= traitmax - 9) {
     for (int i = traitmax - 16; i < traitmax; i++) {
      mvwprintz(w, 21 + i - traitmax, xoff, c_ltgray, "\
                                     ");	// Clear the line
      if (u->has_trait(i))
       mvwprintz(w, 21 + i - traitmax, xoff, col_on_pas, traits[i].name.c_str());
      else
       mvwprintz(w, 21 + i - traitmax, xoff, col_off_pas, traits[i].name.c_str());
     }
    } else {
     for (int i = cur_trait - 7; i < cur_trait + 9; i++) {
      mvwprintz(w, 12 + i - cur_trait, xoff, c_ltgray, "\
                                     ");	// Clear the line
      if (u->has_trait(i))
       mvwprintz(w, 12 + i - cur_trait, xoff, col_on_pas, traits[i].name.c_str());
      else
       mvwprintz(w, 12 + i - cur_trait, xoff, col_off_pas, traits[i].name.c_str());
     }
    }
    using_adv = !using_adv;
    wrefresh(w);
    break;
   case 'k':
    if (using_adv) {
     if (cur_adv > 1)
      cur_adv--;
    } else {
     if (cur_dis > PF_SPLIT + 1)
      cur_dis--;
    }
    break;
   case 'j':
   if (using_adv) {
     if (cur_adv < PF_SPLIT - 1)
      cur_adv++;
    } else {
     if (cur_dis < PF_MAX - 1)
      cur_dis++;
    }
    break;
   case ' ':
   case '\n':
    if (u->has_trait(cur_trait)) {
     if (points + traits[cur_trait].points >= 0) {
      u->toggle_trait(cur_trait);
      points += traits[cur_trait].points;
      if (using_adv)
       num_good -= traits[cur_trait].points;
      else
       num_bad += traits[cur_trait].points;
     } else
      mvwprintz(w,  3, 2, c_red, "Points left: %d  ", points);
    } else if (using_adv && num_good + traits[cur_trait].points >
                            max_trait_points)
     popup("Sorry, but you can only take %d points of advantages.",
           max_trait_points);
    else if (!using_adv && num_bad - traits[cur_trait].points >
                           max_trait_points)
     popup("Sorry, but you can only take %d points of disadvantages.",
           max_trait_points);
    else if (points >= traits[cur_trait].points) {
     u->toggle_trait(cur_trait);
     points -= traits[cur_trait].points;
     if (using_adv)
      num_good += traits[cur_trait].points;
     else
      num_bad -= traits[cur_trait].points;
    }
    break;
   case '<':
    return -1;
   case '>':
    return 1;
  }
 } while (true);
}

int set_profession(WINDOW* w, game* g, player *u, int &points)
{
    draw_tabs(w, "PROFESSION");

    WINDOW* w_description = newwin(3, 78, 21 + getbegy(w), 1 + getbegx(w));

    int cur_id = 1;
    int retval = 0;

    //may as well stick that +1 on for convenience
    profession const** sorted_profs = new profession const*[profession::count()+1];
    for (profmap::const_iterator iter = profession::begin(); iter != profession::end(); ++iter)
    {
        profession const* prof = &(iter->second);
        sorted_profs[prof->id()] = prof;
    }

    do
    {
        int netPointCost = sorted_profs[cur_id]->point_cost() - u->prof->point_cost();
        mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);
        // Clear the bottom of the screen.
        mvwprintz(w_description, 0, 0, c_ltgray, "\
                                                                             ");
        mvwprintz(w_description, 1, 0, c_ltgray, "\
                                                                             ");
        mvwprintz(w_description, 2, 0, c_ltgray, "\
                                                                             ");
        mvwprintz(w,  3, 40, c_ltgray, "                                    ");
        if (points >= netPointCost)
        {
            mvwprintz(w,  3, 20, c_green, "Profession %s costs %d points (net: %d)",
                      sorted_profs[cur_id]->name().c_str(), sorted_profs[cur_id]->point_cost(),
                      netPointCost);
        }
        else
        {
            mvwprintz(w,  3, 20, c_ltred, "Profession %s costs %d points (net: %d)",
                      sorted_profs[cur_id]->name().c_str(), sorted_profs[cur_id]->point_cost(),
                      netPointCost);
        }
        mvwprintz(w_description, 0, 0, c_green, sorted_profs[cur_id]->description().c_str());

        for (int i = 1; i < 17; ++i)
        {
            mvwprintz(w, 4 + i, 2, c_ltgray, "\
                                             ");	// Clear the line
            int id = i;
            if ((cur_id < 7) || (profession::count() < 16))
            {
                //do nothing
            }
            else if (cur_id >= profession::count() - 9)
            {
                id = profession::count() - 16 + i;
            }
            else
            {
                id += cur_id - 7;
            }

            if (id > profession::count())
            {
                break;
            }

            if (u->prof != sorted_profs[id])
            {
                mvwprintz(w, 4 + i, 2, (sorted_profs[id] == sorted_profs[cur_id] ? h_ltgray : c_ltgray),
                          sorted_profs[id]->name().c_str());
            }
            else
            {
                mvwprintz(w, 4 + i, 2,
                          (sorted_profs[id] == sorted_profs[cur_id] ? hilite(COL_SKILL_USED) : COL_SKILL_USED),
                          sorted_profs[id]->name().c_str());
            }
        }

        wrefresh(w);
        wrefresh(w_description);
        switch (input())
        {
            case 'j':
                if (cur_id < profession::count())
                cur_id++;
            break;

            case 'k':
                if (cur_id > 1)
                cur_id--;
            break;

            case '\n':
                if (netPointCost <= points) {
                    u->prof = profession::prof(sorted_profs[cur_id]->ident()); // we've got a const*
                    points -= netPointCost;
                }
            break;

            case '<':
                retval = -1;
            break;

            case '>':
                retval = 1;
            break;
        }
    } while (retval == 0);

    delete[] sorted_profs;
    return retval;
}

int set_skills(WINDOW* w, game* g, player *u, int &points)
{
 draw_tabs(w, "SKILLS");

 WINDOW* w_description = newwin(3, 78, 21 + getbegy(w), 1 + getbegx(w));

 int cur_sk = 0;
 Skill *currentSkill = Skill::skill(cur_sk);

 do {
  mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);
// Clear the bottom of the screen.
  mvwprintz(w_description, 0, 0, c_ltgray, "\
                                                                             ");
  mvwprintz(w_description, 1, 0, c_ltgray, "\
                                                                             ");
  mvwprintz(w_description, 2, 0, c_ltgray, "\
                                                                             ");
  mvwprintz(w,  3, 40, c_ltgray, "                                    ");
  if (points >= u->skillLevel(currentSkill) + 1)
   mvwprintz(w,  3, 30, COL_SKILL_USED, "Upgrading %s costs %d points",
             currentSkill->name().c_str(), u->skillLevel(currentSkill) + 1);
  else
   mvwprintz(w,  3, 30, c_ltred, "Upgrading %s costs %d points",
             currentSkill->name().c_str(), u->skillLevel(currentSkill) + 1);
  mvwprintz(w_description, 0, 0, COL_SKILL_USED, currentSkill->description().c_str());

  if (cur_sk <= 7) {
   for (int i = 0; i < 17; i++) {
     Skill *thisSkill = Skill::skill(i);

    mvwprintz(w, 5 + i, 2, c_ltgray, "\
                                             ");	// Clear the line
    if (u->skillLevel(thisSkill) == 0) {
     mvwprintz(w, 5 + i, 2, (i == cur_sk ? h_ltgray : c_ltgray),
               thisSkill->name().c_str());
    } else {
     mvwprintz(w, 5 + i, 2,
               (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED),
               "%s ", thisSkill->name().c_str());
     for (int j = 0; j < u->skillLevel(thisSkill); j++)
      wprintz(w, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
    }
   }
  } else if (cur_sk >= Skill::skills.size() - 9) {
   for (int i = Skill::skills.size() - 16; i < Skill::skills.size(); i++) {
     Skill *thisSkill = Skill::skill(i);
    mvwprintz(w, 21 + i - Skill::skills.size(), 2, c_ltgray, "\
                                             ");	// Clear the line
    if (u->skillLevel(thisSkill) == 0) {
     mvwprintz(w, 21 + i - Skill::skills.size(), 2,
               (i == cur_sk ? h_ltgray : c_ltgray), thisSkill->name().c_str());
    } else {
     mvwprintz(w, 21 + i - Skill::skills.size(), 2,
               (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "%s ",
               thisSkill->name().c_str());
     for (int j = 0; j < u->skillLevel(thisSkill); j++)
      wprintz(w, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
    }
   }
  } else {
   for (int i = cur_sk - 7; i < cur_sk + 9; i++) {
     Skill *thisSkill = Skill::skill(i);
    mvwprintz(w, 12 + i - cur_sk, 2, c_ltgray, "\
                                             ");	// Clear the line
    if (u->skillLevel(thisSkill) == 0) {
     mvwprintz(w, 12 + i - cur_sk, 2, (i == cur_sk ? h_ltgray : c_ltgray),
               thisSkill->name().c_str());
    } else {
     mvwprintz(w, 12 + i - cur_sk, 2,
               (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED),
               "%s ", thisSkill->name().c_str());
     for (int j = 0; j < u->skillLevel(thisSkill); j++)
      wprintz(w, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
    }
   }
  }

  wrefresh(w);
  wrefresh(w_description);
  switch (input()) {
   case 'j':
     if (cur_sk < Skill::skills.size() - 1)
      cur_sk++;
    currentSkill = Skill::skill(cur_sk);
    break;
   case 'k':
    if (cur_sk > 0)
     cur_sk--;
    currentSkill = Skill::skill(cur_sk);
    break;
   case 'h':
     if (u->skillLevel(currentSkill)) {
      u->skillLevel(currentSkill).level(u->skillLevel(currentSkill) - 2);
      points += u->skillLevel(currentSkill) + 1;
    }
    break;
   case 'l':
     if (points >= u->skillLevel(currentSkill) + 1) {
       points -= u->skillLevel(currentSkill) + 1;
       u->skillLevel(currentSkill).level(u->skillLevel(currentSkill) + 2);
    }
    break;
   case '<':
    return -1;
   case '>':
    return 1;
  }
 } while (true);
}

int set_description(WINDOW* w, game* g, player *u, int &points)
{
 draw_tabs(w, "DESCRIPTION");

 mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);

 mvwprintz(w, 6, 2, c_ltgray, "\
Name: ______________________________     (Press TAB to move off this line)");
 mvwprintz(w, 8, 2, c_ltgray, "\
Gender: Male Female                      (Press spacebar to toggle)");
 mvwprintz(w,10, 2, c_ltgray, "\
When your character is finished and you're ready to start playing, press >");
 mvwprintz(w,12, 2, c_ltgray, "\
To go back and review your character, press <");
 mvwprintz(w, 14, 2, c_green, "\
To pick a random name for your character, press ?.");
 mvwprintz(w, 16, 2, c_green, "\
To save this character as a template, press !.");

 int line = 1;
 bool noname = false;
 long ch;

 do {
  if (u->male) {
   mvwprintz(w, 8, 10, c_ltred, "Male");
   mvwprintz(w, 8, 15, c_ltgray, "Female");
  } else {
   mvwprintz(w, 8, 10, c_ltgray, "Male");
   mvwprintz(w, 8, 15, c_ltred, "Female");
  }

  if (!noname) {
   mvwprintz(w, 6, 8, c_ltgray, u->name.c_str());
   if (line == 1)
    wprintz(w, h_ltgray, "_");
  }
  if (line == 2)
   mvwprintz(w, 8, 2, h_ltgray, "Gender:");
  else
   mvwprintz(w, 8, 2, c_ltgray, "Gender:");

  wrefresh(w);
  ch = input();
  if (noname) {
   mvwprintz(w, 6, 8, c_ltgray, "______________________________");
   noname = false;
  }

  if (ch == '>') {
   if (points > 0 && !query_yn("Remaining points will be discarded, are you sure you want to proceed?")) {
    continue;
   } else if (u->name.size() == 0) {
    mvwprintz(w, 6, 8, h_ltgray, "______NO NAME ENTERED!!!!_____");
    noname = true;
    wrefresh(w);
    if (!query_yn("Are you SURE you're finished? Your name will be randomly generated.")) {
     continue;
    } else {
     u->pick_name();
     return 1;
    }
   } else if (query_yn("Are you SURE you're finished?")) {
    return 1;
   } else {
    continue;
   }
  } else if (ch == '<') {
   return -1;
  } else if (ch == '!') {
   if (points > 0) {
    popup("You cannot save a template with unused points!");
   } else
    save_template(u);
   mvwprintz(w,12, 2, c_ltgray,"To go back and review your character, press <");
   wrefresh(w);
  } else if (ch == '?') {
   mvwprintz(w, 6, 8, c_ltgray, "______________________________");
   u->pick_name();
  } else {
   switch (line) {
    case 1:
     if (ch == KEY_BACKSPACE || ch == 127) {
      if (u->name.size() > 0) {
       mvwprintz(w, 6, 8 + u->name.size(), c_ltgray, "_");
       u->name.erase(u->name.end() - 1);
      }
     } else if (ch == '\t') {
      line = 2;
      mvwprintz(w, 6, 8 + u->name.size(), c_ltgray, "_");
     } else if (((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                  ch == ' ') && u->name.size() < 30) {
      u->name.push_back(ch);
     }
     break;
    case 2:
     if (ch == ' ')
      u->male = !u->male;
     else if (ch == 'k' || ch == '\t') {
      line = 1;
      mvwprintz(w, 8, 8, c_ltgray, ":");
     }
     break;
   }
  }
 } while (true);
}

int player::random_good_trait()
{
 return rng(1, PF_SPLIT - 1);
}

int player::random_bad_trait()
{
 return rng(PF_SPLIT + 1, PF_MAX - 1);
}

int random_skill()
{
  return rng(1, Skill::skills.size() - 1);
}

int calc_HP(int strength, bool tough)
{
 return (60 + 3 * strength) * (tough ? 1.2 : 1);
}

void save_template(player *u)
{
 std::string name = string_input_popup("Name of template:");
 if (name.length() == 0)
  return;
 std::stringstream playerfile;
 playerfile << "data/" << name << ".template";
 std::ofstream fout;
 fout.open(playerfile.str().c_str());
 fout << u->save_info();
}

// vim:tw=80:sw=4:ts=4:et:fdm=marker:fdl=0:
