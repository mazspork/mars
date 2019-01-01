#include <alloc.h>
#include <stdio.h>
#include <stdlib.h>

struct abc {
	int a, b;
	};

struct abc huge *p;
struct abc huge *q;

main (int aa, char **bb) {
	unsigned long i, ant = atol (bb [1]);

	p = (struct abc huge *) farcalloc (ant, sizeof (struct abc));

	q = p;

	for (i = 0; i < ant; i++) {
		p [i].a = i;
		p[i].b = ~i;
		}

	q = p;

	printf ("Testing with pointers...\n");

	for (i = 0; i < ant; i++) {
		if ((q->a != i) || (q->b != ~i)) printf ("%lu\n", i);
		q++;
		}

	printf ("Testing with array indexes...\n");

	for (i = 0; i < ant; i++) {
		if (p [i].a != i || p [i].b != ~i) printf ("%lu\n", i);
		}

	farfree (p);
	}

