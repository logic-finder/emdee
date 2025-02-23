#ifndef READLINE_H
#define READLINE_H

#include <stdio.h>

/**
 * @brief `READLINE_UNIT` is a macro limiting the number of characters to be read in one call of `fgets`.
 */
#define READLINE_UNIT   256

/**
 * @brief `readline` reads a line from a stream `fp`.
 * @param fp a file pointer
 * @return a line which has been read; this value needs to be `free`d.
 * @note In failure, this function stops the program.
 */
char *readline(FILE *fp);

#endif