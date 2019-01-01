// options.h
// contains prototypes for the option class

enum opt_type { O_END, O_INT, O_STRING, O_FLAG };

struct options {
	int type;			// data field identificator
	int textx, texty;	// coords for descriptive field
	char *text;		// text for descriptive field
	int x, y;			// coords for input-field
	void *data;		// pointer to actual data;
	char *desctext;	// descriptive text;
	};

class opt_screen {
	options *opt_list;
	int current_option, max_option;
	char *buffer;
public:
	opt_screen (options *);
	~opt_screen ();
	void print ();
	void print (options *);
	int modify ();
	void select (int);
	void clear ();
	};
