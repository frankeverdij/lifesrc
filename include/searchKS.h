#ifndef SEARCHKS_H
#define SEARCHKS_H

#include "enums.h"
#include "state.h"

Status searchKS(const Bool);
Bool proceedKS(Cell *, State, Bool);
Bool setCellKS(Cell * const , const State, const Bool);
Cell * backupKS(void);

#endif /* SEARCHKS_H */
