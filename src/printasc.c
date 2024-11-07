#include "lifesrc.h"
#include "printasc.h"
#include "state.h"

static const char *ascii[128] =
    {".", "O", "\033[1m.\033[0m", "\033[1mO\033[0m", "\033[7m.\033[0m", "\033[7mO\033[0m", "\033[1m\033[7m.\033[0m", "\033[1m\033[7mO\033[0m",
     "\033[2m.\033[0m", "\033[2mO\033[0m", "\033[2m.\033[0m", "\033[2mO\033[0m",
     "\033[2m\033[7m.\033[0m","\033[2m\033[7mO\033[0m", "\033[2m\033[7m.\033[0m", "\033[2m\033[7mO\033[0m",
     "\033[4m.\033[0m", "\033[4mO\033[0m", "\033[1m\033[4m.\033[0m", "\033[1m\033[4mO\033[0m", "\033[7m\033[4m.\033[0m", "\033[7m\033[4mO\033[0m", "\033[1m\033[7m\033[4m.\033[0m", "\033[1m\033[7m\033[4mO\033[0m",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "?", " ", "\033[1m?\033[0m", " ", "\033[7m?\033[0m", " ", "\033[1m\033[7m?\033[0m", " ",
     "\033[2m?\033[0m", " ", "\033[2m?\033[0m", " ", "\033[2m\033[7m?\033[0m", " ", "\033[2m\033[7m?\033[0m", " ",
     "\033[4m?\033[0m", " ", "\033[1m\033[4m?\033[0m", " ", "\033[7m\033[4m?\033[0m", " ", "\033[1m\033[7m\033[4m?\033[0m", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "+", " ", "\033[1m+\033[0m", " ", "\033[7m+\033[0m", " ", "\033[1m\033[7m+\033[0m", " ",
     "\033[2m+\033[0m", " ", "\033[2m+\033[0m", " ", "\033[2m\033[7m+\033[0m", " ", "\033[2m\033[7m+\033[0m", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     "X", " ", "\033[1mX\033[0m", " ", "\033[7mX\033[0m", " ", "\033[1m\033[7mX\033[0m", " ",
     "\033[2mX\033[0m", " ", "\033[2mX\033[0m", " ", "\033[2m\033[7mX\033[0m", " ", "\033[2m\033[7mX\033[0m", " ",
     " ", " ", " ", " ", " ", " ", " ", " ",
     " ", " ", " ", " ", " ", " ", " ", " "};

void printAsc(const int gen, const Bool augment, const globals * const g)
{
    Bool changeFlag;
    int idx;
    Cell * cell;

    for (int row = 1; row <= g->rowMax; row++)
    {
        for (int col = 1; col <= g->colMax; col++)
        {

            cell = findCell(row, col, gen, g);
            idx = cell->state;

            changeFlag = FALSE;
            for (int i = 0; i < g->genMax; i++)
            {
                if (i == gen) continue;
                if (findCell(row, col, i, g)->state != idx) changeFlag = TRUE;
            }

            if (cell->flags & FROZENCELL) idx = 64;
            if (!(cell->flags & CHOOSECELL)) idx = 96;
            if (augment)
            {
                /* cells with changed states between generations are marked bold */
                if (changeFlag) idx += 2;
                /* inverse highlight the first searchable cell */
                if (cell->index == 0) idx += 4;
                /* dim all cells not in the searchlist */
                if (cell->index < 0) idx += 8;
            }

            ttyPrintf("%s ", ascii[idx]);
        }

        ttyWrite("\n", 1);
    }

    return;
}
