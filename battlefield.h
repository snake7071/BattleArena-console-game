#ifndef BATTLEFIELD_H
#define BATTLEFIELD_H

#include <ncurses.h>
#include "data.h"

// Minimum window dimensions
#define MIN_WINDOW_WIDTH  80
#define MIN_WINDOW_HEIGHT 24

// Grid dimensions (will scale based on window size)
#define MAX_GRID_WIDTH  10
#define MAX_GRID_HEIGHT 10

// Panel dimensions (will scale based on window size)
#define MIN_STATUS_HEIGHT 8
#define MIN_STATUS_WIDTH 30
#define MIN_UNIT_LIST_HEIGHT 10
#define MIN_UNIT_LIST_WIDTH 35
#define MIN_HINTS_HEIGHT 3
#define MIN_MESSAGE_HEIGHT 3

// Item selection menu dimensions
#define ITEM_MENU_WIDTH 40
#define ITEM_MENU_HEIGHT 15
#define ITEMS_PER_PAGE 5

// Game states for context-sensitive hints
typedef enum {
    STATE_INIT,
    STATE_POSITIONING,    // New state for initial unit positioning
    STATE_SELECT_UNIT,
    STATE_MOVE_UNIT,     // New state for moving selected unit
    STATE_SELECT_ACTION, // New state for choosing action
    STATE_SELECT_TARGET,
    STATE_COMBAT_RESULT,
    STATE_GAME_OVER
} GameState;

// Action types
typedef enum {
    ACTION_MOVE,
    ACTION_ATTACK,
    ACTION_SPECIAL,
    ACTION_END_TURN
} ActionType;

// Action menu
typedef struct {
    WINDOW *win;
    int selected_action;
    bool can_move;
    bool can_attack;
    bool has_special;
} ActionMenu;

// Grid cell structure
typedef struct {
    UNIT *unit;
    int team;  // 1 or 2
} GridCell;

typedef struct {
    int x, y;
} Position;

typedef struct {
    int width;          // Actual grid width based on window size
    int height;         // Actual grid height based on window size
    int cell_width;     // Width of each cell in characters
    int cell_height;    // Height of each cell in characters
    int start_x;        // Starting X position of grid
    int start_y;        // Starting Y position of grid
} GridDimensions;

typedef struct {
    GridCell cells[MAX_GRID_HEIGHT][MAX_GRID_WIDTH];
    Position positions[2][5];  // Store positions for each team's units
    int unit_counts[2];       // Store unit count for each team
    WINDOW *main_win;         // Main game window
    WINDOW *status_win;       // Window for displaying unit status
    WINDOW *message_win;      // Window for displaying combat messages
    WINDOW *unit_list_win;    // Window for displaying all units' status
    WINDOW *hints_win;        // Window for displaying context-sensitive hints
    Position cursor_pos;      // Current cursor position
    Position selected_pos;    // Currently selected unit position
    bool has_selection;       // Whether a unit is currently selected
    GameState state;          // Current game state
    GridDimensions grid_dims; // Current grid dimensions
} Battlefield;

// Item selection menu structure
typedef struct {
    WINDOW *win;
    int current_page;
    int selected_item;
    int total_pages;
    const ITEM *available_items;
    int num_items;
} ItemMenu;

// Function declarations
int manhattan_distance(int x1, int y1, int x2, int y2);
void init_battlefield(Battlefield *bf);
bool is_valid_position(int x, int y);
void place_unit(Battlefield *bf, UNIT *unit, int team, int x, int y);
void remove_unit(Battlefield *bf, int x, int y);
void draw_battlefield(WINDOW *win, const Battlefield *bf);

// Window management functions
void create_status_windows(Battlefield *bf, int parent_height, int parent_width);
void destroy_status_windows(Battlefield *bf);
void resize_windows(Battlefield *bf, WINDOW *main_win);
void calculate_grid_dimensions(Battlefield *bf, int parent_height, int parent_width);
bool check_window_size(int height, int width);

// Display functions
void update_status_panel(Battlefield *bf, const UNIT *selected_unit, const Position *cursor_pos);
void display_combat_message(Battlefield *bf, const char *format, ...);
void display_controls_hint(Battlefield *bf, const char *hint);
void update_unit_list(Battlefield *bf);
void highlight_cursor(WINDOW *win, const Battlefield *bf);
void highlight_selected_unit(WINDOW *win, const Battlefield *bf);
void update_all_displays(WINDOW *win, Battlefield *bf, const UNIT *selected_unit);
const char *get_item_summary(const ITEM *item);

// New hint system functions
void update_hints(Battlefield *bf);
const char *get_state_hint(GameState state);
void set_game_state(Battlefield *bf, GameState new_state);

// Combat functions
bool is_valid_attack_target(const Battlefield *bf, const UNIT *attacker, int x1, int y1, int x2, int y2);
int calculate_damage(const UNIT *attacker, const UNIT *defender);
void update_combat_stats(Battlefield *bf, UNIT *attacker, UNIT *target, int damage);
bool perform_combat(Battlefield *bf, Position *att_pos, Position *target_pos, int *remaining_units);

// Item selection functions
void create_item_menu(ItemMenu *menu, int parent_height, int parent_width);
void destroy_item_menu(ItemMenu *menu);
const ITEM *show_item_selection(ItemMenu *menu, const char *title, int slots_available);
void draw_item_menu(ItemMenu *menu, const char *title);
void update_item_menu(ItemMenu *menu);

// Unit positioning and action functions
bool move_unit(Battlefield *bf, int from_x, int from_y, int to_x, int to_y);
void create_action_menu(ActionMenu *menu, int parent_height, int parent_width);
void destroy_action_menu(ActionMenu *menu);
ActionType show_action_menu(ActionMenu *menu, const UNIT *unit);
void update_action_menu(ActionMenu *menu, const UNIT *unit);
bool is_valid_move(const Battlefield *bf, int from_x, int from_y, int to_x, int to_y);
bool has_special_ability(const UNIT *unit);
void use_special_ability(Battlefield *bf, const UNIT *unit, int x, int y);

// Visual feedback functions
void highlight_valid_moves(WINDOW *win, const Battlefield *bf, int x, int y);
void highlight_attack_range(WINDOW *win, const Battlefield *bf, const UNIT *unit, int x, int y);

#endif // BATTLEFIELD_H 