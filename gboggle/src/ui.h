#ifndef __UI_H__
#define __UI_H__

#include <gtk/gtk.h>

#include "board.h"
#include "boggle.h"

#define DEFAULT_BOARD_WIDTH 4
#define DEFAULT_BOARD_HEIGHT 4
#define GAME_LENGTH_SEC 120

/*
 * global variables
 */

const gchar * const *alphabet;
guint *weights;
GNode *dictionary;
board *brd;

GtkWidget *main_win;
GtkWidget *board_widget;
GtkWidget *main_vbox, *top_hbox, *left_vbox, *right_vbox, *bottom_hbox;
GtkWidget *guess_label, *time_label, *score_label;
GtkWidget *guess_entry;
GtkWidget *new_game_button;
GtkWidget *history_tree_view;
GtkWidget *preferences_dialog;

GtkListStore *history_list_store, *solutions_list_store;

GPtrArray *found_words;
GPtrArray *guessed_words;
GPtrArray *solutions;

gint score;

guint board_width;
guint board_height;

GTimeVal game_start;
gint timer_tag; 

gulong guess_changed_id;
gulong guess_keypressed_id;

/*
 * functions
 */

void
create_main_window (guint boardw,
                    guint boardh);

void
create_preferences_dialog (GPtrArray *confs);

void
init_game (GNode *trie,
           const gchar * const *alphabet, 
           guint *weights);

void
start_game ();

void
stop_game ();

void
history_add (const gchar *word,
             guess_st st); 

void
list_solutions (GPtrArray *solutions,
                GPtrArray *found_words);

void
mark_path (coord **path);


#endif /* __UI_H__ */
