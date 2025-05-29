#ifndef TTY_H
#define TTY_H

#include "enums.h"

extern    Bool    ttyOpen(void);
extern    Bool    ttyCheck(void);
extern    Bool    ttyRead(const char *, char *, int);
extern    void    ttyPrintf(const char *, ...);
extern    void    ttyStatus(const char *, ...);
extern    void    ttyWrite(const char *, int);
extern    void    ttyHome(void);
extern    void    ttyEEop(void);
extern    void    ttyFlush(void);
extern    void    ttyClose(void);

#endif /* TTY_H */
