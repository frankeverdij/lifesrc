/*
 * Life search program - actual search routines.
 * Author: David I. Bell.
 * Based on the algorithms by Dean Hickerson that were
 * included with the "xlife 2.0" distribution.  Thanks!
 * Changes for arbitrary Life rules by Nathan S. Thompson.
 */

/*
 * Define this as a null value so as to define the global variables
 * defined in lifesrc.h here.
 */
#define	EXTERN

#include "lifesrc.h"
#include "state.h"
#include "flags.h"
#include "transition.h"
#include "implication.h"
#include "nextstate.h"
#include "description.h"
#include "sortorder.h"
#include "setstate.h"

/*
 * Table of state values.
 */
static State states[nStates] = {OFF, ON, UNK};

/*
 * Table of transitions.
 * Given the state of a cell and its neighbors in one generation,
 * this table determines the state of the cell in the next generation.
 * The table is indexed by the descriptor value of a cell.
 */
static	State	transit[1024];


/*
 * Table of implications.
 * Given the state of a cell and its neighbors in one generation,
 * this table determines deductions about the cell and its neighbors
 * in the previous generation.
 * The table is indexed by the descriptor value of a cell.
 */
static	Flags	implic[1024];


/*
 * Other local data.
 */
static	int	newCellCount;		/* cells ready for allocation */
static	int	auxCellCount;		/* cells in auxillary table */
static  int searchIdx;
static  int fullSearchIdx;
static	Cell *	newCells;		/* cells ready for allocation */
static	Cell *	deadCell;		/* boundary cell value */
static	Cell **	searchList;		/* current list of cells to search */
static	Cell *	cellTable[MAX_CELLS];	/* table of usual cells */
static	Cell *	auxTable[AUX_CELLS];	/* table of auxillary cells */
static	RowInfo	dummyRowInfo;		/* dummy info for ignored cells */
static	ColInfo	dummyColInfo;		/* dummy info for ignored cells */


/*
 * Local procedures
 */
static	void	initSearchOrder(void);
static	void	linkCell(Cell *);
static	State	choose(const Cell *);
static	Cell *	symCell(const Cell *);
static	Cell *	mapCell(const Cell *, Bool);
static	Cell *	allocateCell(void);
static	Cell *	getNormalUnknown(void);
/*static	Cell *	getAverageUnknown(void);*/
static	Status	consistify(Cell * const);
static	Status	consistify10(Cell * const);
static	Status	examineNext(void);
static	Bool	checkWidth(const Cell *);
static	int	getDesc(const Cell * const);
/* static	Cell *	(*getUnknown)(void); */


/*
 * Initialize the table of cells.
 * Each cell in the active area is set to unknown state.
 * Boundary cells are set to zero state.
 */
void
initCells(void)
{
	int	row;
	int	col;
	int	gen;
	int	i;
	Bool	edge;
	Cell *	cell;
	Cell *	cell2;

	/*
	 * Check whether valid parameters have been set.
	 */
	if ((rowMax <= 0) || (rowMax > ROW_MAX))
		fatal("Row number out of range");

	if ((colMax <= 0) || (colMax > COL_MAX))
		fatal("Column number out of range");

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
	deadCell = allocateCell();

	for (i = 0; i < MAX_CELLS; i++)
		cellTable[i] = allocateCell();

	/*
	 * Link the cells together.
	 */
	for (col = 0; col <= colMax+1; col++)
	{
		for (row = 0; row <= rowMax+1; row++)
		{
			for (gen = 0; gen < genMax; gen++)
			{
				edge = ((row == 0) || (col == 0) ||
					(row > rowMax) || (col > colMax));

				cell = findCell(row, col, gen);
				cell->gen = gen;
				cell->row = row;
				cell->col = col;
				cell->choose = TRUE;
				cell->rowInfo = &dummyRowInfo;
				cell->colInfo = &dummyColInfo;

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

	/*
	 * If there is a non-standard mapping between the last generation
	 * and the first generation, then change the future and past pointers
	 * to implement it.  This is for translations and flips.
	 */
	if (rowTrans || colTrans || flipRows || flipCols || flipQuads)
	{
		for (row = 0; row <= rowMax+1; row++)
		{
			for (col = 0; col <= colMax+1; col++)
			{
				cell = findCell(row, col, genMax - 1);
				cell2 = mapCell(cell, TRUE);
				cell->future = cell2;
				cell2->past = cell;

				cell = findCell(row, col, 0);
				cell2 = mapCell(cell, FALSE);
				cell->past = cell2;
				cell2->future = cell;
			}
		}
	}

	/*
	 * Initialize the row and column info addresses for generation 0.
	 */
	for (row = 1; row <= rowMax; row++)
	{
		for (col = 1; col <= colMax; col++)
		{
			cell = findCell(row, col, 0);
			cell->rowInfo = &rowInfo[row];
			cell->colInfo = &colInfo[col];
		}
	}

	initSearchOrder();

/*	if (follow)
		getUnknown = getAverageUnknown;
	else
		getUnknown = getNormalUnknown;
*/
	newSet = setTable;
	nextSet = setTable;
	baseSet = setTable;

	curGen = 0;
	curStatus = OK;
	initNextState(bornRules, liveRules);
	initTransit(states, transit);
	initImplic(states, implic);
	/*
	printf("Cell struct is: %zd\n", sizeof(Cell));
	for (int i=0;i<576;i++) {
	    if (i%32 == 0)
	        printf("\n");
	    printf(" %02x", transit[i]);
	}
	printf("\n---\n");
	for (int i=0;i<576;i++) {
	    if (i%32 == 0)
	        printf("\n");
	    printf(" %02x", implic[i]);
	}
	*/
}


/*
 * Order the cells to be searched by building the search table list.
 * This list is built backwards from the intended search order.
 * The default is to do searches from the middle row outwards, and
 * from the left to the right columns.  The order can be changed though.
 */
static void
initSearchOrder(void)
{
	int	row;
	int	col;
	int	gen;
	int	count;
	Cell *	cell;
	Cell *	table[MAX_CELLS];
	globals_struct g;
	
	g.colMax = colMax;
	g.rowMax = rowMax;
	g.parent = parent;
	g.orderGens = orderGens;
	g.orderMiddle = orderMiddle;
	g.orderWide = orderWide;
    g.sortOrder = SORTORDER_TOPDOWN;
	/*
	 * Make a table of cells that will be searched.
	 * Ignore cells that are not relevant to the search due to symmetry.
	 */
	count = 0;

	for (gen = 0; gen < genMax; gen++)
		for (col = 1; col <= colMax; col++)
			for (row = 1; row <= rowMax; row++)
	{
		if (rowSym && (col >= rowSym) && (row * 2 > rowMax + 1))
			continue;

		if (colSym && (row >= colSym) && (col * 2 > colMax + 1))
			continue;

		table[count++] = findCell(row, col, gen);
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
	    searchList[i] = table[i];
	searchList[count] = NULL;
    searchIdx = 0;
	fullSearchIdx = searchIdx;
}



/*
 * Set the state of a cell to the specified state.
 * The state is either ON or OFF.
 * Returns ERROR if the setting is inconsistent.
 * If the cell is newly set, then it is added to the set table.
 */
Status
setCell(Cell * const cell, const State state, const Bool free)
{
	if (cell->state == state)
	{
		DPRINTF("setCell %d %d %d to state %s already set\n",
			cell->row, cell->col, cell->gen,
			(state == ON) ? "on" : "off");

		return OK;
	}

	if (cell->state != UNK)
	{
		DPRINTF("setCell %d %d %d to state %s inconsistent\n",
			cell->row, cell->col, cell->gen,
			(state == ON) ? "on" : "off");

		return ERROR;
	}
/*
	if (cell->gen == 0)
	{
		if (useCol && (colInfo[useCol].onCount == 0)
			&& (colInfo[useCol].setCount == rowMax) && inited)
		{
			return ERROR;
		}

		if (state == ON)
		{
			if (maxCount && (cellCount >= maxCount))
			{
				DPRINTF("setCell %d %d 0 on exceeds maxCount\n",
					cell->row, cell->col);

				return ERROR;
			}

			if (nearCols && (cell->near <= 0) && (cell->col > 1)
				&& inited)
			{
				return ERROR;
			}

			if (colCells && (cell->colInfo->onCount >= colCells)
				&& inited)
			{
				return ERROR;
			}

			if (colWidth && inited && checkWidth(cell))
				return ERROR;

			if (nearCols)
				adjustNear(cell, 1);

			cell->rowInfo->onCount++;
			cell->colInfo->onCount++;
			cell->colInfo->sumPos += cell->row;
			cellCount++;
		}
	}
*/
	DPRINTF("setCell %d %d %d to %s, %s successful\n",
		cell->row, cell->col, cell->gen,
		(free ? "free" : "forced"), ((state == ON) ? "on" : "off"));

	*newSet++ = cell;

	setState(cell, state);
	cell->free = free;
/*	cell->colInfo->setCount++;

	if ((cell->gen == 0) && (cell->colInfo->setCount == rowMax))
		fullColumns++;
*/
	return OK;
}


/*
 * Calculate the current descriptor for a cell.
 */
static int
getDesc(const Cell * const cell)
{
	return sumToDesc(cell->state, cell->sumNear);
}


/*
 * Consistify a cell.
 * This means examine this cell in the previous generation, and
 * make sure that the previous generation can validly produce the
 * current cell.  Returns ERROR if the cell is inconsistent.
 */
static Status
consistify(Cell * const cell)
{
	Cell *	prevCell;
	int	desc;
	State	state, cellState;
	Flags	flags;

	/*
	 * If we are searching for parents and this is generation 0, then
	 * the cell is consistent with respect to the previous generation.
	 */
/*	if (parent && (cell->gen == 0))
		return OK;
*/
	/*
	 * First check the transit table entry for the previous
	 * generation.  Make sure that this cell matches the ON or
	 * OFF state demanded by the transit table.  If the current
	 * cell is unknown but the transit table knows the answer,
	 * then set the now known state of the cell.
	 */
	prevCell = cell->past;
	desc = getDesc(prevCell);
	state = transit[desc];

	if (state != UNK)
	    if (state != cell->state)
    		if (setCell(cell, state, FALSE) == ERROR)
	    		return ERROR;
	cellState = cell->state;
	if (cellState == UNK)
		return OK;

	/*
	 * Now look up the previous generation in the implic table.
	 * If this cell implies anything about the cell or its neighbors
	 * in the previous generation, then handle that.
	 */
	flags = implic[desc];

	if (flags == 0)
		return OK;

	DPRINTF("Implication flags %x\n", flags);

	if (cellState == OFF)
	{
	    if (flags & N0IC0)
	        if (setCell(prevCell, OFF, FALSE) != OK)
		    return ERROR;
	    if (flags & N0IC1)
	        if (setCell(prevCell, ON, FALSE) != OK)
		    return ERROR;
	    state = UNK;
	    if (flags & N0ICUN0)
	        state = OFF;
	    if (flags & N0ICUN1)
	        state = ON;
	}
	else  /* cellState == ON */
	{
	    if (flags & N1IC0)
	        if (setCell(prevCell, OFF, FALSE) != OK)
		        return ERROR;
	    if (flags & N1IC1)
	        if (setCell(prevCell, ON, FALSE) != OK)
		        return ERROR;
	    state = UNK;
	    if (flags & N1ICUN0)
	        state = OFF;
	    if (flags & N1ICUN1)
	        state = ON;
	}

	if (state == UNK)
	{
		DPRINTF("Implications successful\n");

		return OK;
	}

	/*
	 * For each unknown neighbor, set its state as indicated.
	 * Return an error if any neighbor is inconsistent.
	 */
	DPRINTF("Forcing unknown neighbors of cell %d %d %d %s\n",
		prevCell->row, prevCell->col, prevCell->gen,
		((state == ON) ? "on" : "off"));

	if ((prevCell->cul->state == UNK) &&
		(setCell(prevCell->cul, state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((prevCell->cu->state == UNK) &&
		(setCell(prevCell->cu, state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((prevCell->cur->state == UNK) &&
		(setCell(prevCell->cur, state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((prevCell->cl->state == UNK) &&
		(setCell(prevCell->cl, state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((prevCell->cr->state == UNK) &&
		(setCell(prevCell->cr, state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((prevCell->cdl->state == UNK) &&
		(setCell(prevCell->cdl, state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((prevCell->cd->state == UNK) &&
		(setCell(prevCell->cd, state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((prevCell->cdr->state == UNK) &&
		(setCell(prevCell->cdr, state, FALSE) != OK))
	{
		return ERROR;
	}

	DPRINTF("Implications successful\n");

	return OK;
}


/*
 * See if a cell and its neighbors are consistent with the cell and its
 * neighbors in the next generation.
 */
static Status
consistify10(Cell * const cell)
{
    Cell * future = cell->future;
    
	if (consistify(cell) != OK)
		return ERROR;

	if (consistify(future) != OK)
		return ERROR;

	if (consistify(future->cul) != OK)
		return ERROR;

	if (consistify(future->cu) != OK)
		return ERROR;

	if (consistify(future->cur) != OK)
		return ERROR;

	if (consistify(future->cl) != OK)
		return ERROR;

	if (consistify(future->cr) != OK)
		return ERROR;

	if (consistify(future->cdl) != OK)
		return ERROR;

	if (consistify(future->cd) != OK)
		return ERROR;

	if (consistify(future->cdr) != OK)
		return ERROR;

	return OK;
}


/*
 * Examine the next choice of cell settings.
 */
static Status
examineNext(void)
{
	Cell *	cell;

	/*
	 * If there are no more cells to examine, then what we have
	 * is consistent.
	 */
	if (nextSet == newSet)
		return CONSISTENT;

	/*
	 * Get the next cell to examine, and check it out for symmetry
	 * and for consistency with its previous and next generations.
	 */
	cell = *nextSet++;

	DPRINTF("Examining saved cell %d %d %d (%s) for consistency\n",
		cell->row, cell->col, cell->gen,
		(cell->free ? "free" : "forced"));

	if (cell->loop && (setCell(cell->loop, cell->state, FALSE) != OK))
	{
		return ERROR;
	}

	return consistify10(cell);
}


/*
 * Set a cell to the specified value and determine all consequences we
 * can from the choice.  Consequences are a contradiction or a consistency.
 */
Status
proceed(Cell * cell, State state, Bool free)
{
	int	status;

	if (setCell(cell, state, free) != OK)
		return ERROR;

	for (;;)
	{
		status = examineNext();

		if (status == ERROR)
			return ERROR;

		if (status == CONSISTENT)
			return OK;
	}
}


/*
 * Back up the list of set cells to undo choices.
 * Returns the cell which is to be tried for the other possibility.
 * Returns NULL_CELL on an "object cannot exist" error.
 */
Cell *
backup(void)
{
	Cell *	cell;

	searchIdx = fullSearchIdx;

	while (newSet != baseSet)
	{
		cell = *--newSet;

		DPRINTF("backing up cell %d %d %d, was %s, %s\n",
			cell->row, cell->col, cell->gen,
			((cell->state == ON) ? "on" : "off"),
			(cell->free ? "free": "forced"));

/*		if ((cell->state == ON) && (cell->gen == 0))
		{
			cell->rowInfo->onCount--;
			cell->colInfo->onCount--;
			cell->colInfo->sumPos -= cell->row;
			cellCount--;
			adjustNear(cell, -1);
		}

		if ((cell->gen == 0) && (cell->colInfo->setCount == rowMax))
			fullColumns--;

		cell->colInfo->setCount--;
*/
		if (!cell->free)
		{
			setState(cell, UNK);
			cell->free = TRUE;

			continue;
		}

		nextSet = newSet;

		return cell;
	}

	nextSet = baseSet;
	return NULL_CELL;
}


/*
 * Do checking based on setting the specified cell.
 * Returns ERROR if an inconsistency was found.
 */
Status
go(Cell * cell, State state, Bool free)
{
	Status	status;

	quitOk = FALSE;

	for (;;)
	{
		status = proceed(cell, state, free);

		if (status == OK)
			return OK;

		cell = backup();

		if (cell == NULL_CELL)
			return ERROR;

		free = FALSE;
		state = 1 - cell->state;
		setState(cell, UNK);
	}
}


/*
 * Find another unknown cell in a normal search.
 * Returns NULL_CELL if there are no more unknown cells.
 */
static Cell *
getNormalUnknown(void)
{
	Cell *	cell;

	for (int i = searchIdx; cell = searchList[i]; i++)
	{
		if (!cell->choose)
			continue;

		if (cell->state == UNK)
		{
			searchIdx = i;

			return cell;
		}
	}

	return NULL_CELL;
}


/*
 * Find another unknown cell when averaging is done.
 * Returns NULL_CELL if there are no more unknown cells.
 */
/*static Cell *
getAverageUnknown(void)
{
	Cell *	cell;
	Cell *	bestCell;
	int	bestDist;
	int	curDist;
	int	wantRow;
	int	curCol;
	int	testCol;

	bestCell = NULL_CELL;
	bestDist = -1;

	cell = searchList;

	while (cell)
	{
		searchList = cell;
		curCol = cell->col;

		testCol = curCol - 1;

		while ((testCol > 0) && (colInfo[testCol].onCount <= 0))
			testCol--;

		if (testCol > 0)
		{
			wantRow = colInfo[testCol].sumPos /
				colInfo[testCol].onCount;
		}
		else
			wantRow = (rowMax + 1) / 2;

		for (; cell && (cell->col == curCol); cell = cell->search)
		{
			if (!cell->choose)
				continue;

			if (cell->state == UNK)
			{
				curDist = cell->row - wantRow;

				if (curDist < 0)
					curDist = -curDist;

				if (curDist > bestDist)
				{
					bestCell = cell;
					bestDist = curDist;
				}
			}
		}

		if (bestCell)
			return bestCell;
	}

	return NULL_CELL;
}
*/

/*
 * Choose a state for an unknown cell, either OFF or ON.
 * Normally, we try to choose OFF cells first to terminate an object.
 * But for follow generations mode, we try to choose the same setting
 * as a nearby generation.
 */
static State
choose(const Cell * cell)
{
	/*
	 * If we are following cells in other generations,
	 * then try to do that.
	 */
	if (followGens)
	{
		if ((cell->past->state == ON) ||
			(cell->future->state == ON))
		{
			return ON;
		}

		if ((cell->past->state == OFF) ||
			(cell->future->state == OFF))
		{
			return OFF;
		}
	}

	return ON;
}


/*
 * The top level search routine.
 * Returns if an object is found, or is impossible.
 */
Status
search(void)
{
	Cell *	cell;
	Bool	free;
	Bool	needWrite;
	State	state;

	cell = getNormalUnknown();

	if (cell == NULL_CELL)
	{
		cell = backup();

		if (cell == NULL_CELL)
			return ERROR;

		free = FALSE;
		state = 1 - cell->state;
		setState(cell, UNK);
	}
	else
	{
		state = choose(cell);
		free = TRUE;
	}

	for (;;)
	{
		/*
		 * Set the state of the new cell.
		 */
		if (go(cell, state, free) != OK)
			return NOT_EXIST;

		/*
		 * If it is time to dump our state, then do that.
		 */
		if (dumpFreq && (++dumpcount >= dumpFreq))
		{
			dumpcount = 0;
			dumpState(dumpFile);
		}

		/*
		 * If we have enough columns found, then remember to
		 * write it to the output file.  Also keep the last
		 * columns count values up to date.
		 */
		needWrite = FALSE;

		if (outputCols &&
			(fullColumns >= outputLastCols + outputCols))
		{
			outputLastCols = fullColumns;
			needWrite = TRUE;
		}

		if (outputLastCols > fullColumns)
			outputLastCols = fullColumns;

		/*
		 * If it is time to view the progress,then show it.
		 */
		if (needWrite || (viewFreq && (++viewCount >= viewFreq)))
		{
			viewCount = 0;
			printGen(curGen);
		}

		/*
		 * Write the progress to the output file if needed.
		 * This is done after viewing it so that the write
		 * message will stay visible for a while.
		 */
		if (needWrite)
			writeGen(outputFile, TRUE);

		/*
		 * Check for commands.
		 */
		if (ttyCheck())
			getCommands();

		/*
		 * Get the next unknown cell and choose its state.
		 */
		cell = getNormalUnknown();

		if (cell == NULL_CELL)
			return FOUND;

		state = choose(cell);
		free = TRUE;
	}
}


/*
 * Increment or decrement the near count in all the cells affected by
 * this cell.  This is done for all cells in the next columns which are
 * within the distance specified the nearCols value.  In this way, a
 * quick test can be made to see if a cell is within range of another one.
 */
/*void
adjustNear(Cell * cell, int inc)
{
	Cell *	curCell;
	int	count;
	int	colCount;

	for (colCount = nearCols; colCount > 0; colCount--)
	{
		cell = cell->cr;
		curCell = cell;

		for (count = nearCols; count-- >= 0; curCell = curCell->cu)
			curCell->near += inc;

		curCell = cell->cd;

		for (count = nearCols; count-- > 0; curCell = curCell->cd)
			curCell->near += inc;
	}
}
*/

/*
 * Check to see if setting the specified cell ON would make the width of
 * the column exceed the allowed value.  For symmetric objects, the width
 * is only measured from the center to an edge.  Returns TRUE if the cell
 * would exceed the value.
 */
static Bool
checkWidth(const Cell * cell)
{
	int		left;
	int		width;
	int		minRow;
	int		maxRow;
	int		srcMinRow;
	int		srcMaxRow;
	const Cell *	ucp;
	const Cell *	dcp;
	Bool		full;

	if (!colWidth || !inited || cell->gen)
		return FALSE;

	left = cell->colInfo->onCount;

	if (left <= 0)
		return FALSE;

	ucp = cell;
	dcp = cell;
	width = colWidth;
	minRow = cell->row;
	maxRow = cell->row;
	srcMinRow = 1;
	srcMaxRow = rowMax;
	full = TRUE;

	if ((rowSym && (cell->col >= rowSym)) ||
		(flipRows && (cell->col >= flipRows)))
	{
		full = FALSE;
		srcMaxRow = (rowMax + 1) / 2;

		if (cell->row > srcMaxRow)
		{
			srcMinRow = (rowMax / 2) + 1;
			srcMaxRow = rowMax;
		}
	}

	while (left > 0)
	{
		if (full && (--width <= 0))
			return TRUE;

		ucp = ucp->cu;
		dcp = dcp->cd;

		if (ucp->state == ON)
		{
			if (ucp->row >= srcMinRow)
				minRow = ucp->row;

			left--;
		}

		if (dcp->state == ON)
		{
			if (dcp->row <= srcMaxRow)
				maxRow = dcp->row;

			left--;
		}
	}

	if (maxRow - minRow >= colWidth)
		return TRUE;

	return FALSE;
}


/*
 * Check to see if any other generation is identical to generation 0.
 * This is used to detect and weed out all objects with subPeriods.
 * (For example, stable objects or period 2 objects when using -g4.)
 * Returns TRUE if there is an identical generation.
 */
Bool
subPeriods(void)
{
	int		row;
	int		col;
	int		gen;
	const Cell *	cellG0;
	const Cell *	cellGn;

	for (gen = 1; gen < genMax; gen++)
	{
		if (genMax % gen)
			continue;

		for (row = 1; row <= rowMax; row++)
		{
			for (col = 1; col <= colMax; col++)
			{
				cellG0 = findCell(row, col, 0);
				cellGn = findCell(row, col, gen);

				if (cellG0->state != cellGn->state)
					goto nextGen;
			}
		}

		return TRUE;
nextGen:;
	}

	return FALSE;
}


/*
 * Return the mapping of a cell from the last generation back to the first
 * generation, or vice versa.  This implements all flipping and translating
 * of cells between these two generations.  This routine should only be
 * called for cells belonging to those two generations.
 */
static Cell *
mapCell(const Cell * cell, Bool forward)
{
	int	row;
	int	col;
	int	tmp;

	row = cell->row;
	col = cell->col;

	if (flipRows && (col >= flipRows))
		row = rowMax + 1 - row;

	if (flipCols && (row >= flipCols))
		col = colMax + 1 - col;

	if (flipQuads)
	{				/* NEED TO GO BACKWARDS */
		tmp = col;
		col = row;
		row = colMax + 1 - tmp;
	}

	if (forward)
	{
		row += rowTrans;
		col += colTrans;
	}
	else
	{
		row -= rowTrans;
		col -= colTrans;
	}

	if (forward)
		return findCell(row, col, 0);
	else
		return findCell(row, col, genMax - 1);
}


/*
 * Make the two specified cells belong to the same loop.
 * If the two cells already belong to loops, the loops are joined.
 * This will force the state of these two cells to follow each other.
 * Symmetry uses this feature, and so does setting stable cells.
 * If any cells in the loop are frozen, then they all are.
 */
void
loopCells(Cell * cell1, Cell * cell2)
{
	Cell *	cell;
	Bool	frozen;

	/*
	 * Check simple cases of equality, or of either cell
	 * being the deadCell.
	 */
	if ((cell1 == deadCell) || (cell2 == deadCell))
		fatal("Attemping to use deadCell in a loop");

	if (cell1 == cell2)
		return;

	/*
	 * Make the cells belong to their own loop if required.
	 * This will simplify the code.
	 */
	if (cell1->loop == NULL)
		cell1->loop = cell1;

	if (cell2->loop == NULL)
		cell2->loop = cell2;

	/*
	 * See if the second cell is already part of the first cell's loop.
	 * If so, they they are already joined.  We don't need to
	 * check the other direction.
	 */
	for (cell = cell1->loop; cell != cell1; cell = cell->loop)
	{
		if (cell == cell2)
			return;
	}

	/*
	 * The two cells belong to separate loops.
	 * Break each of those loops and make one big loop from them.
	 */
	cell = cell1->loop;
	cell1->loop = cell2->loop;
	cell2->loop = cell;

	/*
	 * See if any of the cells in the loop are frozen.
	 * If so, then mark all of the cells in the loop frozen
	 * since they effectively are anyway.  This lets the
	 * user see that fact.
	 */
	frozen = cell1->frozen;

	for (cell = cell1->loop; cell != cell1; cell = cell->loop)
	{
		if (cell->frozen)
			frozen = TRUE;
	}

	if (frozen)
	{
		cell1->frozen = TRUE;

		for (cell = cell1->loop; cell != cell1; cell = cell->loop)
			cell->frozen = TRUE;
	}
}


/*
 * Return a cell which is symmetric to the given cell.
 * It is not necessary to know all symmetric cells to a single cell,
 * as long as all symmetric cells are chained in a loop.  Thus a single
 * pointer is good enough even for the case of both row and column symmetry.
 * Returns NULL_CELL if there is no symmetry.
 */
static Cell *
symCell(const Cell * cell)
{
	int	row;
	int	col;
	int	nRow;
	int	nCol;

	if (!rowSym && !colSym && !pointSym && !fwdSym && !bwdSym)
		return NULL_CELL;

	row = cell->row;
	col = cell->col;
	nRow = rowMax + 1 - row;
	nCol = colMax + 1 - col;

	/*
	 * If this is point symmetry, then this is easy.
	 */
	if (pointSym)
		return findCell(nRow, nCol, cell->gen);

	/*
	 * If there is symmetry on only one axis, then this is easy.
	 */
	if (!colSym)
	{
		if (col < rowSym)
			return NULL_CELL;

		return findCell(nRow, col, cell->gen);
	}

	if (!rowSym)
	{
		if (row < colSym)
			return NULL_CELL;

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


/*
 * Link a cell to its eight neighbors in the same generation, and also
 * link those neighbors back to this cell.
 */
static void
linkCell(Cell * cell)
{
	int	row;
	int	col;
	int	gen;
	Cell *	pairCell;

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


/*
 * Find a cell given its coordinates.
 * Most coordinates range from 0 to colMax+1, 0 to rowMax+1, and 0 to genMax-1.
 * Cells within this range are quickly found by indexing into cellTable.
 * Cells outside of this range are handled by searching an auxillary table,
 * and are dynamically created as necessary.
 */
Cell *
findCell(int row, int col, int gen)
{
	Cell *	cell;
	int	i;

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
	for (i = 0; i < auxCellCount; i++)
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

	cell = allocateCell();
	cell->row = row;
	cell->col = col;
	cell->gen = gen;
/*	cell->rowInfo = &dummyRowInfo;
	cell->colInfo = &dummyColInfo;
*/
	auxTable[auxCellCount++] = cell;

	return cell;
}


/*
 * Allocate a new cell.
 * The cell is initialized as if it was a boundary cell.
 * Warning: The first allocation MUST be of the deadCell.
 */
static Cell *
allocateCell(void)
{
	Cell *	cell;

	/*
	 * Allocate a new chunk of cells if there are none left.
	 */
	if (newCellCount <= 0)
	{
		newCells = (Cell *) malloc(sizeof(Cell) * ALLOC_SIZE);

		if (newCells == NULL)
			fatal("Cannot allocate cell structure");

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

/* END CODE */
