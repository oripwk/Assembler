/* 
 * Author: Ori Popowski/Vladimir Moki
 */

/*
 * Data Structures:
 * 1. array `instructions' - each entry is of type `InsEntry' and represents a machine word in the code segment. `InsEntry' has three fields:
 *    `code', `symbol_flag', and `symbol'. `code' is an unsigned short that will eventually hold the coded machine word; `symbol_flag' is an unsigned char that specifies
 *    if this entry represents a symbol; `symbol' is used to hold the name of that symbol. It is unused if the entry does not represent a symbol.
 * 2. array `data' - each entry is an unsigned short that represents a machine word in the Data Segment.
 * 3. hash table `hashtab' - each entry represents a symbol. Has four fields:
 *    `name', `ARE', `index', `data_ins'. `name' holds the name of the symbol; `ARE' holds a char which is either `r' or `e', while `r' represents ``rellocatable'' and `e' represents ``external'';
 *    `index' is the value of ic/dc. It is ic if the symbol is associated with instruction and dc if the symbol is associated with data;
 *    `data_ins' represents either a symbol which is associated with data or instruction using `d' or `i'.
 *
 * General Algorithm:
 * `pass1' reads the source file line by line.
 * 1. `pass1' fills the `data' array with all the data that was encountered in the program.
 * 2. `pass1' fills the `instructions' array except for cells that are associated with symbols.
 * 3. `pass1' builds the symbol table completely.
 * 4. `pass1' writes the `.ent' file, if necessary.

 * `pass2' runs through `instructions' and then through `data'.
 * 1. `pass2' writes the `.ob' file.
 * 2. `pass2' writes the `.ext' file, if necessary.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "pass1.h"
#include "hash.h"
#include "lib.h"
#include "main.h"
#include "pass2.h"

char fname[MAX_FNAME + 1];	/* source file name without extension */

/* tries to open the `.as' file and returns the pointer */
static FILE *fopenw(char *name);

int main(int argc, char *argv[])
{
	FILE *fp;

	if (argc == 1) {
		fprintf(stderr, "error: no files specified.\n");
		return 1;
	}
		
	while (--argc > 0) {
		if (strlen(*++argv) >= MAX_FNAME + 1) {
			fprintf(stderr, "error: file name too long: %s.\n", *argv);
			return 1;
		}
		errno = 0;
		if ((fp = fopenw(*argv)) == NULL) {
			perror(*argv);
			return 1;
		}
		strcpy(fname, *argv);
		pass1(fp);
		pass2();
		init_hash();
		initialize(instructions);
		initialize(data);
	}
	init_hash();	/* clean up memory used by hash table */
    return 0;
}

static FILE *fopenw(char *name)
{
	char f_name[MAX_FNAME + 4];
	
	strcpy(f_name, name);
	strcat(f_name, ".as");
	
	return fopen(f_name, "r");
}
