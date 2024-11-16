#ifndef ENUM_H
#define ENUM_H

/*
 * Bool type
 */
typedef	int		Bool;

#define	FALSE		((Bool) 0)
#define	TRUE		((Bool) 1)


/*
 * Status returned by routines
 */
typedef enum {OK, ERROR, CONSISTENT, NOT_EXIST, FOUND} Status;

/*
 * Which sort directions do we support?
 */
typedef enum {DEFAULT, DIAG, BACKDIAG, KNIGHT, TOPDOWN, LEFTRIGHT,
                CENTEROUT, MIDDLECOLOUT} SortOrder;

#endif /* ENUM_H */
