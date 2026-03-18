#ifndef HIK_APT_TIME_H
#define HIK_APT_TIME_H
#ifdef __cplusplus
extern "C" {
#endif
#include <time.h>

clock_t hik_apt_clock_thread(void);

clock_t hik_apt_clock_process(void);

clock_t hik_apt_clock_real(void);

#ifdef __cplusplus
}
#endif

#endif // HIK_APT_TIME_H
