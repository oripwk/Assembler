/* 
 * Author: Ori Popowski/Vladimir Moki
 */

#include <string.h>
#include <errno.h>

#include "pass1.h"
#include "hash.h"
#include "lib.h"
#include "main.h"

/* converts `num' to octal n-digits number, and puts it in `s'. */
static void to_oct_n(unsigned num, char *s, int n);

/*
 * There are two loops. The first runs through `instructions' array and writes all the code segment into the `.ob' file.
 * The second runs through `data' array and writes the data segment into the `.ob' file.
 *
 * In the first loop, for every `instructions' cell that contains a symbol, we lookup it's name in the hash table.
 * If this symbol is rellocatable, then if it refers to data we add up the code segment's length and then print.
 * If this symbol is external we print ``000000'' and then write it to the `.ext' file.
 */
void pass2(void)
{
	int i, j;
	hash_cell *p;			/* to catch a hash_cell returned by `lookup' */
	unsigned short code;		/* a temp variable which hold a coded machine word */
	char f_name[MAX_FNAME + 5];	/* used to hold the original file name + an extension i.e. `.ext'/`.ob' */	
	char buffer[7];			/* will hold octal numbers up to 6 digits long and a '\0' */
	FILE *ob;			/* will point to `.ob' file */
	FILE *ex;			/* will point to `.ext' file */
	
	int flag = 0;			/* to tell if we've already encountered an external symbol within hash table */
	
	/* make `f_name' hold the current processed file's name and the extension ".ob" */
	strcpy(f_name, fname);
	strcat(f_name, ".ob");
	
	errno = 0;
	if ((ob = fopen(f_name, "w")) == NULL)
		perror(f_name);
	
	/* now `.ob' file is opened. start to write into it */
	
	/* prints code's length and data's length in octal machine words */
	fprintf(ob, "\t%o %o\n", ic, dc);
	
	/* runs through `instructions' array */
	for (i = 0; i < ic; i++) {
		
		/* prints `i' in 4-digit octal number. `i' is the address. */
		to_oct_n(i, buffer, 4);
		fprintf(ob, "%s\t", buffer);
		
		/* if the current machine word is a label,
		 *  and we don't know it's address */
		if (instructions[i].symbol_flag) {
			if ((p = lookup(instructions[i].symbol)) == NULL)
				error("label %s isn't specified anywhere in the file.\n", instructions[i].symbol);
			/* symbol was found in symbol table */	
			switch (p->ARE) {
				case 'r':	/* we've found the label in the hash table and it is rellocatable */
					code = p->index;
					/* if this label is associated with data (not code), then
					 * we need to add up the whole code segment's length (=ic) */
					if (p->data_ins == 'd')
						code += ic;
					/* print the label's address in octal */
					to_oct_n(code, buffer, 6);
					fprintf(ob, "%s\t", buffer);

					fputc('r', ob);
					break;
					
				case 'e':	/* we've found the label in the hash table and it is external */
					fprintf(ob, "000000\t");	/* according to project's specs, if an operand is */
					fputc('e', ob);			/*  an external symbol, then it's address is zero */
					
					/* if this is the first time we encounter an external symbol */
					if (!flag) {
						flag = 1;
						strcpy(f_name, fname);
						strcat(f_name, ".ext");
						/* create a `.ext' file */
						errno = 0;
						if ((ex = fopen(f_name, "w")) == NULL)
							perror(f_name);
					}
					/* now write this symbol's name into the `.ext' file, followed
					 * by the address it was referred at */
					fprintf(ex, "%s\t%o\n", instructions[i].symbol, i);
					break;
			}
		/* if current machine word is not a symbol... */
		} else {
			/* ...then the `code' field has already been filled by `pass1', and we just need to write it as-is */
			to_oct_n(instructions[i].code, buffer, 6);
			fprintf(ob, "%s\t", buffer);
			
			fputc('a', ob);
		}
		fputc('\n', ob);
	}
	
	/* now just write the data segment */
	for (j = 0; j < dc; i++, j++) {
		to_oct_n(i, buffer, 4);
		fprintf(ob, "%s\t", buffer);
		
		to_oct_n(data[j], buffer, 6);
		fprintf(ob, "%s\n", buffer);
	}
}


static void to_oct_n(unsigned num, char *s, int n)
{
	int i;
	for (i = n-1; i >= 0; i--, num >>= 3)
		s[i] = (num & '\x07') + '0';
	s[n] = '\0';
}
