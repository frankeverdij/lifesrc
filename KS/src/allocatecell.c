#include <stdlib.h>
#include "cell.h"
#include "state.h"
#include "bool.h"
#include "macros.h"

#define    ALLOCSIZE    100        /* chunk size for cell allocation */

static Cell * newCells = NULL;   /* storage for allocating cells */
static int    newCellCount = 0;  /* amount of allocated cells in block */

/*
 * Allocate a new cell.
 * The cell is initialized as if it was a boundary cell.
 */
Cell * allocateCell()
{
    Cell * cell;

    /*
     * Allocate a new chunk of cells if there are none left.
     */
    if (newCellCount <= 0)
    {
        newCells = (Cell *) malloc(sizeof(Cell) * ALLOCSIZE);

        if (newCells == NULL)
        {
            FATAL("Cannot allocate cell structure\n");
        }

        //record_malloc(1,(void*)newcells);

        newCellCount = ALLOCSIZE;
    }

    newCellCount--;
    cell = newCells++;

    /*
     * Fill in the cell as if it was a boundary cell.
     */
    cell->state = OFF;
    cell->free = FALSE;
    cell->frozen = FALSE;
    cell->active = TRUE;
    cell->choose = TRUE;
    cell->gen = -1;
    cell->row = -1;
    cell->col = -1;
    cell->sumNear = 0;
    cell->index = -1;
    cell->past = cell;
    cell->future = cell;
    cell->cul = cell;
    cell->cu = cell;
    cell->cur = cell;
    cell->cl = cell;
    cell->cr = cell;
    cell->cdl = cell;
    cell->cd = cell;
    cell->cdr = cell;
    cell->loop = cell;

    return cell;
}
