#define CENTRE_TEXT CENTER_TEXT	// Dem Yankees can't spell!
#define FSPLEN    16	// File Specification maximum name length
#define OPCODES   11	// Number of operation codes & pseudo instructions
#define UNKNOWN   11	// If recognize() returns UNKNOWN, it's not listed
#define BADEXP    0x7FFF	// bad expression
#define SYMSIZ    0x40	// default symbol table stack space
#define MAXPRGSIZ 0x200	// default maximum redcode program size
#define CRESIZ    0x1000	// default core size
#define MAXCS	   0x8000	// max core size
#define DEFTIME   5000	// 5000 instructions * 2 before timeout
#define DEFRUN    1		// default no. of battles in a single run
#define IMMEDIATE 0		// addressing modes (two-bit bitfield)
#define DIRECT    1
#define INDIRECT  2
#define AUTODECREMENT 3
#define BADMODE 4
#define ALL  1			// flags for internal addressing
#define BTOB 2
#define ATOB 4
#define MAXSPL 1024		// max spawned tasks
#define CR putchar ('\n')
#define BRITE	15
#define NOBRITE 7

#define CURSOR_LEFT		0x4B
#define CURSOR_RIGHT	0x4D
#define CURSOR_UP		0x48
#define CURSOR_DOWN		0x50
#define CURSOR_HOME		0x47
#define CURSOR_END		0x4F
#define CURSOR_DEL		0x53
#define CTRL_CRSR_LEFT	0x73
#define CTRL_CRSR_RIGHT	0x74
#define TAB			0x09

#define hex(i) (char)((i)+((i) > 9 ? '7' : '0'))
#define LOCSIZ (sizeof (int) * 3)

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned int location;
