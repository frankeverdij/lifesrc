#include "lifesrc.h"

/*
 * Return a cell which is symmetric to the given cell.
 * It is not necessary to know all symmetric cells to a single cell,
 * as long as all symmetric cells are chained in a loop.  Thus a single
 * pointer is good enough even for the case of both row and column symmetry.
 * Returns NULL if there is no symmetry.
 */
Cell * symCell(Cell * cell)
{
    int row;
    int col;
    int nrow;
    int ncol;

    row = cell->row;
    col = cell->col;
    nrow = rowmax + 1 - row;
    ncol = colmax + 1 - col;

    /*
     * If this is point symmetry, then this is easy.
     */
    if (pointsym)      // origin*4 symmetry
        return findcell(nrow, ncol, cell->gen);

    if(rowsym && colsym && fwdsym && bwdsym) { // octagonal, this is gonna be tough
        // if on an axis
        if(nrow==row || ncol==col)
            return findcell(ncol, row, cell->gen);
        // if on a diagonal
        if(row==col || row==ncol)
            return findcell(ncol, row, cell->gen);
        if((col>nrow && row<nrow)||(col<nrow && row>nrow)) // octants 1,5
            return findcell(nrow, col, cell->gen);  // flip rows
        if((col<nrow && col>ncol)||(col>nrow && col<ncol)) // 2,6
            return findcell(ncol, nrow, cell->gen);  // fwd diag
        if((col>row && col<ncol)||(col<row && col>ncol))   // 3,7
            return findcell(row, ncol, cell->gen);  // flip cols
        if((col<row && row<nrow)||(col>row && row>nrow))   // 4,8
            return findcell(col, row, cell->gen);   // bwd diag
    }

    if(colsym && rowsym) {
        /*
         * Here is there is both row and column symmetry.
         * First see if the cell is in the middle row or middle column,
         * and if so, then this is easy.
         */
        if ((nrow == row) || (ncol == col))
            return findcell(nrow, ncol, cell->gen);

        /*
         * The cell is really in one of the four quadrants, and therefore
         * has four cells making up the symmetry.  Link this cell to the
         * symmetrical cell in the next quadrant clockwise.
         */
        if ((row < nrow) == (col < ncol))
            return findcell(row, ncol, cell->gen);  // quadrant 2 or 4
        else
            return findcell(nrow, col, cell->gen);  // quadrant 1 or 3
    }

    if(colsym)  // col sym
        return findcell(row,ncol,cell->gen);

    if(rowsym)  // row sym
        return findcell(nrow,col,cell->gen);

    if(fwdsym && bwdsym) {  // diagonal 4-fold
        // if on a diagonal...
        if(row==col || row==ncol)
            return findcell(nrow,ncol,cell->gen);

        // Not on a diagonal.
        if((col<row)==(col<nrow))
            return findcell(col,row,cell->gen);
        else
            return findcell(ncol,nrow,cell->gen);
    }

    if(fwdsym)       // fwd diag
        return findcell(ncol,nrow,cell->gen);

    if(bwdsym)       // bwd diag
        return findcell(col,row,cell->gen);

    return NULL;   // crash if we get here :)
}
