// marsutil.cpp
//
// supplemental routines and utilities for the MARS system

#include <stdarg.h>
#include <stdio.h>              // for printf(), scanf() and sprintf()
#include <string.h>             // for strcpy(), strncpy(), strcmp()
#include <ctype.h>              // for isupper(), isascii() etc...
#include <fcntl.h>              // for file access flags
#include <io.h>         // for read() and write()
#include <graphics.h>   // Turbo C++ graphics templates
#include <conio.h>              // keyboard routines
#include <process.h>    // back-to-dos routines
#include <dos.h>                // ms-dos calls
#include <alloc.h>              // allocation
#include <stdlib.h>             // standard library (eg. rand())
#include <time.h>               // to set random seed
#include "mars.h"

//////////////// externals ////////////////

extern unsigned long spread [2];
extern int dblines;
extern int maxcolor;
extern int xpels;
extern int ypels;
extern int task [2];
extern int tasks [2];
extern int pc [2] [MAXSPL];
extern int xsize;
extern int ysize;
extern int pixelsize;
extern int maxx;
extern int maxy;
extern int gstat;
extern int runflag;
extern int splits;
extern char checkerboard [8];
extern char worksp [81];
extern char wname [2] [13];
extern char opcode [16] [4];
extern char addrmode [6];
extern char *stdext;
extern location coresize;
extern location inrange (int);
extern char *itoh (unsigned, unsigned, unsigned);
extern redcode_program arena;

//////////////// prototypes ////////////////

void cleargrid ();
void gclose ();
void gdisins (int, int, int);
void gdrawborder (int);
void gplot (int, int, int, int);
void gprtf (int, int, char *, ...);
void gsetloc (int, int);
void outchar (char, int, int);
void splitbar (int, int);
int ginit (unsigned int);
void centre (char *);
void disassemble (unsigned, int);
void diw (int, int);
void graphdis ();
void disassemble_warriors (int);
void list_warriors (int);
void sort (int);
void strcpyx (char *, char *);
void wait (char *);
char *itoh (unsigned, unsigned, unsigned);
location inrange (int);
int narrow (int, int);

//////////////// globals ////////////////

int graphdriver = DETECT;
int graphmode;

void graphdis () {
        int olddisloc, disloc, i, j, k, l, m, n, diff;

        j = getmaxx ();
        setfillstyle (EMPTY_FILL, BLACK);
        bar (0, 0, j, dblines * 8 - 1);
        setcolor (maxcolor);

        for (disloc = 0;;) {
                gsetloc (disloc, CURSOR);       // Show cursor
                olddisloc = disloc;                     // Remember cursor

                setfillstyle (EMPTY_FILL, BLACK);
                bar (0, 0, j, dblines * 8 - 1);
                for (i = 0, l = disloc; i < dblines && !kbhit (); i++) {
                        gdisins (l++, 0, i * 8);
                        l %= coresize;
                        }

                i = getch ();

                if (!i) i = getch ();
                        else if (i == TAB) i = TAB;
                                else break;

                switch (i) {
                        case CURSOR_RIGHT:
                                disloc++;
                                break;
                        case CURSOR_LEFT:
                                disloc--;
                                break;
                        case CURSOR_UP:
                                disloc -= xpels;
                                if (disloc < 0) {
                                        disloc += coresize + xpels - coresize % xpels;
                                        if (disloc >= coresize) disloc -= xpels;
                                        }
                                break;
                        case CURSOR_DOWN:
                                disloc += xpels;
                                if (disloc >= coresize) {
                                        disloc -= coresize;
                                        disloc += coresize % xpels;
                                        if (disloc >= xpels) disloc -= xpels;
                                        }
                                break;
                        case CURSOR_HOME:
                                disloc = 0;
                                break;
                        case CURSOR_END:
                                disloc = coresize - 1;
                                break;
                        case CTRL_CRSR_RIGHT:
                                disloc += dblines;
                                break;
                        case CTRL_CRSR_LEFT:
                                disloc -= dblines;
                                break;
                        case CURSOR_DEL:
                                arena.clear (disloc, USER);
                                break;
                        case CURSOR_INS:
                                if ((disloc = arena.writer (disloc)) < 0)
                                        disloc = olddisloc;
                                break;
                        case TAB:
                                diff = coresize;
                                for (l = 0; l < 2; l++)
                                        for (m = 0; m < tasks [l]; m++) {
                                                n = pc [l] [m] - disloc;
                                                if (n > 0 && n < diff) {
                                                        k = pc [l] [m];
                                                        diff = n;
                                                        }
                                                }
                                disloc = k % coresize;
                                break;

                        default:
                                break;
                        }

                disloc = inrange (disloc);
                gsetloc (olddisloc, arena.owner (olddisloc));   // Clear cursor
                }

        gsetloc (disloc, arena.owner (disloc));
        gsetloc (olddisloc, arena.owner (olddisloc));
        setfillstyle (EMPTY_FILL, BLACK);
        bar (0, 0, getmaxx (), dblines * 8 - 1);
        setfillstyle (SOLID_FILL, maxcolor);
        setcolor (maxcolor);
        }

void gdisins (int address, int x, int y) {
        unsigned int i;
        int iw, m1, m2, o1, o2, t, tx, f, j;
        char *p;

        if ((iw = arena.opcode (address)) >= ERR) iw = ERR;
        if ((m1 = arena.modeA (address)) >= BADMODE) m1 = BADMODE;
        if ((m2 = arena.modeB (address)) >= BADMODE) m1 = BADMODE;
        o1 = arena.operandA (address);
        o2 = arena.operandB (address);

        gprtf (x, y, "%s", itoh (address, 4, 2));
        x += 40;

        for (p = worksp, f = j = 0; j < 2; j++) {
                t = tasks [j];
                for (i = 0; i < t; i++) {
                        if (pc [j] [i] == address) {
                                if (f) sprintf (p++, "/");
                                sprintf (p, "%s", wname [j]);
                                p += strlen (wname [j]);
                                f++;
                                break;
                                }
                        }
                }

        if (f) {
                sprintf (p, ":");
                gprtf (x, y, worksp);
                }

        x += 160;

        gprtf (x, y, opcode [iw]);
        x += 32;

        if (iw != DAT && iw != SPL) {
                if (m1 != 1) {
                        outchar (addrmode [m1], x, y);
                        x += 8;
                        }
                sprintf (worksp, "%d", o1);
                gprtf (x, y, worksp);
                x += textwidth (worksp);
                if (iw != JMP) {
                        outchar (',', x, y);
                        x += 8;
                        }
                }

        if (iw != JMP) {
                if (m2 != 1) {
                        outchar (addrmode [m2], x, y);
                        x += 8;
                        }
                sprintf (worksp, "%d", o2);
                gprtf (x, y, worksp);
                }

        i = arena.owner (address);
        j = arena.writer (address);

        if (j == MARS)
                sprintf (worksp, "(written by MARS at start of battle)");
        else if (j == USER)
                sprintf (worksp, "(written by YOU during disassembly)");
        else if (i == PLAYER1 || i == PLAYER2)
                sprintf (worksp, "(written by %s at %X)", wname [i], j);
        else sprintf (worksp, "");

        x = 360;
        gprtf (x, y, worksp);
        }

int ginit (unsigned int request) {
        unsigned int i;

        initgraph (&graphdriver, &graphmode, "");
        if (graphresult () < 0) return (0);

        xsize = getmaxx () - 3;
        ysize = getmaxy () - (dblines * 8) - 3;
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

        settextstyle (DEFAULT_FONT, HORIZ_DIR, 1);
        settextjustify (LEFT_TEXT, TOP_TEXT);

        return (gstat); // success
        }

void gdrawborder (int req) {
        while (req % xpels) gsetloc (req++, OFFCORE);
        rectangle (0, (dblines * 8), maxx + 2, (dblines * 8) + maxy + 2);
        }

void gplot (int x, int y, int colour, int p) {

        switch (colour) {
                case PLAYER1:
                        setfillpattern (checkerboard, maxcolor);
                        bar (x, y, x + p, y + p);
                        break;
                case PLAYER2:
                        setfillstyle (SOLID_FILL, maxcolor);
                        bar (x, y, x + p, y + p);
                        break;
                case OFFCORE:
                        setfillstyle (CLOSE_DOT_FILL, maxcolor);
                        bar (x, y, x + p, y + p);
                        break;
                case BLACKOUT:
                        setfillstyle (EMPTY_FILL, BLACK);
                        bar (x, y, x + p, y + p);
                        break;
                case PCON:
                        setcolor (maxcolor);
                        line (x, y - 1, x + p, y - 1);
                        break;
                case PCOFF:
                        setcolor (0);
                        line (x, y - 1, x + p, y - 1);
                        break;
                case CURSOR:
                        setfillstyle (EMPTY_FILL, BLACK);
                        bar (x, y, x + p, y + p);
                        setcolor (maxcolor);
                        rectangle (x, y, x + p, y + p);
                        break;
                }
        }

void gsetloc (int loc, int player) {
        int xcoord, ycoord;
        unsigned int temp;

        if (runflag && player < 2) spread [player]++;
        if (!gstat) return;

        xcoord = (loc % xpels) * pixelsize + 2;
        ycoord = (loc / xpels) * pixelsize + (dblines * 8) + 2;
        gplot (xcoord, ycoord, player, (pixelsize - 2));
        }

void gclose () {
        if (gstat) closegraph ();
        }

void gprtf (int x, int y, char *fmt, ...) {
        va_list argptr;                 // Argument list pointer
        char str [140];                 // Buffer to build sting into

        if (gstat) {                                            // if graphics mode
                va_start (argptr, fmt);                 // Initialize va_ functions
                vsprintf (str, fmt, argptr);            // prints string to buffer
                outtextxy (x, y, str);                  // Send string in graphics mode
                va_end (argptr);                                // Close va_ functions
                }
        }

void splitbar (int value, int p) {
        if (!gstat) return;
        if (p) setfillstyle (SOLID_FILL, maxcolor);
                else setfillpattern (checkerboard, maxcolor);

        int deltax = (getmaxx () - 80) / splits;
        int x = 80 + deltax * (value - 1);
        int y = p * 16;

        if (value) bar (x, y, x + deltax - 2 , y + 8);

        setfillstyle (EMPTY_FILL, BLACK);
        bar (x + deltax, y, x + 2 * deltax - 2, y + 8);
        }

void cleargrid () {
        if (!gstat) return;
        setfillstyle (EMPTY_FILL, BLACK);
        bar (2, (dblines * 8) + 2, maxx - 2, (dblines * 8) + maxy - 2);
        }

void outchar (char c, int x, int y) {
        char buf [2];
        *buf = c;
        buf [1] = 0;
        gprtf (x, y, buf);
        }

void list_warriors (int rm) {
        int i, j, k, rmw1 = 1, rmw2 = 1, linecount = 0;

        clrscr ();
        sort (0);               // sort the PCs
        sort (1);

        i = *tasks, j = tasks [1], k = 0;

        if (!rm) {              // if one is completely dead
                if (!i) {
                        i = 1;
                        rmw1 = 0;
                        }
                if (!j) {
                        j = 1; rmw2 = 0;
                        }
                }

        while (i | j) {
                if (!linecount) {
                        centre ("Warrior Task List"); CR;
                        strcpy (worksp, "Task:           >               >       ");
                        strcpyx (&worksp [16], *wname);
                        strcpyx (&worksp [32], wname [1]);
                        puts (worksp);
                        puts ("---------       --------        --------");
                        linecount = 3;
                        }

                printf ("Task #%02d :\t", k++);
                if (i) printf ("@ %4X %s\t", pc [0] [--i], rmw1?"      ":"(dead)");
                  else printf ("\t\t");
                if (j) printf ("@ %4X %s", pc [1] [--j], rmw2?"      ":"(dead)");
                CR;

                if (++linecount > 20) {
                        CR;
                        wait (0);
                        clrscr ();
                        linecount = 0;
                        }
                }
        CR;
        wait (0);
        }

// make sure the given address is within limits of core

location inrange (int val) {
        if (val < 0) val += coresize;
        return val % coresize;
        }

void sort (int player) {                // Sort all warriors in address order
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

void strcpyx (char *s1, char *s2) { // like strcpy but the zero terminator is not copied
        while (*s2) *s1++ = *s2++;
        }

void wait (char *s) {   // wait ("message") or wait (NULL)
        if (s) puts (s);
        printf ("Press any key to continue...");
        while (!kbhit ());
        getch ();
        }

// Centre string on screen

void centre (char *text) {
        int i;
        const screenwidth = 80;

        for (i = (screenwidth - strlen (text)) >> 1; i; i--) putchar (' ');
        cprintf ("%s\r\n", text);
        }

int sqr (int x) {
        int i;
        for (i = 1; i < 200; i++) if (i * i > x) break; // OK, it's bad
        return (i - 1);
        }

void disassemble_warriors (int rm) {
        location a;

        clrscr ();
        centre ("Redcode Disassembler and core memory dump");
        printf ("\nAddress (hex): ");
        scanf ("%x", &a);
        disassemble (a, rm);
        }

void disassemble (location address, int rm) {
        int i, j, f, iw, m1, m2, o1, o2, c, rm2, df, movin = 1;

        while (movin) {
                if ((iw = arena.opcode (address)) >= ERR) iw = ERR;
                if ((m1 = arena.modeA (address)) >= BADMODE) m1 = BADMODE;
                if ((m2 = arena.modeB (address)) >= BADMODE) m1 = BADMODE;
                o1 = arena.operandA (address);
                o2 = arena.operandB (address);

                printf ("%s", itoh (address, 4, 2));
                printf ("%s", itoh (iw, 1, 1));
                printf ("%s", itoh (m1, 1, 101));
                printf ("%s", itoh (o1, 4, 1));
                printf ("%s", itoh (m2, 1, 101));
                printf ("%s", itoh (o2, 4, 3));

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
                        diw (m1, o1);
                        if (iw != JMP) putchar (',');
                        }
                if (iw != JMP) diw (m2, o2);

                if (df && (!tasks [0] || !tasks [1]))
                        printf ("\t<- This is where it died");

                CR;

                if (++address == coresize) address = 0;

                if (kbhit ()) {
                        getch ();
                        printf ("Stop/Continue? ");
                        c = tolower (getche ());
                        putchar (13);
                        if (c == 's') movin = 0;
                        }

                }
        }

void diw (int m, int v) {
        if (m != 1) putchar (addrmode [m]);     // direct addressing mode assumed
        printf ("%d", v);
        }

char *itoh (unsigned int value, unsigned int digits, unsigned int spaces) {
        static char buf [0x10];
        char c = ' ', *p = buf;

        while (digits--) *p++ = (char)(hex (value / (1 << (digits << 2)) & 0xF));

        if (spaces > 100) {     // if 10x is provided, use dashes instead
                spaces -= 100;
                c = '-';
                }

        while (spaces--) *p++ = c;
        *p = 0;
        return buf;
        }

int narrow (int a, int b) {     // boundary checking
        const c = 4;
        return (a + c >= b && a - c <= b);
        }
