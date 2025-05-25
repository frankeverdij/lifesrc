#include "lifesrc.h"

/*
 * Check to see if any other generation is identical to generation 0.
 * This is used to detect and weed out all objects with subPeriods.
 * (For example, stable objects or period 2 objects when using -g4.)
 * Returns TRUE if there is an identical generation.
 */
Bool subPeriods(void)
{
    const Cell * cellG0;
    const Cell * cellGn;

    for (int gen = 1; gen < genMax; gen++)
    {
        if (genMax % gen)
            continue;

        for (int row = 1; row <= rowMax; row++)
        {
            for (int col = 1; col <= colMax; col++)
            {
                cellG0 = findCell(row, col, 0);
                cellGn = findCell(row, col, gen);

                if (cellG0->state != cellGn->state)
                    goto nextGen;
            }
        }

        return TRUE;
nextGen:;
    }

    return FALSE;
}
