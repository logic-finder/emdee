#ifndef MISC_H
#define MISC_H

#include <stdio.h>

/**
 * @brief `PROG_NAME` is a macro representing the name of this program.
 */
#define PROG_NAME "emdee"

/**
 * @brief `STR` makes `token` a string.
 * @param token a token to become a string
 */
#define STR(token) #token

/**
 * @brief `CALLSTR` calls `STR`. This macro is useful when `x` is a macro that needs an expansion.
 *    For example, if we used `STR(__LINE__)` instead of `CALLSTR(__LINE__)`, it would not get a
 *    proper macro expansion that we want. (`__LINE__` itself would remain verbatim as a string.)
 *    Therefore we need a wrapper for `STR()`, which is `CALLSTR()`.
 * @param macro a macro to become a string after its expansion
 */
#define CALLSTR(macro) STR(macro)

/**
 * @brief `DIAG` is a macro for representing a diagnostic message.
 */
#define DIAG "at line " CALLSTR(__LINE__) " of file " CALLSTR(__FILE__) "."

/**
 * @brief `YELLOW` is a macro for representing a yellow-colored text.
 * @param text a string to give color to
 */
#define YELLOW(text) "\033[0;33m" #text "\033[0m"

/**
 * @brief `WARN` is a macro for making an easy print of a descriptive warning message.
 * @param msg a warning message to print out
 */
#define WARN(msg, ...) raise_warn(msg "\n    from " YELLOW(%s()) " " DIAG, __VA_ARGS__, __func__)

/**
 * @brief `ERR` is a macro for making an easy print of a descriptive error message.
 * @param msg an error message to print out
 */
#define ERR(msg) raise_err("%s\n    from " YELLOW(%s()) " " DIAG, msg, __func__)

/**
 * @brief `ERR2` is a macro like `ERR`, but it takes a format string.
 * @param msg a format string
 * @param __VA_ARGS__ variable argument(s)
 */
#define ERR2(msg, ...) raise_err(msg "\n    from " YELLOW(%s()) " " DIAG, __VA_ARGS__, __func__)

/**
 * @brief `raise_warn` prints an warning message to `stderr`.
 * @param warn_msg a warning message to print out
 * @param ... variable number of arguments
 */
void raise_warn(const char *warn_msg, ...);

/**
 * @brief `raise_err` prints an error message to `stderr` and gives the program an
 *    abnormal termination, i.e. `abort`.
 * @param err_msg an error message to print out
 * @param ... variable number of arguments
 */
void raise_err(const char *err_msg, ...);

/**
 * @brief `salloc` is a wrapper which calls `malloc` and tests
 *    the return value of it. If failed, the program is `abort`ed.
 * @param size a size to request
 * @return the pointer returned by the `malloc` call
 * @note "salloc" means "safe malloc."
 */
void *salloc(size_t size);

#endif