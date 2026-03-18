
#ifndef _DSPTTK_CMD_XRAY_H_
#define _DSPTTK_CMD_XRAY_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "dspcommon.h"
#include "sal_type.h"
#include "dspttk_posix_ipc.h"

#define XRAW_ELEMENT_BYTE_SIZE           (sizeof(XRAY_ELEMENT))
#define RAW_BYTE(energy)                 ((energy == XRAY_ENERGY_SINGLE) ? 2 : 4)
#define NRAW_BYTE_SIZE(w, h, energy)     ((energy == XRAY_ENERGY_SINGLE) ? (w * h * 2) : ( w * h * 5))
#define DSPTTK_XRAW_SIZE(w, h, energy)  ((energy == XRAY_ENERGY_SINGLE) ? (w * h << 1) : (w * h << 2)) /* XRAW数据长度 */

#define DSPTTK_PBSLICE_CNT_ONSCREEN      (960)          // 条带回拉模式，屏幕上允许存在的最多条带数
#define TRANS_NODE_NUM                     (64)          //创建节点数

typedef struct
{
    XSP_SLICE_CONTENT enSliceCont;  /* 条带标记 */
    XSP_SLICE_CONTENT enSliceTagBefor;  /* 条带标记 */
    UINT32 u32NrawWidth;            /* 归一化数据宽度（未做填充或插值缩放） */
    UINT32 u32NrawHeight;           /* 归一化数据高度 */
    UINT16 *pu16XrawData;           /* 自适应前的数据用于归一化的原始数据 */
    UINT8 *pu8GrayData;             /* 灰度图 */
    UINT16 *pu16FullLinerData;      /* 一行满载的数据 */
    UINT16 *pu16ZeroLinerData;      /* 一行本底的数据 */
    float *pfNrawData;              /* 归一化处理的值 */
} XRAY_NRAW_INFO;

/* xraw保存的数据类型 */
typedef struct
{
    UINT32 u32TotalCnt;
    XRAW_DT enDataTypePre;
} XRAW_STORE_CTRL;


/* 转存数据路径 */
typedef struct
{
    DSPTTK_LIST *lstTranPack;              /* 转存队列 */
    sem_t semEnTrans;                     // 转存信号量
    UINT64 u64PackIdNode[TRANS_NODE_NUM];            /* 当前存储的包裹id节点 */
    UINT32 u32CurSaveId;                /* saveID */
    UINT64 u64FirstPackageId;           /* 第一个包裹ID */
    UINT64 u64CurPackageId;             /* 当前包裹ID */
} XRAY_PACKAGE_INFO;

/**
 * 条带回拉模式，当前屏幕上的条带范围 
 * 暂仅支持回拉帧中所有条带高度均相同的，避免出现半个条带的复杂情况，但实际使用中会遇到的 
 */
typedef struct
{
    DSPTTK_LIST *lstPbSlice; // 当前屏幕上的条带号链表，以正向回拉为基准，从头节点到尾节点条带号依次递减
    UINT64 u64SliceNo[DSPTTK_PBSLICE_CNT_ONSCREEN]; // 条带号，0为非法
    UINT32 u32StartCol;     // 起始条带处于当前屏幕上的高度（条带后半段）
    UINT32 u32EndCol;       // 结束条带处于当前屏幕上的高度（条带前半段）
    pthread_mutex_t mutexStatus;            /// 状态的互斥锁
} PLAYBACK_SLICE_RANGE;


/**
 * @fn      dspttk_get_global_trans_prm
 * @brief   获取转存全局结构体
 * 
 * @param   [IN] u32Chan XRay通道号
 * 
 * @return  [OUT] &gstTransInfo 全局结构体地址
 */
XRAY_PACKAGE_INFO *dspttk_get_global_trans_prm(UINT32 u32Chan);

/**
 * @fn      dspttk_get_gpbslice_prm
 * @brief   获取条带回拉时的全局结构体
 * 
 * @return  条带回拉时的全局结构体
 */
PLAYBACK_SLICE_RANGE *dspttk_get_gpbslice_prm(UINT32 u32Chan);

/**
 * @fn      dspttk_get_current_trans_package_id
 * @brief   获取转存最新的包裹id
 * 
 * @param   [IN] u32Chan XRay通道号
 * 
 * @return  [OUT] u64PackageId 包裹ID号
 */
UINT64 dspttk_get_current_trans_package_id(UINT32 u32Chan);

/**
 * @fn      dspttk_get_first_trans_package_id
 * @brief   获取转存第一个包裹id
 * 
 * @param   [IN] u32Chan XRay通道号
 * 
 * @return  [OUT] u64FirstPackageId 包裹ID号
 */
UINT64 *dspttk_get_first_trans_package_id(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_imaging_param
 * @brief   设置三色六色显示
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DEFORMITY_CORRECTION 畸变校正参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_imaging_param(UINT32 u32Chan);


/**
 * @fn      dspttk_xray_change_speed
 * @brief   切速度命令
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xray_change_speed(UINT32 u32Chan);

/**
 * @fn      dspttk_xray_rtpreview_start
 * @brief   开启实时预览
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xray_rtpreview_start(UINT32 u32Chan);

/**
 * @fn      dspttk_xray_rtpreview_stop
 * @brief   过包状态重置
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
UINT64 dspttk_xray_rtpreview_stop(UINT32 u32Chan);

/**
 * @fn      dspttk_xray_all_status_stop
 * @brief   关闭实时预览
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xray_all_status_stop(UINT32 u32Chan);

/**
 * @fn      dspttk_xray_playback_stop
 * @brief   关闭回拉功能
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
UINT64 dspttk_xray_playback_stop(UINT32 u32Chan);

/**
 * @fn      dspttk_xray_playback_left
 * @brief   左向条带回拉
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
UINT64 dspttk_xray_playback_left(UINT32 u32Chan);

/**
 * @fn      dspttk_xray_playback_right
 * @brief   右向条带回拉
 * 
 * @param   [IN] u32Chan 无效参数，实际不使用
 * 
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
UINT64 dspttk_xray_playback_right(UINT32 u32Chan);

/**
 * @fn      dspttk_xray_trans
 * @brief   开启转存
 *
 * @param   [IN] u32Chan 通道号，无效参数，不区分通道号
 * @param   [IN] XRAY_PARAM 隐含入参，转存参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xray_trans(UINT32 u32Chan);

/**
 * @fn      dspttk_xray_rtpreview_change_speed
 * @brief   切换实时过包速度
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 * @param   [IN] XRAY_SPEED_PARAM 隐含入参，速度参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xray_rtpreview_change_speed(UINT32 u32Chan);

/**
 * @fn      dspttk_xray_rtpreview_fillin_blank
 * @brief   设置实时预览置白参数
 *
 * @param   [IN] u32Chan 通道号，范围[0, pstShareParam->xrayChanCnt-1]
 * @param   [IN]  隐含入参，XSP_ENHANCED_SCAN_PARAM 强扫设置的参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */

UINT64 dspttk_xray_rtpreview_fillin_blank(UINT32 u32Chan);
/**
 * @fn      dspttk_rtpreview_enhanced_scan
 * @brief   设置强扫
 *
 * @param   [IN] u32Chan 通道号，无效参数，不区分通道号
 * @param   [IN]  隐含入参，XSP_ENHANCED_SCAN_PARAM 强扫设置的参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_rtpreview_enhanced_scan(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_vertical_mirror
 * @brief   设置垂直（上下）镜像
 *
 * @param   [IN] u32Chan 通道号，范围[0, pstShareParam->xrayChanCnt-1]
 * @param   [IN]  隐含入参，XSP_VERTICAL_MIRROR_PARAM 垂直镜像参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_vertical_mirror(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_color_table
 * @brief   设置颜色表（双能使用）
 *
 * @param   [IN] u32Chan 通道号，无效参数，不区分通道号
 * @param   [IN]  隐含入参，XSP_COLOR_TABLE_T 颜色表参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_color_table(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_pseudo_color
 * @brief   设置伪彩（仅单能使用）
 *
 * @param   [IN] u32Chan 通道号，无效参数，不区分通道号
 * @param   [IN]  隐含入参，XSP_PSEUDO_COLOR_T 伪彩参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_pseudo_color(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_default_style
 * @brief   设置默认风格
 *
 * @param   [IN] u32Chan 通道号，无效参数，不区分通道号
 * @param   [IN]  隐含入参，XSP_DEFAULT_STYLE 默认风格参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_default_style(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_alert_unpen
 * @brief   设置难穿透等及灵敏度等
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ALERT_TYPE 默认可疑有机物参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_alert_unpen(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_alert_unpen
 * @brief   设置可疑有机物并设置灵敏度
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ALERT_TYPE 默认可疑物参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_alert_sus(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_tip
 * @brief   设置Tip插入
 *
 * @param   [IN] u32Chan 通道号，无效参数，不区分通道号
 * @param   [IN]  隐含入参 XSP_TIP_DATA_ST Tip参数  
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_tip(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_bkg
 * @brief   设置背景阈值
 *
 * @param   [IN] u32Chan 通道号，无效参数，不区分通道号
 * @param   [IN] XRAY_PARAM 隐含入参，XSP_BKG_PARAM 背景阈值参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_bkg(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_default_enhance
 * @brief   设置默认增强系数
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ORIGINAL_MODE_PARAM 设置默认增强系数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_default_enhance(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_contrast
 * @brief   设置对比度
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_CONTRAST_PARAM 设置对比度参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_contrast(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_deformity_correction
 * @brief   设置畸变校正
 *
 * @param   [IN] u32Chan 通道号，无效参数，不区分通道号
 * @param   [IN] XRAY_PARAM 隐含入参，XSP_DEFORMITY_CORRECTION 畸变校正参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_deformity_correction(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_color_adjust
 * @brief   设置冷暖色调调节
 *
 * @param   [IN] u32Chan 通道号，无效参数，不区分通道号
 * @param   [IN] XRAY_PARAM 隐含入参，XSP_COLOR_ADJUST_PARAM 冷暖色调调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_color_adjust(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_anti
 * @brief   设置反色
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ANTI_COLOR_PARAM 反色调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_anti(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_gray_or_color
 * @brief   设置彩色或者黑白
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DISP_MODE 彩色/黑彩 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_gray_or_color(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_z789
 * @brief   设置原子序数Z789
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DISP_MODE 原子序数Z789 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_z789(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_default_mode
 * @brief   设置恢复默认
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DEFAULT_STYLE 设置显示调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_default_mode(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_le
 * @brief   设置超增
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_LE_PARAM 局部增强 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_le(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_lp_or_hp
 * @brief   设置高穿低穿
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 隐含入参，XSP_HP_PARAM XSP_LP_PARAM 高穿低穿 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_lp_or_hp(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_organic
 * @brief   设置有机物剔除
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DISP_MODE 有机物剔除 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_organic(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_inorganic
 * @brief   设置无机物剔除
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DISP_MODE 无机物 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_inorganic(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_ue
 * @brief   设置超增
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_UE_PARAM 超增 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_ue(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_absorptance_add
 * @brief   设置吸收率增加
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_VAR_ABSOR_PARAM 吸收率 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_absorptance_add(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_absorptance_sub
 * @brief   设置吸收率减少
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_VAR_ABSOR_PARAM 吸收率 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_absorptance_sub(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_ee
 * @brief   设置边增
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_EE_PARAM 边增 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_ee(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_lum_add
 * @brief   设置亮度增加
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_LUMINANCE_PARAM 亮度调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_lum_add(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_lum_sub
 * @brief   设置亮度减少
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_VAR_ABSOR_PARAM 有机物剔除 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_lum_sub(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_original_mode
 * @brief   设置原始显示
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ORIGINAL_MODE_PARAM 有机物剔除 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_original_mode(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_rm_blank_slice
 * @brief   设置空白条带移除
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_SET_RM_BLANK_SLICE 包裹后的空白条带控制结构体
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_rm_blank_slice(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_package_divide_method
 * @brief   设置包裹分割方式
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，HOST_CMD_XSP_PACKAGE_DIVIDE 包包裹分割方式结构体
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_package_divide_method(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_stretch
 * @brief   设置拉伸模式，依靠算法内部实现，
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，对应结构体XSP_STRETCH_MODE_PARAM 
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_stretch(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_bkg_color
 * @brief   设置显示图像背景色
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，结构体XSP_BKG_COLOR
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_bkg_color(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_gamma
 * @brief   设置gamma参数 此参数会影响亮度
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，结构体 XSP_GAMMA_PARAM
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_gamma(UINT32 u32Chan);

/**
 * @fn      dspttk_xsp_set_bkg_color
 * @brief   设置AIXSP和传统XSP权重
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，结构体XSP_AIXSP_RATE_PARAM
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_aixsp_rate(UINT32 u32Chan);

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_CMD_XRAY_H_ */

