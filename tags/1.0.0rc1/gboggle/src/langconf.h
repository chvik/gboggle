#ifndef __LANGCONF_H__
#define __LANGCONF_H__

#include <glib.h>

#define GLOBAL_CONF_DIR SYSCONFDIR
#define GLOBAL_CONF_NAME "gbogglerc"
#define USER_CONF_DIR ""   /* under home dir */
#define USER_CONF_NAME ".gbogglerc"
#define MAX_LETTER_LENGTH 2

struct langconf
{
    gchar *lang;        /* name of language */
    GPtrArray *alphabet;   /* array of strings like "a", "b", "qu"...
                           NULL terminated */
    GArray *weights;     /* weights of letters */
    gchar *dictf;       /* file name of word list */
};

enum parser_state
{
    PARSER_INIT,
    PARSER_LANG,
    PARSER_DICTFILE_KEYWORD,
    PARSER_DICTFILE,
    PARSER_LETTER,
    PARSER_WEIGHT
};

GPtrArray *
read_langconf();


GPtrArray *
read_langconf_from (gchar *filename);

#define LANGCONF_ERROR_MSG(msg,file,line) fprintf (stderr, "%s in %s line %d\n", (msg), (file), (line))

#endif /* __LANGCONF_H__ */
