#ifndef STATE_H
#define STATE_H

typedef unsigned char State;

/*
 * States of a cell
 */
#define	OFF	((State) 0x00)		/* cell is known off */
#define	ON	((State) 0x01)		/* cell is known on */
#define	UNK	((State) 0x40)		/* cell is unknown */

#define	nStates	3			/* number of states */

/*
 * Table of state values.
 */
static const State states[nStates] = {OFF, ON, UNK};

#endif /* STATE_H */
