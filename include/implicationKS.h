#ifndef IMPLICATIONKS_H
#define IMPLICATIONKS_H

#include "enums.h"
#include "state.h"

/*
 * Descriptor for use with the implications table
 */
#define SUMTODESCKS(a, b, c)  ((a) + 2 * (b) + 4 * (c))

/*
 * Size of the implication array for the values OFF=0, ON=1 and UNK=64 (0x40)
 */
#define IMPLICSIZEKS 2304

/*
 * IMPLIC flag values.
 */
typedef unsigned char FLAGS;
#define IMPBAD  ((FLAGS) 0x00)  // the cell state is inconsistent
#define IMPUN   ((FLAGS) 0x01)  // change unknown neighbors (there are some)
#define IMPUN1  ((FLAGS) 0x02)  // change unknown neighbors to 1 (if not set, then change to 0)
#define IMPC    ((FLAGS) 0x04)  // change current cell (it is unknown)
#define IMPC1   ((FLAGS) 0x08)  // change current cell to 1 (if not set, change it to 0)
#define IMPN    ((FLAGS) 0x10)  // change new cell (it is unknown)
#define IMPN1   ((FLAGS) 0x20)  // change new cell to 1 (if not set, change it to 0)
#define IMPVOID ((FLAGS) 0x40)  // invalid/unset implication
#define IMPOK   ((FLAGS) 0x80)  // valid state

void initImplicKS(const State * born, const State * live, FLAGS * implic);

#endif /* IMPLICATIONKS_H */
