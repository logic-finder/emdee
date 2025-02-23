#ifndef STRING2_H
#define STRING2_H

#include <stdbool.h>
#include "list.adt.h"

/**
 * @brief `str2_findch` checks whether `ch` appears in `scanset`.
 * @param scanset an array of characters
 * @param ch a character
 * @return If appears, `true`; otherwise, `false`.
 */
bool str2_findch(const char *scanset, char ch);

/**
 * @brief `str2_hasonly` checks whether `str` consists only of `ch`.
 * @param str a string to test
 * @param ch a character to test
 * @return If `ch` is the only character which appears in `str`, it returns 1.
 *    Otherwise, it returns 0.
 * @note If `str` is empty, this function returns 1, which is (vacuously) true.
 */  
int str2_hasonly(const char *str, char ch);

/**
 * @brief `str2_gowith` checks whether `str` begins with `substr`.
 * @param str a string to test
 * @param substr a string to find in `str`
 * @param offset a position to begin a search; range: 0 <= offset < str.length
 * @return If `str` begins with `substr`, then it returns `true`.
 *    Otherwise, it returns `false`.
 */
bool str2_gowith(const char *str, const char *substr, int offset);

/**
 * @brief `str2_endwith` checks whether `str` ends with `substr`.
 * @param str a string to test
 * @param substr a string to find in `str`
 * @param offset a position of a next character of a character at which a search ends; range: `-1` <= offset < str.length. See also the note below.
 * @return If `str` ends with `substr`, it returns `true`.
 *    Otherwise, it returns `false`.
 * @note In the `offset` parameter, -1 is a special value which represents the null-terminating character.
 */
bool str2_endwith(const char *str, const char *substr, int offset);

/**
 * @brief `str2_split` divides `str` into its components against `delim`.
 *    If `delim` is equal to `\0`, `str` is divided into each character.
 *    Meanwhile, if a `delim` is escaped by an `escaper`, that `delim`
 *    does not make a division. (`escaper` is only effective when
 *    `delim` is not `\0`.)
 * @param list a list to store the result into
 * @param str a string to split into
 * @param delim a delimiter character causing a division
 * @param escaper a character causing an escape
 */
void str2_split(list_t *list, const char *str, char delim, char escaper);

/**
 * @brief `str2_trim` erases the leading and trailing whitespaces from `str`.
 *    After trimming, if the first and last characters are equal to `border`,
 *    that characters gets a trim too.
 * @param str a string to get a removal of leading/trailing whitespaces
 * @param border a character
 * @return a trimmed string
 */
char *str2_trim(const char *str, char border);

/**
 * @brief `str2_escape` gives `str` one-to-one translations based on `from` and `to`
 *    if an `escaper` is in front of any charcater in `from`.
 *    For example, given `escaper` = '\', `from` = "pq", `to` = "rs",
 *    and `str` = "pq\p\q", the translated result is "pqrs" since "\p"
 *    becomes 'r' and "\q" becomes 's'.
 * @param str a string having escaped characters
 * @param escaper a character causing an escape
 * @param from an array of characters which can be escaped
 * @param to an array of characters into which escaped characters are translated
 * @return a string into which all escaped characters in `str` have been translated
 */
char *str2_escape(const char *str, char escaper, const char *from, const char *to);

/**
 * @brief `str2_duplicate` makes a `malloc`ed copy of `src`.
 * @param src a source string
 * @return a string copied from `src`
 */
char *str2_duplicate(const char *src);

#endif