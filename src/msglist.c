#include <mokosuite/ui/gui.h>
#include <mokosuite/pim/messagesdb.h>
#include <mokosuite/utils/misc.h>
//#include <mokosuite/utils/settings-service.h>

#include "globals.h"
#include "msglist.h"

#include <glib/gi18n-lib.h>

static void _delete(void* mokowin, Evas_Object* obj, void* event_info)
{
    mokowin_hide((MokoWin *)mokowin);
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
    elm_bubble_label_set(msg, peer);
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

#if 0
    thread_t* t;
    t = g_new0(thread_t, 1);
    t->peer = "155";
    t->text = "Le abbiamo addebitato 9 euro per l'offerta del cazzo aggiuntivo.";
    t->timestamp = time(NULL);
    t->marked = TRUE;

    t = g_new0(thread_t, 1);
    t->peer = "+393296061565";
    t->text = "Ciau more ti amo troppiximo!! Lo sai oggi ti ho fatto un regalino!:)!";
    t->timestamp = time(NULL);
    t->marked = FALSE;
#endif
}

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

    Evas_Object* reply = elm_entry_add(win->win);
    elm_entry_single_line_set(reply, FALSE);
    evas_object_size_hint_weight_set(reply, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(reply, EVAS_HINT_FILL, 0.0);

    mokowin_pack_end(win, reply, TRUE);
    evas_object_show(reply);

    // store window :)
    t->data[THREAD_DATA_MSGLIST] = win;

    // TEST
    evas_object_resize(win->win, 480, 640);
    test_messages(t);

    /* TODO scroll to end
    int h;
    evas_object_geometry_get(win->vbox, NULL, NULL, NULL, &h);
    elm_scroller_region_show(win->scroller, 0, h, 480, 640);
    */
}
