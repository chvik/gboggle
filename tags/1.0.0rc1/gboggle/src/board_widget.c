#include <math.h>
#include <gtk/gtksignal.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "board_widget.h"
#include "board.h"

static void board_widget_class_init(BoardWidgetClass *klass);
static void board_widget_init(BoardWidget *boardw);
static void event_box_clicked_cb(GtkWidget *event_box, GdkEventButton *event,
        gpointer user_data);

static const GdkColor bgcolor_marked = BGCOLOR_MARKED;
static const GdkColor bgcolor_unmarked = BGCOLOR_UNMARKED;
static const GdkColor bgcolor_active = BGCOLOR_ACTIVE;

#define FIELD(bw,i,j)    (bw)->fields[((j) * (bw)->width + (i))]

enum {
    FIELD_PRESSED_SIGNAL,
    LAST_SIGNAL
};

static guint board_widget_signals[LAST_SIGNAL] = { 0 };

GType board_widget_get_type()
{
    static GType boardw_type = 0;

    if(!boardw_type)
    {
        static const GTypeInfo boardw_info =
        {
            sizeof(BoardWidgetClass),
            NULL, /* base init */
            NULL, /* base_finalize */
            (GClassInitFunc) board_widget_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(BoardWidget),
            0,
            (GInstanceInitFunc) board_widget_init,
        };

        boardw_type = g_type_register_static(GTK_TYPE_TABLE, "BoardWidget",
                &boardw_info, 0);
    }

    return boardw_type;
}

static void board_widget_class_init(BoardWidgetClass *klass)   
{
    /* emits "field-pressed" if one of the fields is clicked
     * void callback (GtkWidget *, coord *, gpointer) */
    board_widget_signals[FIELD_PRESSED_SIGNAL] =
        g_signal_new ("field-pressed",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                0,
                NULL, NULL,
                g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE,
                1, G_TYPE_POINTER);
}

static void board_widget_init(BoardWidget *boardw)
{
    boardw->fieldw = FIELDW;
    boardw->fieldh = FIELDH;
    boardw->width = 0;
    boardw->height = 0;
    boardw->is_active = FALSE;
}

GtkWidget *
board_widget_new (gint width, gint height)
{
    gint i, j;
    BoardWidget *boardw = BOARD_WIDGET (g_object_new (board_widget_get_type (), 
                                                      NULL));

    boardw->width = width;
    boardw->height = height;
    boardw->fields = g_new (GtkWidget *, boardw->width * boardw->height);
    gtk_table_resize (GTK_TABLE (boardw), boardw->width, boardw->height);
    gtk_table_set_homogeneous (GTK_TABLE (boardw), TRUE);

    for (i = 0; i < boardw->width; ++i)
        for (j = 0; j < boardw->height; ++j) {
            GtkWidget *label;
            GtkWidget *evbox;
            
            evbox = gtk_event_box_new ();
            g_signal_connect (G_OBJECT (evbox), "button-release-event",
                    G_CALLBACK (event_box_clicked_cb), boardw);
            label = gtk_label_new ("");
            gtk_label_set_width_chars (GTK_LABEL (label), 2);
            gtk_container_add (GTK_CONTAINER (evbox), label);
            FIELD (boardw, i, j) = evbox;
            gtk_table_attach_defaults (GTK_TABLE (boardw), 
                    evbox, i, i+1, j, j+1);
            gtk_widget_set_size_request (evbox,
                                         boardw->fieldw,
                                         boardw->fieldh);
            
            gtk_widget_show (label);
            gtk_widget_show (evbox);
        }
    
    board_widget_initbg (boardw);

    return GTK_WIDGET (boardw);
}

void
board_widget_init_with_board (BoardWidget *boardw, board *brd)
{
    gint i, j;

    g_assert (brd->width == boardw->width);
    g_assert (brd->height == boardw->height);

    for(i = 0; i < boardw->width; ++i)
        for(j = 0; j < boardw->height; ++j) {
            GtkWidget *label;
            gchar *upper;
            gchar *markup;

            label = gtk_bin_get_child (GTK_BIN (FIELD (boardw, i, j)));
            upper = g_utf8_strup (board_gcharp_at (brd, i, j), -1);
            markup = g_strdup_printf ("<span font_desc=\"%d\">%s</span>",
                                      BOARD_WIDGET_FONT_SIZE,
                                      upper);
/*            gtk_label_set_text (GTK_LABEL (label), upper);*/
            gtk_label_set_markup (GTK_LABEL (label), markup);
            g_free (upper);
            g_free (markup);
        }

    board_widget_initbg (boardw);
}

static void
board_widget_unmark_field (BoardWidget *boardw, gint x, gint y)
{
    gtk_widget_modify_bg (FIELD(boardw, x, y), GTK_STATE_NORMAL,
                          &bgcolor_unmarked);
}

void
board_widget_initbg (BoardWidget *boardw)
{
    gint x, y;
    
    for (y = 0; y < boardw->height; ++y)
        for (x = 0; x < boardw->width; ++x)
            board_widget_unmark_field (boardw, x, y);
}

void
board_widget_mark_field (BoardWidget *boardw, gint x, gint y,
                         gdouble scale)
{
    GdkColor color;
    guint compl;

    g_assert (scale >= 0 && scale <= 1);
    
    compl = .8 * (1 - pow(scale, 3.)) * 0xffff;
    color.pixel = 0;
    color.red = compl;
    color.green = 0xffff;
    color.blue = compl;
    gtk_widget_modify_bg (FIELD(boardw, x, y), GTK_STATE_NORMAL,
                          &color);
}

void
board_widget_set_active (BoardWidget *boardw, gboolean is_active)
{
    boardw->is_active = is_active;
}

static void
event_box_clicked_cb(GtkWidget *event_box, GdkEventButton *event,
        gpointer user_data)
{
    gint i, j;
    BoardWidget *boardw = BOARD_WIDGET (user_data);

    if (!boardw->is_active)
        return;

    for (i = 0; i < boardw->width; ++i)
        for (j = 0; j < boardw->height; ++j) {
            if (FIELD (boardw, i, j) == event_box) {
                coord *c = g_new (coord, 1);
                c->x = i;
                c->y = j;
                g_signal_emit (G_OBJECT (boardw),
                               board_widget_signals[FIELD_PRESSED_SIGNAL], 0,
                               c);
                return;
            }
        }
}

