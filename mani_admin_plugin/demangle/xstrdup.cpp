/* xstrdup.c -- Duplicate a string in memory, using xmalloc.
   This trivial function is in the public domain.
   Ian Lance Taylor, Cygnus Support, December 1995.  */

/*

@deftypefn Replacement char* xstrdup (const char *@var{s})

Duplicates a character string without fail, using @code{xmalloc} to
obtain memory.

@end deftypefn

*/

#include <sys/types.h>
#include <string.h>
#include "ansidecl.h"
#include "libiberty.h"

char *
xstrdup (const char *s)
{
  register size_t len = strlen (s) + 1;
  register char *ret = XNEWVEC (char, len);
  return (char *) memcpy (ret, s, len);
}
