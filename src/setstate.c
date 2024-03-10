#include "setstate.h"

void setState(const int cell, const State state)
{
    /* backup previous state */
    int diffState = state - cellTable[cell];
    /* set cell state */
    cellTable[cell] = state;
    /* correct the neighbor sum for cells touching this cell */
    for (int i = O_CUL; i <= O_CDR; i++)
        cellTable[cellTable[cell + i] + O_SUMNEAR] += diffState;

    return;
}
