#include "lifesrc.h"
#include "sortorder.h"

/*
 * The sort routine for searching->
 */
int orderSortFunc(const void * addr1, const void * addr2, void * gvars)
{
	const int*	arg1;
	const int*	arg2;
	int	c1;
	int	c2;
	int	midcol;
	int	midrow;
	int	dif1;
	int	dif2;
	int gen_diff;
	globals_struct * g;

	arg1 = (const int *) addr1;
	arg2 = (const int *) addr2;

	c1 = *arg1;
	c2 = *arg2;
    g = (globals_struct*) gvars;

    int row1, row2;
    int col1, col2;
    int gen1, gen2;
    int rcg1 = cellTable[c1 + O_RC0G];
    int rcg2 = cellTable[c2 + O_RC0G];

    gen1 = rcg1 & 0x0f;
    rcg1 >>= 16;
	row1 = rcg1 & 0x0f;
	rcg1 >>= 8;
	col1 = rcg1 & 0x0f;

    gen2 = rcg2 & 0x0f;
    rcg2 >>= 16;
	row2 = rcg2 & 0x0f;
	rcg2 >>= 8;
	col2 = rcg2 & 0x0f;

	// Put generation 0 first
	// or if calculating parents, put generation 0 last
	gen_diff = 0;
	if (g->parent) {
		if (gen1 < gen2) gen_diff = 1;
		if (gen1 > gen2) gen_diff = -1;
	}
	else {
		if (gen1 < gen2) gen_diff = -1;
		if (gen1 > gen2) gen_diff = 1;
	}

	/*
	 * If on equal position or not ordering by all generations
	 * then sort primarily by generations
	 */
	if (((row1 == row2) && (col1 == col2)) || !g->orderGens)
	{
		if (gen_diff!=0) return gen_diff;
		// if we are here, it is the same Cell
	}

	if(g->sortOrder==SORTORDER_DIAG) {
		if(col1+row1 > col2+row2) return (g->orderInvert)?(-1):1;
		if(col1+row1 < col2+row2) return (g->orderInvert)?1:(-1);
		if(abs(col1-row1) > abs(col2-row2)) return (g->orderWide)?1:(-1);
		if(abs(col1-row1) < abs(col2-row2)) return (g->orderWide)?(-1):1;
		return gen_diff;
	}
	if(g->sortOrder==SORTORDER_BACKDIAG) {
		if(colMax-col1+row1 > colMax-col2+row2) return (g->orderInvert)?(-1):1;
		if(colMax-col1+row1 < colMax-col2+row2) return (g->orderInvert)?1:(-1);
		if(abs(colMax-col1-row1) > abs(colMax-col2-row2)) return (g->orderWide)?1:(-1);
		if(abs(colMax-col1-row1) < abs(colMax-col2-row2)) return (g->orderWide)?(-1):1;
		return gen_diff;
	}
	else if(g->sortOrder==SORTORDER_KNIGHT) {
		if(col1*2+row1 > col2*2+row2) return (g->orderInvert)?(-1):1;
		if(col1*2+row1 < col2*2+row2) return (g->orderInvert)?1:(-1);
		if(abs(col1-row1) > abs(col2-row2)) return (g->orderWide)?1:(-1);
		if(abs(col1-row1) < abs(col2-row2)) return (g->orderWide)?(-1):1;
		return gen_diff;
	}
	else if(g->sortOrder==SORTORDER_TOPDOWN) {
		if(row1 > row2) return (g->orderInvert)?(-1):1;
		if(row1 < row2) return (g->orderInvert)?1:(-1);
		midcol = (g->colMax + 1) / 2;
		dif1 = abs(col1 - midcol);
		dif2 = abs(col2 - midcol);
		if (dif1 < dif2) return (g->orderWide ? -1 : 1);
		if (dif1 > dif2) return (g->orderWide ? 1 : -1);
		return gen_diff;
	}
	else if(g->sortOrder==SORTORDER_LEFTRIGHT) {
		if(col1 > col2) return (g->orderInvert)?(-1):1;
		if(col1 < col2) return (g->orderInvert)?1:(-1);
		midrow = (g->rowMax + 1) / 2;
		dif1 = abs(row1 - midrow);
		dif2 = abs(row2 - midrow);
		if (dif1 < dif2) return (g->orderWide ? -1 : 1);
		if (dif1 > dif2) return (g->orderWide ? 1 : -1);
		return gen_diff;
	}
	else if(g->sortOrder==SORTORDER_CENTEROUT) {
		double midcolf, midrowf, d1, d2;
		midcolf = (1.0+(double)g->colMax) / 2.0;
		midrowf = (1.0+(double)g->rowMax) / 2.0;
		d1 = (midcolf-(double)col1)*(midcolf-(double)col1) +
			(midrowf-(double)row1)*(midrowf-(double)row1);
		d2 = (midcolf-(double)col2)*(midcolf-(double)col2) +
			(midrowf-(double)row2)*(midrowf-(double)row2);
		if(d1>d2) return 1;
		if(d1<d2) return -1;
		return gen_diff;
	}
	else if(g->orderMiddle || g->sortOrder==SORTORDER_MIDDLECOLOUT) {
		// the ordering is from the center column outwards
		midcol = (g->colMax + 1) / 2;
		dif1 = abs(col1 - midcol);
		dif2 = abs(col2 - midcol);
		if (dif1 < dif2) return -1;
		if (dif1 > dif2) return 1;

		midrow = (g->rowMax + 1) / 2;
		dif1 = abs(row1 - midrow);
		dif2 = abs(row2 - midrow);
		if (dif1 < dif2) return (g->orderWide ? -1 : 1);
		if (dif1 > dif2) return (g->orderWide ? 1 : -1);
		return gen_diff;
	}

	// else left-to-right sort order

	if (col1 < col2) return -1;
	if (col1 > col2) return 1;

	/*
	 * Sort on the row number.
	 * By default, this is from the middle row outwards.
	 * But if wide ordering is set, the ordering is from the edge
	 * inwards.  Note that we actually set the ordering to be the
	 * opposite of the desired order because the initial setting
	 * for new Cells is OFF.
	 */
	midrow = (g->rowMax + 1) / 2;
	dif1 = abs(row1 - midrow);
	dif2 = abs(row2 - midrow);
	if (dif1 < dif2) return (g->orderWide ? -1 : 1);
	if (dif1 > dif2) return (g->orderWide ? 1 : -1);

	return gen_diff;
}

