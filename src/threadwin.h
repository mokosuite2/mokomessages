#ifndef __THREADWIN_H
#define __THREADWIN_H

#include <mokosuite/utils/remote-config-service.h>
#include <mokosuite/pim/messagesdb.h>

MessageThread* thread_win_get_thread(const char* peer);

void thread_win_activate(void);
void thread_win_hide(void);

void thread_win_init(RemoteConfigService *config);

extern MessageThread* new_thread;

#endif  /* __THREADWIN_H */
