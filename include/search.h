#ifndef SEARCH_H
#define SEARCH_H

#include "enums.h"
#include "state.h"

Status search(const Bool);
Bool proceed(Cell *, State, Bool);
Bool setCell(Cell * const , const State, const Bool);
Cell * backup(void);

#endif /* SEARCH_H */
