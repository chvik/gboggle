#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "board_widget.h"
#include "board.h"
#include "boggle.h"
#include "ui.h"
#include "langconf.h"

struct langconf *conf;

#define DICTFILE "dict"

/*******************************************
 * main
 *******************************************/

int main(int argc, char **argv)
{
    GNode *trie;
    const gchar * const *alph;
    
    langconfs = read_langconf ();
    if (!langconfs || langconfs->len == 0)
    {
        g_printf ("No valid language configuration found\n");
        return 1;
    }

    set_language (0);
    init_game ();
        
    gtk_init (&argc, &argv);
    
    create_preferences_dialog (langconfs);
    create_main_window (DEFAULT_BOARD_WIDTH, DEFAULT_BOARD_HEIGHT);

    gtk_main ();

    return 0;
}
            
