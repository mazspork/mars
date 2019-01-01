/*****************************************************************************
 * REDCODE Disassembler ver. 1.1   (C) 1988 Mazoft Software (DK)
 * Format: RCDSM <filename(.typ)>
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>

#define sgn(i) (i >= 0)
#define hex(i) (i)+((i) > 9 ? '7' : '0')

char modes [5] = { '#', '$', '@', '<', '?' };

char opcode [16] [4] = {     /* List of legal REDCODE TLA-opcodes */
     "DAT", "MOV", "ADD", "SUB", "JMP", "JMZ",
     "JMN", "DJN", "CMP", "SPL", "???"
     };

char *copyright = "Redcode MARS Disassembler v. 1.0 - (C) 1988 Mazoft Software (DK)\n\n";
int f;

main (argc, argv)
int argc;
char **argv;
{
	FILE *fp;
	char wname [13];
	int i, start, iw, loc, m1, m2, o1, o2;

	f = 0;

	if (argc < 2) {
		puts ("Redcode Disassembler ver 1.1 - (c) 1988 Maz Spork\nUsage: RCDSM <filename(.type)>");
		exit (20);
		}

	if (argc == 3 && tolower (argv [2][1]) == 'a') f = 1;

	for (i = 0; argv [1] [i] && argv [1] [i] != '.'; i++) wname [i] = argv [1] [i];
     if (!argv [1] [i]) strcpy (&wname [i], ".mrs");
		else strcpy (&wname [i], &argv [1] [i]);

	if (!(fp = fopen (wname, "rb"))) {
		printf ("Error opening %s - misspelled or non-existing!\n", wname);
		exit (20);
		}

	start = getint (fp);
	loc = 0;

	while ((iw = getc (fp)) != EOF) {
		if (iw > 9) iw = 10;
		m1 = getc (fp);
		m2 = m1 & 15;
		m1 = m1 >> 4;
		if (m1 > 3) m1 = 4;
		if (m2 > 3) m2 = 4;
		if (iw > 10) iw = 11;
		o1 = getint (fp);
		o2 = getint (fp);
		itoh (loc, 4, 2);
		itoh (iw, 1, 1);
		itoh (m1, 1, 101);
		itoh (o1, 4, 1);
		itoh (m2, 1, 101);
		itoh (o2, 4, 3);
		if (loc == start) printf ("START:");
		printf ("\t%s\t", opcode [iw]);
          if (iw && iw != 9) diw (m1, o1, loc);
		if (iw != 4) diw (m2, o2, loc);
		putchar ('\n');
		loc++;
		}

	fclose (fp);
     }

diw (m, v, l)
int m, v;
{
	if (m != 1) putchar (modes [m]);
     printf ("%d", v);
	if (m && f) printf (" =%X", l + v);
	putchar ('\t');
	}

int getint (fh)
FILE *fh;
{
	return (getc (fh) + 256 * getc (fh));
	}

itoh (value, digits, spaces)      /* fast hex-print + trailing spaces*/
unsigned int value, digits, spaces;
{
	unsigned int i, k;
	char c = ' ';

	while (digits--)
		putchar (hex (value / (power (2, digits << 2)) & 0x0F));

     if (spaces > 100) {     /* if 10x is provided, use dashes instead */
          spaces -= 100;
          c = '-';
          }

	while (spaces--) putchar (c);
     }

int power (x, y)      /* x to the power of y as integers, not floating pt */
int x, y;
{
     int r;

     for (r = 1; y > 0; y--) r *= x;     /* r mult'd x - y times */
     return (r);
     }
