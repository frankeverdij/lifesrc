#include "lifesrc.h"
#include "state.h"

#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

#define MAXCOLS_RLE 70

static int buffer[ROW_MAX * COL_MAX] = {0};

char stateToString(const State state)
{
    if (state == ON)
        return 'o';
    if (state == UNK)
        return '?';
    if (state == 255)
        return '$';

    return 'b';
}

void printState(const State state, int * stateCounter, int * colCounter)
{
    /* print a line of a particular state */
    if (*stateCounter == 0)
        return;
    if (*stateCounter == 1)
    {
         ttyPrintf("%c", stateToString(state));
         (*colCounter)++;
    }
    else
    {
        ttyPrintf("%d%c", *stateCounter, stateToString(state));
        *colCounter += (*stateCounter > 9) ? 3 : 2 ;
    }
    *stateCounter = 0;

    if (*colCounter > MAXCOLS_RLE)
    {
        ttyPrintf("\n");
        *colCounter = 0;
    }
    
    return;
}

void engineRLE(const char *rule)
{
    int rmin = rowMax + 1;
    int rmax = 0;
    int cmin = colMax + 1;
    int cmax = 0;
    
    State state = 0, prevState = 0;
    int colCounter = 0;
    int stateCounter = 0;
    int lineCounter = -1;
    
    /* first determine pattern limits */
    for (int row = 1; row <= rowMax; row ++)
    {
        for (int col = 1; col <= colMax; col++)
        {
            if (buffer[col + row * colMax] != OFF)
            {
                rmin = min(rmin, row);
                rmax = max(rmax, row);
                cmin = min(cmin, col);
                cmax = max(cmax, col);
            }
        }
    }
    
    /* print header */
    ttyPrintf("x = %d, y = %d, rule = %s\n", cmax - cmin + 1, rmax - rmin + 1, rule);
    
    /* start coding the pattern */
    for (int row = rmin; row <= rmax; row ++)
    {
        stateCounter = 0;
        for (int col = cmin; col <= cmax; col++)
        {
            state = buffer[col + row * colMax];
            
            /* set prevState only at the very beginning of the pattern evaluation */
            if (lineCounter < 0)
            {
                lineCounter = 0;
                prevState = state;
            }

            /* state change? */
            if (prevState != state)
            {
                /* is it a non-zero state and do we have linebreaks? */
                if ((state) && (lineCounter))
                { 
                    /* print and reset the linebreaks */
                    printState(255, &lineCounter, &colCounter);
                }

                printState(prevState, &stateCounter, &colCounter);
                prevState = state;
            }

            stateCounter++;
        }
 
        lineCounter++;
        if (state)
        {
            printState(prevState, &stateCounter, &colCounter);
            prevState = 0;
        }
    }
    ttyPrintf("!\n");
    
    return;   
}

void printRLE(const int gen, const char *rule)
{
    const Cell *cell;

    /* initialize buffer */
    for (int row = 1; row <= rowMax; row ++)
    {
        for (int col = 1; col <= colMax; col++)
        {
            cell = findCell(row , col, gen);
            buffer[col + row * colMax] = cell->state;
        }
    }
    
    engineRLE(rule);
    
    return;
}
