
#ifndef __EDID_H_
#define __EDID_H_

#include<platform_hal.h>

/* 信号类型 */
typedef enum
{
    EDID_SIGNAL_DIGTAL,     /* 数字信号 */
    EDID_SIGNAL_ANALOG,     /* 模拟信号 */
    EDID_SIGNAL_BUTT,
} EDID_SIGNAL_TYPE_E;

/* detailed timing块描述的数据类型 */
typedef enum
{
    EDID_DETAILED_TIMING,
    EDID_DETAILED_DESC,
    EDID_DETAILED_BUTT,
} EDID_DETAILED_TYPE_E;

/* 对监视器的描述类型 */
typedef enum
{
    EDID_DESC_SN,
    EDID_DESC_STRING,
    EDID_DESC_LIMIT,
    EDID_DESC_NAME,         /* 当前仅支持配置监视器的名字 */
    EDID_DESC_COLOR,
    EDID_DESC_STANDARD,
    EDID_DESC_BUTT,
} EDID_DESC_TYPE_E;

typedef struct
{
    EDID_DESC_TYPE_E enDes;     /* 对应描述信息的类型 */
    union
    {
        char szName[16];
    };
} EDID_DESC_INFO_S;

/* detailed timing块描述的数据 */
typedef struct
{
    EDID_DETAILED_TYPE_E enType;    /* detailed timing的18个字节存放的数据类型 */
    union
    {
        BT_TIMING_E enTiming;       /* 对应时序 */
        EDID_DESC_INFO_S stDescInfo;/* 描述信息 */
    };
} EDID_DETAILED_INFO_S;

/* 详细描述类型与标志的映射 */
typedef struct
{
    EDID_DESC_TYPE_E enType;
    UINT8 u8Flag;
} EDID_DESC_FALG_MAP_S;

/* 获取显示EDID的类型 */
typedef enum
{
    EDID_DISP_DEV_TYPE_HISI_HDMI,       /* 通过HISI接口获取 */
    EDID_DISP_DEV_TYPE_IIC,             /* 通过IIC获取 */
    EDID_DISP_DEV_TYPE_BUTT,
} EDID_DISP_DEV_TYPE_E;

typedef enum tagHDMI_ID_E {
    SOC_HDMI_ID_0         = 0,               /* HDMI 0 */
    SOC_HDMI_ID_BUTT
}SOC_HDMI_ID_E;

/* 获取EDID的外设 */
typedef struct
{
    EDID_DISP_DEV_TYPE_E enDispType;
    union
    {
        SOC_HDMI_ID_E enHiHdmi;
        IIC_DEV_S stI2cDev;
    };
} EDID_DISP_DEV_S;



/* 设置芯片内的EDID */
typedef INT32 (*EDID_SetChipEdid)(UINT32 u32Chn, VIDEO_EDID_BUFF_S *pstEdidBuff, CAPT_CABLE_E enCable);

INT32 EDID_RegisterSetFunc(UINT32 u32Chn, EDID_SetChipEdid pfuncSetChipEdid);
VIDEO_EDID_BUFF_S* EDID_GetEdid(UINT32 u32Chn, CAPT_CABLE_E enCable);
INT32 EDID_SetEdid(UINT32 u32Chn, const UINT8 *pu8Buff, UINT32 u32Len, EDID_SIGNAL_TYPE_E *penSignalType);
INT32 EDID_DispLoopBack(UINT32 u32Chn, UINT32 u32DispChn, UINT32 u32ViNum, EDID_SIGNAL_TYPE_E enSignal);
INT32 EDID_GetEdidInfo(UINT32 u32Chn, CAPT_EDID_INFO_S *pstEdidInfo);
INT32 EDID_ReLoadEdid(UINT32 u32Chn, CAPT_CABLE_E enCable);

#endif


