# C Battleship Implementation

## Synopsis

This repository contains a command-line implementation of the classic Battleship board game, developed in the C programming language. The project serves as a demonstration of fundamental Unix/Linux systems programming concepts, notably inter-process communication (IPC) using POSIX shared memory (`mmap`) and process management via the `fork()` system call.

The application features a single-player mode where the user competes against a computer-controlled opponent (AI). Game state, including board layouts and turn progression, is synchronized between the parent (player) and child (AI) processes through a shared memory segment.

## Core Features

*   **Single-Player vs. AI Gameplay**: Implements the complete game logic for a one-on-one match against an AI opponent on an 8x8 grid.
*   **Inter-Process Communication**: Utilizes `fork()` to create a distinct process for the AI. Gameplay is synchronized between the player and AI processes using a shared memory region established with `shm_open()` and `mmap()`.
*   **Session Persistence**: Provides functionality to save the entire game state (all boards, current turn) to a file (`savegame.txt`), allowing a session to be closed and resumed later.
*   **Dynamic User Interface**: The command-line menu adapts to the current game context, presenting relevant options such as "Continue Game" only when applicable.
*   **Automated and Manual Board Configuration**: Features randomized, automatic placement of ship fleets for both the player and the AI, with an option for the player to re-randomize their layout before a match begins.
*   **State Export**: Allows the player to pause the game and export the current state of all game boards to a human-readable text file (`boards.txt`) for review.

## Technical Architecture

The program's architecture is based on a two-process model:

1.  **Parent Process**: This process manages the main user interface (the menu) and represents the human player. It is responsible for handling user input, initiating the game, and displaying board states.

2.  **Child Process**: Created via `fork()`, this process executes the logic for the AI opponent. It operates concurrently and independently, making strategic decisions based on the game state visible in shared memory.

Communication and synchronization are achieved as follows:
*   A shared memory object is created using `shm_open()` and its size is set with `ftruncate()`.
*   Both parent and child processes map this object into their respective virtual address spaces using `mmap()`, allowing them to read from and write to the same memory region.
*   A `turn` flag within this shared segment acts as a simple semaphore to control game flow, ensuring that only one process can execute a move at any given time.

## System Requirements

*   **Compiler**: A C compiler supporting the C99 standard or later (e.g., GCC, Clang).
*   **Operating System**: A POSIX-compliant operating system (e.g., Linux, macOS, or other Unix-like systems).
*   **Libraries**: The POSIX real-time library (`librt`) is required for shared memory functions.

## Build Instructions

1.  Navigate to the project's root directory in a terminal.
2.  Compile the source code using the following command:

    ```bash
    gcc main.c -o battleship -lrt
    ```
    *Note: The `-lrt` flag is essential for linking the POSIX real-time library, which provides the implementations for `shm_open()` and `shm_unlink()`.*

## Usage Instructions

### Execution

Execute the compiled binary from the terminal:

```bash
./battleship
```

### Main Menu Operations

The application will present a menu with the following options:

*   **1. Start New Game**: Initializes a new game session, discarding any previous state.
*   **2. Load Game from File**: Resumes a previously saved session from `savegame.txt`.
*   **3. Continue Game**: Continues an active game that was previously paused by returning to the menu. (This option is only visible if a game is in progress).
*   **4. Display Grids**: Renders the current state of the player's primary grid and targeting grid.
*   **5. Re-locate Ships**: Re-initializes the player's ship positions with a new random layout.
*   **6. Exit**: Terminates the application.

### In-Game Commands

During the player's turn, the following inputs are accepted:

*   **Attack Coordinates (`row col`)**: Enter a row and column number (e.g., `3 4`) to fire upon a target cell.
*   **`p` - Pause and Export**: Pauses the game momentarily to write the current state of all boards to `boards.txt` and then resumes the turn.
*   **`m` - Return to Menu**: Persists the complete current game state to `savegame.txt` and exits the active game session, returning control to the main menu.

## File Output

The program generates the following files in its execution directory:

*   **`battleship`**: The compiled executable binary.
*   **`savegame.txt`**: A data file storing the serialized game state. It is created or overwritten when saving a game or returning to the menu.
*   **`boards.txt`**: A human-readable text file containing a formatted representation of all game boards. It is generated upon exiting the game or using the in-game 'p' command.
