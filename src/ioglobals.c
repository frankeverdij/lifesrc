#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "getnum.h"

void writeGlobals(FILE *fp, const Status currentStatus, const long stepConflicts, const globals * const g)
{
    fprintf(fp, "P");

    fprintf(fp, " %d", (int)currentStatus); /* current status of search */
    fprintf(fp, " %d", g->rowMax);       /* maximum number of rows */
    fprintf(fp, " %d", g->colMax);       /* maximum number of columns */
    fprintf(fp, " %d", g->genMax);       /* maximum number of generations */
    fprintf(fp, " %d", g->rowTrans);     /* translation of rows */
    fprintf(fp, " %d", g->colTrans);     /* translation of columns */
    fprintf(fp, " %d", g->rowSym);      /* enable row symmetry starting at column */
    fprintf(fp, " %d", g->colSym);      /* enable column symmetry starting at row */
    fprintf(fp, " %d", g->pointSym);    /* enable symmetry with central point */
    fprintf(fp, " %d", g->fwdSym);      /* enable forward diagonal symmetry */
    fprintf(fp, " %d", g->bwdSym);      /* enable backward diagonal symmetry */
    fprintf(fp, " %d", g->flipRows);    /* flip rows at column number from last to first generation */
    fprintf(fp, " %d", g->flipCols);    /* flip columns at row number from last to first generation */
    fprintf(fp, " %d", g->flipFwd);     /* flip forward diagonal (/) from last to first gen */
    fprintf(fp, " %d", g->flipBwd);     /* flip backward diagonal (\) from last to first gen */
    fprintf(fp, " %d", g->flipQuads);   /* flip quadrants from last to first gen */
    fprintf(fp, " %d", g->parent);      /* only look for parents */
    fprintf(fp, " %d", g->allObjects);  /* look for all objects including subPeriods */
    fprintf(fp, " %d", g->setDeep);     /* set cleared cells deeply from init file */
    fprintf(fp, " %d", g->nearCols);     /* maximum distance to be near columns */
    fprintf(fp, " %d", g->maxCount);     /* maximum number of cells in generation 0 */
    fprintf(fp, " %d", g->useRow);       /* row that must have at least one ON cell */
    fprintf(fp, " %d", g->useCol);       /* column that must have at least one ON cell */
    fprintf(fp, " %d", g->colCells);     /* maximum cells in a column */
    fprintf(fp, " %d", g->colWidth);     /* maximum width of each column */
    fprintf(fp, " %d", g->follow);      /* follow average position of previous column */
    fprintf(fp, " %d", g->orderWide);   /* ordering tries to find wide objects */
    fprintf(fp, " %d", g->orderGens);   /* ordering tries all gens first */
    fprintf(fp, " %d", g->orderInvert); /* Inverts direction of non-wide orderings */
    fprintf(fp, " %d", g->orderMiddle); /* ordering tries middle columns first */
    fprintf(fp, " %d", g->followGens);  /* try to follow setting of other gens */
    fprintf(fp, " %d", g->chooseUnknown); /* First choice for unknown cell, either ON or OFF */
    fprintf(fp, " %ld", stepConflicts);   /* step counter for one Proceed-Backup action */
    fprintf(fp, " %d", g->sortOrder);    /* sort direction */

    fprintf(fp, "\n");

    return;
}

int readGlobals(char * buf, Status currentStatus, long stepConflicts, globals * const g)
{
    if (buf[0] != 'P')
        return EXIT_FAILURE;

    const char * cp = &buf[1];

    currentStatus = (Status) getNum(&cp, 0); /* current status of search */
    g->rowMax = getNum(&cp, 0);       /* maximum number of rows */
    g->colMax = getNum(&cp, 0);       /* maximum number of columns */
    g->genMax = getNum(&cp, 0);       /* maximum number of generations */
    g->rowTrans = getNum(&cp, 0);     /* translation of rows */
    g->colTrans = getNum(&cp, 0);     /* translation of columns */
    g->rowSym = getNum(&cp, 0);      /* enable row symmetry starting at column */
    g->colSym = getNum(&cp, 0);      /* enable column symmetry starting at row */
    g->pointSym = getNum(&cp, 0);    /* enable symmetry with central point */
    g->fwdSym = getNum(&cp, 0);      /* enable forward diagonal symmetry */
    g->bwdSym = getNum(&cp, 0);      /* enable backward diagonal symmetry */
    g->flipRows = getNum(&cp, 0);    /* flip rows at column number from last to first generation */
    g->flipCols = getNum(&cp, 0);    /* flip columns at row number from last to first generation */
    g->flipFwd = getNum(&cp, 0);     /* flip forward diagonal (/) from last to first gen */
    g->flipBwd = getNum(&cp, 0);     /* flip backward diagonal (\) from last to first gen */
    g->flipQuads = getNum(&cp, 0);   /* flip quadrants from last to first gen */
    g->parent = getNum(&cp, 0);      /* only look for parents */
    g->allObjects = getNum(&cp, 0);  /* look for all objects including subPeriods */
    g->setDeep = getNum(&cp, 0);     /* set cleared cells deeply from init file */
    g->nearCols = getNum(&cp, 0);     /* maximum distance to be near columns */
    g->maxCount = getNum(&cp, 0);     /* maximum number of cells in generation 0 */
    g->useRow = getNum(&cp, 0);       /* row that must have at least one ON cell */
    g->useCol = getNum(&cp, 0);       /* column that must have at least one ON cell */
    g->colCells = getNum(&cp, 0);     /* maximum cells in a column */
    g->colWidth = getNum(&cp, 0);     /* maximum width of each column */
    g->follow = getNum(&cp, 0);      /* follow average position of previous column */
    g->orderWide = getNum(&cp, 0);   /* ordering tries to find wide objects */
    g->orderGens = getNum(&cp, 0);   /* ordering tries all gens first */
    g->orderInvert = getNum(&cp, 0); /* Inverts direction of non-wide orderings */
    g->orderMiddle = getNum(&cp, 0); /* ordering tries middle columns first */
    g->followGens = getNum(&cp, 0);  /* try to follow setting of other gens */
    g->chooseUnknown = getNum(&cp, 0); /* First choice for unknown cell, either ON or OFF */
    stepConflicts = getNum(&cp, 0);   /* step counter for one Proceed-Backup action */
    g->sortOrder = getNum(&cp, 0);    /* sort direction */

    return EXIT_SUCCESS;
}
