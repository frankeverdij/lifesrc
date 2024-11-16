#ifndef FLAGS_H
#define FLAGS_H

/*
 * IMPLIC flag values.
 */
typedef	unsigned char	Flags;

#define	N0IC0	((Flags) 0x01)	/* new cell 0 ==> current cell 0 */
#define	N0IC1	((Flags) 0x02)	/* new cell 0 ==> current cell 1 */
#define	N0ICUN0	((Flags) 0x04)	/* new cell 0 ==> current unknown neighbors 0 */
#define	N0ICUN1	((Flags) 0x08)	/* new cell 0 ==> current unknown neighbors 1 */
#define	N1IC0	((Flags) 0x10)	/* new cell 1 ==> current cell 0 */
#define	N1IC1	((Flags) 0x20)	/* new cell 1 ==> current cell 1 */
#define	N1ICUN0	((Flags) 0x40)	/* new cell 1 ==> current unknown neighbors 0 */
#define	N1ICUN1	((Flags) 0x80)	/* new cell 1 ==> current unknown neighbors 1 */

// IMPLIC flag values for KS search

#define IMPBAD	((Flags) 0x00)	// the Cell State is inconsistent
#define IMPUN	((Flags) 0x01)	// change unknown neighbors (there are some)
#define IMPUN1	((Flags) 0x02)	// change unknown neighbors to 1 (if not set, then change to 0)
#define IMPC	((Flags) 0x04)	// change current Cell (it is unknown)
#define IMPC1	((Flags) 0x08)	// change current Cell to 1 (if not set, change it to 0)
#define IMPN	((Flags) 0x10)	// change new Cell (it is unknown)
#define IMPN1	((Flags) 0x20)	// change new Cell to 1 (if not set, change it to 0)
#define IMPVOID ((Flags) 0x40)  // invalid/unset implication
#define IMPOK   ((Flags) 0x80)  // valid State

#endif /* FLAGS_H */

