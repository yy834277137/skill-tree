/******************************************************************************
* 
* 版权信息：版权所有 (c) 201X, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：hik_apt_math.h
* 文件标示： __HIK_APT_MATH_H__
* 摘    要： 跨平台数学库
* 
* 当前版本：1.0
* 作    者：秦川6
* 日    期：2020-08-12
* 备    注：
******************************************************************************/


/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 * $FreeBSD: src/lib/msun/src/openlibm.h,v 1.82 2011/11/12 19:55:48 theraven Exp $
 */


#ifndef __HIK_APT_MATH_H__
#define	__HIK_APT_MATH_H__

#if (defined(_WIN32) || defined (_MSC_VER)) && !defined(__WIN32__)
    #define __WIN32__
#endif

#if !defined(__arm__) && !defined(__wasm__)
#define OLM_LONG_DOUBLE
#endif

#ifndef __pure2
#define __pure2
#endif

#ifdef _WIN32
# ifdef IMPORT_EXPORTS
#  define OLM_DLLEXPORT __declspec(dllimport)
# else
#  define OLM_DLLEXPORT __declspec(dllexport)
# endif
#else
#define OLM_DLLEXPORT __attribute__ ((visibility("default")))
#endif




/*
 * Most of these functions depend on the rounding mode and have the side
 * effect of raising floating-point exceptions, so they are not declared
 * as __pure2.  In C99, FENV_ACCESS affects the purity of these functions.
 */

#if defined(__cplusplus)
extern "C" {
#endif
/* Symbol present when OpenLibm is used. */
int hik_apt_isopenlibm(void);

/*
 * ANSI/POSIX
 */
OLM_DLLEXPORT int	__hik_apt_fpclassifyd(double) __pure2;
OLM_DLLEXPORT int	__hik_apt_fpclassifyf(float) __pure2;
OLM_DLLEXPORT int	__hik_apt_fpclassifyl(long double) __pure2;
OLM_DLLEXPORT int	__hik_apt_isfinitef(float) __pure2;
OLM_DLLEXPORT int	__hik_apt_isfinite(double) __pure2;
OLM_DLLEXPORT int	__hik_apt_isfinitel(long double) __pure2;
OLM_DLLEXPORT int	__hik_apt_isinff(float) __pure2;
OLM_DLLEXPORT int	__hik_apt_isinfl(long double) __pure2;
OLM_DLLEXPORT int	__hik_apt_isnanf(float) __pure2;
OLM_DLLEXPORT int	__hik_apt_isnanl(long double) __pure2;
OLM_DLLEXPORT int	__hik_apt_isnormalf(float) __pure2;
OLM_DLLEXPORT int	__hik_apt_isnormal(double) __pure2;
OLM_DLLEXPORT int	__hik_apt_isnormall(long double) __pure2;
OLM_DLLEXPORT int	__hik_apt_signbit(double) __pure2;
OLM_DLLEXPORT int	__hik_apt_signbitf(float) __pure2;
OLM_DLLEXPORT int	__hik_apt_signbitl(long double) __pure2;

OLM_DLLEXPORT double	hik_apt_acos(double);
OLM_DLLEXPORT double	hik_apt_asin(double);
OLM_DLLEXPORT double	hik_apt_atan(double);
OLM_DLLEXPORT double	hik_apt_atan2(double, double);
OLM_DLLEXPORT double	hik_apt_cos(double);
OLM_DLLEXPORT double	hik_apt_sin(double);
OLM_DLLEXPORT double	hik_apt_tan(double);

OLM_DLLEXPORT double	hik_apt_cosh(double);
OLM_DLLEXPORT double	hik_apt_sinh(double);
OLM_DLLEXPORT double	hik_apt_tanh(double);

OLM_DLLEXPORT double	hik_apt_exp(double);
OLM_DLLEXPORT double	hik_apt_frexp(double, int *);	/* fundamentally !__pure2 */
OLM_DLLEXPORT double	hik_apt_ldexp(double, int);
OLM_DLLEXPORT double	hik_apt_log(double);
OLM_DLLEXPORT double	hik_apt_log10(double);
OLM_DLLEXPORT double	hik_apt_modf(double, double *);	/* fundamentally !__pure2 */

OLM_DLLEXPORT double	hik_apt_pow(double, double);
OLM_DLLEXPORT double	hik_apt_sqrt(double);

OLM_DLLEXPORT double	hik_apt_ceil(double);
OLM_DLLEXPORT double	hik_apt_fabs(double) __pure2;
OLM_DLLEXPORT double	hik_apt_floor(double);
OLM_DLLEXPORT double	hik_apt_fmod(double, double);

/*
 * These functions are not in C90.
 */
OLM_DLLEXPORT double	hik_apt_acosh(double);
OLM_DLLEXPORT double	hik_apt_asinh(double);
OLM_DLLEXPORT double	hik_apt_atanh(double);
OLM_DLLEXPORT double	hik_apt_cbrt(double);
OLM_DLLEXPORT double	hik_apt_erf(double);
OLM_DLLEXPORT double	hik_apt_erfc(double);
OLM_DLLEXPORT double	hik_apt_exp2(double);
OLM_DLLEXPORT double	hik_apt_expm1(double);
OLM_DLLEXPORT double	hik_apt_fma(double, double, double);
OLM_DLLEXPORT double	hik_apt_hypot(double, double);
OLM_DLLEXPORT int	hik_apt_ilogb(double) __pure2;
OLM_DLLEXPORT int	(hik_apt_isinf)(double) __pure2;
OLM_DLLEXPORT int	(hik_apt_isnan)(double) __pure2;
OLM_DLLEXPORT double	hik_apt_lgamma(double);
OLM_DLLEXPORT long long hik_apt_llrint(double);
OLM_DLLEXPORT long long hik_apt_llround(double);
OLM_DLLEXPORT double	hik_apt_log1p(double);
OLM_DLLEXPORT double	hik_apt_log2(double);
OLM_DLLEXPORT double	hik_apt_logb(double);
OLM_DLLEXPORT long	hik_apt_lrint(double);
OLM_DLLEXPORT long	hik_apt_lround(double);
OLM_DLLEXPORT double	hik_apt_nan(const char *) __pure2;
OLM_DLLEXPORT double	hik_apt_nextafter(double, double);
OLM_DLLEXPORT double	hik_apt_remainder(double, double);
OLM_DLLEXPORT double	hik_apt_remquo(double, double, int *);
OLM_DLLEXPORT double	hik_apt_rint(double);




OLM_DLLEXPORT double	hik_apt_copysign(double, double) __pure2;
OLM_DLLEXPORT double	hik_apt_fdim(double, double);
OLM_DLLEXPORT double	hik_apt_fmax(double, double) __pure2;
OLM_DLLEXPORT double	hik_apt_fmin(double, double) __pure2;
OLM_DLLEXPORT double	hik_apt_nearbyint(double);
OLM_DLLEXPORT double	hik_apt_round(double);
OLM_DLLEXPORT double	hik_apt_scalbln(double, long);
OLM_DLLEXPORT double	hik_apt_scalbn(double, int);
OLM_DLLEXPORT double	hik_apt_tgamma(double);
OLM_DLLEXPORT double	hik_apt_trunc(double);

/*
 * BSD math library entry points
 */
OLM_DLLEXPORT int	hik_apt_isnanf(float) __pure2;

/*
 * Reentrant version of hik_apt_lgamma; passes signgam back by reference as the
 * second argument; user must allocate space for signgam.
 */
OLM_DLLEXPORT double	hik_apt_lgamma_r(double, int *);

/*
 * Single sine/cosine function.
 */
OLM_DLLEXPORT void	hik_apt_sincos(double, double *, double *);


/* float versions of ANSI/POSIX functions */
OLM_DLLEXPORT float	hik_apt_acosf(float);
OLM_DLLEXPORT float	hik_apt_asinf(float);
OLM_DLLEXPORT float	hik_apt_atanf(float);
OLM_DLLEXPORT float	hik_apt_atan2f(float, float);
OLM_DLLEXPORT float	hik_apt_cosf(float);
OLM_DLLEXPORT float	hik_apt_sinf(float);
OLM_DLLEXPORT float	hik_apt_tanf(float);

OLM_DLLEXPORT float	hik_apt_coshf(float);
OLM_DLLEXPORT float	hik_apt_sinhf(float);
OLM_DLLEXPORT float	hik_apt_tanhf(float);

OLM_DLLEXPORT float	hik_apt_exp2f(float);
OLM_DLLEXPORT float	hik_apt_expf(float);
OLM_DLLEXPORT float	hik_apt_expm1f(float);
OLM_DLLEXPORT float	hik_apt_frexpf(float, int *);	/* fundamentally !__pure2 */
OLM_DLLEXPORT int	hik_apt_ilogbf(float) __pure2;
OLM_DLLEXPORT float	hik_apt_ldexpf(float, int);
OLM_DLLEXPORT float	hik_apt_log10f(float);
OLM_DLLEXPORT float	hik_apt_log1pf(float);
OLM_DLLEXPORT float	hik_apt_log2f(float);
OLM_DLLEXPORT float	hik_apt_logf(float);
OLM_DLLEXPORT float	hik_apt_modff(float, float *);	/* fundamentally !__pure2 */

OLM_DLLEXPORT float	hik_apt_powf(float, float);
OLM_DLLEXPORT float	hik_apt_sqrtf(float);

OLM_DLLEXPORT float	hik_apt_ceilf(float);
OLM_DLLEXPORT float	hik_apt_fabsf(float) __pure2;
OLM_DLLEXPORT float	hik_apt_floorf(float);
OLM_DLLEXPORT float	hik_apt_fmodf(float, float);
OLM_DLLEXPORT float	hik_apt_roundf(float);

OLM_DLLEXPORT float	hik_apt_erff(float);
OLM_DLLEXPORT float	hik_apt_erfcf(float);
OLM_DLLEXPORT float	hik_apt_hypotf(float, float);
OLM_DLLEXPORT float	hik_apt_lgammaf(float);
OLM_DLLEXPORT float	hik_apt_tgammaf(float);

OLM_DLLEXPORT float	hik_apt_acoshf(float);
OLM_DLLEXPORT float	hik_apt_asinhf(float);
OLM_DLLEXPORT float	hik_apt_atanhf(float);
OLM_DLLEXPORT float	hik_apt_cbrtf(float);
OLM_DLLEXPORT float	hik_apt_logbf(float);
OLM_DLLEXPORT float	hik_apt_copysignf(float, float) __pure2;
OLM_DLLEXPORT long long hik_apt_llrintf(float);
OLM_DLLEXPORT long long hik_apt_llroundf(float);
OLM_DLLEXPORT long	hik_apt_lrintf(float);
OLM_DLLEXPORT long	hik_apt_lroundf(float);
OLM_DLLEXPORT float	hik_apt_nanf(const char *) __pure2;
OLM_DLLEXPORT float	hik_apt_nearbyintf(float);
OLM_DLLEXPORT float	hik_apt_nextafterf(float, float);
OLM_DLLEXPORT float	hik_apt_remainderf(float, float);
OLM_DLLEXPORT float	hik_apt_remquof(float, float, int *);
OLM_DLLEXPORT float	hik_apt_rintf(float);
OLM_DLLEXPORT float	hik_apt_scalblnf(float, long);
OLM_DLLEXPORT float	hik_apt_scalbnf(float, int);
OLM_DLLEXPORT float	hik_apt_truncf(float);

OLM_DLLEXPORT float	hik_apt_fdimf(float, float);
OLM_DLLEXPORT float	hik_apt_fmaf(float, float, float);
OLM_DLLEXPORT float	hik_apt_fmaxf(float, float) __pure2;
OLM_DLLEXPORT float	hik_apt_fminf(float, float) __pure2;

/*
 * Float versions of reentrant version of hik_apt_lgamma; passes signgam back by
 * reference as the second argument; user must allocate space for signgam.
 */
OLM_DLLEXPORT float	hik_apt_lgammaf_r(float, int *);

/*
 * Single sine/cosine function.
 */
OLM_DLLEXPORT void	hik_apt_sincosf(float, float *, float *);


/*
 * long double versions of ISO/POSIX math functions
 */
OLM_DLLEXPORT long double	hik_apt_acoshl(long double);
OLM_DLLEXPORT long double	hik_apt_acosl(long double);
OLM_DLLEXPORT long double	hik_apt_asinhl(long double);
OLM_DLLEXPORT long double	hik_apt_asinl(long double);
OLM_DLLEXPORT long double	hik_apt_atan2l(long double, long double);
OLM_DLLEXPORT long double	hik_apt_atanhl(long double);
OLM_DLLEXPORT long double	hik_apt_atanl(long double);
OLM_DLLEXPORT long double	hik_apt_cbrtl(long double);
OLM_DLLEXPORT long double	hik_apt_ceill(long double);
OLM_DLLEXPORT long double	hik_apt_copysignl(long double, long double) __pure2;
OLM_DLLEXPORT long double	hik_apt_coshl(long double);
OLM_DLLEXPORT long double	hik_apt_cosl(long double);
OLM_DLLEXPORT long double	hik_apt_erfcl(long double);
OLM_DLLEXPORT long double	hik_apt_erfl(long double);
OLM_DLLEXPORT long double	hik_apt_exp2l(long double);
OLM_DLLEXPORT long double	hik_apt_expl(long double);
OLM_DLLEXPORT long double	hik_apt_expm1l(long double);
OLM_DLLEXPORT long double	hik_apt_fabsl(long double) __pure2;
OLM_DLLEXPORT long double	hik_apt_fdiml(long double, long double);
OLM_DLLEXPORT long double	hik_apt_floorl(long double);
OLM_DLLEXPORT long double	hik_apt_fmal(long double, long double, long double);
OLM_DLLEXPORT long double	hik_apt_fmaxl(long double, long double) __pure2;
OLM_DLLEXPORT long double	hik_apt_fminl(long double, long double) __pure2;
OLM_DLLEXPORT long double	hik_apt_fmodl(long double, long double);
OLM_DLLEXPORT long double	hik_apt_frexpl(long double value, int *); /* fundamentally !__pure2 */
OLM_DLLEXPORT long double	hik_apt_hypotl(long double, long double);
OLM_DLLEXPORT int		hik_apt_ilogbl(long double) __pure2;
OLM_DLLEXPORT long double	hik_apt_ldexpl(long double, int);
OLM_DLLEXPORT long double	hik_apt_lgammal(long double);
OLM_DLLEXPORT long long	hik_apt_llrintl(long double);
OLM_DLLEXPORT long long	hik_apt_llroundl(long double);
OLM_DLLEXPORT long double	hik_apt_log10l(long double);
OLM_DLLEXPORT long double	hik_apt_log1pl(long double);
OLM_DLLEXPORT long double	hik_apt_log2l(long double);
OLM_DLLEXPORT long double	hik_apt_logbl(long double);
OLM_DLLEXPORT long double	hik_apt_logl(long double);
OLM_DLLEXPORT long		hik_apt_lrintl(long double);
OLM_DLLEXPORT long		hik_apt_lroundl(long double);
OLM_DLLEXPORT long double	hik_apt_modfl(long double, long double *); /* fundamentally !__pure2 */
OLM_DLLEXPORT long double	hik_apt_nanl(const char *) __pure2;
OLM_DLLEXPORT long double	hik_apt_nearbyintl(long double);
OLM_DLLEXPORT long double	hik_apt_nextafterl(long double, long double);
OLM_DLLEXPORT double		hik_apt_nexttoward(double, long double);
OLM_DLLEXPORT float		hik_apt_nexttowardf(float, long double);
OLM_DLLEXPORT long double	hik_apt_nexttowardl(long double, long double);
OLM_DLLEXPORT long double	hik_apt_powl(long double, long double);
OLM_DLLEXPORT long double	hik_apt_remainderl(long double, long double);
OLM_DLLEXPORT long double	hik_apt_remquol(long double, long double, int *);
OLM_DLLEXPORT long double	hik_apt_rintl(long double);
OLM_DLLEXPORT long double	hik_apt_roundl(long double);
OLM_DLLEXPORT long double	hik_apt_scalblnl(long double, long);
OLM_DLLEXPORT long double	hik_apt_scalbnl(long double, int);
OLM_DLLEXPORT long double	hik_apt_sinhl(long double);
OLM_DLLEXPORT long double	hik_apt_sinl(long double);
OLM_DLLEXPORT long double	hik_apt_sqrtl(long double);
OLM_DLLEXPORT long double	hik_apt_tanhl(long double);
OLM_DLLEXPORT long double	hik_apt_tanl(long double);
OLM_DLLEXPORT long double	hik_apt_tgammal(long double);
OLM_DLLEXPORT long double	hik_apt_truncl(long double);

/* Reentrant version of hik_apt_lgammal. */
OLM_DLLEXPORT long double	hik_apt_lgammal_r(long double, int *);

/*
 * Single sine/cosine function.
 */
OLM_DLLEXPORT void	hik_apt_sincosl(long double, long double *, long double *);
#if defined(__cplusplus)
}
#endif
#endif /* !__HIK_APT_MATH_H__ */
