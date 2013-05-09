#include "game.h"
#include "setvector.h"

#define MUTATION(mut) id = mut; mutation_data[id].valid = true

#define PREREQS(...) \
  setvector(&(mutation_data[id].prereqs), __VA_ARGS__, NULL)

#define CANCELS(...) \
  setvector(&(mutation_data[id].cancels), __VA_ARGS__, NULL)

#define CHANGES_TO(...) \
  setvector(&(mutation_data[id].replacements), __VA_ARGS__, NULL)

#define LEADS_TO(...) \
  setvector(&(mutation_data[id].additions), __VA_ARGS__, NULL)

void game::init_mutations()
{
 int id = 0;
// Set all post-PF_MAX to valid
 for (int i = PF_MAX + 1; i < PF_MAX2; i++)
  mutation_data[i].valid = true;

 mutation_data[PF_MARLOSS].valid = false; // Never valid!

 MUTATION(PF_FLEET);
  CANCELS (PF_PONDEROUS1, PF_PONDEROUS2, PF_PONDEROUS3);
  CHANGES_TO (PF_FLEET2);

 MUTATION(PF_FLEET2);
  PREREQS (PF_FLEET);
  CANCELS (PF_PONDEROUS1, PF_PONDEROUS2, PF_PONDEROUS3);

 MUTATION(PF_FASTHEALER);
  CANCELS (PF_ROT1, PF_ROT2, PF_ROT3);
  CHANGES_TO (PF_FASTHEALER2);

 MUTATION(PF_LIGHTEATER);

 MUTATION(PF_NIGHTVISION);
  CHANGES_TO (PF_NIGHTVISION2);

 MUTATION(PF_POISRESIST);

 MUTATION(PF_THICKSKIN);

 MUTATION(PF_DEFT);

 MUTATION(PF_ANIMALEMPATH);

 MUTATION(PF_TERRIFYING);

 MUTATION(PF_DISRESISTANT);
  CHANGES_TO (PF_DISIMMUNE);

 MUTATION(PF_ROBUST);

 MUTATION(PF_ADRENALINE);

 MUTATION(PF_PRETTY);
  CANCELS (PF_UGLY, PF_DEFORMED, PF_DEFORMED2, PF_DEFORMED3);
  CHANGES_TO (PF_BEAUTIFUL);

 MUTATION(PF_LIAR);
  CANCELS (PF_TRUTHTELLER);

 MUTATION(PF_MYOPIC);

 MUTATION(PF_HEAVYSLEEPER);

 MUTATION(PF_LIGHTWEIGHT);

 MUTATION(PF_SMELLY);
  CHANGES_TO (PF_SMELLY2);

 MUTATION(PF_WEAKSTOMACH);
  CHANGES_TO (PF_NAUSEA);

 MUTATION(PF_UGLY);
  CANCELS (PF_PRETTY, PF_BEAUTIFUL, PF_BEAUTIFUL2, PF_BEAUTIFUL3);
  CHANGES_TO (PF_DEFORMED);

 MUTATION(PF_TRUTHTELLER);
  CANCELS (PF_LIAR);


// Mutation-only traits start here

 MUTATION(PF_HOOVES);
  CANCELS (PF_PADDED_FEET, PF_LEG_TENTACLES);

 MUTATION(PF_SKIN_ROUGH);
  CHANGES_TO (PF_SCALES, PF_FEATHERS, PF_LIGHTFUR, PF_CHITIN, PF_PLANTSKIN);

 MUTATION(PF_NIGHTVISION2);
  PREREQS (PF_NIGHTVISION);
  CHANGES_TO (PF_NIGHTVISION3);

 MUTATION(PF_NIGHTVISION3);
  PREREQS (PF_NIGHTVISION2);
  LEADS_TO (PF_INFRARED);

 MUTATION(PF_FASTHEALER2);
  CANCELS (PF_ROT1, PF_ROT2, PF_ROT3);
  PREREQS (PF_FASTHEALER);
  CHANGES_TO (PF_REGEN);

 MUTATION(PF_REGEN);
  CANCELS (PF_ROT1, PF_ROT2, PF_ROT3);
  PREREQS (PF_FASTHEALER2);

 MUTATION(PF_FANGS);
  CANCELS (PF_BEAK);

 MUTATION(PF_MEMBRANE);
  PREREQS (PF_EYEBULGE);

 MUTATION(PF_SCALES);
  PREREQS (PF_SKIN_ROUGH);
  CHANGES_TO (PF_THICK_SCALES, PF_SLEEK_SCALES);

 MUTATION(PF_THICK_SCALES);
  PREREQS (PF_SCALES);
  CANCELS (PF_SLEEK_SCALES, PF_FEATHERS);

 MUTATION(PF_SLEEK_SCALES);
  PREREQS (PF_SCALES);
  CANCELS (PF_THICK_SCALES, PF_FEATHERS);

 MUTATION(PF_LIGHT_BONES);
  CHANGES_TO (PF_HOLLOW_BONES);

 MUTATION(PF_FEATHERS);
  PREREQS (PF_SKIN_ROUGH);
  CANCELS(PF_THICK_SCALES, PF_SLEEK_SCALES);

 MUTATION(PF_LIGHTFUR);
  PREREQS (PF_SKIN_ROUGH);
  CANCELS (PF_SCALES, PF_FEATHERS, PF_CHITIN, PF_PLANTSKIN);
  CHANGES_TO (PF_FUR);

 MUTATION(PF_FUR);
  CANCELS (PF_SCALES, PF_FEATHERS, PF_CHITIN, PF_PLANTSKIN);
  PREREQS (PF_LIGHTFUR);

 MUTATION(PF_CHITIN);
  CANCELS (PF_SCALES, PF_FEATHERS, PF_LIGHTFUR, PF_PLANTSKIN);
  CHANGES_TO (PF_CHITIN2);

 MUTATION(PF_CHITIN2);
  PREREQS (PF_CHITIN);
  CHANGES_TO (PF_CHITIN3);

 MUTATION(PF_CHITIN3);
  PREREQS (PF_CHITIN2);

 MUTATION(PF_SPINES);
  CHANGES_TO (PF_QUILLS);

 MUTATION (PF_QUILLS);
  PREREQS (PF_SPINES);

 MUTATION (PF_PLANTSKIN);
  CANCELS (PF_FEATHERS, PF_LIGHTFUR, PF_FUR, PF_CHITIN, PF_CHITIN2, PF_CHITIN3,
           PF_SCALES);
  CHANGES_TO (PF_BARK);
  LEADS_TO (PF_THORNS, PF_LEAVES);

 MUTATION(PF_BARK);
  PREREQS (PF_PLANTSKIN);

 MUTATION(PF_THORNS);
  PREREQS (PF_PLANTSKIN, PF_BARK);

 MUTATION(PF_LEAVES);
  PREREQS (PF_PLANTSKIN, PF_BARK);

 MUTATION(PF_NAILS);
  CHANGES_TO (PF_CLAWS, PF_TALONS);

 MUTATION(PF_CLAWS);
  PREREQS (PF_NAILS);
  CANCELS (PF_TALONS);

 MUTATION(PF_TALONS);
  PREREQS (PF_NAILS);
  CANCELS (PF_CLAWS);

 MUTATION(PF_PHEROMONE_INSECT);
  PREREQS (PF_SMELLY2);
  CANCELS (PF_PHEROMONE_MAMMAL);

 MUTATION(PF_PHEROMONE_MAMMAL);
  PREREQS (PF_SMELLY2);
  CANCELS (PF_PHEROMONE_INSECT);

 MUTATION(PF_DISIMMUNE);
  PREREQS (PF_DISRESISTANT);

 MUTATION(PF_POISONOUS);
  PREREQS (PF_POISRESIST);

 MUTATION(PF_SLIME_HANDS);
  PREREQS (PF_SLIMY);

 MUTATION(PF_COMPOUND_EYES);
  PREREQS (PF_EYEBULGE);

 MUTATION(PF_PADDED_FEET);
  CANCELS (PF_HOOVES, PF_LEG_TENTACLES);

 MUTATION(PF_HOOVES);
  CANCELS (PF_PADDED_FEET);

 MUTATION(PF_SAPROVORE);
  PREREQS (PF_CARNIVORE);
  CANCELS (PF_HERBIVORE, PF_RUMINANT);

 MUTATION(PF_RUMINANT);
  PREREQS (PF_HERBIVORE);
  CANCELS (PF_CARNIVORE, PF_SAPROVORE);

 MUTATION(PF_HORNS);
  PREREQS (PF_HEADBUMPS);
  CANCELS (PF_ANTENNAE);
  CHANGES_TO (PF_HORNS_CURLED, PF_HORNS_POINTED, PF_ANTLERS);

 MUTATION(PF_HORNS_CURLED);
  PREREQS (PF_HORNS);
  CANCELS (PF_ANTENNAE, PF_HORNS_POINTED, PF_ANTLERS);

 MUTATION(PF_HORNS_POINTED);
  PREREQS (PF_HORNS);
  CANCELS (PF_ANTENNAE, PF_HORNS_CURLED, PF_ANTLERS);

 MUTATION(PF_ANTLERS);
  PREREQS (PF_HORNS);
  CANCELS (PF_ANTENNAE, PF_HORNS_CURLED, PF_HORNS_POINTED);

 MUTATION(PF_ANTENNAE);
  PREREQS (PF_HEADBUMPS);
  CANCELS (PF_HORNS, PF_HORNS_CURLED, PF_HORNS_POINTED, PF_ANTLERS);

 MUTATION(PF_TAIL_STUB);
  CHANGES_TO (PF_TAIL_LONG, PF_TAIL_FIN);

 MUTATION(PF_TAIL_FIN);
  PREREQS (PF_TAIL_STUB);
  CANCELS (PF_TAIL_LONG, PF_TAIL_FLUFFY, PF_TAIL_STING, PF_TAIL_CLUB);

 MUTATION(PF_TAIL_LONG);
  PREREQS (PF_TAIL_STUB);
  CANCELS (PF_TAIL_FIN);
  CHANGES_TO (PF_TAIL_FLUFFY, PF_TAIL_STING, PF_TAIL_CLUB);

 MUTATION(PF_TAIL_FLUFFY);
  PREREQS (PF_TAIL_LONG);
  CANCELS (PF_TAIL_STING, PF_TAIL_CLUB, PF_TAIL_FIN);

 MUTATION(PF_TAIL_STING);
  PREREQS (PF_TAIL_LONG);
  CANCELS (PF_TAIL_FLUFFY, PF_TAIL_CLUB, PF_TAIL_FIN);

 MUTATION(PF_TAIL_CLUB);
  PREREQS (PF_TAIL_LONG);
  CANCELS (PF_TAIL_FLUFFY, PF_TAIL_STING, PF_TAIL_FIN);

 MUTATION(PF_PAINREC1);
  CHANGES_TO (PF_PAINREC2);

 MUTATION(PF_PAINREC2);
  PREREQS (PF_PAINREC1);
  CHANGES_TO (PF_PAINREC3);

 MUTATION(PF_PAINREC3);
  PREREQS (PF_PAINREC2);

 MUTATION(PF_MOUTH_TENTACLES);
  PREREQS (PF_MOUTH_FLAPS);
  CANCELS (PF_MANDIBLES);

 MUTATION(PF_MANDIBLES);
  PREREQS (PF_MOUTH_FLAPS);
  CANCELS (PF_BEAK, PF_FANGS, PF_MOUTH_TENTACLES);

 MUTATION(PF_WEB_WALKER);
  LEADS_TO (PF_WEB_WEAVER);

 MUTATION(PF_WEB_WEAVER);
  PREREQS (PF_WEB_WALKER);
  CANCELS (PF_SLIMY);

 MUTATION(PF_STR_UP);
  CHANGES_TO (PF_STR_UP_2);

 MUTATION(PF_STR_UP_2);
  PREREQS (PF_STR_UP);
  CHANGES_TO (PF_STR_UP_3);

 MUTATION(PF_STR_UP_3);
  PREREQS (PF_STR_UP_2);
  CHANGES_TO (PF_STR_UP_4);

 MUTATION(PF_STR_UP_4);
  PREREQS (PF_STR_UP_3);

 MUTATION(PF_DEX_UP);
  CHANGES_TO (PF_DEX_UP_2);

 MUTATION(PF_DEX_UP_2);
  PREREQS (PF_DEX_UP);
  CHANGES_TO (PF_DEX_UP_3);

 MUTATION(PF_DEX_UP_3);
  PREREQS (PF_DEX_UP_2);
  CHANGES_TO (PF_DEX_UP_4);

 MUTATION(PF_DEX_UP_4);
  PREREQS (PF_DEX_UP_3);

 MUTATION(PF_INT_UP);
  CHANGES_TO (PF_INT_UP_2);

 MUTATION(PF_INT_UP_2);
  PREREQS (PF_INT_UP);
  CHANGES_TO (PF_INT_UP_3);

 MUTATION(PF_INT_UP_3);
  PREREQS (PF_INT_UP_2);
  CHANGES_TO (PF_INT_UP_4);

 MUTATION(PF_INT_UP_4);
  PREREQS (PF_INT_UP_3);

 MUTATION(PF_PER_UP);
  CHANGES_TO (PF_PER_UP_2);

 MUTATION(PF_PER_UP_2);
  PREREQS (PF_PER_UP);
  CHANGES_TO (PF_PER_UP_3);

 MUTATION(PF_PER_UP_3);
  PREREQS (PF_PER_UP_2);
  CHANGES_TO (PF_PER_UP_4);

 MUTATION(PF_PER_UP_4);
  PREREQS (PF_PER_UP_3);


// Bad mutations below this point

 MUTATION(PF_HEADBUMPS);
  CHANGES_TO (PF_HORNS, PF_ANTENNAE);

 MUTATION(PF_EYEBULGE);
  LEADS_TO (PF_MEMBRANE);
  CHANGES_TO (PF_COMPOUND_EYES);

 MUTATION(PF_PALE);
  CHANGES_TO (PF_ALBINO);
  LEADS_TO (PF_TROGLO);

 MUTATION(PF_SPOTS);
  CHANGES_TO (PF_SORES);

 MUTATION(PF_SMELLY2);
  PREREQS (PF_SMELLY);
  LEADS_TO (PF_PHEROMONE_INSECT, PF_PHEROMONE_MAMMAL);

 MUTATION(PF_DEFORMED);
  CANCELS (PF_PRETTY, PF_BEAUTIFUL, PF_BEAUTIFUL2, PF_BEAUTIFUL3);
  PREREQS (PF_UGLY);
  CHANGES_TO (PF_DEFORMED2);

 MUTATION(PF_DEFORMED2);
  CANCELS (PF_PRETTY, PF_BEAUTIFUL, PF_BEAUTIFUL2, PF_BEAUTIFUL3);
  PREREQS (PF_DEFORMED);
  CHANGES_TO (PF_DEFORMED3);

 MUTATION(PF_DEFORMED3);
  CANCELS (PF_PRETTY, PF_BEAUTIFUL, PF_BEAUTIFUL2, PF_BEAUTIFUL3);
  PREREQS (PF_DEFORMED2);

 MUTATION(PF_BEAUTIFUL);
  CANCELS (PF_UGLY, PF_DEFORMED, PF_DEFORMED2, PF_DEFORMED3);
  PREREQS (PF_PRETTY);
  CHANGES_TO (PF_BEAUTIFUL2);

 MUTATION(PF_BEAUTIFUL2);
  CANCELS (PF_UGLY, PF_DEFORMED, PF_DEFORMED2, PF_DEFORMED3);
  PREREQS (PF_BEAUTIFUL);
  CHANGES_TO (PF_BEAUTIFUL3);

 MUTATION(PF_BEAUTIFUL3);
  CANCELS (PF_UGLY, PF_DEFORMED, PF_DEFORMED2, PF_DEFORMED3);
  PREREQS (PF_BEAUTIFUL2);

 MUTATION(PF_HOLLOW_BONES);
  PREREQS (PF_LIGHT_BONES);

 MUTATION(PF_NAUSEA);
  PREREQS (PF_WEAKSTOMACH);
  CHANGES_TO (PF_VOMITOUS);

 MUTATION(PF_VOMITOUS);
  PREREQS (PF_NAUSEA);

 MUTATION(PF_ROT1);
  CANCELS (PF_FASTHEALER, PF_FASTHEALER2, PF_REGEN);
  CHANGES_TO (PF_ROT2);

 MUTATION(PF_ROT2);
  CANCELS (PF_FASTHEALER, PF_FASTHEALER2, PF_REGEN);
  PREREQS (PF_ROT1);
  CHANGES_TO (PF_ROT3);

 MUTATION(PF_ROT3);
  CANCELS (PF_FASTHEALER, PF_FASTHEALER2, PF_REGEN);
  PREREQS (PF_ROT2);

 MUTATION(PF_ALBINO);
  PREREQS (PF_PALE);

 MUTATION(PF_SORES);
  PREREQS (PF_SPOTS);

 MUTATION(PF_TROGLO);
  CANCELS (PF_SUNLIGHT_DEPENDENT);
  CHANGES_TO (PF_TROGLO2);

 MUTATION(PF_TROGLO2);
  CANCELS (PF_SUNLIGHT_DEPENDENT);
  PREREQS (PF_TROGLO);
  CHANGES_TO (PF_TROGLO3);

 MUTATION(PF_TROGLO3);
  CANCELS (PF_SUNLIGHT_DEPENDENT);
  PREREQS (PF_TROGLO2);

 MUTATION(PF_BEAK);
  CANCELS (PF_FANGS, PF_MANDIBLES);

 MUTATION(PF_RADIOACTIVE1);
  CHANGES_TO (PF_RADIOACTIVE2);

 MUTATION(PF_RADIOACTIVE2);
  PREREQS (PF_RADIOACTIVE1);
  CHANGES_TO (PF_RADIOACTIVE3);

 MUTATION(PF_RADIOACTIVE3);
  PREREQS (PF_RADIOACTIVE2);

 MUTATION(PF_SLIMY);
  LEADS_TO (PF_SLIME_HANDS);

 MUTATION(PF_CARNIVORE);
  CANCELS (PF_HERBIVORE, PF_RUMINANT);
  LEADS_TO (PF_SAPROVORE);

 MUTATION(PF_HERBIVORE);
  CANCELS (PF_CARNIVORE, PF_SAPROVORE);
  LEADS_TO (PF_RUMINANT);

 MUTATION(PF_PONDEROUS1);
  CANCELS (PF_FLEET, PF_FLEET2);
  CHANGES_TO (PF_PONDEROUS2);

 MUTATION(PF_PONDEROUS2);
  CANCELS (PF_FLEET, PF_FLEET2);
  PREREQS (PF_PONDEROUS1);
  CHANGES_TO (PF_PONDEROUS3);

 MUTATION(PF_PONDEROUS3);
  CANCELS (PF_FLEET, PF_FLEET2);
  PREREQS (PF_PONDEROUS2);

 MUTATION(PF_SUNLIGHT_DEPENDENT);
  CANCELS (PF_TROGLO, PF_TROGLO2, PF_TROGLO3);

 MUTATION(PF_GROWL);
  CHANGES_TO (PF_SNARL);

 MUTATION(PF_SNARL);
  PREREQS (PF_GROWL);

 MUTATION(PF_COLDBLOOD);
  CHANGES_TO (PF_COLDBLOOD2);

 MUTATION(PF_COLDBLOOD2);
  PREREQS (PF_COLDBLOOD);
  CHANGES_TO (PF_COLDBLOOD3);

 MUTATION(PF_COLDBLOOD3);
  PREREQS (PF_COLDBLOOD2);

 MUTATION(PF_SHOUT1);
  CHANGES_TO (PF_SHOUT2);

 MUTATION(PF_SHOUT2);
  PREREQS (PF_SHOUT1);
  CHANGES_TO (PF_SHOUT3);

 MUTATION(PF_SHOUT3);
  PREREQS (PF_SHOUT2);

 MUTATION(PF_WINGS_STUB);
  CHANGES_TO (PF_WINGS_BIRD, PF_WINGS_BAT, PF_WINGS_INSECT);

 MUTATION(PF_WINGS_BIRD);
  PREREQS (PF_WINGS_STUB);
  CANCELS (PF_WINGS_BAT, PF_WINGS_INSECT);

 MUTATION(PF_WINGS_BAT);
  PREREQS (PF_WINGS_STUB);
  CANCELS (PF_WINGS_BIRD, PF_WINGS_INSECT);

 MUTATION(PF_WINGS_INSECT);
  PREREQS (PF_WINGS_STUB);
  CANCELS (PF_WINGS_BIRD, PF_WINGS_BAT);

 MUTATION(PF_ARM_TENTACLES);
  CHANGES_TO (PF_ARM_TENTACLES_4);

 MUTATION(PF_ARM_TENTACLES_4);
  PREREQS (PF_ARM_TENTACLES);
  CHANGES_TO (PF_ARM_TENTACLES_8);

 MUTATION(PF_ARM_TENTACLES_8);
  PREREQS (PF_ARM_TENTACLES_4);

 MUTATION(PF_LEG_TENTACLES);
  CANCELS (PF_PADDED_FEET, PF_HOOVES);

 MUTATION(PF_SHELL);
  PREREQS (PF_CHITIN);
  CANCELS (PF_CHITIN3);


}



std::vector<pl_flag> mutations_from_category(mutation_category cat)
{
 std::vector<pl_flag> ret;
 switch (cat) {

 case MUTCAT_LIZARD:
  setvector(&ret,
PF_THICKSKIN, PF_INFRARED, PF_FANGS, PF_MEMBRANE, PF_SCALES, PF_TALONS,
PF_SLIT_NOSTRILS, PF_FORKED_TONGUE, PF_TROGLO, PF_WEBBED, PF_CARNIVORE,
PF_COLDBLOOD2, PF_STR_UP_2, PF_DEX_UP_2, NULL);
  break;

 case MUTCAT_BIRD:
  setvector(&ret,
PF_QUICK, PF_LIGHTEATER, PF_DEFT, PF_LIGHTSTEP, PF_BADBACK, PF_GLASSJAW,
PF_NIGHTVISION, PF_HOLLOW_BONES, PF_FEATHERS, PF_TALONS, PF_BEAK, PF_FLEET2,
PF_WINGS_BIRD, PF_COLDBLOOD, PF_DEX_UP_3, NULL);
  break;

 case MUTCAT_FISH:
  setvector(&ret,
PF_SMELLY2, PF_NIGHTVISION2, PF_FANGS, PF_MEMBRANE, PF_GILLS, PF_SLEEK_SCALES,
PF_TAIL_FIN, PF_DEFORMED, PF_THIRST, PF_WEBBED, PF_SLIMY, PF_COLDBLOOD2, NULL);
  break;

 case MUTCAT_BEAST:
  setvector(&ret,
PF_DEFT, PF_ANIMALEMPATH, PF_TERRIFYING, PF_ADRENALINE, PF_MYOPIC, PF_FORGETFUL,
PF_NIGHTVISION2, PF_FANGS, PF_FUR, PF_CLAWS, PF_PHEROMONE_MAMMAL,
PF_PADDED_FEET, PF_TAIL_FLUFFY, PF_SMELLY2, PF_DEFORMED2, PF_HUNGER, PF_TROGLO,
PF_CARNIVORE, PF_SNARL, PF_SHOUT3, PF_CANINE_EARS, PF_STR_UP_4, NULL);
  break;

 case MUTCAT_CATTLE:
  setvector(&ret,
PF_THICKSKIN, PF_DISRESISTANT, PF_NIGHTVISION, PF_FUR, PF_PHEROMONE_MAMMAL,
PF_HOOVES, PF_RUMINANT, PF_HORNS, PF_TAIL_LONG, PF_DEFORMED, PF_PONDEROUS2,
PF_CANINE_EARS, PF_STR_UP_2, NULL);

 case MUTCAT_INSECT:
  setvector(&ret,
PF_QUICK, PF_LIGHTEATER, PF_POISRESIST, PF_NIGHTVISION, PF_TERRIFYING,
PF_HEAVYSLEEPER, PF_NIGHTVISION2, PF_INFRARED, PF_CHITIN2, PF_PHEROMONE_INSECT,
PF_ANTENNAE, PF_WINGS_INSECT, PF_TAIL_STING, PF_MANDIBLES, PF_DEFORMED,
PF_TROGLO, PF_COLDBLOOD3, PF_STR_UP, PF_DEX_UP, NULL);
  break;

 case MUTCAT_PLANT:
  setvector(&ret,
PF_DISIMMUNE, PF_HEAVYSLEEPER, PF_BADHEARING, PF_FASTHEALER2, PF_BARK,
PF_THORNS, PF_LEAVES, PF_DEFORMED2, PF_PONDEROUS3, PF_STR_UP_2, NULL);
  break;

 case MUTCAT_SLIME:
  setvector(&ret,
PF_POISRESIST, PF_ROBUST, PF_CHEMIMBALANCE, PF_REGEN, PF_RADIOGENIC,
PF_DISIMMUNE, PF_POISONOUS, PF_SLIME_HANDS, PF_SMELLY2, PF_DEFORMED3,
PF_HOLLOW_BONES, PF_VOMITOUS, PF_HUNGER, PF_THIRST, PF_SORES, PF_TROGLO,
PF_WEBBED, PF_UNSTABLE, PF_RADIOACTIVE1, PF_SLIMY, PF_DEX_UP, PF_INT_UP, NULL);
  break;

 case MUTCAT_TROGLO:
  setvector(&ret,
PF_QUICK, PF_LIGHTEATER, PF_MYOPIC, PF_NIGHTVISION3, PF_INFRARED, PF_REGEN,
PF_DISIMMUNE, PF_POISONOUS, PF_SAPROVORE, PF_SLIT_NOSTRILS, PF_ALBINO,
PF_TROGLO3, PF_SLIMY, NULL);
  break;

 case MUTCAT_CEPHALOPOD:
  setvector(&ret,
PF_GILLS, PF_MOUTH_TENTACLES, PF_SLIT_NOSTRILS, PF_DEFORMED, PF_THIRST,
PF_BEAK, PF_SLIMY, PF_COLDBLOOD, PF_ARM_TENTACLES_8, PF_SHELL, PF_LEG_TENTACLES,
NULL);
  break;

 case MUTCAT_SPIDER:
  setvector(&ret,
PF_FLEET, PF_POISRESIST, PF_NIGHTVISION3, PF_INFRARED, PF_FUR, PF_CHITIN3,
PF_POISONOUS, PF_COMPOUND_EYES, PF_MANDIBLES, PF_WEB_WEAVER, PF_TROGLO,
PF_CARNIVORE, PF_COLDBLOOD, PF_DEX_UP_2, NULL);
  break;

 case MUTCAT_RAT:
  setvector(&ret,
PF_DISRESISTANT, PF_NIGHTVISION2, PF_FANGS, PF_FUR, PF_CLAWS, PF_TAIL_LONG,
PF_WHISKERS, PF_DEFORMED3, PF_VOMITOUS, PF_HUNGER, PF_TROGLO2, PF_GROWL, NULL);
  break;

 }

 return ret;
}
