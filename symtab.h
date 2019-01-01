// en h‘gte i det bin‘re tr‘. indeholder pointere til venstre og
// h›jre datter, samt en balace-faktor og selve indholdet.

struct link {
	link* left;					// pointer til venstre h‘gte
	link* right;					// pointer til h›jre h‘gte
	int bf : 2;                // balance-faktor (2 bits)
	char* name;						// dette name har...
	int value;						// denne v‘rdi

	link (char* s, int i);
	~link (void) { delete name; }
	};

class symtab {
	link* root;	
public:
	symtab () { root = NULL; };			// konstrukt›r
	~symtab ();								// destrukt›r
	void deallocate (link* = 0);				// deallocateing (rekursiv)
	symtab& insert (char*, int);		// inds‘t en v‘rdi i tabellen
	int get (char*);								// get en v‘rdi i tabellen
	};
