#ifndef ENUM_H
#define ENUM_H

/*
 * Status returned by routines
 */
//typedef enum {OK, ERROR, CONSISTENT, NOT_EXIST, FOUND} Status;

/*
 * Which sort directions do we support?
 */
typedef enum {DEFAULT, DIAG, BACKDIAG, KNIGHT, TOPDOWN, LEFTRIGHT,
                CENTEROUT, MIDDLECOLOUT} SortOrder;

#endif /* ENUM_H */
