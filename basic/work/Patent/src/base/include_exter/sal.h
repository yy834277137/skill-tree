/***
 * @file   sal.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  System Abstraction Layer.
 *         包含对系统抽象层的函数、系统API 封装的头文件，程序、驱动只要包含
 *         这个头文件即可直接使用大部分系统函数等。
 * @author rayin_dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef _SAL_H_
#define _SAL_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
/* 系统头文件 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <malloc.h>
#include <signal.h>
#include <semaphore.h>
#include <dlfcn.h>
#include <math.h>


/* 基本头文件 */
#include "../common/sal_type.h"
#include "../common/sal_macro.h"
#include "../common/sal_errno.h"
#include "../common/sal_bits.h"

/* 系统抽象层接口头文件 */
#include "../sal/sal_cond.h"
#include "../sal/sal_fop.h"
#include "../sal/sal_mem_new.h"
#include "../sal/sal_mem_stats.h"
#include "../sal/sal_mq.h"
#include "../sal/sal_mutex.h"
#include "../sal/sal_process.h"
#include "../sal/sal_rwlock.h"
#include "../sal/sal_sem_posix.h"
#include "../sal/sal_sem_systemV.h"
#include "../sal/sal_shm.h"
#include "../sal/sal_thr.h"
#include "../sal/sal_time.h"
#include "../sal/sal_timer.h"
#include "../sal/sal_secure.h"
#include "../sal/sal_rtld.h"

/* 音视频媒体模块 */
#include "../media/sal_video.h"
#include "../media/sal_audio.h"
#include "../media/bmp_info.h"
#include "../media/stream_bits_info_def.h"
#include "../media/resolution_appDsp.h"

/* 基本数据结构 */
#include "../dsa/dsa_list.h"
#include "../dsa/dsa_que.h"
#include "../dsa/dsa_fixed_size_que.h"
#include "../dsa/dsa_dynamic_list.h"
#include "../dsa/dsa_que.h"

/* 图像颜色空间 头文件 */
#include "../color/color_data.h"
#include "../color/color_space.h"

/* 日志 */
#include "../log/sal_trace.h"
#include "../log/log_client.h"

#endif

