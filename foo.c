
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


void main (int argc, char** argv)
{

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