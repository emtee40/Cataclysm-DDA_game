#pragma once
#ifndef CRAFTING_H
#define CRAFTING_H

#include <list>
#include <vector>

class item;
class inventory;
class npc;
class player;
class recipe;

// removes any (removable) ammo from the item and stores it in the
// players inventory.
void remove_ammo( item &dis_item, player &p );
// same as above but for each item in the list
void remove_ammo( std::list<item> &dis_items, player &p );

const recipe *select_crafting_recipe( int &batch_size );

void set_item_food( item &newit );
void set_item_inventory( item &newit );

#endif
