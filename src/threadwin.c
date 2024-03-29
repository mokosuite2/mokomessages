
#include "globals.h"
#include <mokosuite/ui/gui.h>
#include <mokosuite/utils/misc.h>
#include <mokosuite/pim/messagesdb.h>
#include <mokosuite/utils/remote-config-service.h>
#include "msglist.h"

#include <glib/gi18n-lib.h>

/* finestra principale */
static MokoWin* win = NULL;

/* lista conversazioni */
static Evas_Object* th_list;
static Elm_Genlist_Item_Class th_itc = {0};

/* current threads query */
static void* current_query = NULL;

/* current new thread */
MessageThread* new_thread = NULL;

/* threads hash table */
static GHashTable* thread_store = NULL;

static void _delete(void* mokowin, Evas_Object* obj, void* event_info)
{
    mokowin_hide((MokoWin *)mokowin);
    elm_exit();
}

static void _list_selected(void *data, Evas_Object *obj, void *event_info)
{
    // deselect item
    elm_genlist_item_selected_set((Elm_Genlist_Item*)event_info, FALSE);

    // create/show conversation
    MessageThread* t = (MessageThread*) elm_genlist_item_data_get((Elm_Genlist_Item*)event_info);

    // open a thread
    if (t) {
        thread_data_t* th_data = (thread_data_t*) t->data;
        if (!th_data->message_list)
            msg_list_init(t);
    }

    // new thread
    else {
        EINA_LOG_DBG("composing new message");
        t = new_thread = msg_list_new(NULL);
    }

    msg_list_activate(t);
}

void thread_win_activate(void)
{
    g_return_if_fail(win != NULL);

    mokowin_activate(win);
}

void thread_win_hide(void)
{
    g_return_if_fail(win != NULL);

    mokowin_hide(win);
}

static char* _newmsg_genlist_label_get(void *data, Evas_Object * obj, const char *part)
{
    if (!strcmp(part, "elm.text"))
        return g_strdup(_("New message"));

    else if (!strcmp(part, "elm.text.sub"))
        return g_strdup(_("Compose new message"));

    return NULL;
}

static char* _th_genlist_label_get(void *data, Evas_Object * obj, const char *part)
{
    MessageThread* t = data;

    if (!strcmp(part, "elm.text"))
        return g_strdup_printf("%s (%d)", t->peer, t->total_count);

    else if (!strcmp(part, "elm.text.sub"))
        return g_strdup(t->content);

    else if (!strcmp(part, "elm.text.right"))
        return get_time_repr(t->timestamp);

    return NULL;
}

static void _list_realized(void *data, Evas_Object *obj, void *event_info)
{
    MessageThread* t = (MessageThread*) elm_genlist_item_data_get((Elm_Genlist_Item*)event_info);
    if (t && t->unread_count > 0) {
        Evas_Object* e = (Evas_Object *) elm_genlist_item_object_get((Elm_Genlist_Item*)event_info);
        edje_object_signal_emit(e, "elm,marker,enable", "elm");
    }
}

static Elm_Genlist_Item* new_message_item(Evas_Object* list)
{
    // aggiungi il primo elemento "Nuovo messaggio"
    Elm_Genlist_Item_Class *itc = g_new0(Elm_Genlist_Item_Class, 1);
    itc->item_style = "generic_sub";
    itc->func.label_get = _newmsg_genlist_label_get;

    return elm_genlist_item_append(list, itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
}

static Evas_Object* thread_list(void)
{
    Evas_Object* list = elm_genlist_add(win->win);
    elm_genlist_bounce_set(list, FALSE, FALSE);
    elm_genlist_horizontal_mode_set(list, ELM_LIST_LIMIT);
    elm_genlist_homogeneous_set(list, FALSE);

    evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_smart_callback_add(list, "selected", _list_selected, NULL);
    evas_object_smart_callback_add(list, "realized", _list_realized, NULL);

    // prepara l'itc
    th_itc.item_style = "thread";
    th_itc.func.label_get = _th_genlist_label_get;

    // first element: new message
    new_message_item(list);

    evas_object_show(list);
    return list;
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

void _thread(MessageThread* th, gpointer userdata)
{
    EINA_LOG_DBG("THREAD %p, userdata=%p", th, userdata);

    thread_data_t* data = calloc(1, sizeof(thread_data_t));
    th->data = (void*) data;

    data->list_item = elm_genlist_item_append(th_list, &th_itc, th, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
    g_hash_table_insert(thread_store, g_strdup(th->peer), th);
}

static void _message(MessageEntry* e, void* userdata)
{
    if (current_query)
        messagesdb_foreach_stop(current_query);

    // FIXME reload threads for now
    elm_genlist_clear(th_list);

    // delete store
    g_hash_table_destroy(thread_store);
    // TODO free thread function
    thread_store = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, /* TODO */NULL);

    new_message_item(th_list);
    current_query = messagesdb_foreach_thread(_thread, NULL);
}

MessageThread* thread_win_get_thread(const char* peer)
{
    return g_hash_table_lookup(thread_store, peer);
}

void thread_win_init(RemoteConfigService *config)
{
    // TODO free thread function
    thread_store = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, /* TODO */NULL);

    // overlay per gli elementi della lista dei thread
    elm_theme_overlay_add(NULL, "elm/genlist/item/thread/default");
    elm_theme_overlay_add(NULL, "elm/genlist/item_odd/thread/default");

    win = mokowin_new("mokomessages", TRUE);
    if (win == NULL) {
        EINA_LOG_ERR("[ThreadWin] Cannot create main window. Exiting");
        return;
    }

    win->delete_callback = _delete;

    elm_win_title_set(win->win, _("Messaging"));

    mokowin_create_vbox(win, FALSE);
    mokowin_set_title(win, _("Messaging"));

    mokowin_menu_enable(win);
    mokowin_menu_set(win, make_menu());

    th_list = thread_list();
    mokowin_pack_start(win, th_list, FALSE);

    // init messagesdb
    messagesdb_init(_message, _message, NULL);

    // load threads :)
    current_query = messagesdb_foreach_thread(_thread, NULL);

    // TEST
    evas_object_resize(win->win, 480, 640);
}
