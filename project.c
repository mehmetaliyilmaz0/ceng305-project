// Include necessary headers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define WIDTH 8
#define GRID_SIZE 64
#define SHIP_COUNT 14  // Total ship cells
#define HIT 1
#define MISS 2
#define UNMARKED 0

#define CHILD 0
#define PARENT 1
#define VERTICAL 0

typedef struct {
    char turn;
    char data;
    char board[GRID_SIZE];
    char opponentBoard[GRID_SIZE];
    char ships[GRID_SIZE];
    char opponentShips[GRID_SIZE];
    int player;
} GameState;

// Function prototypes
int displayMenu(int gameInProgress);
int checkIfShipInPosition(char *ships, int position);
void drawBoard(char *board, char *ships, int showShips);
void cleanMemory(char *area);
void placeShip(char *ships, int length, int orientation, int pos);
int tilesAvailable(char *ships, int orientation, int length, int pos);
void autoPlaceShips(char *ships);
int allShipsSunk(char *board, char *ships);
int chooseAttackCell(char *opponentBoard);
void playGame(char* turn, char* data, char* board, char* opponentBoard, int player, char* ships, char* opponentShips);
void saveGameState(GameState *state);
int loadGameState(GameState *state);
void saveBoardsToFile(char *board, char *opponentBoard, char *ships, char *opponentShips);
void saveGameStateOnExit(char* turn, char* data, char* board, char* opponentBoard, char* ships, char* opponentShips, int player);

int main() {
    // Initialize game state variables
    char ships[GRID_SIZE] = {0};
    char opponentShips[GRID_SIZE] = {0};
    char board[GRID_SIZE] = {0};
    char opponentBoard[GRID_SIZE] = {0};
    int gameInProgress = 0; // Flag to indicate if a game can be continued
    int gameStarted = 0;

    GameState state;

    while (1) {
        int choice = displayMenu(gameInProgress);

        if (gameInProgress) {
            switch (choice) {
                case 1: // Start New Game
                    autoPlaceShips(ships);
                    memset(board, 0, GRID_SIZE);
                    memset(opponentBoard, 0, GRID_SIZE);
                    gameStarted = 1;
                    gameInProgress = 0;
                    printf("\nNew game started.\n");
                    break;

                case 2: // Load Game from File
                    if (loadGameState(&state)) {
                        memcpy(ships, state.ships, GRID_SIZE);
                        memcpy(opponentShips, state.opponentShips, GRID_SIZE);
                        memcpy(board, state.board, GRID_SIZE);
                        memcpy(opponentBoard, state.opponentBoard, GRID_SIZE);
                        gameStarted = 1;
                        gameInProgress = 1;
                        printf("\nGame loaded successfully.\n");
                    } else {
                        printf("\nNo saved game found.\n");
                    }
                    break;

                case 3: // Continue Game
                    gameStarted = 1;
                    printf("\nContinuing the game...\n");
                    break;

                case 4: // Display Grids
                    if (gameInProgress) {
                        printf("\nYour Board:\n");
                        drawBoard(board, ships, 1);
                        printf("\nOpponent's Board:\n");
                        drawBoard(opponentBoard, opponentShips, 0);
                    } else {
                        printf("\nNo game in progress. Start a new game or load a game first.\n");
                    }
                    break;

                case 5: // Re-locate Ships
                    if (gameInProgress) {
                        autoPlaceShips(ships);
                        memset(board, 0, GRID_SIZE);
                        printf("\nYour ships have been relocated.\n");
                    } else {
                        printf("\nNo game in progress. Start a new game or load a game first.\n");
                    }
                    break;

                case 6: // Exit
                    printf("\nExiting the game. Saving final board states...\n");
                    saveBoardsToFile(board, opponentBoard, ships, opponentShips);
                    exit(0);
                    break;

                default:
                    printf("\nInvalid choice. Please enter a valid option.\n");
                    break;
            }
        } else {
            switch (choice) {
                case 1: // Start New Game
                    autoPlaceShips(ships);
                    memset(board, 0, GRID_SIZE);
                    memset(opponentBoard, 0, GRID_SIZE);
                    gameStarted = 1;
                    gameInProgress = 0;
                    printf("\nNew game started.\n");
                    break;

                case 2: // Load Game from File
                    if (loadGameState(&state)) {
                        memcpy(ships, state.ships, GRID_SIZE);
                        memcpy(opponentShips, state.opponentShips, GRID_SIZE);
                        memcpy(board, state.board, GRID_SIZE);
                        memcpy(opponentBoard, state.opponentBoard, GRID_SIZE);
                        gameStarted = 1;
                        gameInProgress = 1;
                        printf("\nGame loaded successfully.\n");
                    } else {
                        printf("\nNo saved game found.\n");
                    }
                    break;

                case 3: // Display Grids
                    printf("\nNo game in progress. Start a new game or load a game first.\n");
                    break;

                case 4: // Re-locate Ships
                    printf("\nNo game in progress. Start a new game or load a game first.\n");
                    break;

                case 5: // Exit
                    printf("\nExiting the game. Goodbye!\n");
                    exit(0);
                    break;

                default:
                    printf("\nInvalid choice. Please enter a valid option.\n");
                    break;
            }
        }

        // If the game has started, proceed to play
        if (gameStarted) {
            // Initialize shared memory for inter-process communication
            int shm_fd = shm_open("buffer", O_CREAT | O_RDWR, 0666);
            ftruncate(shm_fd, GRID_SIZE * 2 + 2);
            void *ptr = mmap(0, GRID_SIZE * 2 + 2, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            cleanMemory(ptr);

            // Set up turn and data
            char *turn = (char *)ptr;
            char *data = (char *)ptr + 1;
            *turn = PARENT; // Parent starts first
            *data = 0;

            pid_t pid = fork();
            srand(time(NULL) ^ getpid());

            if (pid == 0) {  // Child process
                // If continuing a game, opponentShips are already loaded
                if (!gameInProgress) {
                    autoPlaceShips(opponentShips);
                }
                playGame(turn, data, opponentBoard, board, CHILD, opponentShips, ships);
                munmap(ptr, GRID_SIZE * 2 + 2);
                close(shm_fd);
                exit(0);
            } else {  // Parent process
                playGame(turn, data, board, opponentBoard, PARENT, ships, opponentShips);
                wait(NULL);
                munmap(ptr, GRID_SIZE * 2 + 2);
                close(shm_fd);
                shm_unlink("buffer");
            }

            // After the game ends or returns to menu
            if (*turn == -1) {
                // Game ended normally
                gameStarted = 0;
                gameInProgress = 0;
            } else if (*turn == -2) {
                // Returned to menu
                gameStarted = 0;
                gameInProgress = 1;
            }
        }
    }

    return 0;
}

// Function to display the menu and get user's choice
int displayMenu(int gameInProgress) {
    printf("\nBattleship Game Menu:\n");
    printf("1. Start New Game\n");
    printf("2. Load Game from File\n");
    if (gameInProgress) {
        printf("3. Continue Game\n");
        printf("4. Display Grids\n");
        printf("5. Re-locate Ships\n");
        printf("6. Exit\n");
        printf("Enter your choice (1-6): ");
    } else {
        printf("3. Display Grids\n");
        printf("4. Re-locate Ships\n");
        printf("5. Exit\n");
        printf("Enter your choice (1-5): ");
    }

    int choice;
    scanf("%d", &choice);
    // Clear the input buffer
    while (getchar() != '\n');

    return choice;
}

int checkIfShipInPosition(char *ships, int position) {
    if (position < 0 || position >= GRID_SIZE) return false;
    return ships[position] == 1;
}

void drawBoard(char *board, char *ships, int showShips) {
    printf("\n   ");
    for (int col = 0; col < WIDTH; col++) {
        printf(" %d", col);
    }
    printf("\n");

    for (int row = 0; row < WIDTH; row++) {
        printf("%d  ", row);
        for (int col = 0; col < WIDTH; col++) {
            int index = row * WIDTH + col;
            char symbol = '.';

            if (board[index] == HIT) {
                symbol = 'X';
            } else if (board[index] == MISS) {
                symbol = 'O';
            } else if (ships[index] == 1 && showShips) {
                symbol = 'S';
            }

            printf(" %c", symbol);
        }
        printf("\n");
    }
}

void cleanMemory(char *area) {
    memset(area, 0, GRID_SIZE * 2 + 2);
}

void placeShip(char *ships, int length, int orientation, int pos) {
    for (int i = 0; i < length; i++) {
        int p = pos + (orientation ? i : WIDTH * i);
        ships[p] = 1;
    }
}

int tilesAvailable(char *ships, int orientation, int length, int pos) {
    int row, col;
    for (int i = 0; i < length; i++) {
        int p = pos + (orientation ? i : WIDTH * i);
        if (p < 0 || p >= GRID_SIZE)
            return false;

        row = p / WIDTH;
        col = p % WIDTH;
        if (ships[p] == 1) {
            return false;
        }
    }
    return true;
}

void autoPlaceShips(char *ships) {
    int lengths[] = {4, 3, 3, 2, 2};
    for (int i = 0; i < 5; i++) {
        while (true) {
            int orientation = rand() % 2;
            int row = rand() % WIDTH;
            int col = rand() % WIDTH;
            int pos = row * WIDTH + col;

            if (orientation == VERTICAL && row + lengths[i] > WIDTH)
                continue;
            if (orientation != VERTICAL && col + lengths[i] > WIDTH)
                continue;

            if (tilesAvailable(ships, orientation, lengths[i], pos)) {
                placeShip(ships, lengths[i], orientation, pos);
                break;
            }
        }
    }
}

int allShipsSunk(char *board, char *ships) {
    for (int i = 0; i < GRID_SIZE; i++) {
        if (ships[i] == 1 && board[i] != HIT)
            return false;
    }
    return true;
}

int chooseAttackCell(char *opponentBoard) {
    int pos;
    do {
        pos = rand() % GRID_SIZE;
    } while (opponentBoard[pos] != UNMARKED);
    return pos;
}

void playGame(char* turn, char* data, char* board, char* opponentBoard, int player, char* ships, char* opponentShips) {
    if (player == PARENT) {
        printf("\nGame started. You are the Parent player.\n");
    }

    while (*turn != -1) {
        if (*turn == player) {
            int pos;
            int row, col;

            if (player == PARENT) {
                // Parent player: prompt for input
                drawBoard(board, ships, 1); // Show own ships
                printf("\nYour turn. Enter attack coordinates (row col), 'p' to pause, or 'm' to return to menu: ");
                char input[10];
                fgets(input, sizeof(input), stdin);

                // Handle 'p' and 'm' commands
                if (input[0] == 'p' || input[0] == 'P') {
                    // Save current boards to text file
                    saveBoardsToFile(board, opponentBoard, ships, opponentShips);
                    printf("\nGame paused. Boards saved to 'boards.txt'.\n");
                    continue; // Continue the game
                }

                if (input[0] == 'm' || input[0] == 'M') {
                    // Return to menu
                    saveGameStateOnExit(turn, data, board, opponentBoard, ships, opponentShips, player);
                    printf("\nReturning to menu...\n");
                    *turn = -2; // Signal to return to menu
                    return;
                }

                if (sscanf(input, "%d %d", &row, &col) != 2 || row < 0 || row >= WIDTH || col < 0 || col >= WIDTH) {
                    printf("Invalid coordinates. Try again.\n");
                    continue;
                }
                pos = row * WIDTH + col;
            } else {
                // Child player: AI move
                pos = chooseAttackCell(opponentBoard);
                printf("\nChild attacks position (%d, %d)\n", pos / WIDTH, pos % WIDTH);
            }

            // Check if the position has already been attacked
            if (opponentBoard[pos] != UNMARKED) {
                if (player == PARENT) {
                    printf("Position already attacked. Try again.\n");
                }
                continue;
            }

            // Update the opponent's board
            int hit = checkIfShipInPosition(opponentShips, pos);
            opponentBoard[pos] = hit ? HIT : MISS;

            // Inform the player of the result
            if (player == PARENT) {
                printf("%s!\n", hit ? "Hit" : "Miss");
            } else {
                printf("%s\n", hit ? "Child hits your ship!" : "Child misses.");
            }

            // Check for victory
            if (allShipsSunk(opponentBoard, opponentShips)) {
                if (player == PARENT) {
                    printf("\nYou won! Congratulations.\n");
                } else {
                    printf("\nChild won the game.\n");
                }
                *turn = -1;
                return;
            }

            // Switch turn
            *turn = !player;
        } else {
            // Opponent's turn: wait or display a message
            if (player == PARENT) {
                printf("\nChild's turn...\n");
                sleep(1); // Wait a bit before checking again
            }
        }
    }
}

void saveGameState(GameState *state) {
    FILE *file = fopen("savegame.txt", "w");
    if (file != NULL) {
        fprintf(file, "%d %d %d\n", state->turn, state->data, state->player);

        // Write board arrays
        for (int i = 0; i < GRID_SIZE; i++)
            fprintf(file, "%d ", state->board[i]);
        fprintf(file, "\n");

        for (int i = 0; i < GRID_SIZE; i++)
            fprintf(file, "%d ", state->opponentBoard[i]);
        fprintf(file, "\n");

        // Write ships arrays
        for (int i = 0; i < GRID_SIZE; i++)
            fprintf(file, "%d ", state->ships[i]);
        fprintf(file, "\n");

        for (int i = 0; i < GRID_SIZE; i++)
            fprintf(file, "%d ", state->opponentShips[i]);
        fprintf(file, "\n");

        fclose(file);
    } else {
        perror("Failed to save game state");
    }
}

int loadGameState(GameState *state) {
    FILE *file = fopen("savegame.txt", "r");
    if (file != NULL) {
        if (fscanf(file, "%d %d %d", (int *)&(state->turn), (int *)&(state->data), &(state->player)) != 3) {
            fclose(file);
            return 0;
        }

        // Read board arrays
        for (int i = 0; i < GRID_SIZE; i++)
            if (fscanf(file, "%d", (int *)&(state->board[i])) != 1) {
                fclose(file);
                return 0;
            }

        for (int i = 0; i < GRID_SIZE; i++)
            if (fscanf(file, "%d", (int *)&(state->opponentBoard[i])) != 1) {
                fclose(file);
                return 0;
            }

        // Read ships arrays
        for (int i = 0; i < GRID_SIZE; i++)
            if (fscanf(file, "%d", (int *)&(state->ships[i])) != 1) {
                fclose(file);
                return 0;
            }

        for (int i = 0; i < GRID_SIZE; i++)
            if (fscanf(file, "%d", (int *)&(state->opponentShips[i])) != 1) {
                fclose(file);
                return 0;
            }

        fclose(file);
        return 1; // Successfully loaded
    }
    return 0; // No saved game found
}

void saveBoardsToFile(char *board, char *opponentBoard, char *ships, char *opponentShips) {
    FILE *file = fopen("boards.txt", "w");
    if (file != NULL) {
        fprintf(file, "Player's Board:\n");
        for (int i = 0; i < GRID_SIZE; i++) {
            fprintf(file, "%d ", board[i]);
            if ((i + 1) % WIDTH == 0)
                fprintf(file, "\n");
        }
        fprintf(file, "\nPlayer's Ships:\n");
        for (int i = 0; i < GRID_SIZE; i++) {
            fprintf(file, "%d ", ships[i]);
            if ((i + 1) % WIDTH == 0)
                fprintf(file, "\n");
        }
        fprintf(file, "\nOpponent's Board:\n");
        for (int i = 0; i < GRID_SIZE; i++) {
            fprintf(file, "%d ", opponentBoard[i]);
            if ((i + 1) % WIDTH == 0)
                fprintf(file, "\n");
        }
        fprintf(file, "\nOpponent's Ships:\n");
        for (int i = 0; i < GRID_SIZE; i++) {
            fprintf(file, "%d ", opponentShips[i]);
            if ((i + 1) % WIDTH == 0)
                fprintf(file, "\n");
        }
        fclose(file);
    } else {
        perror("Failed to save boards to file");
    }
}

void saveGameStateOnExit(char* turn, char* data, char* board, char* opponentBoard, char* ships, char* opponentShips, int player) {
    GameState state;
    state.turn = *turn;
    state.data = *data;
    memcpy(state.board, board, GRID_SIZE);
    memcpy(state.opponentBoard, opponentBoard, GRID_SIZE);
    memcpy(state.ships, ships, GRID_SIZE);
    memcpy(state.opponentShips, opponentShips, GRID_SIZE);
    state.player = player;
    saveGameState(&state);
    printf("\nGame state saved to 'savegame.txt'.\n");

    // Also save the boards
    saveBoardsToFile(board, opponentBoard, ships, opponentShips);
}
