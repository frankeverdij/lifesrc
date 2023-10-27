#include "lifesrc.h"

static const char *blockGfx[128] =
    {" ", "\u2580", "\u2584", "\u2588", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\U0001fb8e", " ", "\U0001fb92", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\U0001fb8f", "\U0001fb91", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "\U0001fb90", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " "};

void printBlk(const int gen)
{
    int row;
    int col;
    int twoStates;
    const Cell *up, *down;
    
    for (row = 0; row < rowMax; row += 2)
    {
        for (col = 1; col <= colMax; col++)
        {
            up = findCell(row + 1, col, gen);
            twoStates = up->state;
            
            if (row + 1 < rowMax)
            {
                down = findCell(row + 2, col, gen);
                twoStates += 2 * down->state;
            }
            
            ttyPrintf("%s", blockGfx[twoStates]);
        }

        ttyWrite("\n", 1);
    }

    return;
}
