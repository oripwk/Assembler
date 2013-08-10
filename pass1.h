/* 
 * Author: Ori Popowski/Vladimir Moki
 */

#include <stdio.h>

#define MAXINS 2048	/* `instruction' array's maximum length */
#define MAXDATA 2048	/* `data' array's maximum length */


/* InsEntry: represents an entry in `instructions' array */
typedef struct {
	unsigned short	code;		/* holds the machine code */
	unsigned char 	symbol_flag;	/* is this a symbol or not? */
	char 	*symbol;		/* symbol's name */
} InsEntry;

extern unsigned short data[MAXDATA];
extern InsEntry instructions[MAXINS];

/* Instruction Count, Data Count */
extern int ic, dc;

/* 
 * `pass1' reads the source file line by line. It does:
 * 1.  Fills the `data' array with all the data that was encountered in the program.
 * 2.  Fills the `instructions' array except for cells that are associated with symbols.
 * 3.  Builds the symbol table completely.
 * 4.  Writes the `.ent' file, if necessary.
 */
void pass1(FILE *src);
