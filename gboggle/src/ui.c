#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>
#include <libintl.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_MAEMO
#include <hildon/hildon.h>
#endif

#include "ui.h"
#include "appdata.h"
#include "board_widget.h"
#include "board.h"
#include "boggle.h"
#include "langconf.h"
#include "util.h"

/*
 * constants
 */

#ifdef HAVE_MAEMO
#define VPAD 5
#define ICON_BUTTON_SIZE 50
#define BUTTON_ICON_SIZE GTK_ICON_SIZE_MENU
#ifdef     HAVE_DIABLO
#define    GUESS_ENTRY_WIDTH 10
#else
#define    GUESS_ENTRY_WIDTH 12
#endif // HAVE_DIABLO
#else
#define VPAD 12
#define GUESS_ENTRY_WIDTH 10
#define BUTTON_ICON_SIZE GTK_ICON_SIZE_BUTTON
#endif // HAVE_MAEMO

#define HPAD 12
#define GUESS_HPAD 3
#define LIST_HEIGHT (DEFAULT_BOARD_HEIGHT * FIELDH)
#define LIST_WIDTH 200 

#define SCORE_FORMAT _("Found: %d of %d")
#define FINAL_SCORE_FORMAT _("Found: %d of %d")
#define TIME_FORMAT _("Time: %d:%02d")

/* static function declarations */
static void submit_guess (gboolean keep_state);
static void create_progress_dialog (GtkWidget **dialog, GtkWidget **pbar);
static void reset_score_label (GtkWidget *label);
static GtkWidget *create_main_menu (void);

/*
 * callbacks
 */

static void
guess_changed (GtkEditable *editable, gpointer data)
{
    const gchar *guess;
    guess_st st;
    GPtrArray *paths;
    GArray *path;
 
    board_widget_initbg (BOARD_WIDGET (app_data.board_widget));

    guess = gtk_entry_get_text (GTK_ENTRY (editable));
    DEBUGMSG ("guess %s\n", guess);
    paths = g_ptr_array_new ();
    st = find_str_on_board (paths, guess, app_data.brd);
    
    if (st == good_guess)
    {
        path = g_ptr_array_remove_index (paths, 0);
        mark_path (path);
        if (app_data.current_path)
            path_free (app_data.current_path);
        app_data.current_path = path;
    }
    else
    {
        if (app_data.current_path)
            path_free (app_data.current_path);
         app_data.current_path = NULL;
    }
    
    while (paths->len) {
        GArray *path = g_ptr_array_remove_index (paths, 0);
        path_free (path);
    }
    g_ptr_array_free (paths, TRUE);

    DEBUGMSG ("status %d\n", st);
}

static gboolean
guess_keypressed (GtkEditable *editable, GdkEventKey *event, gpointer data)
{
    switch (event->keyval) {
        case GDK_Return:
        case GDK_KP_Enter:
            submit_guess (FALSE);
            return TRUE;
        case GDK_space:
			// If the space button is pressed,
			// keep the current state of the board so the user can continue
			// entering from where they left off, for example, typing
			// a singular word then simply typing s-Enter to submit its
			// plural form.
        	submit_guess(TRUE);
        	return TRUE;
        default:
            return FALSE;
    }
}

static void
guess_submit_clicked (GtkButton *button, gpointer data)
{
    submit_guess (FALSE);
    gtk_widget_grab_focus (app_data.guess_entry);
}

static void
guess_del_clicked (GtkButton *button, gpointer data)
{
    GArray *path = app_data.current_path;
    gint len;
    gint i;
    const gchar *str;
    gint pos = 0;

    if (!path)
        return;

    len = path_length (path);
    if (len > 0)
    {
        g_array_remove_index (path, len - 1);
        len = path_length (path);
    }

    board_widget_initbg (BOARD_WIDGET (app_data.board_widget));
    mark_path (path);

    g_signal_handlers_block_by_func (app_data.guess_entry, guess_changed, 
            NULL);
    gtk_editable_delete_text (GTK_EDITABLE (app_data.guess_entry), 0, -1);
    for (i = 0; i < len; ++i)
    {
        coord c = path_index (path, i);

        str = board_gcharp_at (app_data.brd, c.x, c.y);
        gtk_editable_insert_text (GTK_EDITABLE (app_data.guess_entry),
                str, -1, &pos);
    }
    g_signal_handlers_unblock_by_func (app_data.guess_entry, guess_changed, 
            NULL);
    gtk_widget_grab_focus (app_data.guess_entry);
    gtk_editable_select_region (GTK_EDITABLE (app_data.guess_entry), -1, -1);
}

static void
game_stop_button_clicked (GtkButton *button, gpointer data)
{
	stop_game();
}

static void
solutions_tree_view_changed (GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    guint sol_index;
    GArray *path;

    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        return;
    }
    
    gtk_tree_model_get (model, &iter, 1, &sol_index, -1);
    path = (GArray *)g_ptr_array_index (app_data.solutions, sol_index);
    board_widget_initbg (BOARD_WIDGET (app_data.board_widget));
    mark_path (path);
}

static void
new_game_button_clicked (GtkButton *button, gpointer user_data)
{
    start_game ();
}

static void
game_new_callback (GtkMenuItem *menu_item, gpointer user_data)
{
    if (app_data.timer_tag)
    {
        /* during game */
        g_source_remove (app_data.timer_tag);
        g_signal_handler_disconnect (app_data.guess_entry,
                app_data.guess_changed_id);
        g_signal_handler_disconnect (app_data.guess_entry, 
                app_data.guess_keypressed_id);
        gtk_entry_set_text (GTK_ENTRY (app_data.guess_entry), "");
    }
    start_game ();
}

static void
game_end_callback (GtkMenuItem *menu_item, gpointer user_data)
{
    stop_game ();
}

static void
preferences_callback (GtkMenuItem *menu_item, gpointer user_data)
{
    gint l;
    gint result;
    gint new_game_time;
    GBoggleSettings settings = { .language = NULL };
    
    gtk_combo_box_set_active (GTK_COMBO_BOX (app_data.lang_combo),
            app_data.sel_lang);
    result = gtk_dialog_run (GTK_DIALOG (app_data.preferences_dialog));
    gtk_widget_hide (app_data.preferences_dialog);
    if (result != GTK_RESPONSE_OK)
        return;

    while (gtk_events_pending ())
        gtk_main_iteration();


    l = gtk_combo_box_get_active (GTK_COMBO_BOX (app_data.lang_combo));
    DEBUGMSG ("lang %d selected\n", l);
    struct langconf *conf = 
        (struct langconf *) g_ptr_array_index (app_data.langconfs, 
                                               l);
    settings.language = conf->lang;
    
    settings.game_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON ( app_data.time_pref_entry) );
    app_data.game_time = settings.game_time;
    
    save_settings(&settings);

    if (l != app_data.sel_lang)
    {
        if (set_language (l))
        {
            update_title ();
        }
        else
        {
            DEBUGMSG ("lang set to %d failed, revert to %d\n", l,
                    app_data.sel_lang);
        }
    }
}

static void
field_pressed_callback (GtkWidget *widget, coord *cp, gpointer data)
{
    coord last_field;
    GArray *path;
    gint len;
    gint i;
    gint pos;
    const gchar *str;

    DEBUGMSG ("pressed: %d %d\n", cp->x, cp->y);
    if (!app_data.current_path || 
            path_length (app_data.current_path) == 0) {
        path = path_new ();
        g_array_append_val (path, *cp);
    } else {
        len = path_length (app_data.current_path);
        last_field = path_index (app_data.current_path, len - 1);
        if (abs (last_field.x - cp->x) > 1 ||
            abs (last_field.y - cp->y) > 1)
        {
            /* illegal field */
            DEBUGMSG("not a neighbour");
            return;
        }
        for (i = 0; i < len; ++i)
        {
            coord p = path_index (app_data.current_path, i);

            if (p.x == cp->x && p.y == cp->y)
            {
                /* field is already marked */
                return;
            }
        }

        path = path_new ();
        for (i = 0; i < len; ++i)
        {
            coord c = path_index (app_data.current_path, i);
            g_array_append_val (path, c);
        }
        g_array_append_val (path, *cp);
        path_free (app_data.current_path);
    }

    board_widget_initbg (BOARD_WIDGET (app_data.board_widget));
    mark_path (path);
    app_data.current_path = path;

    str = board_gcharp_at (app_data.brd, cp->x, cp->y);
    g_signal_handlers_block_by_func (app_data.guess_entry, guess_changed, 
            NULL);
    gtk_entry_set_position (GTK_ENTRY (app_data.guess_entry), -1);
    pos = gtk_editable_get_position (GTK_EDITABLE (app_data.guess_entry));
    gtk_editable_insert_text (GTK_EDITABLE (app_data.guess_entry),
            str, -1, &pos);
    gtk_editable_set_position (GTK_EDITABLE (app_data.guess_entry), pos);
    g_signal_handlers_unblock_by_func (app_data.guess_entry, guess_changed, 
            NULL);
}

static gboolean
timer_func (gpointer data)
{
    gint min, sec;
    GTimeVal curr_time;
    gchar buf[16];

    g_get_current_time (&curr_time);
    // Use variable game time instead of static one
    // so it can be set by the user.
    //sec = GAME_LENGTH_SEC - (curr_time.tv_sec - app_data.game_start.tv_sec);
    sec = app_data.game_time - (curr_time.tv_sec - app_data.game_start.tv_sec);
    if (sec < 0)
        sec = 0;
    min = sec / 60;
    sec %= 60;
    g_snprintf (buf, sizeof (buf), TIME_FORMAT, min, sec);
    gtk_label_set_text (GTK_LABEL (app_data.time_label), buf);

    if(min == 0 && sec == 0)
    {
        stop_game();
    }

    return TRUE;
}

static void
update_pbar (gdouble frac, gpointer data)
{
    GtkWidget *pbar = GTK_WIDGET (data);

    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), frac);
    while (gtk_events_pending ())
        gtk_main_iteration();

}


/*
 * functions
 */

void
history_data_cell_func (GtkTreeViewColumn *col,
                        GtkCellRenderer   *renderer,
                        GtkTreeModel      *model,
                        GtkTreeIter       *iter,
                        gpointer           user_data)
{
    guess_st st;
    gchar buf[60];
    gchar *word;
    gchar trunc_word[21];

    gtk_tree_model_get (model, iter, 0, &word, -1);
    gtk_tree_model_get (model, iter, 1, (guint *)&st, -1);

    strncpy (trunc_word, word, sizeof (trunc_word) - 1);

    switch (st)
    {
        case good_guess:
            /* TRANSLATORS: message for a proper guess */
            g_snprintf (buf, sizeof (buf), _("%s - ok"), trunc_word);
            g_object_set (renderer, "foreground", "Green",
                          "foreground-set", TRUE, NULL);
            break;

        case not_in_dictionary:
            g_snprintf (buf, sizeof (buf), _("%s - not in dictionary"),
                        trunc_word);
            g_object_set (renderer, "foreground", "Red",
                          "foreground-set", TRUE, NULL);
            break;

        case not_in_board:
            g_snprintf (buf, sizeof (buf), _("%s - not on board"), 
                        trunc_word);
            g_object_set (renderer, "foreground", "Red",
                          "foreground-set", TRUE, NULL);
            break;

        case already_guessed:
            g_snprintf (buf, sizeof (buf), _("%s - already guessed"),
                        trunc_word);
            g_object_set (renderer, "foreground", "Red",
                          "foreground-set", TRUE, NULL);
            break;

        case too_short:
            g_snprintf (buf, sizeof (buf), _("%s - too short"),
                        trunc_word);
            g_object_set (renderer, "foreground", "Red",
                          "foreground-set", TRUE, NULL);
            break;
    }

    g_object_set (renderer, "text", buf, NULL);
}

static GtkWidget *
create_history_tree_view (GtkTreeModel *model)
{
    GtkWidget *tree_view;
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;

    tree_view = gtk_tree_view_new_with_model (model);    
    
    col = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), col);
    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (tree_view), "enable-search", FALSE, NULL);

    gtk_tree_view_column_pack_start (col, renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func (col, renderer, 
                                             history_data_cell_func,
                                             NULL, NULL);

    g_object_unref (model);
    gtk_tree_selection_set_mode (
            gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
                                         GTK_SELECTION_NONE);

    return tree_view;
}

static GtkWidget *
create_solution_tree_view (GtkTreeModel *model)
{
    GtkWidget *tree_view;
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    GtkTreeSelection *select;

    tree_view = gtk_tree_view_new_with_model (model);    
    
    col = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), col);
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (col, renderer, TRUE);
    gtk_tree_view_column_set_attributes (col, renderer, "text", 0, NULL);
    g_object_set (G_OBJECT (tree_view), "enable-search", FALSE, NULL);

    g_object_unref (model);
    select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
    gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT (select), "changed",
                      G_CALLBACK (solutions_tree_view_changed),
                      NULL);

    return tree_view;
}

#ifdef HAVE_FREMANTLE
static GtkWidget *
create_hildon_touch_menu (void)
{
    GtkWidget *button;
    GtkWidget *menu = hildon_app_menu_new ();

    button = hildon_gtk_button_new (HILDON_SIZE_AUTO);
    gtk_button_set_label (GTK_BUTTON (button), _("Settings"));
    hildon_app_menu_append (HILDON_APP_MENU (menu), button);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (preferences_callback), NULL);
    app_data.settings_menubutton = button;

    gtk_widget_show_all (menu);

    return menu;
}

#else // HAVE_FREMANTLE

static GtkWidget *
create_main_menu (void)
{
    GtkWidget *main_menu, *game_menu, *settings_menu, *menu_item;

#ifdef HAVE_MAEMO
    main_menu = gtk_menu_new ();
#else
    main_menu = gtk_menu_bar_new ();
#endif
    game_menu = gtk_menu_new ();
    settings_menu = gtk_menu_new ();

    /* Game/New */
    menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (game_menu), menu_item);
    g_signal_connect (G_OBJECT (menu_item), "activate",
                      G_CALLBACK (game_new_callback), NULL);
    gtk_widget_show (menu_item);
    app_data.new_menuitem = menu_item;

    /* Game/Stop */
    menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_STOP, NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (game_menu), menu_item);
    g_signal_connect (G_OBJECT (menu_item), "activate",
                      G_CALLBACK (game_end_callback), NULL);
    gtk_widget_set_sensitive (menu_item, FALSE);
    gtk_widget_show (menu_item);
    app_data.stop_menuitem = menu_item;
    
    /* Game/Exit */
    menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (game_menu), menu_item);
    g_signal_connect (G_OBJECT (menu_item), "activate",
                      G_CALLBACK (gtk_main_quit), NULL);
    gtk_widget_show (menu_item);

    /* Game */
    menu_item = gtk_menu_item_new_with_mnemonic (_("_Game"));
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), menu_item);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), game_menu);
    gtk_widget_show (menu_item);
    
    /* Settings/Preferences */
    menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES,
            NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (settings_menu), menu_item);
    g_signal_connect (G_OBJECT (menu_item), "activate",
                      G_CALLBACK (preferences_callback), NULL);
    gtk_widget_show (menu_item);
    app_data.prefs_menuitem = menu_item;

    /* Settings */
    menu_item = gtk_menu_item_new_with_mnemonic (_("_Settings"));
    gtk_menu_shell_append (GTK_MENU_SHELL (main_menu), menu_item);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), settings_menu);
    gtk_widget_show (menu_item);

    gtk_widget_show (main_menu);

    return main_menu;
}
#endif // HAVE_FREMANTLE

void
create_main_window (gint boardw, gint boardh)
{
    GtkWidget *solutions_tree_view;
    GtkWidget *scrolled_history, *scrolled_solutions;
    GtkWidget *main_vbox;
    GtkWidget *upper_hbox;
    GtkWidget *lower_hbox;
    GtkWidget *wordlist_align;
    GtkWidget *guess_align;
    GtkWidget *time_score_align;
    GtkWidget *guess_hbox;
    GtkWidget *time_score_hbox;
    gchar *zero_time;

#ifdef HAVE_MAEMO
    app_data.main_win = hildon_window_new ();
    hildon_program_add_window (app_data.program, 
                               HILDON_WINDOW(app_data.main_win));
#else
    app_data.main_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#endif

    gtk_container_set_border_width (GTK_CONTAINER (app_data.main_win), 10);

    app_data.board_width = boardw;
    app_data.board_height = boardh;

    app_data.board_widget = board_widget_new (boardw, boardh);
    board_widget_set_active (BOARD_WIDGET (app_data.board_widget), FALSE);
    app_data.guess_label = gtk_label_new (_("Guess:"));
    app_data.guess_entry = gtk_entry_new ();
    app_data.new_game_button = gtk_button_new_with_label (_("New Game"));

    app_data.game_stop_button = gtk_button_new ();
    gtk_button_set_image (GTK_BUTTON (app_data.game_stop_button), 
            gtk_image_new_from_stock (GTK_STOCK_STOP,
                BUTTON_ICON_SIZE));
    gtk_widget_set_sensitive (app_data.game_stop_button, FALSE);

    gtk_entry_set_width_chars (GTK_ENTRY (app_data.guess_entry), GUESS_ENTRY_WIDTH);
    gtk_entry_set_activates_default (GTK_ENTRY (app_data.guess_entry), TRUE);
    main_vbox = gtk_vbox_new (FALSE, 0);
    upper_hbox = gtk_hbox_new (FALSE, 0);
    lower_hbox = gtk_hbox_new (FALSE, 0);
    wordlist_align = gtk_alignment_new (1, 0, 1, 1);
    guess_align = gtk_alignment_new (0, 0, 0, 0);
    time_score_align = gtk_alignment_new (1, 0.5, 0, 0);
    guess_hbox = gtk_hbox_new (FALSE, 0);
    time_score_hbox = gtk_hbox_new (FALSE, 0);
    app_data.wordlist_notebook = gtk_notebook_new ();

    zero_time = g_strdup_printf (TIME_FORMAT, 0, 0);
    app_data.time_label = gtk_label_new (zero_time);
    g_free (zero_time);

    app_data.score_label = gtk_label_new ("");
    reset_score_label (app_data.score_label);

    app_data.guess_submit = gtk_button_new ();
    gtk_button_set_image (GTK_BUTTON (app_data.guess_submit), 
            gtk_image_new_from_stock (GTK_STOCK_APPLY,
                BUTTON_ICON_SIZE));
    app_data.guess_del = gtk_button_new ();
    gtk_button_set_image (GTK_BUTTON (app_data.guess_del), 
            gtk_image_new_from_stock (GTK_STOCK_UNDO,
                BUTTON_ICON_SIZE));

    /* guess history */
    app_data.history_list_store = gtk_list_store_new (2, G_TYPE_STRING,
            G_TYPE_INT);
    scrolled_history = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_history),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    app_data.history_tree_view = 
        create_history_tree_view 
        (GTK_TREE_MODEL (app_data.history_list_store));
    gtk_widget_set_size_request (app_data.history_tree_view, LIST_WIDTH, -1);
    gtk_tree_view_set_headers_visible 
        (GTK_TREE_VIEW (app_data.history_tree_view), FALSE);
    
    /* solutions */
    app_data.solutions_list_store = 
        gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
    scrolled_solutions = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_solutions),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    solutions_tree_view = create_solution_tree_view (
                             GTK_TREE_MODEL (app_data.solutions_list_store));
    gtk_widget_set_size_request (solutions_tree_view, LIST_WIDTH, -1);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (solutions_tree_view),
            FALSE);
    
    gtk_box_pack_start (GTK_BOX (upper_hbox), app_data.board_widget,
                        TRUE, TRUE, 10);
//    gtk_container_add (GTK_CONTAINER (wordlist_align), 
//                       app_data.wordlist_notebook);
//    gtk_box_pack_start (GTK_BOX (upper_hbox), wordlist_align,
    gtk_box_pack_start (GTK_BOX (upper_hbox), app_data.wordlist_notebook,
                        TRUE, TRUE, 10);
    gtk_container_add (GTK_CONTAINER (guess_align), guess_hbox);
    gtk_box_pack_start (GTK_BOX (lower_hbox), guess_align,
                        TRUE, TRUE, 10);
    gtk_container_add (GTK_CONTAINER (time_score_align), time_score_hbox);
    gtk_box_pack_start (GTK_BOX (lower_hbox), time_score_align,
                        TRUE, TRUE, 10);


    gtk_notebook_append_page (GTK_NOTEBOOK (app_data.wordlist_notebook),
            scrolled_history,
            gtk_label_new (_("Guesses")));
    gtk_notebook_append_page (GTK_NOTEBOOK (app_data.wordlist_notebook),
            scrolled_solutions,
            gtk_label_new (_("Missed words")));

    // maemo menus
#ifdef HAVE_FREMANTLE
    hildon_window_set_app_menu (HILDON_WINDOW (app_data.main_win),
                                HILDON_APP_MENU (create_hildon_touch_menu ()));
#endif    
#ifdef HAVE_DIABLO
    hildon_window_set_menu (HILDON_WINDOW (app_data.main_win), 
                            GTK_MENU (create_main_menu ()));
#endif

#ifdef HAVE_MAEMO    
    gtk_widget_set_size_request(app_data.guess_submit, ICON_BUTTON_SIZE,
                                ICON_BUTTON_SIZE);
    gtk_widget_set_size_request(app_data.guess_del, ICON_BUTTON_SIZE,
                                ICON_BUTTON_SIZE);
    gtk_widget_set_size_request(app_data.game_stop_button, ICON_BUTTON_SIZE,
                                ICON_BUTTON_SIZE);
    gtk_widget_set_size_request(lower_hbox, -1, ICON_BUTTON_SIZE);
#else
    gtk_box_pack_start (GTK_BOX (main_vbox), create_main_menu (), 
                        FALSE, FALSE, 2);
    gtk_widget_set_size_request(app_data.board_widget, 200, 200);
#endif
    gtk_box_pack_start (GTK_BOX (main_vbox), upper_hbox, 
            TRUE, TRUE, VPAD);
    gtk_box_pack_start (GTK_BOX (main_vbox), lower_hbox, 
                        FALSE, FALSE, VPAD);

    gtk_container_add (GTK_CONTAINER (scrolled_history),
            app_data.history_tree_view);
    gtk_container_add (GTK_CONTAINER (scrolled_solutions),
            solutions_tree_view);
    gtk_box_pack_start (GTK_BOX (guess_hbox),
            app_data.new_game_button, FALSE, FALSE, GUESS_HPAD);
    gtk_box_pack_start (GTK_BOX (guess_hbox), app_data.guess_label,
            TRUE, TRUE, GUESS_HPAD);
    gtk_box_pack_start (GTK_BOX (guess_hbox), app_data.guess_entry,
            TRUE, TRUE, GUESS_HPAD);
    gtk_box_pack_start (GTK_BOX (guess_hbox), app_data.guess_submit,
            FALSE, FALSE, GUESS_HPAD);
    gtk_box_pack_start (GTK_BOX (guess_hbox), app_data.guess_del,
            FALSE, FALSE, GUESS_HPAD);
    gtk_box_pack_start (GTK_BOX (time_score_hbox), app_data.time_label,
            FALSE, FALSE, GUESS_HPAD);
    gtk_box_pack_start (GTK_BOX (time_score_hbox), app_data.score_label,
            FALSE, FALSE, GUESS_HPAD);
    gtk_box_pack_start (GTK_BOX (time_score_hbox), app_data.game_stop_button,
            FALSE, FALSE, GUESS_HPAD);
    gtk_container_add (GTK_CONTAINER (app_data.main_win), main_vbox);

    g_signal_connect (G_OBJECT (app_data.main_win), "destroy", G_CALLBACK (exit), NULL);
    g_signal_connect (G_OBJECT (app_data.new_game_button), "clicked",
                      G_CALLBACK (new_game_button_clicked), NULL);
    g_signal_connect (G_OBJECT (app_data.game_stop_button), "clicked",
                      G_CALLBACK (game_stop_button_clicked), NULL);
    g_signal_connect (G_OBJECT (app_data.board_widget), "field-pressed",
        G_CALLBACK (field_pressed_callback), NULL);
    g_signal_connect (G_OBJECT (app_data.guess_submit), "clicked",
        G_CALLBACK (guess_submit_clicked), NULL);
    g_signal_connect (G_OBJECT (app_data.guess_del), "clicked",
        G_CALLBACK (guess_del_clicked), NULL);

    gtk_widget_show (app_data.board_widget);
    gtk_widget_show_all (scrolled_history);
    gtk_widget_show_all (scrolled_solutions);
    gtk_widget_show (guess_hbox);
    gtk_widget_show (guess_align);
    gtk_widget_show (time_score_hbox);
    gtk_widget_show (time_score_align);
    gtk_widget_show (app_data.wordlist_notebook);
    gtk_widget_show (app_data.new_game_button);
    gtk_widget_show (app_data.time_label);
    gtk_widget_show (app_data.score_label);
    gtk_widget_show (app_data.game_stop_button);
    gtk_widget_show_all (upper_hbox);
    gtk_widget_show (lower_hbox);
    gtk_widget_show (main_vbox);
    gtk_widget_show (app_data.main_win);
    /* app_data.guess_* remain invisible */

    gtk_editable_set_editable (GTK_EDITABLE (app_data.guess_entry), FALSE);

}

void
create_preferences_dialog (void)
{
    GtkWidget *dialog;
    GtkWidget *table;
    GtkWidget *lang_label;
    GtkWidget *time_pref_label;
    GtkWidget *zero_is_unlimited_label;
    gint i;
    char default_time[20];

    dialog = gtk_dialog_new_with_buttons (_("Preferences"),
                                          GTK_WINDOW (app_data.main_win),
                                          GTK_DIALOG_MODAL |
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);

    lang_label = gtk_label_new (_("Language:"));
    gtk_misc_set_alignment (GTK_MISC (lang_label), 0, 0.5);
    app_data.lang_combo = gtk_combo_box_new_text ();
    for (i = 0; i < app_data.langconfs->len; ++i)
    {
        struct langconf *conf = 
            (struct langconf *) g_ptr_array_index (app_data.langconfs, i);
        
        DEBUGMSG ("lang %d %s\n", i, conf->lang);
        gtk_combo_box_append_text (GTK_COMBO_BOX (app_data.lang_combo), conf->lang);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (app_data.lang_combo), 0);
    
    // Add a widget to select how long (in seconds) the game should last.
    time_pref_label = gtk_label_new(_("Game Time (secs):"));
    gtk_misc_set_alignment (GTK_MISC (time_pref_label), 0, 0.5);
    app_data.time_pref_spinner_adjust =  GTK_ADJUSTMENT( gtk_adjustment_new(app_data.game_time, 0, 3600, 30, 0, 0) );
    app_data.time_pref_entry = gtk_spin_button_new(app_data.time_pref_spinner_adjust, 30, 0);
    
    zero_is_unlimited_label = gtk_label_new(_("Enter 0 for untimed game."));

    table = gtk_table_new (3, 1, FALSE);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), table);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);
    gtk_table_attach_defaults (GTK_TABLE (table), lang_label, 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (table), app_data.lang_combo, 1, 2, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (table), time_pref_label, 0, 1, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (table), app_data.time_pref_entry, 1, 2, 1, 2);
    gtk_table_attach_defaults (GTK_TABLE (table), zero_is_unlimited_label, 0, 2, 2, 3);

    gtk_widget_show_all (table);

    app_data.preferences_dialog = dialog;
}

gboolean
set_language_by_name (const gchar *language)
{
    gint i;

    for (i = 0; i < app_data.langconfs->len; ++i) {
        struct langconf *conf = 
            (struct langconf *)g_ptr_array_index(app_data.langconfs, i);
        if (g_str_equal(conf->lang, language)) {
            return set_language (i);
        }
    }

    g_warning("language not found: %s", language);
    return FALSE;
}

gboolean
set_language (gint l)
{
    struct langconf *conf;
    GNode *trie;
    GPtrArray *alph;
    GtkWidget *pdialog;
    GtkWidget *pbar;
    
    conf = (struct langconf *)g_ptr_array_index(app_data.langconfs, l);
    alph = conf->alphabet;
    trie = g_node_new (NULL);
    create_progress_dialog (&pdialog, &pbar);

    if(trie_load (trie, alph, conf->dictf, update_pbar, (gpointer)pbar) != 0)
    {
        g_printf ("%s: failed to open\n", conf->dictf);
        gtk_widget_destroy (pdialog);
        return FALSE;
    }

    gtk_widget_destroy (pdialog);
    DEBUGMSG("dictionary loaded");

    if (app_data.dictionary)
        g_node_destroy(app_data.dictionary);
    app_data.dictionary = trie;
    app_data.alphabet = alph;
    app_data.weights = (gint *)conf->weights->data;
    app_data.sel_lang = l;

    return TRUE;
}

void
init_game ()
{
    GBoggleSettings *settings;

    app_data.timer_tag = 0;
    app_data.solutions = NULL;
    app_data.found_words = NULL;
    app_data.guessed_words = 0;
    app_data.brd = NULL;
    app_data.current_path = NULL;

    create_main_window (DEFAULT_BOARD_WIDTH, DEFAULT_BOARD_HEIGHT);
    settings = load_settings ();
    app_data.game_time = settings->game_time;
    create_preferences_dialog ();
    
    if (!settings->language || !set_language_by_name (settings->language))
        set_language (0);

    update_title ();
}

void
start_game (void)
{
    guess_st st;
    board_dispose (app_data.brd);
    solutions_dispose (app_data.solutions);
    app_data.solutions = NULL;
    /* XXX free ptrarrays */
    gtk_list_store_clear (app_data.history_list_store);
    gtk_list_store_clear (app_data.solutions_list_store);

    app_data.brd =  board_new (DEFAULT_BOARD_WIDTH, DEFAULT_BOARD_HEIGHT,
            app_data.alphabet, app_data.weights, app_data.dictionary);
    app_data.found_words = g_ptr_array_new ();
    app_data.guessed_words = g_ptr_array_new ();
    app_data.score = 0;
    if (app_data.current_path)
        path_free (app_data.current_path);
    app_data.current_path = NULL;

    board_widget_init_with_board ( BOARD_WIDGET (app_data.board_widget),
            app_data.brd);
    board_widget_set_active (BOARD_WIDGET (app_data.board_widget), TRUE);

#ifdef HAVE_FREMANTLE
    gtk_widget_set_sensitive (app_data.settings_menubutton, FALSE);
#else    
    gtk_widget_set_sensitive (app_data.stop_menuitem, TRUE);
    gtk_widget_set_sensitive (app_data.prefs_menuitem, FALSE);
#endif    
    gtk_widget_hide (app_data.new_game_button);
    gtk_widget_set_sensitive (app_data.game_stop_button, TRUE);
    gtk_widget_show (app_data.guess_label);
    gtk_widget_show (app_data.guess_entry);
    gtk_widget_show (app_data.guess_del);
    gtk_widget_show (app_data.guess_submit);
    gtk_editable_set_editable (GTK_EDITABLE (app_data.guess_entry), TRUE);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (app_data.wordlist_notebook),
            0);
    gtk_widget_grab_focus (app_data.guess_entry);
    
    app_data.guess_changed_id = 
        g_signal_connect (G_OBJECT (app_data.guess_entry), "changed", 
                          G_CALLBACK(guess_changed), NULL);
    app_data.guess_keypressed_id =
        g_signal_connect (G_OBJECT (app_data.guess_entry), "key-press-event", 
                          G_CALLBACK(guess_keypressed), NULL);

    // Here, set the game time to whatever the user specified in
    // the preferences.
    
    // If game time is 0, don't set a timer.
    if (app_data.game_time > 0)
    {
    	app_data.timer_tag = g_timeout_add (100, timer_func, NULL);    
    	g_get_current_time (&app_data.game_start);
    }
    
    app_data.all_words = (GTree *) find_all_solutions(app_data.brd, &app_data.solutions);
    reset_score_label (app_data.score_label);
}

void
stop_game (void)
{
    guess_st st;
    
    // If we didn't set a timer (untimed game), don't
    // remove it.
    if (app_data.timer_tag) { g_source_remove (app_data.timer_tag); }
    app_data.timer_tag = 0;

    gtk_widget_hide (app_data.guess_label);
    gtk_widget_hide (app_data.guess_entry);
    gtk_widget_hide (app_data.guess_submit);
    gtk_widget_hide (app_data.guess_del);
    gtk_widget_show (app_data.new_game_button);
    gtk_widget_set_sensitive (app_data.game_stop_button, FALSE);
#ifdef HAVE_FREMANTLE    
    gtk_widget_set_sensitive (app_data.settings_menubutton, TRUE);
#else    
    gtk_widget_set_sensitive (app_data.new_menuitem, TRUE);
    gtk_widget_set_sensitive (app_data.stop_menuitem, FALSE);
    gtk_widget_set_sensitive (app_data.prefs_menuitem, TRUE);
#endif    
    board_widget_set_active (BOARD_WIDGET (app_data.board_widget), FALSE);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (app_data.wordlist_notebook),
            1);

    gtk_editable_set_editable (GTK_EDITABLE (app_data.guess_entry), FALSE);
    g_signal_handler_disconnect (app_data.guess_entry,
            app_data.guess_changed_id);
    g_signal_handler_disconnect (app_data.guess_entry, 
            app_data.guess_keypressed_id);
    gtk_entry_set_text (GTK_ENTRY (app_data.guess_entry), "");

    // Do this before the game starts, instead
    //app_data.solutions = g_ptr_array_new ();
    //st = search_solution (app_data.solutions, NULL, app_data.brd, TRUE);
    list_solutions_and_score (app_data.solutions, app_data.found_words);
}


void
history_add (const gchar *word, guess_st st)
{
    GtkTreeIter iter;
    GtkTreePath *path;

    gtk_list_store_insert (app_data.history_list_store, &iter, 0);
    gtk_list_store_set (app_data.history_list_store, &iter, 0, word, 1,
            (guint)st, -1);

    path = gtk_tree_path_new_from_indices (0, -1);
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (app_data.history_tree_view),
                                  path, NULL, FALSE, 0.0, 0.0);
    gtk_tree_path_free (path);
}


void
list_solutions_and_score (GPtrArray *solutions, GPtrArray *found_words)
{
    GtkTreeIter iter;
    GPtrArray *words;
    GArray *sol_index;
    gint i;
    gchar *str;
    
    missing_solutions (&words, &sol_index, app_data.brd, app_data.solutions,
            app_data.found_words, app_data.all_words);
    for (i = 0; i < words->len; ++i)
    {
        gtk_list_store_append (app_data.solutions_list_store, &iter);
        gtk_list_store_set (app_data.solutions_list_store, &iter,
                            0, (gchar *)g_ptr_array_index (words, i),
                            1, g_array_index (sol_index, guint, i),
                            -1);
        /*g_debug ("%s added", (gchar *)g_ptr_array_index (words, i));*/
    }

    str = g_strdup_printf (FINAL_SCORE_FORMAT, app_data.score, 
            app_data.score + words->len);
    gtk_label_set_text (GTK_LABEL (app_data.score_label), str);
    g_free (str);
}

void
mark_path (GArray *path)
{
    gint len, i;

    len = path_length (path);
    for (i = 0; i < len; ++i)
    {
        coord c = path_index (path, i);
        board_widget_mark_field (BOARD_WIDGET (app_data.board_widget),
                c.x, c.y, 
                len > 1 ? (gdouble)i  / (len - 1) :
                1);
    }
}

void
update_title (void)
{
    char *title;
    struct langconf *conf = 
        (struct langconf *) g_ptr_array_index (app_data.langconfs, 
                app_data.sel_lang);
        
    title = g_strdup_printf ("%s", conf->lang);
    gtk_window_set_title (GTK_WINDOW (app_data.main_win), title);
    g_free (title);
}

static void
submit_guess (gboolean keep_state)
{
    guess_st st;
    const gchar *guess;
    gchar *nguess;
    gint word_val;

    guess = gtk_entry_get_text (GTK_ENTRY (app_data.guess_entry));
    nguess = normalize_guess (guess);

    if (g_str_equal(nguess, ""))
        return;
    DEBUGMSG ("guess submitted: %s\n", nguess);

    st = process_guess (nguess, app_data.brd, 
            app_data.guessed_words, &word_val);
    DEBUGMSG ("guess state: %d\n", st);
    if (st == good_guess)
    {
        gchar *str;

        g_ptr_array_add (app_data.found_words, nguess);
        app_data.score += word_val;
        str = g_strdup_printf(SCORE_FORMAT, app_data.score, g_tree_nnodes(app_data.all_words));
        gtk_label_set_text (GTK_LABEL (app_data.score_label), str);
        g_free (str);
    }
    DEBUGMSG ("history add: %s %d\n", guess, st);
    history_add (guess, st);

    if (app_data.score == g_tree_nnodes(app_data.all_words))
    {
        // found all the words, stop the game
        stop_game ();
        return;
    }

    // If flag is set, do not clear.
    // Allows user to hit Ctrl-Enter to submit a guess
    // and then continue from that point (for example to
    // submit a singular word and then simply type s-Enter to
    // submit its plural form).
    if (!keep_state)
    {
        DEBUGMSG("Clearing state...\n");
        board_widget_initbg (BOARD_WIDGET (app_data.board_widget));
        gtk_entry_set_text (GTK_ENTRY (app_data.guess_entry), "");
        if (app_data.current_path)
            path_free (app_data.current_path);
        app_data.current_path = NULL;
    }
}

static void
create_progress_dialog (GtkWidget **dialogp, GtkWidget **pbar)
{
    GtkWidget *dialog;
    GtkWidget *label;

    dialog = gtk_dialog_new ();
    gtk_window_set_title (GTK_WINDOW (dialog), APPNAME);
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), 
                    GTK_WINDOW (app_data.main_win));
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);

    label = gtk_label_new (_("Loading dictionary"));
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), label);
    *pbar = gtk_progress_bar_new ();
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), *pbar);
    gtk_widget_show_all (dialog);
    *dialogp = dialog;
}

static void
reset_score_label (GtkWidget *label)
{
    gchar *zero_score;

    zero_score = g_strdup_printf (SCORE_FORMAT, 0, app_data.all_words != NULL ? g_tree_nnodes(app_data.all_words) : 0);
    gtk_label_set_text (GTK_LABEL (label), zero_score);
    g_free (zero_score);
}
