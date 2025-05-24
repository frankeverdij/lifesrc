#include <stdlib.h>
#include "allocatecell.h"
#include "deadcell.h"
#include "state.h"
#include "enums.h"
#include "macros.h"

static Cell * newCells = NULL;   /* storage for allocating cells */
static int    newCellCount = 0;  /* amount of allocated cells in block */

/*
 * Allocate a new cell.
 * The cell is initialized as if it was a boundary cell.
 * Warning: The first allocation MUST be of the deadCell.
 */
Cell * allocateCell(void)
{
    Cell * cell;

    /*
     * Allocate a new chunk of cells if there are none left.
     */
    if (newCellCount <= 0)
    {
        newCells = (Cell *) malloc(sizeof(Cell) * ALLOC_SIZE);

        if (newCells == NULL)
            FATAL("Cannot allocate cell structure\n");

        newCellCount = ALLOC_SIZE;
    }

    newCellCount--;
    cell = newCells++;

    /*
     * If this is the first allocation, then make deadCell be this cell.
     */
    if (deadCell == NULL)
        deadCell = cell;

    /*
     * Fill in the cell as if it was a boundary cell.
     */
    cell->state = OFF;
    cell->free = FALSE;
    cell->frozen = FALSE;
    cell->choose = TRUE;
    cell->gen = -1;
    cell->row = -1;
    cell->col = -1;
    cell->sumNear = 0;
    cell->index = -1;
    cell->past = deadCell;
    cell->future = deadCell;
    cell->cul = deadCell;
    cell->cu = deadCell;
    cell->cur = deadCell;
    cell->cl = deadCell;
    cell->cr = deadCell;
    cell->cdl = deadCell;
    cell->cd = deadCell;
    cell->cdr = deadCell;
    cell->loop = NULL;

    return cell;
}
