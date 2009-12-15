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
static void board_widget_reset_content(BoardWidget *boardw);
static void size_request_cb(GtkWidget *boardw, GtkRequisition *requisition,
                            gpointer user_data);
static void size_allocate_cb(GtkWidget *boardw, GtkAllocation *allocation,
                             gpointer user_data);

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
    boardw->font_size = BOARD_WIDGET_FONT_SIZE;
    boardw->brd = NULL;

    g_signal_connect(G_OBJECT(boardw), "size-request", 
                     G_CALLBACK(size_request_cb),
                     boardw);
    g_signal_connect(G_OBJECT(boardw), "size-allocate", 
                     G_CALLBACK(size_allocate_cb),
                     boardw);
}

GtkWidget *
board_widget_new (gint width, gint height)
{
    gint i, j;
    BoardWidget *boardw = BOARD_WIDGET (g_object_new (board_widget_get_type (), 
                                                      NULL));
    GtkStyle *style;

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
            gtk_widget_set_name(evbox, "BoardCell");
            g_signal_connect (G_OBJECT (evbox), "button-release-event",
                    G_CALLBACK (event_box_clicked_cb), boardw);
            label = gtk_label_new ("");
            gtk_widget_set_name(label, "BoardLetter");
            gtk_label_set_width_chars (GTK_LABEL (label), 2);
            gtk_container_add (GTK_CONTAINER (evbox), label);
            FIELD (boardw, i, j) = evbox;
            gtk_table_attach_defaults (GTK_TABLE (boardw), 
                    evbox, i, i+1, j, j+1);
            
            gtk_widget_show (label);
            gtk_widget_show (evbox);
        }
    
    style = gtk_rc_get_style(FIELD(boardw, 0, 0));
    boardw->normalBgColor = style->bg[GTK_STATE_NORMAL];
    boardw->selectedBgColor = style->bg[GTK_STATE_SELECTED];

    board_widget_initbg (boardw);

    return GTK_WIDGET (boardw);
}

void
board_widget_init_with_board (BoardWidget *boardw, board *brd)
{
    gint i, j;

    g_assert (brd->width == boardw->width);
    g_assert (brd->height == boardw->height);

    boardw->brd = brd;
    board_widget_reset_content(boardw);
    board_widget_initbg (boardw);
}

static void
board_widget_unmark_field (BoardWidget *boardw, gint x, gint y)
{
    GtkWidget *field = FIELD(boardw, x, y);
    gtk_widget_modify_bg (field, GTK_STATE_NORMAL, &boardw->normalBgColor);
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
    GdkColor normalColor;
    GdkColor selectedColor;
    GdkColor color;
    double alpha;
    GtkWidget *field = FIELD(boardw, x, y);
    GtkStyle *style = gtk_rc_get_style(field);

    g_assert (scale >= 0.0 && scale <= 1.0);
    
    normalColor = boardw->normalBgColor;
    selectedColor = boardw->selectedBgColor;

//    alpha = .7 * (1 - pow(scale, 3.));
    alpha = .7 * (1 - scale*scale);
    color.pixel = 0;
    color.red = alpha * normalColor.red + (1 - alpha) * selectedColor.red;
    color.green = alpha * normalColor.green + (1 - alpha) * selectedColor.green;
    color.blue = alpha * normalColor.blue + (1 - alpha) * selectedColor.blue;
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

static void                
size_request_cb(GtkWidget *widget,
                GtkRequisition *requisition,
                gpointer user_data)
{
//    g_debug ("request: %d, %d", requisition->width, requisition->height);
}

static void
size_allocate_cb (GtkWidget *widget,
                  GtkAllocation *allocation,
                  gpointer user_data)
{
    BoardWidget *boardw = BOARD_WIDGET(widget);
    gint font_size = 
        MIN ((double)allocation->width / boardw->width,
             (double)allocation->height / boardw->height) / 
        BOARD_WIDGET_FIELD_FONT_RATIO;
//    g_debug ("allocation: %d %d", allocation->width, allocation->height);
    boardw->fieldw = allocation->width / boardw->width;
    boardw->fieldh = allocation->height / boardw->height;
 
    if (font_size != boardw->font_size) {
        boardw->font_size = font_size;
        board_widget_reset_content (boardw);
    }
}

static void
board_widget_reset_content(BoardWidget *boardw)
{
    gint i, j;

//    g_debug("reset content %d", boardw->font_size);

    if (!boardw->brd)
        return;

    for (i = 0; i < boardw->width; ++i)
        for (j = 0; j < boardw->height; ++j) {
            GtkWidget *label;
            gchar *upper;
            gchar *markup;

            label = gtk_bin_get_child (GTK_BIN (FIELD (boardw, i, j)));
            upper = g_utf8_strup (board_gcharp_at (boardw->brd, i, j), -1);
            markup = g_strdup_printf ("<span font_desc=\"%d\">%s</span>",
                                      boardw->font_size,
                                      upper);
            gtk_label_set_markup (GTK_LABEL (label), markup);
            g_free (upper);
            g_free (markup);
        }
}

