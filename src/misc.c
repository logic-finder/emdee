#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"

extern void *salloc(size_t size) {
   void *p;
   
   p = malloc(size);
   if (p)
      return p;
   
   ERR2("%s", strerror(errno));
}

extern void raise_warn(const char *warn_msg, ...) {
   va_list ap;
   
   va_start(ap, warn_msg);
   fprintf(stderr, "%s: \033[48;5;220;30m WARNING \033[0m ", PROG_NAME);
   vfprintf(stderr, warn_msg, ap);
   va_end(ap);
   fprintf(stderr, "\n");
}

extern void raise_err(const char *err_msg, ...) {
   va_list ap;
   
   va_start(ap, err_msg);
   fprintf(stderr, "%s: \033[101;97m ERROR \033[0m ", PROG_NAME);
   vfprintf(stderr, err_msg, ap);
   va_end(ap);
   fprintf(stderr, "\n");
   abort();
}