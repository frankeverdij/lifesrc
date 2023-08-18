#include <stdio.h>
#include <stdlib.h>
#include "state.h"
#include "nextstate.h"

static State unkRules[9 * ((int) UNK + 1)] = { UNK };

void initNextState(const State * bornRules, const State * liveRules)
{
    for (int i = 0 ; i < 9 ; i++)
    {
        unkRules[i + 9 * (int) OFF] = bornRules[i];
        unkRules[i + 9 * (int) ON ] = liveRules[i];
        unkRules[i + 9 * (int) UNK] = (bornRules[i] == liveRules[i]) ? bornRules[i] : UNK;
    }
}


State nextState(const State state, const int onCount)
{
    return unkRules[onCount + 9 * (int) state];
}


static State
nextState2(const State state, const int onCount, State * bornRules, State * liveRules)
{
	switch (state)
	{
		case ON:
			return liveRules[onCount];

		case OFF:
			return bornRules[onCount];

		case UNK:
			if (bornRules[onCount] == liveRules[onCount])
				return bornRules[onCount];

			/* fall into default case */

		default:
			return UNK;
	}
}

/*
int main (void) {
    const State bornRules[9] = { OFF, OFF, OFF, ON, OFF, OFF, OFF, OFF, OFF };
    const State liveRules[9] = { OFF, OFF, ON,  ON, OFF, OFF, OFF, OFF, OFF };
    initNextState(bornRules, liveRules);
    for (int i = 0; i < 9 ; i++) {
        printf("%d ",nextState( OFF, i));
    }
    printf("\n");
    for (int i = 0; i < 9 ; i++) {
        printf("%d ",nextState2( OFF, i, bornRules, liveRules));
    }
    printf("\n");
    for (int i = 0; i < 9 ; i++) {
        printf("%d ",nextState( ON, i));
    }
    printf("\n");
    for (int i = 0; i < 9 ; i++) {
        printf("%d ",nextState2( ON, i, bornRules, liveRules));
    }
    printf("\n");
    for (int i = 0; i < 9 ; i++) {
        printf("%d ",nextState( UNK, i));
    }
    printf("\n");
    for (int i = 0; i < 9 ; i++) {
        printf("%d ",nextState2( UNK, i, bornRules, liveRules));
    }
    printf("\n");
    
    return 0;
}
*/
