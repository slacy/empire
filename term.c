/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
term.c -- this file contains various routines used to control the
user communications area of the terminal.  This area consists of
the top 3 lines of the terminal where messages are displayed to the
user and input is acquired from the user.

There are two types of output in this area.  One type is interactive
output.  This consists of a prompt line and an error message line.
The other type of output is informational output.  The user must
be given time to read informational output.

Whenever input is received, the top three lines are cleared and the
screen refreshed as the user has had time to read these lines.  We
also clear the 'need_delay' flag, saying that the user has read the
information on the screen.

When information is to be displayed, if the 'need_delay' flag is set,
we refresh the screen and pause momentarily to give the user a chance
to read the lines.  The new information is then displayed, and the
'need_delay' flag is set.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curses.h>
#include <ctype.h>
#include <stdarg.h>

#include "empire.h"
#include "extern.h"

static bool need_delay;
static FILE *my_stream;

/*
Here are routines that handle printing to the top few lines of the
screen.  'topini' should be called at initialization, and whenever
we finish printing information to the screen.
*/

void
topini(void)
{
    info ("", "", "");
}
/*
Write a message to one of the top lines.
*/

static void vtopmsg(int line, const char *fmt, va_list varglist)
/* assemble command in printf(3) style, print to a top line */
{
    char junkbuf[STRSIZE];
	
    if (line < 1 || line > NUMTOPS)
	line = 1;
    (void) move (line - 1, 0);
    vsnprintf(junkbuf, sizeof(junkbuf), fmt, varglist);
    (void) addstr (junkbuf);
    (void) clrtoeol();
}

void
topmsg(int line, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vtopmsg(line, fmt, ap);
    va_end(ap);
}

/*
Print a prompt on the first message line.
*/

void
prompt(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vtopmsg(1, fmt, ap);
    va_end(ap);
}

/*
Print an error message on the second message line.
*/

void
error(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vtopmsg(2, fmt, ap);
    va_end(ap);
}

/*
Print out extra information.
*/

void
extra(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vtopmsg(3, fmt, ap);
    va_end(ap);
}


/*
Print out a generic error message.
*/

void
huh(void)
{
    error ("Type H for Help.");
}

/*
Display information on the screen.  If the 'need_delay' flag is set,
we force a delay, then print the information.  After we print the
information, we set the need_delay flag.
*/

void
info(char *a, char *b, char *c)
{
    if (need_delay) delay ();
    topmsg (1, a);
    topmsg (2, b);
    topmsg (3, c);
    need_delay = (a || b || c);
}

void
set_need_delay(void)
{
    need_delay = true;
}

void
comment (char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if (need_delay)
	delay ();
    topmsg (1, "");
    topmsg (2, "");
    vtopmsg (3, fmt, ap);
    need_delay = (fmt != 0);
    va_end(ap);
}
	
void
pdebug(char *fmt, ...)
{
    va_list ap;

    if (!print_debug) return;

    va_start(ap, fmt);
    if (need_delay)
	delay ();
    topmsg (1, "");
    topmsg (2, "");
    vtopmsg (3, fmt, ap);
    need_delay = (fmt != 0);
    va_end(ap);
}

/* kermyt begin */

void
vksend(const char *fmt, va_list varglist)
{
    if(!(my_stream=fopen("info_list.txt","a")))
    {
	error("Cannot open info_list.txt");
	return;
    }
    vfprintf(my_stream, fmt, varglist);
    fclose(my_stream);
    return;
}

void
ksend(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vksend(fmt, ap);
    va_end(ap);
}
/* kermyt end */

/*
Get a string from the user, echoing characters all the while.
*/

void
get_str(char *buf, int sizep)
{
    (void) echo();
    get_strq(buf, sizep);
    (void) noecho();
}

/*
Get a string from the user, ignoring the current echo mode.
*/

void
get_strq(char *buf, int sizep)
{
    (void) nocrmode ();
    (void) refresh ();
    (void) getnstr (buf, sizep);
    need_delay = false;
    info ("", "", "");
    (void) crmode ();
}

/*
Get a character from the user and convert it to uppercase.
*/

char
get_chx(void)
{
    char c;

    c = get_cq ();

    if (islower(c))
	return (toupper(c));
    else
	return (c);
}

/*
Input an integer from the user.
*/

int
getint(char *message)
{
    char buf[STRSIZE];
    char *p;

    for (;;) { /* until we get a legal number */
	prompt (message,0,0,0,0,0,0,0,0);
	get_str (buf, sizeof (buf));
		
	for (p = buf; *p; p++) {
	    if (*p < '0' || *p > '9') {
		error ("Please enter an integer.",0,0,0,0,0,0,0,0);
		break;
	    }
	}
	if (*p == 0) { /* no error yet? */
	    if (p - buf > 7) /* too many digits? */
		error ("Please enter a small integer.",0,0,0,0,0,0,0,0);
	    else return (atoi (buf));
	}
    }
}

/*
Input a character from the user with echoing.
*/

char
get_c(void)
{
    char c; /* one char and a null */

    (void) echo ();
    c = get_cq ();
    (void) noecho ();
    return (c);
}

/*
Input a character quietly.
*/

char
get_cq(void)
{
    char c;

    (void) crmode ();
    (void) refresh ();
    c = getch ();
    topini (); /* clear information lines */
    (void) nocrmode ();
    return (c);
}

/*
Input a yes or no response from the user.  We loop until we get
a valid response.  We return true iff the user replies 'y'.
*/

bool
getyn(char *message)
{
    char c;

    for (;;) {
	prompt (message,0,0,0,0,0,0,0,0);
	c = get_chx ();

	if (c == 'Y') return (true);
	if (c == 'N') return (false);

	error ("Please answer Y or N.",0,0,0,0,0,0,0,0);
    }
}

/*
Input an integer in a range.
*/

int
get_range(char *message, int low, int high)
{
    int result;

    for (;;) {
	result = getint (message);

	if (result >= low && result <= high) return (result);

	error ("Please enter an integer in the range %d..%d.",low, high);
    }
}

/*
Print a screen of help information.
*/

void
help(char **text, int nlines)
{
    int i, r, c;
    int text_lines;

    text_lines = (nlines + 1) / 2; /* lines of text */

    clear_screen ();

    pos_str (NUMTOPS, 1, text[0]); /* mode */
    pos_str (NUMTOPS, 41, "See empire(6) for more information.");

    for (i = 1; i < nlines; i++) {
	if (i > text_lines)
	    pos_str (i - text_lines + NUMTOPS + 1, 41, text[i]);
	else pos_str (i + NUMTOPS + 1, 1, text[i]);
    }

    pos_str (text_lines + NUMTOPS + 2,  1, "--Piece---Yours-Enemy-Moves-Hits-Cost");
    pos_str (text_lines + NUMTOPS + 2, 41, "--Piece---Yours-Enemy-Moves-Hits-Cost");

    for (i = 0; i < NUM_OBJECTS; i++) {
	if (i >= (NUM_OBJECTS+1)/2) {
	    r = i - (NUM_OBJECTS+1)/2;
	    c = 41;
	}
	else {
	    r = i;
	    c = 1;
	}
	pos_str (r + text_lines + NUMTOPS + 3, c,"%-12s%c     %c%6d%5d%6d",
		  piece_attr[i].nickname,
		  piece_attr[i].sname,
		  tolower (piece_attr[i].sname),
		  piece_attr[i].speed,
		  piece_attr[i].max_hits,
		  piece_attr[i].build_time,0,0);		//FLAG

    }
    (void) refresh ();
}

#define COL_DIGITS ((MAP_WIDTH <= 100) ? 2 : ((MAP_WIDTH <= 1000 ? 3 : (1 / 0))))

int
loc_disp(int loc)
{
    int row = loc / MAP_WIDTH;
    int nrow = row;
    int col = loc % MAP_WIDTH;
    ASSERT (loc == (row * MAP_WIDTH) + col);
    int i;
    for (i = COL_DIGITS; i > 0; i--) {
	nrow *= 10; }
    move (LINES - 1, 0);
    return nrow + col;
}

/* end */
