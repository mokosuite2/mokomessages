#include <mokosuite/ui/gui.h>
#include <mokosuite/pim/messagesdb.h>
#include <mokosuite/utils/misc.h>
#include <freesmartphone-glib/ogsmd/call.h>
#include <freesmartphone-glib/ogsmd/sms.h>
#include <freesmartphone-glib/opimd/messages.h>
#include <phone-utils.h>

#include "globals.h"
#include "msglist.h"
#include "threadwin.h"

#include <glib/gi18n-lib.h>

// size of first block of messages to load
#define FIRST_BLOCK_COUNT       10
// size of next blocks of messages to load on scroll
#define NEXT_BLOCK_COUNT        5

typedef struct {
    GHashTable* pim_data;
    char* content;
    guint64 timestamp;

    MessageThread* thread;
} sms_send_pack;

static Evas_Object* make_menu(MokoWin* win, MessageThread* t);

// message pim callbacks
static void _message(MessageEntry* e, void* userdata, bool prepend);
static void _message_prepend(MessageEntry* e, void* userdata);
static void _message_append(MessageEntry* e, void* userdata);

static bool _delete_real(void* mokowin)
{
    MokoWin* win = MOKO_WIN(mokowin);
    mokowin_destroy(win);

    Eina_List* iter;
    MessageEntry* e;
    MessageThread* t = win->data;
    thread_data_t* th_data = (thread_data_t*) t->data;

    // reset all
    th_data->requested = 0;

    EINA_LIST_FOREACH(th_data->entries, iter, e) {
        messagesdb_free_entry(e);
        g_free(e->data);
    }

    // cleanup
    th_data->message_list = NULL;
    th_data->entries = eina_list_free(th_data->entries);
    th_data->destroying = FALSE;
    th_data->loading = FALSE;
    th_data->sending = FALSE;   // just for cleanup

    // TODO free message entries (?)

    return FALSE;
}

static void _delete(void* mokowin, Evas_Object* obj, void* event_info)
{
    MokoWin* win = MOKO_WIN(mokowin);
    MessageThread* t = win->data;
    thread_data_t* th_data = (thread_data_t*) t->data;

    // destroyed window flag
    th_data->destroying = TRUE;

    // disconnect signals if we're not sending
    if (!th_data->sending)
        messagesdb_disconnect(t->peer);

    if (!th_data->loading && !th_data->sending)
        _delete_real(win);
    else
        // hide and wait for loader or sender to destroy the window
        mokowin_hide(win);
}

static Eina_Bool _load_next_bunch(void* th)
{
    MessageThread* t = (MessageThread*) th;
    thread_data_t* th_data = (thread_data_t*) t->data;

    // load next bunch of messages
    th_data->query = messagesdb_foreach(_message_append, t->peer, TRUE, th_data->requested, NEXT_BLOCK_COUNT, t);
    th_data->requested += NEXT_BLOCK_COUNT;

    return FALSE;
}

static void _bottom(void* th, Evas_Object* obj, void* event_info)
{
    MessageThread* t = (MessageThread*) th;
    thread_data_t* th_data = (thread_data_t*) t->data;
    EINA_LOG_DBG("bottom edge reached!");

    // load next bunch of messages
    th_data->loading = TRUE;
    ecore_timer_add(0.05, _load_next_bunch, th);
}

void msg_list_activate(MessageThread* t)
{
    thread_data_t* th_data = (thread_data_t*) t->data;
    if (th_data->message_list)
        mokowin_activate(th_data->message_list);
}

void msg_list_hide(MessageThread* t)
{
    thread_data_t* th_data = (thread_data_t*) t->data;
    if (th_data->message_list)
        mokowin_hide(th_data->message_list);
}

static void _options_popup_click(gpointer popup, gpointer data, int index, gboolean final)
{
    MessageEntry* e = (MessageEntry*) data;
    message_data_t* mt = (message_data_t*) e->data;
    MessageThread* t = mt->thread;
    thread_data_t* th_data = (thread_data_t*) t->data;

    switch (index) {

        // delete message
        case 1:
            // set as deleted
            mt->deleting = TRUE;
            // delete from storage
            messagesdb_delete_message(e->id);
            // remove message box
            evas_object_del(mt->container);
            // unreference message
            th_data->entries = eina_list_remove(th_data->entries, e);
            // free data
            messagesdb_free_entry(e);
            g_free(mt);

            break;

        // forward
        case 2:
            // TODO

            break;

        default:
            break;
    }
}

static void _contact_popup_click(gpointer popup, gpointer data, int index, gboolean final)
{
    MessageEntry* e = (MessageEntry*) data;

    switch (index) {

        // call
        case 1:
            EINA_LOG_DBG("calling %s", e->peer);
            ogsmd_call_initiate(e->peer, "voice", NULL, NULL);

            break;

        // add to contacts
        case 2:
            // TODO

            break;

        default:
            break;
    }
}

static void message_anchor(void* me, Evas_Object* lbl, void* event_info)
{
    Elm_Entry_Anchorblock_Info* event = event_info;
    EINA_LOG_DBG("clicked anchor \"%s\" (%d)", event->name, event->button);

    // only left mouse button allowed ;)
    if (event->button != 1) return;

    MessageEntry* e = (MessageEntry*) evas_object_data_get(lbl, "message_entry");
    message_data_t* mt = (message_data_t*) e->data;
    MessageThread* t = mt->thread;
    thread_data_t* th_data = (thread_data_t*) t->data;

    // contact options popup
    if (!strcmp(event->name, "contact")) {
        MokoPopupMenu *p = moko_popup_menu_new(th_data->message_list, NULL, MOKO_POPUP_BUTTONS, _contact_popup_click, e);

        if (e->peer != NULL) {
            // call contact
            char *s = g_strdup_printf(_("Call %s"), e->peer);
            moko_popup_menu_add(p, s, 1, FALSE);
            g_free(s);

            // add to contacts
            moko_popup_menu_add(p, _("Add to contacts"), 2, FALSE);
        }

        mokoinwin_activate(MOKO_INWIN(p));
    }

    // message options popup
    else if (!strcmp(event->name, "options")) {
        MokoPopupMenu *p = moko_popup_menu_new(th_data->message_list, NULL, MOKO_POPUP_BUTTONS, _options_popup_click, e);

        // delete message
        moko_popup_menu_add(p, _("Delete message"), 1, FALSE);

        // forward message
        moko_popup_menu_add(p, _("Forward"), 2, FALSE);

        mokoinwin_activate(MOKO_INWIN(p));
    }
}

static void update_message_status(MessageEntry* e)
{
    message_data_t* mt = (message_data_t*) e->data;
    MessageThread* t = mt->thread;
    thread_data_t* th_data = (thread_data_t*) t->data;

    // window is being destroyed or message is being deleted -- exit
    if (th_data->destroying || mt->deleting) return;

    if (mt->status == NULL) {
        Evas_Object *icon = elm_icon_add(th_data->message_list->win);
        evas_object_show(icon);

        mt->status = icon;
        elm_box_pack_end(mt->hbox, icon);
    }

    const char* file = NULL;
    if (e->direction == DIRECTION_OUTGOING && e->is_new) {
        if (mt->error)
            file = MOKOMESSAGES_DATADIR "/msg_error.png";
        else
            file = MOKOMESSAGES_DATADIR "/msg_pending.png";
    }

    if (file) {
        elm_icon_file_set(mt->status, file, NULL);
        //evas_object_size_hint_min_set(mt->status, 100, 100);
        // TODO icona dimensionata correttamente? :S
        //elm_icon_smooth_set(mt->status, TRUE);
        elm_icon_no_scale_set(mt->status, TRUE);
        elm_icon_scale_set(mt->status, FALSE, FALSE);
    }

    // no icon to set, delete icon
    else if (mt->status) {
        evas_object_del(mt->status);
        mt->status = NULL;
    }
}

static Evas_Object* create_message(MokoWin* win, MessageEntry* e, bool prepend, Evas_Object** hbox, Evas_Object** container)
{
    Evas_Object* msg = elm_frame_add(win->win);
    evas_object_size_hint_weight_set(msg, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(msg, EVAS_HINT_FILL, 0.0);
    elm_object_style_set(msg, "outdent_top");
    evas_object_show(msg);

    *container = msg;

    Evas_Object* lbl = elm_anchorblock_add(win->win);
    evas_object_size_hint_weight_set(lbl, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(lbl, EVAS_HINT_FILL, 0.0);
    evas_object_smart_callback_add(lbl, "anchor,clicked", message_anchor, e);
    evas_object_data_set(lbl, "message_entry", e);

    // create text
    char* stext = g_strdup_printf(
        "<a href=contact><b>%s</b></a>: %s<br><a color=#333 href=options>Options</a><br><color=#333 font_size=7>%s</>",
        (e->direction == DIRECTION_INCOMING) ? e->peer : _("Me"), e->content, get_time_repr(e->timestamp));

    elm_anchorblock_text_set(lbl, stext);
    g_free(stext);

    evas_object_show(lbl);

    Evas_Object* hb = elm_box_add(win->win);
    elm_box_horizontal_set(hb, TRUE);
    evas_object_size_hint_weight_set(hb, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(hb, EVAS_HINT_FILL, 0.0);
    evas_object_show(hb);

    *hbox = hb;

    elm_box_pack_start(hb, lbl);

    elm_frame_content_set(msg, hb);

    if (prepend)
        mokowin_pack_start(win, msg, FALSE);
    else
        mokowin_pack_end(win, msg, FALSE);
    return msg;
}

static void sms_sent(GError* error, gint id, const char* timestamp, gpointer userdata)
{
    MessageEntry* e = userdata;
    message_data_t* mt = (message_data_t*) e->data;
    MessageThread* t = mt->thread;
    thread_data_t* th_data = (thread_data_t*) t->data;

    // sending is finished
    th_data->sending = FALSE;

    if (error) {
        EINA_LOG_WARN("send error: %s", error->message);
        // mark error and notify
        mt->error = TRUE;
        // TODO notification
    }
    else {
        EINA_LOG_DBG("message sent successfully");
        // mark message as sent
        messagesdb_set_message_new(e->id, FALSE);
        e->is_new = FALSE;
        mt->error = FALSE;
    }

    update_message_status(e);

    // message is sent now, we can destroy if we had been ordered to
    if (th_data->destroying)
        ecore_idler_add(_delete_real, th_data->message_list);
}

static void _message_prepend(MessageEntry* e, void* userdata)
{
    // matching new thread!!
    MessageThread* t = (MessageThread*) userdata;
    if (e && userdata && new_thread == t) {
        thread_data_t* th_data = (thread_data_t*) new_thread->data;
        th_data->destroying = TRUE;

        // dummy message_data_t
        message_data_t* mt = calloc(1, sizeof(message_data_t));
        e->data = (void*) mt;
        mt->thread = t;

        EINA_LOG_DBG("sending new message to the network");
        ogsmd_sms_send_text_message(e->peer, e->content, FALSE, sms_sent, e);

        // just close the window for now
        _delete(th_data->message_list, th_data->message_list->win, NULL);
        return;
    }

    else if (!e && new_thread == t) {
        // ok :)
        new_thread = NULL;
        return;
    }
#if 0
    MessageThread* t = (MessageThread*) userdata;
    thread_data_t* th_data = (thread_data_t*) t->data;

    // matching new thread!!
    if (e && userdata && new_thread == t) {
        char* peer = phone_utils_normalize_number(t->peer);
        t = thread_win_get_thread(peer);
        EINA_LOG_DBG("retrieved thread (%s) = %p", peer, t);
        g_free(peer);

        // use current thread to rebuild window
        thread_data_t* th_new = (thread_data_t*) new_thread->data;
        th_data->message_list = th_new->message_list;
        th_data->reply_entry = th_new->reply_entry;
        th_data->send_button = th_new->send_button;

        // free (old) new thread data
        evas_object_del(th_new->rcpt_box);
        g_free(th_new);
        g_free(new_thread);
        new_thread = NULL;
        userdata = t;
    }
#endif

    _message(e, userdata, TRUE);

    // new outgoing message - send it to the network
    if (e && e->direction == DIRECTION_OUTGOING && e->is_new) {
        // re-enable reply entry
        message_data_t* mt = (message_data_t*) e->data;
        MessageThread* t = mt->thread;
        thread_data_t* th_data = (thread_data_t*) t->data;

        EINA_LOG_DBG("sending message to the network");
        ogsmd_sms_send_text_message(e->peer, e->content, FALSE, sms_sent, e);

        // window is being destroyed -- stop here
        if (th_data->destroying) return;

        elm_entry_entry_set(th_data->reply_entry, "");
        elm_object_disabled_set(th_data->send_button, FALSE);
        elm_object_disabled_set(th_data->reply_entry, FALSE);
    }
}

static void _message_append(MessageEntry* e, void* userdata)
{
    _message(e, userdata, FALSE);
}

static void _message(MessageEntry* e, void* userdata, bool prepend)
{
    MessageThread* t = userdata;
    thread_data_t* th_data = (thread_data_t*) t->data;

    if (th_data->destroying) {
        // stop message processing and destroy window
        messagesdb_foreach_stop(th_data->query);
        // destroy window if we're not sending
        if (!th_data->sending)
            ecore_idler_add(_delete_real, th_data->message_list);
    }

    if (e != NULL) {
        EINA_LOG_DBG("Message %d from %s", e->id, e->peer);
        th_data->entries = eina_list_append(th_data->entries, e);

        // create message data
        message_data_t* mt = calloc(1, sizeof(message_data_t));
        e->data = (void*) mt;

        mt->thread = t;
        mt->content = create_message(th_data->message_list, e, prepend, &mt->hbox, &mt->container);

        update_message_status(e);
    }

    // no more messages, scroll down!
    else {
        int c = eina_list_count(th_data->entries);
        EINA_LOG_DBG("%d messages requested, %d loaded", th_data->requested, c);
        th_data->requested = c;
        th_data->loading = FALSE;
    }
}

/* org.freesmartphone.PIM.Messages.Add callback */
static void _message_added(GError* error, const char* path, gpointer userdata)
{
    if (error) {
        EINA_LOG_WARN("error adding message: %s", error->message);
        return;
    }

    EINA_LOG_DBG("message added to opimd");
    // a questo punto aspettiamo il segnale di opimd
}

static void send_message(void* mokowin, Evas_Object* obj, void* event_info)
{
    MokoWin* win = MOKO_WIN(mokowin);
    MessageThread* t = (MessageThread*) win->data;
    thread_data_t* th_data = (thread_data_t*) t->data;

    // commence sending
    th_data->sending = TRUE;

    // disable all
    elm_object_disabled_set(th_data->send_button, TRUE);
    elm_object_disabled_set(th_data->reply_entry, TRUE);
    if (th_data->new_thread)
        elm_object_disabled_set(th_data->rcpt_entry, TRUE);

    time_t ts = time(NULL);

    if (th_data->new_thread)
        t->peer = g_strdup(elm_entry_entry_get(th_data->rcpt_entry));

    // new thread -- connect peer signal
    if (th_data->new_thread) {
        char* peer = phone_utils_normalize_number(t->peer);
        if (!peer) peer = g_strdup(t->peer);
        messagesdb_connect(_message_prepend, peer, t);
        g_free(peer);
    }

    // prepare opimd data
    GHashTable* msg = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_value_free);
    g_hash_table_insert(msg, "Direction", g_value_from_string("out"));
    g_hash_table_insert(msg, "Peer", g_value_from_string(t->peer));
    g_hash_table_insert(msg, "Source", g_value_from_string("SMS"));
    g_hash_table_insert(msg, "Content", g_value_from_string(elm_entry_entry_get(th_data->reply_entry)));
    g_hash_table_insert(msg, "New", g_value_from_bool(TRUE));
    g_hash_table_insert(msg, "Timestamp", g_value_from_int(ts));

    // add message to opimd
    sms_send_pack* pack = calloc(1, sizeof(sms_send_pack));
    pack->thread = t;
    pack->content = g_strdup(elm_entry_entry_get(th_data->reply_entry));
    pack->timestamp = ts;

    opimd_messages_add(msg, _message_added, pack);
}

static void rcpt_changed(void* mokowin, Evas_Object* obj, void* event_info)
{
    MokoWin* win = MOKO_WIN(mokowin);
    mokowin_set_title(win, elm_entry_entry_get(obj));
}

static Evas_Object* make_composer(MokoWin* win)
{
    MessageThread* t = (MessageThread*) win->data;
    thread_data_t* th_data = (thread_data_t*) t->data;

    Evas_Object* hbox = elm_box_add(win->win);
    elm_box_horizontal_set(hbox, TRUE);
    evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, 0.0);

    Evas_Object* reply = elm_entry_add(win->win);
    elm_entry_single_line_set(reply, FALSE);
    evas_object_size_hint_weight_set(reply, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(reply, EVAS_HINT_FILL, EVAS_HINT_FILL);
    th_data->reply_entry = reply;

    elm_box_pack_start(hbox, reply);
    evas_object_show(reply);

    Evas_Object* send = elm_button_add(win->win);
    elm_button_label_set(send, _("Send"));
    evas_object_size_hint_weight_set(send, 0.0, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(send, 1.0, EVAS_HINT_FILL);
    evas_object_smart_callback_add(send, "clicked", send_message, win);
    th_data->send_button = send;

    elm_box_pack_end(hbox, send);
    evas_object_show(send);

    return hbox;
}

static Evas_Object* make_recipient(MokoWin* win)
{
    MessageThread* t = (MessageThread*) win->data;
    thread_data_t* th_data = (thread_data_t*) t->data;

    Evas_Object* hbox = elm_box_add(win->win);
    elm_box_horizontal_set(hbox, TRUE);
    evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, 0.0);
    th_data->rcpt_box = hbox;

    Evas_Object* to = elm_label_add(win->win);
    elm_label_label_set(to, _("To:"));
    evas_object_size_hint_weight_set(to, 0.0, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(to, 0.0, EVAS_HINT_FILL);

    elm_box_pack_start(hbox, to);
    evas_object_show(to);

    Evas_Object* rcpt = elm_entry_add(win->win);
    elm_entry_single_line_set(rcpt, TRUE);
    evas_object_size_hint_weight_set(rcpt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(rcpt, EVAS_HINT_FILL, EVAS_HINT_FILL);

    evas_object_smart_callback_add(rcpt, "changed", rcpt_changed, win);
    th_data->rcpt_entry = rcpt;

    elm_box_pack_end(hbox, rcpt);
    evas_object_show(rcpt);
    return hbox;
}

static void thread_call(void* th, Evas_Object* obj, void* event_info)
{
    MessageThread* t = th;
    thread_data_t* th_data = (thread_data_t*) t->data;

    EINA_LOG_DBG("calling %s", t->peer);
    mokowin_menu_hide(th_data->message_list);
    ogsmd_call_initiate(t->peer, "voice", NULL, NULL);
}

static void thread_cancel(void* th, Evas_Object* obj, void* event_info)
{
    MessageThread* t = th;
    thread_data_t* th_data = (thread_data_t*) t->data;

    EINA_LOG_DBG("discarding message");
    _delete(th_data->message_list, th_data->message_list->win, NULL);
}

static Evas_Object* make_menu(MokoWin* win, MessageThread* t)
{
    thread_data_t* th_data = (thread_data_t*) t->data;
    Evas_Object *m = elm_table_add(win->win);
    elm_table_homogenous_set(m, TRUE);
    evas_object_size_hint_weight_set(m, 1.0, 0.0);
    evas_object_size_hint_align_set(m, -1.0, 1.0);

    if (th_data->new_thread) {
        /* discard message */
        mokowin_menu_hover_button_with_callback(win, m, _("Discard"), 0, 0, 1, 1, thread_cancel, t);
    }

    else {
        /* pulsante nuovo messaggio */
        mokowin_menu_hover_button_with_callback(win, m, _("Call"), 0, 0, 1, 1, thread_call, t);

        /* pulsante cancella tutto */
        mokowin_menu_hover_button(win, m, _("View contact"), 1, 0, 1, 1);

        /* pulsante impostazioni */
        mokowin_menu_hover_button(win, m, _("Delete thread"), 2, 0, 1, 1);
    }

    return m;
}

static MokoWin* new_window(MessageThread* t)
{
    MokoWin* win = mokowin_new("mokosmsthread", TRUE);
    if (win == NULL) {
        EINA_LOG_ERR("cannot create main window");
        return NULL;
    }

    win->delete_callback = _delete;
    win->data = t;

    mokowin_create_vbox(win, TRUE);
    mokowin_menu_enable(win);

    mokowin_menu_set(win, make_menu(win, t));

    // bottom edge reached
    evas_object_smart_callback_add(win->scroller, "edge,bottom", _bottom, t);

    // new thread -- create recipient entry
    thread_data_t* th_data = (thread_data_t*) t->data;
    if (th_data->new_thread) {
        Evas_Object* rcpt = make_recipient(win);
        mokowin_pack_start(win, rcpt, FALSE);
        evas_object_show(rcpt);
    }

    Evas_Object* reply = make_composer(win);
    mokowin_pack_end(win, reply, TRUE);
    evas_object_show(reply);

    // TEST
    evas_object_resize(win->win, 480, 640);
    return win;
}

MessageThread* msg_list_new(const char* peer)
{
    MessageThread* t = calloc(1, sizeof(MessageThread));
    thread_data_t* th_data = calloc(1, sizeof(thread_data_t));

    t->data = (void*) th_data;
    th_data->new_thread = TRUE;

    MokoWin* win = new_window(t);
    elm_win_title_set(win->win, _("New message"));
    mokowin_set_title(win, "");

    th_data->message_list = win;

    return t;
}

void msg_list_init(MessageThread* t)
{
    MokoWin* win = new_window(t);

    // store some useful stuff :)
    thread_data_t* th_data = (thread_data_t*) t->data;
    th_data->message_list = win;
    win->data = t;

    char* s = g_strdup_printf(_("Conversation with %s"), t->peer);
    elm_win_title_set(win->win, s);
    g_free(s);

    mokowin_set_title(win, t->peer);

    // connect to MessagesDb
    messagesdb_connect(_message_prepend, t->peer, t);

    // load messages!
    th_data->requested = FIRST_BLOCK_COUNT;
    th_data->query = messagesdb_foreach(_message_append, t->peer, TRUE, 0, FIRST_BLOCK_COUNT, t);
    th_data->loading = TRUE;
}
