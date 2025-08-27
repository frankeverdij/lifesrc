#include "lifesrc.h"

/*
 * Return the mapping of a cell from the last generation back to the first
 * generation, or vice versa.  This implements all flipping and translating
 * of cells between these two generations.  This routine should only be
 * called for cells belonging to those two generations.
 */
Cell * mapCell(const Cell * cell, Bool forward)
{
    int row;
    int col;
    int tmp;

    row = cell->row;
    col = cell->col;

    if (flipRows && (col >= flipRows))
        row = rowMax + 1 - row;

    if (flipCols && (row >= flipCols))
        col = colMax + 1 - col;

    if (flipPoint)
    {                /* NEED TO GO BACKWARDS */
        tmp = col;
        col = colMax + 1 - col;
        row = rowMax + 1 - row;
    }

    if (flipQuads)
    {                /* NEED TO GO BACKWARDS */
        tmp = col;
        col = row;
        row = colMax + 1 - tmp;
    }

    if (forward)
    {
        if (flipFwd)
        {       /* For Glide Symmetry */
            tmp = col;
            col = rowMax + 1 - col;
            row = colMax + 1 - tmp;
        }
        if (flipBwd)
        {
            tmp = col;
            col = row;
            row = tmp;
        }

        row += rowTrans;
        col += colTrans;
    }
    else
    {
        row -= rowTrans;
        col -= colTrans;

        if (flipFwd)
        {       /* For Glide Symmetry */
            tmp = col;
            col = rowMax + 1 - col;
            row = colMax + 1 - tmp;
        }
        if (flipBwd)
        {
            tmp = col;
            col = row;
            row = tmp;
        }
    }

    if (forward)
        return findCell(row, col, 0);
    else
        return findCell(row, col, genMax - 1);
}
