 /*************************************************************************************************************\
 *									MARS 																										   *
 *				   Memory Array Redcode Simulator																					   *
 *																																				   *
 *		IBM PC/XT/AT and PS/2 Version 3.0G by Mazoft Software (DK)															   *
 *		Copyright (C) 1988, 1990. All rights reserved. ! GAMMA VERSION ! 													   *
 *																																				   *
 *		Syntax: MARS <warriorA> <warriorB> (<flags> ...)																		   *
 *																																				   *
 *		Switch	/c controls core memory size (default 4K, max 32K)															   *
 *					/s controls signature of battle (for instant replay)														   *
 *					/t controls timeout before draw (no winner)																	   *
 *					/d is the debug switch (autoselects /n too)																	   *
 *					/n no graphics 																										   *
 *					/e controls the execution speed (defaults to 0 = fastest)												   *
 *					/g override graphics board autoselect																			   *
 *					/p parallel execution (defaults to OFF) 																		   *
 *					/x number of SPLits allowed																						   *
 *					/w rights and licences																									*
 *					/l lines in disassembly window																						*
 *             /r number of runs                                                                               *
 *                                                                                                             *
 *		Must link with the following object files:                                                               *
 *                                                                                                             *
 *			REDCODE.OBJ   MARSUTIL.OBJ   OPTIONS.OBJ                                                              *

To Do:

a) Flag, der fort‘ller, at vi ikke vil have edit-menuen
	- resettes hvis der g†s tilbage !
b) Indirektion i disassembleren - g†r til b-feltets adresse !
	- relativt fra PC'en.

 *																																				   *
 \*************************************************************************************************************/

#include <stdio.h>		// for printf(), scanf() and sprintf()
#include <string.h>		// for strcpy(), strncpy(), strcmp()
#include <ctype.h>		// for isupper(), isascii() etc...
#include <graphics.h>	// Turbo C++ graphics templates
#include <conio.h>		// keyboard routines
#include <process.h>		// back-to-dos routines
#include <dos.h>			// ms-dos calls
#include <stdlib.h>		// standard library (eg. rand())
#include <time.h>			// to set random seed
#include "options.h"    // option class
#include "mars.h"			// global mars defines

#define plural(i) ((i)==1?"":"s")

//////////////// externals ////////////////

extern void cleargrid ();
extern void gclose ();
extern void gdisins (int, int, int);
extern void gdrawborder (int);
extern void gplot (int, int, int, int);
extern void gprtf (int, int, char *, ...);
extern void gsetloc (int, int);
extern void outchar (char, int, int);
extern void splitbar (int, int);
extern int ginit (unsigned int);

extern int graphdriver;
extern int graphmode;
extern void centre (char *);
extern void disassemble (unsigned, int);
extern void diw (int, int);
extern void graphdis ();
extern void disassemble_warriors (int);
extern void list_warriors (int);
extern void sort (int);
extern void strcpyx (char *, char *);
extern void wait (char *);
extern char *itoh (unsigned, unsigned, unsigned);
extern location inrange (int);
extern int narrow (int, int);

//////////////// prototypes /////////////////

void display_header (void);
void display_switches (void);
void display_licence (void);
int get_choice (int);

//////////////// globals ////////////////

char
	addrmode [6] = "#$@<?",
	checkerboard [8] = {
		0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
		},
	*copyright = "MARS System 3.0 - (C) 1988,1990 Mazoft Software (DK)\n",
	opcode [16] [4] = {     // List of legal REDCODE TLA-opcodes
		"DAT", "MOV", "ADD", "SUB", "JMP", "JMZ", "JMN", "DJN",
		"CMP", "SPL", "???", "???", "???", "???", "???", "???"
		},
	stdext [5] = ".mrs",
	*syntaxstr = "Syntax:\tMARS <warrior A> <warrior B> (<flags> ...)\nor:\tMARS -? for help",
	wname [2] [13],
	tname [2] [40] = { "Phobos", "Deimos" },
	worksp [81];

int
	active = 0,				// active player
	delayval = 0,			// default delay after each cycle of execution
	dblines = 4,			// default lines in debug window
	dies [2],				// vector for counting dead programs
	gmax,						// pixel values for x
	gmin,						// and y
	graphflag = 1,			// graphics on/off flag
	gstat = 0,				// graphics status (FALSE if no graphics)
	itimer = DEFTIME,		// default timer value
	maxcolor,				// highest (brightest) colour available
	maxx,						// no. of pixels in x direction
	maxy,						// in y direction
	multruns = DEFRUN,	// number of runs
	otasks [2],				// save task numbers here
	parflag = 0,			// parallel mode flag
	pc [2] [MAXSPL],		// vector of program counters
	repro [2],				// number of reproductions for each warrior
	runflag = 0,			// TRUE if game is running
	isignature = 0,		// Signature (0 = random)
	splits = 64,			// default number of SPLits allowed
	task [2],				// vector of current task numbers
	tasks [2],				// vector of total number of tasks
	totaldies [2],			// total number of deaths through multiple runs
	wins [2],				// number of wins for each warrior
	xsize,					// size of graphics window
	ysize;					// size of graphics window

unsigned int
	coresize = CRESIZ,	// size of core arena battlefield
	debug = 0,				// TRUE if extensive debug info is to be shown
	pixelsize,				// size of a picture element
	xpels,					// no. of pels in x direction
	ypels;					// no. of pels in y

unsigned long
	spread [2],				// number of memory writes for each player
	totalspread [2];		// total number of memory writes through multiple runs

options marsoptions [13] = {	// default values that might want to be changed

//	  Type      X    Y  Field text                       X  Y   Address       Description of field (help text)
//	  --------- --  --  ------------------------------- --  --  ------------  --------------------------------
	{ O_STRING, 20,  7, "Warrior A:",                   31,  7, &tname[0][0], "Name of the first redcode warrior" },
	{ O_STRING, 20,  8, "Warrior B:",                   31,  8, &tname[1][0], "Name of the second redcode warrior" },
	{ O_INT,	   17, 10, "Size of core:",                31, 10, &coresize,    "Number of locations (adresses) in the core" },
	{ O_INT,	   12, 11, "Timer start value:",           31, 11, &itimer,      "The initial value of the timeout counter" },
	{ O_INT,	    5, 12, "Number of SPLits allowed:",    31, 12, &splits,      "The maximum allowed number of tasks for each warrior" },
	{ O_INT,	    2, 13, "Lines in disassembly window:", 31, 13, &dblines,     "The vertical height of the window used for disassembling the core" },
	{ O_INT,	   13, 14, "Battle signature:",            31, 14, &isignature,  "This number allows you to review a battle identically" },
	{ O_INT,	   14, 15, "Battle iterator:",             31, 15, &multruns,    "The number of times the two warriors are to battle" },
	{ O_INT,	    6, 16, "Execution delay (in ms):",     31, 16, &delayval,    "Slowing the game down allows you to examine the events more closely" },
	{ O_FLAG,    6, 18, "Graphics",                      3, 18, &graphflag,   "This flag enables or disables graphics for the battle" },
	{ O_FLAG,    6, 19, "Parallel execution mode",       3, 19, &parflag,     "Invoke a special parallel mode for execution of SPLit warriors" },
	{ O_FLAG,    6, 20, "Debug mode",                    3, 20, &debug,       "Run MARS in debug mode (not recommended)" },
	{ O_END }
	};

redcode_program
	arena;				// the actual core

//////////////// function bodies ////////////////

int main (int argc, char **argv) {
	char c, *cp, errmsg [80];
	int i, j, k, l, m, n, f, t, newadr, p, winner, loser, runs, signature,
		fi = 0, qrt, qflag, choice, disloc, olddisloc, draw, timer;
	long pcycles;
	unsigned long calunits, calsize;
	location temploc;
	time_t timeparam;

	for (l = 1; l < argc; l++) {

		if (argv [l] [0] == '?') {
			argv [l] [0] = '-';
			argv [l] [1] = '?';
			}

		if (argv [l] [0] == '/' || argv [l] [0] == '-') { // switch identifiers
			c = tolower (argv [l] [1]);		// get switch
			k = 2;
			if (!argv [l] [2]) {		// if the user put
				l++;						// a space between the
				k = 0;					// switch and its value
				}
			t = atoi (&argv [l] [k]);

			switch (c) {
				case 'c': coresize = (unsigned) t; break;
				case 's': isignature = t; break;
				case 't': itimer = t; break;
				case 'd': debug = 1;
				case 'n': graphflag = 0; break;
				case 'e': delayval = t; break;
				case 'p': parflag = 1; break;
				case 'r': multruns = t; break;
				case 'x': splits = t; break;
				case 'g': graphdriver = t; break;
				case 'l': dblines = t; break;
				case 'w':
					clrscr ();
					display_header ();
					display_licence ();
					exit (10);
				case '?':
				default:
					clrscr ();
					display_header ();
					display_switches ();
					exit (10);
				}
			}
		else {				// not a switch - must be a warrior name
			strcpy (tname [fi++], argv [l]);
			}
		}

	do {	// the "run program again" loop
	clrscr ();

	opt_screen choices (marsoptions);	// use option screen class
	strcpy (errmsg, "");

retry1:

	display_header ();

retry2:

	textattr (BRITE);
	gotoxy (1, 24);
	cprintf (errmsg);

	for (i = 0; i < (78 - wherex ()); i++) putchar (' ');

	textattr (NOBRITE);
	gotoxy (1,25);
	printf ("Use cursor keys to move between fields, RETURN to continue or ESC to abort.");

	choices.print ();								// print choices for mars-options
	while ((i = choices.modify ()) > 0);	// do modifications until done

	if (i == -1) {
		clrscr ();
		exit (10);
		}

	if (coresize > MAXCS) {
		choices.select (2);
		sprintf (errmsg, "Error 1: Core size request too big - maximum %u", MAXCS);
		goto retry2;
		}

	if (splits > MAXSPL) {
		choices.select (4);
		sprintf (errmsg, "Error 2: Maximum number of SPLits is %d", MAXSPL);
		goto retry2;
		}

	if (dblines < 4 || dblines > 16) {
		choices.select (5);
		sprintf (errmsg, "Error 3: The graphics disassembly window must be in the range 4 to 16");
		goto retry2;
		}

	if (!multruns) {
		choices.select (7);
		sprintf (errmsg, "Error 4: You must specify at least one battle iteration");
		goto retry2;
		}

	clrscr ();

	if (graphdriver > 10) graphflag = 0;
	gstat = graphflag ? ginit (coresize) : 0;	// requested no-graphics mode ?

	if (gstat < 0) {
		gclose ();
		printf ("Graphic Resolution is too small for the requested core representation, thus\n");
		gstat = 0;
		}

	if (!gstat) {
		_setcursortype (_NOCURSOR);
		wait ("The battle will be fought without bit-mapped graphics\n");
		clrscr ();
		}

	time (&timeparam);
	signature = isignature ? isignature : (unsigned) timeparam;
	srand ((unsigned) signature);

	arena.initialize (coresize);	// allocate memory for core representation

	wins [0] = wins [1] = 0;		// nobody has won yet
	totaldies [0] = totaldies [1] = 0;
	totalspread [0] = totalspread [1] = 0;
	runs = multruns;					// set flag if extd statistics is needed

	while (runs--) {

		arena.clear (BLACKOUT);			// nobody owns anything yet

		otasks [0] = otasks [1] = 0;	// saves vector to spot differences
		tasks [0] = tasks [1] = 1;		// each warrior starts off with 1 task
		timer = itimer;					// initialize timer value
		if (!timer) timer--;				// a negative timer is infinite
		dies [0]	= dies [1] = 0;		// nobody has died yet
		spread [0] = spread [1] = 0;	// no memory writes yet
		task [0]	= task [1] = 0;		// current tasks are both #0
		pcycles = 0;						// counts execution cycles

		for (i = 0; i < 2; i++) {

			if ((pc [i] [0] = arena.enter_warrior (tname [i], wname [i], i)) < 0) {
				arena.cleanup ();
				gclose ();
				choices.select (i);

				switch (pc [i] [0]) {
					case -1:
						sprintf (errmsg, "Error 5: Warrior \"%s\" not found on disk", wname [i]);
						break;
					case -2:
						sprintf (errmsg, "Error 6: Unable to read from %s", wname [i]);
						break;
					case -3:
					case -4:
						choices.select (2);
						sprintf (errmsg, "Error 7: Not enough room in core for the two warriors");
						break;
					}
				goto retry1;
				}

			if (gstat) {
				gprtf (0, 1 + 16 * i, "%8s:", wname [i]);
				splitbar (1, i);
				}
			}

		reenter:

		if (gstat)				// flash all program counters
			for (i = 0; i < 10 && !kbhit (); i++) {
				for (k = 0; k < 2; k++)
					for (j = 0; j < tasks [k]; j++) {
						gsetloc (pc [k] [j], i & 1 ? k : BLACKOUT);
						gsetloc (pc [k] [j], i & 1 ? PCON : PCOFF);
						}
				delay (250);
				}

		runflag = 1;

		if (!parflag) {				// running in "normal" mode
			while ((*tasks && tasks [1]) && timer && !kbhit ()) {
				if (--timer < 0) timer = -1;
				for (p = 0; p < 2; p++) {
					if ((newadr = arena.execute (p, pc [p] [task [p]])) == -1) { // garbage collection
						gsetloc (pc [p] [task [p]], PCOFF);
						tasks [p]--;
						dies [p]++;
						for (i = task [p]; i < tasks [p]; i++)
							pc [p] [i] = pc [p] [i + 1];
						splitbar (tasks [p], p);
						}
					else {
						gsetloc (pc [p] [task [p]], PCOFF);
						gsetloc (newadr, PCON);
						pc [p] [task [p]++] = newadr;
						}
					if (task [p] >= tasks [p]) task [p] = 0;
					}

				if (!gstat)
					if (debug)
						printf ("%s tasks %d, %s tasks %d, timer = %5d\n", wname [0], tasks [0], wname [1], tasks [1], timer);
					else {
						if (otasks [0] != tasks [0] || otasks [1] != tasks [1]) {
							printf ("\x0D""Battle #%3d: %s tasks %3d, %s tasks %3d", multruns - runs, wname [0], tasks [0], wname [1], tasks [1]);
							otasks [0] = tasks [0];
							otasks [1] = tasks [1];
							}
						}

				delay (delayval);
				pcycles++;
				}
			}
		else {					// running in "parallel" mode
			while ((*tasks && tasks [1]) && timer && !kbhit ()) {
				if (--timer < 0) timer = -1;
				for (p = 0; p < 2; p++) {
					for (k = tasks [p], task [p] = j = i = 0; i < k; i++) {   // the tasks
						active = pc [p] [j];
						if ((newadr = arena.execute (p, pc [p] [j])) == -1) {
							gsetloc (pc [p] [j], PCOFF);
							tasks [p]--;
							dies [p]++;
							for (l = j; l < tasks [p]; l++)
								pc [p] [l] = pc [p] [l + 1];
							splitbar (tasks [p], p);
							}
						else {
							gsetloc (pc [p] [j], PCOFF);
							gsetloc (newadr, PCON);
							pc [p] [j++] = newadr;
							}
						task [p]++;
						}
					}

				if (!gstat)
					if (debug)
						printf ("%s tasks %d, %s tasks %d, timer = %5d\n", wname [0], tasks [0], wname [1], tasks [1], timer);
					else {
						if (otasks [0] != tasks [0] || otasks [1] != tasks [1]) {
							printf ("\x0D""Battle #%3d: %s tasks %3d, %s tasks %3d", multruns - runs, wname [0], tasks [0], wname [1], tasks [1]);
							otasks [0] = tasks [0];
							otasks [1] = tasks [1];
							}
						}

				delay (delayval);
				pcycles++;
				}
			}

		choice = runflag = 0;

		if (qflag = kbhit ()) {
			if (tolower (getch()) == 'd' && gstat) {
				graphdis ();
				for (i = 0; i < 2; i++) {
					for (j = 0; j < tasks [i]; j++)
						splitbar (j + 1, i);
					gprtf(0, 1 + 16 * i, "%8s:", wname [i]);
					}
				goto reenter;
				}

			if (gstat) {
				m = getgraphmode ();
				restorecrtmode ();
				}

			do {
				clrscr ();
				_setcursortype (_NORMALCURSOR);
				textattr (BRITE);
				centre ("-- EXECUTION SEQUENCE HALTED --");
				textattr (NOBRITE);
				CR; CR;
				puts ("* Battle Status:");
				if (multruns > 1) {
					printf ("  Of %d battles, this is number %d. So far,\n", multruns, multruns - runs);
					for (i = 0; i < 2; i++)
						printf ("    %s has won %d battle%s,\n", wname [i], wins [i], plural (wins [i]));
					i = (multruns - runs) - wins [0] - wins [1] - 1;
					printf ("    %d time%s the two fought to a draw.\n", i, plural (i));
					printf ("\n  In the current battle (number %d),\n", multruns - runs);
					}
				for (i = 0; i < 2; i++)
					printf ("    %s has %d task%s and a spread factor of %lu.\n",
							wname [i], tasks [i], plural (tasks [i]), spread [i]);
				if (timer != -1)
					printf ("    The timer stands at %u, interrupted after", timer);
				else
					printf ("    The timer is inactive, interrupted after");
				printf (" %lu cycles of execution.\n\n", pcycles);
				puts ("* Choices of action:-");
				puts ("    0. Terminate the battle (no winner)");
				puts ("    1. Disassemble the core");
				puts ("    2. List the warriors' positions in the core");
				puts ("    3. Battle analysis");
				puts ("    4. Resume the battle\n");

				_setcursortype (_NORMALCURSOR);

				switch (choice = get_choice (4)) {
					case 0:
						runs = 0;
						break;
					case 1:
						disassemble_warriors (1);
						break;
					case 2:				// Examine warriors
						list_warriors (1);
						break;
					case 3:				// analyze the game
						clrscr ();
						textattr (BRITE);
						centre ("Battle Analysis");
						textattr (NOBRITE);
						CR; CR;
						for (i = 0; i < 2; i++) {
							printf ("Warrior #%d, \"%s\":\n", i, wname [i]);
							if (qrt = dies [i] + tasks [i] - 1) {
								printf ("  Regenerates on average every %d cycles.\n", pcycles / qrt);
								printf ("  Of %d new child task%s, ", qrt, plural (qrt));
								if (dies [i]) printf ("%d ha%s not survived.\n", dies [i], dies [i] == 1 ? "s":"ve");
								else printf ("all have survived so far.\n");
								}
							else puts ("  No child tasks have been spawned");

							for (j = 0; j < tasks [i]; j++) {
								for (f = k = 0; k < j; k++)
									if (narrow (pc [i] [j], pc [i] [k])) f++;
								if (!f) {
									for (k = j + 1; k < tasks [i]; k++)
										if (narrow (pc [i] [j], pc [i] [k])) f++;
									if (f) printf ("  There are %d warriors running suspiciously close around location %X\n", f + 1, pc [i] [j]);
									}
								}
							CR;
							}

						for (j = 0; j < tasks [0]; j++) {
							for (f = k = 0; k < tasks [1]; k++)
								if (narrow (pc [0] [j], pc [1] [k])) f++;
							if (f) printf ("%s and %s probably shares a program at location %X\n", wname [0], wname [1], pc [0] [j]);
							}

						wait (0);
						break;
					case 4:				// resume execution
						_setcursortype (_NOCURSOR);

						if (gstat) {
							setgraphmode (m);
							gdrawborder (coresize);

							for (i = 0; i < coresize; i++)
								if (arena.owner (i) != 3)
									gsetloc (i, arena.owner (i));

							for (i = 0; i < 2; i++) {
								setcolor (maxcolor);
								gprtf(0, 1 + 16 * i, "%8s:", wname [i]);
								for (j = 0; j < tasks [i]; j++)
									splitbar (j + 1, i);
								}
							}
						else clrscr ();
						otasks [0] = otasks [1] = MAXSPL + 1;
						goto reenter;
						}
				} while (choice);	// choice 0 terminates loop
			}

		loser  = tasks [0] ? 1 : 0;
		winner = tasks [1] ? 1 : 0;
		draw = ((tasks [0] == tasks [1]) || (timer == 0));

		if (!qflag) {
			if (!gstat) {
				if (draw)
					printf ("\tDraw.\n");
				else
					printf ("\t%s wins.\n", wname [winner]);
				for (i = 0; i < (multruns == 1 ? 20 : 1) && !kbhit (); i++) delay (250);
				}
			else {
				setfillstyle (EMPTY_FILL, BLACK);
				setcolor (maxcolor);

				if (!draw) {
					for (i = 0; i < 2; i++) {
						if (!tasks [i]) {
							bar (64, i * 16, getmaxx (), i * 16 + 15);
							gprtf (72, i * 16 + 1, "has lost. %s", (multruns == 1) ? "Press D to disassemble core or any other key to see statistics" : "");
							}
						}

					for (i = 0; i < (multruns == 1 ? 200 : 5) && !kbhit (); i++)
						for (j = 0; j < 2; j++) {
							delay (250);
							for (k = 0; k < 2; k++)
								if (!tasks [k])
									gsetloc (pc [k] [0], j ? k : BLACKOUT);
							}

					if (kbhit ()) if ((tolower (getch ()) == 'd') && gstat) graphdis ();
					}
				else {	// timer == 0
					bar (64, 0, getmaxx (), 31);
					gprtf (72, 9, (tasks [0] == 0 && tasks [1] == 0) ?
						"The battle came to a draw due to both warriors dying simultaneously." :
						"The battle came to a draw due to timer exhaustion.");
					for (i = 0; i < (multruns == 1 ? 400 : 10) && !kbhit (); i++) delay (250);
					}
				}
			}

		for (i = 0; i < 2; i++) {
			totalspread [i] += spread [i];
			totaldies [i] += dies [i];
			}
		if (!draw) wins [winner]++;

		if (gstat) {
			m = getgraphmode ();
			restorecrtmode ();
			if (runs) {
				setgraphmode (m);
				gdrawborder (coresize);
				}
			}
		}	// number of runs

	gclose();	// back to normal

	do {
		clrscr ();
		_setcursortype (_NORMALCURSOR);
		CR;
		textattr (BRITE);
		centre ("-- POST MORTEM --");
		sprintf (worksp, "\"%s\" versus \"%s\"", wname [0], wname [1]);
		centre (worksp);
		textattr (NOBRITE);
		CR;

		if (multruns > 1) {
			printf ("* After a sequence of %d battles,\n", multruns);
			for (i = 0; i < 2; i++)
				printf ("    %s won %d battle%s,\n", wname [i], wins [i], plural (wins [i]));
			i = multruns - wins [0] - wins [1];
			printf ("    %d time%s the two fought to a draw.\n", i, plural (i));
			printf ("\nTotals:   \tSpread Factor      Reproductions      Deaths\n");
			for (i = 0; i < 2; i++)
				printf ("%s:      \t   %6lu             %6u         %6u\n", wname [i], totalspread [i], repro [i], totaldies [i]);

			puts ("\n* Choices of action:-");
			puts ("    0. Exit MARS");
			puts ("    1. Run MARS again\n");

			if ((choice = get_choice (1)) == 1) choice = 3;

			}
		else {
			if (qflag) {
				printf ("* The battle was interrupted externally after %lu cycles of execution.\n", pcycles);
				printf ("  MARS voted the game ");
				textattr (BRITE);
				cprintf ("a draw.\r\n");
				textattr (NOBRITE);
				}
			else if (!timer) {
				printf ("* Timer exhausted. MARS interrupted the battle and voted the game ");
				textattr (BRITE);
				cprintf ("a draw.\r\n");
				textattr (NOBRITE);
				printf ("  At this point, after %lu cycles,\n", pcycles);
				printf ("    %s had %d task%s running,\n", wname [0], tasks [0], plural (tasks [0]));
				printf ("    %s had %d task%s running.\n", wname [1], tasks [1], plural (tasks [1]));
				}
			else if (!tasks [0] && !tasks [1]) {
				puts ("* Both warriors executed an undefined instruction at the same clock cycle.");
				printf ("  MARS voted the game ");
				textattr (BRITE);
				cprintf ("a draw.\r\n");
				textattr (NOBRITE);
				}
			else {
				puts ("* Victory!");
				textattr (BRITE);
				cprintf ("    %s wins", wname [winner]);
				textattr (NOBRITE);
				printf (" the battle after %lu cycles of execution.\n", pcycles );
				printf ("    %s executed an undefined instruction at location %X\n", wname [1 - winner], pc [1 - winner] [0]);
				printf ("    %s had at that time %d active task%c\n", wname [winner], tasks [winner], tasks [winner] == 1 ? ' ' : 's');
				}

			printf ("\n\t\tSpread Factor      Reproductions      Deaths\n");
			for (i = 0; i < 2; i++)
				printf ("%s:      \t   %6lu             %6u         %6u\n", wname [i], spread [i], repro [i], dies [i]);

			printf ("\nBattle signature: %u\n", signature);

			puts ("\n* Choices of action:-");
			puts ("    0. Exit MARS");
			puts ("    1. Disassemble the remains in the core");
			puts ("    2. List surviving warriors");
			puts ("    3. Run MARS again\n");

			choice = get_choice (3);

			if (choice == 1) disassemble_warriors (1);
				else if (choice == 2) list_warriors (0);
			}
		} while (choice && choice != 3);

	arena.cleanup ();
	argc = 0;
	} while (choice);

	if (wins [0] == wins [1]) return 0;				// came to a draw
		else if (wins [0] > wins [1])	return 1;	// came to a player 1 win
			else return 2;									// came to a player 2 win
	}

void display_header (void) {
	textattr (BRITE);
	centre ("-- MARS --");
	centre ("Memory Array Redcode Simulator");
	textattr (NOBRITE);
	centre ("Version 3.0 (16-Aug-90)");
	CR;
	centre ("(C) Maz Spork 1988, 1990");
	CR;
	}

void display_switches (void) {
	printf ("Switches:\n");
	printf ("C : Size of core battlefield            D : Debug mode (autoselects opt N)\n");
	printf ("    (max %5d) (default %5d)        E : Execution speed delay in milli-\n", MAXCS, CRESIZ);
	printf ("S : Signature for identical replays         seconds.\n");
	printf ("    (defaults to random value)          P : Modified execution mode for REAL\n");
	printf ("X : Number of SPLits allowed            	 parallel execution\n");
	printf ("    (defaults to 64)                    T : Timer start value (default %d)\n", DEFTIME);
	printf ("L : Number of lines in the graphics     W : Display licence information\n");
	printf ("    disassembly window                  N : No Graphics (defaults to ON)\n");
	printf ("G : Override graphics auto-selection:\n");
	printf ("     1: Color Graphics Adapter (CGA)    2: Multi-Color Graphics (MCGA)\n");
	printf ("     3: Enhanced Graphics Adapter (EGA) 4: EGA (64 colours)\n");
	printf ("     5: EGA (Monochrome)                6: IBM 8514 graphics\n");
	printf ("     7: Hercules Monochrome Graphics    8: AT&T 400 graphics\n");
	printf ("     9: Video Graphics Array (VGA)     10: PC3270 graphics\n\n");
	printf ("During the game, press D to enter the in-core disassembler");
	}

void display_licence (void) {
	puts ("\nThis is a revision of the original Core Wars MARS system 1.0 by Maz Spork.");
	puts ("It is placed in the Public Domain for your benefit, and may be used or abused");
	puts ("in any way you like. If the source code is included with the assembler/simula-");
	puts ("tor, you are free to modify and enhance to your hearts content PROVIDED that");
	puts ("you NEVER charge ANY money for the software. It should NOT be sold, neither");
	puts ("in its original or in any modified form.");
	puts ("If you really like this software, and feel the urge to give some reward to the");
	puts ("brilliant programmer who wrote it, donate a generous amount of money to AIDS");
	puts ("research so that we will all survive at the end of the day.");
	puts ("If you would like information about future releases of this software, please");
	puts ("write to");
	puts ("\tMaz Spork, Howitzvej 21 (flat 2), DK-2000 Frederiksberg");
	puts ("This REDCODE/MARS sytem is compatible with what I think will be the ICWS'");
	puts ("CoreWar'88 standard - though the voting hasn't finished yet. It also employs");
	puts ("a better parallel mode, which I hope will be the standard one day. Please");
	puts ("refer to the manual for more information.");
	}

int get_choice (int limit) {
	int choice = -1;
	while (choice < 0 || choice > limit) {
		printf ("\x0D""Choice: ");
		choice = getche () - '0';
		}
	return choice;
	}

