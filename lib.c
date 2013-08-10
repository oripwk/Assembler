/* 
 * Author: Ori Popowski/Vladimir Moki
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

void error(char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	fprintf(stderr, "%s: error: ", fname);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
}

char *strdupp(char *str)
{
	char *strtmp;
	strtmp = (char *) malloc(MAXSYMB+1);
	strcpy(strtmp, str);
	return strtmp;
}

