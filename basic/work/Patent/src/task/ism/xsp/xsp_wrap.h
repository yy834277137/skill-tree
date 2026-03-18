/**
 * @file    xsp_wrap.h
 * @note    Hangzhou Hikvision Digital Technology Co., Ltd. All Right Reserved
 * @brief   统一各个XSP库接口
 */

#ifndef __XSP_WRAP_H__
#define __XSP_WRAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sal.h"
#include "capbility.h"
#include "platform_hal.h"
#include "ximg_proc.h"

/* ============================ 宏/枚举 ============================ */
#define XSP_PROCESS_RAW_HEIGHT_MIN	16          /* XSP库处理数据高最小值限制 */
#define XSP_CORRECT_RAW_HEIGHT_MIN	16          /* XSP库校正数据高最小值限制 */
#define XSP_VAR_ABSORP_LEVEL_MAX	64          /* 可变吸收率最大等级数，64级，0~63 */
#define XSP_PACK_HAS_SLICE_NUM_MAX  256         /* 包裹中条带个数，若支持子条带，则以子条带计 */
#define XSP_PACK_IN1_SLICE_NUM_MAX  8           /* 单个条带中最多的包裹数 */
#define XSP_HANDLE_NUM_MAX          4           /* XSP库最多申请的Handle数 */

/* 双能RAW数据偏移，返回(UINT8 *)类型，为加快运算，用移位来代替跳过低能、高能数据的乘2、乘4 */
#define XRAW_LE_OFFSET(p_xraw, w, h)	((UINT8 *)(p_xraw)) /* 低能RAW数据相对p_xraw起始偏移 */
#define XRAW_HE_OFFSET(p_xraw, w, h)	((UINT8 *)(p_xraw) + ((w) * (h) << 1)) /* 高能RAW数据相对p_xraw起始偏移 */
#define XRAW_Z_OFFSET(p_xraw, w, h)		((UINT8 *)(p_xraw) + ((w) * (h) << 2)) /* Z原子序数数据相对p_xraw起始偏移 */

/* 双能RAW数据长度，单位：字节 */
#define XRAW_LE_SIZE(w, h)	((w) * (h) << 1)            /* 低能RAW数据长度 */
#define XRAW_HE_SIZE(w, h)	((w) * (h) << 1)            /* 高能RAW数据长度 */
#define XRAW_DE_SIZE(w, h)	((w) * (h) << 2)            /* 双能数据长度（低能+高能） */
#define XRAW_Z_SIZE(w, h)	((w) * (h))                 /* Z原子序数数据长度 */
#define XRAW_ORI_Z_SIZE(w, h)	((w) * (h) << 1)       /* 未超分Z原子序数数据长度 */

#define XRAW_DEZ_SIZE(w, h)	((w) * (h) * 5)             /* 归一化双能数据长度（包括原子序数Z） */

/* 单能RAW数据长度，单位：字节 */
#define XRAW_SE_SIZE(w, h)	((w) * (h) << 1)            /* 单能RAW数据长度 */

/* 融合灰度图数据长度，单位：字节 */
#define XSP_GRAYFUSE_SIZE(w, h) ((w) * (h) << 1)        /* 融合灰度图数据长度 */

/* XSP校正类型 */
typedef enum
{
    XSP_CORR_UNDEF = 0,             /* 未定义，预留 */
    XSP_CORR_EMPTYLOAD = 1,         /* 本底校正 */
    XSP_CORR_FULLLOAD = 2,          /* 满载校正 */
    XSP_CORR_AUTOFULL = 3,          /* 自动满载 */
    XSP_CORR_AFTERGLOW = 3          /* 余辉校正，仅双能支持 */
} XSP_CORRECT_TYPE;

/*镜像模式*/
typedef enum
{
    XSP_MIRROR_NON = 0,        /* 恢复默认，无镜像 */
    XSP_MIRROR_H = 1,          /* 水平镜像，暂不支持 */
    XSP_MIRROR_V = 2,          /* 垂直镜像 */

    XSP_MIRROR_NUM             /*总数*/
} XSP_MIRROR_T;

/* 输出图旋转类型 */
typedef enum
{
    XSP_ROTATE_NON = 0,         /* 不旋转 */
    XSP_ROTATE_90 = 1,          /* 顺时针旋转90度 */
    XSP_ROTATE_180 = 2,         /* 顺时针旋转180度，不支持 */
    XSP_ROTATE_270 = 3,         /* 顺时针旋转270度 */

    XSP_ROTATE_NUM              /* 输出图旋转类型个数 */
} XSP_ROTATE_T;

/* 创建XSP Handle时指定该Handle的处理数据类型 */
typedef enum
{
    XSP_HANDLE_UNDEF    = 0,    // 未定义
    XSP_HANDLE_RT_PB    = 1,    // 实时和回拉处理类型，每个通道（xray_in_chan）建立一个
    XSP_HANDLE_TRANS    = 2,    // 转存处理类型，只需要建立一个，所有通道（xray_in_chan）共享
} XSP_HANDLE_TYPE;

/* 视角类型 */
typedef enum
{
    XSP_VANGLE_NA           = 0,    // 未定义
    XSP_VANGLE_PRIMARY      = 1,    // 主视角
    XSP_VANGLE_SECONDARY    = 2,    // 辅视角
} XSP_VISUAL_ANGLE;

/*TIP注入结果状态（该枚举应用于tip结果回调）*/
typedef enum
{
    XSP_TIPED_NONE       = 0,   /*TIP无注入*/
    XSP_TIPED_PROCESSING = 1,   /*TIP注入过程中*/
    XSP_TIPED_SUCESS     = 2,   /*TIP注入成功*/
    XSP_TIPED_FAIL       = 3,   /*TIP注入失败*/
}XSP_TIPED_STATUS;

/*默认增强模式（预设过包模式）*/
typedef enum
{
    XSP_DEFAULT_STATE_CLOSE = 0,    // 默认增强关，即原始模式
    XSP_DEFAULT_STATE_MODE0 = 1,    // 默认增强模式（预设过包模式）1
    XSP_DEFAULT_STATE_MODE1 = 2,    // 默认增强模式（预设过包模式）2
    XSP_DEFAULT_STATE_MODE2 = 3,    // 默认增强模式（预设过包模式）3
    XSP_DEFAULT_STATE_NUM
}XSP_DEFAULT_STATE;

/**
 * @fn      xsp_get_correction_data
 * @brief   获取本底、满载、余辉等校正数据
 * @param   p_handle[IN] 图像处理通道对应的句柄
 * @param   corr_type[IN] 校正数据类型，参考枚举XSP_CORRECT_TYPE
 * @param   p_corr_data[OUT] 校正数据Buffer，可为NULL，为NULL时仅输出校正数据长度，用于申请Buffer
 * @param   p_corr_size[OUT] 校正数据长度，单位：Byte
 * @return SAL_STATUS SAL_SOK-设置成功，SAL_FAIL-设置失败
 */
#define xsp_get_correction_data	g_xsp_lib_api.get_correction_data

/**
 * @fn      xsp_set_correction_data
 * @brief   设置本底、满载、余辉等校正数据
 * @param   p_handle[IN] 图像处理通道对应的句柄
 * @param   corr_type[IN] 校正数据类型，参考枚举XSP_CORRECT_TYPE
 * @param   p_corr_data[IN] 校正数据Buffer
 * @param   corr_raw_width[IN] 校正RAW数据宽
 * @param   corr_raw_height[IN] 校正RAW数据高
 * @return SAL_STATUS SAL_SOK-设置成功，SAL_FAIL-设置失败
 */
#define xsp_set_correction_data g_xsp_lib_api.set_correction_data


/* ========================= 结构体/联合体 ========================== */
typedef struct
{
    void *p_handle;     // 算法库句柄
    U32 width_max;      // 初始化该handle时传入的图像宽
    U32 height_max;     // 初始化该handle时传入的图像高
    XSP_HANDLE_TYPE handle_type;    // 处理类型
}XSP_HANDLE_ATTR;

/*TIP输入数据*/
typedef struct
{
    BOOL b_enable;              /*是否开始注入*/
    U32 position;               /*注入位置,仅研究院算法使用*/
    U32 tip_width;              /*宽度*/
    U32 tip_height;             /*高度*/

    U16 *p_tip_normalized;      /*归一化后的TIP数据缓存*/
} XSP_TIP_INJECT;

/*TIP输出数据*/
typedef struct
{
    XSP_TIPED_STATUS en_tiped_status;           /*tip注入的结果状态*/
    S32 tiped_pos;              /*tip注入的位置，自研算法输出结果，而研究院需要输入*/
    U16 *p_xraw_normalized;     /*归一化后xraw数据*/
} XSP_XRAW_TIPED;

/*难穿透与可疑有机物识别结果*/
typedef struct
{
    UINT32 num;
    XSP_RECT rect[XSP_IDENTIFY_NUM];
} XSP_IDENTIFY_S;

/*包裹检测结果*/
typedef struct
{
    U32 package_num;
    XSP_RECT package_rect[XSP_PACK_IN1_SLICE_NUM_MAX];   /*包裹区域*/
    INT32 blank_prob;
} XSP_PACKAGE_INDENTIFY;

/* 实时过包透传数据，用于匹配条带信息 */
typedef struct
{
    UINT64 u64TimeProcPipe0S;       /* 流水idx0起始时间 */
    UINT64 u64TimeProcPipe0E;       /* 流水idx0结束时间 */
    UINT64 u64TimeProcPipe1S;       /* 流水idx1起始时间 */
    UINT64 u64TimeProcPipe1E;       /* 流水idx1结束时间 */
    UINT64 u64TimeProcPipe2S;       /* 流水idx2起始时间 */
    UINT64 u64TimeProcPipe2E;       /* 流水idx2结束时间 */
    UINT32 u32SliceNo;              /* 实时过包条带号 */
    BOOL bDumpEnable;               /* dump数据是否使能 */
    CHAR chDumpDir[64];             /* dump数据路径 */
}XSP_DEBUG_ALG;

/* 实时过包输入数据 */
typedef struct
{
    XIMAGE_DATA_ST stXrawIn;        /* 未归一化的XRAW数据，平面格式 */
    XSP_TIP_INJECT st_tip_inject;   /* TIP注入参数 */
    BOOL bPseudo;                   /* 是否伪空白，TRUE-伪空白 */
    XSP_DEBUG_ALG st_debug;         /* 调试信息 */
    void *pPassthData;              /* 透传数据 */
} XSP_RTPROC_IN;

/* 实时过包输出数据 */
typedef struct
{
    SAL_STATUS enResult;                        /* 执行结果 */
    XIMAGE_DATA_ST stNrawOut;                   /* 超分的归一化数据 */
    XIMAGE_DATA_ST stOriNrawOut;                /* 未超分的归一化数据 */
    XIMAGE_DATA_ST stBlendOut;                  /* 成像算法处理的中间结果，即融合灰度图，整屏图像处理作为输入Buffer，其他情况作为输出Buffer */
    XIMAGE_DATA_ST stDispOut;                   /* 显示数据 */
    XIMAGE_DATA_ST stAiYuvOut;                  /* AI YUV数据 */
    XSP_XRAW_TIPED st_xraw_tiped;               /* 带有tip图像的归一化xraw数据输出 */
    XSP_PACKAGE_INDENTIFY st_package_indentify; /* 包裹检测结果 */
    XSP_DEBUG_ALG st_debug;                     /* 调试信息 */
    void *pPassthData;                          /* 透传数据 */
} XSP_RTPROC_OUT;

typedef struct
{
    XIMAGE_DATA_ST stNrawIn;                     /* 归一化数据 */
    XIMAGE_DATA_ST stBlendIn;                    /* 成像算法处理的中间结果，即融合灰度图，整屏图像处理作为输入Buffer，其他情况作为输出Buffer */
    XSP_ZDATA_VERSION zdata_ver;                /* 成像算法原子序数版本，目前仅支持V1，V2 */
    U32 neighbour_top;                          /* 上邻域高度 */
    U32 neighbour_bottom;                       /* 下邻域高度 */
    BOOL bUseBlend;                             /* 输入的融合灰度图是否有效，若为FALSE，则p_gray_fuse作为输出Buffer */
} XSP_PBPROC_IN;

typedef XIMAGE_DATA_ST XSP_PBPROC_OUT;

typedef struct
{
    XIMAGE_DATA_ST stNrawIn;                     /* 归一化数据 */
    XIMAGE_DATA_ST stBlendIn;                    /* 成像算法处理的中间结果，即融合灰度图，整屏图像处理作为输入Buffer，其他情况作为输出Buffer */
    XSP_ZDATA_VERSION zdata_ver;                /* 成像算法原子序数版本，目前仅支持V1，V2 */
} XSP_TRANS_IN;

typedef XIMAGE_DATA_ST XSP_TRANS_OUT;

typedef XIMAGE_DATA_ST XSP_IDT_IN;

/*TIP数据*/
typedef struct
{
    U32 enable;             /*使能开关，1表示开始，0表示结束*/
    U32 col;                /*开始注入行号*/
    U32 processing;         /*1表示注入过程中，0表示没有注入*/
    S32 slice_count;        /*注入的条带数据*/
    U32 last_tip_end;       /*上一次*/
} WRAP_TIP_PROCESS;

/*临时变量*/
typedef struct
{
    U32 b_Enable;                         /* 是否使能*/
    WRAP_TIP_PROCESS wrap_tip_process;
} WRAP_CHN_PRM;

typedef struct tagDualCommon
{
    WRAP_CHN_PRM wrap_chn[XSP_HANDLE_NUM_MAX];          /**<  XSP通道参数*/
} WRAP_COMMON;

/*应用下发设备属性参数*/
typedef struct
{
    ISM_DEVTYPE_E ism_dev_type;             /* X-RAY安检机机型大类 */
    UINT32 xray_width_max;                  /* X-RAY源输入数据宽最大值，即探测板像素数×探测板数量 */
    UINT32 xray_height_max;                 /* X-RAY源输入数据高最大值，即全屏显示横向对应的RAW数据长度 */
    UINT32 xray_slice_height_max;           /* X-RAY源输入条带高最大值 */
    FLOAT32 xray_slice_resize;              /* X-RAY源输入条带的超分系数，算法内部X/Y方向都会用这个值 */
    XRAY_DET_VENDOR xray_det_vendor;        /* X-RAY探测板供应商 */
    XRAY_ENERGY_NUM xray_energy_num;        /* X-RAY光机能量类型数 */
    UINT32 xray_in_chan_cnt;                /* X-RAY源数量*/
    XSP_VISUAL_ANGLE visual_angle;          /* X-RAY视角类型，主视角或者俯视角*/
    XRAY_SOURCE_TYPE_E enXraySource;        /* X-RAY射线源类型 */
    XSP_LIB_SRC enXspLibSrc;                /* XSP算法库类型*/
    UINT32 passth_data_size;                /* 流水线处理透传数据长度，单位：字节 */
    void (*rtproc_pipeline_cb)(void *p_handle, XSP_RTPROC_OUT *pstRtProcOut); /* 流水线处理回调函数输出 */
} XRAY_DEV_ATTR;

typedef struct
{
    void *(*create_handle)(XSP_HANDLE_TYPE handle_type, XRAY_DEV_ATTR *p_dev_attr);
    SAL_STATUS (*reload_zTable)(void *p_handle, XRAY_DEV_ATTR *p_dev_attr);
    SAL_STATUS (*set_curve)(void *p_handle, S32 s32KeyIdx, CHAR *pPath);
    CHAR *(*get_version)(void *p_handle); /* p_handle传入NULL即可，create_handle时已经初始化了 */
    SAL_STATUS (*set_akey)(void *p_handle, S32 s32OptNum, S32 key, S32 value1, S32 value2);
    SAL_STATUS (*get_akey)(void *p_handle, S32 key, S32 *pImageVal, S32 *pVal1, S32 *pVal2);
    SAL_STATUS (*get_all_key)(void *p_handle);
    SAL_STATUS (*set_blank_region)(void *p_handle, U32 top_margin, U32 bottom_margin);
    SAL_STATUS (*get_correction_data)(void *p_handle, XSP_CORRECT_TYPE corr_type, U16 *p_corr_data, U32 *p_corr_size);
    SAL_STATUS (*set_correction_data)(void *p_handle, XSP_CORRECT_TYPE corr_type, U16 *p_corr_data, U32 corr_raw_width, U32 corr_raw_height);
    SAL_STATUS (*set_disp_color)(void *p_handle, U32 type); /*显示*/
    SAL_STATUS (*set_rotate)(void *p_handle, U32 type, XRAY_PROC_TYPE enType); /*旋转*/
    SAL_STATUS (*get_rotate)(void *p_handle, U32 *p_type); /*旋转*/
    SAL_STATUS (*set_mirror)(void *p_handle, U32 type); /*镜像*/
    SAL_STATUS (*get_mirror)(void *p_handle, U32 *p_type); /*镜像*/
    SAL_STATUS (*set_pseudo_color)(void *p_handle, U32 type); /*伪彩*/
    SAL_STATUS (*set_luminance)(void *p_handle, S32 type); /*亮度*/

    SAL_STATUS (*set_default_enhance)(void *p_handle, XSP_DEFAULT_STATE defStyle, S32 value); /*默认增强模式*/
    SAL_STATUS (*set_anti)(void *p_handle, U32 enable); /*反色*/
    SAL_STATUS (*get_anti)(void *p_handle, S32 *s32GetAnti); /*获取反色*/
    SAL_STATUS (*set_lp)(void *p_handle, U32 enable); /*低穿*/
    SAL_STATUS (*set_hp)(void *p_handle, U32 enable); /*高穿*/
    SAL_STATUS (*set_le)(void *p_handle, U32 enable); /*局增*/
    SAL_STATUS (*set_ue)(void *p_handle, U32 enable, U32 value); /*超增*/
    SAL_STATUS (*set_ee)(void *p_handle, BOOL enable, U32 value); /*边增*/
    SAL_STATUS (*set_absor)(void *p_handle, U32 enable, U32 value); /*可变吸收率*/
    SAL_STATUS (*set_unpen_alert)(void *p_handle, U32 enable, U32 sensitivity, U32 color); /*难穿透报警*/
    SAL_STATUS (*set_sus_alert)(void *p_handle, U32 enable, U32 sensitivity, U32 color); /*可疑物报警*/
    SAL_STATUS (*set_bg)(void *p_handle, U32 thr, U32 fullupdate_ratio); /*背景*/
    SAL_STATUS (*set_color_table)(void *p_handle, U32 type); /*成像颜色配置表*/
    SAL_STATUS (*set_colors_imaging)(void *p_handle, U32 type); /*三色六色显示*/
    SAL_STATUS (*set_deformity_correction)(void *p_handle, U32 enable); /*畸变矫正*/
    SAL_STATUS (*set_dose_correct)(void *p_handle, U32 value); /*剂量波动修正*/
    SAL_STATUS (*set_color_adjust)(void *p_handle, U32 value); /*颜色调节*/
    SAL_STATUS (*set_belt_speed)(void *p_handle, FLOAT32 f32BeltSpeed, UINT32 line_per_slice,XRAY_PROC_SPEED enSpeed);  // 过包速度
    SAL_STATUS (*set_noise_reduction)(void *p_handle, U32 level); /*降噪等级*/
    SAL_STATUS (*set_contrast)(void *p_handle, U32 level); /*对比度*/
    SAL_STATUS (*set_stretch_ratio)(void *p_handle,U32 stretch_ratio); /*设置图像拉伸度*/
    SAL_STATUS (*set_le_th_range)(void *p_handle, U32 lower_limit, U32 upper_limit); /*局增灰度阈值范围*/
    SAL_STATUS (*set_dt_gray_range)(void *p_handle, U32 lower_limit, U32 upper_limit); /*双能分辨灰度范围*/
    SAL_STATUS (*set_bkg_color)(void *p_handle, U32 u32BkgColor);   /* 设置成像背景色 */
    SAL_STATUS (*get_zdata_version)(void *p_handle, U32 *type);/*获取原子序数版本支持V1，V2*/
    SAL_STATUS (*set_coldhot_threshold)(void *p_handle, U32 level);  /* 设置冷热源参数 */
    SAL_STATUS (*set_gamma)(void *p_handle, U32 mode_switch, U32 value); /*gamma参数设置*/
    SAL_STATUS (*set_aixsp_rate)(void *p_handle, U32 value); /*aixsp权重参数设置*/

    SAL_STATUS (*rtpreview_process)(void *p_handle, XSP_RTPROC_IN st_xraw_realtime_original, XSP_RTPROC_OUT *pst_xraw_realtime_normalized);
    SAL_STATUS (*playback_process)(void *p_handle, XSP_PBPROC_IN *pst_pbproc_in, XSP_PBPROC_OUT *pstPbProcOut);
    SAL_STATUS (*transform_process)(void *p_handle, XSP_TRANS_IN *pst_trans_in, XSP_TRANS_OUT *pstTransOut);
    SAL_STATUS (*identify_process)(void *p_handle, XSP_IDT_IN *pstIdtIn, XSP_IDENTIFY_S **pstXspIdtResult);

} XSP_LIB_API;


/* =================== 函数申明，static && extern =================== */
SAL_STATUS hka_xsp_lib_reg(const XSP_LIB_SRC lib_src);


/* =================== 全局变量，static && extern =================== */
extern XSP_LIB_API g_xsp_lib_api;


#ifdef __cplusplus
}
#endif

#endif

