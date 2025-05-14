#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include "battlefield.h"

// Utility macros
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

int manhattan_distance(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

void init_battlefield(Battlefield *bf) {
    memset(bf, 0, sizeof(Battlefield));
    bf->unit_counts[0] = 0;
    bf->unit_counts[1] = 0;
    bf->cursor_pos.x = 0;
    bf->cursor_pos.y = 0;
    bf->has_selection = false;
    bf->state = STATE_INIT;
}

bool is_valid_position(int x, int y) {
    return x >= 0 && x < MAX_GRID_WIDTH && y >= 0 && y < MAX_GRID_HEIGHT;
}

void place_unit(Battlefield *bf, UNIT *unit, int team, int x, int y) {
    if (!is_valid_position(x, y)) return;
    
    int idx = bf->unit_counts[team-1];
    if (idx >= 5) return;
    
    bf->cells[y][x].unit = unit;
    bf->cells[y][x].team = team;
    bf->positions[team-1][idx].x = x;
    bf->positions[team-1][idx].y = y;
    bf->unit_counts[team-1]++;
}

void remove_unit(Battlefield *bf, int x, int y) {
    if (!is_valid_position(x, y)) return;
    
    int team = bf->cells[y][x].team;
    if (team == 0) return;
    
    // Find and remove position
    for (int i = 0; i < bf->unit_counts[team-1]; i++) {
        if (bf->positions[team-1][i].x == x && bf->positions[team-1][i].y == y) {
            // Shift remaining positions
            for (int j = i; j < bf->unit_counts[team-1] - 1; j++) {
                bf->positions[team-1][j] = bf->positions[team-1][j+1];
            }
            break;
        }
    }
    
    bf->cells[y][x].unit = NULL;
    bf->cells[y][x].team = 0;
    bf->unit_counts[team-1]--;
}

bool check_window_size(int height, int width) {
    return height >= MIN_WINDOW_HEIGHT && width >= MIN_WINDOW_WIDTH;
}

void calculate_grid_dimensions(Battlefield *bf, int parent_height, int parent_width) {
    // Calculate available space
    int available_width = parent_width - MIN_STATUS_WIDTH - 4;  // Leave space for borders
    int available_height = parent_height - MIN_MESSAGE_HEIGHT - MIN_HINTS_HEIGHT - 4;
    
    // Calculate cell size
    bf->grid_dims.cell_width = 6;  // Minimum width for unit names
    bf->grid_dims.cell_height = 3; // Minimum height for readability
    
    // Calculate grid size
    bf->grid_dims.width = MAX_GRID_WIDTH;
    bf->grid_dims.height = MAX_GRID_HEIGHT;
    
    // Calculate starting position
    bf->grid_dims.start_x = 2;
    bf->grid_dims.start_y = 2;
}

void create_status_windows(Battlefield *bf, int parent_height, int parent_width) {
    // Calculate dimensions based on window size
    calculate_grid_dimensions(bf, parent_height, parent_width);
    
    int status_width = MIN_STATUS_WIDTH;
    int status_height = MIN_STATUS_HEIGHT;
    int unit_list_width = MIN_UNIT_LIST_WIDTH;
    int unit_list_height = parent_height - status_height - MIN_HINTS_HEIGHT - 4;
    
    // Create status window on the right side
    bf->status_win = newwin(status_height, status_width, 
                           1, parent_width - status_width - 1);
    box(bf->status_win, 0, 0);
    
    // Create unit list window below status window
    bf->unit_list_win = newwin(unit_list_height, unit_list_width,
                              status_height + 2, parent_width - unit_list_width - 1);
    box(bf->unit_list_win, 0, 0);
    
    // Create hints window at the bottom
    bf->hints_win = newwin(MIN_HINTS_HEIGHT, parent_width - 2,
                          parent_height - MIN_HINTS_HEIGHT - MIN_MESSAGE_HEIGHT - 1, 1);
    box(bf->hints_win, 0, 0);
    
    // Create message window at the bottom
    bf->message_win = newwin(MIN_MESSAGE_HEIGHT, parent_width - 2, 
                            parent_height - MIN_MESSAGE_HEIGHT - 1, 1);
    box(bf->message_win, 0, 0);
    
    // Enable scrolling for message window
    scrollok(bf->message_win, TRUE);
}

void resize_windows(Battlefield *bf, WINDOW *main_win) {
    int parent_height, parent_width;
    getmaxyx(main_win, parent_height, parent_width);
    
    // Destroy existing windows
    destroy_status_windows(bf);
    
    // Recreate windows with new dimensions
    create_status_windows(bf, parent_height, parent_width);
}

void destroy_status_windows(Battlefield *bf) {
    if (bf->status_win) {
        delwin(bf->status_win);
        bf->status_win = NULL;
    }
    if (bf->message_win) {
        delwin(bf->message_win);
        bf->message_win = NULL;
    }
    if (bf->unit_list_win) {
        delwin(bf->unit_list_win);
        bf->unit_list_win = NULL;
    }
    if (bf->hints_win) {
        delwin(bf->hints_win);
        bf->hints_win = NULL;
    }
}

void update_status_panel(Battlefield *bf, const UNIT *selected_unit, const Position *cursor_pos) {
    WINDOW *win = bf->status_win;
    werase(win);
    box(win, 0, 0);
    
    mvwprintw(win, 0, 2, " Unit Info ");
    
    if (selected_unit) {
        wattron(win, A_BOLD);
        mvwprintw(win, 2, 2, "Name: %s", selected_unit->name);
        wattroff(win, A_BOLD);
        
        // Show HP with color based on remaining health percentage
        int hp_percent = (selected_unit->hp * 100) / 100;  // Assuming max HP is 100
        if (hp_percent > 66) wattron(win, COLOR_PAIR(1));  // Green
        else if (hp_percent > 33) wattron(win, COLOR_PAIR(3));  // Yellow
        else wattron(win, COLOR_PAIR(2));  // Red
        
        mvwprintw(win, 3, 2, "HP: %d/100", selected_unit->hp);
        wattroff(win, COLOR_PAIR(1) | COLOR_PAIR(2) | COLOR_PAIR(3));
        
        // Show items with their stats
        if (selected_unit->item1) {
            mvwprintw(win, 4, 2, "Item 1: %s", selected_unit->item1->name);
            mvwprintw(win, 5, 4, "ATT:%d DEF:%d RNG:%d",
                     selected_unit->item1->att,
                     selected_unit->item1->def,
                     selected_unit->item1->range);
        }
        if (selected_unit->item2) {
            mvwprintw(win, 6, 2, "Item 2: %s", selected_unit->item2->name);
            mvwprintw(win, 7, 4, "ATT:%d DEF:%d RNG:%d",
                     selected_unit->item2->att,
                     selected_unit->item2->def,
                     selected_unit->item2->range);
        }
    }
    
    if (cursor_pos) {
        mvwprintw(win, 9, 2, "Position: (%d,%d)", cursor_pos->x, cursor_pos->y);
        
        // Show unit under cursor if any
        GridCell *cell = &bf->cells[cursor_pos->y][cursor_pos->x];
        if (cell->unit) {
            UNIT *unit = cell->unit;
            mvwprintw(win, 10, 2, "Unit here: %s", unit->name);
            mvwprintw(win, 11, 2, "Team: %d  HP: %d", cell->team, unit->hp);
        }
    }
    
    wrefresh(win);
}

void display_combat_message(Battlefield *bf, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    WINDOW *win = bf->message_win;
    wmove(win, 1, 1);
    wclrtoeol(win);
    
    // Print the message
    wmove(win, 1, 2);
    vw_printw(win, format, args);
    
    wrefresh(win);
    va_end(args);
}

void display_controls_hint(Battlefield *bf, const char *hint) {
    WINDOW *win = bf->message_win;
    mvwprintw(win, 2, 2, "Controls: %s", hint);
    wrefresh(win);
}

bool is_valid_attack_target(const Battlefield *bf, const UNIT *attacker, int x1, int y1, int x2, int y2) {
    if (!is_valid_position(x2, y2)) return false;
    
    // Check if target position has an enemy unit
    const GridCell *target_cell = &bf->cells[y2][x2];
    if (!target_cell->unit || target_cell->team == bf->cells[y1][x1].team) {
        return false;
    }
    
    // Check range using Manhattan distance
    int dist = manhattan_distance(x1, y1, x2, y2);
    int range = attacker->item1->range;
    if (attacker->item2 && attacker->item2->range > range) {
        range = attacker->item2->range;
    }
    
    return dist <= range;
}

int calculate_damage(const UNIT *attacker, const UNIT *defender) {
    // Calculate total attack power
    int attack = attacker->item1->att;
    if (attacker->item2) {
        attack += attacker->item2->att;
    }
    
    // Calculate total defense
    int defense = defender->item1->def;
    if (defender->item2) {
        defense += defender->item2->def;
    }
    
    // Calculate damage (minimum 1)
    int damage = attack - defense;
    return damage > 0 ? damage : 1;
}

void draw_battlefield(WINDOW *win, const Battlefield *bf) {
    werase(win);
    
    const GridDimensions *dims = &bf->grid_dims;
    
    // Draw grid
    for (int y = 0; y < dims->height; y++) {
        for (int x = 0; x < dims->width; x++) {
            int px = dims->start_x + x * dims->cell_width;
            int py = dims->start_y + y * dims->cell_height;
            
            // Draw cell border
            mvwhline(win, py, px, ACS_HLINE, dims->cell_width);
            mvwhline(win, py + dims->cell_height - 1, px, ACS_HLINE, dims->cell_width);
            mvwvline(win, py, px, ACS_VLINE, dims->cell_height);
            mvwvline(win, py, px + dims->cell_width - 1, ACS_VLINE, dims->cell_height);
            
            // Draw corners
            mvwaddch(win, py, px, ACS_ULCORNER);
            mvwaddch(win, py, px + dims->cell_width - 1, ACS_URCORNER);
            mvwaddch(win, py + dims->cell_height - 1, px, ACS_LLCORNER);
            mvwaddch(win, py + dims->cell_height - 1, px + dims->cell_width - 1, ACS_LRCORNER);
            
            // Draw unit if present
            if (bf->cells[y][x].unit) {
                UNIT *u = bf->cells[y][x].unit;
                wattron(win, COLOR_PAIR(bf->cells[y][x].team));
                mvwprintw(win, py + 1, px + 1, "%-4.4s", u->name);
                
                // Show HP bar
                int hp_width = (u->hp * (dims->cell_width - 2)) / 100;
                wattron(win, A_REVERSE);
                for (int i = 0; i < hp_width; i++) {
                    mvwaddch(win, py + 2, px + 1 + i, ' ');
                }
                wattroff(win, A_REVERSE);
                wattroff(win, COLOR_PAIR(bf->cells[y][x].team));
            }
        }
    }
    
    // Show movement range if a unit is selected
    if (bf->has_selection) {
        UNIT *selected = bf->cells[bf->selected_pos.y][bf->selected_pos.x].unit;
        if (selected) {
            if (bf->state == STATE_MOVE_UNIT) {
                highlight_valid_moves(win, bf, bf->selected_pos.x, bf->selected_pos.y);
            } else if (bf->state == STATE_SELECT_TARGET) {
                highlight_attack_range(win, bf, selected, bf->selected_pos.x, bf->selected_pos.y);
            }
        }
    }
    
    wrefresh(win);
}

const char *get_item_summary(const ITEM *item) {
    static char buffer[50];
    if (!item) return "None";
    snprintf(buffer, sizeof(buffer), "%s (A:%d,D:%d,R:%d)", 
             item->name, item->att, item->def, item->range);
    return buffer;
}

void update_unit_list(Battlefield *bf) {
    WINDOW *win = bf->unit_list_win;
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " Unit List ");
    
    int y = 1;
    
    // Display Army 1
    wattron(win, COLOR_PAIR(1));
    mvwprintw(win, y++, 2, "Army 1:");
    wattroff(win, COLOR_PAIR(1));
    
    for (int i = 0; i < bf->unit_counts[0]; i++) {
        Position *pos = &bf->positions[0][i];
        UNIT *unit = bf->cells[pos->y][pos->x].unit;
        mvwprintw(win, y++, 2, "%s [%d,%d] HP:%d", 
                  unit->name, pos->x, pos->y, unit->hp);
        mvwprintw(win, y++, 3, "1:%s", get_item_summary(unit->item1));
        if (unit->item2) {
            mvwprintw(win, y++, 3, "2:%s", get_item_summary(unit->item2));
        }
    }
    
    y++;
    
    // Display Army 2
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, y++, 2, "Army 2:");
    wattroff(win, COLOR_PAIR(2));
    
    for (int i = 0; i < bf->unit_counts[1]; i++) {
        Position *pos = &bf->positions[1][i];
        UNIT *unit = bf->cells[pos->y][pos->x].unit;
        mvwprintw(win, y++, 2, "%s [%d,%d] HP:%d", 
                  unit->name, pos->x, pos->y, unit->hp);
        mvwprintw(win, y++, 3, "1:%s", get_item_summary(unit->item1));
        if (unit->item2) {
            mvwprintw(win, y++, 3, "2:%s", get_item_summary(unit->item2));
        }
    }
    
    wrefresh(win);
}

void highlight_cursor(WINDOW *win, const Battlefield *bf) {
    const GridDimensions *dims = &bf->grid_dims;
    int px = dims->start_x + bf->cursor_pos.x * dims->cell_width;
    int py = dims->start_y + bf->cursor_pos.y * dims->cell_height;
    
    wattron(win, A_BOLD | COLOR_PAIR(3));
    mvwhline(win, py, px, ACS_HLINE, dims->cell_width);
    mvwhline(win, py + dims->cell_height - 1, px, ACS_HLINE, dims->cell_width);
    mvwvline(win, py, px, ACS_VLINE, dims->cell_height);
    mvwvline(win, py, px + dims->cell_width - 1, ACS_VLINE, dims->cell_height);
    
    mvwaddch(win, py, px, ACS_ULCORNER);
    mvwaddch(win, py, px + dims->cell_width - 1, ACS_URCORNER);
    mvwaddch(win, py + dims->cell_height - 1, px, ACS_LLCORNER);
    mvwaddch(win, py + dims->cell_height - 1, px + dims->cell_width - 1, ACS_LRCORNER);
    wattroff(win, A_BOLD | COLOR_PAIR(3));
}

void highlight_selected_unit(WINDOW *win, const Battlefield *bf) {
    if (!bf->has_selection) return;
    
    const GridDimensions *dims = &bf->grid_dims;
    int px = dims->start_x + bf->selected_pos.x * dims->cell_width;
    int py = dims->start_y + bf->selected_pos.y * dims->cell_height;
    
    wattron(win, A_BOLD | COLOR_PAIR(4));
    mvwhline(win, py, px, ACS_HLINE, dims->cell_width);
    mvwhline(win, py + dims->cell_height - 1, px, ACS_HLINE, dims->cell_width);
    mvwvline(win, py, px, ACS_VLINE, dims->cell_height);
    mvwvline(win, py, px + dims->cell_width - 1, ACS_VLINE, dims->cell_height);
    
    mvwaddch(win, py, px, ACS_ULCORNER);
    mvwaddch(win, py, px + dims->cell_width - 1, ACS_URCORNER);
    mvwaddch(win, py + dims->cell_height - 1, px, ACS_LLCORNER);
    mvwaddch(win, py + dims->cell_height - 1, px + dims->cell_width - 1, ACS_LRCORNER);
    wattroff(win, A_BOLD | COLOR_PAIR(4));
}

const char *get_state_hint(GameState state) {
    switch (state) {
        case STATE_INIT:
            return "Welcome to Battle Arena! Arrow keys: Navigate | Enter: Select | S: Save | Q: Quit";
        case STATE_POSITIONING:
            return "Position your units | Arrow keys: Move | Enter: Place/Pick up | Space: Done | Esc: Cancel";
        case STATE_SELECT_UNIT:
            return "Select a unit to command | Arrow keys: Move cursor | Enter: Select unit | Esc: Cancel";
        case STATE_MOVE_UNIT:
            return "Choose where to move (2 squares max) | Arrow keys: Move | Enter: Confirm | Esc: Cancel";
        case STATE_SELECT_ACTION:
            return "Choose action | ↑↓: Select | Enter: Confirm | Esc: Cancel | Range shown in yellow";
        case STATE_SELECT_TARGET:
            return "Choose target within range | Arrow keys: Move | Enter: Attack | Esc: Cancel";
        case STATE_COMBAT_RESULT:
            return "Combat resolved! Press any key to continue... | Current HP shown in status panel";
        case STATE_GAME_OVER:
            return "Game Over! Press any key to return to main menu | S: Save replay";
        default:
            return "Use arrow keys to navigate | Enter: Select | Esc: Cancel | Q: Quit";
    }
}

void set_game_state(Battlefield *bf, GameState new_state) {
    bf->state = new_state;
    update_hints(bf);
}

void update_hints(Battlefield *bf) {
    WINDOW *win = bf->hints_win;
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " Hints ");
    
    // Get hint for current state
    const char *hint = get_state_hint(bf->state);
    
    // Center the hint text
    int win_width, win_height;
    getmaxyx(win, win_height, win_width);
    int hint_len = strlen(hint);
    int x = (win_width - hint_len) / 2;
    if (x < 2) x = 2;
    
    mvwprintw(win, 1, x, "%s", hint);
    wrefresh(win);
}

void update_all_displays(WINDOW *win, Battlefield *bf, const UNIT *selected_unit) {
    // Draw base battlefield
    draw_battlefield(win, bf);
    
    // Draw highlights
    highlight_selected_unit(win, bf);
    highlight_cursor(win, bf);
    
    // Update all panels
    update_status_panel(bf, selected_unit, &bf->cursor_pos);
    update_unit_list(bf);
    update_hints(bf);
    
    // Refresh main window
    wrefresh(win);
}

void create_item_menu(ItemMenu *menu, int parent_height, int parent_width) {
    menu->win = newwin(ITEM_MENU_HEIGHT, ITEM_MENU_WIDTH,
                      (parent_height - ITEM_MENU_HEIGHT) / 2,
                      (parent_width - ITEM_MENU_WIDTH) / 2);
    menu->current_page = 0;
    menu->selected_item = 0;
    menu->available_items = items;
    menu->num_items = NUMBER_OF_ITEMS;
    menu->total_pages = (menu->num_items + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    
    keypad(menu->win, TRUE);
    box(menu->win, 0, 0);
}

void destroy_item_menu(ItemMenu *menu) {
    if (menu->win) {
        delwin(menu->win);
        menu->win = NULL;
    }
}

void draw_item_menu(ItemMenu *menu, const char *title) {
    WINDOW *win = menu->win;
    werase(win);
    box(win, 0, 0);
    
    // Draw title
    mvwprintw(win, 0, (ITEM_MENU_WIDTH - strlen(title)) / 2, " %s ", title);
    
    // Draw items for current page
    int start_idx = menu->current_page * ITEMS_PER_PAGE;
    int items_on_page = MIN(ITEMS_PER_PAGE, menu->num_items - start_idx);
    
    for (int i = 0; i < items_on_page; i++) {
        const ITEM *item = &menu->available_items[start_idx + i];
        int y = 2 + i * 2;
        
        // Highlight selected item
        if (menu->selected_item == start_idx + i) {
            wattron(win, A_REVERSE);
        }
        
        mvwprintw(win, y, 2, "%-20.20s", item->name);
        mvwprintw(win, y + 1, 4, "ATT:%2d DEF:%2d RNG:%2d SLT:%d",
                 item->att, item->def, item->range, item->slots);
        
        if (menu->selected_item == start_idx + i) {
            wattroff(win, A_REVERSE);
        }
    }
    
    // Draw page info
    mvwprintw(win, ITEM_MENU_HEIGHT - 2, 2, "Page %d/%d", 
              menu->current_page + 1, menu->total_pages);
    
    // Draw controls
    mvwprintw(win, ITEM_MENU_HEIGHT - 1, 2, 
              "↑↓:Select  ←→:Page  Enter:Choose  Esc:Cancel");
    
    wrefresh(win);
}

const ITEM *show_item_selection(ItemMenu *menu, const char *title, int slots_available) {
    const ITEM *selected = NULL;
    
    while (1) {
        draw_item_menu(menu, title);
        int ch = wgetch(menu->win);
        
        switch (ch) {
            case KEY_UP:
                if (menu->selected_item > 0) {
                    menu->selected_item--;
                }
                break;
                
            case KEY_DOWN:
                if (menu->selected_item < menu->num_items - 1) {
                    menu->selected_item++;
                }
                break;
                
            case KEY_LEFT:
                if (menu->current_page > 0) {
                    menu->current_page--;
                    menu->selected_item = menu->current_page * ITEMS_PER_PAGE;
                }
                break;
                
            case KEY_RIGHT:
                if (menu->current_page < menu->total_pages - 1) {
                    menu->current_page++;
                    menu->selected_item = menu->current_page * ITEMS_PER_PAGE;
                }
                break;
                
            case 10: // Enter
                {
                    const ITEM *item = &menu->available_items[menu->selected_item];
                    if (item->slots <= slots_available) {
                        selected = item;
                        return selected;
                    } else {
                        // Flash the window to indicate invalid selection
                        flash();
                        beep();
                    }
                }
                break;
                
            case 27: // Escape
                return NULL;
        }
        
        // Ensure selected item is on current page
        if (menu->selected_item < menu->current_page * ITEMS_PER_PAGE) {
            menu->selected_item = menu->current_page * ITEMS_PER_PAGE;
        }
        if (menu->selected_item >= (menu->current_page + 1) * ITEMS_PER_PAGE) {
            menu->selected_item = (menu->current_page + 1) * ITEMS_PER_PAGE - 1;
        }
    }
    
    return selected;
}

bool is_valid_move(const Battlefield *bf, int from_x, int from_y, int to_x, int to_y) {
    if (!is_valid_position(to_x, to_y)) return false;
    if (bf->cells[to_y][to_x].unit != NULL) return false;
    
    // Calculate Manhattan distance
    int dist = manhattan_distance(from_x, from_y, to_x, to_y);
    return dist <= 2; // Units can move up to 2 squares per turn
}

bool move_unit(Battlefield *bf, int from_x, int from_y, int to_x, int to_y) {
    if (!is_valid_move(bf, from_x, from_y, to_x, to_y)) return false;
    
    UNIT *unit = bf->cells[from_y][from_x].unit;
    int team = bf->cells[from_y][from_x].team;
    
    // Update unit position in positions array
    for (int i = 0; i < bf->unit_counts[team-1]; i++) {
        if (bf->positions[team-1][i].x == from_x && 
            bf->positions[team-1][i].y == from_y) {
            bf->positions[team-1][i].x = to_x;
            bf->positions[team-1][i].y = to_y;
            break;
        }
    }
    
    // Move unit to new position
    bf->cells[to_y][to_x].unit = unit;
    bf->cells[to_y][to_x].team = team;
    bf->cells[from_y][from_x].unit = NULL;
    bf->cells[from_y][from_x].team = 0;
    
    return true;
}

void create_action_menu(ActionMenu *menu, int parent_height, int parent_width) {
    menu->win = newwin(8, 20, 
                      parent_height/2 - 4,
                      parent_width/2 - 10);
    menu->selected_action = 0;
    keypad(menu->win, TRUE);
}

void destroy_action_menu(ActionMenu *menu) {
    if (menu->win) {
        delwin(menu->win);
        menu->win = NULL;
    }
}

void update_action_menu(ActionMenu *menu, const UNIT *unit) {
    menu->can_move = true; // Can always move unless already moved
    menu->can_attack = unit->item1 != NULL; // Can attack if has weapon
    menu->has_special = has_special_ability(unit);
}

ActionType show_action_menu(ActionMenu *menu, const UNIT *unit) {
    update_action_menu(menu, unit);
    
    const char *actions[] = {
        "Move",
        "Attack",
        "Special",
        "End Turn"
    };
    
    while (1) {
        werase(menu->win);
        box(menu->win, 0, 0);
        mvwprintw(menu->win, 0, 6, " Actions ");
        
        for (int i = 0; i < 4; i++) {
            if (i == menu->selected_action) {
                wattron(menu->win, A_REVERSE);
            }
            
            // Gray out unavailable actions
            if ((i == 0 && !menu->can_move) ||
                (i == 1 && !menu->can_attack) ||
                (i == 2 && !menu->has_special)) {
                wattron(menu->win, A_DIM);
            }
            
            mvwprintw(menu->win, i + 2, 2, "%-12s", actions[i]);
            
            if (i == menu->selected_action) {
                wattroff(menu->win, A_REVERSE);
            }
            if ((i == 0 && !menu->can_move) ||
                (i == 1 && !menu->can_attack) ||
                (i == 2 && !menu->has_special)) {
                wattroff(menu->win, A_DIM);
            }
        }
        
        mvwprintw(menu->win, 6, 2, "↑↓:Select Enter:OK");
        wrefresh(menu->win);
        
        int ch = wgetch(menu->win);
        switch (ch) {
            case KEY_UP:
                do {
                    menu->selected_action = (menu->selected_action - 1 + 4) % 4;
                } while (
                    (menu->selected_action == 0 && !menu->can_move) ||
                    (menu->selected_action == 1 && !menu->can_attack) ||
                    (menu->selected_action == 2 && !menu->has_special)
                );
                break;
                
            case KEY_DOWN:
                do {
                    menu->selected_action = (menu->selected_action + 1) % 4;
                } while (
                    (menu->selected_action == 0 && !menu->can_move) ||
                    (menu->selected_action == 1 && !menu->can_attack) ||
                    (menu->selected_action == 2 && !menu->has_special)
                );
                break;
                
            case 10: // Enter
                switch (menu->selected_action) {
                    case 0: return menu->can_move ? ACTION_MOVE : -1;
                    case 1: return menu->can_attack ? ACTION_ATTACK : -1;
                    case 2: return menu->has_special ? ACTION_SPECIAL : -1;
                    case 3: return ACTION_END_TURN;
                }
                break;
                
            case 27: // Escape
                return -1;
        }
    }
}

bool has_special_ability(const UNIT *unit) {
    // Check if unit has any items with special abilities
    if (unit->item1 && unit->item1->radius > 0) return true;
    if (unit->item2 && unit->item2->radius > 0) return true;
    return false;
}

void use_special_ability(Battlefield *bf, const UNIT *unit, int x, int y) {
    // First, let's check if this unit has any items with area damage abilities
    // (items with a radius > 0 can hit multiple targets!)
    const ITEM *special_item = NULL;
    if (unit->item1 && unit->item1->radius > 0) special_item = unit->item1;
    else if (unit->item2 && unit->item2->radius > 0) special_item = unit->item2;
    
    // If we didn't find any items with area damage, we can't use a special ability
    if (!special_item) return;
    
    // Time to create some fireworks! Let's hit everything in range
    int radius = special_item->radius;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int target_x = x + dx;
            int target_y = y + dy;
            
            // Make sure we don't try to attack outside the battlefield
            if (!is_valid_position(target_x, target_y)) continue;
            
            // Check if there's an enemy unit at this spot
            UNIT *target = bf->cells[target_y][target_x].unit;
            if (target && bf->cells[target_y][target_x].team != bf->cells[y][x].team) {
                // Calculate and deal the damage
                int damage = calculate_damage(unit, target);
                target->hp -= damage;
                
                // Let everyone know what happened!
                display_combat_message(bf, "%s hits %s for %d area damage!",
                                     unit->name, target->name, damage);
                
                // Check if we defeated the target
                if (target->hp <= 0) {
                    display_combat_message(bf, "%s has been defeated!", target->name);
                    remove_unit(bf, target_x, target_y);
                }
            }
        }
    }
}

void update_combat_stats(Battlefield *bf, UNIT *attacker, UNIT *target, int damage) {
    // Update target's HP
    target->hp -= damage;
    
    // Update all displays to show new stats
    display_combat_message(bf, "%s hits %s for %d damage! %s HP: %d",
                         attacker->name, target->name, damage,
                         target->name, target->hp);
    
    // Flash the target's position
    wattron(bf->status_win, A_BOLD | COLOR_PAIR(2));  // Red for damage
    wrefresh(bf->status_win);
    usleep(100000);  // Brief flash
    wattroff(bf->status_win, A_BOLD | COLOR_PAIR(2));
    
    // Update all displays immediately
    update_unit_list(bf);
    wrefresh(bf->status_win);
    wrefresh(bf->unit_list_win);
}

bool perform_combat(Battlefield *bf, Position *att_pos, Position *target_pos, int *remaining_units) {
    UNIT *attacker = bf->cells[att_pos->y][att_pos->x].unit;
    UNIT *target = bf->cells[target_pos->y][target_pos->x].unit;
    
    if (!attacker || !target) return false;
    
    // Highlight attacker
    bf->cursor_pos = *att_pos;
    bf->has_selection = true;
    bf->selected_pos = *att_pos;
    update_all_displays(bf->main_win, bf, attacker);
    usleep(300000);
    
    // Show attack animation
    bf->cursor_pos = *target_pos;
    update_all_displays(bf->main_win, bf, attacker);
    usleep(300000);
    
    // Calculate and apply damage
    int damage = calculate_damage(attacker, target);
    update_combat_stats(bf, attacker, target, damage);
    
    // Check for defeat
    if (target->hp <= 0) {
        display_combat_message(bf, "%s has been defeated!", target->name);
        remove_unit(bf, target_pos->x, target_pos->y);
        (*remaining_units)--;
        update_all_displays(bf->main_win, bf, NULL);
        usleep(500000);
    }
    
    // Reset selection
    bf->has_selection = false;
    update_all_displays(bf->main_win, bf, NULL);
    usleep(300000);
    
    return true;
}

void highlight_valid_moves(WINDOW *win, const Battlefield *bf, int x, int y) {
    const GridDimensions *dims = &bf->grid_dims;
    
    // Check all positions within movement range (2 squares)
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int new_x = x + dx;
            int new_y = y + dy;
            
            if (is_valid_move(bf, x, y, new_x, new_y)) {
                int px = dims->start_x + new_x * dims->cell_width;
                int py = dims->start_y + new_y * dims->cell_height;
                
                // Highlight valid move squares in blue
                wattron(win, COLOR_PAIR(1) | A_DIM);
                mvwhline(win, py, px, ACS_HLINE, dims->cell_width);
                mvwhline(win, py + dims->cell_height - 1, px, ACS_HLINE, dims->cell_width);
                mvwvline(win, py, px, ACS_VLINE, dims->cell_height);
                mvwvline(win, py, px + dims->cell_width - 1, ACS_VLINE, dims->cell_height);
                wattroff(win, COLOR_PAIR(1) | A_DIM);
            }
        }
    }
}

void highlight_attack_range(WINDOW *win, const Battlefield *bf, const UNIT *unit, int x, int y) {
    if (!unit || !unit->item1) return;
    
    const GridDimensions *dims = &bf->grid_dims;
    int max_range = unit->item1->range;
    if (unit->item2 && unit->item2->range > max_range) {
        max_range = unit->item2->range;
    }
    
    // Check all positions within attack range
    for (int dy = -max_range; dy <= max_range; dy++) {
        for (int dx = -max_range; dx <= max_range; dx++) {
            int target_x = x + dx;
            int target_y = y + dy;
            
            if (is_valid_position(target_x, target_y)) {
                int dist = manhattan_distance(x, y, target_x, target_y);
                if (dist <= max_range) {
                    int px = dims->start_x + target_x * dims->cell_width;
                    int py = dims->start_y + target_y * dims->cell_height;
                    
                    // Highlight attack range squares in yellow
                    wattron(win, COLOR_PAIR(3) | A_DIM);
                    mvwhline(win, py, px, ACS_HLINE, dims->cell_width);
                    mvwhline(win, py + dims->cell_height - 1, px, ACS_HLINE, dims->cell_width);
                    mvwvline(win, py, px, ACS_VLINE, dims->cell_height);
                    mvwvline(win, py, px + dims->cell_width - 1, ACS_VLINE, dims->cell_height);
                    wattroff(win, COLOR_PAIR(3) | A_DIM);
                }
            }
        }
    }
} 