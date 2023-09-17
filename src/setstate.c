#include "lifesrc.h"
#include "state.h"

void setState(Cell * const cell, const State state)
{
    /* backup previous state */
    State diffState = state - cell->state;
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

    return;
}
