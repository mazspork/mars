// rcasm.cpp
//
// REDCODE Assembler ver. 3.0 - (C) 1988 Mazoft Software (DK)
// Format: RCASM <filename(.typ)> ...
// Outputs: <filename>.mrs as executeable MARS code
// "My heaven will be a big heaven - and I will walk through the front door"
//

#include <iostream.h>
#include <fstream.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "symtab.h"

const FSPLEN = 16;			// File Specification maximum name length 
const OPCODES = 12;			// Number of operation codes & pseudo instructions 
const UNKNOWN = 12;			// If recognize() returns UNKNOWN, it's not listed
const BADEXP = 0x7FFF;		// bad expression 
const SYMSIZ = 0x40;			// default symbol table stack space 
const MAXPRGSIZ = 0x200;	// default maximum redcode program size 
const IMMEDIATE     = 0;	// addressing modes (two-bit bitfield) 
const DIRECT        = 1;
const INDIRECT      = 2;
const AUTODECREMENT = 3;
const STACKSIZE = 0x100;	// evaluator's stack size 
const MAXERR = 16;			// errors before break 
const none = -1;

#define report(c) {printf(errortext,(c),ipfile,curline,errormsg [(c)]);errno++;opc=UNKNOWN;}

inline char* plural (int i) { return i ? "s" : ""; }
inline char hex (int i) { return i + (i > 9 ? '7' : '0'); }

struct op;
int add (int, int);
int subtract (int, int);
int multiply (int, int);
int divide (int, int);
int goback (void);
struct op *vop (char);
void process (char*);
void spool (char*, FILE*, int);
void instruct (void);

int stack [STACKSIZE];
char opstk [STACKSIZE];
int stackptr = 0;
int opsptr = 0;
int unary, len;

struct op {			// structure for operators in expression evaluator 
	int priority;			// priority 
	char type;			// character definition 
	int (*handler)();		// address of handler routine 
	};

struct op olist [] = {		// list of legal operators 
	1, '+', &add,
	1, '-', &subtract,
	2, '*', &multiply,
	2, '/', &divide,
	0, 0, &goback
	};

char opcode [16] [4] = {     // List of REDCODE TLA-opcodes and directives 
	"DAT", "MOV", "ADD", "SUB", "JMP", "JMZ",
	"JMN", "DJN", "CMP", "SPL", "END", "EQU"
	};

enum opcode_token {
	DAT, MOV, ADD, SUB, JMP, JMZ, JMN, DJN, CMP, SPL, END, EQU
	};

enum lexemtype {
	lex_label, lex_opcode, lex_operand, lex_none
	};

symtab symbols;

char *copyright = "Redcode MARS Assembler v. 3.0 - (C) 1988-1990 Maz H. Spork\n\n";

char *errormsg [] = {								// error messages
	"Line too long",									// 0 
	"Invalid opcode",									// 1 
	"Invalid expression or label not found",	// 2 
	"Invalid addressing mode",						// 3 
	"Double definition of label",					// 4 
	"Start indicator misspelled or missing",	// 5 
	"Out of stack space",							// 6 
	"Too many errors",								// 7 
	"Too few operands",								// 8 
	"Immediate addressing not allowed here",	// 9 
	"Garbled line or too many operands",		// 10 
	"Equate directive has no label",				// 11 
	"No source file(s) specified",            // 12
	"Heaven sends you no promises"				// 99
	};

int flag;							     // flag is reset if first column on line 
unsigned int program [MAXPRGSIZ * 3];

void main (int argc, char** argv) {

	cout << copyright;					// Min p›lle 

	if (argc < 2) {          // No parameter supplied? 
		cout << errormsg[12] << "\n";
		exit (20);
		}

	if (*argv [1] == '?') {
		instruct ();
		exit (0);
		}

	int success = 0, processed = 0;
	for (int argcnt = 2; argcnt <= argc; argcnt++)
		success += process (argv [argcnt]), processed++;

	cout << processed " files processed; " << success " files OK\n";
	}


/*

	Context-free grammar for a Redcode source program line:

		program_line :=
			label
			label opcode
			label opcode operands
			comment
			label comment
			label opcode comment
			label opcode operands comment

		comment :=
			';' until-eol

		label :=
			identifier
			identifier '!'
			identifier ':'

		opcode :=
			DAT 
			MOV 
			ADD
			SUB 
			JMP
			JMZ
			JMN
			DJN
			CMP
			SPL
			END
			EQU

		operands :=
			expression
			expression expression
			expression ',' expression



*/


// process a file

int process (char* filename) {
	char opfile [FSPLEN], ipfile [FSPLEN], lstfile [FSPLEN], operand [64];
	char line [0x100], *charptr, *lineptr, c;
	int pass, curline, errno = 0, i, j, position, lasterr, ok;
	int opc;
	unsigned int mode, value, curloc, wasloc, *progptr;
	unsigned int modes [2], values [2], curop, tmpop;

	if (strlen (filename > 12) {     // Name too long? 
		printf ("Bad source file name %s.\n", argv [1]);
		exit (20);
		}

	for (charptr = filename; *charptr; *charptr++ = toupper (*charptr));

	strcpy (opfile, filename);     // Copy name to obj and lst fsps 
	strcpy (ipfile, filename);
	strcpy (lstfile, filename);

	for (charptr = ipfile; *charptr != '.'; charptr++) // find extension 
		if (!*charptr) strcpy (charptr--, ".RDC");     // (if any) 

	strcpy (opfile+(charptr-ipfile), ".MRS");
	strcpy (lstfile+(charptr-ipfile), ".LST");

	istream source;
	ostream object, listing;

	source.open (ipfile, ios::in);
	object.open (opfile, ios::out);
	listing.open (lstfile, ios::out);	

	if (!source) {
		cout << "Couldn't open source file " << ipfile << " for input\n";
		return 0;
		}

	if (!object) {
		cout << "Couldn't open object file " << opfile << " for output\n";
		return 0;
		}

	if (!listing) {
		cout << "Couldn't open assembly listing file " << lstfile << " for output\n";
		return 0;
		}

	listing << copyright;
	listing << "line  addr  o A-oper B-oper label   opcode  operands\n\n";

	progptr = &program [1];     // make room for relative start address 

	cout << "Assembling " << ipfile << "\n";

	char line [buffersize];
	source.width (buffersize);

	char c, *ident;
	lexemtype lexem;

	for (pass = 0; errno < MAXERR && pass < 2; pass++) {
		cout << "Pass " << pass + 1 << "\n";
		source.seekg (0);

		curloc = 0;
		curline = 1;

		while (!source.eof () && errno < MAXERR) {
			
			source >> line;

			wasloc = curloc, lasterr = errno;
			values [0] = values [1] = modes [0] = modes [1] = 0;
			curop = flag = ok = 0, opc = UNKNOWN;

			// go through line of code

			lineptr = line;
			lexem   = lex_none;

			while (ident = getword (lineptr) {
				c = *ident;

				if (c == ';') break;					// rest is comment, leave loop

				switch (c) {							// addressing mode:
					case '$' : mode = DIRECT;        ident++; break;
					case '#' : mode = IMMEDIATE;     ident++; break;
					case '@' : mode = INDIRECT;      ident++; break;
					case '<' : mode = AUTODECREMENT; ident++; break;
					default: if (isalnum (c) || c == '+' || c == '-' || c == '!')
									mode = DIRECT;
								else if (!pass) report (3);
									break;
					}

				// find out what input is (label, opcode, operand);

				if ((tmpop = recognize (charptr)) == UNKNOWN) {	// opcode ? 
					if (pass) {		// pass 2
						if (flag) {
							if ((value = evaluate (charptr, wasloc)) == BADEXP) {
								if (opc == UNKNOWN) {
									if (!ok) {
										report (1);
										ok = 1;
										}
									}
								else {
									report (2);
									cout << charptr << "not evaluated\n";
									}
								}
							else {
								if (curop == 2)
									report (10);
								else {
									modes [curop] = mode;
									values [curop++] = value;
									}
								}
							}
						}
					else {     // pass 1 
						if (!flag) {     // pass 1, column 0 
							if (evaluate (operand, 0) != BADEXP) {
								report (4);
								}
							else {
								symbols.insert (strlwr (operand), curloc);
								}
							} // flag 
						} // pass 
					} // known 
				else {
					if (ok) {
						if (!pass) report (10);
						}
					else if ((opc = tmpop) < 10) {
						curloc++;		// only increment PC if opcode is real 
						ok = 1;
						}
					} // known/unknown word 
				} // while words in line 
	
			if (lasterr != errno) values [0] = values [1] = modes [0] = modes [1] = 0;

			if (pass) {     // tidy the code 
				position = fprintf (listing, "%4d ", curline);
				if (opc >= 0 && opc < 10) {     // if not end directive, or error 
					putc (' ', listing);
					position = 2 + itoh (listing, curloc - ok, 4, 2) + itoh (listing, opc, 1, 1);
			
					if (opc == DAT || opc == SPL) {
						fprintf (listing, "0-0000 ");
						position += 7;
						modes [1] = modes [0];
						values [1] = values [0];
						modes [0] = values [0] = 0;
						}
					else
						position += itoh (listing, modes [0], 1, 101) + itoh (listing, values [0], 4, 1);
		
					if (opc == JMP) {
						fprintf (listing, "0-0000 ");
						position += 7;
						}
					else
						position += itoh (listing, modes [1], 1, 101) + itoh (listing, values [1], 4, 1);
		
					if (opc == JMP || opc == SPL || opc == DAT) {
						if (!curop) report (8);
						}
					else {
						if (curop < 2) report (8);
						}
		
					if ((((opc > DAT && opc < JMP) || opc == END) && !modes [1]) || (opc > SUB && opc < CMP && !modes [0])) report (9);
						position--;
						}
					else if (opc == UNKNOWN) {
							// comment line or something 
						}
					else {	// assembly directives 
						putc ('(', listing);
						if (opc == 11) {			// EQU 
	
							for (j = 0; j < symbols && symboltable [j].value != wasloc; j++);
								symboltable [j].value = values [0];
							itoh (listing, values [0], 4, 0);
							}
						if (opc == 10) {			// END 
							fseek (source, 0L, SEEK_END);
							itoh (listing, 0, 4, 0);
							}
						fprintf (listing, ") ");
						position += 7;
						}
	
					if (opc < 10) {
						*progptr++ = (unsigned) opc + (modes [0] *0x1000) + (modes [1] * 0x100);
						*progptr++ = values [0];
						*progptr++ = values [1];
						}
	
					spool (line, listing, position + 1);
					}
				else {
					if (opc == 11) {
						if (!symbols || symboltable [symbols - 1].value != wasloc) {
						report (11);
						}
					else strupr (symboltable [symbols - 1].name);
					}
	
				if (opc == 10) fseek (source, 0L, SEEK_END);
				} // if pass 
			curline++;
			} // while lines in file 
		} // for pass 

	if (errno >= MAXERR) report (7);

	if ((*program = i = evaluate ("start", 0)) == BADEXP) {
		puts ("Warning: no start indicator, first instruction word assumed");
		*program = 0;	// assume initial PC to be physical start of code 
		}

	if (!curloc) puts ("Warning: no code was assembled - program length is zero");

	fprintf (listing, "\n\nCode size     = %4Xh location%c\nStart address = %4Xh relative", curloc, curloc == 1 ? ' ' : 's', *program);
	if (i == BADEXP) fprintf (listing, " (assumed)");     // if start wasn't specified 
	if (symbols) fprintf (listing, "\nSymbol Table:\n\n");

	for (i = 0; i < symbols; i++) {
		fprintf (listing, "%s", symbolptr [i]->name);
		for (j = 0; j < 17 - strlen (symbolptr [i]->name); j++)
			putc (' ', listing);
		fprintf (listing, "= ");
		itoh (listing, symbolptr [i]->value,4,3);
		putc (i & 1 ? '\n' : ' ', listing);     // newline or space 
		}

	if ((i & 1)) putc ('\n', listing);     // extra newline if odd end above 

	cour << curline - 1 << " line" 
	cout << "\n%d line%sassembled : %d error%c\n", curline - 1, curline == 2 ? " " : "s ", errno, errno == 1 ? ' ' : 's';

	fwrite (program, sizeof (int), curloc * 3 + 1, object);

	source.close ();
	object.close ();
	listing.close ();

	return 1;
	}

int recognize (char* word) {    // Compare an instruction to the opcode list 
	int i;
	for (i = 0; i < OPCODES; i++) if (!strcmpi (word, opcode [i])) break;
	return (i);     				// case insensitive search 
	}

char* getnext (char&* srce) {     // retrieve one word from line of source 
	static char ident [64];
	int l1 = 0, l2 = 0;

	while (*srce == ' ' || *srce == '\t' || *srce == ',') l1++, srce++;
	flag += l1;
	while (*srce && !isspace (*srce) && *srce != ',') *dest++ = *srce++, l2++;
	*dest = '\0';
	return (l2 ? l1 + l2 : 0);
	}

void spool (char* text, ostream& s, int pos) {     // spool leading spaces + rest of line 
	int i = 0;

	for (; pos < 29; pos++) s << (char) ' ';

	while (*text) {
		if (*text == '\t')
			do {
				s << (char) ' ';
				i++;
				} while (i % 8);
		else {
			s << (char) *text;
			i++;
			}
		text++;
		}

	s << endl;
	}

int itoh (FILE* stream, unsigned value, unsigned igits, unsigned spaces) {     // fast hex-print + trailing spaces
	int i, j, k;
	char c = ' ';
	k = digits + spaces;          // return length of written string 

	while (digits--) putc (hex ((value >> (digits * 4)) & 0xF), stream);

	if (spaces > 100) {     // if 10x is provided, use dashes instead 
		spaces -= 100;
		c = '-';
		}

	for (; spaces;  spaces--) putc (c, stream);
	return (k);
	}

int power (int x, int y) {     // x to the power of y as integers, not floating pt 
	  int r;
	  for (r = 1; y > 0; y--) r *= x;     // r mult'd x - y times 
     return (r);
     }

//	EXPRESSION EVALUATOR:
 *	Evaluate integer expression, ascii characters pointed to by string.
 *	offset holds current PC to be subtracted from labels to achieve true
 *	relative addressing.
 
int evaluate (char* expression, unsigned offset) {
	int f = 0;

	while (*expression && !isspace (*expression)) {
		if (f) {
	          while (opsptr && priory (*expression) <= priory (opstk [opsptr-1]))
				push (operate (pop (), opstk [--opsptr], pop ()));
			opstk [opsptr++] = *expression++;
			}

		push (getexp (expression, offset));
		expression += len;
		f++;
		}

	while (opsptr) push (operate (pop (), opstk [--opsptr], pop()));
	return (pop ());
	}

inline unsigned operate (int v1, char o, int v2) {
	return ((*((vop (o))->handler)) (v1, v2));
	}

op *vop (char o) {	// returns address of operator struct member 
	op *p = olist;

	while (p->type && p->type != o) p++;
	return (p);
	}

int priory (char v) {				// returns hierachial priority of operator 
	return (vop (v)->priority);
	}

int push (int v) {				// push v onto evaluator's stack 
	stack [stackptr++] = v;
	return (v);
	}

int pop (void)				// returns top value on evaluator's stack 
	return (stackptr ? stack [--stackptr] : 0);
	}

int add (int a, int b) {
	return (a == BADEXP || b == BADEXP ? BADEXP : a + b);
	}

int subtract (int a, int b) {
	return (a == BADEXP || b == BADEXP ? BADEXP : a - b);
	}

int multiply (int a, int b) {
	return (a == BADEXP || b == BADEXP ? BADEXP : a * b);
	}

int divide (int a, int b) {
	return (a == BADEXP || b == BADEXP ? BADEXP : a / b);
	}

goback (void)		// used for safety-return pointers (stub routine)
	return (BADEXP);
	}

int getexp (char* source, int offset) {
	int c, i, n;
	char s [32], *p = s;

	unary = 1;		// assume positive expression 
	len = 0;
	i = -1;

	while (i < 0 && !isalnum (c = *source)) {
		switch (c) {
			case '-': unary *= -1; break;
			case '+': unary *= 1;  break;
			case '!': i = offset;  break;
			default : unary = 2;   break;
			}
		len++, source++;
		}

	if (i >= 0) return (i);
	if (unary == 2) return (BADEXP);

	while (isalnum (*source)) *p++ = *source++, len++;

	*p = '\0';

	if (isalpha (*s)) {						// labels start with alphabetic char 
		for (i = 0; i < symbols; i++)
			if (!strcmpi (s, symboltable [i].name)) break;
		if (i == symbols) return (BADEXP);
		if (isupper (symboltable [i].name[0])) offset = 0;
		return ((symboltable [i].value - offset) * unary);
		}

	for (p = s, n = 0; isdigit (*p); p++) n = 10 * n + (int) (*p - '0');
		return (*p ? BADEXP : n * unary);

	}

void instruct (void) {
	cout << "\nThis assembler accepts Redcode assembly language programs, which follows\n"
		  << "the CoreWar'88 standard.\n\n"
		  << "\nInstruction format:\n"
		  << "(label) <OPC> (<m><opr>) (<m><opr>) (; comment)\n"
		  << "\t\"label\" is an optional assembly label"
		  << "\t\"OPC\" is the opcode or directive (see below)"
		  << "\t\"m\" is an optional addressing mode (see below)"
		  << "\t\"opr\" is an operand (label or expression)"
		  << "\t\";\" treats rest of line as an unassembled comment\n"
		  << "Redcode instructions:\n"
		  << "\tDAT b\t\tIllegal opcode, provides data"
		  << "\tMOV a b\t\tMove data"
		  << "\tADD a b\t\tAdd a to b, result in b"
		  << "\tSUB a b\t\tSubtract a from b, result in b"
		  << "\tJMP a\t\tJump to a"
		  << "\tJMZ a b\t\tJump to a if b is zero"
		  << "\tJMN a b\t\tJump to a if b is non-zero"
		  << "\tDJN a b\t\tDecrement b and jump to a if non-zero"
		  << "\tSPL b\t\tSpawn child task at location b"
		  << "\tEQU n\t\tEQUATE directive"
		  << "\tEND\t\tEND directive\n"
		  << "Addressing modes:\n"
		  << "\t$\tDirect addressing\n\t#\tImmediate addressing"
		  << "\t@\tIndirect addressing\n\t<\tIndirect with pre-decrement";
	}