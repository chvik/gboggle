#ifndef __BOARD_WIDGET_H__
#define __BOARD_WIDGET_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "board.h"

#define BOARD_WIDGET_FONT_SIZE 24
#define BOARD_WIDGET_FIELD_FONT_RATIO (5 / 2)

G_BEGIN_DECLS

#define BOARD_WIDGET_TYPE            (board_widget_get_type ())
#define BOARD_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOARD_WIDGET_TYPE, BoardWidget))
#define BOARD_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOARD_WIDGET_TYPE, BoardWidgetClass))
#define IS_BOARD_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOARD_WIDGET_TYPE))
#define IS_BOARD_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOARD_WIDGET_TYPE))

#define FIELDW (BOARD_WIDGET_FONT_SIZE * BOARD_WIDGET_FIELD_FONT_RATIO)
#define FIELDH (BOARD_WIDGET_FONT_SIZE * BOARD_WIDGET_FIELD_FONT_RATIO)

#ifdef HAVE_MAEMO
#define BGCOLOR_UNMARKED { 0, 0xf0f0, 0xf0f0, 0xf0f0 }
#else
#define BGCOLOR_UNMARKED { 0, 0xffff, 0xffff, 0xffff }
#endif

#define BGCOLOR_MARKED { 0, 0x7fff, 0xffff, 0x7fff }
#define BGCOLOR_ACTIVE { 0, 0, 0xefff, 0 }

typedef struct _BoardWidget BoardWidget;
typedef struct _BoardWidgetClass BoardWidgetClass;

struct _BoardWidget
{
    GtkTable table;

    gint width;
    gint height;
    gint fieldw;
    gint fieldh;
    gint font_size;

    gboolean is_active;
    GtkWidget **fields;

    board *brd;
};

struct _BoardWidgetClass
{
    GtkTableClass parent_class;

    void (* board_widget) (BoardWidget *boardw);
};

GType
board_widget_get_type ();

GtkWidget *
board_widget_new (gint width,
                  gint height);

void
board_widget_init_with_board (BoardWidget *boardw,
                              board *brd);

void
board_widget_initbg (BoardWidget *boardw);

/* marks field by setting background color from a gradient map
 * scale can be set between 0 and 1 */
void
board_widget_mark_field (BoardWidget *boardw,
                         gint x,
                         gint y, 
                         gdouble scale);

/* sets board widget sensitive/insensitive to field clicks */
void
board_widget_set_active (BoardWidget *boardw, gboolean is_active);

G_END_DECLS

#endif /* __BOARD_WIDGET_H__ */
