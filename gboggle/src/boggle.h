#ifndef __BOGGLE_H__
#define __BOGGLE_H__

#include <glib.h>

#include "board.h"

#define MIN_WORD_LENGTH 3

typedef enum _guess_st guess_st;

enum _guess_st
{
    good_guess,
    not_in_board,
    not_in_dictionary,
    already_guessed,
    too_short
};

/* loads words from file */
gint
trie_load (GNode *root,
           GPtrArray *alphabet,
           const gchar *filename,
           void (*progress_cb) (gdouble, gpointer),
           gpointer cb_data);

void
trie_add (GNode *root, 
          GPtrArray *alphabet,
          const letter *word);

/* checks if guess can be found in dictionary and in board
 * if guess is NULL finds all solutions and returns good_guess */
guess_st
search_solution (GPtrArray *solutions,
                letter *guess,
                const board *brd, 
                gboolean use_trie);

/* returns unique list of missing words with their index in solutions */
void
missing_solutions (GPtrArray  **words,
                   GArray     **sol_index,
                   board       *brd,
                   GPtrArray   *solutions,
                   GPtrArray   *found_words,
                   GTree 	   *all_words);

gchar *
normalize_guess (const gchar *guess);

/* returns value of word in word_val
 * guess needs to be normalized */
guess_st
process_guess (const gchar *guess,
               board *brd,
               GPtrArray *guessed_words,
               gint *word_val);

guess_st
find_str_on_board (GPtrArray *paths,
                   const gchar *str,
                   board *brd);

void
solutions_dispose (GPtrArray *solutions);

#endif /* __BOGGLE_H__ */
