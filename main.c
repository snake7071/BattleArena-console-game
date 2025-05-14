// Hey! This is our game's main file where everything comes together.
// We need these libraries to make our game work:
#include <ncurses.h>      // This gives us cool text-based graphics
#include <string.h>       // For working with text
#include <stdlib.h>       // For basic stuff like memory management
#include <locale.h>       // Helps with special characters
#include <stdio.h>        // For reading/writing files
#include <stdbool.h>      // So we can use true/false
#include <unistd.h>       // For system stuff like sleep
#include "data.h"         // Our game items and units
#include "battlefield.h"  // The game board and battle logic

// These files contain our cool ASCII art for the menu
#define TITLE_FILE     "title.txt"
#define LEFT_ART_FILE  "astolfo_left.txt"
#define RIGHT_ART_FILE "astolfo_right.txt"

// Where we save our game progress
#define SAVE_FILE      "savefile.dat"

// How big our game board is
#define GRID_WIDTH  10   // 10 squares wide
#define GRID_HEIGHT 10   // 10 squares tall

// Menu stuff
static const char *labels[]      = { "Start", "Exit" };                         // Main menu options
static const char *mode_labels[] = { "AI Game", "Simple Game", "Load Game", "Back" };  // Game modes
enum { BTN_START=0, BTN_EXIT, BTN_COUNT };           // Makes it easier to work with menu buttons
enum { MODE_AI=0, MODE_SIMPLE, MODE_LOAD, MODE_BACK, MODE_COUNT };  // Different game modes
#define BTN_W 20    // How wide our buttons are
#define BTN_H 5     // How tall our buttons are

// Error codes for when something goes wrong
#define ERR_UNIT_COUNT  (-1)    // Too many or too few units
#define ERR_ITEM_COUNT  (-2)    // Problem with items
#define ERR_WRONG_ITEM  (-3)    // Invalid item selected
#define ERR_SLOTS       (-4)    // Not enough inventory slots

// This helps us load and display ASCII art
typedef struct {
    int width, height;   // Size of the art
    char **lines;        // The actual art content
} AsciiArt;

// These are function declarations - we'll define them later
static Position find_closest_enemy(const Battlefield *bf, int team, int x, int y);
static bool move_towards_target(Battlefield *bf, Position *unit_pos, Position target);

static int update_field(UNIT *field[], int count);
static void draw_field1D(WINDOW *win,
                         UNIT *pole1[], int n1,
                         UNIT *pole2[], int n2);
static int item_index(const ITEM *it);
static bool save_game(const char *filename,
                      UNIT a1[], int n1,
                      UNIT a2[], int n2,
                      int turn);
static bool load_game(const char *filename,
                      UNIT a1[], int *n1,
                      UNIT a2[], int *n2,
                      int *turn);

// This function keeps track of which units are still alive
static int update_field(UNIT *field[], int count) {
    int alive = 0;
    for(int i = 0; i < count; i++) {
        if(field[i]->hp > 0)  // If the unit still has health
            field[alive++] = field[i];  // Keep it in our list
    }
    return alive;  // Return how many units are still fighting
}

// Finds which item number an item is in our database
static int item_index(const ITEM *it) {
    if(!it) return -1;  // If there's no item, return -1
    for(int i = 0; i < NUMBER_OF_ITEMS; i++)
        if(&items[i] == it) return i;  // Found it!
    return -1;  // Item not found in our database
}

// This is what we save for each unit when saving the game
typedef struct {
    char name[MAX_NAME+1];  // Unit's name
    int  hp;                // Current health
    int  idx1;             // First item's index in our database
    int  idx2;             // Second item's index in our database
} SaveUnit;

// Saves the current game state to a file
static bool save_game(const char *filename,
                      UNIT a1[], int n1,  // Army 1's units and count
                      UNIT a2[], int n2,  // Army 2's units and count
                      int turn)           // Whose turn it is
{
    // Try to open the save file
    FILE *f = fopen(filename, "wb");
    if(!f) return false;  // Couldn't open file :(
    
    // Save the basic game info
    fwrite(&n1, sizeof(int), 1, f);    // How many units in army 1
    fwrite(&n2, sizeof(int), 1, f);    // How many units in army 2
    fwrite(&turn, sizeof(int), 1, f);  // Whose turn it is

    // Save each unit from army 1
    SaveUnit su;
    for(int i = 0; i < n1; i++) {
        memset(&su, 0, sizeof su);  // Clear the save unit struct
        strncpy(su.name, a1[i].name, MAX_NAME);  // Copy the unit's name
        su.hp = a1[i].hp;                        // Save current health
        su.idx1 = item_index(a1[i].item1);      // Save what items they have
        su.idx2 = item_index(a1[i].item2);
        fwrite(&su, sizeof su, 1, f);  // Write this unit to the file
    }
    
    // Save each unit from army 2 (same process)
    for(int i = 0; i < n2; i++) {
        memset(&su, 0, sizeof su);
        strncpy(su.name, a2[i].name, MAX_NAME);
        su.hp = a2[i].hp;
        su.idx1 = item_index(a2[i].item1);
        su.idx2 = item_index(a2[i].item2);
        fwrite(&su, sizeof su, 1, f);
    }
    
    fclose(f);  // Close the file
    return true;  // Everything saved successfully!
}

// Loads a saved game from a file
static bool load_game(const char *filename,
                      UNIT a1[], int *n1,  // Where to put army 1's units
                      UNIT a2[], int *n2,  // Where to put army 2's units
                      int *turn)           // Where to store whose turn it is
{
    // Try to open the save file
    FILE *f = fopen(filename, "rb");
    if(!f) return false;  // File doesn't exist or can't be opened
    
    // Read the basic game info
    if(fread(n1, sizeof(int), 1, f) != 1) { fclose(f); return false; }
    if(fread(n2, sizeof(int), 1, f) != 1) { fclose(f); return false; }
    if(fread(turn, sizeof(int), 1, f) != 1) { fclose(f); return false; }
    
    // Make sure the army sizes make sense
    if(*n1 < 1 || *n1 > 5 || *n2 < 1 || *n2 > 5) { fclose(f); return false; }

    // Load each unit for army 1
    SaveUnit su;
    for(int i = 0; i < *n1; i++) {
        if(fread(&su, sizeof su, 1, f) != 1) { fclose(f); return false; }
        strncpy(a1[i].name, su.name, MAX_NAME);  // Restore unit's name
        a1[i].hp = su.hp;                        // Restore health
        // Restore their items (if they had any)
        a1[i].item1 = (su.idx1 >= 0 && su.idx1 < NUMBER_OF_ITEMS) ? &items[su.idx1] : NULL;
        a1[i].item2 = (su.idx2 >= 0 && su.idx2 < NUMBER_OF_ITEMS) ? &items[su.idx2] : NULL;
    }
    
    // Load each unit for army 2 (same process)
    for(int i = 0; i < *n2; i++) {
        if(fread(&su, sizeof su, 1, f) != 1) { fclose(f); return false; }
        strncpy(a2[i].name, su.name, MAX_NAME);
        a2[i].hp = su.hp;
        a2[i].item1 = (su.idx1 >= 0 && su.idx1 < NUMBER_OF_ITEMS) ? &items[su.idx1] : NULL;
        a2[i].item2 = (su.idx2 >= 0 && su.idx2 < NUMBER_OF_ITEMS) ? &items[su.idx2] : NULL;
    }
    
    fclose(f);  // Close the file
    return true;  // Everything loaded successfully!
}

// This is our main game function where two players battle it out!
int simple_game_curses(UNIT a1[], int *n1,       // Army 1's units and count
                      UNIT a2[], int *n2,       // Army 2's units and count
                      int init_turn,            // Who goes first
                      WINDOW *win)              // The window to draw in
{
    // Get the size of our game window
    int wy, wx;
    getmaxyx(win, wy, wx);
    
    // Set up our window for the game
    scrollok(win, FALSE);     // Don't let text scroll
    curs_set(0);             // Hide the cursor
    keypad(win, TRUE);       // Let us use arrow keys
    
    // Create our game board and get it ready
    Battlefield bf;
    init_battlefield(&bf);
    bf.main_win = win;  // Remember which window we're using
    create_status_windows(&bf, wy, wx);  // Make windows for game info
    
    // Put army 1's units on the left side of the board
    for (int i = 0; i < *n1; i++) {
        place_unit(&bf, &a1[i], 1, 0, i * 2);  // Space them out every other square
    }
    
    // Put army 2's units on the right side of the board
    for (int i = 0; i < *n2; i++) {
        place_unit(&bf, &a2[i], 2, GRID_WIDTH - 1, i * 2);
    }
    
    // Set up our game state
    int turn = init_turn;              // Whose turn is it
    UNIT *selected_unit = NULL;        // Which unit is selected
    bool has_moved = false;            // Has the current unit moved
    bool has_attacked = false;         // Has the current unit attacked
    
    // Show the initial game board
    set_game_state(&bf, STATE_SELECT_UNIT);
    update_all_displays(win, &bf, selected_unit);
    
    // Main game loop - keep going until one army is defeated
    while (*n1 > 0 && *n2 > 0) {
        // Show whose turn it is
        display_combat_message(&bf, "Player %d's turn", turn);
        
        // Get player input
        int ch = wgetch(win);
        
        // Handle save game request
        if (ch == 's' || ch == 'S') {
            if (save_game(SAVE_FILE, a1, *n1, a2, *n2, turn)) {
                display_combat_message(&bf, "Game saved to %s", SAVE_FILE);
            } else {
                display_combat_message(&bf, "Save failed!");
            }
            continue;
        }
        
        // Handle quit request
        if (ch == 'q' || ch == 'Q') {
            break;
        }
        
        // Keep track if we need to update the display
        bool update_needed = false;
        bool action_taken = false;
        
        // Handle different key presses
        switch (ch) {
            case KEY_UP:     // Move cursor up
                if (bf.cursor_pos.y > 0) {
                    bf.cursor_pos.y--;
                    update_needed = true;
                }
                break;
            case KEY_DOWN:   // Move cursor down
                if (bf.cursor_pos.y < GRID_HEIGHT-1) {
                    bf.cursor_pos.y++;
                    update_needed = true;
                }
                break;
            case KEY_LEFT:   // Move cursor left
                if (bf.cursor_pos.x > 0) {
                    bf.cursor_pos.x--;
                    update_needed = true;
                }
                break;
            case KEY_RIGHT:  // Move cursor right
                if (bf.cursor_pos.x < GRID_WIDTH-1) {
                    bf.cursor_pos.x++;
                    update_needed = true;
                }
                break;
            case 10: // Enter
                switch (bf.state) {
                    case STATE_SELECT_UNIT:
                        {
                            GridCell *cell = &bf.cells[bf.cursor_pos.y][bf.cursor_pos.x];
                            if (cell->unit && cell->team == turn) {
                                selected_unit = cell->unit;
                                bf.has_selection = true;
                                bf.selected_pos = bf.cursor_pos;
                                set_game_state(&bf, STATE_SELECT_ACTION);
                                update_needed = true;
                                
                                ActionMenu menu;
                                create_action_menu(&menu, wy, wx);
                                update_action_menu(&menu, selected_unit);
                                menu.can_move = !has_moved;
                                menu.can_attack = !has_attacked;
                                
                                ActionType action = show_action_menu(&menu, selected_unit);
                                destroy_action_menu(&menu);
                                
                                switch (action) {
                                    case ACTION_MOVE:
                                        if (!has_moved) {
                                            set_game_state(&bf, STATE_MOVE_UNIT);
                                            display_combat_message(&bf, "Choose where to move %s", selected_unit->name);
                                        }
                                        break;
                                    case ACTION_ATTACK:
                                        if (!has_attacked) {
                                            set_game_state(&bf, STATE_SELECT_TARGET);
                                            display_combat_message(&bf, "Choose target for %s", selected_unit->name);
                                        }
                                        break;
                                    case ACTION_END_TURN:
                                        turn = 3 - turn;
                                        has_moved = false;
                                        has_attacked = false;
                                        bf.has_selection = false;
                                        selected_unit = NULL;
                                        set_game_state(&bf, STATE_SELECT_UNIT);
                                        break;
                                    default:
                                        bf.has_selection = false;
                                        selected_unit = NULL;
                                        set_game_state(&bf, STATE_SELECT_UNIT);
                                        break;
                                }
                            }
                        }
                        break;
                        
                    case STATE_MOVE_UNIT:
                        if (is_valid_move(&bf, bf.selected_pos.x, bf.selected_pos.y,
                                        bf.cursor_pos.x, bf.cursor_pos.y)) {
                            move_unit(&bf, bf.selected_pos.x, bf.selected_pos.y,
                                    bf.cursor_pos.x, bf.cursor_pos.y);
                            has_moved = true;
                            action_taken = true;
                            
                            turn = 3 - turn;
                            has_moved = false;
                            has_attacked = false;
                            bf.has_selection = false;
                            selected_unit = NULL;
                            set_game_state(&bf, STATE_SELECT_UNIT);
                            update_needed = true;
                        } else {
                            display_combat_message(&bf, "Invalid move position!");
                        }
                        break;
                        
                    case STATE_SELECT_TARGET:
                        if (is_valid_attack_target(&bf, selected_unit,
                                                 bf.selected_pos.x, bf.selected_pos.y,
                                                 bf.cursor_pos.x, bf.cursor_pos.y)) {
                            Position target_pos = bf.cursor_pos;
                            int *remaining = (bf.cells[target_pos.y][target_pos.x].team == 1) ? n1 : n2;
                            perform_combat(&bf, &bf.selected_pos, &target_pos, remaining);
                            has_attacked = true;
                            action_taken = true;
                            
                            turn = 3 - turn;
                            has_moved = false;
                            has_attacked = false;
                            bf.has_selection = false;
                            selected_unit = NULL;
                            set_game_state(&bf, STATE_SELECT_UNIT);
                            update_needed = true;
                        } else {
                            display_combat_message(&bf, "Invalid attack target!");
                        }
                        break;
                        
                    default:
                        break;
                }
                break;
                
            case 27: // Escape
                if (bf.state != STATE_SELECT_UNIT) {
                    bf.has_selection = false;
                    selected_unit = NULL;
                    set_game_state(&bf, STATE_SELECT_UNIT);
                    update_needed = true;
                }
                break;
        }
        
        if (update_needed) {
            update_all_displays(win, &bf, selected_unit);
        }
        
        if (action_taken) {
            display_combat_message(&bf, "Turn ended. Player %d's turn", turn);
            usleep(500000);
        }
    }
    
    // Show winner
    if (*n1 > 0) {
        wattron(win, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(win, wy/2, (wx-12)/2, "PLAYER 1 WINS!");
        wattroff(win, COLOR_PAIR(1) | A_BOLD);
        display_combat_message(&bf, "Player 1 is victorious!");
    } else {
        wattron(win, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(win, wy/2, (wx-12)/2, "PLAYER 2 WINS!");
        wattroff(win, COLOR_PAIR(2) | A_BOLD);
        display_combat_message(&bf, "Player 2 is victorious!");
    }
    
    display_controls_hint(&bf, "Press any key to continue...");
    wgetch(win);
    
    // Cleanup
    destroy_status_windows(&bf);
    return 0;
}

static const ITEM* find_item(const char *name) {
    for (int i = 0; i < NUMBER_OF_ITEMS; i++) {
        if (strcmp(items[i].name, name) == 0)
            return &items[i];
    }
    return NULL;
}

static void fix_blanks(char *s) {
    char *p = s;
    while (*p) {
        if ((unsigned char)p[0]==0xE2 &&
            (unsigned char)p[1]==0xA0 &&
            (unsigned char)p[2]==0x80)
        {
            *p = ' ';
            memmove(p+1, p+3, strlen(p+3)+1);
        }
        p++;
    }
}

static AsciiArt load_art(const char *fname) {
    AsciiArt A = {0,0,NULL};
    FILE *f = fopen(fname,"r");
    if (!f) return A;
    char buf[512];
    while (fgets(buf,sizeof(buf),f)) {
        int len = strlen(buf);
        if (len && buf[len-1]=='\n') buf[--len]=0;
        fix_blanks(buf);
        A.lines = realloc(A.lines, sizeof(char*)*(A.height+1));
        A.lines[A.height] = strdup(buf);
        if (len > A.width) A.width = len;
        A.height++;
    }
    fclose(f);
    return A;
}

static void free_art(AsciiArt *A) {
    for (int i = 0; i < A->height; i++) free(A->lines[i]);
    free(A->lines);
}

static void draw_background(int maxh, int maxw,
                            AsciiArt *title,
                            AsciiArt *artL,
                            AsciiArt *artR)
{
    clear();
    attron(COLOR_PAIR(1));
    box(stdscr,0,0);
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(2)|A_BOLD);
    int tx = (maxw - title->width)/2;
    for(int i=0;i<title->height;i++)
        mvprintw(1+i, tx, "%s", title->lines[i]);
    attroff(COLOR_PAIR(2)|A_BOLD);

    int avail_h = maxh-2-title->height;
    int startL = title->height+2 + (avail_h-artL->height)/2;
    int startR = title->height+2 + (avail_h-artR->height)/2;
    for(int i=0;i<artL->height;i++)
        mvprintw(startL+i, 2, "%s", artL->lines[i]);
    for(int i=0;i<artR->height;i++)
        mvprintw(startR+i, maxw-artR->width-2, "%s", artR->lines[i]);
    refresh();
}

static void draw_base(int maxh, int maxw,
                      AsciiArt *title,
                      AsciiArt *artL,
                      AsciiArt *artR,
                      WINDOW *btn[BTN_COUNT])
{
    draw_background(maxh,maxw,title,artL,artR);
    int bx = (maxw-BTN_W)/2;
    int by = 1+title->height+1;
    for(int i=0;i<BTN_COUNT;i++){
        if(btn[i]){ delwin(btn[i]); btn[i]=NULL; }
        btn[i] = newwin(BTN_H, BTN_W, by + i*(BTN_H+1), bx);
    }
}

static void draw_button(WINDOW *w, const char *label, int sel){
    wbkgd(w, COLOR_PAIR(sel?3:4));
    if(sel) wattron(w,A_BOLD); else wattroff(w,A_BOLD);
    box(w,0,0);
    mvwprintw(w, BTN_H/2, (BTN_W-strlen(label))/2, "%s", label);
    wrefresh(w);
}

static int show_mode_menu(int maxh,int maxw,
                          AsciiArt *title,
                          AsciiArt *artL,
                          AsciiArt *artR,
                          WINDOW *btn[BTN_COUNT])
{
    draw_background(maxh,maxw,title,artL,artR);
    WINDOW *sub[MODE_COUNT];
    int sx = (maxw-BTN_W)/2;
    int th = MODE_COUNT*BTN_H+(MODE_COUNT-1);
    int sy = (maxh-th)/2;
    int sel=0,ch;
    for(int i=0;i<MODE_COUNT;i++){
        sub[i] = newwin(BTN_H,BTN_W,sy+i*(BTN_H+1),sx);
        draw_button(sub[i], mode_labels[i], i==sel);
    }
    while((ch=getch())!=ERR){
        if(ch==KEY_UP)    sel=(sel-1+MODE_COUNT)%MODE_COUNT;
        if(ch==KEY_DOWN)  sel=(sel+1)%MODE_COUNT;
        if(ch==10){
            for(int i=0;i<MODE_COUNT;i++) delwin(sub[i]);
            return sel;
        }
        for(int i=0;i<MODE_COUNT;i++)
            draw_button(sub[i], mode_labels[i], i==sel);
    }
    for(int i=0;i<MODE_COUNT;i++) delwin(sub[i]);
    return MODE_SIMPLE;
}

#define PADDING 2
#define SPACING 10
#define ICON_COLOR 1
static void draw_field1D(WINDOW *win,
                         UNIT *pole1[], int n1,
                         UNIT *pole2[], int n2)
{
    int wy, wx;
    getmaxyx(win, wy, wx);
    werase(win);

    int midY = wy/2 - 1;
    start_color();
    init_pair(ICON_COLOR, COLOR_YELLOW, COLOR_BLACK);

    for (int i = 0; i < n1; i++) {
        int x = PADDING + i * SPACING;
        if (x + 6 >= wx) break;
        mvwprintw(win, midY - 1, x, "%s(%3d)", pole1[i]->name, pole1[i]->hp);
        wattron(win, COLOR_PAIR(ICON_COLOR));
        mvwprintw(win, midY,     x, " O ");
        mvwprintw(win, midY + 1, x, "/|\\");
        mvwprintw(win, midY + 2, x, "/ \\");
        wattroff(win, COLOR_PAIR(ICON_COLOR));
    }

    for (int i = 0; i < n2; i++) {
        int x = wx - PADDING - (i+1) * SPACING;
        if (x < 0) break;
        mvwprintw(win, midY - 1, x, "%s(%3d)", pole2[i]->name, pole2[i]->hp);
        wattron(win, COLOR_PAIR(ICON_COLOR));
        mvwprintw(win, midY,     x, " O ");
        mvwprintw(win, midY + 1, x, "\\|/");
        mvwprintw(win, midY + 2, x, "/ \\");
        wattroff(win, COLOR_PAIR(ICON_COLOR));
    }

    wrefresh(win);
}

#include <unistd.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
int simulate_battle_curses(UNIT a1[], int n1, UNIT a2[], int n2, int max_rounds, WINDOW *win) {
    int wy, wx;
    getmaxyx(win, wy, wx);
    
    scrollok(win, FALSE);
    curs_set(0);
    
    // Initialize battlefield
    Battlefield bf;
    init_battlefield(&bf);
    bf.main_win = win;  // Store main window for combat updates
    create_status_windows(&bf, wy, wx);
    
    // Place armies
    for (int i = 0; i < n1; i++) {
        place_unit(&bf, &a1[i], 1, 0, i * 2);
    }
    for (int i = 0; i < n2; i++) {
        place_unit(&bf, &a2[i], 2, MAX_GRID_WIDTH - 1, i * 2);
    }
    
    update_all_displays(win, &bf, NULL);
    display_combat_message(&bf, "Battle starting...");
    display_controls_hint(&bf, "Q: Quit simulation | Space: Pause/Resume | Any key: Step");
    usleep(1000000);
    
    int round = 1;
    bool paused = false;
    bool step_mode = false;
    
    while (n1 > 0 && n2 > 0 && max_rounds != 0) {
        display_combat_message(&bf, "Round %d", round++);
        update_all_displays(win, &bf, NULL);
        usleep(500000);
        
        // Handle user input
        nodelay(win, TRUE);
        int ch = wgetch(win);
        if (ch == 'q' || ch == 'Q') break;
        if (ch == ' ') {
            paused = !paused;
            step_mode = false;
        }
        if (ch != ERR) step_mode = true;
        
        if (paused) {
            display_combat_message(&bf, "Battle paused. Space: Resume, Q: Quit, Any key: Step");
            nodelay(win, FALSE);
            ch = wgetch(win);
            if (ch == 'q' || ch == 'Q') break;
            if (ch == ' ') {
                paused = false;
                display_combat_message(&bf, "Battle resumed!");
            } else {
                step_mode = true;
            }
        }
        
        // Army 1 actions
        if (n1 > 0 && n2 > 0) {
            for (int i = 0; i < bf.unit_counts[0]; i++) {
                Position *att_pos = &bf.positions[0][i];
                UNIT *attacker = bf.cells[att_pos->y][att_pos->x].unit;
                
                // Try to attack first
                bool acted = false;
                Position *target_pos = NULL;
                int min_dist = MAX_GRID_WIDTH + MAX_GRID_HEIGHT;
                
                for (int j = 0; j < bf.unit_counts[1]; j++) {
                    Position *pos = &bf.positions[1][j];
                    if (is_valid_attack_target(&bf, attacker, att_pos->x, att_pos->y, pos->x, pos->y)) {
                        int dist = manhattan_distance(att_pos->x, att_pos->y, pos->x, pos->y);
                        if (dist < min_dist) {
                            min_dist = dist;
                            target_pos = pos;
                        }
                    }
                }
                
                if (target_pos) {
                    perform_combat(&bf, att_pos, target_pos, &n2);
                    acted = true;
                } else {
                    // If can't attack, try to move towards closest enemy
                    Position closest = find_closest_enemy(&bf, 1, att_pos->x, att_pos->y);
                    if (closest.x != -1) {
                        acted = move_towards_target(&bf, att_pos, closest);
                    }
                }
                
                if (acted && step_mode) {
                    nodelay(win, FALSE);
                    display_combat_message(&bf, "Press any key to continue...");
                    wgetch(win);
                }
                
                update_all_displays(win, &bf, NULL);
                usleep(200000);
            }
        }
        
        // Army 2 actions (similar logic)
        if (n2 > 0 && n1 > 0) {
            for (int i = 0; i < bf.unit_counts[1]; i++) {
                Position *att_pos = &bf.positions[1][i];
                UNIT *attacker = bf.cells[att_pos->y][att_pos->x].unit;
                
                bool acted = false;
                Position *target_pos = NULL;
                int min_dist = MAX_GRID_WIDTH + MAX_GRID_HEIGHT;
                
                for (int j = 0; j < bf.unit_counts[0]; j++) {
                    Position *pos = &bf.positions[0][j];
                    if (is_valid_attack_target(&bf, attacker, att_pos->x, att_pos->y, pos->x, pos->y)) {
                        int dist = manhattan_distance(att_pos->x, att_pos->y, pos->x, pos->y);
                        if (dist < min_dist) {
                            min_dist = dist;
                            target_pos = pos;
                        }
                    }
                }
                
                if (target_pos) {
                    perform_combat(&bf, att_pos, target_pos, &n1);
                    acted = true;
                } else {
                    // If can't attack, try to move towards closest enemy
                    Position closest = find_closest_enemy(&bf, 2, att_pos->x, att_pos->y);
                    if (closest.x != -1) {
                        acted = move_towards_target(&bf, att_pos, closest);
                    }
                }
                
                if (acted && step_mode) {
                    nodelay(win, FALSE);
                    display_combat_message(&bf, "Press any key to continue...");
                    wgetch(win);
                }
                
                update_all_displays(win, &bf, NULL);
                usleep(200000);
            }
        }
        
        if (max_rounds > 0) max_rounds--;
    }
    
    // Display final result with visual emphasis
    if (n1 > 0 && n2 <= 0) {
        wattron(win, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(win, wy/2, (wx-12)/2, "ARMY 1 WINS!");
        wattroff(win, COLOR_PAIR(1) | A_BOLD);
        display_combat_message(&bf, "Army 1 is victorious!");
    }
    else if (n2 > 0 && n1 <= 0) {
        wattron(win, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(win, wy/2, (wx-12)/2, "ARMY 2 WINS!");
        wattroff(win, COLOR_PAIR(2) | A_BOLD);
        display_combat_message(&bf, "Army 2 is victorious!");
    }
    else {
        mvwprintw(win, wy/2, (wx-8)/2, "DRAW!");
        display_combat_message(&bf, "Battle ended in a draw!");
    }
    
    wrefresh(win);
    display_controls_hint(&bf, "Press any key to continue...");
    nodelay(win, FALSE);
    wgetch(win);
    
    destroy_status_windows(&bf);
    return 0;
}

static int setup_unit_curses(WINDOW *win, int *y, UNIT *unit) {
    ItemMenu menu;
    int wy, wx;
    getmaxyx(win, wy, wx);
    create_item_menu(&menu, wy, wx);
    
    // Get unit name
    echo(); curs_set(1);
    char name_buf[MAX_NAME+1];
    mvwprintw(win, (*y)++, 2, "Enter unit name: ");
    wrefresh(win);
    wgetnstr(win, name_buf, MAX_NAME);
    noecho(); curs_set(0);
    strncpy(unit->name, name_buf, MAX_NAME);
    
    // Select first item
    mvwprintw(win, (*y)++, 2, "Select primary item:");
    wrefresh(win);
    const ITEM *item1 = show_item_selection(&menu, "Select Primary Item", 2);
    if (!item1) {
        mvwprintw(win, (*y)++, 2, "ERR: Must select primary item");
        wrefresh(win);
        destroy_item_menu(&menu);
        return ERR_ITEM_COUNT;
    }
    unit->item1 = item1;
    
    // Select second item (optional)
    mvwprintw(win, (*y)++, 2, "Select secondary item (optional):");
    wrefresh(win);
    int slots_left = 2 - item1->slots;
    const ITEM *item2 = show_item_selection(&menu, "Select Secondary Item", slots_left);
    unit->item2 = item2;  // Can be NULL
    
    unit->hp = 100;
    destroy_item_menu(&menu);
    return 0;
}

static int read_army_curses(WINDOW *win, int *y, UNIT army[], int *out_count) {
    char buf[64];
    
    echo(); curs_set(1);
    mvwprintw(win, (*y)++, 2, "Enter unit count (1-5): ");
    wrefresh(win);
    wgetnstr(win, buf, sizeof(buf)-1);
    noecho(); curs_set(0);
    
    int cnt = strtol(buf, NULL, 10);
    if (cnt < 1 || cnt > 5) {
        mvwprintw(win, (*y)++, 2, "ERR_UNIT_COUNT");
        wrefresh(win);
        return ERR_UNIT_COUNT;
    }
    
    for (int i = 0; i < cnt; i++) {
        mvwprintw(win, (*y)++, 2, "Setting up unit %d:", i + 1);
        wrefresh(win);
        
        int result = setup_unit_curses(win, y, &army[i]);
        if (result < 0) {
            return result;
        }
        
        mvwprintw(win, (*y)++, 2, "Unit created: %s with %s", 
                  army[i].name, army[i].item1->name);
        if (army[i].item2) {
            wprintw(win, " and %s", army[i].item2->name);
        }
        wrefresh(win);
    }
    
    *out_count = cnt;
    return 0;
}

// This is where our game starts!
int main() {
    // Set up our terminal to handle special characters and colors
    setlocale(LC_ALL,"");
    initscr();              // Start up the terminal graphics
    noecho();              // Don't show typed characters
    cbreak();              // React to keys immediately
    curs_set(0);           // Hide the cursor
    keypad(stdscr, TRUE);  // Let us use special keys
    start_color();         // Turn on colors!
    
    // Set up our color pairs - each pair is for different game elements
    init_pair(1, COLOR_BLUE, COLOR_BLACK);     // Army 1 (Player) - Blue on black
    init_pair(2, COLOR_RED, COLOR_BLACK);      // Army 2 (Enemy) - Red on black
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);   // Cursor highlight - Yellow on black
    init_pair(4, COLOR_GREEN, COLOR_BLACK);    // Selected unit - Green on black
    init_pair(5, COLOR_CYAN, COLOR_BLACK);     // Menu highlight - Cyan on black
    init_pair(6, COLOR_WHITE, COLOR_BLACK);    // Normal text - White on black
    
    // Load our cool ASCII art for the menu
    AsciiArt title = load_art(TITLE_FILE);     // Game title
    AsciiArt left = load_art(LEFT_ART_FILE);   // Left side decoration
    AsciiArt right = load_art(RIGHT_ART_FILE); // Right side decoration

    // Create our menu buttons
    WINDOW *btn[BTN_COUNT] = {NULL};
    int maxh, maxw;
    getmaxyx(stdscr, maxh, maxw);  // Get screen size

    // Draw our main menu
    draw_base(maxh, maxw, &title, &left, &right, btn);
    for(int i = 0; i < BTN_COUNT; i++)
        draw_button(btn[i], labels[i], i == 0);  // Highlight first button

    // Menu loop - keep going until player chooses something
    int sel = 0;  // Currently selected button
    int ch;       // Key that was pressed
    while((ch = getch()) != ERR) {
        // Handle window resize
        if(ch == KEY_RESIZE) {
            getmaxyx(stdscr, maxh, maxw);
            draw_base(maxh, maxw, &title, &left, &right, btn);
            sel = 0;
        }
        
        // Move selection up/down
        if(ch == KEY_UP || ch == KEY_LEFT)
            sel = (sel - 1 + BTN_COUNT) % BTN_COUNT;
        if(ch == KEY_DOWN || ch == KEY_RIGHT)
            sel = (sel + 1) % BTN_COUNT;
        
        // Player made a choice!
        if(ch == 10) {  // Enter key
            if(sel == BTN_EXIT) break;  // They chose Exit
            
            // Show game mode menu
            int mode = show_mode_menu(maxh, maxw, &title, &left, &right, btn);
            clear();
            
            if(mode == MODE_BACK) {
                // They want to go back to main menu
            }
            else if (mode == MODE_AI) {
                // They chose AI game mode
                clear();
                int lh = maxh - 4, lw = maxw - 2;
                WINDOW *logwin = newwin(lh, lw, 2, 1);
                box(stdscr, 0, 0);
                mvprintw(1, (maxw-12)/2, " Battle Log ");
                wrefresh(stdscr);

                scrollok(logwin, TRUE);
                werase(logwin);

                // Set up both armies
                UNIT army1[5], army2[5];
                int c1, c2, err, y = 1;
                
                // Try to create the armies
                if ((err = read_army_curses(logwin, &y, army1, &c1)) < 0 ||
                    (err = read_army_curses(logwin, &y, army2, &c2)) < 0)
                {
                    // Something went wrong during setup
                    mvwprintw(logwin, y+1, 2, "Setup failed (%d). Press any key…", err);
                    wrefresh(logwin);
                    wgetch(logwin);
                } else {
                    // Start the AI battle!
                    simulate_battle_curses(army1, c1, army2, c2, -1, logwin);
                    
                    // Show end game options
                    const char *opts[] = { "Main Menu", "Exit" };
                    int selb = 0, btny = lh-3, btnx0 = (lw-20)/2, spacing = 12;
                    keypad(logwin, TRUE);
                    
                    // Let player choose what to do next
                    while(1) {
                        for(int i = 0; i < 2; i++) {
                            int x = btnx0 + i*spacing;
                            if(i == selb) wattron(logwin, A_REVERSE);
                            mvwprintw(logwin, btny, x, "%s", opts[i]);
                            if(i == selb) wattroff(logwin, A_REVERSE);
                        }
                        wrefresh(logwin);
                        
                        int ch = wgetch(logwin);
                        if(ch == KEY_LEFT && selb > 0) selb--;
                        if(ch == KEY_RIGHT && selb < 1) selb++;
                        if(ch == 10) break;  // Enter key
                    }
                    
                    // Handle their choice
                    if(selb == 1) {
                        delwin(logwin);
                        endwin();
                        exit(0);
                    }
                }
                
                // Clean up and go back to main menu
                delwin(logwin);
                draw_base(maxh, maxw, &title, &left, &right, btn);
                sel = 0;
                for(int i = 0; i < BTN_COUNT; i++)
                    draw_button(btn[i], labels[i], i == sel);
            }
            else if (mode == MODE_SIMPLE) {
                clear();
                int lh = maxh - 4, lw = maxw - 2;
                WINDOW *logwin = newwin(lh, lw, 2, 1);
                box(stdscr, 0, 0);
                mvprintw(1, (maxw-12)/2, " Simple Game ");
                wrefresh(stdscr);

                scrollok(logwin, TRUE);
                werase(logwin);
                int y=1, err;
                UNIT army1[5], army2[5]; int c1,c2;
                if ((err = read_army_curses(logwin, &y, army1, &c1)) < 0 ||
                    (err = read_army_curses(logwin, &y, army2, &c2)) < 0)
                {
                    mvwprintw(logwin, y+1, 2, "Setup failed (%d). Press any key…", err);
                    wrefresh(logwin);
                    wgetch(logwin);
                } else {
                    simple_game_curses(army1,&c1,army2,&c2,1,logwin);
                }
                delwin(logwin);
                draw_base(maxh,maxw,&title,&left,&right,btn); sel=0; for(int i=0;i<BTN_COUNT;i++) draw_button(btn[i], labels[i], i==sel);
            }
            else if(mode==MODE_LOAD){
                clear();
                int lh = maxh - 4, lw = maxw - 2;
                WINDOW *logwin = newwin(lh, lw, 2, 1);
                box(stdscr, 0, 0);
                mvprintw(1, (maxw-12)/2, " Load Game ");
                wrefresh(stdscr);

                UNIT army1[5], army2[5]; int c1,c2, turn_s;
                if(load_game(SAVE_FILE, army1,&c1, army2,&c2, &turn_s)){
                    mvwprintw(logwin,1,2,"Game loaded! Press any key to continue…"); wrefresh(logwin); wgetch(logwin);
                    werase(logwin);
                    simple_game_curses(army1,&c1,army2,&c2,turn_s,logwin);
                } else {
                    mvwprintw(logwin,1,2,"Load failed. Press any key…"); wrefresh(logwin); wgetch(logwin);
                }
                delwin(logwin);
                draw_base(maxh,maxw,&title,&left,&right,btn); sel=0; for(int i=0;i<BTN_COUNT;i++) draw_button(btn[i], labels[i], i==sel);
            }
        }
        for(int i=0;i<BTN_COUNT;i++)
            draw_button(btn[i], labels[i], i==sel);
    }

    for(int i=0;i<BTN_COUNT;i++) if(btn[i]) delwin(btn[i]);
    free_art(&title); free_art(&left); free_art(&right);
    endwin();
    return 0;
}

// Add new function to find closest enemy
static Position find_closest_enemy(const Battlefield *bf, int team, int x, int y) {
    Position closest = {-1, -1};
    int min_dist = MAX_GRID_WIDTH + MAX_GRID_HEIGHT;
    
    int enemy_team = (team == 1) ? 1 : 0;  // enemy_team index is team-1
    for (int i = 0; i < bf->unit_counts[enemy_team]; i++) {
        const Position *enemy_pos = &bf->positions[enemy_team][i];
        int dist = manhattan_distance(x, y, enemy_pos->x, enemy_pos->y);
        if (dist < min_dist) {
            min_dist = dist;
            closest.x = enemy_pos->x;
            closest.y = enemy_pos->y;
        }
    }
    return closest;
}

// This function helps a unit move closer to their target
static bool move_towards_target(Battlefield *bf, Position *unit_pos, Position target) {
    // If we're already at the target, we can't move closer!
    if (unit_pos->x == target.x && unit_pos->y == target.y)
        return false;
    
    // Figure out which direction we need to go
    int dx = (target.x > unit_pos->x) ? 1 : (target.x < unit_pos->x) ? -1 : 0;
    int dy = (target.y > unit_pos->y) ? 1 : (target.y < unit_pos->y) ? -1 : 0;
    
    // Try moving horizontally first
    if (dx != 0 && is_valid_move(bf, unit_pos->x, unit_pos->y,
                                unit_pos->x + dx, unit_pos->y)) {
        // Move the unit one square left or right
        move_unit(bf, unit_pos->x, unit_pos->y,
                 unit_pos->x + dx, unit_pos->y);
        unit_pos->x += dx;
        return true;
    }
    
    // If we can't move horizontally, try moving vertically
    if (dy != 0 && is_valid_move(bf, unit_pos->x, unit_pos->y,
                                unit_pos->x, unit_pos->y + dy)) {
        // Move the unit one square up or down
        move_unit(bf, unit_pos->x, unit_pos->y,
                 unit_pos->x, unit_pos->y + dy);
        unit_pos->y += dy;
        return true;
    }
    
    // If we get here, we couldn't move closer to the target
    return false;
}
