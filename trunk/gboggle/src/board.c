#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "board.h"

/* returns random letter from alphabet according to weights */

letter random_letter(const gchar * const *alphabet, const guint *weights)
{
    gint i, c, cml, lb, ub;
    guint len = g_strv_length((gchar **)alphabet);
    guint *cmlweights = g_malloc((len+1) * sizeof(guint));

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
           const guint *weights, GNode *trie)
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

letter *
str2letters (const gchar * const *alphabet, const gchar *str)
{
    const gchar *p = str;
    gint len = strlen ((const char *)str);
    letter *letters = (letter *)g_malloc ((len + 1) * sizeof(gchar));
    gint i;
    
    if (!g_utf8_validate (str, len, NULL))
    {
        g_free (letters);
        return NULL;
    }
    
    i = 0;
    while (*p)
    {
        gunichar unich;
        gchar utf8buf[6];
        gint l;
            
        unich = g_utf8_get_char (p);
        l = g_unichar_to_utf8 (unich, utf8buf);
        utf8buf[l] = 0;
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
            k = g_unichar_to_utf8 (unich2, utf8buf2 + l);
            utf8buf2[l + k] = 0;
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
    
    for (len = 0; path[len]; ++len);
    l = g_malloc ((len + 1) * sizeof (letter));
    for (i = 0; i < len; ++i)
    {
        l[i] = brd->letters[path[i]->y * brd->width + path[i]->x];
    }
    l[len] = 0;
    return l;
}

 
