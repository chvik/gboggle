#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include "boggle.h"

/*
 * Static declarations
 */

static guess_st
depth_search (letter *guess, const board *brd, gint startx, gint starty,
              GPtrArray *solutions, GArray *path_prefix,
              GNode *trie_node, const gboolean *seen);

static gint
str_compare (gconstpointer a, gconstpointer b, gpointer data);

static gboolean
append_word (gpointer word, gpointer index, gpointer words);

static gboolean
append_index (gpointer word, gpointer index, gpointer sol_index);

/*
 * Definitions
 */

void
trie_add (GNode *root, GPtrArray *alphabet, const letter *word)
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

/* returns missing solutions in alphabetical order in words
 * and their index of solutions in sol_index */
void
missing_solutions (GPtrArray **words, GArray **sol_index,
                   board *brd, GPtrArray *solutions, GPtrArray *found_words)
{
    GTree *soltree;
    gint i;

    soltree = g_tree_new_full ((GCompareDataFunc)str_compare,
                               NULL, NULL, NULL);
    for (i = 0; i < solutions->len; ++i)
    {
        GArray *path;
        gchar *word;
        gint len;
        gint j;
        
        path = (GArray *)g_ptr_array_index (solutions, i);
        len = path_length (path);
        word = g_new0 (gchar , len * 6 + 1);
        /* utf8 wide chars cannot exceed 6 bytes */
        for (j = 0; j < len; ++j)
        {
            const gchar *chp;
            coord c;

            c = path_index (path, j);
            chp = board_gcharp_at (brd, c.x, c.y);
            strncat (word, chp, len * 6 - strlen (word));
        }
        if (!g_tree_lookup (soltree, (gconstpointer)word))
        {
            guint *index;

            index = g_new (guint, 1);
            *index = i;
            g_tree_insert (soltree, (gpointer)word, (gpointer)index);
        }
    }

    for (i = 0; i < found_words->len; ++i)
    {
        gchar *fw;
        gpointer key, val;

        fw = g_ptr_array_index (found_words, i);
        if(g_tree_lookup_extended (soltree, (gconstpointer)fw,
                                   &key, &val))
        {
            g_tree_remove (soltree, (gconstpointer)fw);
            g_free (key);
            g_free (val);
        }
    }

    *words = g_ptr_array_new ();
    *sol_index = g_array_new (FALSE, FALSE, sizeof (guint));
    g_tree_foreach (soltree, (GTraverseFunc)append_word,
                   (gpointer)*words);
    g_tree_foreach (soltree, (GTraverseFunc)append_index,
                    (gpointer)*sol_index);
    g_tree_destroy (soltree);
}


guess_st
search_solution (GPtrArray *solutions, letter *guess, const board *brd, 
                 gboolean use_trie)
{
    gboolean *seen;
    gint x, y;
    GArray *path;
    guess_st st;
    
    seen = g_new0 (gboolean, brd->width * brd->height);
    path = path_new ();
    for (y = 0; y < brd->height; ++y)
        for (x = 0; x < brd->width; ++x)
        {
            st = depth_search (guess, brd, x, y, solutions, path,
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
              GPtrArray *solutions, GArray *path_prefix,
              GNode *trie_node, const gboolean *seen)
{
    gint dx, dy;
    guess_st tmpst;
    gboolean found_leaf;
    gint len;
    GNode *child = NULL;
    GArray *new_path_prefix;
    int i;
    gboolean *new_seen;
    coord here;
    guess_st retval = not_in_board;
    gboolean found_solution = FALSE;
 
    len = path_length (path_prefix);
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
    new_path_prefix = path_new ();
    for (i = 0; i < len; ++i)
    {
        coord c = path_index (path_prefix, i);
        g_array_append_val (new_path_prefix, c);
    }
    here.x = startx;
    here.y = starty;
    g_array_append_val (new_path_prefix, here);

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
            GArray *path_prefix_copy;
            gint i;

            /* add copy of the path to solutions */
            path_prefix_copy = path_new ();
            for (i = 0; i < len + 1; ++i)
            {
                coord c = path_index (new_path_prefix, i);
                g_array_append_val (path_prefix_copy, c);
            }
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
    path_free (new_path_prefix);
    g_free (new_seen);

    if (guess)
        return retval;
        
    return good_guess;
}


gchar *
normalize_guess (const gchar *guess)
{
    gchar *norm;

    norm = g_utf8_strdown (g_utf8_normalize (guess, -1, G_NORMALIZE_DEFAULT_COMPOSE), -1);
    return norm;
}

guess_st
process_guess (const gchar *guess, board *brd, GPtrArray *guessed_words,
               gint *word_val)
{
    guess_st st;
    GPtrArray *solutions;
    GPtrArray *letters_arr;
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

    letters_arr = str2letters (brd->alphabet, guess);
    if (!letters_arr || letters_arr->len == 0)
        return not_in_board;

    st = not_in_board;
    for (i = 0; i < letters_arr->len; ++i)
    {
        letter *ltr = g_ptr_array_index (letters_arr, i);
        guess_st tmpst;
        
        /* check length again because of multichar letters (eg. "qu") */
        for (len = 0; ltr[len] && len < MIN_WORD_LENGTH; ++len);
        if (len < MIN_WORD_LENGTH)
        {
            return too_short;
        }
        
        solutions = g_ptr_array_new ();
        tmpst = search_solution (solutions, ltr, brd, TRUE);

        if (tmpst == good_guess)
        {
            *word_val = 1; /* TODO */
            return tmpst;
        }
        if (tmpst == not_in_dictionary)
        {
            st = tmpst;
        }
    }
    return st;
}


guess_st
find_str_on_board (GPtrArray *paths, const gchar *str, board *brd)
{
    gchar *lower;
    letter *ltr;
    GPtrArray *ltr_arr;
    guess_st st;
    gint i;
    
    g_assert (paths);

    lower = g_utf8_strdown (g_utf8_normalize (str, -1,
                                              G_NORMALIZE_DEFAULT_COMPOSE),
                            -1);
    if (!lower[0])
        return not_in_board;

    ltr_arr = str2letters (brd->alphabet, lower);
    g_free (lower);
    if (!ltr_arr || ltr_arr->len == 0)
        return not_in_board;

    for (i = 0; i < ltr_arr->len; ++i)
    {
        ltr = g_ptr_array_index (ltr_arr, i);
        st = search_solution (paths, ltr, brd, FALSE);
        g_assert (st != not_in_dictionary);
        DEBUGSTM (g_printf ("search result: %d\n", st));
        if (st == good_guess)
            return st;
    }
    return st;
}

#define BUFLENGTH 256

gint
trie_load (GNode *root, GPtrArray *alphabet, const gchar *filename, 
        void (*progress_cb) (gdouble, gpointer), gpointer cb_data)
{
    FILE *dictf;
    char buf[BUFLENGTH];
    letter *letters;
    gint gcharlen, letterlen;
    GPtrArray *letters_arr;
    glong filelen;
    gint word_count = 0;

    dictf = g_fopen (filename, "r");
    if (!dictf)
        return -1;

    fseek (dictf, 0, SEEK_END);    
    filelen = ftell (dictf);
    fseek (dictf, 0, SEEK_SET);

    while (fgets (buf, BUFLENGTH - 1, dictf))
    {
        gint i;

        ++word_count;
        /* call progress_cb at every 1000th word */
        if (progress_cb && word_count % 1000 == 0)
        {
            glong pos = ftell (dictf);
            progress_cb ((gdouble)pos / (gdouble)filelen, cb_data);
        }

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
        letters_arr = str2letters (alphabet, buf);
        if (!letters_arr || letters_arr->len == 0)
            continue;                       
        /* XXX
         * put all possible letter decomposition into trie
         */
        for (i = 0; i < letters_arr->len; ++i)
        {
            letters = g_ptr_array_index (letters_arr, i);
            for (letterlen = 0; letters[letterlen]; ++letterlen);
            if (letterlen < MIN_WORD_LENGTH)
                continue;
            trie_add (root, alphabet, letters);
        }
        g_ptr_array_free (letters_arr, TRUE);
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

gint
str_compare (gconstpointer a, gconstpointer b, gpointer data)
{
    return g_utf8_collate (a, b);
}

gboolean
append_word (gpointer word, gpointer index, gpointer words)
{
    g_ptr_array_add (words, word);
    return FALSE;
}

gboolean
append_index (gpointer word, gpointer index, gpointer sol_index)
{
    g_array_append_val (sol_index, *(guint *)index);
    return FALSE;
}


