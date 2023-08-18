#include "state.h"
#include "description.h"

/*
 * Return the descriptor value for a cell and the sum of its neighbors.
 */
int sumToDesc(const State state, const int sum)
{
    return sum * 2 + state;
}

