#include <gconf/gconf-client.h>
#include "util.h"

#define GCONF_PREFIX "/apps/gboggle"
#define GCONF_KEY_LANGUAGE GCONF_PREFIX "/language"
#define GCONF_KEY_GAME_TIME GCONF_PREFIX "/game_time"

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
    gint game_time = gconf_client_get_int (gconf_client, 
                                          GCONF_KEY_GAME_TIME, 
                                          NULL);
    GBoggleSettings *settings = g_new0(GBoggleSettings, 1);
    settings->language = language;
    if (game_time == -1) settings->game_time = 0;
    else if (!game_time) settings->game_time = GAME_LENGTH_SEC;
    else settings->game_time = game_time;
    DEBUGMSG("Game time: %d\n", settings->game_time);

    g_object_unref(gconf_client);
    return settings;
}

void
save_settings(const GBoggleSettings *settings)
{    
    GConfClient *gconf_client = gconf_client_get_default();
    gboolean res;
    gint saved_game_time;
    g_assert(settings);
    if (settings->language) {
        res = gconf_client_set_string(gconf_client,
                                               GCONF_KEY_LANGUAGE, 
                                               settings->language, 
                                               NULL);
        if (!res) {
            g_warning("saving %s into gconf was failed", GCONF_KEY_LANGUAGE);
        }
    }
    
    if (settings->game_time == 0) { saved_game_time = -1; }
    else { saved_game_time = settings->game_time; }
     
    res = gconf_client_set_int(gconf_client,
                              GCONF_KEY_GAME_TIME, 
                              saved_game_time, 
                              NULL);
    if (!res) {
    	g_warning("saving %s into gconf was failed", GCONF_KEY_GAME_TIME);
    }
    else
    {
      	DEBUGMSG("Saved game time as %d\n", saved_game_time);
	}
}
