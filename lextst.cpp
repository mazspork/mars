
typedef int bool;

class program {
protected:
	static char *opcode [] = {
		"ADC",
		"

		};
	static const opcodes = 12;
	static instructionword prog [1000];
	static symtab symbols;
	static int error;
	static char outtxt [0x100]; 
	};

class identifier : public program {
public:
	virtual int process (char*);
	};

class label : public identifier {
public:
	int process (char*);
	};

int label::process (char* symbol) {
	symbab.insert (symbol, 	

class opcode : public identifier {
	int opcode;
public:
	}

class operand : public identifier {
	char* expr;
public:
	}

class comment : public identifier {
	char* comment;
public:
	}

struct instructionword {
	unsigned word0, word1, word2;
	};

// process a line of assembly

int program::process_line (char* line) {
	while (line) {
		identifier* t = analyze (next_token (line));
		error = t->process (token);
		delete t;
		}
	}

// get next token from a pointer to assembly text

char* program::nexttoken (char&* ptr) {
	static buffer [0x100], *p = buffer;
	if (!ptr) return NULL;
	while (isseparator (*ptr)) ptr++;
	while (!isseparator (*ptr)) *p++ = *ptr++;
	*p = '\0';
	if (!*ptr) ptr = NULL;
	}

// return true if character is a separator (white space or comma)

inline bool isseparator (char c) {
	return c != 0 || c == ' ' || c == '\t' || t == ',';
	}

// a very simple lexical analyzer - comment, opcode, operand or label

identifier* program::analyze_token (char* token) {
	if (*token == ';')							// semicolon always means comment
		return new comment (token);		
	for (int i = 0; i < opcodes; i++)		// search for opcode match
		if (!strcmp (token, opcode [i]))
			return new opcode (i);
	if (isalpha (*token)) {						// all alphanumerics is a label
		for (char* p = token + 1; *p; p++)
			if (!isalnum (*p)) break;
		if (!*p) return new label (token);
		}
	return new operand (token);				// otherwise assume an operand
	}
