#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

enum direction { N, NE, E, SE, S, SW, W, NW };

typedef struct {
    uint64_t board;
    char *name;
    char *symbol;
} Player;

Player *newPlayer(char *name, char *symbol, uint64_t board) {
    Player *player;
    if ((player = malloc(sizeof(Player))) == NULL) {
        perror("Memory Allocation failed");
        exit(EXIT_FAILURE);
    }

    player->name = strdup(name);
    player->symbol = strdup(symbol);
    player->board = board;

    return player;
}

void freePlayers(Player *players[]) {
    for (int i = 0; i < 3; i++) {
        free(players[i]);
    }
}

int getBitCount(uint64_t num) {
    int bitCount = 0;
    while (num) {
        bitCount += num & 1;
        num >>= 1;
    }
    return bitCount;
}

void printScore(int bScore, int wScore) {
    printf("\n         ╭───┬────┬─┬───┬────╮");
    printf("\n         │ B │ %02d │ │ W │ %02d │", bScore, wScore);
    printf("\n         ╰───┴────┴─┴───┴────╯\n\n");
}

void printBoard(Player *players[], int count) {
    printScore(getBitCount(players[0]->board), getBitCount(players[1]->board));

    char topLetters[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    printf("    ");
    for (int i = 0; i < 8; i++) {
        printf("%c   ", topLetters[i]);
    }
    printf("\n  ╭───┬───┬───┬───┬───┬───┬───┬───╮\n");
    for (int i = 0; i < 64; i++) {
        if (i % 8 == 0) {
            printf("%d │", (i / 8) + 1);
        }

        uint64_t pos = (uint64_t)1 << i;
        int isOccupied = false;
        for (int i = 0; i < count; i++) {
            if ((players[i]->board & pos) > 0) {
                printf(" %s │", players[i]->symbol);
                isOccupied = true;
            }
        }
        if (!isOccupied) {
            printf("   │");
        }

        if ((i + 1) % 8 == 0 && i < 56) {
            printf("\b│\n  ├───┼───┼───┼───┼───┼───┼───┼───┤\n");
        }
    }

    printf("\b│\n  ╰───┴───┴───┴───┴───┴───┴───┴───╯\n");
}

char *getInput(char *playerName, Player *players[]) {
    printf("\n%s's: ", playerName);
    // Read into buffer
    char buffer[256];
    if (fgets(buffer, 256, stdin) == NULL) {
        printf("goodbye\n");
        freePlayers(players);
        exit(EXIT_SUCCESS);
    }

    int length = strlen(buffer);
    if (length != 3 || buffer[2] != '\n') {
        return NULL;
    }

    char *input;
    if ((input = malloc(3)) == NULL) {
        perror("memory allocation failed\n");
        freePlayers(players);
        exit(EXIT_FAILURE);
    }

    strncpy(input, buffer, 3);
    input[2] = '\0';
    return input;
}

int calcOffset(char *input) {
    //
    if (input[0] < 'a' || input[0] > 'h') {
        return -1;
    }

    if (input[1] < '1' || input[1] > '8') {
        return -1;
    }

    int y = (input[1] - '1') * 8;
    int x = input[0] - 'a';

    return x + y;
}

int getPosIndex(uint64_t pos) {
    int index = -1;
    while (pos) {
        index++;
        pos >>= 1;
    }
    return index;
}

uint64_t updatePos(enum direction dir, uint64_t pos) {
    int index = getPosIndex(pos);

    if (dir == N && index > 7) {
        pos >>= 8;
        return pos;
    }

    if (dir == NE && index > 7 && (index + 1) % 8 > 0) {
        pos >>= 7;
        return pos;
    }

    if (dir == E && (index + 1) % 8 > 0) {
        pos <<= 1;
        return pos;
    }

    if (dir == SE && index < 56 && (index + 1) % 8 > 0) {
        pos <<= 9;
        return pos;
    }

    if (dir == S && index < 56) {
        pos <<= 8;
        return pos;
    }

    if (dir == SW && index < 57 && index % 8 > 0) {
        pos <<= 7;
        return pos;
    }

    if (dir == W && index % 8 > 0) {
        pos >>= 1;
        return pos;
    }

    if (dir == NW && index > 8 && index % 8 > 0) {
        pos >>= 9;
        return pos;
    }

    return 0;
}

uint64_t searchBoard(Player *players[], uint64_t pos, uint64_t stack, int curr,
                     int other, enum direction dirs[], int dirCount) {
    uint64_t board = players[curr]->board | players[other]->board;
    int depth = getBitCount(stack);

    // Base case 1: can't play on a used grid
    if (depth == 0 && (board & pos) > 0) {
        return 0;
    }

    // Base case 2: can't play on space after 1st play
    if (depth > 0 && (~board & pos) > 0) {
        return 0;
    }

    // Base case 3: can't play on own tile in 2nd move
    if (depth == 1 && (players[curr]->board & pos) > 0) {
        return 0;
    }

    // Update stack
    stack |= pos;

    // Base case 4: we found a path
    if (depth > 1 && (players[curr]->board & pos) > 0) {
        return stack;
    }

    // Recursive case
    uint64_t result = 0;
    for (int i = 0; i < dirCount; i++) {
        // Update position ensuring that its within bounds
        enum direction dir = dirs[i];
        uint64_t newPos = updatePos(dir, pos);
        if (newPos == 0) {
            continue;
        }
        enum direction newDirs[1] = {dir};

        result |= searchBoard(players, newPos, stack, curr, other, newDirs, 1);
    }

    if (result > 0) {
        return result;
    }

    return 0;
}

uint64_t getMoves(Player *players[], int curr, int other) {
    enum direction dirs[] = {N, NE, E, SE, S, SW, W, NW};

    uint64_t moves = 0;
    for (int i = 0; i < 64; i++) {
        uint64_t pos = (uint64_t)1 << i;

        moves |= searchBoard(players, pos, 0, curr, other, dirs, 8);
    }

    return moves;
}

int main() {
    Player *players[3];
    players[0] = newPlayer("Black", "◉", (uint64_t)0x810000000);
    players[1] = newPlayer("White", "○", (uint64_t)0x1008000000);
    players[2] = newPlayer("Hint", "‣", (uint64_t)0x0);

    int curr = 0;
    int other = 1;
    char *message = "";
    unsigned canPlay = 0;
    enum direction dirs[] = {N, NE, E, SE, S, SW, W, NW};

    while (true) {
        // Get available moves
        uint64_t availableMoves = getMoves(players, curr, other);
        if (availableMoves == 0) {
            canPlay = (canPlay << 1) + 1;
            continue;
        }

        // Assign placeholders for available moves
        uint64_t board = players[curr]->board | players[other]->board;
        players[2]->board = availableMoves ^ (availableMoves & board);

        // Redraw board
        system("clear");
        printBoard(players, 3);
        printf("%s", message);

        if (canPlay >= 7) {
            break;
        }

        // Get input
        char *input = getInput(players[curr]->name, players);
        if (input == NULL) {
            message = "\n--- Invalid input! ---\n";
            continue;
        }

        int offset = calcOffset(input);
        free(input);
        if (offset < 0) {
            message = "\n--- Invalid input! ---\n";
            continue;
        }

        // Has to be a valid move
        uint64_t pos = (uint64_t)1 << offset;
        if ((pos & (availableMoves & ~board)) == 0) {
            message = "\n-- Can't play there! --\n";
            continue;
        }

        // // Can't play on a spot that's taken
        if ((board & pos) > 0) {
            message = "\n--- Can't play there! ---\n";
            continue;
        }

        uint64_t result = searchBoard(players, pos, 0, curr, other, dirs, 8);

        // update board
        players[curr]->board |= result;
        players[other]->board =
            players[other]->board ^ (players[other]->board & result);

        // update current
        curr = (curr + 1) % 2;
        other = (curr + 1) % 2;
        message = "";

        // Update canPlay
        canPlay >>= 1;
    }

    freePlayers(players);

    return 0;
}
