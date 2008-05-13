#ifndef __APPDATA_H__
#define __APPDATA_H__

#include <gtk/gtk.h>
#include "langconf.h"
#include "board.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MAEMO
#include <hildon/hildon-program.h>
#endif

struct _AppData {
    GPtrArray *alphabet;
    gint *weights;
    GNode *dictionary;
    board *brd;
    GPtrArray *langconfs;
    gint sel_lang;

#ifdef HAVE_MAEMO
    HildonProgram *program;
#endif

    GtkWidget *main_win;
    GtkWidget *board_widget;
    GtkWidget *guess_label;
    GtkWidget *time_label;
    GtkWidget *score_label;
    GtkWidget *guess_entry;
    GtkWidget *guess_submit;
    GtkWidget *guess_del;
    GtkWidget *new_game_button;
    GtkWidget *history_tree_view;
    GtkWidget *preferences_dialog;
    GtkWidget *lang_combo;
    GtkWidget *end_game_menu_item;
    GtkWidget *wordlist_notebook;

    GtkListStore *history_list_store;
    GtkListStore *solutions_list_store;

    GPtrArray *found_words;
    GPtrArray *guessed_words;
    GPtrArray *solutions;
    GArray *current_path;

    gint score;

    gint board_width;
    gint board_height;

    GTimeVal game_start;
    gint timer_tag;

    gulong guess_changed_id;
    gulong guess_keypressed_id;
};

typedef struct _AppData AppData;

extern AppData app_data;

#define APPNAME "gboggle"

#endif /* __APPDATA_H__ */
