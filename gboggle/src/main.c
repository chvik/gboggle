#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "appdata.h"
#include "board_widget.h"
#include "board.h"
#include "boggle.h"
#include "ui.h"
#include "langconf.h"


AppData app_data;

int main(int argc, char **argv)
{
    app_data.langconfs = read_langconf ();
    if (!app_data.langconfs || app_data.langconfs->len == 0)
    {
        g_printf ("No valid language configuration found\n");
        return 1;
    }

    set_language (0);
    init_game ();
        
    gtk_init (&argc, &argv);
    
    create_preferences_dialog ();
    create_main_window (DEFAULT_BOARD_WIDTH, DEFAULT_BOARD_HEIGHT);

    gtk_main ();

    return 0;
}
            
