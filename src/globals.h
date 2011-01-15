/*
 * MokoMessages
 * Globals definitions
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

#ifndef __GLOBALS_H
#define __GLOBALS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Elementary.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <mokosuite/pim/contactsdb.h>
#include <mokosuite/pim/messagesdb.h>
#include <mokosuite/ui/gui.h>

// default log domain
#undef EINA_LOG_DOMAIN_DEFAULT
#define EINA_LOG_DOMAIN_DEFAULT _log_dom
extern int _log_dom;

#ifdef DEBUG
#define LOG_LEVEL   EINA_LOG_LEVEL_DBG
#else
#define LOG_LEVEL   EINA_LOG_LEVEL_INFO
#endif

#define MOKOMESSAGES_SYSCONFDIR     SYSCONFDIR "/mokosuite"
#define MOKOMESSAGES_DATADIR        DATADIR "/mokosuite/messages"

typedef struct {
    /* -- thread win stuff -- */

    /* thread list item */
    Elm_Genlist_Item* list_item;

    /* contact entry */
    ContactEntry* contact;

    /* -- message list stuff -- */

    /* message list window */
    MokoWin* message_list;

    /* reply entry */
    Evas_Object* reply_entry;

    /* send button */
    Evas_Object* send_button;

    /* message list is being destroyed */
    bool destroying;

    /* message list is loading */
    bool loading;

    /* a message is being sent */
    bool sending;

    /* number of messages requested */
    int requested;

    /* number of messages loaded */
    int loaded;

    /* MessagesDB query */
    void* query;

} thread_data_t;


typedef struct {
    /* thread */
    MessageThread* thread;

    /* content AnchorBlock */
    Evas_Object* content;

    /* hbox for content + icon */
    Evas_Object* hbox;

    /* status icon */
    Evas_Object* status;

    /* error flag */
    bool error;

} message_data_t;

#endif  /* __GLOBALS_H */
