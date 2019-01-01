/*****************************************************************************
 *						  	MARS
 *				   Memory Array Redcode Simulator
 *
 *	IBM PC/XT/AT Version 2.1G by Mazoft Software (DK)
 *	Copyright (C) 1988, 1989 all rights reserved
 *
 * Denne source er skrevet i standard K&R (Classic) C. ANSI-style
 * protoyper er ikke brugt - du f†r en masse warnings...
 *
 *	Syntax: MARS <warriorA> <warriorB> (<flags> ...)
 *
 *	Switch	/c controls core memory size (default 4K, max 32K)
 *			/s controls signature of battle (for instant replay)
 *			/t controls timeout before draw (no winner)
 *			/d is the debug switch (autoselects /n too)
 *			/n no graphics
 *			/e controls the execution speed (defaults to 0 = fastest)
 *			/g override graphics board autoselect
 *			/p parallel execution (defaults to OFF)
 *			/8 use CoreWar 88 standard execution
 *
 *	MARS - Deus Ex Machina
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <io.h>
#include <graphics.h>

#define CENTRE_TEXT CENTER_TEXT	/* Dem Yankees can't spell! */
#define FSPLEN 16		/* File Specification maximum name length */
#define OPCODES 11		/* Number of operation codes & pseudo instructions */
#define UNKNOWN 11		/* If recognize() returns UNKNOWN, it's not listed */
#define BADEXP 0x7FFF	/* bad expression */
#define SYMSIZ 0x40		/* default symbol table stack space */
#define MAXPRGSIZ 0x200	/* default maximum redcode program size */
#define CRESIZ 0x1000	/* default core size */
#define DEFTIME 5000	/* 5000 instructions * 2 before timeout */
#define IMMEDIATE 0		/* addressing modes (two-bit bitfield) */
#define DIRECT 1
#define INDIRECT 2
#define AUTODECREMENT 3
#define ALL 1			/* flags for internal addressing */
#define BTOB 2
#define ATOB 4
#define MAXSPL 64		/* max spawned tasks */
#define CR putchar ('\n')

#define repeat do
#define until(a) while (!(a))
#define hex(i) (char)((i)+((i) > 9 ? '7' : '0'))

char *stdext = ".mrs";		/* standard MARS object code filename extension */
char *copyright = "MARS System 2.0G - (C) 1988,1989 Mazoft Software (DK)\n";
char *syntaxstr = "Syntax:\tMARS <warrior A> <warrior B> (<flags> ...)\nor:\tMARS -? for help";
char *addrmode = "#$@<?";

enum opcodes { DAT, MOV, ADD, SUB, JMP, JMZ, JMN, DJN, CMP, SPL };

char opcode [16] [4] = {     /* List of legal REDCODE TLA-opcodes */
     "DAT", "MOV", "ADD", "SUB", "JMP", "JMZ",
     "JMN", "DJN", "CMP", "SPL", "???"
     };

struct instructionword {
	unsigned int word0;
	int word1, word2;
	} *core;			/* start of core */

char checkerboard [8] = {
	 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
	 };

int pc [2] [MAXSPL], coresize;				/* 2 * 64 pointers into the core */
int task [2], tasks [2];						/* number of active tasks */
unsigned long spread [2];
int lowl = 0, highl = 0;						/* occupation range */
int gmin, gmax;							/* screen window limits */
int pixelsize, xpels, ypels, maxx, maxy, xsize, ysize;
int maxcolor, runflag = 0, gstat = 0;
unsigned int a, mode, debug;					/* addressing mode */
char wname [2] [13], worksp [81];
int dies [2];
int graphmode = 0, graphdriver = DETECT;		/* the default */

gprintf(int x, int y, char *fmt, ...);

main (argc, argv)
int argc;
char **argv;
{
	char c, *cp;
	int i, j, k, l, m, f, qrt;
	int newadr, p, winner, loser, fileindex [2], fi = 0;
	int graphflag = 1, parflag = 1;
	int timer = DEFTIME, delay = 0, qflag, choice;
	long int signature = 0, pcycles = 0;

	time (&signature);			/* unique to every run */
	lowl = highl = debug = 0;
	coresize = CRESIZ;			/* default core size */
	i = 2 + 1;			/* 1 command + N 2 */

	clrscr ();
	centre ("-- MARS --");
	centre ("Memory Array Redcode Simulator");
	centre ("Version 2.1G (24-Apr-89)");
	CR;
	centre ("(C) Maz Spork 1988, 1989");
	CR;

	for (l = 1; l < argc; l++) {

		if (argv [l] [0] == '?') {
			argv [l] [0] = '-';
			argv [l] [1] = '?';
			}

		if (argv [l] [0] == '/' || argv [l] [0] == '-') {	/* switch identifiers */
			c = tolower (argv [l] [1]);
			k = argv [l] [2] ? 2 : 0;

			switch (c) {
				case 'c':
					if (!k) l++;
					coresize = atoi (&argv [l] [k]);
					break;
				case 's':
					if (!k) l++;
					signature = (long) atoi (&argv [l] [k]);
					break;
				case 't':
					if (!k) l++;
					timer = (long) atoi (&argv [l] [k]);
					break;
				case 'd':
					debug = 1;
				case 'n':
					graphflag = 0;
					break;
				case	'e':
					if (!k) l++;
					delay = atoi (&argv [l] [k]);
					break;
				case 'w':
					puts ("\nThis is a revision of the original Core Wars MARS system 1.0 by Maz Spork.");
					puts ("It is placed in the Public Domain for your benefit, and may be used or abused");
					puts ("in any way you like. If the source code is included with the assembler/simula-");
					puts ("tor, you are free to modify and enhance to your hearts content PROVIDED that");
					puts ("you NEVER charge ANY money for the software. It should NOT be sold, neither");
					puts ("in its original or in any modified form.");
					puts ("If you really like this software, and feel the urge to give some reward to the");
					puts ("brilliant programmer who wrote it, donate a generous amout of money to AIDS");
					puts ("research so we'll all survive at the end of the day.");
					puts ("If you would like information about future releases of this software, please");
					puts ("write to");
					puts ("\n\tMaz Spork, Howitzvej 21 (flat 2), DK-2000 Frederiksberg\n");
					puts ("This REDCODE/MARS sytem is compatible with what I think will be the ICWS'");
					puts ("CoreWar'88 standard - though the voting hasn't finished yet. It also employs");
					puts ("a better parallel mode, which I hope will be the standard one day. Please");
					puts ("refer to the manual for more information.");
					exit (20);
				case 'p':
					parflag = 0;
					break;
				case '8':
					puts ("Can't use CoreWar'88 standard - ICWS standards focal hasn't finished it.");
					break;
				case 'g':
					if (!k) l++;
					graphdriver = atoi (&argv [l] [k]);
					if (graphdriver == 11) graphflag = 0;
					break;
				default:
					printf ("Bad switch option %c, here's the help screen instead:\n\n", c);
				case '?':
					printf ("    Switches:\n\tC : Size of core battlefield (max 32767) (default %d)\n", CRESIZ);
					puts ("\tD : Debug mode (autoselects option N) (default off)");
					puts ("\tE : Execution speed delay roughly in 100us intervals (default 0)");
					puts ("\tN : No Graphics (default off)");
					puts ("\tP : Modified ICWS'88 mode for REAL parallel task execution");
					puts ("\tS : Signature (for identical replays) (default random)");
					printf ("\tT : Start timer (default %d)\n", DEFTIME);
					puts ("\tW : Rights and licences");
					puts ("\t8 : ICWS Standards Focal's ICWS'88 standard");
					puts ("\tG : Override auto-select on graphics board:");
					puts ("\t     0: auto-detect                  1: Color Graphics Adapter (CGA)");
					puts ("\t     2: Multi-Color Graphics (MCGA)  3: Enhanced Graphics Adapter (EGA)");
					puts ("\t     4: EGA (64 colors)              5: EGA (Monochrome)");
					puts ("\t     6: IBM 8514 graphics            7: Hercules Monochrome Graphics");
					puts ("\t     8: AT&T 400 graphics            9: Very Graphics Adapter (VGA)");
					puts ("\t    10: PC3270 graphics             11: No graphics (also option N)");
					exit (20);
					}
			}
		else {		/* not a switch */
			fileindex [fi++] = l;
			}
		}

	if (fi != 2) {				/* wrong no. of parameters */
		puts (syntaxstr);
		exit (20);
		}

	if (coresize < 0) {
		printf ("Core size request too big - max 32767\n");
		exit (20);
		}

	CR;	/* The following 20 lines of code is awful, but it works ! */
	sprintf (worksp, "     Battle Overview : %s versus %s     ", argv [*fileindex], argv [fileindex [1]]);
	centre (worksp);
	for (cp = worksp; *cp; *cp++) *cp = '-';
	centre (worksp);
	CR;
	sprintf (worksp, "Coresize: %u locations", coresize);
	centre (worksp);
	if (timer)
		sprintf (worksp, "Timeout counter starts at %u", timer);
	else
		sprintf (worksp, "There is no timeout");
	centre (worksp);
	sprintf (worksp, "Debug mode is %s", debug ? "on" : "off");
	centre (worksp);
	if (delay)
		sprintf (worksp, "Execution delay is %u * 100us per cycle", delay);
	else
		sprintf (worksp, "There is no execution delay");
	centre (worksp);
	sprintf (worksp, "Parallelized task execution mode is %s", parflag ? "off" : "on");
	centre (worksp);
	sprintf (worksp, "Warrior #1 (%s) has colour ±", argv [1]);
	centre (worksp);
	sprintf (worksp, "Warrior #2 (%s) has colour Û", argv [2]);
	centre (worksp);
	sprintf (worksp, "Offcore memory has colour °");
	centre (worksp);
	sprintf (worksp, "Unoccupied memory is coloured black");
	centre (worksp);

	CR;
	centre ("Press a key to start the battle");
	while (!kbhit()); getch (); CR;

	clrscr ();
	gstat = graphflag ? ginit (coresize) : 0;	/* requested no-graphics mode ? */

	if (gstat < 0) {
		gclose ();
		puts ("Graphic Resolution too small for requested core representation.");
		gstat = 0;
		}

	if (!gstat) {
		wait ("Battle will commence without visual spectation (no graphics!)\n");
		CR;
		}

	srand ((unsigned int) signature);

	if (!(core = (struct instructionword *) malloc (sizeof (struct instructionword) * coresize))) {
		gclose ();
		printf ("Core memory could not be allocated (try a smaller coresize - switch c)");
		exit (20);
		}

	memset (core, 0, sizeof (struct instructionword) * coresize);	/* clear core */

	for (i = 0; i < 2; i++) {
		pc [i] [0] = loadwarrior (argv [fileindex [i]], wname [i]);
		if (gstat) {
			gprintf (0, 1 + 16 * i, "%s", wname [i]);
			splitbar (1, i);
			}
		}

	*tasks = tasks [1] = 1;	/* both start with 1 task each */
	*task = task [1] = 0;		/* current tasks are both #0	 */
	*dies = dies [1] = *spread = spread [1] = 0;
	if (!timer) timer--;

goagain:

	if (gstat)					/* flash all program counters */
		for (i = 0; i < 10; i++) {
			for (k = 0; k < 2; k++)
				for (j = 0; j < tasks [k]; j++)
					gsetloc (pc [k] [j], i & 1 ? k : 3);
			for (j = 0; j < 0x7FFF; j++);
			}
	else CR;

	runflag = 1;

	if (parflag) {					/* for non-parallel mode */
		while ((*tasks && tasks [1]) && timer && !kbhit ()) {
			if (--timer < 0) timer = -1;
			for (p = 0; p < 2; p++) {
				if ((newadr = execute (p, pc [p] [task [p]])) == -1) {	/* garbage collection */
					tasks [p]--;
					dies [p]++;
					for (i = task [p]; i < tasks [p] - 1; i++)
						pc [p] [i] = pc [p] [i + 1];
					splitbar (tasks [p], p);
					}
				else {
					pc [p] [task [p]] = newadr;
					task [p]++;
					}
				if (task [p] == tasks [p]) task [p] = 0;
				}
			if (!gstat) printf ("%s tasks %2d, %s tasks %2d, timer = %5d%c", *wname, *tasks, wname [1], tasks [1], timer, debug ? '\n' : 13);
			for (i = delay; i; i--);
			pcycles++;
			}
		}
	else {                             /* for parallel mode */
		while ((*tasks && tasks [1]) && timer && !kbhit ()) {
			if (--timer < 0) timer = -1;
			for (p = 0; p < 2; p++) {
				for (k = tasks [p], task [p] = j = i = 0; i < k; i++) {   /* the tasks */
					if ((newadr = execute (p, pc [p] [j])) == -1) {
						tasks [p]--;
						dies [p]++;
						for (l = j; l < tasks [p]; l++)
							pc [p] [l] = pc [p] [l + 1];
						splitbar (tasks [p], p);
						}
					else pc [p] [j++] = newadr;
					task [p]++;
					}
				}
			if (!gstat) printf ("%s tasks %2d, %s tasks %2d, timer = %5d%c", *wname, *tasks, wname [1], tasks [1], timer, debug ? '\n' : 13);
			for (i = delay; i; i--);
			pcycles++;
			}
		}

	choice = runflag = 0;

	if (qflag = kbhit ()) {		/* Key was pressed */
		getch ();				/* Remove key */

		if (gstat) {
			m = getgraphmode ();
			restorecrtmode ();
			}

		do {
			clrscr ();
			centre ("-- EXECUTION SEQUENCE HALTED --");
			CR; CR;
			puts ("* Battle Status:");
			for (i = 0; i < 2; i++)
				printf ("   %s has %d task%s and a spread factor of %lu.\n",
						wname [i], tasks [i], tasks [i] == 1 ? "":"s", spread [i]);
			if (timer != -1)
				printf ("   The timer stands at %u, interrupted after", timer);
			else
				printf ("   The timer is inactive, interrupted after");
			printf (" %lu cycles of execution.\n\n", pcycles);
			puts ("* Choices of action:-\n");
			puts ("\t0. Stop battle (no winner)");
			puts ("\t1. Disassemble memory");
			puts ("\t2. List warriors");
			puts ("\t3. Battle analysis");
			puts ("\t4. Resume battle\n");

			do {
				printf ("\015Choice: ");
				putchar (choice = getch ());
				} while (choice < '0' || choice > '4');

			choice -= '0';

			switch (choice) {
				case 1:
					disassemble_warriors (1);
					break;
				case 2:				/* Examine warriors */
					list_warriors (1);
					break;
				case 3:				/* analyze the game */
					clrscr ();
					centre ("Battle Analysis");
					CR; CR;
					for (i = 0; i < 2; i++) {
						printf ("Warrior #%d, \"%s\":\n", i, wname [i]);
						if (qrt = dies [i] + tasks [i] - 1) {
							printf ("  Regrenerates on average every %d cycles.\n", pcycles / qrt);
							printf ("  Of %d new child task%s, ", qrt, qrt==1?"":"s");
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

					for (j = 0; j < *tasks; j++) {
						for (f = k = 0; k < tasks [1]; k++)
							if (narrow (pc [0] [j], pc [1] [k])) f++;
						if (f) printf ("%s and %s probably shares a program at location %X\n", *wname, wname [1], pc [0] [j]);
						}

					wait (0);
					break;
				case 4:				/* resume execution */
					if (gstat) {
						setgraphmode (m);
						gdrawborder (coresize);
						for (i = 0; i < 2; i++) {
							gprintf (0, 1 + 16 * i, "%s", wname [i]);
							for (j = 0; j < tasks [i]; j++)
								splitbar (j + 1, i);
							}
						}
					goto goagain;
				}
			} while (choice);
		}

	loser  = *tasks ? 1 : 0;	/* loser = !*tasks */
	winner = !loser;

	if (!qflag && gstat && timer != 0) /* flash loser's last program counter */
		for (i = 0; i < 10; i++)
			for (k = 0; k < 2; k++) { /* 0=on, 1=off */
				gsetloc (pc [loser] [0], k ? loser : 3);
				for (j = 0; j < 0x7FFF; j++);
				}

	gclose();	/* back to normal */

	do {
		clrscr ();
		CR;
		centre ("-- POST MORTEM --");
		sprintf (worksp, "\"%s\" versus \"%s\"", *wname, wname [1]);
		centre (worksp);
		CR;
		if (qflag) {
			printf ("* The battle was interrupted externally after %lu cycles of execution.\n", pcycles);
			puts ("  MARS voted the game a draw.");
			}
		else	if (!timer) {
			puts ("* Timer exhausted. MARS interrupted the battle and voted the game a draw.");
			printf ("  At this point, after %lu cycles,\n", pcycles);
			printf ("   %s had %d task%s running,\n", *wname, *tasks, *tasks == 1 ? "":"s");
			printf ("   %s had %d task%s running.\n", wname [1], tasks [1], tasks [1] == 1 ? "":"s");
			}
		else if (!*tasks && !tasks [1]) {
			puts ("* Both warriors executed an undefined instruction at the same clock cycle.");
			puts ("  MARS voted the game a draw.");
			}
		else {
			puts ("* Victory!");
			printf ("   %s wins the battle after %lu cycles of execution.\n", wname [winner], pcycles );
			printf ("   %s executed an undefined instruction at location %X\n", wname [1 - winner], pc [1 - winner] [0]);
			printf ("   %s had at that time %d active task%c\n", wname [winner], tasks [winner], tasks [winner] == 1 ? ' ' : 's');
			}

		printf              ("\n\t\tSpread Factor      Reproductions      Deaths\n");
		for (i = 0; i < 2; i++)
			printf ("%s:      \t   %6lu             %6u         %6u\n", wname [i], spread [i], dies [i] + tasks [i] - 1, dies [i]);

		printf ("\nChoices of action:-\n\n   0. Exit MARS\n   1. Disassemble core\n   2. List surviving warriors\n\n");

		do {
			putchar (13);
			printf ("Select: ");
			i = getch ();
			} while (i < '0' || i > '2');

		putchar (i);
		if (i == '1') disassemble_warriors (0);
		else if (i == '2') list_warriors (0);
		} while (i != '0');

	printf ("\n\nBattle signature: %d (for identical replay)\n", signature);

	free (core);	/* Get rid of this */
	}


/* list all active warrior's current execution locations (not finished)
 */

list_warriors (int rm) {
	int i, j, k, rmw1 = 1, rmw2 = 1;

	clrscr ();
	sort (0);			/* sort the PCs */
	sort (1);
	centre ("Warrior Task List"); CR;
	strcpy (worksp, "Task:           >               >       ");
	strcpyx (&worksp [16], *wname);
	strcpyx (&worksp [32], wname [1]);
	puts (worksp);
	puts ("---------       --------        --------");
	i = *tasks, j = tasks [1], k = 0;

	if (!rm) {		/* if one is completely dead */
		if (!i) { i = 1; rmw1 = 0; }
		if (!j) { j = 1; rmw2 = 0; }
		}

	while (i | j) {
		printf ("Task #%02d :\t", k++);
		if (i) printf ("@ %4X %s\t", pc [0] [--i], rmw1?"      ":"(dead)");
		  else printf ("\t\t");
		if (j) printf ("@ %4X %s", pc [1] [--j], rmw2?"      ":"(dead)");
		CR;
		}
	CR;
	wait (0);
	}

/* Generic disassembly of redcode instructions
 */

disassemble_warriors (int rm) {
	clrscr ();
	centre ("Redcode Disassembler and core memory dump");
	printf ("\nAddress (hex): ");
	scanf ("%x", &a);
	disassemble (a, rm);
	}

sort (int player) {			/* Sort all warriors in address order */
	int i, t, flag;

	if (tasks [player] < 2) return;
	do {
		for (flag = 0, i = 0; i < tasks [player] - 1; i++) {
			if (pc [player] [i] < pc [player] [i + 1]) {
				t = pc [player] [i];
				pc [player] [i] = pc [player] [i + 1];
				pc [player] [i + 1] = t;
				flag = 1;
				}
			}
		} while (flag);
	}

strcpyx (char *s1, char *s2) { /* like strcpy but the zero terminator is not copied */
	while (*s2) *s1++ = *s2++;
	}

int wait (char *s) {	/* wait ("message") or wait (NULL) */
	if (s) puts (s);
	printf ("Press any key to continue...");
	while (!kbhit ());
	getch ();
	}

int execute (int player, int location) { /* returns 0 if dead, otherwise new location */
	int ea1, ea2, m1, newpc = location, a;

	ea1 = effadr (location, 1, player); m1 = mode;	/* a-field effective address */
	ea2 = effadr (location, 2, player);			/* b-field effective address */

	if (debug) printf ("  %s(%X) @ %X: %X %X %X : EA1 = %X (%X,%X), EA2 = %X (%X,%X)\n",
					wname [player], task [player], location, core [location].word0, core [location].word1, core [location].word2,
					ea1, core [ea1].word1, core [ea1].word2,ea2, core [ea2].word1, core [ea2].word2);

	switch (instruction (&core [location])) {
		case (MOV):		/* move data from source to destination */
			move (&core [ea1], &core [ea2], m1 == IMMEDIATE ? ATOB : ALL);
			gsetloc (ea2, player);
			newpc++;		/* go to next location */
			break;
		case (ADD):
			add (&core [ea1], &core [ea2], m1 == IMMEDIATE ? ATOB : BTOB);
			gsetloc (ea2, player);
			newpc++;		/* go to next location */
			break;
		case (SUB):
			subtract (&core [ea1], &core [ea2], m1 == IMMEDIATE ? ATOB : BTOB);
			gsetloc (ea2, player);
			newpc++;		/* go to next location */
			break;
		case (JMP):		/* use effective address */
			newpc = ea1;
			break;
		case (JMZ):		/* only if b-field is 0 */
			if (core [ea2].word2)
				newpc++;
			else
				newpc = ea1;
			break;
		case (JMN):		/* not if b-field is 0 */
			if (!core [ea2].word2)
				newpc++;
			else
				newpc = ea1;
			break;
		case (DJN):		/* decrement b-field and use ea if not zero */
			if (--core [ea2].word2)
				newpc = ea1;
			else
				newpc++;
			gsetloc (ea2, player);
			break;
		case (CMP):		/* increment pc if comparison equal */
			if (compare (&core [ea1], &core [ea2], m1 == IMMEDIATE ? ATOB : ALL)) newpc++;
			newpc++;
			break;
		case (SPL):		/* one more task */
			if (tasks [player] < MAXSPL)
				pc [player] [tasks [player]++] = ea2;
			else
				dies [player]++;
			newpc++;
			splitbar (tasks [player], player);
			break;
		default:			/* undefined instruction - task dies */
			return (-1);
		}

	return (inrange (newpc));
	}

int inrange (int val) {	/* modulus supports negative numbers */
	return (((val % coresize) + (val < 0 ? coresize : 0)));
	}

move (source, destination, movemode)	/* move data */
struct instructionword *source, *destination;
int movemode;
{
	switch (movemode) {	/* check which move */
		case (ALL):	/* whole instruction word */
			destination -> word0 = source -> word0;
			destination -> word1 = source -> word1;
			destination -> word2 = source -> word2;
			break;
		case (BTOB):	/* b-field to b-field */
			destination -> word0 &= 0xF0FF;
			destination -> word0 |= source -> word0 & 0x0F00;
			destination -> word2 = source -> word2;
			break;
		case (ATOB):	/* a-field to b-field */
			destination -> word0 &= 0xF0FF;
			destination -> word0 |= (source -> word0 & 0xF000) / 0x10;
			destination -> word2 = source -> word1;
		}
	}

add (source, destination, addmode)	/* add data */
struct instructionword *source, *destination;
int addmode;
{
	destination -> word2 += (addmode == ATOB ? source -> word1 : source -> word2);
	}

subtract (source, destination, submode)	/* subtract data */
struct instructionword *source, *destination;
int submode;
{
	destination -> word2 -= (submode == ATOB ? source -> word1 : source -> word2);
	}

int compare (insa, insb, cmpmode)
struct instructionword *insa, *insb;
int cmpmode;
{
	return (cmpmode == ATOB ? (insa -> word1 == insb -> word2) : ((insa -> word0 == insb -> word0) && (insa -> word1 == insb -> word1) && (insa -> word2 == insb -> word2)));
	}

int effadr (location, operand, p)	/* effective address operand 1 or 2 */
int location, operand, p;
{
	int value, address, temp;

	if (operand == 1) {	/* extract relevant information from instruction word */
		value = core [location].word1;
		mode = (core [location].word0 & 0xF000) / 0x1000;
		}
	else {
		value = core [location].word2;
		mode = (core [location].word0 & 0x0F00) / 0x100;
		}

	switch (mode & 3) {		/* select effective address from addressing mode and operand */
		case (IMMEDIATE):
			address = location;
			break;
		case (DIRECT):
			address = location + value;
			break;
		case (INDIRECT):
			temp = inrange (location + value);
			address = core [temp].word2 + temp;
			break;
		case (AUTODECREMENT):
			temp = inrange (location + value);
			core [temp].word2--;
			gsetloc (temp, p);
			address = core [temp].word2 + temp;
			break;
		}

	return (inrange (address));
	}

int instruction (inswrd)
struct instructionword *inswrd;
{
	return ((inswrd -> word0) & 0xFF);
	}

int loadwarrior (text, namedest)	/* ret's PC for this warrior */
char *text, *namedest;
{
	int handle, i;
	int prog [MAXPRGSIZ * (sizeof (struct instructionword) / sizeof (int))];
	int t, loc, len, initpc, zpos, *p = prog;
	char name [13];
	static int hilo = 0, lastlen;

	zpos = finddot (text, name);

	if ((handle = _open (name, O_RDONLY)) == -1) {
		gclose ();
		printf ("\nBut \"%s\" is not present on the disk...\n", name);
		exit (20);
		}

	for (i = 0; i < zpos; i++) namedest [i] = toupper (name [i]);
	namedest [zpos] = name [zpos] = '\0';

	if ((len = _read (handle, prog, MAXPRGSIZ * sizeof (struct instructionword))) <= 0) {
		gclose ();
		printf ("\n\nWould not read %s, error %d - exiting\n", name, len);
		exit (20);
		}

	len = (len - sizeof (int)) / sizeof (struct instructionword);	/* minus start indicator, divided by instructionword's length */
	_close (handle);	/* and that's all we need */

	if (hilo) {
		t = 0;
		if (len + lastlen > coresize) {
			gclose ();
			puts ("Not enough room in core for the two warriors - exiting");
			exit (20);
			}
		do {			/* find an unoccupied spot (tries 1024 times) */
			while ((loc = rand () % coresize) <= highl && loc + len >= lowl) if (++t > 1024) {
				gclose ();
				printf ("Could not locate %s (try larger core)\n", name);
				exit (20);
				}
			} while ((highl >= coresize && loc <= highl % coresize) || (loc + len - 1>= coresize && loc + len >= lowl));
		}
	else loc = rand () % coresize;

	initpc = ((lowl = loc) + *p++) % coresize;	/* starting location relative to physical start */
	highl = lowl + len - 1;					/* inclusive last location so minus 1 */

	for (t = len; t; t--) {
		core [loc].word0 = *p++;			/* Instruction word A */
		core [loc].word1 = *p++;			/* Instruction word B */
		core [loc].word2 = *p++;			/* Instruction word C */
		gsetloc (loc, hilo);	/* indicate occupation on memory map */
		loc++; loc %= coresize;
		}

	if (!gstat)
		printf ("\"%s\" located from &%X to &%X (&%X bytes)\n", name, lowl, highl % coresize, len);

	hilo++;
	lastlen = len;
	return (initpc);
	}

int finddot (name1, name2)
char *name1, *name2;
{
	int i;
	char c;

	for (i = 0; (*name2 = c = *name1++) && c != '.'; i++) name2++;
	if (c)
		strcpy (name2, name1 - 1);
	else
		strcpy (name2, stdext);	/* add extension to filename */

	return (i);
	}

/* Centre string on screen */

centre (char *text) {
	int i;
	const screenwidth = 80;

	for (i = (screenwidth - strlen (text)) >> 1; i; i--) putchar (' ');
	puts (text);
	}

int sqr (int x) {
	int i;
	for (i = 1; i < 200; i++) if (i * i > x) break;	/* OK, it's bad */
	return (i - 1);
	}

int ginit (int request) {
	int i;

	initgraph (&graphdriver, &graphmode, "");
	if (graphresult () < 0) return (0);

	xsize = getmaxx () - 3;
	ysize = getmaxy () - 32 - 3;
	maxcolor = getmaxcolor ();

	pixelsize = 128;

	while (pixelsize && (xpels = xsize / pixelsize) * (ysize / pixelsize) < request)
		pixelsize--;

	if (pixelsize < 3) return (-1);

	ypels = (request / xpels) + (request % xpels != 0);

	maxx = xpels * pixelsize;
	maxy = ypels * pixelsize;

	gstat = 1;
	gdrawborder (request);

/*	settextjustify (CENTRE_TEXT, TOP_TEXT);
	settextstyle (TRIPLEX_FONT, HORIZ_DIR, 3);
	outtextxy (getmaxx () / 2, 0, "Core Wars Battlefield");
*/
	settextstyle (DEFAULT_FONT, HORIZ_DIR, 1);
	settextjustify (LEFT_TEXT, TOP_TEXT);

	return (gstat);	/* success */
	}

gdrawborder (int req) {
	while (req % xpels) gsetloc (req++, 2);
	rectangle (0, 32, maxx + 2, 32 + maxy + 2);
	}

gplot (x, y, colour, p)	/* colour is 0 or 1, p is size */
int x, y, colour, p;
{
	switch (colour) {
		case 0: 	setfillpattern (checkerboard, maxcolor);	/* on-off cross-hatch */
				break;
		case 1: 	setfillstyle (SOLID_FILL, maxcolor);
				break;
		case 2: 	setfillstyle (CLOSE_DOT_FILL, maxcolor);
				break;
		case 3: 	setfillstyle (EMPTY_FILL, BLACK);
				break;
		}
	bar (x, y, x + p, y + p);
	}

gsetloc (loc, player)
int loc, player;
{
	int xcoord, ycoord;

	if (runflag && player < 2) spread [player]++;
	if (!gstat) return;		/* && player < 2 ??? */

	xcoord = (loc % xpels) * pixelsize + 2;
	ycoord = (loc / xpels) * pixelsize + 34;
	gplot (xcoord, ycoord, player, pixelsize - 2);
	}

gclose () {
	if (gstat) closegraph ();
	}

gprintf(int x, int y, char *fmt, ...) {
	va_list argptr;		
	char str [140];

	if (gstat) {   
		va_start (argptr, fmt);	   
		vsprintf (str, fmt, argptr);
		outtextxy (x, y, str);		 
		va_end (argptr);
		}
	}

splitbar (int value, int p) {
	int x, y, deltax;

	if (!gstat) return;

	if (p)
		setfillstyle (SOLID_FILL, maxcolor);
	else
		setfillpattern (checkerboard, maxcolor);

	deltax = (getmaxx() - 64) / 64;
	x = 64 + deltax * value;
	y = p * 16;

	if (value) bar (x, y, x + deltax - 2 , y + 8);

	setfillstyle (EMPTY_FILL, BLACK);
	bar (x + deltax, y, x + 2 * deltax - 2, y + 8);
	}

cleargrid () {
	if (!gstat) return;
	setfillstyle (EMPTY_FILL, BLACK);
	bar (2, 34, maxx - 2, 32 + maxy - 2);
	}

int disassemble (unsigned address, int rm) {
	int i, j, f, iw, m1, m2, o1, o2, c, rm2, df;
	int movin = 1;
	struct instructionword *caddr;

	caddr = &core [address % coresize];

	while (movin) {
		iw = caddr->word0 & 0xFF;
		if (iw > 9) iw = 10;
		i = caddr->word0 >> 8;
		m2 = i & 0x0F;
		m1 = (i >> 4) & 0x0F;
		if (m1 > 3) m1 = 4;
		if (m2 > 3) m2 = 4;
		o1 = caddr->word1;
		o2 = caddr->word2;

		itoh (address, 4, 2);
		itoh (iw, 1, 1);
		itoh (m1, 1, 101);
		itoh (o1, 4, 1);
		itoh (m2, 1, 101);
		itoh (o2, 4, 3);

		for (df = f = 0, j = 0; j < 2; j++) {
			rm2 = tasks [j];
			if (!rm) rm2 = 1;
			for (i = 0; i < rm2; i++) {
				if (pc [j] [i] == address) {
					if (f) printf ("\n\t\t\t");
					strcpy (worksp, "              ");
					if (tasks [j])
						sprintf (&worksp [24], "%s(%d):", wname [j], i);
					else {
						sprintf (&worksp [24], "%s(*):", wname [j]);
						df = 1;
						}
					strcpyx (worksp, &worksp [24]);
					printf (worksp);
					f = 1;
					}
				}
			}

		if (!f) printf ("              ");
		printf ("%s ", opcode [iw]);
		if (iw != DAT && iw != SPL) {
			diw (m1, o1, address);
			if (iw != JMP) putchar (',');
			}
		if (iw != JMP) diw (m2, o2, address);

		if (df && (!tasks [0] || !tasks [1]))
			printf ("\t<- This is where it died");

		CR;
		address++;
		caddr++;

		if (address == coresize) {
			address = 0;
			caddr = core;
			}

		if (kbhit ()) {
			getch ();
			printf ("Stop/Continue? ");
			c = getche ();
			putchar (13);
			if (c == 'S' || c == 's') movin = 0;
			}
		}
	}

diw (m, v, l)
int m, v, l;
{
	if (m != 1) putchar (addrmode [m]);	/* direct addressing mode assumed */
	printf ("%d", v);

/*
	if (m)
		printf (" (%X)", coresize % (l + v));
	else
		printf ("       ");
*/
	}

itoh (value, digits, spaces)      /* fast hex-print + trailing spaces*/
unsigned int value, digits, spaces;
{
	char c = ' ';

	while (digits--) putchar (hex (value / (1 << (digits << 2)) & 0xF));

     if (spaces > 100) {     /* if 10x is provided, use dashes instead */
          spaces -= 100;
          c = '-';
          }

	while (spaces--) putchar (c);		/* What shall we do with all the empty spaces which we used to fill ... */
	  }

int narrow (int a, int b) {	/* boundary checking */
	const c = 4;
	return (a + c >= b && a - c <= b);
	}

/* End of MARS.C */
