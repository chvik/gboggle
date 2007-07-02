#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "board_widget.h"
#include "board.h"
#include "boggle.h"
#include "ui.h"
#include "langconf.h"

const gchar * const en_alphabet[] = { "a", "b", "c", "d", "e", "f", "g", "h",
    "i", "j", "k", "l", "m", "n", "o", "p", "qu", "r", "s", "t", "u", "v", "w",
    "x", "y", "z", NULL };
guint en_weights[] = { /*a*/ 8, 3, 3, 3, /*e*/ 10, 2, 3, 3,
                       /*i*/ 7, 1, 2, /*l*/ 5, 3, 5, /*o*/ 6, 3,
                       /*q*/ /*1*/ 5,
                       /*r*/ 4, 5, 5, /*u*/ 4, 2, 2, 1, 3, 1 };

struct langconf *conf;

#define DICTFILE "dict"

/*******************************************
 * main
 *******************************************/

int main(int argc, char **argv)
{
    GNode *trie;
    const gchar * const *alph;
    
    GPtrArray *confs;
   
    confs = read_langconf ();
    if (!confs || confs->len == 0)
    {
        g_printf ("No valid language configuration found\n");
        return 1;
    }

    conf = (struct langconf *)g_ptr_array_index(confs, 0);
    trie = g_node_new (NULL);
    alph = (const gchar * const *)conf->alphabet->pdata;
    init_game (trie, alph, (guint *)conf->weights->data);
        
    if (trie_load (trie, alph, conf->dictf) != 0)
    {
        g_printf ("%s: failed to open\n", conf->dictf);
        return 1;
    }

    gtk_init (&argc, &argv);
    
    create_main_window (DEFAULT_BOARD_WIDTH, DEFAULT_BOARD_HEIGHT);

    gtk_main ();

    return 0;
}
            
