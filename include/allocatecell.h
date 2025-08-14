#ifndef ALLOCATECELL_H
#define ALLOCATECELL_H

#include "cell.h"
#include "enums.h"

#define ALLOC_SIZE 50000 /* chunk size for cell allocation */

Cell * allocateCell(const Bool);

#endif /* ALLOCATECELL_H */
