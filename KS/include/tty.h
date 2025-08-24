#ifndef TTY_H
#define TTY_H

#include "bool.h"

extern    Bool    ttyopen(void);
extern    Bool    ttycheck(void);
extern    Bool    ttyread(const char *, char *, int);
extern    void    ttyprintf(const char *, ...);
extern    void    ttystatus(const char *, ...);
extern    void    ttywrite(const char *, int);
extern    void    ttyhome(void);
extern    void    ttyeeop(void);
extern    void    ttyflush(void);
extern    void    ttyclose(void);

#endif /* TTY_H */
