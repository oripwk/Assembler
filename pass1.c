/* 
 * Author: Ori Popowski/Vladimir Moki
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#include "pass1.h"
#include "lib.h"
#include "hash.h"
#include "main.h"
#include "lib.h"


#define LINE 128	/* maximum line length */
#define INTT_MAX 32767
#define INTT_MIN -(INTT_MAX+1)

#define HLT 15		/* `hlt' offset in `commtab' */
#define DATA 16		/* `.data' offset in `commtab' */
#define STRING 17	/* `.string' offset in `commtab' */
#define ENTRY 18	/* `.entry' offset in `commtab' */
#define EXTERN 19	/* `.extern' offset in `commtab' */


enum addressing {IMMEDIATE = 1, DIRECT = 2, INDIRECT = 8, REGISTER = 16};
enum type {REG, VALUE, SYMBOL};


/* Command: represents a command or directive */
typedef struct {
	char *name;			/* command/directive name */
	int opsnum;			/* number of operands */
	int numeric_val;		/* numeric value */
	unsigned char leg_addr_src;	/* legal addressing modes for source */
	unsigned char leg_addr_tar; 	/* legal addressing modes for target */
} Command;

/* Operand: represents an operand */
typedef struct {
	unsigned char addrmode;	/* addressing mode */
	unsigned char type;	/* type: register/value/symbol */
	union {
		char *symbol;
		short num;
	} data;
} Operand;

/* EntNode: represents a node in a linked list which hold
 * names of `.entries' that appear in the source file */
typedef struct N {
	char symbol[MAXSYMB+1];
	struct N *next;
} EntNode;

unsigned short data[MAXDATA];
InsEntry instructions[MAXINS];


/* Instruction Count, Data Count */
int ic, dc;

static const Command commtab[] = 
{	
	{"mov", 2, 0, 	0 | IMMEDIATE |	DIRECT | INDIRECT | REGISTER, 0	| DIRECT | INDIRECT | REGISTER},
	{"cmp", 2, 1, 	0 | IMMEDIATE | DIRECT | INDIRECT | REGISTER, 0 | IMMEDIATE | DIRECT | INDIRECT | REGISTER},
	{"add", 2, 2, 	0 | IMMEDIATE | DIRECT | INDIRECT | REGISTER, 0 | DIRECT | INDIRECT | REGISTER},
	{"sub", 2, 3, 	0 | IMMEDIATE | DIRECT | INDIRECT | REGISTER, 0 | DIRECT | INDIRECT | REGISTER},
	{"and", 2, 4, 	0 | IMMEDIATE | DIRECT | INDIRECT | REGISTER, 0 | DIRECT | INDIRECT | REGISTER},
	{"xor", 2, 5, 	0 | IMMEDIATE | DIRECT | INDIRECT | REGISTER, 0 | DIRECT | INDIRECT | REGISTER},
	{"lea", 2, 6, 	0 | DIRECT, 0 | DIRECT | INDIRECT | REGISTER},
	{"inc", 1, 7, 	0, 0 | DIRECT |	INDIRECT | REGISTER},
	{"dec", 1, 8, 	0, 0 | DIRECT | INDIRECT | REGISTER},
	{"jmp", 1, 9, 	0, 0 | DIRECT | INDIRECT},
	{"bne", 1, 10, 	0, 0 | DIRECT | INDIRECT},
	{"red", 1, 11, 	0, 0 | DIRECT | INDIRECT | REGISTER},
	{"prn", 1, 12, 	0, 0 | IMMEDIATE | DIRECT | INDIRECT | REGISTER},
	{"jsr", 1, 13, 	0, 0 | DIRECT | INDIRECT},
	{"rts", 0, 14, 	0, 0},
	{"hlt", 0, 15, 	0, 0},
	{".data", -1, -1, 0, 0},	/* #16 */
	{".string", -1, -1, 0, 0},	/* #17 */
	{".entry", -1, -1, 0, 0},	/* #18 */
	{".extern", -1, -1, 0, 0},	/* #19 */
	{NULL, -1, -1, 0, 0},
};	

/* 
 * gets a short with only one bit set,
 * and returns it's index, while LSB has
 * index of 1 and the index increases as we
 * get closer to MSB.
 * example: if it gets a binary 100 then it returns 3
 */
static int bit2num(unsigned short num);

/* checks if `label' is a valid label. if it is, then the colons at the end are exchanged with null char. */
static int valid_label(char *label);

/* checks if a `symbol' is a valid symbol */
static int valid_symbol(char *symbol);

/* converts string `snum' into short int and returns it */
static short strtos(char *snum, int lines);

/* 
 * checks if `snum' contains only zero's
 * example: if `snum' is ``00000000'' it returns `1'.
 */
static int iszero(char *snum);

/* makes *pop1 and *pop2 point to strings which contain operands' names */
static int parseoperands(char **pop1, char **pop2, char *bufptr, int lines);

/* parses and inserts the data after a `.data' directive into `data' array */
static void parsedata(char *bufptr, int lines);

/* gets `op' which points to a string that contains an operand name and fills the corresponding `Operand' structure with data about it */
static void getop(char *op, Operand *ope, int lines);

/* gets op1 and op2 which point to a string representing operands and fills the corresponding structures with data about them */
static void getops(char *op1, char *op2, Operand *ope1, Operand *ope2, int lines);

/* fills an entry which is pointed to by `ic' in array `instruction'. uses `fill_entry' to do the job */
static void fill_instruction_entries(char *op1, char *op2, Operand *ope1, Operand *ope2, int *reserve, int opsnum, int i, int lines);

/* fills entries. uses `handletype'*/
static void fill_entries(Operand *ope1, Operand *ope2, int *reserve, int opsnum, int i);

/* 
 * according to `type' field at `ope' (register/value/symbol), sets the relevant bits in the 
 * first machine word of an instruction, and fills an additional machine words if necessary.
 * i.e. the `type' is symbol or value.
 */
static void handletype(Operand *ope, int *reserve);

/* 
 * takes `bufptr' and if `bufptr' points to beginning of "<one or more chars here>", which is parameter for 
 * `.string' directive, then it inserts the characters ASCII code into `data' array
 */
static void parsestring(char *bufptr, int lines);

/* takes a symbol and builds a new `EntNode' with it */
static EntNode *newnode(char *symbol);

/* takes `s' and checks if it is a name of register/command/directive */
static int is_keyword(char *s);

/* write `.entries' into entries file */
static void write_ents(EntNode *head);

/*
 * `pass1' reads the source file line-by-line.
 * The overall strategy is as follows:
 * 
 * 1. The `instructions' array is filled with the proper coding (i.e. the `code' field is filled) for:
 *    1) First words of an instruction. 2) Additional words of an instruction that contain immediate values (i.e. #-1).
 *    Additional words of an instruction that refer to a symbol operands are no coded. Instead, the `symbol_flag' is set to one, and `symbol' is filled with the symbol's name.
 *
 * 2. The `data' array is filled completely.
 * 
 * 3. The symbol table is being built. Every label that is encountered, is installed in the hash-table with the additional fields:
 *    1) `ARE' will hold `r' for rellocatable label and `e' for external label (i.e. label that was declared with `.external').
 *    2) `index' will hold the current value of `ic' if the label refers to an instruction and the current value of `dc' if the label refers to data.
 *    3) `data_ins' will hold `d' if the label refers to data and `i' if the label refers to instruction. This is used in the writing of the object file: If the label refers to data, then because
 *       data should appear after the code segment, we need to add up the whole code segment length.
 *
 * 4. For every `.entry' directive, a node with entry's name is added to a linked list of entries.
 *
 * 5. After the main loop, the linked list is traversed and the `.ent' file is generated. 
 */
void pass1(FILE *src)
{
	char buffer[LINE];	/* this buffer holds the current line being processed */
	char *bufptr;
	char token[LINE];	/* holds a token */
	
	char label[LINE];	/* points to labels at beginning of sentences */
	int labf;		/* label flag - was there a label? */
	
	int lines;		/* holds the number of current processed line */
	int i;
	
	char *op1, *op2;	/* points to a parsed string which refer to operand 1 / operand 2 */
	Operand ope1, ope2;	/* holds data about operand 1 / operand 2 */
	int num_operands;	/* holds number of operands in the current line */
	
	int tmp;		/* temporarily holds ic/dc and then used as the index value for the hash-table */
	int reserve;		/* how many entries ahead in `instructions' array to reserve for current instruction */
	
	EntNode *head = NULL;
	EntNode *nodeptr;
	
	ic = dc = 0;
	
	for (lines = 0; fgets(buffer, sizeof(buffer), src); lines++)
	{
		labf = 0;
		
		/* skip white-spaces */
		for (bufptr = buffer; isspace(*bufptr); bufptr++)
			;
			
		/* if empty line or comment line */	
		if (*bufptr == '\0' || *bufptr == ';')
			continue;
		
		sscanf(bufptr, "%s", token);
		
		/* lookup `token' in commands tab list */
		for (i = 0; commtab[i].name != NULL; i++)
			if (strcmp(commtab[i].name, token) == 0)
				break;
				
		/* if token is not an a command/directive */		
		if (commtab[i].name == NULL)
		{		
			/* if it is a label that doesn't begin at first column */
			if (bufptr > buffer)
				error("line %d: first token is not an identifier and doesn't start on first column.\n", lines);
			/* if it starts at first column but is not a valid label */
			if (!valid_label(token))
				error("line %d: invalid label.\n", lines);
			
			/* now we know that `token' is a valid label */
			labf = 1;
			strcpy(label, token);	/* so `token' can be used to hold
							 * other tokens without losing the label's name */
			
			/* move bufptr to end of this label */
			for (; !isspace(*bufptr) && *bufptr != '\0'; bufptr++)
				;
			/* skip white-spaces until next token */
			for (; isspace(*bufptr); bufptr++)
				;
			
			/* if there is only a label in this line */
			if (*bufptr == '\0')
				error("line %d: line only with a label.\n", lines);
			
			sscanf(bufptr, "%s", token);
			
			/* now `token' is supposed to hold a command/directive */

			/* lookup in commands tab list */
			for (i = 0; commtab[i].name != NULL; i++)
				if (strcmp(commtab[i].name, token) == 0)
					break;
			
			/* if token is not an a command/directive */
			if (commtab[i].name == NULL)
				error("line %d: expects command or directive after label.\n", lines);	
		}	
		
		/* move bufptr to end of this token */
		for (; !isspace(*bufptr) && *bufptr != '\0'; bufptr++)
			;
				
		/* skip white-spaces until next token */
		for (; isspace(*bufptr); bufptr++)
			;
		
		/* now `bufptr' should point to directive parameters or command operands */
		
		/* if operation line */
		if (i <= HLT) {
			reserve = 0;
			tmp = ic;
			num_operands = parseoperands(&op1, &op2, bufptr, lines);
			
			/* if # of operands in this sentence doesn't match the expected # of operands by this command */
			if (commtab[i].opsnum != num_operands)
				error("line %d: invalid number of operands.\n", lines);
				
			fill_instruction_entries(op1, op2, &ope1, &ope2, &reserve, num_operands, i, lines);
			
			ic += reserve + 1;
			if (labf) {
				/* insert label into symbol table */
				if (install(label, 'r', tmp, 'i') == NULL)
					error("line %d: error ocurred label processing.\n", lines);
			}
			continue;
		}		
		if (i == DATA) {	/* if .data */
			tmp = dc;
			if (*bufptr == '\0') 
				error("line %d: no parameters to `.data'.\n", lines);
			/* `bufptr' now points to a token */
			parsedata(bufptr, lines);
			if (labf) { /* insert label into symbol table */
				if (install(label, 'r', tmp, 'd') == NULL)
					error("line %d: error ocurred label processing.\n", lines); 
			}
			continue;
		}
		if (i == STRING) {	/* if .string */
			tmp = dc;
			parsestring(bufptr, lines);
			if (labf) { /* insert label into symbol table */
				if (install(label, 'r', tmp, 'd') == NULL)
					error("line %d: error ocurred label processing.\n", lines);
			}
			continue;
		}
		if (i == ENTRY) {	/* if .entry */	
			if (*bufptr == '\0')
				error("line %d: no parameter to `.entry'.\n", lines);

			sscanf(bufptr, "%s", token);
			
			if (!valid_symbol(token))
				error("line %d: invalid symbol.\n", lines);
			
			if (head == NULL)
				nodeptr = head = newnode(token);
			else {
				nodeptr->next = newnode(token);
				nodeptr = nodeptr->next;
			}
			continue;
		}
		if (i == EXTERN) {	/* if .extern */
			if (*bufptr == '\0')
				error("line %d: no parameter to `.entry'.\n", lines);
			
			sscanf(bufptr, "%s", token);
			
			if (!valid_symbol(token))
				error("line %d: invalid symbol.\n", lines);
				
			if (install(token, 'e', 0, '\0') == NULL)
				error("line %d: error ocurred label processing.\n", lines);
		}
	} /* `for' loop */

	/* write entries file */
	write_ents(head);
	
	free_list(EntNode, head);
	
}

static int valid_label(char *label)
{
	int i;
	if (isdigit(*label))
		return 0;
	for (i = 0; *label != '\0' && *label != ':'; label++, i++)
		if (!isalnum(*label))
			return 0;
	if (label[0] == ':' && label[1] == '\0' && i <= MAXSYMB && !is_keyword(label)) {
		label[0] = '\0';
		return 1;
	}
	return 0;
	
}

static int valid_symbol(char *symbol)
{
	int i;
	if (isdigit(*symbol))
		return 0;
	for (i = 0; *symbol != '\0'; symbol++, i++)
		if (!isalnum(*symbol))
			return 0;
	return i <= MAXSYMB;
}

static short strtos(char *snum, int lines)
{
	long num;
	char c;
	char *endptr = &c;
	
	num = (strtol(snum, &endptr, 10));
	
	if (*endptr != '\0' || (num == 0 && !iszero(snum)))
		error("line %d: \"%s\" is invalid data.\n", lines, snum);
		
	if (num > INTT_MAX || num < INTT_MIN)
		error("line %d: integers too high / low.\n", lines);
		
	return (short) num;
}

static int iszero(char *snum)
{
	for (; *snum != '\0'; snum++)
		if (*snum != '0')
			return 0;
	return 1;
}

static int parseoperands(char **pop1, char **pop2, char *bufptr, int lines)
{

	char *nulchar;		/* where to insert '\0' */

	if (*bufptr == '\0') {	/* no operands */
		*pop1 = *pop2 = NULL;
		return 0;
	}
	if (*bufptr == ',')
		error("line %d: unnecessary comma(s).\n", lines);

	*pop1 = bufptr;

	/* go to the end of this operand */
	while (!isspace(*bufptr) && *bufptr != ',' && *bufptr != '\n' && *bufptr != '\0')
		++bufptr;

	/* if the first operand's end is also the end of the line */
	if (*bufptr == '\n' || *bufptr == '\0') {
		if (*bufptr == '\n')
			*bufptr = '\0';
		*pop2 = NULL;
		return 1;
	}

	nulchar = bufptr;
	
	/* ignore 1st operand's following spaces */
	while (isspace(*bufptr))
		++bufptr;

	if (*bufptr == '\0') { /* this line was exhausted */
		*nulchar = '\0';
		*pop2 = NULL;
		return 1;
	}
	if (*bufptr != ',')
		error("line %d: missing comma(s).\n", lines);
	else
		++bufptr;

	/* `bufptr' points right after the comma now */
	*nulchar = '\0';	/* terminate 1st operand's string */

	/* ignore 2nd operand's leading spaces */
	while (isspace(*bufptr))
		++bufptr;
	if (*bufptr == '\0' || *bufptr == ',')
		error("line %d: unnecessary comma(s).\n", lines);
		
	*pop2 = bufptr;
	/* go to the end of this operand */
	while (!isspace(*bufptr) && *bufptr != ',' && *bufptr != '\n' && *bufptr != '\0')
		++bufptr;
	if (*bufptr == '\n' || *bufptr == '\0') {
		if (*bufptr == '\n')
			*bufptr = '\0';
		return 2;
	}
	nulchar = bufptr;
	/* check for excessive characters in the end of this sentence */
	for (; *bufptr != '\0'; bufptr++)
		if (!isspace(*bufptr))
			error("line %d: excessive character(s) at end of sentence.\n", lines);

	*nulchar = '\0';
	/* all is well */
	return 2;
}

static void parsedata(char *bufptr, int lines)
{
	char *snum;	/* ``string num''. points to a beginning of a number */
	char tmp;
	int end;	/* flag if `bufptr' is in the end of the buffer */
	
	for (;;)
	{
		end = 0;
		snum = bufptr;
		/* go to the end of this token */
		while (!isspace(*bufptr) && *bufptr != ',' && *bufptr != '\n')
			++bufptr;
		if (*bufptr == ',' && bufptr == snum)
			error("line %d: unnecessary comma(s).\n", lines);
		
		end = (*bufptr == '\n' || *bufptr == '\0') ? 1 : 0;
		tmp = *bufptr;
		*bufptr = '\0';
		
		data[dc++] = strtos(snum, lines);
		
		if (end)
			break;
			
		*bufptr = tmp;
		/* ignore following blanks that might occur after the token */
		while (isspace(*bufptr))
			++bufptr;
		if (*bufptr == '\0')
			break;			/* this line was exhausted */
		if (*bufptr != ',')
			error("line %d: missing comma(s).\n", lines);
		else
			++bufptr;	/* now it points right after the comma */
		/* ignore blanks that might occur between the comma and the next token */
		while (isspace(*bufptr))
			++bufptr;
		if (*bufptr == '\0')
			error("line %d: unnecessary comma(s).\n", lines);
	}
}
	
static void getops(char *op1, char *op2, Operand *ope1, Operand *ope2, int lines)
{
	getop(op1, ope1, lines);
	getop(op2, ope2, lines);
}

static void getop(char *op, Operand *ope, int lines)
{
	char *bufptr = op;
	int indirect = 0;
	
	if (*bufptr == '#') {
		ope->data.num = strtos(++bufptr, lines);
		ope->addrmode = IMMEDIATE;
		ope->type = VALUE;
		return;
	}
	if (bufptr[0] == 'r' && '0' <= bufptr[1] && bufptr[1] <= '7' && bufptr[2] == '\0') {
		ope->data.num = bufptr[1] - '0';
		ope->addrmode = REGISTER;
		ope->type = REG;
		return;
	}
	if (*bufptr == '@') {
		++bufptr;
		indirect = 1;
	}
	if (valid_symbol(bufptr)) {
		ope->data.symbol = bufptr;
		ope->addrmode = indirect ? INDIRECT : DIRECT;
		ope->type = SYMBOL;
		return;
	} else
		error("line %d: invalid label.\n", lines);
}

static int bit2num(unsigned short num)
{
	int i;
	for (i = 0; num != 1; num >>=1)
		++i;
	return i;
}

static void fill_instruction_entries(char *op1, char *op2, Operand *ope1, Operand *ope2, int *reserve, int opsnum, int i, int lines)
{
	switch (opsnum)
	{
		case 0:
			fill_entries(NULL, NULL, reserve, 0, i);
			break;
			
		case 1:
			getop(op1, ope1, lines);
			if (ope1->addrmode & commtab[i].leg_addr_tar)	/* is operand's addressing mode legal? */
				fill_entries(ope1, ope2, reserve, opsnum, i);
			else
				error("line %d: invalid addressing modes.\n", lines);
			break;
		case 2:
			getops(op1, op2, ope1 ,ope2, lines);
			if ((ope1->addrmode & commtab[i].leg_addr_src) && (ope2->addrmode & commtab[i].leg_addr_tar))	/* are operands' addressing mode legal? */
				fill_entries(ope1, ope2, reserve, opsnum, i);
			else
				error("line %d: invalid addressing modes.\n", lines);
			break;
	}
}
static void fill_entries(Operand *ope1, Operand *ope2, int *reserve, int opsnum, int i)
{
	instructions[ic].symbol_flag = 0;			/* first word of instruction is never a symbol */
	instructions[ic].code |= commtab[i].numeric_val;	/* fill the first field of this word with command's code */
	instructions[ic].code <<= 3;
	if (opsnum == 2) 	/* if we have two operands then the "source operand addresing mode" field must be filled */
		instructions[ic].code |= bit2num(ope1->addrmode);
	instructions[ic].code <<= 3;
	if (opsnum == 2)
		/* fill "source register number" field (may be left blank) 
		 * and fill relevant information in additional machine word if necessary */
		handletype(ope1, reserve);
	instructions[ic].code <<= 3;
	/* fill "target operand addresing mode" field */
	if (opsnum == 1) 
		instructions[ic].code |= bit2num(ope1->addrmode);
	else if (opsnum == 2)
		instructions[ic].code |= bit2num(ope2->addrmode);
	instructions[ic].code <<= 3;
	/* fill "target operand register number" field (may be left blank)
	 * and fill relevant information in additional machine word if necessary */
	if (opsnum == 1)
		handletype(ope1, reserve);
	else if (opsnum == 2)
		handletype(ope2, reserve);
}
	
static void handletype(Operand *ope, int *reserve)
{
	switch (ope->type)
	{
		case REG:
			instructions[ic].code |= ope->data.num;
			break;
		case VALUE:
			++(*reserve);
			instructions[ic+*reserve].code |= ope->data.num;
			instructions[ic+*reserve].symbol_flag = 0;
			break;
		case SYMBOL:
			++(*reserve);
			instructions[ic+*reserve].symbol = strdupp(ope->data.symbol);
			instructions[ic+*reserve].symbol_flag = 1;
			break;
	}
}

static void parsestring(char *bufptr, int lines)
{
	if (*bufptr == '\0')
		error("line %d: no parameters to `.data'.\n", lines);

	if (*bufptr != '\"')
		error("line %d: expected \" after `.string'.\n", lines);

	++bufptr;
	
	if (*bufptr == '\"')
		error("line %d: empty string.\n", lines);
		
	for (; *bufptr != '\"'; bufptr++) {
		if (*bufptr == '\0')
			error("line %d: no closing \".\n", lines);
		data[dc++] = *bufptr;
	}
	data[dc++] = '\0';
	++bufptr;
	for (; *bufptr != '\0'; bufptr++)
		if (!isspace(*bufptr))
			error("line %d: excessive character(s) at end of sentence.\n", lines);
}

static EntNode *newnode(char *symbol)
{
	EntNode *n = (EntNode *) malloc(sizeof (EntNode));
	strcpy(n->symbol, symbol);
	n->next = NULL;
	return n;
}

static int is_keyword(char *s)
{
	int i;
	
	for (i = 0; i <= HLT; i++)
		if (strcmp(commtab[i].name, s) == 0)
			return 1;
	for (; commtab[i].name != NULL; i++)
		if (strcmp(commtab[i].name+1, s) == 0)
			return 1;
	return !(  strcmp("r0", s) && strcmp("r1", s)
		&& strcmp("r2", s) && strcmp("r3", s)
		&& strcmp("r4", s) && strcmp("r5", s)
		&& strcmp("r6", s) && strcmp("r7", s) );
	
}

static void write_ents(EntNode *head)
{
	if (head == NULL)
		return;
	else {
		FILE *pen;	/* pointer to `entries' file */
		char en[MAX_FNAME+5]; /* name of entries file */
		hash_cell *h;	/* used to catch the pointer returned by `lookup' */
		EntNode *nodeptr;
		
		strcpy(en, fname);
		strcat(en, ".ent");
		
		errno = 0;
		if ((pen = fopen(en, "w")) == NULL)
			perror(en);

		for (nodeptr = head; nodeptr != NULL; nodeptr = nodeptr->next) {
			if ((h = lookup(nodeptr->symbol)) == NULL)
				error("the label specified by entry doesn't exist in this source file.\n");
			fprintf(pen, "%s:\t%o\n", nodeptr->symbol, (h->data_ins == 'i') ? (h->index) : (h->index + ic));
		}
	}
}
