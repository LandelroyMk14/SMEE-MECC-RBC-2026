#include <stdlib.h>

#define XPOS 0
#define YPOS 1
#define XNEG 2
#define YNEG 3

typedef struct {
    int direction;
    int position[2];
} state;

// Initial struct
state robot_state = {XPOS, 0, 0};


char *navigate(state *state, int dest[2])
{
    static char moves[100] = "";

    int orientation = state->direction;
    int *pos = state->position;

    int n = 0;

    int x_move = dest[0] - pos[0];
    int y_move = dest[1] - pos[1];

    // Move in X direction
    if (x_move != 0) {
        int required;

        if (x_move > 0)
            required = XPOS;
        else
            required = XNEG;

        int turn = (required - orientation + 4) % 4;

        if (turn == 1)
            moves[n++] = 'R';
        else if (turn == 2)
            moves[n++] = 'B';
        else if (turn == 3)
            moves[n++] = 'L';

        orientation = required;

        for (int i = 0; i < abs(x_move); i++)
            moves[n++] = 'F';
    }

    // Move in Y direction
    if (y_move != 0) {
        int required;

        if (y_move > 0)
            required = YPOS;
        else
            required = YNEG;

        int turn = (required - orientation + 4) % 4;

        if (turn == 1)
            moves[n++] = 'R';
        else if (turn == 2)
            moves[n++] = 'B';
        else if (turn == 3)
            moves[n++] = 'L';

        orientation = required;

        for (int i = 0; i < abs(y_move); i++)
            moves[n++] = 'F';
    }

    state->direction = orientation;
    moves[n] = '\0';
    return moves;
}