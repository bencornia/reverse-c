#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum { N, NE, E, SE, S, SW, W, NW } direction_t;

typedef struct {
    unsigned long long moves;
    char *name;
    char *symbol;
} player_t;

player_t *newPlayer(char *name, char *symbol, unsigned long long board) {
    player_t *player;
    if ((player = malloc(sizeof(player_t))) == NULL) {
        perror("Memory Allocation failed");
        exit(EXIT_FAILURE);
    }

    player->name = strdup(name);
    player->symbol = strdup(symbol);
    player->moves = board;

    return player;
}

typedef struct {
    // Construction characters
    char *sepHoriz;
    char *sepVert;
    char *intersectTop;
    char *intersectBottom;
    char *intersectLeft;
    char *intersectRight;
    char *intersectCenter;
    char *cornerTopLeft;
    char *cornerTopRight;
    char *cornerBottomLeft;
    char *cornerBottomRight;

    // Axis labels
    char xLabels[8];
    char yLabels[8];
} gameboard_t;

void logger(player_t *players[], int canPlay) {
    FILE *file = fopen("log.txt", "a");
    fprintf(file, "%llx,%llx,%x\n", players[0]->moves, players[1]->moves,
            canPlay);

    fclose(file);
}

void freePlayers(player_t *players[]) {
    for (int i = 0; i < 3; i++) {
        free(players[i]);
    }
}

int getBitCount(unsigned long long num) {
    int bitCount = 0;
    while (num) {
        bitCount += num & 1;
        num >>= 1;
    }
    return bitCount;
}

void printScore(player_t *player1, player_t *player2) {
    printf("\n         ╭───┬────┬─┬───┬────╮");
    printf("\n         │ %s │ %02d │ │ %s │ %02d │", player1->symbol,
           getBitCount(player1->moves), player2->symbol,
           getBitCount(player2->moves));
    printf("\n         ╰───┴────┴─┴───┴────╯\n\n");
}

void printBoard(player_t *players[], int count) {
    printScore(players[0], players[1]);

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

        unsigned long long pos = (unsigned long long)1 << i;
        int isOccupied = false;
        for (int i = 0; i < count; i++) {
            if ((players[i]->moves & pos) > 0) {
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

char *getInput(char *playerName, player_t *players[]) {
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

int getPosIndex(unsigned long long pos) {
    int index = -1;
    while (pos) {
        index++;
        pos >>= 1;
    }
    return index;
}

unsigned long long updatePos(direction_t dir, unsigned long long pos) {
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

unsigned long long searchBoard(player_t *players[], unsigned long long pos,
                               unsigned long long stack, int curr, int other,
                               direction_t dirs[], int dirCount) {
    unsigned long long board = players[curr]->moves | players[other]->moves;
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
    if (depth == 1 && (players[curr]->moves & pos) > 0) {
        return 0;
    }

    // Update stack
    stack |= pos;

    // Base case 4: we found a path
    if (depth > 1 && (players[curr]->moves & pos) > 0) {
        return stack;
    }

    // Recursive case
    unsigned long long result = 0;
    for (int i = 0; i < dirCount; i++) {
        // Update position ensuring that its within bounds
        direction_t dir = dirs[i];
        unsigned long long newPos = updatePos(dir, pos);
        if (newPos == 0) {
            continue;
        }
        direction_t newDirs[1] = {dir};

        result |= searchBoard(players, newPos, stack, curr, other, newDirs, 1);
    }

    if (result > 0) {
        return result;
    }

    return 0;
}

unsigned long long getMoves(player_t *players[], int curr, int other) {
    direction_t dirs[] = {N, NE, E, SE, S, SW, W, NW};

    unsigned long long moves = 0;
    for (int i = 0; i < 64; i++) {
        unsigned long long pos = (unsigned long long)1 << i;

        moves |= searchBoard(players, pos, 0, curr, other, dirs, 8);
    }

    return moves;
}

void switchCurrentPlayer(int *curr, int *other) {
    *curr = (*curr + 1) % 2;
    *other = (*other + 1) % 2;
}

int main() {
    player_t *players[3] = {
        newPlayer("Black", "◉", (unsigned long long)0x810000000),
        newPlayer("White", "○", (unsigned long long)0x1008000000),
        newPlayer("Hint", "‣", (unsigned long long)0x0)};

    int curr = 0;
    int other = 1;
    char *message = "";
    unsigned canPlay = 0;
    direction_t dirs[] = {N, NE, E, SE, S, SW, W, NW};

    while (true) {
        // Get available moves
        unsigned long long availableMoves = getMoves(players, curr, other);

        // Assign placeholders for available moves
        unsigned long long board = players[curr]->moves | players[other]->moves;
        players[2]->moves = availableMoves ^ (availableMoves & board);

        // Redraw board
        system("clear");
        printBoard(players, 3);

        if (canPlay >= 7) {
            break;
        }

        printf("%s", message);
        if (availableMoves == 0) {
            switchCurrentPlayer(&curr, &other);
            canPlay = (canPlay << 1) + 1;
            continue;
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
        unsigned long long pos = (unsigned long long)1 << offset;
        if ((pos & (availableMoves & ~board)) == 0) {
            message = "\n-- Can't play there! --\n";
            continue;
        }

        // // Can't play on a spot that's taken
        if ((board & pos) > 0) {
            message = "\n--- Can't play there! ---\n";
            continue;
        }

        unsigned long long result = searchBoard(
            players, pos, (unsigned long long)0, curr, other, dirs, 8);

        // update board
        players[curr]->moves |= result;
        players[other]->moves =
            players[other]->moves ^ (players[other]->moves & result);

        // update current
        switchCurrentPlayer(&curr, &other);
        message = "";

        canPlay >>= 1;
    }

    freePlayers(players);

    return 0;
}
