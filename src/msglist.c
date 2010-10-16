#include <mokosuite/ui/gui.h>
#include <mokosuite/pim/messagesdb.h>
#include <mokosuite/utils/misc.h>
//#include <mokosuite/utils/settings-service.h>

#include "globals.h"
#include "msglist.h"

#include <glib/gi18n-lib.h>

/* finestra principale */
static MokoWin* win = NULL;

static void _delete(void* mokowin, Evas_Object* obj, void* event_info)
{
    mokowin_hide((MokoWin *)mokowin);
}

void msg_list_activate(void)
{
    g_return_if_fail(win != NULL);

    mokowin_activate(win);
}

void msg_list_hide(void)
{
    g_return_if_fail(win != NULL);

    mokowin_hide(win);
}

static Evas_Object* create_bubble(const char* peer, const char* text, const char* side)
{
    Evas_Object* msg = elm_bubble_add(win->win);
    evas_object_size_hint_weight_set(msg, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(msg, EVAS_HINT_FILL, 1.0);
    elm_bubble_label_set(msg, peer);
    elm_bubble_info_set(msg, get_time_repr(time(NULL)));
    elm_bubble_corner_set(msg, side);
    evas_object_show(msg);

    Evas_Object* lbl = elm_label_add(win->win);
    evas_object_size_hint_weight_set(lbl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lbl, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_label_line_wrap_set(lbl, TRUE);
    elm_label_label_set(lbl, text);
    evas_object_show(lbl);
    elm_bubble_content_set(msg, lbl);

    elm_box_pack_end(win->vbox, msg);
    return msg;
}

static void test_messages(void)
{
    create_bubble(
        "155",
        "Messaggio di testo<br>Ciao zio come stai?<br>Io bene :)<br>Bella!",
        "bottom_left"
    );
    create_bubble(
        "+393296061565",
        "Test linee lunghe qua una volta c'era un bel messaggio l'ho dovuto togliere perche' svn e' pubblico :(",
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

static Evas_Object* make_menu(void)
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

void msg_list_init(MessageThread* thread)
{
    win = mokowin_new("mokosmsthread", TRUE);
    if (win == NULL) {
        g_error("[MSgList] Cannot create main window. Exiting");
        return;
    }

    win->delete_callback = _delete;

    char* s = g_strdup_printf(_("Conversation with %s"), thread->peer);
    elm_win_title_set(win->win, s);
    g_free(s);

    //elm_win_borderless_set(win->win, TRUE);

    mokowin_create_vbox(win, TRUE);
    mokowin_menu_enable(win);

    mokowin_menu_set(win, make_menu());

    test_messages();
    // TODO carica le conversazioni :)
    // TODO g_idle_add(...);
}
