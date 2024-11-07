#ifndef WRITEGLOBALS_H
#define WRITEGLOBALS_H

#include "globals.h"
#include "state.h"

int readGlobals(char *, Status, long, globals * const);
void writeGlobals(FILE *, const Status, const long, const globals * const);

#endif /* WRITEGLOBALS_H */
