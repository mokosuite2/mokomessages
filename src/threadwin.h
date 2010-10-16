#ifndef __THREADWIN_H
#define __THREADWIN_H

//#include <mokosuite/utils/settings-service.h>

void thread_win_activate(void);
void thread_win_hide(void);

#if 0
void thread_win_init(MokoSettingsService *settings);
#else
void thread_win_init(void *settings);
#endif

#endif  /* __THREADWIN_H */
