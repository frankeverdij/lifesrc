#ifndef ALLOCATECELL_H
#define ALLOCATECELL_H

#include "cell.h"

#define ALLOC_SIZE 50000 /* chunk size for cell allocation */

Cell * allocateCell(void);

#endif /* ALLOCATECELL_H */
