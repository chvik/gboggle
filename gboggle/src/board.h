#ifndef __BOARD_H__
#define __BOARD_H__

#include <glib.h>

typedef struct _board board;
typedef struct _coord coord;
typedef guchar letter; /* indexes of alphabet incremented by one 
                          in order to keep zero marking end of array */

struct _board
{
    const gchar * const *alphabet;  /* NULL terminated array of UTF-8 strings */
    letter *letters;   /* width*height sized array */
    GNode *trie;
    gint width;
    gint height;
};

struct _coord
{
    guint x;
    guint y;
};


board *
board_new (gint width, 
           gint height,
           const gchar * const *alphabet,
           const guint *weights,
           GNode *trie);

void
board_dispose (board *brd);

const gchar *
board_gcharp_at (const board *brd,
                 gint x,
                 gint y);

letter
board_letter_at (const board *brd,
                 gint x,
                 gint y);

const gchar *
letter2gcharp (const gchar * const *alphabet,
               letter l);

letter
gcharp2letter (const gchar * const *alphabet,
               const gchar *ch);

/* returns GPtrArray of newly allocated array of letters
 * returns NULL if str contains characters not contained by alphabet */
GPtrArray *
str2letters (const gchar * const *alphabet,
             const gchar *str);
letter *
str2letters_ (const gchar * const *alphabet,
             const gchar *str);



/* returns zero terminated array, must be freed by the caller */
letter *
coords2letters (const board *brd,
                const coord **path);

#endif /* __BOARD_H__ */
