#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "options.h"

#define DELETE			0x08
#define TAB			0x09
#define NEWLINE		0x0D
#define ESCAPE			0x1B
#define CURSOR_HOME		0x47
#define CURSOR_UP		0x48
#define CURSOR_PGUP		0x49
#define CURSOR_LEFT		0x4B
#define CURSOR_RIGHT	0x4D
#define CURSOR_END		0x4F
#define CURSOR_DOWN		0x50
#define CURSOR_PGDN		0x51
#define CURSOR_INS		0x52
#define CURSOR_DEL		0x53
#define CTRL_CRSR_LEFT	0x73
#define CTRL_CRSR_RIGHT	0x74

opt_screen::opt_screen (options *o) {
	opt_list = o;
	current_option = max_option = 0;
	buffer = new char [80];
	while (opt_list [max_option].type != O_END) max_option++;
	}

opt_screen::~opt_screen () {
	delete buffer;
	}

void opt_screen::print () {
	options *opt = opt_list;

	while (opt->type != O_END) print (opt++);
	}

void opt_screen::print (options *opt) {
	int i;

	gotoxy (opt->textx, opt->texty);
	cprintf (opt->text);
	gotoxy (opt->x, opt->y);

	switch (opt->type) {
		case O_INT:
			cprintf ("%u   ", *(int*) opt->data);
			break;
		case O_STRING:
			cprintf ("%s   ", (char*) opt->data);
			break;
		case O_FLAG:
			gotoxy (opt->x - 1, opt->y);
			cprintf ("[%c]", *(int*) opt->data ? 'x' : ' ');
			break;
		default:
			break;
		}
	}

void opt_screen::select (int o) {
	current_option = o;
	}

int opt_screen::modify () {
	int c, d, i;
	char *p = buffer;
	options *opt = &opt_list [current_option];

	_setcursortype (_NOCURSOR);
	gotoxy (1,23);
	cprintf (opt->desctext);
	i = 78 - wherex ();
	while (--i) putchar (' ');

	_setcursortype (_SOLIDCURSOR);
	gotoxy (opt->x, opt->y);
	while (!kbhit ());
	if ((d = getch ()) == ESCAPE) return -1;
	if (d == NEWLINE) return 0;
	if (iscntrl (d)) d = 0;

	if (d) {
		clear ();
		if (opt->type == O_INT || opt->type == O_STRING) {
			*p++ = d;
			gotoxy (opt->x, opt->y);
			putchar (d);
			i = 1;
			while ((c = getch ()) != NEWLINE) {
				if (c == ESCAPE) return -1;
				if (!c) {
					d = 0;
					break;
					}
				if (c == DELETE) {
					if (i) {
						p--;
						i--;
						putchar (8);
						putchar (32);
						putchar (8);
						}
					}
				else if (i < 79) {
					if (!iscntrl (c)) {
						*p++ = c;
						i++;
						putchar (c);
						}
					}
				}
			*p = '\0';
			}

		switch (opt->type) {
			case O_INT:
				* (int*) opt->data = atoi (buffer);
				break;
			case O_STRING:
				strcpy ((char*) opt->data, buffer);
				break;
			case O_FLAG:
				*(int*) opt->data = !*(int*) opt->data;
				break;
			default:
				break;
			}
		print (opt);
		if (opt->type != O_FLAG && d)
			if (++current_option == max_option) current_option = 0;
		}

	if (!d) {
		switch (getch ()) {
			case CURSOR_UP:
				if (current_option == 0) current_option = max_option;
				current_option--;
				break;
			case CURSOR_DOWN:
				if (++current_option == max_option) current_option = 0;
				break;
			default:
				break;
			}
		}

	return 1;
	}

void opt_screen::clear () {
	int i;
	options *opt = &opt_list [current_option];

	switch (opt->type) {
		case O_INT:
			i = strlen (itoa (*(int*)opt->data, buffer, 10));
			break;
		case O_FLAG:
			i = 1;
			break;
		case O_STRING:
			i = strlen ((char*) opt->data);
			break;
		default:
			break;
		}

	gotoxy (opt->x, opt->y);
	while (i--) putchar (' ');
	}

