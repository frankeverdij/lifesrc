#ifndef STATE_H
#define STATE_H

typedef unsigned char State;

/*
 * States of a cell
 */
#define	OFF	((State) 0x00)		/* cell is known off */
#define	ON	((State) 0x01)		/* cell is known on */
#define	UNK	((State) 0x20)		/* cell is unknown */

// KS Variant

#define UNK_KS ((State) 0)      /* cell is unknown */
#define ON_KS  ((State) 1)      /* cell is known on */
#define OFF_KS ((State) 9)      /* cell is known off */

#define	nStates	3			/* number of states */

#endif /* STATE_H */
