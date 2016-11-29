/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
empire.c -- this file contains initialization code, the main command
parser, and the simple commands.
*/

#include <stdio.h>
#include "empire.h"
#include "extern.h"

void c_examine(void), c_movie(void);

/*
 * 03a 01Apr88 aml .Hacked movement algorithms for computer.
 * 02b 01Jun87 aml .First round of bug fixes.
 * 02a 01Jan87 aml .Translated to C.
 * 01b 27May85 cal .Fixed round number update bug. Made truename simple.
 * 01a 01Sep83 cal .Taken from a Decus tape
 */

void
empire(void)
{
    void do_command(char);
    void print_zoom();

    char order;
    int turn = 0;

    ttinit (); /* init tty */
    rndini (); /* init random number generator */

    clear_screen (); /* nothing on screen */
    pos_str (7, 0, "EMPIRE, Version 5.00 site Amdahl 1-Apr-1988");
    pos_str (8, 0, "Detailed directions are in EMPIRE.DOC\n");
    (void) redisplay ();

    if (!restore_game ()) /* try to restore previous game */
	init_game (); /* otherwise init a new game */

    /* Command loop starts here. */

    for (;;) { /* until user quits */
	if (automove) { /* don't ask for cmd in auto mode */
	    user_move ();
	    comp_move (1);
	    if (++turn % save_interval == 0)
		save_game ();
	}
	else {
	    prompt (""); /* blank top line */
	    redisplay();
	    prompt ("Your orders? ");
	    order = get_chx (); /* get a command */
	    do_command (order);
	}
    }
}

/*
Execute a command.
*/

void
do_command(char orders)
{
    void c_debug(char order), c_quit(void), c_sector(void), c_map(void);
    void c_give(void);

    char e;
    int ncycle;

    switch (orders) {
    case 'A': /* turn on auto move mode */
	automove = true;
	error ("Now in Auto-Mode");
	user_move ();
	comp_move (1);
	save_game ();
	break;

    case 'C': /* give a city to the computer */
	c_give ();
	break;
	
    case 'D': /* display round number */
	error ("Round #%d", date);
	break;

    case 'E': /* examine enemy map */
	if (resigned) c_examine ();
	else huh (); /* illegal command */
	break;

    case 'F': /* print map to file */
	c_map ();
	break;

    case 'G': /* give one free enemy move */
	comp_move (1);
	break;

    case 'H': /* help */
	help (help_cmd, cmd_lines);
	break;

    case 'J': /* edit mode */
	ncycle = cur_sector ();
	if (ncycle == -1) ncycle = 0;
	edit (sector_loc (ncycle));
	break;

    case 'M': /* move */
	user_move ();
	comp_move (1);
	save_game ();
	break;

    case 'N': /* give enemy free moves */
	ncycle = getint ("Number of free enemy moves: ");
	comp_move (ncycle);
	save_game ();
	break;

    case 'P': /* print a sector */
	c_sector ();
	break;

    case '\026': /* some interrupt */
    case 'Q': /* quit */
	c_quit ();
	break;

    case 'R': /* restore game */
	clear_screen ();
	e = restore_game ();
	break;

    case 'S': /* save game */
	save_game ();
	break;
	
    case 'T': /* trace: toggle save_movie flag */
	save_movie = !save_movie;
	if (save_movie) comment ("Saving movie screens to 'empmovie.dat'.");
	else comment ("No longer saving movie screens.");
	break;

    case 'W': /* watch movie */
	if (resigned || debug) replay_movie ();
	else error ("You cannot watch movie until computer resigns.");
	break;

    case 'Z': /* print compressed map */
	print_zoom (user_map);
	break;

    case '\014': /* redraw the screen */
	redraw ();
	break;

    case '+': /* change debug state */
	e = get_chx();
	if ( e  ==  '+' )
	    debug = true;
	else if ( e  ==  '-' )
	    debug = false;
	else huh ();
	break;

    default:
	if (debug)
	    c_debug (orders); /* debug */
	else
	    huh (); /* illegal command */
	break;
    }
}

/*
Give an unowned city (if any) to the computer.  We make
a list of unowned cities, choose one at random, and mark
it as the computers.
*/

void
c_give(void)
{
    int unowned[NUM_CITY];
    count_t i, count;

    count = 0; /* nothing in list yet */
    for (i = 0; i < NUM_CITY; i++) {
	if (city[i].owner == UNOWNED) {
	    unowned[count] = i; /* remember this city */
	    count += 1;
	}
    }
    if (count == 0) {
	error ("There are no unowned cities.");
	ksend ("There are no unowned cities.");
	return;
    }
    i = irand (count);
    i = unowned[i]; /* get city index */
    city[i].owner = COMP;
    city[i].prod = NOPIECE;
    city[i].work = 0;
    scan (comp_map, city[i].loc);
}

/*
Debugging commands should be implemented here.  
The order cannot be any legal command.
*/

void
c_debug(char order)
{
    char e;

    switch (order) {
    case '#' : c_examine (); break;
    case '%' : c_movie (); break;
	
    case '@': /* change trace state */
	e = get_chx();
	if ( e  ==  '+' )
	    trace_pmap = true;
	else if ( e  ==  '-' )
	    trace_pmap = false;
	else
	    huh ();
	break;

    case '$': /* change print_debug state */
	e = get_chx();
	if ( e  ==  '+' )
	    print_debug = true;
	else if ( e  ==  '-' )
	    print_debug = false;
	else
	    huh ();
	break;

    case '&': /* change print_vmap state */
	print_vmap = get_chx();
	break;

    default: huh (); break;
    }
}

/*
The quit command.  Make sure the user really wants to quit.
*/

void
c_quit(void)
{
    if (getyn ("QUIT - Are you sure? ")) {
	empend ();
    }
}

/*
Print a sector.  Read the sector number from the user
and print it.
*/

void
c_sector(void)
{
    int num;

    num = get_range ("Sector number? ", 0, NUM_SECTORS-1);
    print_sector_u (num);
}

/*
Print the map to a file.  We ask for a filename, attempt to open the
file, and if successful, print out the user's information to the file.
We print the map sideways to make it easier for the user to print
out the map.
*/

void
c_map(void)
{
    FILE *f;
    int i, j;
    char line[MAP_HEIGHT+2];

    prompt ("Filename? ");
    get_str (jnkbuf, STRSIZE);

    f = fopen (jnkbuf, "w");
    if (f == NULL) {
	error ("I can't open that file.");
	return;
    }
    for (i = 0; i < MAP_WIDTH; i++) { /* for each column */
	for (j = MAP_HEIGHT-1; j >= 0; j--) { /* for each row */
	    line[MAP_HEIGHT-1-j] = user_map[row_col_loc(j,i)].contents;
	}
	j = MAP_HEIGHT-1;
	while (j >= 0 && line[j] == ' ') /* scan off trailing blanks */
	    j -= 1;
			
	line[++j] = '\n';
	line[++j] = 0; /* trailing null */
	(void) fputs (line, f);
    }
    (void) fclose (f);
}

/*
Allow user to examine the computer's map.
*/

void
c_examine(void)
{
    int num;

    num = get_range ("Sector number? ", 0, NUM_SECTORS-1);
    print_sector_c (num);
}

/*
We give the computer lots of free moves and
Print a "zoomed" version of the computer's map.
*/

void
c_movie(void)
{
    for (;;) {
	comp_move (1);
	print_zoom (comp_map);
	save_game ();
#ifdef PROFILE
	if (date == 125) empend();
#endif
    }
}

/* end */
