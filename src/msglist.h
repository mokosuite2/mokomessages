#ifndef __MSGLIST_H
#define __MSGLIST_H

#include "globals.h"

void msg_list_init(MessageThread* t);
MessageThread* msg_list_new(const char* peer);

void msg_list_activate(MessageThread* t);
void msg_list_hide(MessageThread* t);

#endif  /* __MSGLIST_H */
