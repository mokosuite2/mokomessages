/*
 * MokoMessages
 * Entry point
 * Copyright (C) 2009-2010 Daniele Ricci <daniele.athome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <mokosuite/utils/utils.h>
#include <mokosuite/ui/gui.h>
#include <mokosuite/pim/pim.h>
#include <mokosuite/pim/messagesdb.h>
#include <freesmartphone-glib/freesmartphone-glib.h>
#include <phone-utils.h>

#include "globals.h"
#include "threadwin.h"

// default log domain
int _log_dom = -1;

int main(int argc, char* argv[])
{
    // TODO: initialize Intl
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    // initialize log
    eina_init();
    _log_dom = eina_log_domain_register(PACKAGE, EINA_COLOR_CYAN);
    eina_log_domain_level_set(PACKAGE, LOG_LEVEL);

    EINA_LOG_INFO("%s version %s", PACKAGE_NAME, VERSION);

    /* other things */
    mokosuite_utils_init();
    mokosuite_pim_init();
    mokosuite_ui_init(argc, argv);
    phone_utils_init();

    EINA_LOG_DBG("Loading data from %s", MOKOMESSAGES_DATADIR);
    elm_theme_extension_add(NULL, MOKOMESSAGES_DATADIR "/theme.edj");

    thread_win_init(NULL);

    // TEST mostra la finestra delle conversazioni
    thread_win_activate();

    elm_run();
    elm_shutdown();

    return EXIT_SUCCESS;
}
