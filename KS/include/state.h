#ifndef STATE_H
#define STATE_H

typedef unsigned char STATE;

/*
 * States of a cell
 */
#define	OFF	((STATE) 0x00)		/* cell is known off */
#define	ON	((STATE) 0x01)		/* cell is known on */
#define	UNK	((STATE) 0x40)		/* cell is unknown */

#endif /* STATE_H */
