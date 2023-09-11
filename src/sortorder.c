#include "lifesrc.h"
#include "sortorder.h"

/*
 * The sort routine for searching->
 */
int orderSortFunc(const void * addr1, const void * addr2, const void * gvars)
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

	if(g->sortOrder==SORTORDER_DIAG) {
		if(c1->col+c1->row > c2->col+c2->row) return 1;
		if(c1->col+c1->row < c2->col+c2->row) return -1;
		if(abs(c1->col-c1->row) > abs(c2->col-c2->row)) return (g->orderWide)?1:(-1);
		if(abs(c1->col-c1->row) < abs(c2->col-c2->row)) return (g->orderWide)?(-1):1;
		return gen_diff;
	}
	else if(g->sortOrder==SORTORDER_KNIGHT) {
		if(c1->col*2+c1->row > c2->col*2+c2->row) return 1;
		if(c1->col*2+c1->row < c2->col*2+c2->row) return -1;
		if(abs(c1->col-c1->row) > abs(c2->col-c2->row)) return (g->orderWide)?1:(-1);
		if(abs(c1->col-c1->row) < abs(c2->col-c2->row)) return (g->orderWide)?(-1):1;
		return gen_diff;
	}
	else if(g->sortOrder==SORTORDER_TOPDOWN) {
		if(c1->row > c2->row) return 1;
		if(c1->row < c2->row) return -1;
		midcol = (g->colMax + 1) / 2;
		dif1 = abs(c1->col - midcol);
		dif2 = abs(c2->col - midcol);
		if (dif1 < dif2) return (g->orderWide ? -1 : 1);
		if (dif1 > dif2) return (g->orderWide ? 1 : -1);
		return gen_diff;
	}
	else if(g->sortOrder==SORTORDER_CENTEROUT) {
		double midcolf, midrowf, d1, d2;
		midcolf = (1.0+(double)g->colMax) / 2.0;
		midrowf = (1.0+(double)g->rowMax) / 2.0;
		d1 = (midcolf-(double)c1->col)*(midcolf-(double)c1->col) + 
			(midrowf-(double)c1->row)*(midrowf-(double)c1->row);
		d2 = (midcolf-(double)c2->col)*(midcolf-(double)c2->col) + 
			(midrowf-(double)c2->row)*(midrowf-(double)c2->row);
		if(d1>d2) return 1;
		if(d1<d2) return -1;
		return gen_diff;
	}
	else if(g->orderMiddle || g->sortOrder==SORTORDER_MIDDLECOLOUT) {
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

/*
 * Old sort routine for searching.
 */
static int
orderSortFuncOld(const void * addr1, const void * addr2, const void *gvars)
{
	const Cell **	cp1;
	const Cell **	cp2;
	const Cell *	c1;
	const Cell *	c2;
	int		midCol;
	int		midRow;
	int		dif1;
	int		dif2;
    globals_struct *g;

	cp1 = ((const Cell **) addr1);
	cp2 = ((const Cell **) addr2);

	c1 = *cp1;
	c2 = *cp2;
    g = (globals_struct*) gvars;

	/*
	 * If we do not order by all generations, then put all of
	 * generation zero ahead of the other generations.
	 */
	if (!g->orderGens)
	{
		if (c1->gen < c2->gen)
			return -1;

		if (c1->gen > c2->gen)
			return 1;
	}

	/*
	 * Sort on the column number.
	 * By default this is from left to right.
	 * But if middle ordering is set, the ordering is from the center
	 * column outwards.
	 */
	if (g->orderMiddle)
	{
		midCol = (colMax + 1) / 2;

		dif1 = c1->col - midCol;

		if (dif1 < 0)
			dif1 = -dif1;

		dif2 = c2->col - midCol;

		if (dif2 < 0)
			dif2 = -dif2;

		if (dif1 < dif2)
			return -1;

		if (dif1 > dif2)
			return 1;
	}
	else
	{
		if (c1->col < c2->col)
			return -1;

		if (c1->col > c2->col)
			return 1;
	}

	/*
	 * Sort "even" positions ahead of "odd" positions.
	 */
	dif1 = (c1->row + c1->col + c1->gen) & 0x01;
	dif2 = (c2->row + c2->col + c2->gen) & 0x01;

	if (dif1 != dif2)
		return dif1 - dif2;

	/*
	 * Sort on the row number.
	 * By default, this is from the middle row outwards.
	 * But if wide ordering is set, the ordering is from the edge
	 * inwards.  Note that we actually set the ordering to be the
	 * opposite of the desired order because the initial setting
	 * for new cells is OFF.
	 */
	midRow = (g->rowMax + 1) / 2;

	dif1 = c1->row - midRow;

	if (dif1 < 0)
		dif1 = -dif1;

	dif2 = c2->row - midRow;

	if (dif2 < 0)
		dif2 = -dif2;

	if (dif1 < dif2)
		return (g->orderWide ? -1 : 1);

	if (dif1 > dif2)
		return (g->orderWide ? 1 : -1);

	/*
	 * Sort by the generation again if we didn't do it yet.
	 */
	if (c1->gen < c2->gen)
		return -1;

	if (c1->gen > c2->gen)
		return 1;

	return 0;
}

