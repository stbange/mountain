#ifndef MOUNTAIN_KITCHEN_H
#define MOUNTAIN_KITCHEN_H
#include <stdint.h>
#include <stdbool.h>
/* IDs in the loose cookbook are stable across reordering. Zero means an empty slot. */
#define PANTRY_ITEMS 384
#define COOKBOOK_RECIPES 512
#define PICKED_PATCHES 128
typedef enum {
    PATCH_BERRY,
    PATCH_HERB,
    PATCH_CAP,
    PATCH_FRUIT,
    PATCH_SEED,
    PATCH_ROOT,
    PATCH_WEED,
    PATCH_POD,
    PATCH_RELAY,
    DISH_CUP,
    DISH_COOKIES,
    DISH_BAKE,
    DISH_BOWL,
    DISH_TEA,
    DISH_GLASS,
    KITCHEN_MODELS
} KitchenModel;
static inline bool kitchen_beverage(KitchenModel kind) {
    return kind == DISH_CUP || kind == DISH_TEA || kind == DISH_GLASS;
}
typedef enum { FORAGE_OPEN, FORAGE_SHELTER, FORAGE_WATER } ForageHabitat;
typedef struct {
    char name[48], habitat[58];
    int region, season;
    ForageHabitat niche;
    KitchenModel form;
} Ingredient;
typedef struct {
    char name[68], method[14], staples[64];
    int region;
    KitchenModel serving;
    uint16_t need[3];
} Recipe;
typedef struct {
    Ingredient ingredients[PANTRY_ITEMS + 1];
    Recipe recipes[COOKBOOK_RECIPES + 1];
    int item_max, recipe_max, items, count, coffee, cookies, house[16];
    int regional[16][PANTRY_ITEMS], regional_count[16];
    int methods[COOKBOOK_RECIPES], method_count;
    struct Mesh *models;
} Cookbook;
typedef struct {
    uint32_t seed;
    int region;
    int64_t x, z;
    double at;
} Harvest;
typedef struct {
    uint16_t stock[PANTRY_ITEMS + 1], portions[COOKBOOK_RECIPES + 1];
    uint8_t found[PANTRY_ITEMS + 1], learned[COOKBOOK_RECIPES + 1];
    Harvest picked[PICKED_PATCHES];
    unsigned picks, tableware, setting;
    int drink, food, pin, custom_food, custom_drink;
    double distance;
    uint64_t revision;
} Pantry;
typedef struct {
    int64_t x, z;
    int item;
    float px, pz;
    bool ripe;
} ForagePatch;
typedef enum {
    SHELF_LOCAL,
    SHELF_READY,
    SHELF_DRINKS,
    SHELF_TRAVEL,
    SHELF_ALL,
    SHELF_COUNT
} RecipeShelf;
#endif
