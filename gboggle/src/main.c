#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <libintl.h>
#include <locale.h>

#include "appdata.h"
#include "board_widget.h"
#include "board.h"
#include "boggle.h"
#include "ui.h"
#include "langconf.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

AppData app_data;

int main(int argc, char **argv)
{
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);

    app_data.langconfs = read_langconf ();
    if (!app_data.langconfs || app_data.langconfs->len == 0)
    {
        g_printf (_("No valid language configuration found\n"));
        return 1;
    }
        
    gtk_init (&argc, &argv);
    
    create_main_window (DEFAULT_BOARD_WIDTH, DEFAULT_BOARD_HEIGHT);
    create_preferences_dialog ();
    set_language (0);
    init_game ();

    gtk_main ();

    return 0;
}
            
