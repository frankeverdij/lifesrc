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
static	size_t	cellTableSize;		/* total size of cellTable */
static	int	auxCellCount = 0;		/* cells in auxillary table */
static int strIntSize;
static  int searchIdx;
static  int searchCount;
static	int	newCells;		/* cells ready for allocation */
static	int	deadCell;		/* boundary cell value */
static	int*	searchList;		/* current list of cells to search */
static  State * stateList;


/*
 * Local procedures
 */
static	void	initSearchOrder(void);
static	void	linkCell(int);
static	State	choose(const int);
static	int	symCell(const int);
static	int	mapCell(const int, Bool);
static void initCell(const int);
static	int	getNormalUnknown(void);
static	Status	consistify(int const);
static	Status	consistify10(int const);
static	Status	examineNext(void);
static	int	getDesc(const int);
void dumpCellTable();


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
	sCrg * ptr;
	int	i;
	Bool edge;
	int	cell;
	int	cell2;

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

    strIntSize = sizeof(Cell)/sizeof(int);
    /*
     * Allocate the cellTable. Add at least one extra cell for the
     * deadcell, which is located at (rowMax+1, colMax+1, genMax).
     * The rest is for the additional cells which we need to add on
     * the fly and reallocate the cellTable if necessary.
     */
    deadCell =  strIntSize * ((rowMax + 2) * (colMax + 2) * genMax);
    
    cellTableSize = deadCell + strIntSize;
	cellTable = (int *)malloc(cellTableSize * sizeof(int));

	/*
	 * The first allocation of a cell MUST be deadCell.
	 */
    initCell(deadCell);

	for (cell = 0; cell <= deadCell; cell += strIntSize)
	{
		initCell(cell);
	}

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
					(row == rowMax + 1) || (col == colMax + 1));

                cell = findCell(row, col, gen);

				/*
				 * If this is not an edge cell, then its state
				 * is unknown and it needs linking to its
				 * neighbors.
				 */
				if (!edge)
				{
					linkCell(cell);
					setState(cell, UNK, stateList);
					ptr = (sCrg * ) &cellTable[cell + O_GENFLAGS];
					ptr->flags |= FREECELL;
				}

				/*
				 * Map time forwards and backwards,
				 * wrapping around at the ends.
				 */
				cellTable[cell + O_PAST] = findCell(row, col,
					(gen+genMax-1) % genMax);

				cellTable[cell + O_FUTURE] = findCell(row, col,
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
	if (rowTrans || colTrans || flipRows || flipCols || flipFwd || flipBwd || flipQuads)
	{
		for (row = 0; row <= rowMax+1; row++)
		{
			for (col = 0; col <= colMax+1; col++)
			{
				cell = findCell(row, col, genMax - 1);
				cell2 = mapCell(cell, TRUE);
				cellTable[cell + O_FUTURE] = cell2;
				cellTable[cell2 + O_PAST] = cell;

				cell = findCell(row, col, 0);
				cell2 = mapCell(cell, FALSE);
				cellTable[cell + O_PAST] = cell2;
				cellTable[cell2 + O_FUTURE] = cell;
			}
		}
	}

	initSearchOrder();

	newSet = setTable;
	nextSet = setTable;
	baseSet = setTable;

	stepConfl = 0;
	curGen = 0;
	curStatus = OK;
	initNextState(bornRules, liveRules);
	initTransit(states, transit);
	initImplic(states, implic);
	dumpCellTable();
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
	int table[MAX_CELLS];
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
	searchCount = 0;

	for (gen = 0; gen < genMax; gen++)
		for (col = 1; col <= colMax; col++)
			for (row = 1; row <= rowMax; row++)
	{
		if (rowSym && (col >= rowSym) && (row * 2 > rowMax + 1))
			continue;

		if (colSym && (row >= colSym) && (col * 2 > colMax + 1))
			continue;

		if (fwdSym && (colMax + 1 >= row + col))
			continue;

		if (bwdSym && (col >= row ))
			continue;

		table[searchCount++] = findCell(row, col, gen);
	}

	/*
	 * Now sort the table based on our desired search order.
	 */
	qsort_r((char *) table, searchCount, sizeof(int), &orderSortFunc, &g);

	/*
	 * Finally build the search list from the table elements in the
	 * final order.
	 */
	searchList = (int *) malloc(sizeof(int) * (searchCount + 1));
	stateList = (State *) malloc(sizeof(State) * (searchCount + 1));

	for (int i = 0; i < searchCount; i++)
	{
	    searchList[i] = table[i];
	    cellTable[searchList[i] + O_INDEX] = i;
	    stateList[i] = cellTable[searchList[i]];
	    sCrg * ptr = (sCrg *)&cellTable[searchList[i] + O_GENFLAGS];
	    DPRINTF("sort %d %d %d\n", ptr->row, ptr->col, ptr->gen);
	}
	searchList[searchCount] = NULL_CELL;
	stateList[searchCount] = ON + UNK;
    searchIdx = 0;
}



/*
 * Set the state of a cell to the specified state.
 * The state is either ON or OFF.
 * Returns ERROR if the setting is inconsistent.
 * If the cell is newly set, then it is added to the set table.
 */
Status
setCell(const int cell, const State state, const Bool free)
{
    //sCrg crg = cellToColRowGen(cell);
    	sCrg * ptr = (sCrg *) &cellTable[cell + O_GENFLAGS];

	if (cellTable[cell] == state)
	{
		DPRINTF("setCell %d %d %d # %d to state %s already set\n",
			ptr->row, ptr->col, ptr->gen, cell,
			(state == ON) ? "on" : "off");

		return OK;
	}

	if (cellTable[cell] != UNK)
	{
		DPRINTF("setCell %d %d %d # %d to state %s inconsistent\n",
			ptr->row, ptr->col, ptr->gen, cell,
			(state == ON) ? "on" : "off");

		return ERROR;
	}
	DPRINTF("setCell %d %d %d # %d to %s, %s successful\n",
		ptr->row, ptr->col, ptr->gen, cell,
		(free ? "free" : "forced"), ((state == ON) ? "on" : "off"));

	*newSet++ = cell;

	setState(cell, state, stateList);
	if (free)
	    cellTable[cell + O_GENFLAGS] |= FREECELL;
	else
	    cellTable[cell + O_GENFLAGS] &= ~FREECELL;

	return OK;
}


/*
 * Calculate the current descriptor for a cell.
 */
static int
getDesc(const int cell)
{
	return SUMTODESC(cellTable[cell], cellTable[cell + O_SUMNEAR]);
}


/*
 * Consistify a cell.
 * This means examine this cell in the previous generation, and
 * make sure that the previous generation can validly produce the
 * current cell.  Returns ERROR if the cell is inconsistent.
 */
static Status
consistify(const int cell)
{
    int row,col,gen;
    //sCrg crg;
	int	prevCell;
	int	desc;
	State	state, cellState;
	Flags	flags;

	/*
	 * First check the transit table entry for the previous
	 * generation.  Make sure that this cell matches the ON or
	 * OFF state demanded by the transit table.  If the current
	 * cell is unknown but the transit table knows the answer,
	 * then set the now known state of the cell.
	 */
	prevCell = cellTable[cell + O_PAST];
	desc = SUMTODESC(cellTable[prevCell], cellTable[prevCell + O_SUMNEAR]);
	state = transit[desc];
	if (state != UNK)
	    if (state != cellTable[cell])
    		if (setCell(cell, state, FALSE) == ERROR)
	    		return ERROR;
	cellState = cellTable[cell];
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
	    	sCrg * ptr = (sCrg *) &cellTable[prevCell + O_GENFLAGS];

//crg = cellToColRowGen(prevCell);
	DPRINTF("Forcing unknown neighbors of cell %d %d %d # %d %s\n",
		ptr->row, ptr->col, ptr->gen, cell,
		((state == ON) ? "on" : "off"));

	if ((cellTable[cellTable[prevCell + O_CUL]] == UNK) &&
		(setCell(cellTable[prevCell + O_CUL], state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((cellTable[cellTable[prevCell + O_CU]] == UNK) &&
		(setCell(cellTable[prevCell + O_CU], state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((cellTable[cellTable[prevCell + O_CUR]] == UNK) &&
		(setCell(cellTable[prevCell + O_CUR], state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((cellTable[cellTable[prevCell + O_CL]] == UNK) &&
		(setCell(cellTable[prevCell + O_CL], state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((cellTable[cellTable[prevCell + O_CR]] == UNK) &&
		(setCell(cellTable[prevCell + O_CR], state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((cellTable[cellTable[prevCell + O_CDL]] == UNK) &&
		(setCell(cellTable[prevCell + O_CDL], state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((cellTable[cellTable[prevCell + O_CD]] == UNK) &&
		(setCell(cellTable[prevCell + O_CD], state, FALSE) != OK))
	{
		return ERROR;
	}

	if ((cellTable[cellTable[prevCell + O_CDR]] == UNK) &&
		(setCell(cellTable[prevCell + O_CDR], state, FALSE) != OK))
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
consistify10(const int cell)
{
	if (consistify(cell) != OK)
		return ERROR;

	if (consistify(cellTable[cell + O_FUTURE]) != OK)
		return ERROR;

	if (consistify(cellTable[cellTable[cell + O_CUL] + O_FUTURE]) != OK)
		return ERROR;

	if (consistify(cellTable[cellTable[cell + O_CU] + O_FUTURE]) != OK)
		return ERROR;

	if (consistify(cellTable[cellTable[cell + O_CUR] + O_FUTURE]) != OK)
		return ERROR;

	if (consistify(cellTable[cellTable[cell + O_CL] + O_FUTURE]) != OK)
		return ERROR;

	if (consistify(cellTable[cellTable[cell + O_CR] + O_FUTURE]) != OK)
		return ERROR;

	if (consistify(cellTable[cellTable[cell + O_CDL] + O_FUTURE]) != OK)
		return ERROR;

	if (consistify(cellTable[cellTable[cell + O_CD] + O_FUTURE]) != OK)
		return ERROR;

	if (consistify(cellTable[cellTable[cell + O_CDR] + O_FUTURE]) != OK)
		return ERROR;

	return OK;
}


/*
 * Examine the next choice of cell settings.
 */
static Status
examineNext(void)
{
	int	cell;
    //sCrg crg;
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
    //crg = cellToColRowGen(cell);
        	sCrg * ptr = (sCrg *) &cellTable[cell + O_GENFLAGS];
	DPRINTF("Examining saved cell %d %d %d # %d (%s) for consistency\n",
		ptr->row, ptr->col, ptr->gen, cell,
		((cellTable[cell + O_GENFLAGS] & FREECELL) ? "free" : "forced"));
    //printf(" en loop %d\n",cellTable[cell + O_LOOP]);

	if (cellTable[cell + O_LOOP] != NULL_CELL)
	{
	    if (setCell(cellTable[cell + O_LOOP], cellTable[cell], FALSE) != OK)
	    {
		    return ERROR;
	    }
    }
	return consistify10(cell);
}


/*
 * Set a cell to the specified value and determine all consequences we
 * can from the choice.  Consequences are a contradiction or a consistency.
 */
Status
proceed(int cell, State state, Bool free)
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
int
backup(void)
{
	int	cell;
    //sCrg crg;
	while (newSet != baseSet)
	{
		cell = *--newSet;
    	sCrg * ptr = (sCrg *) &cellTable[cell + O_GENFLAGS];

        //crg = cellToColRowGen(cell);
		DPRINTF("backing up cell %d %d %d # %d, was %s, %s\n",
			ptr->row,ptr->col,ptr->gen,cell,
			((cellTable[cell] == ON) ? "on" : "off"),
			((cellTable[cell + O_GENFLAGS] & FREECELL) ? "free": "forced"));

		if (!(cellTable[cell + O_GENFLAGS] & FREECELL))
		{
			setState(cell, UNK, stateList);
			cellTable[cell + O_GENFLAGS] |= FREECELL;

			continue;
		}

		nextSet = newSet;
        searchIdx = cellTable[cell + O_INDEX];

		return cell;
	}

	nextSet = baseSet;
	searchIdx = 0;

	return NULL_CELL;
}


/*
 * Do checking based on setting the specified cell.
 * Returns ERROR if an inconsistency was found.
 */
Status
go(int cell, State state, Bool free)
{
	Status	status;

	quitOk = FALSE;

	for (;;)
	{
	    ++stepConfl;
		status = proceed(cell, state, free);

		if (status == OK)
			return OK;

		cell = backup();

		if (cell == NULL_CELL)
			return ERROR;

		free = FALSE;
		state = 1 - cellTable[cell];
		setState(cell, UNK, stateList);
	}
}


/*
 * Find another unknown cell in a normal search.
 * Returns NULL_CELL if there are no more unknown cells.
 */
static int
getNormalUnknown(void)
{
	int	cell;

	for (int i = searchIdx; i < searchCount; i++)
	{
		if (stateList[i] == UNK)
		{
		    cell = searchList[i];
		    if (cellTable[cell + O_GENFLAGS] & CHOOSECELL)
		    {
		    	searchIdx = i;

			    return cell;
			}
		}
	}

	return NULL_CELL;
}


/*
 * Choose a state for an unknown cell, either OFF or ON.
 * Normally, we try to choose OFF cells first to terminate an object.
 * But for follow generations mode, we try to choose the same setting
 * as a nearby generation.
 */
static State
choose(const int cell)
{
	/*
	 * If we are following cells in other generations,
	 * then try to do that.
	 */
	if (followGens)
	{
		if ((cellTable[cell + O_PAST] == ON) ||
			(cellTable[cell + O_FUTURE] == ON))
		{
			return ON;
		}

		if ((cellTable[cell + O_PAST] == OFF) ||
			(cellTable[cell + O_FUTURE] == OFF))
		{
			return OFF;
		}
	}

	return chooseUnknown;
}


/*
 * The top level search routine.
 * Returns if an object is found, or is impossible.
 */
Status
search(const Bool batch)
{
	int	cell;
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
		state = 1 - cellTable[cell];
		setState(cell, UNK, stateList);
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
		if (!batch)
		{
		    if (ttyCheck())
			    getCommands();
	    }

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
 * Check to see if any other generation is identical to generation 0.
 * This is used to detect and weed out all objects with subPeriods.
 * (For example, stable objects or period 2 objects when using -g4.)
 * Returns TRUE if there is an identical generation.
 */
Bool
subPeriods(void)
{
	int row;
	int col;
	int gen;

	int	cellG0;
	int	cellGn;

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

				if (cellTable[cellG0] != cellTable[cellGn])
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
static int
mapCell(const int cell, Bool forward)
{
	int	row;
	int	col;
	int	gen;
	int	tmp;
    //sCrg crg = cellToColRowGen(cell);
    sCrg * ptr = (sCrg *) &cellTable[cell + O_GENFLAGS];

    gen = ptr->gen;
	row = ptr->row;
	col = ptr->col;

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


/*
 * Make the two specified cells belong to the same loop.
 * If the two cells already belong to loops, the loops are joined.
 * This will force the state of these two cells to follow each other.
 * Symmetry uses this feature, and so does setting stable cells.
 * If any cells in the loop are frozen, then they all are.
 */
void
loopCells(int cell1, int cell2)
{
	int	cell;
	Bool	frozen;

	/*
	 * Check simple cases of equality, or of either cell
	 * being the deadCell.
	 */
	if ((cell1 == deadCell) || (cell2 == deadCell))
		fatal("Attemping to use deadCell in a loop");

	if ((cell1 == cell2) && (cell1 != NULL_CELL) && (cell2 != NULL_CELL))
		return;

	/*
	 * Make the cells belong to their own loop if required.
	 * This will simplify the code.
	 */
	if (cellTable[cell1 + O_LOOP] == NULL_CELL)
		cellTable[cell1 + O_LOOP] = cell1;

	if (cellTable[cell2 + O_LOOP] == NULL_CELL)
		cellTable[cell2 + O_LOOP] = cell2;

	/*
	 * See if the second cell is already part of the first cell's loop.
	 * If so, they they are already joined.  We don't need to
	 * check the other direction.
	 */
	for (cell = cellTable[cell1 + O_LOOP]; cell != cell1; cell = cellTable[cell + O_LOOP])
	{
		if ((cell == cell2) && (cell != NULL_CELL) && (cell2 != NULL_CELL))
			return;
	}

	/*
	 * The two cells belong to separate loops.
	 * Break each of those loops and make one big loop from them.
	 */
	cell = cellTable[cell1 + O_LOOP];
	cellTable[cell1 + O_LOOP] = cellTable[cell2 + O_LOOP];
	cellTable[cell2 + O_LOOP] = cell;

	/*
	 * See if any of the cells in the loop are frozen.
	 * If so, then mark all of the cells in the loop frozen
	 * since they effectively are anyway.  This lets the
	 * user see that fact.
	 */
	frozen = cellTable[cell1 + O_GENFLAGS] & FROZENCELL;

	for (cell = cellTable[cell1 + O_LOOP]; cell != cell1; cell = cellTable[cell + O_LOOP])
	{
		if (cellTable[cell + O_GENFLAGS] & FROZENCELL)
			frozen = TRUE;
	}

	if (frozen)
	{
		cellTable[cell1 + O_GENFLAGS] |= FROZENCELL;

		for (cell = cellTable[cell1 + O_LOOP]; cell != cell1; cell = cellTable[cell + O_LOOP])
			cellTable[cell + O_GENFLAGS] |= FROZENCELL;
	}
}


/*
 * Return a cell which is symmetric to the given cell.
 * It is not necessary to know all symmetric cells to a single cell,
 * as long as all symmetric cells are chained in a loop.  Thus a single
 * pointer is good enough even for the case of both row and column symmetry.
 * Returns NULL_CELL if there is no symmetry.
 */
static int
symCell(const int cell)
{
	int	row;
	int	col;
	int	gen;
    // crg = cellToColRowGen(cell);
	sCrg * ptr = (sCrg *) &cellTable[cell + O_GENFLAGS];

	int	nRow;
	int	nCol;

	if (!rowSym && !colSym && !pointSym && !fwdSym && !bwdSym)
		return NULL_CELL;

    gen = ptr->gen;
	row = ptr->row;
	col = ptr->col;

	nRow = rowMax + 1 - row;
	nCol = colMax + 1 - col;

	/*
	 * If this is point symmetry, then this is easy.
	 */
	if (pointSym)
		return findCell(nRow, nCol, gen);

	/*
	 * If this is forward diagonal symmetry, then this is easy.
	 */
	if (fwdSym)
		return findCell(nCol, nRow, gen);

	/*
	 * If this is backward diagonal symmetry, then this is easy.
	 */
	if (bwdSym)
		return findCell(col, row, gen);

	/*
	 * If there is symmetry on only one axis, then this is easy.
	 */
	if (!colSym)
	{
		if (col < rowSym)
			return NULL_CELL;

		return findCell(nRow, col, gen);
	}

	if (!rowSym)
	{
		if (row < colSym)
			return NULL_CELL;

		return findCell(row, nCol, gen);
	}

	/*
	 * Here is there is both row and column symmetry.
	 * First see if the cell is in the middle row or middle column,
	 * and if so, then this is easy.
	 */
	if ((nRow == row) || (nCol == col))
		return findCell(nRow, nCol, gen);

	/*
	 * The cell is really in one of the four quadrants, and therefore
	 * has four cells making up the symmetry.  Link this cell to the
	 * symmetrical cell in the next quadrant clockwise.
	 */
	if ((row < nRow) == (col < nCol))
		return findCell(row, nCol, gen);
	else
		return findCell(nRow, col, gen);
}


/*
 * Link a cell to its eight neighbors in the same generation, and also
 * link those neighbors back to this cell.
 */
static void
linkCell(int cell)
{
	int	row;
	int	col;
	int	gen;
	sCrg * ptr = (sCrg *) &cellTable[cell + O_GENFLAGS];
    //sCrg crg = cellToColRowGen(cell);
	int pairCell;

    gen = ptr->gen;
	row = ptr->row;
	col = ptr->col;

	pairCell = findCell(row - 1, col - 1, gen);
	cellTable[cell + O_CUL] = pairCell;
	cellTable[pairCell + O_CDR] = cell;

	pairCell = findCell(row - 1, col, gen);
	cellTable[cell + O_CU] = pairCell;
	cellTable[pairCell + O_CD] = cell;

	pairCell = findCell(row - 1, col + 1, gen);
	cellTable[cell + O_CUR] = pairCell;
	cellTable[pairCell + O_CDL] = cell;

	pairCell = findCell(row, col - 1, gen);
	cellTable[cell + O_CL] = pairCell;
	cellTable[pairCell + O_CR] = cell;

	pairCell = findCell(row, col + 1, gen);
	cellTable[cell + O_CR] = pairCell;
	cellTable[pairCell + O_CL] = cell;

	pairCell = findCell(row + 1, col - 1, gen);
	cellTable[cell + O_CDL] = pairCell;
	cellTable[pairCell + O_CUR] = cell;

	pairCell = findCell(row + 1, col, gen);
	cellTable[cell + O_CD] = pairCell;
	cellTable[pairCell + O_CU] = cell;

	pairCell = findCell(row + 1, col + 1, gen);
	cellTable[cell + O_CDR] = pairCell;
	cellTable[pairCell + O_CUL] = cell;
}


/*
 * Find a cell given its coordinates.
 * Most coordinates range from 0 to colMax+1, 0 to rowMax+1, and 0 to genMax-1.
 * Cells within this range are quickly found by indexing into cellTable.
 * Cells outside of this range are handled by searching an auxillary table,
 * and are dynamically created as necessary.
 */
int
findCell(int row, int col, int gen)
{
	int	i, cell;
	sCrg * ptr;
	//sCrg crg;

	/*
	 * If the cell is a normal cell, then we know where it is.
	 */
	if ((row >= 0) && (row <= rowMax + 1) &&
		 (col >= 0) && (col <= colMax + 1) &&
		 (gen >= 0) && (gen < genMax))
	{
		cell = strIntSize * ((col * (rowMax + 2) + row) * genMax + gen);
    	return cell;
	}
	
    /*
	 * Is it deadCell?
	 */
	if ((row == rowMax + 1) && (col == colMax + 1) && (gen == genMax))
	{
	    return deadCell;
	}
//	DPRINTF("Looking for %d %d %d\n", row, col, gen);
	/*
	 * It is an auxilliary cell. Iterate though the list and
	 * try to find it.
	 */
    for (cell = deadCell; cell < deadCell + strIntSize * auxCellCount; cell += strIntSize)
    {
        //crg = cellToColRowGen(cell);
 	    ptr = (sCrg *) &cellTable[cell + O_GENFLAGS];
       if ((ptr->row == row) && (ptr->col == col) &&
			(ptr->gen == gen))
		{
//		    DPRINTF("Found Aux Cell %d %d %d # %d\n", crg.row, crg.col, crg.gen, cell);
			return cell;
		}
    }
    
    /*
     * It is a new Auxilliary cell. We have to re-allocate the celltable.
     */
    auxCellCount++;
    cell = deadCell + strIntSize * auxCellCount;
    if (cell >= cellTableSize)
    {
        DPRINTF("resizing cellTable. Was %zd", cellTableSize);
        cellTableSize += strIntSize * 25;
        cellTable = realloc(cellTable, sizeof(int) * cellTableSize);
        DPRINTF(" to %zd\n", cellTableSize);
    }    
    initCell(cell);
    ptr = (sCrg *)&cellTable[cell + O_GENFLAGS];
    ptr->flags = CHOOSECELL;
    ptr->gen = gen;
    ptr->row = row;
    ptr->col = col;
    return cell;
}

/*sCrg cellToColRowGen (int cell)
{
    sCrg crg;
    sCrg * ptr;

    if (cell == deadCell)
    {
        crg.gen = genMax;
        crg.row = rowMax + 1;
        crg.col = colMax + 1;
        crg.flags = 0;
    }
    else if (cell > deadCell)
    {
        ptr = (sCrg *)&cellTable[cell + O_GENFLAGS];
        crg.flags = ptr->flags;
        crg.gen = ptr->gen;
        crg.row = ptr->row;
        crg.col = ptr->col;
    }
    else
    {
    }

    return crg;
}*/

/*
 * Initialize a new cell.
 * The cell is initialized as if it was a boundary cell.
 * Warning: The first allocation MUST be of the deadCell.
 */
static void initCell(int cell)
{
    static int zeroCounter = 0;
    //sCrg crg;
    sCrg * ptr;
	/*
	 * Fill in the cell as if it was a boundary cell.
	 */
	cellTable[cell] = OFF;
	cellTable[cell + O_GENFLAGS] = CHOOSECELL;
	cellTable[cell + O_COLROW] = -1;
	cellTable[cell + O_SUMNEAR] = 0;
	cellTable[cell + O_INDEX] = -1;
	cellTable[cell + O_PAST] = deadCell;
	cellTable[cell + O_FUTURE] = deadCell;
	cellTable[cell + O_CUL] = deadCell;
	cellTable[cell + O_CU] = deadCell;
	cellTable[cell + O_CUR] = deadCell;
	cellTable[cell + O_CL] = deadCell;
	cellTable[cell + O_CR] = deadCell;
	cellTable[cell + O_CDL] = deadCell;
	cellTable[cell + O_CD] = deadCell;
	cellTable[cell + O_CDR] = deadCell;
	cellTable[cell + O_LOOP] = NULL_CELL;

	if (cell < deadCell)
	{
	    ptr = (sCrg *) &cellTable[cell + O_GENFLAGS];
	    cell /= strIntSize;
        ptr->gen = cell % genMax;
        cell /= genMax;
        ptr->row = cell % (rowMax + 2);
        ptr->col = cell / (rowMax + 2);
        ptr->flags = CHOOSECELL;
	}
	if (cell == deadCell)
	{
	    //crg = cellToColRowGen(cell);
	    ptr = (sCrg *) &cellTable[cell + O_GENFLAGS];
	    ptr->flags = 0;
	    ptr->gen = genMax;
	    ptr->row = rowMax + 1;
	    ptr->col = colMax + 1;
	}

	return;
}

void dumpCellTable()
{
    sCrg * ptr;
    for(int i=0; i < cellTableSize; i+=strIntSize)
    {
        ptr = (sCrg *) &cellTable[i+O_GENFLAGS];
        printf("e%d #%d : r%d c%d g%d f%d s%d sum%d i%d l%d\n", i, i/strIntSize, ptr->row, ptr->col, ptr->gen, ptr->flags, cellTable[i], cellTable[i+O_SUMNEAR], cellTable[i+O_INDEX], cellTable[i+O_LOOP]);
        printf("%d %d %d %d %d %d %d %d\n", cellTable[i+O_CUL], cellTable[i+O_CU], cellTable[i+O_CUR], cellTable[i+O_CL],cellTable[i+O_CR],cellTable[i+O_CDL],cellTable[i+O_CD],cellTable[i+O_CDR]);
    }
}

/* END CODE */
