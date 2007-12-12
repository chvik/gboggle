#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "board.h"


static void
rec_str2letters (GPtrArray *array,
                 GArray *letters,
                 const gchar * const *alphabet,
                 const gchar *str);

/* returns random letter from alphabet according to weights */

letter random_letter(const gchar * const *alphabet, const gint *weights)
{
    gint i, c, cml, lb, ub;
    guint len = g_strv_length((gchar **)alphabet);
    gint *cmlweights = g_malloc((len+1) * sizeof(gint));

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
board_new (gint width, gint height, const gchar * const *alphabet,
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
/*        g_printf("%s\n", alphabet[l]);*/
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

const gchar *letter2gcharp(const gchar * const *alphabet, letter l)
{
    /* letter starts from 1 */
    return alphabet[l - 1];
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
gcharp2letter (const gchar * const *alphabet, const gchar *ch)
{
    letter l = 0;
    
    while (alphabet[l] && strcmp((const char *)alphabet[l],
                                 (const char *)ch) != 0) 
    {
        ++l;
    }
    
    if (!alphabet[l])
        return 0;  /* not found */

    return l + 1;
}


GPtrArray *
str2letters (const gchar * const *alphabet, const gchar *str)
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
                 const gchar * const *alphabet,
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

    for (i = 0; alphabet[i]; ++i)
    {
        if (g_str_has_prefix (str, alphabet[i]))
        {
            letter l = i+1;
            GArray *letters2 = g_array_sized_new (TRUE, TRUE, sizeof(letter),
                                                  letters->len + 1);
            g_array_append_vals (letters2, letters->data, letters->len);
            g_array_append_val (letters2, l);
            rec_str2letters (array, letters2, alphabet, 
                             str + strlen(alphabet[i]));
            g_array_free (letters2, TRUE);
        }
    }
}


letter *
str2letters_ (const gchar * const *alphabet, const gchar *str)
{
    const gchar *p = str;
    gint len = strlen ((const char *)str);
    letter *letters = (letter *)g_malloc ((len + 1) * sizeof(gchar));
    GPtrArray *array = g_ptr_array_new();
    gint i;
    
    if (!g_utf8_validate (str, len, NULL))
    {
        g_free (letters);
        g_ptr_array_free (array, TRUE);
        return NULL;
    }
    
    i = 0;
    while (*p)
    {
        gunichar unich;
        gchar utf8buf[6];
        gint len;
        gint i;
            
        unich = g_utf8_get_char (p);
        len = g_unichar_to_utf8 (unich, utf8buf);
        utf8buf[len] = 0;
/*        g_printf ("utf8buf %d %c\n", l, *p);*/
        letters[i] = gcharp2letter (alphabet, utf8buf);        
/*        g_printf ("letter %d\n", letters[i]);*/
        
        if (!letters[i])
        {
            /* read ahead, check if it's a 2-gram letter */
            gchar *q;
            gchar utf8buf2[12];
            gint k;
            gunichar unich2;
            
            q = g_utf8_next_char (p);
            unich2 = g_utf8_get_char (q);
            strcpy (utf8buf2, utf8buf);
            k = g_unichar_to_utf8 (unich2, utf8buf2 + len);
            utf8buf2[len + k] = 0;
            letters[i] = gcharp2letter (alphabet, utf8buf2);

            if(!letters[i])
            {
                g_free (letters);
                return NULL;
            }
            p = q;
        }

        p = g_utf8_next_char (p);
        ++i;
    }
    letters[i] = 0;

    return letters;
} 


letter *
coords2letters (const board *brd, const coord **path)
{
    int i, len;
    letter *l;
    
    len = coords_length (path);
    l = g_malloc ((len + 1) * sizeof (letter));
    for (i = 0; i < len; ++i)
    {
        l[i] = brd->letters[path[i]->y * brd->width + path[i]->x];
    }
    l[len] = 0;
    return l;
}

 
void
coords_free (coord **path)
{
    int i;

    for (i = 0; path[i]; ++i)
        g_free (path[i]);

    g_free (path);
}


gint
coords_length (const coord **path)
{
    int len;

    for (len = 0; path[len]; ++len);
    return len;
}
