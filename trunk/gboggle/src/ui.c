#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>

#include "ui.h"
#include "appdata.h"
#include "board_widget.h"
#include "board.h"
#include "boggle.h"
#include "langconf.h"

/*
 * constants
 */

#define VPAD 12
#define HPAD 12
#define LIST_HEIGHT (DEFAULT_BOARD_HEIGHT * FIELDH)
#define LIST_WIDTH 200 

/*
 * callbacks
 */

static void
guess_changed (GtkEditable *editable, gpointer data)
{
    const gchar *guess;
    guess_st st;
    GPtrArray *paths;
    coord **path;
 
    board_widget_initbg (BOARD_WIDGET (app_data.board_widget));

    guess = gtk_entry_get_text (GTK_ENTRY (editable));
    DEBUGSTM (g_printf ("guess %s\n", guess));
    paths = g_ptr_array_new ();
    st = find_str_on_board (paths, guess, app_data.brd);
    
    if (st == good_guess)
    {
        path = g_ptr_array_index (paths, 0);
        mark_path (path);
    }
    
    DEBUGSTM(g_printf ("status %d\n", st));
}

static gboolean
guess_keypressed (GtkEditable *editable, GdkEventKey *event, gpointer data)
{
    guess_st st;
    const gchar *guess;
    gchar *normalized_guess;
    gint word_val;

    if (event->keyval != GDK_Return)
    {
        return FALSE;
    }

    guess = gtk_entry_get_text (GTK_ENTRY (editable));
    normalized_guess = normalize_guess (guess);
    DEBUGSTM (g_printf ("guess submitted: %s\n", normalized_guess));

    st = process_guess (normalized_guess, app_data.brd, 
            app_data.guessed_words, &word_val);
    DEBUGSTM (g_printf ("guess state: %d\n", st));    
    if (st == good_guess)
    {
        gchar *str;

        g_ptr_array_add (app_data.found_words, normalized_guess);
        app_data.score += word_val;
        str = g_strdup_printf("Score: %d", app_data.score);
        gtk_label_set_text (GTK_LABEL (app_data.score_label), str);
        g_free (str);
    }
    DEBUGSTM (g_printf ("history add: %s %d\n", guess, st));
    history_add (guess, st);

    board_widget_initbg (BOARD_WIDGET (app_data.board_widget));
    gtk_entry_set_text (GTK_ENTRY (app_data.guess_entry), "");

    return FALSE;
}

static void
solutions_tree_view_changed (GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    guint sol_index;
    coord **path;

    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        return;
    }
    
    gtk_tree_model_get (model, &iter, 1, &sol_index, -1);
    path = (coord **)g_ptr_array_index (app_data.solutions, sol_index);
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
    gint l, oldl;
    
    oldl = gtk_combo_box_get_active (GTK_COMBO_BOX (app_data.lang_combo));
    gtk_dialog_run (GTK_DIALOG (app_data.preferences_dialog));
    gtk_widget_hide (app_data.preferences_dialog);
    l = gtk_combo_box_get_active (GTK_COMBO_BOX (app_data.lang_combo));
    g_debug ("lang %d selected\n", l);
    if (l >= 0)
    {
        if (!set_language (l))
        {
            g_debug ("lang set to %d failed, revert to %d\n", l, oldl);
            gtk_combo_box_set_active (GTK_COMBO_BOX (app_data.lang_combo), oldl);
        }
    }
}

static gboolean
timer_func (gpointer data)
{
    gint min, sec;
    GTimeVal curr_time;
    gchar buf[16];

    g_get_current_time (&curr_time);
    sec = GAME_LENGTH_SEC - (curr_time.tv_sec - app_data.game_start.tv_sec);
    if (sec < 0)
        sec = 0;
    min = sec / 60;
    sec %= 60;
    g_snprintf (buf, sizeof (buf), "Time: %d:%02d", min, sec);
    gtk_label_set_text (GTK_LABEL (app_data.time_label), buf);

    if(min == 0 && sec == 0)
    {
        stop_game();
    }

    return TRUE;
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
            g_snprintf (buf, sizeof (buf), "%s - ok", trunc_word);
            g_object_set (renderer, "foreground", "Green",
                          "foreground-set", TRUE, NULL);
            break;

        case not_in_dictionary:
            g_snprintf (buf, sizeof (buf), "%s - not in dictionary",
                        trunc_word);
            g_object_set (renderer, "foreground", "Red",
                          "foreground-set", TRUE, NULL);
            break;

        case not_in_board:
            g_snprintf (buf, sizeof (buf), "%s - not on board", 
                        trunc_word);
            g_object_set (renderer, "foreground", "Red",
                          "foreground-set", TRUE, NULL);
            break;

        case already_guessed:
            g_snprintf (buf, sizeof (buf), "%s - already guessed",
                        trunc_word);
            g_object_set (renderer, "foreground", "Red",
                          "foreground-set", TRUE, NULL);
            break;

        case too_short:
            g_snprintf (buf, sizeof (buf), "%s - too short",
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
    g_object_set (G_OBJECT (col), "title", "Guesses", NULL);

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
    g_object_set (G_OBJECT (col), "title", "Missed words", NULL);
    g_object_set (G_OBJECT (tree_view), "enable-search", FALSE, NULL);

    g_object_unref (model);
    select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
    gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT (select), "changed",
                      G_CALLBACK (solutions_tree_view_changed),
                      NULL);

    return tree_view;
}


void
create_main_window (gint boardw, gint boardh)
{
    GtkWidget *solutions_tree_view;
    GtkWidget *scrolled_history, *scrolled_solutions;
    GtkWidget *menu_bar, *game_menu, *settings_menu, *menu_item;

    app_data.main_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ( GTK_WINDOW (app_data.main_win), "gboggle");
    gtk_container_set_border_width (GTK_CONTAINER (app_data.main_win), 10);

    app_data.board_width = boardw;
    app_data.board_height = boardh;

    app_data.board_widget = board_widget_new (boardw, boardh);
    app_data.guess_label = gtk_label_new ("Guess:");
    app_data.guess_entry = gtk_entry_new ();
    app_data.new_game_button = gtk_button_new_with_label ("New Game");
    gtk_entry_set_width_chars (GTK_ENTRY (app_data.guess_entry), 10);
    app_data.main_vbox = gtk_vbox_new (FALSE, 0);
    app_data.left_vbox = gtk_vbox_new (FALSE, 0);
    app_data.top_hbox = gtk_hbox_new (FALSE, 0);
    app_data.bottom_hbox = gtk_hbox_new (FALSE, 0);
    app_data.right_vbox = gtk_vbox_new (FALSE, 0);
    app_data.time_label = gtk_label_new ("Time: 0:00");
    app_data.score_label = gtk_label_new ("Score: 0");

    /* guess history */
    app_data.history_list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
    scrolled_history = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_history),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    app_data.history_tree_view = create_history_tree_view (GTK_TREE_MODEL (app_data.history_list_store));
    gtk_widget_set_size_request (app_data.history_tree_view, LIST_WIDTH, 
            LIST_HEIGHT);
    
    /* solutions */
    app_data.solutions_list_store = 
        gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
    scrolled_solutions = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_solutions),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    solutions_tree_view = create_solution_tree_view (
                             GTK_TREE_MODEL (app_data.solutions_list_store));
    gtk_widget_set_size_request (solutions_tree_view, LIST_WIDTH, LIST_HEIGHT);
    
    /* menu */
    menu_bar = gtk_menu_bar_new ();
    game_menu = gtk_menu_new ();
    settings_menu = gtk_menu_new ();

    /* Game/New */
    menu_item = gtk_menu_item_new_with_label ("New game");
    gtk_menu_shell_append (GTK_MENU_SHELL (game_menu), menu_item);
    g_signal_connect (G_OBJECT (menu_item), "activate",
                      G_CALLBACK (game_new_callback), NULL);
    gtk_widget_show (menu_item);

    /* Game/End */
    app_data.end_game_menu_item = gtk_menu_item_new_with_label ("End game");
    gtk_menu_shell_append (GTK_MENU_SHELL (game_menu), app_data.end_game_menu_item);
    g_signal_connect (G_OBJECT (app_data.end_game_menu_item), "activate",
                      G_CALLBACK (game_end_callback), NULL);
    gtk_widget_set_sensitive (app_data.end_game_menu_item, FALSE);
    gtk_widget_show (app_data.end_game_menu_item);
    
    /* Game/Exit */
    menu_item = gtk_menu_item_new_with_label ("Exit");
    gtk_menu_shell_append (GTK_MENU_SHELL (game_menu), menu_item);
    g_signal_connect (G_OBJECT (menu_item), "activate",
                      G_CALLBACK (gtk_main_quit), NULL);
    gtk_widget_show (menu_item);

    /* Game */
    menu_item = gtk_menu_item_new_with_label ("Game");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), menu_item);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), game_menu);
    gtk_widget_show (menu_item);
    
    /* Settings/Preferences */
    menu_item = gtk_menu_item_new_with_label ("Preferences");
    gtk_menu_shell_append (GTK_MENU_SHELL (settings_menu), menu_item);
    g_signal_connect (G_OBJECT (menu_item), "activate",
                      G_CALLBACK (preferences_callback), NULL);
    gtk_widget_show (menu_item);

    /* Settings */
    menu_item = gtk_menu_item_new_with_label ("Settings");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), menu_item);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), settings_menu);
    gtk_widget_show (menu_item);
    
    gtk_box_pack_start (GTK_BOX (app_data.left_vbox), app_data.board_widget, FALSE, FALSE, VPAD);
    gtk_box_pack_start (GTK_BOX (app_data.main_vbox), menu_bar, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (app_data.main_vbox), app_data.top_hbox, TRUE, TRUE, VPAD);
    gtk_box_pack_start (GTK_BOX (app_data.main_vbox), app_data.bottom_hbox, TRUE, TRUE, VPAD);
    gtk_container_add (GTK_CONTAINER (scrolled_history), app_data.history_tree_view);
    gtk_container_add (GTK_CONTAINER (scrolled_solutions), solutions_tree_view);
    gtk_box_pack_start (GTK_BOX (app_data.bottom_hbox), app_data.new_game_button, FALSE, FALSE,
                        HPAD);
    gtk_box_pack_start (GTK_BOX (app_data.bottom_hbox), app_data.guess_label,
                        TRUE, TRUE, HPAD);
    gtk_box_pack_start (GTK_BOX (app_data.bottom_hbox), app_data.guess_entry,
                        TRUE, TRUE, HPAD);
    gtk_box_pack_start (GTK_BOX (app_data.bottom_hbox), app_data.time_label, TRUE, TRUE, HPAD);
    gtk_container_add (GTK_CONTAINER (app_data.bottom_hbox), app_data.score_label);
    gtk_box_pack_start (GTK_BOX (app_data.top_hbox), app_data.left_vbox,
            TRUE, TRUE, HPAD);
    gtk_box_pack_start (GTK_BOX (app_data.top_hbox), scrolled_history,
            TRUE, TRUE, HPAD);
    gtk_box_pack_start (GTK_BOX (app_data.top_hbox), scrolled_solutions,
            TRUE, TRUE, HPAD);
    gtk_container_add (GTK_CONTAINER (app_data.main_win), app_data.main_vbox);

    g_signal_connect (G_OBJECT (app_data.main_win), "destroy", G_CALLBACK (exit), NULL);
    g_signal_connect (G_OBJECT (app_data.new_game_button), "clicked",
                      G_CALLBACK (new_game_button_clicked), NULL);

    gtk_widget_show_all(app_data.top_hbox);
    gtk_widget_show(app_data.new_game_button);
    gtk_widget_show(app_data.time_label);
    gtk_widget_show(app_data.score_label);
    gtk_widget_show(app_data.bottom_hbox);
    gtk_widget_show(app_data.main_vbox);
    gtk_widget_show(app_data.main_win);
    gtk_widget_show (menu_bar);
    /* app_data.guess_label and app_data.guess_entry remain invisible */

    gtk_editable_set_editable (GTK_EDITABLE (app_data.guess_entry), FALSE);
}

void
create_preferences_dialog (void)
{
    GtkWidget *dialog;
    GtkWidget *table;
    GtkWidget *lang_label;
    gint i;

    dialog = gtk_dialog_new_with_buttons ("Preferences",
                                          GTK_WINDOW (app_data.main_win),
                                          GTK_DIALOG_MODAL |
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
    lang_label = gtk_label_new ("Language:");
    app_data.lang_combo = gtk_combo_box_new_text ();
    for (i = 0; i < app_data.langconfs->len; ++i)
    {
        struct langconf *conf = 
            (struct langconf *) g_ptr_array_index (app_data.langconfs, i);
        
        g_debug ("lang %d %s\n", i, conf->lang);
        gtk_combo_box_append_text (GTK_COMBO_BOX (app_data.lang_combo), conf->lang);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (app_data.lang_combo), 0);

    table = gtk_table_new (2, 1, FALSE);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), table);
    gtk_table_set_row_spacings (GTK_TABLE (table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);
    gtk_table_attach_defaults (GTK_TABLE (table), lang_label, 0, 1, 0, 1);
    gtk_table_attach_defaults (GTK_TABLE (table), app_data.lang_combo, 1, 2, 0, 1);

    gtk_widget_show_all (table);

    app_data.preferences_dialog = dialog;
}

gboolean
set_language (gint l)
{
    struct langconf *conf;
    GNode *trie;
    const gchar * const *alph;
    
    conf = (struct langconf *)g_ptr_array_index(app_data.langconfs, l);
    alph = (const gchar * const *)conf->alphabet->pdata;
    trie = g_node_new (NULL);
    if(trie_load (trie, alph, conf->dictf) != 0)
    {
        g_printf ("%s: failed to open\n", conf->dictf);
        return FALSE;
    }

    if (app_data.dictionary)
        g_node_destroy(app_data.dictionary);
    app_data.dictionary = trie;
    app_data.alphabet = alph;
    app_data.weights = (gint *)conf->weights->data;
    return TRUE;
}

void
init_game ()
{
    app_data.timer_tag = 0;
    app_data.solutions = NULL;
    app_data.found_words = NULL;
    app_data.guessed_words = 0;
    app_data.brd = NULL;
}

void
start_game ()
{

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
    gtk_label_set_text (GTK_LABEL (app_data.score_label), "");

    board_widget_init_with_board ( BOARD_WIDGET (app_data.board_widget),
            app_data.brd);

    gtk_widget_set_sensitive (app_data.end_game_menu_item, TRUE);
    gtk_widget_hide (app_data.new_game_button);
    gtk_widget_show (app_data.guess_label);
    gtk_widget_show (app_data.guess_entry);
    gtk_editable_set_editable (GTK_EDITABLE (app_data.guess_entry), TRUE);
    gtk_widget_grab_focus (app_data.guess_entry);
    
    app_data.guess_changed_id = 
        g_signal_connect (G_OBJECT (app_data.guess_entry), "changed", 
                          G_CALLBACK(guess_changed), NULL);
    app_data.guess_keypressed_id =
        g_signal_connect (G_OBJECT (app_data.guess_entry), "key-press-event", 
                          G_CALLBACK(guess_keypressed), NULL);
    app_data.timer_tag = g_timeout_add (100, timer_func, NULL);    
    g_get_current_time (&app_data.game_start);
}

void
stop_game ()
{
    guess_st st;
    
    g_source_remove (app_data.timer_tag);
    app_data.timer_tag = 0;

    gtk_widget_hide (app_data.guess_label);
    gtk_widget_hide (app_data.guess_entry);
    gtk_widget_show (app_data.new_game_button);
    gtk_widget_set_sensitive (app_data.end_game_menu_item, FALSE);

    gtk_editable_set_editable (GTK_EDITABLE (app_data.guess_entry), FALSE);
    g_signal_handler_disconnect (app_data.guess_entry,
            app_data.guess_changed_id);
    g_signal_handler_disconnect (app_data.guess_entry, 
            app_data.guess_keypressed_id);
    gtk_entry_set_text (GTK_ENTRY (app_data.guess_entry), "");

    app_data.solutions = g_ptr_array_new ();
    st = search_solution (app_data.solutions, NULL, app_data.brd, TRUE);
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
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (app_data.history_tree_view), path,
                              NULL, FALSE);
    g_free (path);
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
            app_data.found_words);
    for (i = 0; i < words->len; ++i)
    {
        gtk_list_store_append (app_data.solutions_list_store, &iter);
        gtk_list_store_set (app_data.solutions_list_store, &iter,
                            0, (gchar *)g_ptr_array_index (words, i),
                            1, g_array_index (sol_index, guint, i),
                            -1);
        /*g_debug ("%s added", (gchar *)g_ptr_array_index (words, i));*/
    }

    str = g_strdup_printf ("Score: %d of %d", app_data.score, 
            app_data.score + words->len);
    gtk_label_set_text (GTK_LABEL (app_data.score_label), str);
    g_free (str);
}

void
mark_path (coord **path)
{
    gint len, i;

    for (len = 0; path[len]; ++len);
    for (i = 0; i < len; ++i)
    {
        board_widget_mark_field (BOARD_WIDGET (app_data.board_widget),
                                 path[i]->x, path[i]->y, 
                                 len > 1 ? (gdouble)i  / (len - 1) :
                                 1);
    }
}
