#ifndef UTIL_H
#define UTIL_H

#include <libintl.h>
#include <glib.h>

#define _(Str) gettext (Str)

#ifdef DEBUG
#define DEBUGMSG(...) g_debug(__VA_ARGS__)
#else
#define DEBUGMSG(...)
#endif

typedef struct _GBoggleSettings GBoggleSettings;
struct _GBoggleSettings {    
    gchar *language;
};

GBoggleSettings *load_settings(void);
void save_settings(const GBoggleSettings *);

#endif /* UTIL_H */
