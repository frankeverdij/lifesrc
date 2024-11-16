#include <stdio.h>
#include <stdlib.h>

#include "state.h"
#include "enums.h"
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
				desc = SUMTODESC(state, sum);

				implic[desc] =
					implication(state, offCount, onCount);
			}
		}
	}
}

static __inline int
sumtodesc(State futureState, State currentState, int neighborsum)
{
	// UNK = 0
	// ON = 1
	// OFF = 9

	// using the following expression, all different
	// combinations are mapped to different numbers
	// if you don't believe it, just try it

	return (neighborsum*10 + currentState*3 + futureState);
}

/*
 * Initialize the implication table, KS variant.
 */
void initimplic_KS(Flags *implic, const State * bornrules, const State * liverules)
{
	int	nunk, non, noff, cunk, con, coff, funk, fon, foff, naon, caon, faon, desc;
	Bool valid, cison, cisoff, fison, fisoff, nison, nisoff;

	for (desc=0; desc<1024; desc++) {
		implic[desc] = IMPVOID;
	}

	for (nunk=0; nunk<=8; nunk++) { // unknown neighbors
		for (non=0; non+nunk<=8; non++) { // on neighbors from the known ones
			noff=8-(non+nunk); // off known neighbors
			for (cunk=0; cunk<=1; cunk++) { // unknown Cell
				for (con=0; con+cunk<=1; con++) { // on Cell
					coff=1-(con+cunk); // off Cell
					for (funk=0; funk<=1; funk++) { // unknown future Cell
						for (fon=0; fon+funk<=1; fon++) { // on future Cell
							foff=1-(fon+funk); // off future Cell
							desc = sumtodesc((State)(funk*UNK_KS+fon*ON_KS+foff*OFF_KS), (State)(cunk*UNK_KS+con*ON_KS+coff*OFF_KS), nunk*UNK_KS+non*ON_KS+noff*OFF_KS);
							if (implic[desc] != IMPVOID) {
								fprintf(stderr, "Duplicate descriptor!!!");
								exit(1);
							}
							// here we get all possible descriptors
							// now let's try all possible States for that descriptor
							valid = FALSE; // will change to TRUE if we get at least one valid State
							cison = TRUE; // will change to FALSE if we get a valid State with c Cell OFF
							cisoff = TRUE; // will change to FALSE if we get a valid State with c Cell ON
							fison = TRUE; // will change to FALSE if we get a valid State with f Cell OFF
							fisoff = TRUE; // will change to FALSE if we get a valid State with f Cell ON
							nison = TRUE; // will change to FALSE if we get a valid State with one unknown neighbor OFF
							nisoff = TRUE; // will change to FALSE if we get a valid State with one unknown neighbor ON
							for (naon = non; naon <= 8-noff; naon++) { // neighbors
								for (caon = con; caon <= 1-coff; caon++) { // try center
									for (faon = fon; faon <= 1-foff; faon++) { // try future
										// here we have all possible States for the descriptor
										// now for the rules
										if (((caon == 0) && (faon == 0) && !bornrules[naon]) // both dead
											|| ((caon != 0) && (faon == 0) && !liverules[naon]) // dying
											|| ((caon == 0) && (faon != 0) && bornrules[naon]) // birth
											|| ((caon != 0) && (faon != 0) && liverules[naon])) { // survival
											// woohoo! we got a valid State
											valid = TRUE;
											if (caon == 0) {
												cison = FALSE;
											} else {
												cisoff = FALSE;
											}
											if (faon == 0) {
												fison =  FALSE;
											} else {
												fisoff = FALSE;
											}
											if (naon>non) nisoff = FALSE;
											if (naon<non+nunk) nison = FALSE;
										}
									}
								}
							}
							// descriptor examination has ended
							// now for the results
							if (!valid) {
								implic[desc] = IMPBAD;
							} else {
								implic[desc] = IMPOK;
								if (funk != 0) { // future Cell is unknown
									if (fison || fisoff) { // and just one State is possible
										implic[desc] |= IMPN;
										if (fison) {
											implic[desc] |= IMPN1;
										}
									}
								}
								if (cunk != 0) {
									if (cison || cisoff) {
										implic[desc] |= IMPC;
										if (cison) {
											implic[desc] |= IMPC1;
										}
									}
								}
								if (nunk != 0) {
									if (nison || nisoff) {
										implic[desc] |= IMPUN;
										if (nison) {
											implic[desc] |= IMPUN1;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	for (desc=0; desc<1024; desc++) {
		if (implic[desc] == IMPVOID) {
			implic[desc] = IMPBAD;
		}
	}

}


