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

#endif /* UTIL_H */
