#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "misc.h"
#include "readline.h"

extern char *readline(FILE *fp) {
   /*
   Notes about `fgets`:
   (1) It reads N - 1 characters at most.
   (2) Its reading stops if it has read either a newline or EOF.
   (3) It stores the newline character.
   (4) It stores \0 at the end of the string.
   (5) It returns NULL if an error has occurred or it has met
       EOF with no characters having been read yet.
   */
   
   char *buffer, *ret;
   int pos, bufsiz_max;
   bool line_ended;

   bufsiz_max = READLINE_UNIT;
   buffer = salloc(bufsiz_max);
   pos = 0;

   do {
      ret = fgets(buffer + pos, READLINE_UNIT, fp);
      if (!ret && ferror(fp))
         ERR("fgets has returned NULL.");
      
      const int bufsiz = strlen(buffer);
      const bool end_of_line = buffer[bufsiz - 1] == '\n';
      const bool end_of_file = feof(fp) ? true : false;
      const bool overflow_expected = (bufsiz + READLINE_UNIT) > bufsiz_max;

      line_ended = end_of_line || end_of_file;

      if (line_ended)
         break;

      pos += READLINE_UNIT - 1;  /* -1 so as to overwrite the \0. */

      if (!overflow_expected)
         continue;

      bufsiz_max *= 2;
      buffer = realloc(buffer, bufsiz_max);
      if (!buffer)
         ERR("realloc failed!");
   } while (!line_ended);
   
   if (!ret)
      buffer[0] = '\0';
   else
      buffer[strcspn(buffer, "\r\n")] = '\0';
      /*
      Note: strcspn() returns the index of the first occurrence
            of \r or \n.
      */

   return buffer;
}