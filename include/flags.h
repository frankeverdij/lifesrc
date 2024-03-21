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

#endif /* FLAGS_H */

