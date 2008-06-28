#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdio.h>

#include "langconf.h"
#include "util.h"

static void
remove_langs_without_dictionary(GPtrArray *lang_conf);

const GScannerConfig scanner_conf = {
    /* skip chars */
    " \t\n",
    G_CSET_a_2_z G_CSET_A_2_Z,
    G_CSET_a_2_z G_CSET_A_2_Z "0123456789_",
    /* single comments */
    "#\n",
    /* case sensitive */
    TRUE,
    /* skip comments */
    TRUE, TRUE,
    /* scan multi line comments */
    FALSE,
    /* identifiers */
    TRUE, TRUE, FALSE, TRUE,
    /* number formats */
    FALSE, FALSE, FALSE, FALSE, FALSE,
    /* strings */
    TRUE, TRUE,
    /* conversions */
    TRUE, FALSE, TRUE, TRUE, FALSE,
    /* options */
    FALSE, FALSE, 0,
};
    

GPtrArray *
read_langconf()
{
    gchar *filename;
    const gchar *home;
    GPtrArray *conf = NULL;

    home = g_get_home_dir ();
    if (home)
    {
        filename = g_build_filename (home, USER_CONF_DIR, USER_CONF_NAME,
                                     NULL);
        conf = read_langconf_from (filename);    
    }
    if (!conf)
    {
        filename = g_build_filename (GLOBAL_CONF_DIR, GLOBAL_CONF_NAME,
                                     NULL);
        conf = read_langconf_from (filename);
    }
    
    return conf;   
}

GPtrArray *
read_langconf_from (gchar *filename)
{
    GScanner *scanner;
    GTokenType ttype;
    gint fd;
    struct langconf *conf = NULL;
    enum parser_state state;
    GPtrArray *lang_conf;
    gboolean error_occured = FALSE;

    fd = open (filename, O_RDONLY);
    if (fd < 0)
    {
        /*g_printf ("%s: couldn't open\n", filename);*/
        return NULL;
    }

    state = PARSER_INIT;
    lang_conf = g_ptr_array_new ();

    scanner = g_scanner_new (&scanner_conf);
/*    scanner = g_scanner_new (NULL);*/
    g_scanner_input_file (scanner, fd);
    scanner->input_name = filename;

   for (ttype = g_scanner_get_next_token (scanner);
             ttype != G_TOKEN_EOF;
             ttype = g_scanner_get_next_token (scanner))
    {
        gint line = g_scanner_cur_line (scanner);
        /* guint pos = g_scanner_cur_position (scanner);*/
        error_occured = FALSE;

        if(state == PARSER_INIT || state == PARSER_LETTER)
        {
            if (ttype == G_TOKEN_STRING &&
                    strcmp (scanner->value.v_string, "language") == 0)
            {
                if (conf)
                {
                    g_ptr_array_add (lang_conf, conf);
                    DEBUGMSG ("lang %s added\n", conf->lang);
                }
                state = PARSER_LANG;
                conf = g_new0 (struct langconf, 1);
                conf->alphabet = g_ptr_array_new ();
                conf->weights = g_array_new (TRUE, TRUE, sizeof (gint));
                continue;
            }
            else if (state == PARSER_INIT)
            {
                LANGCONF_ERROR_MSG ("keyword language expected", filename, line);
                error_occured = TRUE;
                break;
            }
        }

        switch (state)
        {
            case PARSER_INIT:
                g_assert_not_reached ();
                break;
                   
            case PARSER_LANG:
                if (ttype != G_TOKEN_STRING)
                {
                    LANGCONF_ERROR_MSG ("language name expected after keyword language", filename, line);
                    error_occured = TRUE;
                    break;
                }
                conf->lang = g_strdup (scanner->value.v_string);
                state = PARSER_DICTFILE_KEYWORD;
                break;

            case PARSER_DICTFILE_KEYWORD:
                if (ttype != G_TOKEN_STRING || 
                    strcmp (scanner->value.v_string, "dictfile") != 0)
                {
                    LANGCONF_ERROR_MSG ("keyword \"dictfile\" expected",
                            filename, line);
                    error_occured = TRUE;
                    break;
                }
                state = PARSER_DICTFILE;
                break;

            case PARSER_DICTFILE:
                if (ttype != G_TOKEN_STRING)
                {
                    LANGCONF_ERROR_MSG ("dict filename expected", filename,
                                        line);
                    error_occured = TRUE;
                    break;
                }
                conf->dictf = g_strdup (scanner->value.v_string);
                state = PARSER_LETTER;
                break;

             case PARSER_LETTER:
                if (ttype != G_TOKEN_STRING ||
                        strlen(scanner->value.v_string) > MAX_LETTER_LENGTH)
                {
                    LANGCONF_ERROR_MSG ("letter expected", filename, line);
                    error_occured = TRUE;
                    break;
                }
                g_ptr_array_add (conf->alphabet, 
                        g_strdup (scanner->value.v_string));
                state = PARSER_WEIGHT;
                break;

            case PARSER_WEIGHT:
                if (ttype != G_TOKEN_INT)
                {
                    LANGCONF_ERROR_MSG ("number expected", filename, line);
                    error_occured = TRUE;
                    break;
                }
                g_array_append_val (conf->weights, scanner->value.v_int);
                state = PARSER_LETTER;
                break;
        }

        if (error_occured)
            break;

    } /* for get_next_token */

    if (error_occured)
    {
        gint i;

        for (i = 0; i < lang_conf->len; ++i)
        {
            struct langconf *c = 
                (struct langconf *)g_ptr_array_index (lang_conf, i);                
            g_ptr_array_free (c->alphabet, TRUE);
            g_free (c->weights);
            g_free (c->dictf);
            g_free (c->lang);
        }

        g_ptr_array_free (lang_conf, TRUE);
        return NULL;
    }

    /* adding last language */
    if (conf)
    {
        g_ptr_array_add (lang_conf, conf);
        DEBUGMSG ("lang %s added\n", conf->lang);
    }

    remove_langs_without_dictionary(lang_conf);

    return lang_conf;
}

static void
remove_langs_without_dictionary(GPtrArray *lang_conf)
{
    gint i = 0;

    while (i < lang_conf->len)
    {
        struct langconf *conf = g_ptr_array_index (lang_conf, i);
        if (!g_file_test(conf->dictf, G_FILE_TEST_EXISTS) ||
            !g_file_test(conf->dictf, G_FILE_TEST_IS_REGULAR))
        {
            g_warning ("%s not found, removing %s", conf->dictf, conf->lang);
            g_ptr_array_remove_index (lang_conf, i);
            g_free (conf);
        }
        else
        {
            ++i;
        }
    }
}
