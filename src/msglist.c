#include <mokosuite/ui/gui.h>
#include <mokosuite/pim/messagesdb.h>
#include <mokosuite/utils/misc.h>
//#include <mokosuite/utils/settings-service.h>

#include "globals.h"
#include "msglist.h"

#include <glib/gi18n-lib.h>

static bool _delete_real(void* mokowin)
{
    MokoWin* win = MOKO_WIN(mokowin);
    mokowin_destroy(win);

    // TODO free message entries

    return FALSE;
}

static void _delete(void* mokowin, Evas_Object* obj, void* event_info)
{
    MokoWin* win = MOKO_WIN(mokowin);
    mokowin_hide(win);

    MessageThread* t = win->data;
    messagesdb_disconnect(t->peer);

    // destroy window (delayed)
    ecore_idler_add(_delete_real, win);
}

void msg_list_activate(MessageThread* t)
{
    g_return_if_fail(t->data[THREAD_DATA_MSGLIST] != NULL);

    mokowin_activate(MOKO_WIN(t->data[THREAD_DATA_MSGLIST]));
}

void msg_list_hide(MessageThread* t)
{
    g_return_if_fail(t->data[THREAD_DATA_MSGLIST] != NULL);

    mokowin_hide(MOKO_WIN(t->data[THREAD_DATA_MSGLIST]));
}

static Evas_Object* create_bubble(MokoWin* win, const char* peer, const char* text, const char* side)
{
    Evas_Object* msg = elm_bubble_add(win->win);
    evas_object_size_hint_weight_set(msg, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(msg, EVAS_HINT_FILL, 0.0);
    //elm_bubble_label_set(msg, peer);
    elm_bubble_info_set(msg, get_time_repr(time(NULL)));
    elm_bubble_corner_set(msg, side);
    evas_object_show(msg);

    Evas_Object* lbl = elm_anchorblock_add(win->win);
    evas_object_size_hint_weight_set(lbl, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(lbl, EVAS_HINT_FILL, 0.0);
    elm_anchorblock_text_set(lbl, text);
    evas_object_show(lbl);
    elm_bubble_content_set(msg, lbl);

    elm_box_pack_end(win->vbox, msg);
    return msg;
}

static void test_messages(MessageThread* t)
{
    create_bubble(
        MOKO_WIN(t->data[THREAD_DATA_MSGLIST]),
        "+393296061565",
        "Messaggio di testo<br>Ciao zio come stai?<br>Io bene :)<br>Bella!",
        "bottom_left"
    );
    create_bubble(
        MOKO_WIN(t->data[THREAD_DATA_MSGLIST]),
        "+393296061565",
        "Test linee lunghe qua una volta c'era un bel messaggio l'ho dovuto togliere perche' svn e' pubblico :(",
        "bottom_right"
    );
    create_bubble(
        MOKO_WIN(t->data[THREAD_DATA_MSGLIST]),
        "+393296061565",
        "Ciao :)",
        "bottom_left"
    );
    create_bubble(
        MOKO_WIN(t->data[THREAD_DATA_MSGLIST]),
        "+393296061565",
        "Bella zio!!! :)<br>ZIOOOO!!! CACCHIOOO!!!!<br>CIAO :)",
        "bottom_right"
    );
}

static bool scroll_down(void* data)
{
    MokoWin* win = data;
    int h;
    evas_object_geometry_get(win->vbox, NULL, NULL, NULL, &h);
    elm_scroller_region_show(win->scroller, 0, h, 480, 640);
    return FALSE;
}

static void _message(MessageEntry* e, void* userdata)
{
    MessageThread* t = userdata;

    if (e != NULL) {
        EINA_LOG_DBG("Message %d from %s", e->id, e->peer);
        create_bubble(MOKO_WIN(t->data[THREAD_DATA_MSGLIST]),
            e->peer,
            e->content,
            (e->direction == DIRECTION_INCOMING) ? "bottom_right" : "bottom_left"
        );
    }

    // no more messages, scroll down!
    else {
        ecore_idler_add(scroll_down, t->data[THREAD_DATA_MSGLIST]);
    }
}

static void _load_messages(MessageThread* t)
{
    messagesdb_foreach(_message, t->peer, FALSE, t);
}

static Evas_Object* make_composer(MokoWin* win)
{
    Evas_Object* hbox = elm_box_add(win->win);
    elm_box_horizontal_set(hbox, TRUE);
    evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, 0.0);

    Evas_Object* reply = elm_entry_add(win->win);
    elm_entry_single_line_set(reply, FALSE);
    evas_object_size_hint_weight_set(reply, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(reply, EVAS_HINT_FILL, EVAS_HINT_FILL);

    elm_box_pack_start(hbox, reply);
    evas_object_show(reply);

    Evas_Object* send = elm_button_add(win->win);
    elm_button_label_set(send, _("Send"));
    evas_object_size_hint_weight_set(send, 0.0, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(send, 1.0, EVAS_HINT_FILL);

    elm_box_pack_end(hbox, send);
    evas_object_show(send);

    return hbox;
}

/* TODO */
static Evas_Object* make_menu(MokoWin* win)
{
    Evas_Object *m = elm_table_add(win->win);
    elm_table_homogenous_set(m, TRUE);
    evas_object_size_hint_weight_set(m, 1.0, 0.0);
    evas_object_size_hint_align_set(m, -1.0, 1.0);

    /* pulsante nuovo messaggio */
    Evas_Object *bt_compose = mokowin_menu_hover_button(win, m, _("Compose"), 0, 0, 1, 1);

    /* pulsante cancella tutto */
    Evas_Object *bt_del_all = mokowin_menu_hover_button(win, m, _("Delete all"), 1, 0, 1, 1);

    /* pulsante impostazioni */
    Evas_Object *bt_settings = mokowin_menu_hover_button(win, m, _("Settings"), 2, 0, 1, 1);

    return m;
}

void msg_list_init(MessageThread* t)
{
    MokoWin* win = mokowin_new("mokosmsthread", TRUE);
    if (win == NULL) {
        g_error("[MsgList] Cannot create main window. Exiting");
        return;
    }

    win->delete_callback = _delete;

    char* s = g_strdup_printf(_("Conversation with %s"), t->peer);
    elm_win_title_set(win->win, s);
    g_free(s);

    mokowin_create_vbox(win, TRUE);
    mokowin_menu_enable(win);

    mokowin_menu_set(win, make_menu(win));

    Evas_Object* reply = make_composer(win);
    mokowin_pack_end(win, reply, TRUE);
    evas_object_show(reply);

    // store some useful stuff :)
    t->data[THREAD_DATA_MSGLIST] = win;
    win->data = t;

    messagesdb_connect(_message, t->peer, t);

    // load messages!
    _load_messages(t);

    // TEST
    evas_object_resize(win->win, 480, 640);
}
