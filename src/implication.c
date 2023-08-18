#include "state.h"
#include "flags.h"
#include "implication.h"
#include "nextstate.h"
#include "description.h"

/*
 * Determine the implications of a cell depending on its known neighbor counts.
 * The unknown neighbor count is implicit since there are eight neighbors.
 */
Flags implication(State state, int offCount, int onCount)
{
	Flags	flags;
	State	next;
	int	unkCount;
	int	i;

	unkCount = 8 - offCount - onCount;
	flags = 0;

	if (state == UNK)
	{
		/*
		 * Set them all.
		 */
		flags |= (N0IC0 | N0IC1 | N1IC0 | N1IC1);

		for (i = 0; i <= unkCount; i++)
		{
			/*
			 * Look for contradictions.
			 */
			next = nextState(OFF, onCount + i);

			if (next == ON)
				flags &= ~N1IC1;
			else if (next == OFF)
				flags &= ~N0IC1;

			next = nextState(ON, onCount + i);

			if (next == ON)
				flags &= ~N1IC0;
			else if (next == OFF)
				flags &= ~N0IC0;
		}
	}
	
	if (unkCount)
	{
		flags |= (N0ICUN0 | N0ICUN1 | N1ICUN0 | N1ICUN1);

		if ((state == OFF) || (state == UNK))
		{
			/*
			 * Try unknowns zero.
			 */
			next = nextState(OFF, onCount);

			if (next == ON)
				flags &= ~N1ICUN1;
			else if (next == OFF)
				flags &= ~N0ICUN1;

			/*
			 * Try all ones.
			 */
			next = nextState(OFF, onCount + unkCount);

			if (next == ON)
				flags &= ~N1ICUN0;
			else if (next == OFF)
				flags &= ~N0ICUN0;
		}

		if ((state == ON) || (state == UNK))
		{
			/*
			 * Try unknowns zero.
			 */
			next = nextState(ON, onCount);

			if (next == ON)
				flags &= ~N1ICUN1;
			else if (next == OFF)
				flags &= ~N0ICUN1;

			/*
			 * Try all ones.
			 */
			next = nextState(ON, onCount + unkCount);

			if (next == ON)
				flags &= ~N1ICUN0;
			else if (next == OFF)
				flags &= ~N0ICUN0;
		}

		for (i = 1; i <= unkCount - 1; i++)
		{
			if ((state == OFF) || (state == UNK))
			{
				next = nextState(OFF, onCount + i);

				if (next == ON)
					flags &= ~(N1ICUN0 | N1ICUN1);
				else if (next == OFF)
					flags &= ~(N0ICUN0 | N0ICUN1);
			}

			if ((state == ON) || (state == UNK))
			{
				next = nextState(ON, onCount + i);

				if (next == ON)
					flags &= ~(N1ICUN0 | N1ICUN1);
				else if (next == OFF)
					flags &= ~(N0ICUN0 | N0ICUN1);
			}
		}
	}
  
	return flags;
}

/*
 * Initialize the implication table.
 */
void initImplic(const State * states, Flags *implic)
{
	State	state;
	int	offCount;
	int	onCount;
	int	sum;
	int	desc;
	int	i;

	for (i = 0; i < nStates; i++)
	{
		state = states[i];

		for (offCount = 8; offCount >= 0; offCount--)
		{
			for (onCount = 0; onCount + offCount <= 8; onCount++)
			{
				sum = onCount + (8 - onCount - offCount) * UNK;
				desc = sumToDesc(state, sum);

				implic[desc] =
					implication(state, offCount, onCount);
			}
		}
	}
}
