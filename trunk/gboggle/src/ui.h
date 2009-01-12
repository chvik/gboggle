#ifndef __UI_H__
#define __UI_H__

#include <gtk/gtk.h>

#include "board.h"
#include "boggle.h"

#define DEFAULT_BOARD_WIDTH 4
#define DEFAULT_BOARD_HEIGHT 4

void
create_main_window (gint boardw,
                    gint boardh);

void
create_preferences_dialog (void);

/* l: index in the lang_conf array
 * returns FALSE if loading of the dictionary fails */
gboolean
set_language (gint l);

/* language: name of the language
 * returns DALSE if loading of the dictionary fails */
gboolean
set_language_by_name (const gchar *language);

void
init_game (void);

void
start_game (void);

void
stop_game (void);

void
history_add (const gchar *word,
             guess_st st); 

void
list_solutions_and_score (GPtrArray *solutions,
                          GPtrArray *found_words);

void
mark_path (GArray *path);

void
update_title (void);

#endif /* __UI_H__ */
