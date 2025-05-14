/*
 *  This file contains our game's item database - all the cool stuff your units can use!
 */

#include "data.h"

// Here's our complete list of items that units can equip
// They're organized by how many inventory slots they take up
const ITEM items[NUMBER_OF_ITEMS] = {
    {"Sword", 10, 5, 1, 1, 0},
    {"Shield", 0, 15, 1, 0, 0},
    {"Bow", 8, 0, 1, 3, 0},
    {"Staff", 5, 5, 1, 2, 0},
    {"Axe", 12, 3, 1, 1, 0},
    {"Armor", 0, 20, 2, 0, 0},
    {"Dagger", 7, 2, 1, 1, 0},
    {"Spear", 9, 4, 1, 2, 0},
    {"Wand", 6, 0, 1, 3, 0},
    {"Hammer", 11, 6, 2, 1, 0},
    {"Crossbow", 10, 0, 2, 4, 0},
    {"Mace", 8, 7, 1, 1, 0},
    {"Greatsword", 15, 8, 2, 1, 0},
    {"Fireball Staff", 12, 0, 2, 3, 1},
    {"Ice Staff", 8, 0, 2, 3, 2},
    {"Lightning Rod", 14, 0, 2, 2, 1}
};
