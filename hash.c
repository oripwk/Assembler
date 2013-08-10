/* 
 * Author: Ori Popowski/Vladimir Moki
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "lib.h"

/* calculates value by string content and returns a number */
static unsigned hash(char *s);

hash_cell *lookup(char *s)
{
	hash_cell *np;
	
	for (np = hashtab[hash(s)]; np != NULL; np = np->next)
		if (strcmp(s,np->name) == 0)
			return np;
	return NULL;
}

hash_cell *install(char *name, char are, int i, char type)
{
	hash_cell *np;
	
	unsigned hashval;
	if ((np = lookup(name)) == NULL) { /* not found */
		np = (hash_cell *) malloc(sizeof(*np));
		if (np == NULL || (np->name = strdupp(name)) == NULL)
			return NULL;
		hashval = hash(name);
		np->ARE = are;
		np->index = i;
		np->data_ins = type;
		if (hashtab[hashval] == NULL)
			hashtab[hashval] = np;
		else { /* if hashtab[hashval] is not an empty linked list */
			hash_cell *hp;
			for (hp = hashtab[hashval]; hp->next != NULL; hp = hp->next)
				;
			hp->next = np;
		}
		np->next = NULL;
		
	}
	return np;
}

void init_hash(void)
{
	int i;
	for (i = 0; i < HASHSIZE; i++) {
		free_list(hash_cell, hashtab[i]);
		hashtab[i] = NULL;
	}
}

static unsigned hash(char *s)
{
	unsigned hashval ;
	for (hashval = 0 ; *s != '\0'; s++)
		hashval = *s +31 * hashval ;
	return hashval % HASHSIZE;
}
