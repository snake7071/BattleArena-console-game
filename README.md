# Battle Arena 3.0

A console-based battle arena game with text-based interface using ncurses.

## Features

- 2D grid-based battlefield with Manhattan distance calculations
- Two game modes: Player vs Player and AI Battle
- Save/Load game functionality
- Colorful ncurses-based UI
- Support for various weapons and items with different ranges and effects
- Memory-safe implementation (tested with Valgrind and AddressSanitizer)

## Requirements

- GCC compiler
- ncurses library
- Valgrind (for memory testing)

## Building

```bash
make
```

## Testing

```bash
make test
```

This will run the game through both AddressSanitizer and Valgrind to check for memory issues.

## Game Controls

- Arrow keys: Move cursor/select units
- Enter: Confirm selection
- S: Save game
- Q: Quit game (in menus)

## Game Rules

1. Each player can have 1-5 units
2. Units can equip up to 2 items
3. Items have:
   - Attack power
   - Defense value
   - Range (using Manhattan distance)
   - Area effect radius
   - Slot count (1 or 2)
4. Combat uses Manhattan distance for range calculations
5. Damage calculation: attacker's attack - target's defense (minimum 1)

## File Structure

- `main.c`: Main game logic and UI
- `data.c`: Game data definitions
- `data.h`: Data structures and constants
- `Makefile`: Build configuration
- `savefile.dat`: Save game data

## Save File Format

The save file stores:
- Number of units in each army
- Current turn
- Unit data:
  - Name
  - HP
  - Equipped items 