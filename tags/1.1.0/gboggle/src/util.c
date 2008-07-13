#include <gconf/gconf-client.h>
#include "util.h"

#define GCONF_PREFIX "/apps/gboggle"
#define GCONF_KEY_LANGUAGE GCONF_PREFIX "/language"

/**
 * returned struct is owned by the caller
 */
GBoggleSettings *
load_settings (void)
{
    GConfClient *gconf_client = gconf_client_get_default ();
    gchar *language = gconf_client_get_string (gconf_client, 
                                               GCONF_KEY_LANGUAGE, 
                                               NULL);
    GBoggleSettings *settings = g_new0(GBoggleSettings, 1);
    settings->language = language;

    g_object_unref(gconf_client);
    return settings;
}

void
save_settings(const GBoggleSettings *settings)
{    
    GConfClient *gconf_client = gconf_client_get_default();
    g_assert(settings);
    if (settings->language) {
        gboolean res = gconf_client_set_string(gconf_client,
                                               GCONF_KEY_LANGUAGE, 
                                               settings->language, 
                                               NULL);
        if (!res) {
            g_warning("saving %s into gconf was failed", GCONF_KEY_LANGUAGE);
        }
    }
}
