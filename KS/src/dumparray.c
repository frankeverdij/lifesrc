#include <stdio.h>
#include "cell.h"

void dumparray(Cell ** const table, const int n)
{
    Cell * cell;

    printf("r c g s f o a u\n");
    for (int i=0; i<n; i++)
    {
        cell = table[i];
        if (cell)
            printf("%d %d %d %d %x %x %x %x\n",cell->row, cell->col, cell->gen, cell->state, cell->free, cell->frozen, cell->active, cell->unchecked);
    }
    return;
}

