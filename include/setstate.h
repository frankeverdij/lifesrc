#ifndef SETSTATE_H
#define SETSTATE_H

#include "state.h"

static inline void setState(Cell * const cell, const State state)
{
    /* backup previous state */
    int diffState = state - cell->state;
    /* set cell state */
    cell->state = state;
    /* correct the neighbor sum for cells touching this cell */
    cell->cul->sumNear += diffState;
    cell->cu->sumNear += diffState;
    cell->cur->sumNear += diffState;
    cell->cl->sumNear += diffState;
    cell->cr->sumNear += diffState;
    cell->cdl->sumNear += diffState;
    cell->cd->sumNear += diffState;
    cell->cdr->sumNear += diffState;
}

#endif /* SETSTATE_H */
