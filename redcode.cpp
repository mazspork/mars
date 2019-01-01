#include <stdio.h>		// for printf(), scanf() and sprintf()
#include <string.h>		// for strcpy(), strncpy(), strcmp()
#include <ctype.h>		// for isupper(), isascii() etc...
#include <fcntl.h>		// for file access flags
#include <io.h>		// for read() and write()
#include <process.h>	// back-to-dos routines
#include <dos.h>		// ms-dos calls
#include <alloc.h>		// allocation
#include <stdlib.h>		// standard library (eg. rand())
#include <time.h>		// to set random seed
#include "mars.h"

// prototypes

extern void splitbar (int, int);
extern void gsetloc (int, int);
extern location inrange (int);

// external variables

extern location coresize;
extern int gstat;
extern int pc [2] [MAXSPL];
extern int task [2];
extern int dies [2];
extern int tasks [2];
extern int splits;
extern int repro [2];
extern int debug;
extern char *stdext;

uint redcode_program::opcode (location l) {
	return (uint) inst_field [l].opcode;
	}

uint redcode_program::modeA (location l) {
	return (uint) inst_field [l].modeA;
	}

uint redcode_program::modeB (location l) {
	return (uint) inst_field [l].modeB;
	}

int redcode_program::operandA (location l) {
	return A_field [l];
	}

int redcode_program::operandB (location l) {
	return B_field [l];
	}

int redcode_program::owner (location l) {
	return own [l];
	}

void redcode_program::owner (location l, int i) {
	own [l] = i;
	}

uint redcode_program::writer (location l) {
	return write [l];
	}

void redcode_program::writer (location l, location m) {
	write [l] = m;
	}

void redcode_program::setowwr (location l, int p) {
	if (p == MARS)
		write [l] = MARS;
	else {
		own [l] = p;
		write [l] = pc [p] [task [p]];
		}
	}

void redcode_program::clear (location l, int who) {
	inst_field [l].opcode = 0;
	inst_field [l].modeA = 0;
	inst_field [l].modeB = 0;
	A_field [l] = 0;
	B_field [l] = 0;
	own [l] = 3;
	write [l] = who;
	}

void redcode_program::clear (int who) {
	location i;

	for (i = 0; i < coresize; i++) clear (i, who);
	}

void redcode_program::initialize (location instructions) {
	if ((inst_field = (instruction *) calloc (instructions, sizeof (instruction))) == NULL) {
		printf ("Allocation error inst_field\n");
		exit (10);
		}
	if ((A_field = (int far *) calloc (instructions, sizeof (int))) == NULL) {
		printf ("llocation error A_field\n");
		free (inst_field);
		exit (10);
		}
	if ((B_field = (int far *) calloc (instructions, sizeof (int))) == NULL) {
		printf ("Allocation error B_field\n");
		free (inst_field);
		free (A_field);
		exit (10);
		}
	if ((write = (unsigned int far *) calloc (instructions, sizeof (unsigned int))) == NULL) {
		printf ("Allocation error writer\n");
		free (inst_field);
		free (A_field);
		free (B_field);
		exit (10);
		}
	if ((own = (int far *) calloc (instructions, sizeof (int))) == NULL) {
		printf ("Allocation error inst_field\n");
		free (inst_field);
		free (A_field);
		free (B_field);
		free (write);
		exit (10);
		}
	}

void redcode_program::cleanup () {
	free (inst_field);
	free (A_field);
	free (B_field);
	free (write);
	free (own);
	}

// execute an instruction in the arena
// returns -1 if task dies, otherwise new program counter for task

location redcode_program::execute (int player, location loc) {
	int m1, a;
	location ea1, ea2, newpc = loc;

	ea1 = effadr (loc, 1, player);
	ea2 = effadr (loc, 2, player);
	m1  = inst_field [loc].modeA;

	switch (inst_field [loc].opcode) {
		case (MOV):	// move data from source to destination
			move (ea1, ea2, m1 == IMMEDIATE ? ATOB : ALL);
			gsetloc (ea2, player);
			setowwr (ea2, player);
			newpc++;
			break;
		case (ADD):	// add data from source to destination
			add (ea1, ea2, m1 == IMMEDIATE ? ATOB : BTOB);
			gsetloc (ea2, player);
			setowwr (ea2, player);
			newpc++;
			break;
		case (SUB):	// subtract data from source to destination
			subtract (ea1, ea2, m1 == IMMEDIATE ? ATOB : BTOB);
			gsetloc (ea2, player);
			setowwr (ea2, player);
			newpc++;
			break;
		case (JMP):	// jump to effective address
			newpc = ea1;
			break;
		case (JMZ):	// jump only if b-field is zero
			newpc = B_field [ea2] ? newpc + 1 : ea1;
			break;
		case (JMN):	// jump only if b-field is nonzero
			newpc = B_field [ea2] ? ea1 : newpc + 1;
			break;
		case (DJN):	// decrement b-field and jump if nonzero
			newpc = --B_field [ea2] ? ea1 : newpc + 1;
			gsetloc (ea2, player);
			setowwr (ea2, player);
			break;
		case (CMP):	// increment pc if comparison holds
			if (compare (ea1, ea2, m1 == IMMEDIATE ? ATOB : ALL)) newpc++;
			newpc++;
			break;
		case (SPL):	// one more task
			repro [player]++;
			if (tasks [player] < splits)
				pc [player] [tasks [player]++] = ea2;
			else
				dies [player]++;
			newpc++;
			splitbar (tasks [player], player);
			break;
		default:	// unknown opcode - task dies
			return (-1);
		}

	return (inrange (newpc));
	}


// move data (source, destination, movemode)

void redcode_program::move (location s, location d, int mode) {

	switch (mode) {
		case ALL:		// move everything
			inst_field [d].opcode = inst_field [s].opcode;
			inst_field [d].modeA  = inst_field [s].modeA;
			inst_field [d].modeB  = inst_field [s].modeB;
			A_field [d] = A_field [s];
			B_field [d] = B_field [s];
			break;
		case BTOB:	// move only b-field
			inst_field [d].modeB = inst_field [s].modeB;
			B_field [d] = B_field [s];
			break;
		case ATOB:	// move a-field to b-field
			inst_field [d].modeB = inst_field [s].modeA;
			B_field [d] = A_field [s];
			break;
		}
	}

// ALU-operation: add

void redcode_program::add (location s, location d, int mode) {
	B_field [d] += (mode == ATOB ? A_field [s] : B_field [s]);
	}

// ALU-operation: subtract

void redcode_program::subtract (location s, location d, int mode) {
	B_field [d] -= (mode == ATOB ? A_field [s] : B_field [s]);
	}

// ALU-operation: compare

int redcode_program::compare (location s, location d, int mode) {
	return (mode == ATOB ? (A_field [s] == B_field [d]) :
		(inst_field [s].opcode == inst_field [d].opcode) &&
		(inst_field [s].modeA == inst_field [d].modeA) &&
		(inst_field [s].modeB == inst_field [d].modeB) &&
		(A_field [s] == A_field [d]) &&
		(B_field [s] == B_field [d]));
	}

// effective address calculations
// returns new address

location redcode_program::effadr (location loc, int operand, int p) {
	unsigned mode;
	int value, temp;
	location address;

	if (operand == 1) {
		value = A_field [loc];
		mode  = inst_field [loc].modeA;
		}
	else {
		value = B_field [loc];
		mode  = inst_field [loc].modeB;
		}

	switch (mode) {	// select effective address from addressing mode and operand
		case (IMMEDIATE):
			address = loc;
			break;
		case (DIRECT):
			address = loc + value;
			break;
		case (INDIRECT):
			temp = inrange (loc + value);
			address = B_field [temp] + temp;
			break;
		case (AUTODECREMENT):
			temp = inrange (loc + value);
			B_field [temp]--;
			gsetloc (temp, p);
			setowwr (temp, p);
			address = B_field [temp] + temp;
			break;
		}

	return (inrange (address));
	}

// enter a program into core by filename
// returns location of initial program counter or:
//	-1: file not found
//	-2: read error
//	-3: no room
//	-4: locate error

location redcode_program::enter_warrior (char *text, char *namedest, int player) {
	int handle, i;
	int prog [MAXPRGSIZ * LOCSIZ];
	int t, loc, len, initpc, zpos, *p = prog;
	char name [13], *p1, *p2;
	static int hilo = 0, lastlen, highl = 0, lowl = 0;

	p1 = text;
	p2 = name;
	while (*p1 && *p1 != '.') *p2++ = *p1++;
	if (*p1)
		strcpy (p2, p1);
	else
		strcpy (p2, ".mrs");

	for (p1 = name, p2 = namedest; *p1 != '.'; p1++, p2++) *p2 = toupper (*p1);
	*p2 = '\0';

	if ((handle = _open (name, O_RDONLY)) == -1) return -1;
	if ((len = _read (handle, prog, MAXPRGSIZ * LOCSIZ)) <= 0) return -2;
	_close (handle);	// and that's all we need

	len = (len - sizeof (int)) / LOCSIZ;	// minus start indicator, divided by instructionword's length

	if (hilo) {
		t = 0;
		if (len + lastlen > coresize) return -3;
		do {		// find an unoccupied spot (tries 1024 times)
			while ((loc = rand () % coresize) <= highl && loc + len >= lowl) if (++t > 1024) return -4;
			} while ((highl >= coresize && loc <= highl % coresize) || (loc + len - 1>= coresize && loc + len >= lowl));
		}
	else loc = rand () % coresize;

	initpc = ((lowl = loc) + *p++) % coresize;	// starting location relative to physical start
	highl = lowl + len - 1;					// inclusive last location so minus 1

	for (t = len; t; t--) {					// insert instruction in core
		inst_field [loc].opcode = (*p & 0x00FF);
		inst_field [loc].modeA  = (*p & 0xF000) >> 12;
		inst_field [loc].modeB  = (*p++ & 0x0F00) >> 8;
		A_field [loc] = *p++;
		B_field [loc] = *p++;
		gsetloc (loc, hilo);
		owner (loc, player);
		writer (loc, MARS);
		loc++;
		loc %= coresize;
		}

	if (!gstat && debug)
		printf ("\"%s\" located from &%X to &%X inclusive (&%X bytes)\n", name, lowl, highl % coresize, len);

	if (++hilo == 2) hilo = 0;
	lastlen = len;
	return (initpc);
	}

redcode_program::redcode_program () {
	}

redcode_program::~redcode_program () {
	}
