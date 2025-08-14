#include "lifesrc.h"
#include "allocatecell.h"

static int auxCellCount = 0; /* cells in auxillary table */
static Cell * auxTable[MAX_CELLS]; /* table of auxillary cells */

/*
 * Find a cell given its coordinates.
 * Most coordinates range from 0 to colMax+1, 0 to rowMax+1, and 0 to genMax-1.
 * Cells within this range are quickly found by indexing into cellTable.
 * Cells outside of this range are handled by searching an auxillary table,
 * and are dynamically created as necessary.
 */
Cell * findCell(const int row, const int col, const int gen)
{
    Cell * cell;

    /*
     * If the cell is a normal cell, then we know where it is.
     */
    if ((row >= 0) && (row <= rowMax + 1) &&
        (col >= 0) && (col <= colMax + 1) &&
        (gen >= 0) && (gen < genMax))
    {
        return cellTable[(col * (rowMax + 2) + row) * genMax + gen];
    }

    /*
     * See if the cell is already allocated in the auxillary table.
     */
    for (int i = 0; i < auxCellCount; i++)
    {
        cell = auxTable[i];

        if ((cell->row == row) && (cell->col == col) &&
            (cell->gen == gen))
        {
            return cell;
        }
    }

    /*
     * Need to allocate the cell and add it to the auxillary table.
     */
    if (auxCellCount >= AUX_CELLS)
        fatal("Too many auxillary cells");

    cell = allocateCell(smartOn);
    cell->row = row;
    cell->col = col;
    cell->gen = gen;
    auxTable[auxCellCount++] = cell;
    
    DPRINTF("Aux cell %d allocated at %d %d %d\n", auxCellCount, row, col, gen);

    return cell;
}
