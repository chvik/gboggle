#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "board_widget.h"
#include "board.h"
#include "boggle.h"
#include "ui.h"
#include "langconf.h"

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
print_path (board *brd, GArray *path)
{
    gint i;

    for (i = 0; path[i]; ++i)
    {
        coord c = path_index (path, i);
        g_printf ("%d %d %s\n", c.x, c.y, 
                  board_gcharp_at (brd, c.x, c.y));
    }
    g_printf ("end of path\n");
}

/*******************************************
 * main
 *******************************************/

int main(int argc, char **argv)
{
    int i, j, k;
    guess_st st;
    GNode *trie;
    GPtrArray *l;

    langconfs = read_langconf();
    set_language (1);

    l = str2letters (alphabet, "kacsa");
    l = str2letters (alphabet, "hosszú");


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
            
