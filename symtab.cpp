// symtab4.cpp
// implementation af et symtab-objekt som AVL-tr‘. 

#include <iostream.h>
#include <string.h>
#include <conio.h>
#include "symtab.h"

// konstrukt›r til en h‘gte

link::link (char* s, int i) {
	name = new char [strlen (s) + 1];
	strcpy (name, s);
	value = i;
	left = right = NULL;
	bf = 0;
	}

// destrukt›r til symtablen

symtab::~symtab () {
	deallocate ();
	}

// de-allokering af lager, som optages af h‘gter
// i tr‘et. denne rutine kaldes fra destrukt›ren
// og er rekursiv (post-order traversering).

void symtab::deallocate (link* p) {
	if (!p) p = root;
	if (p->left) deallocate (p->left);
	if (p->right) deallocate (p->right);
	delete p;
	}

// inds‘t en v‘rdi i symtablen
// finder vej til den sorteringsm‘ssigt rigtige h‘gte,
// p†h‘gter ny gren i tr‘et, og rydder derefter op i det.

symtab& symtab::insert (char* s, int i) {
	if (!root) root = new link (s, i);
	else {
		link* p = root, *a = root, *f = NULL, *q = NULL;
		int sgn, d;
		while (p) {
			if (p->bf) a = p, f = q;
			if ((sgn = strcmp (s, p->name)) < 0) q = p, p = p->left;
			else {
				if (sgn > 0) q = p, p = p->right;
				else {
					cerr << "symtab: " << s << " findes allerede "
						  << "(v‘rdi " << p->value << ")\n";
					return *this;
					}
				}
			}
		link* ny = new link (s, i), *b, *c;

		if (sgn < 0) q->left = ny;
			else q->right = ny;
		if (strcmp (s, a->name) > 0) b = p = a->right, d = -1;
			else b = p = a->left, d = 1;
		while ((sgn = strcmp (s, p->name)))
			if (sgn > 0) p->bf = -1, p = p->right;
				else p->bf = 1, p = p->left;

		int ubalance = 1;
		if (!a->bf) a->bf = d, ubalance = 0;
		if (!(a->bf + d)) a->bf = 0, ubalance = 0;
		if (ubalance) {
			if (d == 1) {				// ubalance til venstre
				if (b->bf == 1) a->left = b->right, b->right = a, a->bf = b->bf = 0;
				else {
					c = b->right, b->right = c->left, a->left = c->right,
					c->left = b, c->right = a;
					switch (c->bf) {
						case  1: a->bf = -1, b->bf = 0; break;
						case -1: b->bf = 1, a->bf = 0; break;
						case  0: b->bf = a->bf = 0; break;
						}
					}
				}
			else {						// ubalance til h›jre
				if (b->bf == -1) a->right = b->left, b->left = a, a->bf = b->bf = 0;
				else {
					c = b->left, b->left = c->right, a->right = c->left,
					c->right = b, c->left = a;
					switch (c->bf) {
						case  1: a->bf = 1, b->bf = 0; break;
						case -1: b->bf = -1, a->bf = 0; break;
						case  0: a->bf = b->bf = 0; break;
						}
					}
				}
			c->bf = 0, b = c;
			if (!f) root = b;
				else {
				if (!strcmp (f->left->name, a->name)) f->left = b;
					else f->right = b;
				}
			}
		}
	return *this;
	}

// get en v‘rdi fra symtablen
// bruger preorder traversering

int symtab::get (char* s) {
	link* p = root;
	int sgn;

	while (p)
		if (!(sgn = strcmp (s, p->name))) return p->value;
			else if (sgn < 0) p = p->left;
				else p = p->right;

	return 0;
	}
