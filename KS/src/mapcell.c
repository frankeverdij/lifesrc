#include "lifesrc.h"
#include "cell.h"
#include "bool.h"

/*
 * Return the mapping of a cell from the last generation back to the first
 * generation, or vice versa.  This implements all flipping and translating
 * of cells between these two generations.  This routine should only be
 * called for cells belonging to those two generations.
 */
Cell * mapCell(Cell * cell)
{
    int row;
    int col;
    int tmp;
    Bool forward;

    row = cell->row;
    col = cell->col;
    forward = (cell->gen != 0);

    if (fliprows && (col >= fliprows))
        row = rowmax + 1 - row;

    if (flipcols && (row >= flipcols))
        col = colmax + 1 - col;

    if (flipquads)
    {                /* NEED TO GO BACKWARDS */
        tmp = col;
        col = row;
        row = colmax + 1 - tmp;
    }

    if (forward)
    {
        row += rowtrans;
        col += coltrans;
    }
    else
    {
        row -= rowtrans;
        col -= coltrans;
    }

    if (forward)
        return findcell(row, col, 0);
    else
        return findcell(row, col, genmax - 1);
}
