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
typedef int Status;

#define OK          ((Status) 0)
#define ERROR       ((Status) 1)
#define CONSISTENT  ((Status) 2)
#define NOT_EXIST   ((Status) 3)
#define FOUND       ((Status) 4)

/*
 * Which sort directions do we support?
 */
typedef enum {DEFAULT, DIAG, BACKDIAG, KNIGHT, TOPDOWN, LEFTRIGHT,
                CENTEROUT, MIDDLECOLOUT} SortOrder;

#endif /* ENUM_H */
