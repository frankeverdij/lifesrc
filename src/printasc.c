#include "lifesrc.h"
#include "state.h"

void printAsc(const int gen)
{
    const char * msg;
    Cell * cell;

    for (int row = 1; row <= rowMax; row++)
    {
        for (int col = 1; col <= colMax; col++)
        {
            cell = findCell(row, col, gen);

            switch (cell->state)
            {
                case OFF:
                    msg = ". ";
                    break;

                case ON:
                    msg = "O ";
                    break;

                case UNK:
                    msg = "? ";

                    if (cell->flags & FROZENCELL)
                        msg = "+ ";

                    if (!(cell->flags & CHOOSECELL))
                        msg = "X ";

                    break;
            }

            /*
             * If wide output, print only one character,
             * else print both characters.
             */
            ttyWrite(msg, (colMax < 40) + 1);
        }

        ttyWrite("\n", 1);
    }

    return;
}
