
#ifndef __XTRANS_SYS_H__
#define __XTRANS_SYS_H__


#ifdef OS21
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <os21.h>

#elif defined(__KERNEL__)     /* Linux kernel */
#include <linux/version.h> /* Kernel version */
#include <linux/module.h>
#include <linux/kernel.h>  /* Kernel support */
#if (LINUX_VERSION_CODE == KERNEL_VERSION(2,4,24) )/*  */
#include <linux/config.h>
#endif
#include <linux/fs.h>      /* File operations (fops) defines */
#include <linux/types.h>
#include <linux/ioport.h>  /* Memory/device locking macros   */
#include <linux/errno.h>   /* Defines standard error codes */
#include <linux/sched.h>   /* Defines pointer (current) to current task */
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,41)
#include <linux/workqueue.h>
#else
#include <linux/tqueue.h>
#endif
#include <linux/string.h>

#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#if LINUX_VERSION_CODE != KERNEL_VERSION(2,4,24) /*  */
#include <linux/phy.h>
#endif

#ifndef ARCH_X86
#include <asm/system.h>
#endif
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/cdev.h>    /* Charactor device support */
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/serial_core.h>
#include <linux/namei.h>
#include <linux/io.h>
#else/* linux2.4 */
#include <linux/devfs_fs_kernel.h>
#endif
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/i2c.h>
#include <linux/console.h>
#include <linux/serial_reg.h>
#include <linux/tty_flip.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/file.h>
#include <linux/wait.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/mount.h>

#if LINUX_VERSION_CODE == KERNEL_VERSION(3,4,7)
#include <mach/hardware.h>
#endif

#if ((LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18))&&(LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)))
#include <asm/uaccess.h>
#include <asm/arch/hardware.h>
#include <asm/arch/cpu.h>
#include <asm/semaphore.h>
#include <asm/arch/irqs.h>
#include <asm/arch/gpio.h>
#include <asm/arch/vpss.h>
#else
#include <linux/securebits.h>
#if LINUX_VERSION_CODE != KERNEL_VERSION(2,4,24)
#include <linux/gpio.h>
#endif
#endif


#elif defined(___DSPBIOS___)  /* TI SYSBIOS */
#include <xdc/std.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/gates/GateMutexPri.h>
#include <ti/sysbios/heaps/HeapMem.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/hal/Cache.h>

#include <xdc/runtime/Types.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/IHeap.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Timestamp.h>
#elif defined(_WIN32)
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#define inline __inline

#else                /* Generic Application */
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef ST_OS21        

#include <linux/unistd.h>
#include <semaphore.h>
#include <sys/vfs.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#ifdef __ANDROID__
#include <linux/sem.h>
#include <linux/shm.h>
#include <linux/msg.h>
#else /* __ANDROID__ */
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#endif /* __ANDROID__ */

#include <linux/ioctl.h>
#include <sys/mount.h>
#include <sys/sysinfo.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <sys/mman.h>
#include <linux/errno.h>
#include <linux/kd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#endif /* ST_OS21 */

#include <sys/times.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#endif

/*lint -restore */



#endif /* __xtrans_SYS_H__ */

