#ifndef CELLFLAGS_H
#define CELLFLAGS_H

/*
 * flag bits of a cell
 */
typedef unsigned char cellFlags;

#define FREECELL	((cellFlags) 0x01) /* this cell still has free choice */
#define FROZENCELL	((cellFlags) 0x02) /* this cell is frozen in all gens */
#define CHOOSECELL	((cellFlags) 0x04) /* can choose this cell if unknown */

#endif /* CELLFLAGS_H */
