#define EXTERN extern

#include "lifesrc.h"
#include "allocatecell.h"
#include "linkcell.h"
#include "symcell.h"
#include "mapcell.h"
#include "loopcells.h"
#include "sortorder.h"
#include "enums.h"
#include "flags.h"
#include "transition.h"
#include "implication.h"
#include "implicationKS.h"
#include "nextstate.h"
#include "findcell.h"
#include "setstate.h"
#include "isedge.h"


/*
 * Order the cells to be searched by building the search table list.
 * This list is built backwards from the intended search order.
 * The default is to do searches from the middle row outwards, and
 * from the left to the right columns.  The order can be changed though.
 */
void
initSearchOrder(void)
{
    int row;
    int col;
    int gen;
    int count;
    Cell * cell;
    Cell * table[MAX_CELLS];

    globals_struct g;
    g.colMax = colMax;
    g.rowMax = rowMax;
    g.parent = parent;
    g.orderGens = orderGens;
    g.orderInvert = orderInvert;
    g.orderMiddle = orderMiddle;
    g.orderWide = orderWide;
    g.sortOrder = sortOrder;
    /*
     * Make a table of cells that will be searched.
     * Ignore cells that are not relevant to the search due to symmetry.
     */
    count = 0;

    for (gen = 0; gen < genMax; gen++)
    {
        for (col = 1; col <= colMax; col++)
        {
            for (row = 1; row <= rowMax; row++)
            {
                cell = findCell(row, col,gen);
                if (smartOn)
                {
                    if (!((cell->active) && (cell->state == UNK) && (cell->choose)))
                        continue;
                } else {
                    if (rowSym && (col >= rowSym) && (row * 2 > rowMax + 1))
                        continue;

                    if (colSym && (row >= colSym) && (col * 2 > colMax + 1))
                        continue;

                    if (fwdSym && (colMax + 1 > row + col))
                        continue;

                    if (bwdSym && (col > row ))
                        continue;
                }
                table[count++] = cell;
            }
        }
    }

    /*
     * Now sort the table based on our desired search order.
     */
    qsort_r((char *) table, count, sizeof(Cell *), &orderSortFunc, &g);

    /*
     * Finally build the search list from the table elements in the
     * final order.
     */
    searchList = (Cell **) malloc(sizeof(Cell *) * (count + 1));

    for (int i = 0; i < count; i++)
    {
        searchList[i] = table[i];
        searchList[i]->index = i;
    }
    searchList[count] = NULL;
    searchIdx = 0;
}


/*
 * Initialize the table of cells.
 * Each cell in the active area is set to unknown state.
 * Boundary cells are set to zero state.
 */
void
initCells(void)
{
    int row;
    int col;
    int gen;
    int i;
    Bool edge;
    Cell * cell;
    Cell * cell2;

    /*
     * Check whether valid parameters have been set.
     */
    if ((rowMax <= 0) || (rowMax > ROW_MAX))
        fatal("Row number out of range");

    if ((colMax <= 0) || (colMax > COL_MAX))
        fatal("Column number out of range");

    if ((colMax <= edgeDiagOffset) || (colMax + edgeDiagOffset <= 0))
        fatal("Edge diagonal offset too big");

    if ((edgeDiagOffset != 0) && (rowMax != colMax))
        fatal("Edge diagonal offset set but search area is not square");

    if ((genMax <= 0) || (genMax > GEN_MAX))
        fatal("Generation number out of range");

    if ((rowTrans < -TRANS_MAX) || (rowTrans > TRANS_MAX))
        fatal("Row translation number out of range");

    if ((colTrans < -TRANS_MAX) || (colTrans > TRANS_MAX))
        fatal("Column translation number out of range");

    /*
     * The first allocation of a cell MUST be deadCell.
     * Then allocate the cells in the cell table.
     */
    allocateCell();

    for (i = 0; i < MAX_CELLS; i++)
        cellTable[i] = allocateCell();

    /*
     * Link the cells together.
     */
    for (col = 0; col <= colMax+1; col++)
    {
        for (row = 0; row <= rowMax+1; row++)
        {
            edge = isEdge(row, col);
            for (gen = 0; gen < genMax; gen++)
            {
                cell = findCell(row, col, gen);
                cell->gen = gen;
                cell->row = row;
                cell->col = col;

                cell->active = TRUE;
                cell->choose = TRUE;

                /*
                 * If this is not an edge cell, then its state
                 * is unknown and it needs linking to its
                 * neighbors.
                 */
                if (!edge)
                {
                    linkCell(cell);
                    setState(cell, UNK);
                    cell->free = TRUE;
                }

                /*
                 * Map time forwards and backwards,
                 * wrapping around at the ends.
                 */
                cell->past = findCell(row, col,
                    (gen+genMax-1) % genMax);

                cell->future = findCell(row, col,
                    (gen+1) % genMax);

                /*
                 * If this is not an edge cell, and
                 * there is some symmetry, then put
                 * this cell in the same loop as the
                 * next symmetrical cell.
                 */
                if ((rowSym || colSym || pointSym ||
                    fwdSym || bwdSym) && !edge)
                {
                    loopCells(cell, symCell(cell));
                }
            }
        }
    }

    if (smartOn)
    {
        /*
         * Now for the symmetry
         * Let's look for all loops
         * and select one cell from each loop as active
         */

        for (col = 1; col <= colMax; col++)
        {
            for (row = 1; row <= rowMax; row++)
            {
                for (gen = 0; gen < genMax; gen++)
                {
                    cell = findCell(row, col, gen);

                    if (cell->active)
                    {
                        cell2 = cell->loop;
                        while (cell2 != cell)
                        {
                            cell2->active = FALSE;
                            cell2 = cell2->loop;
                        }
                    }
                }
            }
        }
    }


    /*
     * If there is a non-standard mapping between the last generation
     * and the first generation, then change the future and past pointers
     * to implement it.  This is for translations and flips.
     */
    if (rowTrans || colTrans || flipRows || flipCols || flipFwd || flipBwd || flipQuads)
    {
        for (row = 0; row <= rowMax+1; row++)
        {
            for (col = 0; col <= colMax+1; col++)
            {
                if (!smartOn)
                {
                    cell = findCell(row, col, genMax - 1);
                    cell2 = mapCell(cell, TRUE);
                    cell->future = cell2;
                    cell2->past = cell;
                }

                cell = findCell(row, col, 0);
                cell2 = mapCell(cell, FALSE);
                cell->past = cell2;
                cell2->future = cell;
            }
        }
    }

    initSearchOrder();

    newSet = setTable;
    nextSet = setTable;
    baseSet = setTable;
    searchSet = searchTable;

    stepConfl = 0;
    curGen = 0;
    curStatus = OK;
    if (smartOn)
    {
        initImplicKS(bornRules, liveRules, implic);
    } else {
        initNextState(bornRules, liveRules);
        initTransit(transit);
        initImplic(implic);
    }
}
