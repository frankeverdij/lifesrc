#ifndef STATE_H
#define STATE_H

typedef unsigned char State;

/*
 * States of a cell
 */
#define	OFF	((State) 0x00)		/* cell is known off */
#define	ON	((State) 0x01)		/* cell is known on */
#define	UNK	((State) 0x40)		/* cell is unknown */

#endif /* STATE_H */
