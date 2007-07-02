#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include "boggle.h"

static guess_st
depth_search (letter *guess, const board *brd, gint startx, gint starty,
              GPtrArray *solutions, const coord **path_prefix,
              GNode *trie_node, const gboolean *seen);

void
trie_add (GNode *root, const gchar * const *alphabet, const letter *word)
{
    GNode *child;
    const gchar *p = NULL;

    g_assert (root);
    g_assert (word);

    if (word[0])
        p = letter2gcharp (alphabet, word[0]);

    child = g_node_find_child (root, G_TRAVERSE_ALL, (gpointer)p);
    if (!child)
    {
/*        g_printf ("new node for %s\n", p);*/
        child = g_node_append (root, g_node_new ((gpointer)p));
    }
/*    g_printf ("we're in %s\n", p);*/

    if (word[0])
        trie_add (child, alphabet, word + 1);
}

guess_st
search_solution (GPtrArray *solutions, letter *guess, const board *brd, 
                 gboolean use_trie)
{
    gboolean *seen;
    gint x, y;
    const coord *path;
    guess_st st;
    
    seen = g_new0 (gboolean, brd->width * brd->height);
    path = NULL;
    for (y = 0; y < brd->height; ++y)
        for (x = 0; x < brd->width; ++x)
        {
            st = depth_search (guess, brd, x, y, solutions, &path,
                               use_trie ? brd->trie : NULL, seen);
            g_assert (use_trie || st != not_in_dictionary);
            if (guess && 
                (st == not_in_dictionary || st == good_guess))
            {
                g_free (seen);
                return st;
            }
        }

    g_free (seen);

    if (guess)
        return not_in_board;

    return good_guess;
}

guess_st
depth_search (letter *guess, const board *brd, gint startx, gint starty,
              GPtrArray *solutions, const coord **path_prefix,
              GNode *trie_node, const gboolean *seen)
{
    gint dx, dy;
    guess_st tmpst;
    gboolean found_leaf;
    gint len;
    GNode *child = NULL;
    const coord **new_path_prefix;            
    int i;
    gboolean *new_seen;
    coord *here;
    guess_st retval = not_in_board;
    gboolean found_solution = FALSE;
 
    for (len = 0; path_prefix[len]; ++len);
    g_assert (len >= 0);
    g_assert (len < 16);
    DEBUGSTM (g_printf("depth_search depth: %d from %d %d\n", len, startx, starty));

    if (guess && board_letter_at (brd, startx, starty) != *guess)
        return not_in_board;
    
    DEBUGSTM(g_printf("%s at %d %d\n",
                      board_gcharp_at (brd, startx, starty),
                      startx, starty));

    /* match letter with trie */
    if (trie_node)
    {
        const gchar *p;

        p = board_gcharp_at (brd, startx, starty);
        child = g_node_find_child (trie_node, G_TRAVERSE_ALL, (gpointer)p);
        if (!child)
        {
            return not_in_dictionary;
        }
        DEBUGSTM(g_printf("found in trie\n"));
    }

    /* copy path_prefix into new_path_prefix */
    new_path_prefix = g_new (const coord *, len + 2);
    for (i = 0; i < len; ++i)
    {
        new_path_prefix[i] = path_prefix[i];
    }
    here = g_new (coord, 1);
    here->x = startx;
    here->y = starty;
    new_path_prefix[len] = here;
    new_path_prefix[len + 1] = NULL;

    /* copy seen into new_seen */
    new_seen = g_new (gboolean, brd->width * brd->height);
    for (i = 0; i < brd->height * brd->width; ++i)
        new_seen[i] = seen[i];
    new_seen[brd->height * starty + startx] = TRUE;

    if (trie_node)
    {
        found_leaf = (g_node_find_child (child, G_TRAVERSE_ALL, NULL) !=
                          NULL);
        if (found_leaf && (!guess || !*(guess + 1)))
        {
            DEBUG_PRINTF1 ("leaf found\n");
            found_solution = TRUE;
        }
        if (!found_leaf && guess && !*(guess + 1))
        {
            /* guess is processed, not found in dictionary */
            return not_in_dictionary;
        }
    }
    if (!trie_node && guess && !*(guess + 1))
    {
        /* guess in on the board, not looked up in dictionary */
        found_solution = TRUE;
    }

    if (found_solution)
    {
            coord **path_prefix_copy;
            gint i;

            /* add copy of the path to solutions */
            path_prefix_copy = g_new (coord *, len + 2);
            for (i = 0; i < len + 1; ++i)
            {
                path_prefix_copy[i] = g_new (coord, 1);
                *path_prefix_copy[i] = *new_path_prefix[i];
            }
            path_prefix_copy[len + 1] = NULL;
            g_ptr_array_add (solutions, (gpointer)path_prefix_copy);
            if (guess)
                return good_guess;
    }


    for (dy = -1; dy <= 1; ++dy)
        for (dx = -1; dx <= 1; ++dx)
        {
            gint x, y;

            x = startx + dx;
            y = starty + dy;
            if ((dx == 0 && dy == 0) || x < 0 || y < 0 ||
                 x >= brd->width || y >= brd->height ||
                 seen[y * brd->height + x])
                continue;

            tmpst = depth_search (guess ? guess + 1 : NULL,
                                  brd, x, y,
                                  solutions, new_path_prefix,
                                  child, new_seen);

            if (guess)
            {
                if (tmpst == good_guess)
                {
                    retval = good_guess;
                    break;
                }
                if (tmpst == not_in_dictionary)
                {
                    retval = not_in_dictionary;
                    break;
                }
            }
        } /* for */

    /* free memory allocated by this call */
    g_free (here);
    g_free (new_path_prefix);
    g_free (new_seen);

    if (guess)
        return retval;
        
    return good_guess;
}


gchar *
normalize_guess (const gchar *guess)
{
    gchar *norm;

    norm = g_utf8_strdown (g_utf8_normalize (guess, -1, G_NORMALIZE_ALL), -1);
    return norm;
}

guess_st
process_guess (const gchar *guess, board *brd, GPtrArray *guessed_words,
               guint *word_val)
{
    letter *ltr;
    guess_st st;
    GPtrArray *solutions;
    gint i, len;

    if (g_utf8_strlen (guess, -1) < MIN_WORD_LENGTH)
    {
        return too_short;
    }

    for (i = 0; i < guessed_words->len; ++i)
    {
        if (strcmp((gchar *)g_ptr_array_index (guessed_words, i), guess) == 0)
        {
            return already_guessed;
        }
    }

    g_ptr_array_add (guessed_words, (gpointer)guess);

    ltr = str2letters (brd->alphabet, guess);
    if (!ltr)
        return not_in_board;

    /* check lenght again because of multichar letters (eg. "qu") */
    for (len = 0; ltr[len] && len < MIN_WORD_LENGTH; ++len);
    if (len < MIN_WORD_LENGTH)
    {
        return too_short;
    }
        
    solutions = g_ptr_array_new ();
    st = search_solution (solutions, ltr, brd, TRUE);

    *word_val = 1; /* TODO */

    return st;
       
}


guess_st
find_str_on_board (GPtrArray *paths, const gchar *str, board *brd)
{
    gchar *lower;
    letter *ltr;
    guess_st st;
    
    g_assert (paths);

    lower = g_utf8_strdown (g_utf8_normalize (str, -1, G_NORMALIZE_ALL),
                            -1);
    if (!lower[0])
        return not_in_board;

    ltr = str2letters (brd->alphabet, lower);
    g_free (lower);
    if (!ltr)
        return not_in_board;

    st = search_solution (paths, ltr, brd, FALSE);
    g_assert (st != not_in_dictionary);
    DEBUGSTM (g_printf ("search result: %d\n", st));

    return st;
       
}

#define BUFLENGTH 256

gint
trie_load (GNode *root, const gchar * const *alphabet, const gchar *filename)
{
    FILE *dictf;
    char buf[BUFLENGTH];
    letter *letters;
    gint gcharlen, letterlen;

    dictf = g_fopen (filename, "r");
    if (!dictf)
        return -1;

    while (fgets (buf, BUFLENGTH - 1, dictf))
    {
        gcharlen = strlen (buf);
        if (gcharlen == BUFLENGTH - 1 && buf[BUFLENGTH-2] != '\n')
        {
            /* word truncated */
            continue;
        }
        if (buf[gcharlen-1] == '\n')
        {
            /* remove newline from end */
            buf[gcharlen-1] = 0;
        }
        if (!(letters = str2letters (alphabet, buf)))
            continue;                       
        for (letterlen = 0; letters[letterlen]; ++letterlen);
        if (letterlen < MIN_WORD_LENGTH)
            continue;
        trie_add (root, alphabet, letters);
        g_free (letters);
    }

    return 0;
}

void
solutions_dispose (GPtrArray *solutions)
{
    if (!solutions)
        return;
    g_ptr_array_free (solutions, TRUE);
    /* XXX */
}

