#include "lifesrc.h"

/*
 * Link a cell to its eight neighbors in the same generation, and also
 * link those neighbors back to this cell.
 */
void
linkCell(Cell * cell)
{
    int row;
    int col;
    int gen;
    Cell * pairCell;

    row = cell->row;
    col = cell->col;
    gen = cell->gen;

    pairCell = findCell(row - 1, col - 1, gen);
    cell->cul = pairCell;
    pairCell->cdr = cell;

    pairCell = findCell(row - 1, col, gen);
    cell->cu = pairCell;
    pairCell->cd = cell;

    pairCell = findCell(row - 1, col + 1, gen);
    cell->cur = pairCell;
    pairCell->cdl = cell;

    pairCell = findCell(row, col - 1, gen);
    cell->cl = pairCell;
    pairCell->cr = cell;

    pairCell = findCell(row, col + 1, gen);
    cell->cr = pairCell;
    pairCell->cl = cell;

    pairCell = findCell(row + 1, col - 1, gen);
    cell->cdl = pairCell;
    pairCell->cur = cell;

    pairCell = findCell(row + 1, col, gen);
    cell->cd = pairCell;
    pairCell->cu = cell;

    pairCell = findCell(row + 1, col + 1, gen);
    cell->cdr = pairCell;
    pairCell->cul = cell;
}
