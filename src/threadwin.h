#ifndef __THREADWIN_H
#define __THREADWIN_H

#include <mokosuite/utils/remote-config-service.h>

void thread_win_activate(void);
void thread_win_hide(void);

void thread_win_init(RemoteConfigService *config);

#endif  /* __THREADWIN_H */
