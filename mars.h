#define CENTRE_TEXT CENTER_TEXT	// Dem Yankees can't spell!
#define FSPLEN    16	// File Specification maximum name length
#define BADEXP    0x7FFF	// bad expression
#define SYMSIZ    0x40	// default symbol table stack space
#define MAXPRGSIZ 0x200	// default maximum redcode program size
#define CRESIZ    0x1000	// default core size
#define MAXCS	   0x7FFF	// max core size
#define DEFTIME   5000	// 5000 instructions * 2 before timeout
#define DEFRUN    1		// default no. of battles in a single run
#define ALL  1			// flags for internal addressing
#define BTOB 2
#define ATOB 4
#define MARS -1
#define USER -2
#define MAXSPL 1024		// max spawned tasks
#define CR putchar('\n')
#define BRITE	15
#define NOBRITE 7

#define TAB			0x09
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

#define hex(i) (char)((i)+((i) > 9 ? '7' : '0'))
#define LOCSIZ (sizeof (int) * 3)

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned int location;

enum opcodes  { DAT, MOV, ADD, SUB, JMP, JMZ, JMN, DJN, CMP, SPL, ERR };
enum operands { IMMEDIATE, DIRECT, INDIRECT, AUTODECREMENT, BADMODE };
enum plotopt  { PLAYER1, PLAYER2, OFFCORE, BLACKOUT, PCON, PCOFF, CURSOR };

class redcode_program {
	struct instruction {
		unsigned modeA  : 2;
		unsigned modeB  : 2;
		unsigned opcode : 4;
		} *inst_field;
	int *A_field, *B_field, *own;
	location *write;
public:
	redcode_program ();
	~redcode_program ();
	void initialize (location);
	void cleanup ();
	uint opcode (location);
	uint modeA (location);
	uint modeB (location);
	int operandA (location);
	int operandB (location);
	int owner (location);
	uint writer (location);
	void writer (location, location);
	void owner (location, int);
	void setowwr (location, int);
	void clear (location, int);
	void clear (int);
	location enter_warrior (char *, char *, int);
	void move (location, location, int);
	void add (location, location, int);
	void subtract (location, location, int);
	int compare (location, location, int);
	location execute (int, location);
	location effadr (location, int, int);
	};

