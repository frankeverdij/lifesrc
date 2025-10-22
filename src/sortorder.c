#include <stdlib.h>

#include "cell.h"
#include "sortorder.h"

/*
 * The sort routine for searching->
 */
int orderSortFunc(const void * addr1, const void * addr2, void * gvars)
{
	const Cell **	arg1;
	const Cell **	arg2;
	const Cell *	c1;
	const Cell *	c2;
	int	midcol;
	int	midrow;
	int	dif1;
	int	dif2;
	int gen_diff;
	globals_struct * g;

	arg1 = (const Cell**) addr1;
	arg2 = (const Cell**) addr2;

	c1 = *arg1;
	c2 = *arg2;
    g = (globals_struct*) gvars;

    // Put generation 0 first
    // or if calculating parents, put generation 0 last
    gen_diff = 0;

    if (g->parent)
    {
        if (c1->gen < c2->gen) gen_diff = 1;
        if (c1->gen > c2->gen) gen_diff = -1;
    }
    else
    {
        if (c1->gen < c2->gen) gen_diff = -1;
        if (c1->gen > c2->gen) gen_diff = 1;
    }

    /*
     * If on equal position or not ordering by all generations
     * then sort primarily by generations
     */
    if (((c1->row == c2->row) && (c1->col == c2->col)) || !g->orderGens)
    {
        if (gen_diff!=0) return gen_diff;
        // if we are here, it is the same Cell
    }

    if (g->sortOrder==DIAG)
    {
        if (c1->col + c1->row > c2->col + c2->row) return (g->orderInvert)?(-1):1;
        if (c1->col + c1->row < c2->col + c2->row) return (g->orderInvert)?1:(-1);

        dif1 = c1->col - c1->row;
        dif2 = c2->col - c2->row;
        if (g->orderWide)
        {
            if (dif1 > dif2) return -1;
            if (dif1 < dif2) return 1;
        }
        else
        {
            if (abs(dif1) > abs(dif2)) return 1;
            if (abs(dif1) < abs(dif2)) return -1;
        }

        return gen_diff;
    }
    else if (g->sortOrder==BACKDIAG)
    {
        /*
         * back diagonal sorting has a small problem in that the cells
         * available after symmetry are located lower left instead of
         * upper right. This can be changed in initSortOrder() by changing
         * bwdSym condition to (row > col) but this does not match the
         * condition for smart searches. For now we keep the current condition.
         */
        int mxc1 = g->colMax - c1->col + 1;
        int mxc2 = g->colMax - c2->col + 1;
        if (mxc1 + c1->row > mxc2 + c2->row) return (g->orderInvert)?(-1):1;
        if (mxc1 + c1->row < mxc2 + c2->row) return (g->orderInvert)?1:(-1);

        dif1 = mxc1 - c1->row;
        dif2 = mxc2 - c2->row;
        if (g->orderWide)
        {
            if (dif1 > dif2) return -1;
            if (dif1 < dif2) return 1;
        }
        else
        {
            if (abs(dif1) > abs(dif2)) return 1;
            if (abs(dif1) < abs(dif2)) return -1;
        }

        return gen_diff;
    }
    else if (g->sortOrder==KNIGHT)
    {
        if (c1->col * 2 + c1->row > c2->col * 2 + c2->row) return (g->orderInvert)?(-1):1;
        if (c1->col * 2 + c1->row < c2->col * 2 + c2->row) return (g->orderInvert)?1:(-1);
        if (abs(c1->col - c1->row) > abs(c2->col - c2->row)) return (g->orderWide)?1:(-1);
        if (abs(c1->col - c1->row) < abs(c2->col - c2->row)) return (g->orderWide)?(-1):1;
        return gen_diff;
    }
    else if (g->sortOrder==TOPDOWN)
    {
        if (c1->row > c2->row) return (g->orderInvert)?(-1):1;
        if (c1->row < c2->row) return (g->orderInvert)?1:(-1);

        midcol = (g->colMax + 1) / 2;
        dif1 = c1->col - midcol;
        dif2 = c2->col - midcol;

        if (g->orderWide)
        {
            if (dif1 > dif2) return 1;
            if (dif1 < dif2) return -1;
        }
        else
        {
            if (abs(dif1) > abs(dif2)) return 1;
            if (abs(dif1) < abs(dif2)) return -1;
        }

        return gen_diff;
    }
    else if (g->sortOrder==CENTEROUT)
    {
        double midcolf, midrowf, d1, d2;
        midcolf = (1.0 + (double)g->colMax) / 2.0;
        midrowf = (1.0 + (double)g->rowMax) / 2.0;
        d1 = (midcolf - (double)c1->col) * (midcolf - (double)c1->col) +
            (midrowf - (double)c1->row) * (midrowf - (double)c1->row);
        d2 = (midcolf - (double)c2->col) * (midcolf - (double)c2->col) +
            (midrowf - (double)c2->row) * (midrowf - (double)c2->row);
        // wide ordering needs opposite sign as opposed to the rest.
        if (d1 < d2) return (g->orderWide ? 1 : -1);
        if (d1 > d2) return (g->orderWide ? -1 : 1);
        return gen_diff;
    }
    else if (g->orderMiddle || g->sortOrder==MIDDLECOLOUT)
    {
        // the ordering is from the center column outwards.
        // This is an old option, kept for historical reference
        midcol = (g->colMax + 1) / 2;
        dif1 = abs(c1->col - midcol);
        dif2 = abs(c2->col - midcol);
        if (dif1 < dif2) return -1;
        if (dif1 > dif2) return 1;

        midrow = (g->rowMax + 1) / 2;
        dif1 = abs(c1->row - midrow);
        dif2 = abs(c2->row - midrow);
        if (dif1 < dif2) return (g->orderWide ? -1 : 1);
        if (dif1 > dif2) return (g->orderWide ? 1 : -1);
        return gen_diff;
    }
    else
    {
        // The default is left to right sort order
        if (c1->col > c2->col) return (g->orderInvert)?(-1):1;
        if (c1->col < c2->col) return (g->orderInvert)?1:(-1);

        midrow = (g->rowMax + 1) / 2;
        dif1 = c1->row - midrow;
        dif2 = c2->row - midrow;

        if (g->orderWide)
        {
            if (dif1 > dif2) return 1;
            if (dif1 < dif2) return -1;
        }
        else
        {
            if (abs(dif1) > abs(dif2)) return 1;
            if (abs(dif1) < abs(dif2)) return -1;
        }

        return gen_diff;
    }
}

int orderSortFuncOld(const void * addr1, const void * addr2, void * gvars)
{
	const Cell **	arg1;
	const Cell **	arg2;
	const Cell *	c1;
	const Cell *	c2;
	int	midcol;
	int	midrow;
	int	dif1;
	int	dif2;
	int gen_diff;
	globals_struct * g;

	arg1 = (const Cell**) addr1;
	arg2 = (const Cell**) addr2;

	c1 = *arg1;
	c2 = *arg2;
    g = (globals_struct*) gvars;

	// Put generation 0 first
	// or if calculating parents, put generation 0 last
	gen_diff = 0;
	if (g->parent) {
		if (c1->gen < c2->gen) gen_diff = 1;
		if (c1->gen > c2->gen) gen_diff = -1;
	}
	else {
		if (c1->gen < c2->gen) gen_diff = -1;
		if (c1->gen > c2->gen) gen_diff = 1;
	}

	/*
	 * If on equal position or not ordering by all generations
	 * then sort primarily by generations
	 */
	if (((c1->row == c2->row) && (c1->col == c2->col)) || !g->orderGens)
	{
		if (gen_diff!=0) return gen_diff;
		// if we are here, it is the same Cell
	}

	if(g->sortOrder==DIAG) {
		if(c1->col+c1->row > c2->col+c2->row) return (g->orderInvert)?(-1):1;
		if(c1->col+c1->row < c2->col+c2->row) return (g->orderInvert)?1:(-1);
		if(abs(c1->col-c1->row) > abs(c2->col-c2->row)) return (g->orderWide)?1:(-1);
		if(abs(c1->col-c1->row) < abs(c2->col-c2->row)) return (g->orderWide)?(-1):1;
		return gen_diff;
	}
	if(g->sortOrder==BACKDIAG) {
		if(g->colMax-c1->col+c1->row > g->colMax-c2->col+c2->row) return (g->orderInvert)?(-1):1;
		if(g->colMax-c1->col+c1->row < g->colMax-c2->col+c2->row) return (g->orderInvert)?1:(-1);
		if(abs(g->colMax-c1->col-c1->row) > abs(g->colMax-c2->col-c2->row)) return (g->orderWide)?1:(-1);
		if(abs(g->colMax-c1->col-c1->row) < abs(g->colMax-c2->col-c2->row)) return (g->orderWide)?(-1):1;
		return gen_diff;
	}
	else if(g->sortOrder==KNIGHT) {
		if(c1->col*2+c1->row > c2->col*2+c2->row) return (g->orderInvert)?(-1):1;
		if(c1->col*2+c1->row < c2->col*2+c2->row) return (g->orderInvert)?1:(-1);
		if(abs(c1->col-c1->row) > abs(c2->col-c2->row)) return (g->orderWide)?1:(-1);
		if(abs(c1->col-c1->row) < abs(c2->col-c2->row)) return (g->orderWide)?(-1):1;
		return gen_diff;
	}
	else if(g->sortOrder==TOPDOWN) {
		if(c1->row > c2->row) return (g->orderInvert)?(-1):1;
		if(c1->row < c2->row) return (g->orderInvert)?1:(-1);
		midcol = (g->colMax + 1) / 2;
		dif1 = abs(c1->col - midcol);
		dif2 = abs(c2->col - midcol);
		if (dif1 < dif2) return (g->orderWide ? -1 : 1);
		if (dif1 > dif2) return (g->orderWide ? 1 : -1);
		return gen_diff;
	}
	else if(g->sortOrder==LEFTRIGHT) {
		if(c1->col > c2->col) return (g->orderInvert)?(-1):1;
		if(c1->col < c2->col) return (g->orderInvert)?1:(-1);
		midrow = (g->rowMax + 1) / 2;
		dif1 = abs(c1->row - midrow);
		dif2 = abs(c2->row - midrow);
		if (dif1 < dif2) return (g->orderWide ? -1 : 1);
		if (dif1 > dif2) return (g->orderWide ? 1 : -1);
		return gen_diff;
	}
	else if(g->sortOrder==CENTEROUT) {
		double midcolf, midrowf, d1, d2;
		midcolf = (1.0+(double)g->colMax) / 2.0;
		midrowf = (1.0+(double)g->rowMax) / 2.0;
		d1 = (midcolf-(double)c1->col)*(midcolf-(double)c1->col) + 
			(midrowf-(double)c1->row)*(midrowf-(double)c1->row);
		d2 = (midcolf-(double)c2->col)*(midcolf-(double)c2->col) + 
			(midrowf-(double)c2->row)*(midrowf-(double)c2->row);
	    // wide ordering needs opposite sign as opposed to the rest.
		if(d1 < d2) return (g->orderWide ? 1 : -1);
		if(d1 > d2) return (g->orderWide ? -1 : 1);
		return gen_diff;
	}
	else if(g->orderMiddle || g->sortOrder==MIDDLECOLOUT) {
		// the ordering is from the center column outwards
		midcol = (g->colMax + 1) / 2;
		dif1 = abs(c1->col - midcol);
		dif2 = abs(c2->col - midcol);
		if (dif1 < dif2) return -1;
		if (dif1 > dif2) return 1;

		midrow = (g->rowMax + 1) / 2;
		dif1 = abs(c1->row - midrow);
		dif2 = abs(c2->row - midrow);
		if (dif1 < dif2) return (g->orderWide ? -1 : 1);
		if (dif1 > dif2) return (g->orderWide ? 1 : -1);
		return gen_diff;
	}

	// else left-to-right sort order

	if (c1->col < c2->col) return -1;
	if (c1->col > c2->col) return 1;

	/*
	 * Sort on the row number.
	 * By default, this is from the middle row outwards.
	 * But if wide ordering is set, the ordering is from the edge
	 * inwards.  Note that we actually set the ordering to be the
	 * opposite of the desired order because the initial setting
	 * for new Cells is OFF.
	 */
	midrow = (g->rowMax + 1) / 2;
	dif1 = abs(c1->row - midrow);
	dif2 = abs(c2->row - midrow);
	if (dif1 < dif2) return (g->orderWide ? -1 : 1);
	if (dif1 > dif2) return (g->orderWide ? 1 : -1);

	return gen_diff;
}
