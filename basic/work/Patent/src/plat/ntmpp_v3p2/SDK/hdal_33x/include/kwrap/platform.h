#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#define _ALIGNED(x) __attribute__((aligned(x)))
#define _PACKED_BEGIN
#define _PACKED_END __attribute__ ((packed))
#define _INLINE static inline
#define _ASM_NOP __asm__("nop");
#define _SECTION(sec)

//BSP identifier (only used by linux kernel)
#if defined(__LINUX) && defined(__KERNEL__)
#if defined(CONFIG_NVT_IVOT_PLAT_NA51055)
#define _BSP_NA51055_
#elif defined(CONFIG_NVT_IVOT_PLAT_NA51068)
#define _BSP_NA51068_
#elif defined(CONFIG_NVT_IVOT_PLAT_NA51090)
#define _BSP_NA51090_
#elif defined(CONFIG_NVT_IVOT_PLAT_NA51102)
#define _BSP_NA51102_
#elif defined(CONFIG_NVT_IVOT_PLAT_NA51103)
#define _BSP_NA51103_
#else
#error Unknown platform (VOS kernel-space)
//VOS kernel-space header: code/vos/drivers/include/kwrap/platform.h
//VOS user-space header: code/vos/include/kwrap/platform.h
#endif
#endif

//todo identifier
#define _TODO           0

#endif

