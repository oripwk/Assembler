/* 
 * Author: Ori Popowski/Vladimir Moki
 */

/* echoes an error preceded by the file name in which it
 * occured and dies */
void error(char *fmt, ...);

/* copies `str' into fresh allocated space and returns pointer to it */
char *strdupp(char *str);

#define initialize(x) memset((x), '\0', sizeof(x))

/* frees a linked list pointed to by `p' */
#define free_list(T, p)		\
{				\
    T *next;			\
    while ((p) != NULL) {	\
        next = (p)->next;	\
        free(p);		\
        (p) = next;		\
    }				\
}
