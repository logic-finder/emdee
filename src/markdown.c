#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "list.adt.h"
#include "string2.h"
#include "markdown.h"

/*
`delim_len_t` is a type for representing the necessary
lengths of delimiters to be translated into each html tag.

`emph_len` is equal to 1, since one asterisk or underbar can open/close an emphasis.
`str_emph_len` is equal to 2, by the similar reason.
`code_len` is equal to 1, by the similar reason.
`strike_len` is equal to 2, by the similar reason.
*/
typedef enum delim_len {
   emph_len = 1, str_emph_len,
   code_len = 1,
   strike_len
} delim_len_t;

/*
`delim_t` is an enumeration type for representing the kind of tokens as each alias.

`text` is for strings consisting of only normal characters, e.g. "I am an otaku.".

`asterisk` is for delimiter runs consisting of only asterisk characters, e.g. "*****".
`underscore` is similar with `asterisk`, but only for underscore characters, e.g. "___".
`backtick` is similar with `asterisk`, but only for backtick characters, e.g. "```".
`tilde` is similar with `asterisk`, but only for tilde characters, e.g. "~~".

`line_start` is a special value to indicate the first token.
`line_end` is similar with `line_start`, but it indicates the last token.
*/
typedef enum delim {
   text,
   asterisk = '*', underscore = '_', backtick, tilde = '~',
   line_start, line_end
} delim_t;

/*
`delim_run_t` is an enumeration type for representing the kinds of delimiter runs.

`none_run` is for delimiter runs such as "abc *** def".
`left_run` is for delimiter runs such as "abc ***def".
`right_run` is for delimiter runs such as "abc*** def".
`both_run` is for delimiter runs such as "abc***def".
*/
typedef enum delim_run {
   none_run, left_run, right_run, both_run
} delim_run_t;

/*
`token_t` is a type for representing tokens.

`run` is the contents of a token, i.e. a string.
`delim_type` represents the kind of this token;
   refer to the explanation of `delim_t` type above.
`delim_count` is, roughly speaking, the length of `run`.
`run_type` is meaningful if and only if this token is a delimiter run;
   refer to the explanation of `delim_run_t` type above.
`active` indicates whether this token can be consumed during a translation process.
   Only active tokens are consumed. For example, even if "*" is of
   `left_run`, if it were set to inactive, it couldn't open an emphasis.
*/
typedef struct token {
   char *run;
   delim_t delim_type;
   int delim_count;
   delim_run_t run_type;
   bool active;
} token_t;

/* Note: a backslash character is used to escape a specific character. */
const char escaper  = '\\';
/* Note: this is a list of characters which can be escaped. */
static const char *escapees = "\\*_~`#";

/**
 * @brief `make_nullstr` returns an empty line.
 * @return `"\0"`
 */
static char *make_nullstr(void);

/**
 * @brief `translate_atxhead` translates a sequence of '#' characters (if any)
 *    to a corresponding header. For example, "# Title" becomes "<h1>Title</h1>".
 * @param line a line to translate into
 * @return a line with its atx-heading resolved
 */
static char *translate_atxhead(const char *line);

/**
 * @brief `lex` tokenizes `line` and stores the result into `tokens`.
 *    For example, "***melody***" becomes ["***", "melody", "***"].
 * @param tokens a list to store the tokens into
 * @param line a line to be `lex`ed
 */
static void lex(list_t *tokens, const char *line);

/**
 * @brief `init_active` initializes the `active` field of all tokens to `true`.
 * @param tokens a list consisting of tokens
 */
static void init_active(list_t *tokens);

/**
 * @brief `translate_cdspan` sees tokens whose `delim_type` is `backtick`.
 *    If such tokens meet the conditions for code spans, they get a translation.
 *    For example, ["``", "back", "`", "trace", "``"] becomes
 *    ["<code>", "back", "`", "trace", "</code>"].
 * @param tokens a list consisting of tokens
 */
static void translate_cdspan(list_t *tokens);

/**
 * @brief `set_runtype` sets `run_type` of tokens by examining
 *    whether `run` is sufficient to be `left_run` and/or `right_run`.
 * @param tokens a list consisting of tokens
 */
static void set_runtype(list_t *tokens);

/**
 * @brief `translate_delims` searches for tokens whose `delim_type` is
 *    `asterisk`, `underscore`, or `tilde`. If such tokens meet the conditions for
 *    emphasis, strong emphasis, or strike, they get a translation.
 *    For example, ["***", "silhouette", "_", "faint", "*", "_"] becomes
 *    ["**", "<em>, "silhouette", "_", "faint", "</em>", "_"].
 * @param tokens a list consisting of tokens
 * @note `backtick` and `text` are not handled by this function.
 *    They are processed by `translate_cdspan` and `translate_escapes`,
 *    respectively.
 */
static void translate_delims(list_t *tokens);

/**
 * @brief `translate_escapes` sees tokens whose `delim_type` is `text`.
 *    Substrings like "\*" are the objects of this translation process.
 *    For example, ["<em>", "\_", "</em>"] becomes ["<em>", "_", "</em>"].
 *    Please refer to the variable `escapee` for the list of characters
 *    which can be escaped.
 * @param tokens a list consisting of tokens
 */
static void translate_escapes(list_t *tokens);

/**
 * @brief `render` gives all tokens a sequential assembly. In other words,
 *    all `run`s are combined into one string. For example,
 *    ["<em>", "<strong>", "melody", "</strong>", "</em>"] becomes
 *    "<em><strong>melody</strong></em>".
 * @param tokens a list consisting of tokens
 * @return a rendered string
 */
static char *render(list_t *tokens);

/**
 * @brief `cleanup` frees memory allocated for `token.run`.
 * @param token a token to clean
 * @param _ This parameter is not used.
 * @note This function is used with `list_foreach`.
 *    That's why it has the unnecessary second parameter.
 */
static void cleanup(void *token, int _);

/**
 * @brief `__print_token__` prints all fields of a token to `stdout`.
 * @param token a token to look inside
 * @param index the index of this token
 * @note (1) This function is for debugging.
 * @note (2) This function can be used with `list_foreach`.
 */
static void __print_token__(void *token, int index);

/**
 * @brief `__print_tokens__` prints all fields of all tokens to `stdout`.
 * @param tokens a list consisting of tokens
 * @note This function is for debugging.
 */
static void __print_tokens__(list_t *tokens);

extern char *translate_markdown(const char *line) {
   if (strlen(line) == 0)
      return make_nullstr();
   
   list_t *tokens;
   char *ret;

   tokens = list_create();
   ret = translate_atxhead(line);
   lex(tokens, ret);
   free(ret);
   init_active(tokens);
   translate_cdspan(tokens);
   set_runtype(tokens);
   translate_delims(tokens);
   translate_escapes(tokens);
   ret = render(tokens);
   list_foreach(tokens, cleanup);
   list_destroy(tokens);

   return ret;
}

static char *make_nullstr(void) {
   char *s = salloc(1);
   s[0] = '\0';
   return s;
}

/**
 * @brief `resolve_atxhead` makes a string like "<h1>asdf</h1>" by using the arguments passed.
 *    For example, in the string "# asdf", `init_pos` points to the 'a' and `final_pos` points to the 'f'.
 *    Since the string has only one '#', `heading_level` is equal to 1.
 * @param init_pos the pointer to the first character of a substring
 * @param final_pos the pointer to the last character of a substring
 * @param heading_level the number of '#' characters
 * @return a resulting string
 * @note if both `init_pos` and `final_pos` are `NULL`, it
 *    returns "<hx></hx>".
 */
static char *resolve_atxhead(const char *init_pos, const char *final_pos, int heading_level);

static char *translate_atxhead(const char *line) {
   /*
   line = "  ###   asdf#  ##   \0"
           ^ ^  ^  ^    ^ ^ ^
           1 2  3  4    5 6 7
   */

   const char *initial,   /* 1 */
              *opener,    /* 2 */
              *precont,   /* 3 */
              *content,   /* 4 */
              *postcont;  /* 5 */
   const char *mover;
   char ch;
   ptrdiff_t diff;
   int sharp_count;

   /* searches for opener */
   initial = mover = line;
   while (ch = *mover, ch == ' ')
      mover++;
   if (ch == '\0')
      return str2_duplicate(line);
   if (mover - initial > 3)
      return str2_duplicate(line);  /* up to 3 spaces are allowed before opener */
   opener = mover;
   
   /* searches for pre-content */
   while (ch = *mover, ch == '#')
      mover++;
   diff = mover - opener;
   if (diff == 0 || diff > 6)
      return str2_duplicate(line);  /* 1 ~ 6 unescaped #s are allowed */
   precont = mover;
   sharp_count = precont - opener;

   /* searches for content */
   while (ch = *mover, ch == ' ' || ch == '\t')
      mover++;
   if (ch == '\0' || ch == '#')
      return resolve_atxhead(NULL, NULL, sharp_count);  /* <hx></hx> */
   if (mover - precont == 0)
      return str2_duplicate(line);   /* at least one whitespace must exist */
   content = mover;
   /* Note: `strlen(content)` is at least 1, since if it were
      zero, then this function would already have ended at
      the `if (ch == '\0')` statement. */

   /* searches for post-content */
   mover = &line[strlen(line) - 1]; /* last character */
   while (ch = *mover, ch == ' ' || ch == '\t' || ch == '#')
      mover--;
   mover++;
   while (ch = *mover, ch == '#')
      mover++;
   postcont = mover;

   return resolve_atxhead(content, postcont, sharp_count);
}

static char *resolve_atxhead(const char *init_pos, const char *final_pos, int heading_level) {
   int len;

   if (!init_pos && !final_pos)
      len = 0;
   else
      len = final_pos - init_pos;

   char *content, *result;

   content = salloc(len + 1);
   strncpy(content, init_pos, len);
   content[len] = '\0';

   result = salloc(4 + len + 5 + 1);   /* <hx> = 4, </hx> = 5 */
   sprintf(result, "<h%d>%s</h%d>", heading_level, content, heading_level);

   return result;
}

/**
 * @brief `push_dummy` pushes a dummy token into `tokens`. Please refer to the `lex` function for the usage of this function.
 * @param delim_type a delimiter type of a dummy token to be pushed
 * @param tokens a list consisting of tokens
 */
static void push_dummy(delim_t delim_type, list_t *tokens);

/**
 * @brief `tokenize` divides `line` into meaningful substrings. Please refer to the definition of this function for the kinds of tokens.
 * @param line a string to be divided into
 * @param init_idx a position to begin a division
 * @param ret_delim_type a pointer to a `delim_t` object to store the delimiter type of a token found.
 * @return It returns the position in which a token ends, if it found a token.
 *    Meanwhile, it returns -1 if it encounters the end of `line` before it begins a division.
 */
static int tokenize(const char *line, int init_idx, delim_t *ret_delim_type);

/**
 * @brief `store_token` copies a substring that begins at `init_idx` and finishes at `final_idx` from `line`
 *    and stores it into `tokens`.
 * @param tokens a list to store a token into
 * @param line a string from which a substring is copied
 * @param init_idx the starting position of a substring
 * @param fianl_idx the ending position of a substring
 * @param delim_type the delimiter type of a substring
 */
static void store_token(list_t *tokens, const char *line, int init_idx, int final_idx, delim_t delim_type);

static void lex(list_t *tokens, const char *line) {
   /*
   the first/last token are "dummy" tokens for representing
   the start/end of a line. Thus they do not have meaningful
   contents in it!
   */
   push_dummy(line_start, tokens);

   /*
   We are going to search the initial/final indexes of a token in the line
   and we repeat this process until we have consumed the whole line.
   The reason we get those indexes is, we need to copy only a part of the line
   so that we can store it in the list. For example, let's say the line is:
      01234567890123456789012
     "*ardent**programming***"
      *                        <- ini =  0, fin = 0
       ardent                  <- ini =  1, fin = 6
             **                <- ini =  7, fin = 8
               programming     <- ini =  9, fin = 19
                          ***  <- ini = 20, fin = 22
   In this example, we've found five tokens in total.
   Therefore, now we have seven tokens in the list because of dummy tokens.
   */

   delim_t delim_type;
   int init_idx, final_idx;

   init_idx = 0;
   for (;;) {
      final_idx = tokenize(line, init_idx, &delim_type);
      if (final_idx == -1)
         break;   /* done! reached the end of line */
      store_token(tokens, line, init_idx, final_idx, delim_type);
      init_idx = final_idx + 1;
   }

   push_dummy(line_end, tokens);
}

static void push_dummy(delim_t delim_type, list_t *tokens) {
   token_t dummy = {0};
   
   dummy.delim_type = delim_type;
   dummy.run = NULL;
   list_push(tokens, &dummy, sizeof dummy);
}

static int tokenize(const char *line, int init_idx, delim_t *ret_delim_type) {
   /*
   This function distinguishes the three kinds of tokens.
      TYPE 1: a delimiter run
      e.g. "*", "___", "~~", "````"
      TYPE 2: a backtick string with a single \ character in front of it
      e.g. "\`", "\```"
      TYPE 3: neither TYPE 1 nor TYPE 2, i.e. a string with normal characters
      e.g. "hungry, pizza in need", "can have escaped delim(s) like \*\_."
   
   It may seem strange that \* is TYPE 3 while \` is TYPE 2, considering
   ` is also a delimiter that can be escaped. It's because we need to handle
   a case like this: "\```no escape?\``". Yes, the second \` is not escaped!
   Since \ is regarded as a text inside a code span. Therefore, the example
   is translated into "`<code>no escape?\</code>". Note that first \` is
   indeed escaped.
   */
   
   const int ini = init_idx;  /* init_idx is the more descriptive name,
                                 but it's 8 chars long, too long to type... */

   if (line[ini] == '\0')
      return -1;  /* end of line; terminates. */

   static const char *delims = "*_~`";
   bool type1, type2, escaped;
   delim_t delim_type;
   int fin;  /* abbr. final_idx */

   type1 = str2_findch(delims, line[ini]);
   type2 = (line[ini] == escaper) && (line[ini + 1] == backtick);
   fin = ini;

   if (type1 || type2) {
      if (type1)
         delim_type = line[ini];
      else
      if (type2) {
         delim_type = backtick;
         fin++;   /* because of '\' */
      }

      while (line[fin + 1] != '\0') {
         if (line[fin + 1] != delim_type)
            break;   /* increases until the next char is not this delim. */
         fin++;
      }
   }
   else {   /* neither TYPE 1 nor TYPE 2, therefore it is TYPE 3 */
      delim_type = text;
      while (line[fin + 1] != '\0') {
         escaped = (line[fin] == escaper) && (str2_findch(escapees, line[fin + 1]));
         if (escaped)
            fin++;   /* because of '\' */
         
         type1 = str2_findch(delims, line[fin + 1]);
         type2 = (line[fin + 1] == escaper) && (line[fin + 2] == backtick);
         if (type1 || type2)
            break;

         fin++;   /* increases until TYPE 1 or TYPE 2. */
      }
   }

   *ret_delim_type = delim_type;
   return fin;
}

static void store_token(list_t *tokens, const char *line, int init_idx, int final_idx, delim_t delim_type) {
   int len = final_idx - init_idx + 1;

   char *run = salloc(len + 1);
   strncpy(run, &line[init_idx], len);
   run[len] = '\0';

   token_t token = {0};

   if (delim_type == backtick && line[init_idx] == escaper)
      len--; /* in order not to count the '\' character. */

   token.run = run;
   token.delim_type = delim_type;
   token.delim_count = len;

   list_push(tokens, &token, sizeof token);
}

static void init_active(list_t *tokens) {
   /* Note: this function doesn't affect the length of the list. */
   const int len = list_size(tokens);
   
   for (int i = 1; i < len - 1; i++) {
      token_t *token = list_peek(tokens, i);
      token->active = true;
   }
}

/**
 * @brief `find_btpair` searches for a pair of matching backtick strings whose `delim_count`s are equal.
 *    For example, if we apply this function to this list with `init_idx` as 0 (the beginning of the list):
 *    ["glacial", "`", "stone()", "`"], the results will be:
 *       - oidx = 1, since "`" is the opener and its index is 1.
 *       - cidx = 3, since "`" is the closer and its index is 3.
 *       - opener = "`"
 *       - closer = "`", since the opener and the closer has the same length.
 * @param tokens a list consisting of tokens
 * @param init_idx a position to begin the search
 * @param ret_oidx a pointer to store the position of an opener
 * @param ret_cidx a pointer to store the position of a closer
 * @param ret_opener a pointer to store the opener
 * @param ret_closer a pointer to store the closer
 * @return It returns 0 on a successful search, i.e. both an opener and a closer have been found.
 *    It returns -1 if it couldn't find an opener.
 *    It returns the index of an opener if it found an opener but couldn't find a closer.
 */
static int find_btpair(list_t *tokens, int init_idx, int *ret_oidx, int *ret_cidx, token_t **ret_opener, token_t **ret_closer);

/**
 * @brief `transform_delim` does this job: from "*cheese*" to "<em>cheese</em>".
 * @param tokens a list consisting of tokens
 * @param closer a pointer to a token which is a closer
 * @param opener a pointer to a token which is a opener
 * @param cidx the index of a closer in `tokens`
 * @param oidx the index of a opener in `tokens`
 * @param closing_tag a string whose content is a closing html tag such as "</em>" or "</strong>"
 * @param opening_tag a string whose content is a opening html tag such as "<em>" or "<strong>"
 * @param delim_len a number indicating how much delimiters are to be consumed
 */
static void transform_delim(list_t *tokens, token_t *closer, token_t *opener, int cidx, int oidx, const char *closing_tag, const char *opening_tag, delim_len_t delim_len);

/**
 * @brief `normalize_cdspan` does this job: from "` spacious? `" to "`spacious?`".
 *    In other words, it eliminates a leading and trailing space if both exist.
 * @param tokens a list consisting of tokens
 * @param oidx the index of a opener in `tokens`
 * @param cidx the index of a closer in `tokens`
 */
static void normalize_cdspan(list_t *tokens, int oidx, int cidx);

static void translate_cdspan(list_t *tokens) {
   /* Note: this function DOES affect the length of the list. */

   static const char *code_closing = "</code>";
   static const char *code_opening = "<code>";

   int idx;

   idx = 0;
   for (;;) {
      int oidx, cidx, ret;
      token_t *opener, *closer;

      ret = find_btpair(tokens, idx, &oidx, &cidx, &opener, &closer);
      if (ret == -1)
         return;   /* end of list */
      if (ret != 0) {
         idx = ret + 1;
         continue; /* an opener found, but no closer found */
      }

      transform_delim(
         tokens,
         closer, opener,
         cidx, oidx,
         code_closing, code_opening,
         closer->delim_count
      );

      /*
      Two things happen in the for statement below:
      (1) inactivates all items inside this code span
          in order to prevent any possible misinterpretation.
          For example, in "`right or *wrong` ?*", the first '*'
          should not be translated into <em>, since it is in the code span.
      (2) checks whether the text inside this code span
          consists of only spaces in order to determine
          whether to apply a normalization or not. (See 6.1)
      */

      int only_spaces;
      
      only_spaces = 1;
      for (int k = oidx + 1; k < cidx; k++) {
         token_t *middle = list_peek(tokens, k);
         middle->active = false;
         only_spaces &= str2_hasonly(middle->run, ' ');
      }

      if (!only_spaces)
         normalize_cdspan(tokens, oidx, cidx);
   }
}

/**
 * @brief `split_btstr` is useful in these cases. First, if the opener is of TYPE 2, e.g. \``,
 *    we need to divide it into two parts: \` (the escaped part) and ` (the actual delimiter run part). Second, if the
 *    closer is of TYPE 2, we need to divide it in the same way too: \ (the text part) and `` (the actual delimiter run part).
 * @param tokens a list consisting of tokens
 * @param token a opener or a closer
 * @param token_idx the index of `token`
 * @param backtick_count the length of the actual delimiter run part
 * @param separation_idx a position where the division happens
 * @param ret_token a pointer to store the updated `token`
 * @param ret_token_idx a pointer to store the updated `token_idx`
 */
static void split_btstr(list_t *tokens, token_t *token, int token_idx, int backtick_count, int separation_idx, token_t **ret_token, int *ret_token_idx);

static int find_btpair(list_t *tokens, int init_idx, int *ret_oidx, int *ret_cidx, token_t **ret_opener, token_t **ret_closer) {
   /* Note: this function DOES affect the length of the list. */
   
   token_t *opener, *closer;
   int oidx, cidx, len;

   len = list_size(tokens);
   for (oidx = init_idx; oidx < len - 1; oidx++) {
      opener = list_peek(tokens, oidx);
      if (opener->delim_type != backtick)
         continue;
      if (opener->delim_count == 1 && opener->run[0] == escaper) {
         opener->delim_type = text;
         continue;   /* "\`" can't be an opener; since it is escaped. */
      }
      break;
   }
   if (oidx == len - 1)
      return -1;  /* no opener found */
   
   /*
   If "\`" is in front of the opener, that part is separated
   from the opener, since "\`" is an escaped '`'. For example,
   "\```" gets a separation into "\`" and "``".
   */
   if (opener->run[0] == escaper) {
      split_btstr(
         tokens, opener,
         oidx, (opener->delim_count - 1), 2,
         &opener, &oidx);
      len = list_size(tokens);
   }
   
   for (cidx = oidx + 1; cidx < len - 1; cidx++) {
      closer = list_peek(tokens, cidx);
      if (closer->delim_type != backtick)
         continue;
      if (opener->delim_count == closer->delim_count)
         break;
   }
   if (cidx == len - 1) {
      opener->active = false;
      return oidx;  /* no closer found */
   }

   /*
   Unlike the opener, if "\" is in front of the closer, only
   that part is separated, since no escape occurs inside code
   spans. For example, "``test\``" is translated into
   "<code>test\</code>".
   */
   if (closer->run[0] == escaper)
      split_btstr(
         tokens, closer,
         cidx, closer->delim_count, 1,
         &closer, &cidx);
   
   *ret_oidx = oidx;
   *ret_cidx = cidx;
   *ret_opener = opener;
   *ret_closer = closer;
   return 0;
}

static void split_btstr(list_t *tokens, token_t *token, int token_idx, int backtick_count, int separation_idx, token_t **ret_token, int *ret_token_idx) {
   token_t separated = {0};
   char *buffer;

   buffer = salloc(backtick_count + 1);
   strcpy(buffer, &token->run[separation_idx]); /* no need to put \0 at the end. */
   separated.run = buffer;
   separated.delim_type = backtick;
   separated.delim_count = backtick_count;

   list_insert(tokens, ++token_idx, &separated, sizeof separated);
   
   token->run[separation_idx] = '\0';
   token->delim_type = text;
   token->delim_count = separation_idx;
   
   *ret_token = list_peek(tokens, token_idx),
   *ret_token_idx = token_idx;
}

/**
 * @brief `update_deltok` updates `token`. For example, in "**fierce*", the opener ("**") is two chars long
 *    while the closer ("*") is one char long. Since this string is translated into "*<em>fierce</em>", the translation
 *    consumes one "*" from the opener/closer. That is, we need to adjust the delimiter run and its length.
 *    This is what this function does. So, by this function, the opener becomes "*" and the closer token is deleted
 *    since there is no delimiter left.
 * @param tokens a list consisting of tokens
 * @param token a token to update (opener or closer)
 * @param delim_len a number indicating how much delimiters are to be consumed
 * @param erasion_idx a position where a deletion happens
 */
static void update_deltok(list_t *tokens, token_t *token, delim_len_t delim_len, int erasion_idx);

/**
 * @brief `insert_txttok` inserts a text token into `tokens` at the position of `insertion_idx`.
 * @param tokens a list consisting of tokens
 * @param content the content of the token to be inserted
 * @param insertion_idx a position where an insertion happens
 */
static void insert_txttok(list_t *tokens, const char *content, int insertion_idx);

static void transform_delim(list_t *tokens, token_t *closer, token_t *opener, int cidx, int oidx, const char *closing_tag, const char *opening_tag, delim_len_t delim_len) {
   /*
   Note: since the closer is after the opener, it may be
   more convenient to process the closer first. Otherwise,
   we should keep track of cidx with respect to the
   change of oidx.
   */

   /* processes the closer. */
   update_deltok(tokens, closer, delim_len, cidx);
   insert_txttok(tokens, closing_tag, cidx);

   /* processes the opener. */
   insert_txttok(tokens, opening_tag, oidx + 1);
   update_deltok(tokens, opener, delim_len, oidx);
}

static void update_deltok(list_t *tokens, token_t *token, delim_len_t delim_len, int erasion_idx) {
   list_statcode_t statcode;

   if (token->delim_count - delim_len == 0) {   /* consumed up */
      free(token->run);
      list_erase(tokens, erasion_idx, NULL);
   }
   else {
      token->delim_count -= delim_len;
      token->run[token->delim_count] = '\0';
   }
}

static void insert_txttok(list_t *tokens, const char *content, int insertion_idx) {
   const int len_content = strlen(content);
   token_t token = {0};
   char *buffer;

   buffer = salloc(len_content + 1);
   strcpy(buffer, content); /* no need to put \0 at the end. */
   token.run = buffer;
   token.delim_type = text;
   token.delim_count = len_content;
   token.active = false;

   list_insert(tokens, insertion_idx, &token, sizeof token);
}

static void normalize_cdspan(list_t *tokens, int oidx, int cidx) {
   token_t *first_text, *last_text;
   bool cond1, cond2;

   /*
   Note: oidx might be equivalent to cidx, e.g. `asdf`,
   which has only one text token between the opener and closer.
   */
   first_text = list_peek(tokens, oidx + 1);
   last_text = list_peek(tokens, cidx - 1);

   cond1 = str2_gowith(first_text->run, " ",  0);
   cond2 = str2_endwith(last_text->run, " ", -1);

   if (!cond1 || !cond2)
      return;

   /* the first and last space get a removal. */
   memmove(&first_text->run[0], &first_text->run[1], strlen(&first_text->run[1]) + 1);
   last_text->run[strlen(last_text->run) - 1] = '\0';
}

static void set_runtype(list_t *tokens) {
   /* Note: this function doesn't affect the length of the list. */

   token_t *tok_curr, *tok_prev, *tok_next;
   bool cond1, cond2a, cond2b;
   bool at_line_start, at_line_end;
   char char_preceding, char_following;

   const int len = list_size(tokens);

   /*
   We iterate through 1 < x < len - 1, since idx = 0 and (len - 1) are
   the first and last token, respectively, which are the indicators for
   line-start and line-end and thus do not have actual data in them.
   */
   for (int idx = 1; idx < len - 1; idx++) {
      tok_curr = list_peek(tokens, idx);
      if (tok_curr->delim_type == text)
         continue;   /* since run_type is only meaningful with delims. */
      tok_prev = list_peek(tokens, idx - 1);
      tok_next = list_peek(tokens, idx + 1);
      
      tok_curr->run_type = none_run;

      at_line_start = (tok_prev->delim_type == line_start ? true : false);
      at_line_end   = (tok_next->delim_type == line_end   ? true : false);
      char_preceding = !at_line_start ? tok_prev->run[tok_prev->delim_count - 1] : -1;
      char_following = !at_line_end   ? tok_next->run[0] : -1;

      cond1  = !at_line_end && !isspace(char_following);
      cond2a = !at_line_end && !ispunct(char_following);
      cond2b = at_line_start || isspace(char_preceding) || ispunct(char_preceding);
      
      if (cond1 && (cond2a || cond2b))
         tok_curr->run_type += left_run;

      cond1  = !at_line_start && !isspace(char_preceding);
      cond2a = !at_line_start && !ispunct(char_preceding);
      cond2b = at_line_end || isspace(char_following) || ispunct(char_following);

      if (cond1 && (cond2a || cond2b))
         tok_curr->run_type += right_run;
   }

   /*
   We prevent intraword emphasis via underscore.
   Note: I think 'both_run' causes intraword emphasis.
   */
   for (int idx = 1; idx < len - 1; idx++) {
      tok_curr = list_peek(tokens, idx);
      if (tok_curr->delim_type != underscore)
         continue;
      if (tok_curr->run_type != both_run)
         continue;

      tok_curr->run_type = none_run;

      /* since this token is of `both_run`, it has a previous token and next token. */
      tok_prev = list_peek(tokens, idx - 1);
      if (ispunct(tok_prev->run[tok_prev->delim_count - 1]))
         tok_curr->run_type += left_run;

      tok_next = list_peek(tokens, idx + 1);
      if (ispunct(tok_next->run[0]))
         tok_curr->run_type += right_run;
   }
}

/*
Note: this is the way `translate_delims` works.

Let 'line' be
  line = **foo* *bar**

The expected result is
  translated = <em><em>foo</em> <em>bar</em></em>

After analyzing the line, the list has six items (excluding line_start and line_end):
    ↓2    ↓5
  **foo* *bar**
  ^1   ^3^4  ^6

The order these items are processed:
  ^3 and the last '*' of ^1
  the first '*' of ^6 and ^4
  ^6 and ^1 (remaining '*' from each token is consumed)
*/

/*
Note: These are the summaries of the rules 9 ~ 16.
      (CommonMark Spec ver. 0.31.2)

Rule  9: Handling of emphasis
Rule 10: Handling of strong emphasis
Rule 11: Handling of multiple asterisks
Rule 12: Handling of multiple underscores
Rule 13: Interpretation of **
Rule 14: Interpretation of ***
Rule 15: First emphasis taking precedence
Rule 16: First opener adjacent to closer taking precedence
*/

/**
 * @brief `transform_delims` searches a suitable closer, i.e. a token whose `active` is true, `delim_type` is `*`, `_`, or `~`,
 *    and `run_type` is `right_run` or `both_run`. If it makes a success search on a closer, it invokes a corresponding function
 *    depending on the `delim_type` for the further processing.
 * @param tokens a list consisting of tokens
 * @param init_idx a position to begin searching
 * @return If it found a suitable opener and closer, it returns an index which the next call to this function is using as `init_idx`.
 *    Meanwhile, if it couldn't find a closer, it returns -1.
 */
static int transform_delims(list_t *tokens, int init_idx);

static void translate_delims(list_t *tokens) {
   /* Note: this function DOES affect the length of the list. */

   int init_idx = 0;

   while (init_idx != -1)
      init_idx = transform_delims(tokens, init_idx);
}

/**
 * @brief `handle_emphases` translates a matching pair of asterisk delimiter runs OR a matching pair of underscores delimiter runs,
 *    if they satisfy the conditions for being translated to emphasis.
 * @param tokens a list consisting of tokens
 * @param closer an `active` token whose `delim_type` is `*` or `_` and `run_type` is `right_run` or `both_run`
 * @param cidx a position of the `closer` in the `tokens`
 * @return 0 on success; -1 if it couldn't find a suitable opener.
 */
static int handle_emphases(list_t *tokens, token_t *closer, int cidx);

/**
 * @brief `handle_strike` translates a matching pair of tilde delimiter runs,
 *    if they satisfy the conditions for being translated to strike.
 * @param tokens a list consisting of tokens
 * @param closer an `active` token whose `delim_type` is `*` or `_` and `run_type` is `right_run` or `both_run`
 * @param cidx a position of the `closer` in the `tokens`
 * @return 0 on success; -1 if it couldn't find a suitable opener.
 */
static int handle_strike(list_t *tokens, token_t *closer, int cidx);

static int transform_delims(list_t *tokens, int init_idx) {
   /* Note: this function DOES affect the length of the list. */
   /* Note: it is under Rule 15 and 16 that the way the following logic is organized. */

   /*
   We searches for the first token which meets:
   (1) token->delim_type IS NOT text.
         (also NEITHER line_start NOR line_end, needless to say.)
   (2) token->run_type IS EITHER right_run OR both_run.
   (3) token->active IS true.
   */

   token_t *closer;
   int cidx;
   
   const int len = list_size(tokens);
   
   for (cidx = init_idx; cidx < len - 1; cidx++) {
      closer = list_peek(tokens, cidx);
      if (closer->delim_type == text)
         continue;
      if (closer->run_type < right_run) /* neither right_run nor both_run */
         continue;
      if (closer->active == true)
         break;
   }
   if (cidx == len - 1)
      return -1; /* no closer found */

   /* Now, we give a translation to the opener/closer which have been found. */
   int ret;

   switch (closer->delim_type) {
      case asterisk:
      case underscore:
         ret = handle_emphases(tokens, closer, cidx);
         break;
      case tilde:
         ret = handle_strike(tokens, closer, cidx);
         break;
   }
   if (ret == -1) {
      /* Note: cidx = the location of the first closer. */
      return cidx + 1;
   }

   return cidx;
}

/**
 * @brief `find_opener` searches a matching opener for `closer`.
 * @param tokens a list consisting of tokens
 * @param closer a closer token
 * @param ret_oidx a pointer to `int` to store the index of an opener found
 * @param ret_opener a pointer to `token_t *` to store the pointer to an opener found
 * @return 0 on success; -1 if it couldn't find a valid opener.
 */
static int find_opener(list_t *tokens, token_t *closer, int *ret_oidx, token_t **ret_opener);

/**
 * @brief `mute_openers` makes them inactive all openers between `cidx` and `oidx`.
 * @param tokens a list consisting of tokens
 * @param cidx a position of a closer in the `tokens`
 * @param oidx a position of an opener in the `tokens`
 */
static void mute_openers(list_t *tokens, int cidx, int oidx);

static int handle_emphases(list_t *tokens, token_t *closer, int cidx) {
   /*
   `emph_kind_t` is an enumeration type for representing
   two kinds of emphases: `emphasis` and `strong_emphasis`.
   */
   typedef enum emph_kind {
      emphasis, strong_emphasis
   } emph_kind_t;

   static const char *emph_closing = "</em>";
   static const char *emph_opening = "<em>";
   static const char *str_emph_closing = "</strong>";
   static const char *str_emph_opening = "<strong>";

   token_t *opener;
   int oidx, ret;

   /* searches for a valid emphasis opener. */
   for (oidx = cidx - 1; /* empty */; oidx--) {
      ret = find_opener(tokens, closer, &oidx, &opener);
      if (ret == -1)
         return -1;

      /*
      In order for the opener to be valid "as emphasis",
      the following conditions also need to hold.
      The following rules are relevant: 9, 10, 11, 12, 13, and 14.
      */

      bool cond1, cond2, cond3;

      cond1 = (closer->run_type == both_run)
            || (opener->run_type == both_run);

      if (!cond1)
         break;

      cond2 = (closer->delim_count % 3 == 0);
      cond3 = (opener->delim_count % 3 == 0);

      if (cond2 && cond3)
         break;

      int len_sum;
         
      len_sum = closer->delim_count + opener->delim_count;
      if (len_sum % 3 != 0)
         break;
   }

   /*
   We inactivate "all openers" between the opener and the closer
   in order to prevent any possible misinterpretation. For
   example, the first two '_'s in "*aa _bb _cc* dd_" get inactive.

   One may find it strange why only openers get inactive. The
   reasons are:
      (1) any closer before this closer is already processed or
          it does not have a matching opener. Therefore, it is
          not different from a text token except they do not have
          escaped characters.
      (2) however, there is a possibility that any opener would
          get matched with any closer after this closer, which is
          the situation that we do not want.
      (3) text tokens are going to be processed by `translate_escapes`,
          which targets "active text" tokens only, so we do not
          want to inactivate them.
   Therefore, we only need to inactivate openers. However, it is
   no problem to inactivate closers as well (if we want).
   */
   mute_openers(tokens, cidx, oidx);

   /* resolves the delimiters. */
   emph_kind_t kind;

   if (closer->delim_count >= 2 && opener->delim_count >= 2)
      kind = strong_emphasis;
   else
      kind = emphasis;

   switch (kind) {
      case emphasis:
         transform_delim(
            tokens, closer, opener, cidx, oidx,
            emph_closing, emph_opening, emph_len
         );
         break;
      case strong_emphasis:
         transform_delim(
            tokens, closer, opener, cidx, oidx,
            str_emph_closing, str_emph_opening, str_emph_len
         );
         break;
   }

   return 0;
}

static int find_opener(list_t *tokens, token_t *closer, int *ret_oidx, token_t **ret_opener) {
   /*
   We searche for the first token which meets:
   (1) token->delim_type IS NOT text.
         (also NEITHER line_start NOR line_end, needless to say.)
   (2) token->run_type IS EITHER left_run OR both_run.
   (3) token->active IS true.
   (4) token->delim_type IS EQUAL TO closer->delim_type.
   */

   int oidx;
   token_t *opener;

   for (oidx = *ret_oidx; oidx >= 1; oidx--) {
      opener = list_peek(tokens, oidx);
      if (opener->delim_type == text)
         continue;
      if (opener->run_type != 1 && opener->run_type != 3)
         continue;
      if (opener->active == false)
         continue;
      if (opener->delim_type != closer->delim_type)
         continue;
      break;
   }
   if (oidx == 0)
      return -1;
   
   *ret_oidx = oidx;
   *ret_opener = opener;
   return 0;
}

static void mute_openers(list_t *tokens, int cidx, int oidx) {
   for (int i = oidx + 1; i < cidx; i++) {
      token_t *between = list_peek(tokens, i);
      if (between->delim_type == text)
         continue;
      if (between->run_type != left_run)
         continue;
      between->active = false;
   }
}

static int handle_strike(list_t *tokens, token_t *closer, int cidx) {
   token_t *opener;
   int oidx, ret;

   for (oidx = cidx - 1; ; oidx--) {
      ret = find_opener(tokens, closer, &oidx, &opener);
      if (ret == -1)
         return -1;

      if (opener->delim_count > 1)
         break;
   }

   mute_openers(tokens, cidx, oidx);

   static const char *strike_closing = "</strike>";
   static const char *strike_opening = "<strike>";

   transform_delim(tokens, closer, opener, cidx, oidx,
      strike_closing, strike_opening, strike_len);

   return 0;
}

static void translate_escapes(list_t *tokens) {
   /* Note: this function doesn't affect the length of the list. */

   const int len = list_size(tokens);

   /* target: "active" & "text" items */
   for (int i = 1; i < len - 1; i++) {
      token_t *token = list_peek(tokens, i);
      if (token->active == false)
         continue;
      if (token->delim_type != text)
         continue;

      char *buffer = str2_escape(token->run, escaper, escapees, escapees);
      free(token->run);
      token->run = buffer;
      token->delim_count = strlen(buffer);
   }
}

static char *render(list_t *tokens) {
   /* This is our last task. */
   /* calculates the whole length of the result string. */

   const int len = list_size(tokens);
   int bufsize = 0;
   token_t *token;

   for (int i = 1; i < len - 1; i++) {
      token = list_peek(tokens, i);
      bufsize += token->delim_count;
   }

   /* fills the buffer. */
   char *buffer;

   buffer = salloc(bufsize + 1);
   buffer[0] = '\0'; /* for strcat; buffer hasn't got an initialization. */
   for (int i = 1; i < len - 1; i++) {
      token = list_peek(tokens, i);
      strcat(buffer, token->run);
   }

   return buffer; /* done! */
}

static void cleanup(void *token, int _) {
   token_t *tok = token;

   if (!tok->run)
      return;
   free(tok->run);
}

static void __print_token__(void *token, int index) {
   const token_t *tok = token;

   printf("\033[48;5;148;30m idx = %d \033[0m\n", index);
   printf("tokens[%d].run = %s\n", index, tok->run ? tok->run : "NULL");
   printf("tokens[%d].delim_type = %d\n", index, tok->delim_type);
   printf("tokens[%d].delim_count = %d\n", index, tok->delim_count);
   printf("tokens[%d].run_type = %d\n", index, tok->run_type);
   printf("tokens[%d].active = %d\n", index, tok->active);
}

static void __print_tokens__(list_t *tokens) {
   printf("The contents of this list:\n");
   list_foreach(tokens, __print_token__);
   printf("\ntokens.length = %d\n", list_size(tokens));
}