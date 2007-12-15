#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "board.h"


static void
rec_str2letters (GPtrArray *array,
                 GArray *letters,
                 GPtrArray *alphabet,
                 const gchar *str);

/* returns random letter from alphabet according to weights */

letter random_letter(GPtrArray *alphabet, const gint *weights)
{
    gint i, c, cml, lb, ub;
    guint len = alphabet->len;
    gint *cmlweights = g_malloc ((len+1) * sizeof(gint));

    cml = 0;
    cmlweights[0] = 0;
    for(i = 0; i < len; ++i)
    {
        cml += weights[i];
        cmlweights[i+1] = cml;
    }
    c = g_random_int_range(0, cmlweights[len]);
    /* binary search */
    lb = 0;
    ub = len - 1;
    while(lb + 1 < ub)
    {
        int m = (lb + ub) / 2;
        if(c <= cmlweights[m])
        {
            ub = m;
        }
        if(c >= cmlweights[m])
        {
            lb = m;
        }
    }
    return lb + 1;
}
 
board *
board_new (gint width, gint height, GPtrArray *alphabet,
           const gint *weights, GNode *trie)
{
    int i;
    board *brd;

    brd = (board *)g_malloc(sizeof(board));
    brd->letters = (letter *)g_malloc(width * height * sizeof(letter));
    brd->alphabet = alphabet;
    for(i = 0; i < width * height; ++i)
    {
        letter l = random_letter(alphabet, weights);
        brd->letters[i] = l;
    }
    brd->height = height;
    brd->width = width;
    brd->trie = trie;
    return brd;
}

void
board_dispose (board *brd)
{
    if (!brd)
        return;

    g_free (brd->letters);
    g_free (brd);
}

const gchar *letter2gcharp (GPtrArray *alphabet, letter l)
{
    /* letter starts from 1 */
    return g_ptr_array_index (alphabet, l - 1);
}

const gchar *
board_gcharp_at (const board *brd, gint x, gint y)
{
    return letter2gcharp(brd->alphabet, brd->letters[brd->width * y + x]);
}

letter
board_letter_at (const board *brd, gint x, gint y)
{
    return brd->letters[brd->width * y + x];
}

letter
gcharp2letter (GPtrArray *alphabet, const gchar *ch)
{
    letter l = 0;
    
    while (l < alphabet->len && 
            strcmp((const char *)g_ptr_array_index (alphabet, l),
                (const char *)ch) != 0) 
    {
        ++l;
    }
    
    if (l == alphabet->len)
        return 0;  /* not found */

    return l + 1;
}


GPtrArray *
str2letters (GPtrArray *alphabet, const gchar *str)
{
    GPtrArray *array;
    GArray *letters;

    if (!g_utf8_validate (str, -1, NULL))
    {
        return NULL;
    }

    array = g_ptr_array_new ();
    letters = g_array_new (TRUE, TRUE, sizeof(letter));
    rec_str2letters (array, letters, alphabet, str);
    g_array_free (letters, TRUE);
    return array;
}


static void
rec_str2letters (GPtrArray *array, GArray *letters,
                 GPtrArray *alphabet,
                 const gchar *str)
{
    gint i;

    if (strlen(str) == 0)
    {
        /* we are at the end of the string, append letters to array */
        letter *lp;
        GArray *letters2 = g_array_sized_new (TRUE, TRUE, sizeof(letter),
                                              letters->len + 1);
        g_array_append_vals (letters2, letters->data, letters->len);
        lp = (letter *)g_array_free (letters2, FALSE);
        g_ptr_array_add (array, lp);
        return;
    }

    for (i = 0; i < alphabet->len; ++i)
    {
        gchar *lstr = g_ptr_array_index (alphabet, i);
        if (g_str_has_prefix (str, lstr))
        {
            letter l = i+1;
            GArray *letters2 = g_array_sized_new (TRUE, TRUE, sizeof(letter),
                                                  letters->len + 1);
            g_array_append_vals (letters2, letters->data, letters->len);
            g_array_append_val (letters2, l);
            rec_str2letters (array, letters2, alphabet, 
                             str + strlen(lstr));
            g_array_free (letters2, TRUE);
        }
    }
}


letter *
path2letters (const board *brd, GArray *path)
{
    int i, len;
    letter *l;
    
    len = path_length (path);
    l = g_malloc ((len + 1) * sizeof (letter));
    for (i = 0; i < len; ++i)
    {
        coord c = path_index (path, i);
        l[i] = brd->letters[c.y * brd->width + c.x];
    }
    l[len] = 0;
    return l;
}

GArray *
path_new ()
{
    g_array_new (TRUE, TRUE, sizeof (coord));
}

coord
path_index (GArray *path, gint i)
{
    return g_array_index (path, coord, i);
}

void
path_free (GArray *path)
{
    g_array_free (path, TRUE);
}


gint
path_length (GArray *path)
{
    int len;

    return path->len;
}
