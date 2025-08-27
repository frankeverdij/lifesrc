#include "lifesrc.h"

/*
 * Return a cell which is symmetric to the given cell.
 * It is not necessary to know all symmetric cells to a single cell,
 * as long as all symmetric cells are chained in a loop.  Thus a single
 * pointer is good enough even for the case of both row and column symmetry.
 * Returns NULL if there is no symmetry.
 */
Cell * symCell(const Cell * cell)
{
    int row;
    int col;
    int nRow;
    int nCol;

    if (!rowSym && !colSym && !pointSym && !quadSym && !fwdSym && !bwdSym)
        return NULL;

    row = cell->row;
    col = cell->col;
    nRow = rowMax + 1 - row;
    nCol = colMax + 1 - col;

    /*
     * If this is point symmetry, then this is easy.
     */
    if (pointSym)
        return findCell(nRow, nCol, cell->gen);

    if (quadSym)
        return findCell(col, colMax + 1 - row, cell->gen);

    /*
     * If this is forward diagonal symmetry, then this is easy.
     */
    if (fwdSym)
        return findCell(nCol, nRow, cell->gen);

    /*
     * If this is backward diagonal symmetry, then this is easy.
     */
    if (bwdSym)
        return findCell(col, row, cell->gen);

    /*
     * If there is symmetry on only one axis, then this is easy.
     */
    if (!colSym)
    {
        if (col < rowSym)
            return NULL;

        return findCell(nRow, col, cell->gen);
    }

    if (!rowSym)
    {
        if (row < colSym)
            return NULL;

        return findCell(row, nCol, cell->gen);
    }

    /*
     * Here is there is both row and column symmetry.
     * First see if the cell is in the middle row or middle column,
     * and if so, then this is easy.
     */
    if ((nRow == row) || (nCol == col))
        return findCell(nRow, nCol, cell->gen);

    /*
     * The cell is really in one of the four quadrants, and therefore
     * has four cells making up the symmetry.  Link this cell to the
     * symmetrical cell in the next quadrant clockwise.
     */
    if ((row < nRow) == (col < nCol))
        return findCell(row, nCol, cell->gen);
    else
        return findCell(nRow, col, cell->gen);
}
