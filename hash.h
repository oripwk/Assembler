/* 
 * Author: Ori Popowski/Vladimir Moki
 */

#define  HASHSIZE 101

/* Hash Cell */
typedef struct cell {
	char *name;	/* symbol's name */
	char ARE;	/* Absolute/Rellocatable/External */
	int index;	/* ic/dc index */
	char data_ins;	/* data or instruction? */
	struct cell *next;
} hash_cell;

hash_cell *hashtab[HASHSIZE];

/*
 * gets a string and searches it in `hash_tab'.
 * if the string is found, it does nothing;
 * otherwise, installs the parameters as a new cell in `hash_tab'.
 */
hash_cell *install(char *name, char are, int i, char type);

/*
 * gets a string and searches it in `hash_tab'.
 * if the string is found, it returns a pointer to the corresponding hash cell;
 * otherwise, it returns NULL.
 */
hash_cell *lookup(char *s);

/* initialize hash-table */
void init_hash(void);
