/*
 * Dumb terminal output routine.
 * Does no cursor addressing stuff.
 */

#include <signal.h>
#include <stdarg.h>

#include "lifesrc.h"


static	Bool	inputready;		/* TRUE if input now ready */

static	void	gotinput(int);


/*
 * Open the terminal and enable for detecting terminal input.
 * Returns TRUE if successful.
 */
Bool
ttyopen(void)
{
	signal(SIGINT, gotinput);

	return TRUE;
}


static void
gotinput(int signalNumber)
{
	signal(SIGINT, gotinput);
	inputready = TRUE;
}


/*
 * Close the terminal.
 */
void
ttyclose(void)
{
}


/*
 * Test to see if a keyboard character is ready.
 * Returns nonzero if so (and clears the ready flag).
 */
Bool
ttycheck(void)
{
	Bool	result;

	result = inputready;
	inputready = FALSE;

	return result;
}


/*
 * Print a formatted string to the terminal.
 * The string length is limited to 256 characters.
 */
void
ttyprintf(const char * fmt, ...)
{
	va_list		ap;
	static char	buf[256];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	ttywrite(buf, strlen(buf));
}


/*
 * Print a status message, like printf.
 * The string length is limited to 256 characters.
 */
void
ttystatus(const char * fmt, ...)
{
	va_list		ap;
	static char	buf[256];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	ttywrite(buf, strlen(buf));
}


/*
 * Write the specified number of characters to the terminal.
 */
void
ttywrite(const char * buf, int count)
{
	int	ch;

	while (count-- > 0)
	{
		ch = *buf++;
		putchar(ch);
	}
}


void
ttyhome(void)
{
}


void
ttyeeop(void)
{
}


void
ttyflush(void)
{
	fflush(stdout);
}


/*
 * Return a NULL terminated input line (without the final newline).
 * The specified string is printed as a prompt.
 * Returns TRUE on successful read, or FALSE (with an empty buffer)
 * on end of file or error.
 */
Bool
ttyread(const char * prompt, char * buf, int buflen)
{
	int	len;

	fputs(prompt, stdout);
	fflush(stdout);

	if (fgets(buf, buflen, stdin) == NULL)
	{
		buf[0] = '\0';

		return FALSE;
	}

	len = strlen(buf) - 1;

	if ((len >= 0) && (buf[len] == '\n'))
		buf[len] = '\0';

	return TRUE;
}

/* END CODE */
