#define EXTERN extern

#include "lifesrc.h"
#include "allocatecell.h"
#include "linkcell.h"
#include "symcell.h"
#include "mapcell.h"
#include "loopcells.h"
#include "sortorder.h"
#include "enums.h"
#include "implication.h"
#include "macros.h"


/*
 * Order the cells to be searched by building the search table list.
 * This list is built backwards from the intended search order.
 * The default is to do searches from the middle row outwards, and
 * from the left to the right columns.  The order can be changed though.
 */
void
initsearchorder(void)
{
    int row;
    int col;
    int gen;
    int count;
    Cell * cell;
    Cell * table[MAX_CELLS];
    globals_struct g;
    g.colMax = colmax;
    g.rowMax = rowmax;
    g.parent = parent;
    g.orderGens = ordergens;
    g.orderInvert = 0;
    g.orderMiddle = ordermiddle;
    g.orderWide = orderwide;
    if (diagsort)
        g.sortOrder = DIAG;
    else if (knightsort)
        g.sortOrder = KNIGHT;
    else
        g.sortOrder = DEFAULT;
    /*
     * Make a table of cells that will be searched.
     * Ignore cells that are not relevant to the search due to symmetry.
     */
    count = 0;

    for (gen = 0; gen < genmax; gen++)
    {
        for (col = 1; col <= colmax; col++)
        {
            for (row = 1; row <= rowmax; row++)
            {
                cell = findcell(row, col, gen);
                // cells must be already loaded!!!
                if ((cell->active) && (cell->state == UNK) && (cell->choose))
                {
                    table[count++] = findcell(row, col, gen);
                }
            }
        }
    }

    /*
     * Now sort the table based on our desired search order.
     */
    qsort_r((char *) table, count, sizeof(Cell *), &orderSortFunc, &g);

    /*
     * If we've been here before, wipe the old searchlist
     */
    if (searchlist) free(searchlist);

    /*
     * Finally build the search list from the table elements in the
     * final order.
     */
    searchlist = (Cell **) malloc(sizeof(Cell *) * (count + 1));

    for (int i = 0; i < count; i++)
    {
        searchlist[i] = table[i];
        searchlist[i]->index = i;
    }
    searchlist[count] = NULL;
    searchidx = 0;
}


/*
 * Initialize the table of cells.
 * Each cell in the active area is set to unknown state.
 * Boundary cells are set to zero state.
 */
void
initcells(void)
{
    int row;
    int col;
    int gen;
    int i;
    Bool edge;
    Cell * cell;
    Cell * cell2;

    inited = FALSE;
    searchlist = NULL;

    /*
     * Check whether valid parameters have been set.
     */
    if ((rowmax <= 0) || (rowmax > ROW_MAX))
        FATAL("Row number out of range");

    if ((colmax <= 0) || (colmax > COL_MAX))
        FATAL("Column number out of range");

    if ((genmax <= 0) || (genmax > GEN_MAX))
        FATAL("Generation number out of range");

    if ((rowtrans < -TRANS_MAX) || (rowtrans > TRANS_MAX))
        FATAL("Row translation number out of range");

    if ((coltrans < -TRANS_MAX) || (coltrans > TRANS_MAX))
        FATAL("Column translation number out of range");

    for (i = 0; i < MAX_CELLS; i++)
        cellTable[i] = allocateCell();

    /*
     * Link the cells together.
     */
    for (col = 0; col <= colmax+1; col++)
    {
        for (row = 0; row <= rowmax+1; row++)
        {
            for (gen = 0; gen < genmax; gen++)
            {
                edge = ((row == 0) || (col == 0) ||
                    (row > rowmax) || (col > colmax));

                cell = findcell(row, col, gen);
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
                cell->past = findcell(row, col,
                    (gen+genmax-1) % genmax);

                cell->future = findcell(row, col,
                    (gen+1) % genmax);

                /*
                 * If this is not an edge cell, and
                 * there is some symmetry, then put
                 * this cell in the same loop as the
                 * next symmetrical cell.
                 */
                if((pointsym || colsym || rowsym || fwdsym || bwdsym) && !edge)
                {
                    loopcells(cell, symCell(cell));
                }
            }
        }
    }

    /*
     * Now for the symmetry
     * Let's look for all loops
     * and select one cell from each loop as active
     */

    for (col = 1; col <= colmax; col++)
    {
        for (row = 1; row <= rowmax; row++)
        {
            for (gen = 0; gen < genmax; gen++)
            {
                cell = findcell(row, col, gen);

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


    /*
     * If there is a non-standard mapping between the last generation
     * and the first generation, then change the future and past pointers
     * to implement it.  This is for translations and flips.
     */
    if (rowtrans || coltrans || fliprows || flipcols || flipquads)
    {
        for (row = 0; row <= rowmax+1; row++)
        {
            for (col = 0; col <= colmax+1; col++)
            {
                cell = findcell(row, col, 0);
                cell2 = mapCell(cell);
                cell->past = cell2;
                cell2->future = cell;
            }
        }
    }

    initsearchorder();

    newset = settable;
    nextset = settable;

    searchset = searchtable;

    curstatus = OK;
    initimplic(bornrules, liverules, implic);

    inited = TRUE;
}
