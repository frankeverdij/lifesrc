#include <stdio.h>
#include <ctype.h>

/*
 * Bool type
 */
typedef	int		Bool;

#define	FALSE		((Bool) 0)
#define	TRUE		((Bool) 1)

/*
 * Read a number from a string, eating any leading or trailing blanks.
 * Returns the value, and indirectly updates the string pointer.
 * Returns specified default if no number was found.
 */
long getNum(const char ** cpp, int defnum)
{
    const char * cp;
    long num;
    Bool isNeg;

    isNeg = FALSE;
    cp = *cpp;

    while (isblank(*cp))
        cp++;

    if (*cp == '-')
    {
        cp++;
        isNeg = TRUE;
    }

    if (!isdigit(*cp))
    {
        *cpp = cp;

        return defnum;
    }

    num = 0;

    while (isdigit(*cp))
        num = num * 10 + (*cp++ - '0');

    if (isNeg)
        num = -num;

    while (isblank(*cp))
        cp++;

    *cpp = cp;

    return num;
}
