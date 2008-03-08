#ifndef __BOARD_H__
#define __BOARD_H__

#include <glib.h>

typedef struct _board board;
typedef struct _coord coord;
typedef guchar letter; /* indexes of alphabet incremented by one 
                          in order to keep zero marking end of array */

struct _board
{
    GPtrArray *alphabet;  /* NULL terminated array of UTF-8 strings */
    letter *letters;   /* width*height sized array */
    GNode *trie;
    gint width;
    gint height;
};

struct _coord
{
    gint x;
    gint y;
};


board *
board_new (gint width, 
           gint height,
           GPtrArray *alphabet,
           const gint *weights,
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
letter2gcharp (GPtrArray *alphabet,
               letter l);

letter
gcharp2letter (GPtrArray *alphabet,
               const gchar *ch);

/* returns GPtrArray of newly allocated array of letters
 * returns NULL if str contains characters not contained by alphabet */
GPtrArray *
str2letters (GPtrArray *alphabet,
             const gchar *str);

/* returns zero terminated array, must be freed by the caller */
letter *
path2letters (const board *brd,
                GArray *path);

/* creates empty path */
GArray *
path_new ();

coord
path_index (GArray *path, gint i);

/* frees coord array with member coords */
void
path_free (GArray *path);

gint
path_length (GArray *path);

#endif /* __BOARD_H__ */

