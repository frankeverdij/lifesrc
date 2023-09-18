#include <string.h>
#include <stdbool.h>
#include "state.h"
#include "transition.h"
#include "nextstate.h"
#include "description.h"

/*
 * Determine the transition of a cell depending on its known neighbor counts.
 * The unknown neighbor count is implicit since there are eight neighbors.
 */
State transition(State state, int offCount, int onCount)
{
	bool	onAlways;
	bool	offAlways;
	int	unkCount;
 	int	i;

 	onAlways = true;
	offAlways = true;
	unkCount = 8 - offCount - onCount;
 
	for (i = 0; i <= unkCount; i++)
	{
		switch (nextState(state, onCount + i))
		{
			case ON:
				offAlways = false;
				break;

			case OFF:
				onAlways = false;
				break;

			default:
				return UNK;
		}
	}

	if (onAlways)
		return ON;

	if (offAlways)
		return OFF;

	return UNK;
}

/*
 * Initialize the transition table.
 */
void initTransit(const State * states, State * transit)
{
	int	state;
	int	offCount;
	int	onCount;
	int	sum;
	int	desc;
	int	i;

    memset(transit, -1, 1024*sizeof(State));
	for (i = 0; i < nStates; i++)
	{
		state = states[i];

		for (offCount = 8; offCount >= 0; offCount--)
		{
			for (onCount = 0; onCount + offCount <= 8; onCount++)
			{
				sum = onCount + (8 - onCount - offCount) * UNK;
				desc = SUMTODESC(state, sum);

				transit[desc] =
					transition(state, offCount, onCount);
			}
		}
	}
}
