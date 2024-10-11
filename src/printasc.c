#include "lifesrc.h"
#include "state.h"

static const char *ascii[128] =
    {".", "O", "\033[9m.\033[29m", "\033[9mO\033[29m", "\033[7m.\033[27m", "\033[7mO\033[27m", "\033[4m.\033[24m", "\033[4mO\033[24m", 
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "?", " ", "\033[9m?\033[29m", " ", "\033[7m?\033[27m", " ", "\033[4m?\033[24m", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "+", " ", "\033[9m+\033[29m", " ", "\033[7m+\033[27m", " ", "\033[4m+\033[24m", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "X", " ", "\033[9mX\033[29m", " ", "\033[7mX\033[27m", " ", "\033[4mX\033[24m", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " "};

void printAsc(const int gen, const bool augment)
{
    int idx;
    Cell * cell;

    for (int row = 1; row <= rowMax; row++)
    {
        for (int col = 1; col <= colMax; col++)
        {
            cell = findCell(row, col, gen);

            idx = cell->state;

            if (cell->flags & FROZENCELL) idx = 64;
            if (!(cell->flags & CHOOSECELL)) idx = 96;
            if (augment)
            {
                if (cell->index < 0) idx += 2;
                if (cell->index == 0) idx += 4;
            }

            ttyPrintf("%s ", ascii[idx]);
        }

        ttyWrite("\n", 1);
    }

    return;
}
