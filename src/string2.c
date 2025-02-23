#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "misc.h"
#include "list.adt.h"
#include "string2.h"

extern bool str2_findch(const char *scanset, char ch) {
   bool found;

   found = false;
   while (*scanset)
      if (ch == *scanset++) {
         found = true;
         break;
      }

   return found;
}

extern int str2_hasonly(const char *str, char ch) {
   bool same_char;

   same_char = true;
   while (*str)
      if (*str++ != ch) {
         same_char = false;
         break;
      }

   return same_char ? 1 : 0;
}

extern bool str2_gowith(const char *str, const char *substr, int offset) {
   int len_sub, ret;

   len_sub = strlen(substr);
   ret = strncmp(str + offset, substr, len_sub);

   return ret == 0 ? true : false;
}

extern bool str2_endwith(const char *str, const char *substr, int offset) {
   int len_sub, ret;

   if (offset == -1)
      offset = strlen(str);
   len_sub = strlen(substr);
   ret = strncmp(&str[offset - len_sub], substr, len_sub);

   return ret == 0 ? true : false;

   /*
   ends_with("abcdef", "def", -1);
   0123456
   abcdef0    len = 6, offset = 6      
      def     len = 3
   "abcdef"[6-3] = 'd'.
   */
}

extern void str2_split(list_t *list, const char *str, char delim, char escaper) {
   const char *ini;

   ini = str;
   for (;;) {
      /* these conditions cause a division. */
      if (str[0] == delim || str[0] == '\0' || delim == '\0') {
         if (delim == '\0')
            str++;

         const ptrdiff_t size = str - ini;
         char *buf = salloc(size + 1);

         buf[size] = '\0';
         if (size > 0)
            strncpy(buf, ini, size);
         
         list_push(list, buf, size + 1);
         free(buf);

         if (str[0] == '\0')
            return;
         if (delim != '\0')
            str++;
         ini = str;
      }
      else
      if (str[0] == escaper) {
         if (str[1] == delim)
            str += 2;
         else
            str++;
      }
      else
         str++;
   }
}

extern char *str2_trim(const char *str, char border) {
   char *buf;
   int strsiz;
   const char *ini, *fin;

   strsiz = strlen(str);
   buf = salloc(strsiz + 1);
   
   ini = str;
   while (isspace(*ini))
      ini++;

   if (*ini == '\0') {
      buf[0] = '\0';
      return buf;
   }

   fin = str + strsiz - 1;
   while (isspace(*fin))
      fin--;

   if (*ini == border && *fin == border)
      ini++, fin--;
   
   ptrdiff_t bufsiz;

   bufsiz = fin - ini + 1;
   strncpy(buf, ini, bufsiz);
   buf[bufsiz] = '\0';

   return buf;

   /*
   __abcde__     _ = space
   012345678     ini = 2, fin = 6
   fin - ini = 4, but strlen("abcde") == 5;
   thus a need for "+ 1".
   */
}

extern char *str2_escape(const char *str, char escaper, const char *from, const char *to) {
   /*
   Since this function changes an escape sequence to a character
   (for example, "\p" to 'p'), the length of the result string
   is at most `strlen(str)`, which is the case of `str` having
   no `escaper` at all.
   */
   char *buf;
   int p, q;

   buf = salloc(strlen(str) + 1);
   p = q = 0;
   while (str[p] != '\0')
      if (str[p] == escaper && str2_findch(from, str[p + 1])) {
         ptrdiff_t i = strchr(from, str[p + 1]) - from;
         buf[q++] = to[i];
         p += 2;
      }
      else
         buf[q++] = str[p++];
   buf[q] = '\0';

   return buf;
}

extern char *str2_duplicate(const char *src) {
   char *dest;

   dest = salloc(strlen(src) + 1);
   strcpy(dest, src);

   return dest;
}