# Battle Arena Game

A text-based tactical battle game featuring unit combat, items, and AI opponents, built with ncurses for a beautiful terminal interface.

## Features

- 🎮 Multiple Game Modes:
  - AI Game: Battle against computer-controlled opponents
  - Simple Game: Two-player tactical combat
  - Save/Load: Continue your previous battles

- 🏹 Tactical Combat System:
  - 10x10 grid battlefield
  - Turn-based combat
  - Unit positioning and movement
  - Items and equipment system
  - Health management

- 🎨 Beautiful Text-Based Graphics:
  - ASCII art title screen
  - Animated battle sequences
  - Clean, intuitive interface
  - Status windows for battle information

## Requirements

- GCC compiler
- ncurses library
- make build system
- Linux/Unix terminal or Windows with WSL/Cygwin

### Installing Dependencies

#### On Linux/Unix:
```bash
# Ubuntu/Debian
sudo apt-get install gcc make libncurses5-dev

# Fedora
sudo dnf install gcc make ncurses-devel

# Arch Linux
sudo pacman -S gcc make ncurses
```

#### On Windows:
1. Install WSL (Windows Subsystem for Linux) or Cygwin
2. Follow the Linux instructions above within your chosen environment

## Building the Game

1. Clone or download the repository
2. Open a terminal in the project directory
3. Run make to build the game:
```bash
make
```

## Running the Game

After building, start the game by running:
```bash
./battle_arena
```

## How to Play

### Controls
- Arrow Keys: Navigate menus and move units
- Enter/Return: Select options and confirm actions
- ESC: Back/Cancel (in menus)

### Game Modes

1. **AI Game**
   - Battle against computer-controlled opponents
   - AI uses tactical positioning and item management
   - Great for practice or solo play

2. **Simple Game**
   - Two-player tactical combat
   - Take turns moving units and engaging in combat
   - Manage items and positioning for victory

3. **Load Game**
   - Continue a previously saved battle
   - Maintains unit positions, health, and items

### Gameplay Tips
- Position units strategically on the 10x10 grid
- Use items wisely to gain advantages
- Consider unit health when planning moves
- Block enemy movement with careful positioning
- Save your game before risky maneuvers

## Code Structure

The game is organized into several key components:

- `main.c`: Core game logic and UI management
- `battlefield.c/h`: Battle system and grid management
- `data.c/h`: Item and unit data structures

### Key Features Implementation
- ncurses for terminal graphics and user interface
- Turn-based combat system with tactical positioning
- Save/load system for game state persistence
- AI pathfinding and decision making
- ASCII art integration for visual appeal

## Technical Overview

### Source Files and Components

1. **main.c** - Program Entry and Game Flow
   - Initializes ncurses and system setup
   - Implements the main menu system and game loop
   - Handles user input and game state transitions
   - Manages save/load functionality
   - Coordinates between battlefield and data components

2. **battlefield.h/c** - Battle System Core
   - Defines the 10x10 grid battle system
   - Manages unit positioning and movement
   - Implements combat mechanics and calculations
   - Handles visual rendering and UI windows
   - Controls game state and turn management
   
3. **data.h/c** - Game Data and Entities
   - Defines core game structures (UNIT, ITEM)
   - Implements the item database system
   - Manages unit stats and equipment
   - Provides data validation and constraints

4. **data.o** - Object File
   - Compiled binary object file generated from data.c
   - Contains machine code for data structures and functions
   - Used by the linker to create the final executable
   - Not meant to be directly read or modified

5. **battlefield.o** - Object File
   - Compiled binary object file generated from battlefield.c
   - Contains machine code for battle system functions and grid logic
   - Used by the linker to create the final executable
   - Not meant to be directly read or modified

6. **main.o** - Object File
   - Compiled binary object file generated from main.c
   - Contains machine code for core game logic and UI management
   - Used by the linker to create the final executable
   - Not meant to be directly read or modified
   

### Core Systems Architecture

#### 1. Game State Management
- Uses `GameState` enum to track current game phase:
  - STATE_INIT: Game initialization
  - STATE_POSITIONING: Initial unit placement
  - STATE_SELECT_UNIT: Unit selection phase
  - STATE_MOVE_UNIT: Movement phase
  - STATE_SELECT_ACTION: Action selection
  - STATE_SELECT_TARGET: Target selection
  - STATE_COMBAT_RESULT: Combat resolution
  - STATE_GAME_OVER: End game state

#### 2. UI System (ncurses-based)
- Multiple specialized windows:
  - Main battle grid (10x10)
  - Status panel (unit information)
  - Message window (combat updates)
  - Unit list (army overview)
  - Context-sensitive hints
- Minimum window requirements: 80x24 characters
- Dynamic window resizing support
- ASCII art integration for menus and title screens

#### 3. Battle System
- Grid-based movement using Manhattan distance
- Combat resolution system:
  - Attack range verification
  - Damage calculation based on items
  - Area effect handling
  - Unit state updates
- Turn-based mechanics with action points
- Support for special abilities and items

#### 4. Data Management
- Unit System:
  - Name (up to 100 characters)
  - Health points (HP)
  - Two item slots
  - Team affiliation
- Item System:
  - 16 predefined items
  - Properties: attack, defense, range, radius
  - Slot requirements (1 or 2)
  - Effect area calculations

#### 5. Save/Load System
- Binary file format (savefile.dat)
- Stores:
  - Army compositions
  - Unit positions and health
  - Equipment configurations
  - Current turn state
- Validation and error checking

#### 6. AI System
- Tactical decision making
- Target prioritization
- Pathfinding for unit movement
- Equipment usage optimization
- Position evaluation and selection

### Data Flow

1. **Input Processing**
   ```
   User Input → Input Handler → Game State Update → UI Refresh
   ```

2. **Combat Flow**
   ```
   Unit Selection → Target Selection → Range Check →
   Damage Calculation → State Update → UI Refresh
   ```

3. **Turn Sequence**
   ```
   Start Turn → Unit Actions → State Validation →
   Victory Check → Next Turn/End Game
   ```

### Memory Management

- Dynamic allocation for game entities
- Proper cleanup on game exit
- Memory leak prevention
- Validation through AddressSanitizer and Valgrind

### Error Handling

- Input validation at all stages
- Graceful failure handling
- User-friendly error messages
- State consistency checks

## Development

### Building for Development
```bash
# Build with debug symbols
make CFLAGS="-Wall -Wextra -g"

# Run tests
make test
```

### Memory Testing
The project includes memory testing targets:
```bash
# Run with AddressSanitizer
make test

# Manual Valgrind testing
valgrind --leak-check=full ./battle_arena
```

## Troubleshooting

### Common Issues

1. **Screen Size Error**
   - Ensure your terminal window is large enough
   - Minimum recommended size: 80x24 characters

2. **Graphics Issues**
   - Verify ncurses is properly installed
   - Check terminal supports ASCII characters

3. **Build Failures**
   - Ensure all dependencies are installed
   - Check compiler errors for missing headers

## License

This project is open source and available under the MIT License.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests. 