/*****************************************************************************
 * REDCODE Assembler ver. 1.1   (C) 1988 Mazoft Software (DK)
 * Format: RCASM <filename(.typ)>
 * Outputs: <filename>.mrs as executeable MARS code
 * "My heaven will be a big heaven - and I will walk through the front door"
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define FSPLEN 16       /* File Specification maximum name length */
#define OPCODES 12      /* Number of operation codes & pseudo instructions */
#define UNKNOWN 12      /* If recognize() returns UNKNOWN, it's not listed */
#define BADEXP 0x7FFF   /* bad expression */
#define SYMSIZ 0x40     /* default symbol table stack space */
#define MAXPRGSIZ 0x200 /* default maximum redcode program size */
#define IMMEDIATE 0     /* addressing modes (two-bit bitfield) */
#define DIRECT 1
#define INDIRECT 2
#define AUTODECREMENT 3
#define STACKSIZE 0x100 /* evaluator's stack size */
#define MAXERR 16           /* errors before break */

#define report(c) {printf(errortext,(c),ipfile,curline,errormsg [(c)]);errno++;opc=UNKNOWN;}
#define hex(i) (i)+((i) > 9 ? '7' : '0')

int add ();
int subtract ();
int multiply ();
int divide ();
int goback ();
struct operator *vop ();

int stack [STACKSIZE];
char opstk [STACKSIZE];
int stackptr = 0;
int opsptr = 0;
int unary, len;

struct operator {                       /* structure for operators in expression evaluator */
        int priority;                   /* priority */
        char type;                      /* character definition */
        int (*handler)();               /* address of handler routine */
        };

struct symbol {               /* Symbol Table member consisting of: */
     char name [17];            /* label name */
     unsigned int value;                /* integer value */
     };

struct symbol symboltable [SYMSIZ];     /* n symbols allowed */

struct operator olist [] = {            /* list of legal operators */
        1, '+', &add,
        1, '-', &subtract,
        2, '*', &multiply,
        2, '/', &divide,
        0, 0, &goback
        };

char opcode [16] [4] = {     /* List of REDCODE TLA-opcodes and directives */
        "DAT", "MOV", "ADD", "SUB", "JMP", "JMZ",
        "JMN", "DJN", "CMP", "SPL", "END", "EQU"
     };

char *errortext = "Error %d in %s line %d : %s\n";
char *copyright = "Redcode MARS Assembler v. 1.1 - (C) 1988 Maz Spork\n\n";

char *errormsg [] = {                           /* error messages nos */
     "Line too long",                                                   /* 0 */
     "Invalid opcode",                                                  /* 1 */
     "Invalid expression or label not found",           /* 2 */
     "Invalid addressing mode",                                 /* 3 */
     "Double definition of label",                                      /* 4 */
     "Start indicator misspelled or missing",           /* 5 */
     "Out of stack space",                                              /* 6 */
     "Too many errors",                                                 /* 7 */
     "Too few operands",                                                        /* 8 */
     "Immediate addressing not allowed here",           /* 9 */
     "Garbled line or too many operands",                       /*10 */
        "Equate directive has no label",                                /*11 */
     "Heaven sends you no promises"                             /*12 */
     };

int flag, symbols = 0;     /* flag is reset if first column on line */
unsigned int program [MAXPRGSIZ * 3];

void spool (char*,FILE*,int);

void main (argc, argv)
int argc;
char **argv;
{
     struct symbol *symbolptr [SYMSIZ], *tempsym;
     FILE *source, *object, *listing;
     char opfile [FSPLEN], ipfile [FSPLEN], lstfile [FSPLEN], operand [64];
     char line [0x100], *charptr, *lineptr, c;
     int pass, curline, errno = 0, i, j, position, lasterr, ok;
     int opc;
     unsigned int mode, value, curloc, wasloc, *progptr;
     unsigned int modes [2], values [2], curop, tmpop;

     printf (copyright);     /* Min p0lle */

     if (argc < 2) {          /* No parameter supplied? */
          printf ("No source file specified.\n");
          exit (20);
          }

     if (strlen (argv [1]) > 12) {     /* Name too long? */
          printf ("Bad source file name %s.\n", argv [1]);
          exit (20);
          }

        if (argv [1] [0] == '?') {
                puts ("\nInstruction format:\n");
                puts ("(label) <OPC> (<m><opr>) (<m><opr>) (; comment)\n");
                puts ("\t\"label\" is an optional assembly label");
                puts ("\t\"OPC\" is the opcode or directive (see below)");
                puts ("\t\"m\" is an optional addressing mode (see below)");
                puts ("\t\"opr\" is an operand (label or expression)");
                puts ("\t\";\" treats rest of line as an unassembled comment\n");
                puts ("Redcode instructions:\n");
                puts ("\tDAT b\t\tIllegal opcode, provides data");
          puts ("\tMOV a b\t\tMove data");
                puts ("\tADD a b\t\tAdd a to b, result in b");
                puts ("\tSUB a b\t\tSubtract b from a, result in b");
                puts ("\tJMP a\t\tJump to a");
                puts ("\tJMZ a b\t\tJump to a if b is zero");
                puts ("\tJMN a b\t\tJump to a if b is non-zero");
                puts ("\tDJN a b\t\tDecrement b and jump to a if non-zero");
                puts ("\tSPL b\t\tSpawn child task at location b");
                puts ("\tEQU n\t\tEQUATE directive");
                puts ("\tEND\t\tEND directive\n");
                puts ("Addressing modes:\n");
                puts ("\t$\tDirect addressing\n\t#\tImmediate addressing");
                puts ("\t@\tIndirect addressing\n\t<\tIndirect with pre-decrement");
                exit (0);
                }

     for (charptr = argv [1]; *charptr; *charptr++ = toupper (*charptr));

     strcpy (opfile, argv [1]);     /* Copy name to obj and lst fsps */
     strcpy (ipfile, argv [1]);
     strcpy (lstfile, argv [1]);

     for (charptr = ipfile; *charptr != '.'; charptr++) /* find extension */
          if (!*charptr) strcpy (charptr--, ".RDC");     /* (if any) */

     strcpy (opfile+(charptr-ipfile), ".MRS");
     strcpy (lstfile+(charptr-ipfile), ".LST");

     if (!(source = fopen (ipfile, "r"))) {
          printf ("Source file \"%s\" not found.\n", ipfile);
          exit (20);
          }

     if (!(object = fopen (opfile, "wb"))) {
          printf ("Could not open object file \"%s\".\n", opfile);
          exit (20);
          }

     if (!(listing = fopen (lstfile, "w"))) {
          printf ("Could not open assembly listing file \"%s\".\n", lstfile);
          exit (20);
          }

     fprintf (listing, copyright);
     fprintf (listing, "line  addr  o A-oper B-oper label   opcode  operands\n\n");

     progptr = &program [1];     /* make room for relative start address */

     for (pass = 0; errno < MAXERR && pass < 2; pass++) {
          printf ("Pass %d\n", pass + 1);
          rewind (source);     /* start at beginning each pass */
          curloc = 0;
                curline = 1;

          while (!feof (source) && errno < MAXERR) {
               if ((i = getline (source, lineptr = line)) > 0xFF) report (0);
               if (i == EOF) break;     /* eof anyway? */

               wasloc = curloc;
               lasterr = errno;

                        values [0] = values [1] = modes [0] = modes [1] = 0;
               curop = flag = ok = 0;
               opc = UNKNOWN;

               while (i = getword (lineptr, charptr = operand)) {
                    lineptr += i;

                    if (*charptr == ';') break;     /* rest is comment */

                    switch (*charptr) {     /* addressing mode */
                         case '$' :
                              mode = DIRECT;
                              charptr++;
                              break;
                         case '#' :
                              mode = IMMEDIATE;
                              charptr++;
                              break;
                         case '@' :
                              mode = INDIRECT;
                              charptr++;
                              break;
                         case '<' :
                              mode = AUTODECREMENT;
                              charptr++;
                              break;
                         default:
                                                if (isalnum (*charptr) || *charptr == '+' || *charptr == '-' || *charptr == '!')
                                      mode = DIRECT;
                                                else
                                                        if (!pass) report (3);
                              break;
                         }

                    if ((tmpop = recognize (charptr)) == UNKNOWN) {     /* opcode ? */
                         if (pass) {            /* pass 2 */
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
                                                                        printf ("\"%s\" not evaluated.\n", charptr);
                                                                        }
                                        }
                                   else {
                                        if (curop == 2) {
                                             report (10);
                                                                        }
                                        else {
                                             modes [curop] = mode;
                                             values [curop++] = value;
                                             }
                                        }
                                   }
                              }
                         else {     /* pass 1 */
                              if (!flag) {     /* pass 1, column 0 */
                                   if (symbols == SYMSIZ) {
                                        report (6);     /* out of space */
                                        symbols--;
                                        }
                                                        if (evaluate (operand, 0) != BADEXP) {
                                                                report (4);
                                                                }
                                                        else {
                                           strncpy (symboltable [symbols].name, operand, 16);
                                                                strlwr (symboltable [symbols].name);
                                         symboltable [symbols].name [16] = '\0';
                                            symboltable [symbols].value = curloc;
                                       symbols++;
                                                                }
                                                        } /* flag */
                              } /* pass */
                         } /* known */
                    else {
                                        if (ok) {
                                                if (!pass) report (10);
                                                }
                         else if ((opc = tmpop) < 10) {
                              curloc++;         /* only increment PC if opcode is real */
                              ok = 1;
                              }
                         } /* known/unknown word */
                     } /* while words in line */

               if (lasterr != errno) values [0] = values [1] = modes [0] = modes [1] = 0;

               if (pass) {     /* tidy the code */
                    position = fprintf (listing, "%4d ", curline);
                    if (opc >= 0 && opc < 10) {     /* if not end directive, or error */
                                        putc (' ', listing);
                                        position = 2 + itoh (listing, curloc - ok, 4, 2) + itoh (listing, opc, 1, 1);
                         if (!opc || opc == 9) {     /* SPL or DAT */
                              fprintf (listing, "0-0000 ");
                              position += 7;
                              modes [1] = modes [0];
                              values [1] = values [0];
                              modes [0] = values [0] = 0;
                              }
                         else
                              position += itoh (listing, modes [0], 1, 101) + itoh (listing, values [0], 4, 1);
                         if (opc == 4) {
                              fprintf (listing, "0-0000 ");
                              position += 7;
                              }
                         else
                              position += itoh (listing, modes [1], 1, 101) + itoh (listing, values [1], 4, 1);
                                        if (opc == 4 || opc == 9 || !opc) {
                                                if (!curop) report (8);
                                                }
                                        else {
                                                if (curop < 2) report (8);
                                                }
                         if ((((opc && opc < 4) || opc == 10) && !modes [1]) || (opc > 3 && opc < 8 && !modes [0])) report (9);
                         position--;
                         }
                                else if (opc == UNKNOWN) {
                                        /* comment line or something */
                                        }
                                else {  /* assembly directives */
                                        putc ('(', listing);
                                        if (opc == 11)  {                       /* EQU */
                              for (j = 0; j < symbols && symboltable [j].value != wasloc; j++);
                                                symboltable [j].value = values [0];
                                                itoh (listing, values [0], 4, 0);
                                                }
                                        if (opc == 10) {                        /* END */
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
                                } /* if pass */
                        curline++;
               } /* while lines in file */
          } /* for pass */

        if (errno >= MAXERR) {
                report (7);
                }

     if ((*program = i = evaluate ("start", 0)) == BADEXP) {
          puts ("Warning: no start indicator, first instruction word assumed");
          *program = 0; /* assume initial PC to be physical start of code */
          }

        if (!curloc) puts ("Warning: no code was assembled - program length is zero");

     fprintf (listing, "\n\nCode size     = %4Xh location%c\nStart address = %4Xh relative", curloc, curloc == 1 ? ' ' : 's', *program);
     if (i == BADEXP) fprintf (listing, " (assumed)");     /* if start wasn't specified */
     fprintf (listing, "\nEfficiency    =  %3d%c of stack space used\n", (symbols * 100) / SYMSIZ, '%');
     if (symbols) fprintf (listing, "\nSymbol Table:\n\n");

     for (i = 0; i < symbols; i++) symbolptr [i] = &symboltable [i];
     for (i = 0; i < symbols; i++) {          /* alphasort symboltable */
          for (j = 0; j < symbols; j++) {
               if (strcmpi (symbolptr [i]->name, symbolptr [j]->name) < 0) {
                    tempsym = symbolptr [i];     /* swap */
                    symbolptr [i] = symbolptr [j];
                    symbolptr [j] = tempsym;
                    }
               }
          }

     for (i = 0; i < symbols; i++) {
          fprintf (listing, "%s", symbolptr [i]->name);
          for (j = 0; j < 17 - strlen (symbolptr [i]->name); j++)
               putc (' ', listing);
          fprintf (listing, "= ");
          itoh (listing, symbolptr [i]->value,4,3);
          putc (i & 1 ? '\n' : ' ', listing);     /* newline or space */
          }

     if ((i & 1)) putc ('\n', listing);     /* extra newline if odd end above */

        printf ("\n%d line%sassembled : %d error%c\n", curline - 1, curline == 2 ? " " : "s ", errno, errno == 1 ? ' ' : 's');     /* write how many errors detected */

        fwrite (program, sizeof (int), curloc * 3 + 1, object);

     fclose (source);   /* cloise dem files */
     fclose (object);
     fclose (listing);
     }

int recognize (word)     /* Compare an instruction to the opcode list */
char *word;
{
     int i;

     for (i = 0; i < OPCODES; i++) if (!strcmpi (word, opcode [i])) break;
     return (i);     /* case insensitive search */
     }

int getline (handle, ptr)      /* grab a line of source code */
FILE *handle;
char *ptr;
{
        char c;
     int length = 0;

        if (feof (handle)) return (EOF);

     do {
          *ptr++ = c = (char) getc (handle);
          } while (++length < 0xFF && c != '\n' && c != ':' && !feof (handle));

     *(ptr - 1) = '\0';
        length--;
        c = (char) getc (handle);
        if (feof (handle) && !length) return (EOF);
        ungetc (c, handle);

     return (length);
     }

int getword (srce, dest)      /* retrieve one word from line of source */
char *srce, *dest;
{
     char c;
        int l1, l2;

        l1 = l2 = 0;
     while (*srce == ' ' || *srce == '\t' || *srce == ',') {
          l1++;
          srce++;
          }

        flag += l1;

     while (*srce && !isspace (*srce) && *srce != ',') {
          *dest++ = *srce++;
          l2++;
          }

     *dest = '\0';
        return (l2 ? l1 + l2 : 0);
     }

void spool (text, handle, pos)      /* spool leading spaces + rest of line */
char *text;
FILE *handle;
int pos;
{
     int i = 0;

     for (; pos < 29; pos++) putc ('  ', handle);     /* leading spaces */

     while (*text) {
          if (*text == '\t')
               do {
                    putc (' ', handle);
                    i++;
                    } while (i % 8);
          else {
               putc (*text, handle);     /* source text copy */
               i++;
               }
          text++;
          }

        putc ('\n', handle);
     }

int itoh (stream, value, digits, spaces)      /* fast hex-print + trailing spaces*/
FILE *stream;
unsigned int value, digits, spaces;
{
     int i, j, k;
     char c = ' ';

     k = digits + spaces;          /* return length of written string */

        while (digits--) putc (hex ((value >> (digits * 4)) & 0xF), stream);

     if (spaces > 100) {     /* if 10x is provided, use dashes instead */
          spaces -= 100;
          c = '-';
          }

     for (; spaces;  spaces--) putc (c, stream);
     return (k);
     }

int power (x, y)      /* x to the power of y as integers, not floating pt */
int x, y;
{
     int r;

     for (r = 1; y > 0; y--) r *= x;     /* r mult'd x - y times */
     return (r);
     }

/*      EXPRESSION EVALUATOR:
 *      Evaluate integer expression, ascii characters pointed to by string.
 *      offset holds current PC to be subtracted from labels to achieve true
 *      relative addressing.
 */

int evaluate (expression, offset)
char *expression;
{
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

operate (v1, o, v2)
char o;
int v1, v2;
{
        return ((*((vop (o))->handler)) (v1, v2));
        }

struct operator *vop (o)                /* returns address of operator struct member */
char o;
{
        struct operator *p = olist;

        while (p->type && p->type != o) p++;
        return (p);
        }

int priory (v)                          /* returns hierachial priority of operator */
char v;
{
        return (vop (v)->priority);
        }

push (v)                                        /* push v onto evaluator's stack */
int v;
{
        stack [stackptr++] = v;
        return (v);
        }

int pop ()                              /* returns top value on evaluator's stack */
{
        return (stackptr ? stack [--stackptr] : 0);
        }

int add (a, b)
int a, b;
{
        return (a == BADEXP || b == BADEXP ? BADEXP : a + b);
        }

int subtract (a, b)
int a, b;
{
        return (a == BADEXP || b == BADEXP ? BADEXP : a - b);
        }

int multiply (a, b)
int a, b;
{
        return (a == BADEXP || b == BADEXP ? BADEXP : a * b);
        }

int divide (a, b)
{
        return (a == BADEXP || b == BADEXP ? BADEXP : a / b);
        }

goback ()               /* used for safety-return pointers */
{
        return (BADEXP);
        }

int getexp (source, offset)
char *source;
int offset;
{
        int c, i, n;
        char s [32], *p = s;

        unary = 1;              /* assume positive expression */
        len = 0;
        i = -1;

        while (i < 0 && !isalnum (c = *source)) {
                if (c == '-') {
                        unary *= -1;
                        }
                else if (c == '+') {
                        unary *= +1;
                        }
                else if (c == '!') {
               i = offset;
               }
                else unary = 2;         /* designate error */
          len++;
                source++;
                }

        if (i >= 0) return (i);
        if (unary == 2) return (BADEXP);

        while (isalnum (*source)) {
                *p++ = *source++;
                len++;
                }

        *p = '\0';

     if (isalpha (*s)) {        /* labels start with alphabetic char */
                for (i = 0; i < symbols; i++)
                        if (!strcmpi (s, symboltable [i].name)) break;
                if (i == symbols)
                        return (BADEXP);
                if (isupper (symboltable [i].name[0])) offset = 0;
                return ((symboltable [i].value - offset) * unary);
                }

        for (p = s, n = 0; isdigit (*p); p++) n = 10 * n + (int) (*p - '0');
     return (*p ? BADEXP : n * unary);
     }
