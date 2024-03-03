#include "setstate.h"

void setState(const int idx, const State state, State * stateIndex)
{
    /* backup previous state */
    int diffState = state - cellTable[idx];
    /* set cell state */
    cellTable[idx] = state;
    if (stateIndex)
        if (cellTable[idx + O_INDEX] >= 0)
            stateIndex[cellTable[idx + O_INDEX]] = state;
    /* correct the neighbor sum for cells touching this cell */
    cellTable[cellTable[idx + O_CUL] + O_SUMNEAR] += diffState;
    cellTable[cellTable[idx + O_CU ] + O_SUMNEAR] += diffState;
    cellTable[cellTable[idx + O_CUR] + O_SUMNEAR] += diffState;
    cellTable[cellTable[idx + O_CL ] + O_SUMNEAR] += diffState;
    cellTable[cellTable[idx + O_CR ] + O_SUMNEAR] += diffState;
    cellTable[cellTable[idx + O_CDL] + O_SUMNEAR] += diffState;
    cellTable[cellTable[idx + O_CD ] + O_SUMNEAR] += diffState;
    cellTable[cellTable[idx + O_CDR] + O_SUMNEAR] += diffState;

    return;
}
