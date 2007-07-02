#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "board_widget.h"
#include "board.h"
#include "boggle.h"
#include "ui.h"
#include "langconf.h"

const gchar * const test_alphabet[] = { "a", "b", "c", "d", NULL };
guint test_weights[] = { 1, 4, 2, 0 };
const gchar * const en_alphabet[] = { "a", "b", "c", "d", "e", "f", "g", "h",
    "i", "j", "k", "l", "m", "n", "o", "p", "qu", "r", "s", "t", "u", "v", "w",
    "x", "y", "z", NULL };
guint en_weights[] = { /*a*/ 8, 3, 3, 3, /*e*/ 10, 2, 3, 3,
                       /*i*/ 7, 1, 2, /*l*/ 5, 3, 5, /*o*/ 6, /*p*/ 3,
                       /*q*/ /*1*/ 5,
                       /*r*/ 4, 5, 5, /*u*/ 4, 2, 2, 1, 3, 1 };

#define DICTFILE "dict"

/*
 * test functions
 */

static void trie_traverse (GNode *root, gchar **prefix)
{
    GNode *child;
    gchar **newpref;
    gint len = -1;
    
    if (prefix)
        len = g_strv_length (prefix);
    
    /* end of word: NULL data and not a global root */
    if (root->parent && !root->data)
    {
        int i;

        for(i = 0; i < len; ++i)
        {
            g_printf("%s ", prefix[i]);
        }
        g_printf("\n");
        return;
    }

    newpref = (gchar **)g_malloc ((len + 2) * sizeof(gchar *));
    if (root->data)
    {
        int i;
        
        for (i = 0; i < len; ++i)
            newpref[i] = prefix[i];
        newpref[len] = (gchar *)root->data;
    }
    newpref[len + 1] = NULL;
    
    child = root->children;
    while (child)
    {
        trie_traverse (child, newpref);
        child = child->next;
    }
    g_free (newpref);
}
       
static void
print_path (board *brd, coord **path)
{
    gint i;

    for (i = 0; path[i]; ++i)
    {
        g_printf ("%d %d %s\n", path[i]->x, path[i]->y, 
                  board_gcharp_at (brd, path[i]->x, path[i]->y));
    }
    g_printf ("end of path\n");
}

/*******************************************
 * main
 *******************************************/

int main(int argc, char **argv)
{
    int i, j, k;
    GPtrArray *sol;
    guess_st st;

    g_printf (" hello " "bello\n");
    read_langconf();

    /*
    dictionary = g_node_new (NULL);
    alphabet = en_alphabet;
    weights = en_weights;
        
    if (trie_load (dictionary, alphabet, DICTFILE) != 0)
    {
        g_printf ("%s: failed to open\n", DICTFILE);
        return 1;
    }



    gtk_init (&argc, &argv);
    
    create_main_window (DEFAULT_BOARD_WIDTH, DEFAULT_BOARD_HEIGHT);

    gtk_main ();
*/
    return 0;
}
            
