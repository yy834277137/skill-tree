/**
 * @file   osd_drv_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  osd 模块 drv 层
 * @author heshengyuan
 * @date   2014年10月28日 Create
 * @note
 * @note \n History
   1.Date        : 2014年10月28日 Create
     Author      : heshengyuan
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#ifndef _OSD_DRV_API_H_
#define _OSD_DRV_API_H_

#include "dspcommon.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#define OSD_MAX_CHAN		    (6)           /* 最大编码设备数 */

#define OSD_IDX_NUM_MAX      (100)            /* 最大存放数字 */
#define OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT_IDX_SUBNUM_MAX   (16)             /* OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT最大存放辅数字 */
#define OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX   (64)             /* OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT最大存放辅数字 */

#define OSD_BLOCK_IDX_STRING    (0)                     /* 用于存放违禁品名称 */
#define OSD_BLOCK_IDX_ASCII     (1)                     /* 用于存放ASCII码 */
#define OSD_BLOCK_IDX_NUM_PAREN (2)                     /* 用于存放数字OSD，带括号 */
#define OSD_BLOCK_IDX_NUM_MUL   (3)                     /* 用于存放数字OSD,带乘号“x” */
#define OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT   (4)        /* 用于存放数字OSD,带括号带点带辅数字“(%u.%u)” */
#define OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL   (5)     /* 用于存放数字OSD,带括号带点带辅数字“(主x%u,侧x%u)” */


#define OSD_BLOCK_NUM_MAX       (6)                     /* 允许创建的OSD BLOCK数量 */
#define OSD_STRING_NUM_MAX      (OSD_IDX_NUM_MAX * OSD_IDX_NUM_MAX)    /* OSD字符串最大个数，单个类型最多种类，目前组合（X*X）起来最大 */
#define OSD_STRING_LEN_MAX      (20)                    /* 一行OSD的最大字符数 */
#define OSD_IDX_INVALID         (-1)                    /* 无效索引值 */
#define OSD_COLOR_INVALID       (-1)                    /* 无效的颜色 */

#define OSD_VAR_BLOCK_NUM_MAX   (20)

#define OSD_LAT_SIZE_MAX        (100)                   /* 一个block支持点阵大小的最大数 */
#define OSD_FONT_SIZE_MAX       (100)                   /* 一个block支持字体大小的最大数 */

#define OSD_LAT_PER_BLOCK       (3)                     /* 一个block支持点阵的最大数 */
#define OSD_FONT_PER_LAT        (3)                     /* 一个点阵支持字体的最大数 */

#define OSD_CHAR_TYPE_ASC       0
#define OSD_CHAR_TYPE_FONT8     1 /*ASCII字库外的16*8字点阵*/
#define OSD_CHAR_TYPE_FONT16    2
#define OSD_CHAR_TYPE_TAG       3

#define OSD_CHAR_W_ASC          8
#define OSD_CHAR_W_HZ           16
#define OSD_CHAR_ASC_PRT_START  32          /* 可打印ASCII码起始值，包含其本身 */
#define OSD_CHAR_ASC_PRT_END    126         /* 可打印ASCII码结束值，包含其本身 */

#define GET_HZ_OFFSET(x)        ((94 * ((x >> 8) - 0xa0 - 1) + ((x & 0xff) - 0xa0 - 1)) * 32)
/* #define GET_HZ_OFFSET(x) (((x&0xff)>=0x9f?(94*(((x>>8)-0x80)*2-1)+(x&0xff)-0x9f):((x&0xff)<=0x7e?(94*(((x>>8)-0x80)*2-2)+(x&0xff)-0x40):(94*(((x>>8)-0x80)*2-2)+(x&0xff)-0x40-1)))*32) */
#define GET_ASC_OFFSET(x)       ((x) * 16)
#define GET_ONE(x)              ((x) % 10)
#define GET_TEN(x)              (((x) % 100) / 10)
#define GET_HUNDRED(x)          (((x) % 1000) / 100)
#define GET_THOUSAND(x)         (((x) % 10000) / 1000)
#define OSD_CHAR_CNT            (MAX_CHAR_PER_LINE * OSD_MAX_LINE)
#define OSD_POS_RESET(x)        ((((x) * VI_H_NTSC) / VI_H_PAL + 0xF) & (~0xF))
#define TEST_YEAR(x)            ((x & 3) ? 0 : (x % 100 ? 1 : (x % 400 ? 0 : 1)))

#define MAX_OVERLAY_REGION_PER_CHAN     (8) /* 一个通道最多8个overlay区域 */

/********************************字库***********************/
#define FONT_ASC_LEN            4096
#define FONT_HZ_SIMPL_LEN       267616
#define FONT_HZ_TRAD_LEN        261696
/* 16点阵字库地址，需要由主机下载到DSP */
#define FONT_LIB_ADDR           0x81000000

#define FONT_LIB_ADDR_DM648     0xe2000000  /* dm648字库地址 */
#define FONT_LIB_HZ_LEN         (FONT_HZ_SIMPL_LEN > FONT_HZ_TRAD_LEN ? FONT_HZ_SIMPL_LEN : FONT_HZ_TRAD_LEN)
#define FONT_LIB_RS_LEN         1056        /* 16点阵俄文字库长度 */
#define FONT_LIB_GREECE_LEN     768         /* 16点阵希腊字库长度 */

/*
    OSD：
    汉字宽度16象素，英文8象素,高度都为16象素。
    除特殊字符外，每个字符均为8或16象素。
    每行所有字符宽度之和不得大于编码选择的图像宽度(4CIF:704,CIF、QCIF:352)。
    定义TAG(标签)为可能动态改变的字符集合，可能包含一个或多个字符。
    TAG的代码定义不能和已有的字符代码冲突
*/
#define OSD_TYPE_COMM           0x0         /* 普通设备4个region方式 */
#define OSD_TYPE_ATM            0x1         /* ATM机一个region方式 */
#define OSD_TYPE_DYNAMIC        0x2         /* 动态字库下载方式 */
#define OSD_TYPE_CLEARPART      0x4         /* 局部清除模式，在频繁发送OSD的情况下可以防止闪烁 */

#define OSD_CHAR_MIN_W          8           /* 字符最小宽度 */
#define OSD_CHAR_MIN_H          16          /* 字符最小高度 */

#define OSD_CHAR_MAX_W          64          /* 字符最大宽度 */
#define OSD_CHAR_MAX_H          64          /* 字符最大宽度 */

#define OSD_CHAR_ALIGN_H        8           /* 字符W、X对齐 */
#define OSD_CHAR_ALIGN_V        8           /* 字符H、Y对齐 */

#define FONT64x64_SIZE          (64 * 64)
#define FONT32x32_SIZE          (32 * 32)
#define FONT16x16_SIZE          (16 * 16)
#define FONT8x16_SIZE           (8  * 16)
#define FONT8x8_SIZE            (8  * 8)


#define OSD_TAG_CODE_BASE       0x9000  /* 特殊字符基址 */

/* 4位年：2002 */
#define OSD_YEAR4               (OSD_TAG_CODE_BASE + 0)
/* 2位年：02     */
#define OSD_YEAR2               (OSD_TAG_CODE_BASE + 1)
/* 3位月份：JAN～DEC */
#define OSD_MONTH3              (OSD_TAG_CODE_BASE + 2)
/* 2位月份：1～12 */
#define OSD_MONTH2              (OSD_TAG_CODE_BASE + 3)
/* 2位日期：1～31 */
#define OSD_DAY                 (OSD_TAG_CODE_BASE + 4)
/* 3位星期：MON～SUN */
#define OSD_WEEK3               (OSD_TAG_CODE_BASE + 5)
/* 2位（1位中文）星期：一～日 */
#define OSD_CWEEK1              (OSD_TAG_CODE_BASE + 6)
/* 2位24时制小时：00～23     */
#define OSD_HOUR24              (OSD_TAG_CODE_BASE + 7)
/* 2位12时制小时：00～12 */
#define OSD_HOUR12              (OSD_TAG_CODE_BASE + 8)
/* 2位分钟00～59 */
#define OSD_MINUTE              (OSD_TAG_CODE_BASE + 9)
/* 2位秒00～59 */
#define OSD_SECOND              (OSD_TAG_CODE_BASE + 10)

/* 特殊语言 */
/* 荷兰 */
/* 2位星期：MA～ZO */
#define OSD_WEEK3_DUTCH         (OSD_TAG_CODE_BASE +11)
/* 法国 */
/* 3位星期：LUN～DIM */
#define OSD_WEEK3_FRENCH        (OSD_TAG_CODE_BASE +12)
/* 德国 */
/* 2位星期：MO～SO */
#define OSD_WEEK3_GERMAN        (OSD_TAG_CODE_BASE +13)
/* 意大利 */
/* 3位星期：LUN～DOM */
#define OSD_WEEK3_ITALY         (OSD_TAG_CODE_BASE + 14)
/* 波兰 */
/* 3位星期：PON～NIE */
#define OSD_WEEK3_POLISH        (OSD_TAG_CODE_BASE + 15)
/* 西班牙 */
/* 3位星期：LUN～DOM */
#define OSD_WEEK3_SPANISH       (OSD_TAG_CODE_BASE + 16)
/* 葡萄牙 */
/* 3位星期: SEG～DOM */
#define OSD_WEEK3_PORTUGAL      (OSD_TAG_CODE_BASE + 17)
/* 俄语 */
/* 2位星期 */
#define OSD_WEEK3_RUSSIA        (OSD_TAG_CODE_BASE + 18)
/* 希腊语 */
/* 3位星期 */
#define OSD_WEEK3_GREECE        (OSD_TAG_CODE_BASE + 19)
/* 日本 */
/* 2位（1位日文）星期：一～日 */
#define OSD_CWEEK1_JAPAN        (OSD_TAG_CODE_BASE + 20)
/* 匈牙利 */
/* 3位星期：H\x82t～Vas */
#define OSD_WEEK3_HUNGARY       (OSD_TAG_CODE_BASE + 21)
/* 捷克 */
/* 2位星期：St～Ne */
#define OSD_WEEK3_CZECHISH      (OSD_TAG_CODE_BASE + 22)
/* 斯洛伐克 */
/* 2位星期：St～Ne */
#define OSD_WEEK3_SLOVAKIAN     (OSD_TAG_CODE_BASE + 23)
/* 土耳其 */
/* 3位星期：Paz～Cts */
#define OSD_WEEK3_TURKEY        (OSD_TAG_CODE_BASE + 24)
/* AMPM 上午、下午 */
#define OSD_AMPM                (OSD_TAG_CODE_BASE + 25)
/* 丹麦 */
/* 2位星期:Ma ~ S */
#define OSD_WEEK3_DANISH        (OSD_TAG_CODE_BASE + 26)
/* 斯洛文尼亚 */
/* 3位星期:pon~ ned */
#define OSD_WEEK3_SLOVENIJIA    (OSD_TAG_CODE_BASE + 27)
/* 2位星期(字符宽度为8) */
#define OSD_WEEK2               (OSD_TAG_CODE_BASE + 28)
/* 5位星期(字符宽度为8) */
#define OSD_WEEK5               (OSD_TAG_CODE_BASE + 29)

/* 特殊字符个数 */
#define OSD_TAG_CNT             32

/* 一行中能容纳的最多字符个数，超过编码行宽的字符将被忽略 */

/* 4CIF时 全英文字符 */
//#define MAX_CHAR_PER_LINE       (960/8)

#define OSD_ENC_MODE            0       /* 编码OSD模式 */
#define OSD_DEC_MODE            1       /* 解码OSD模式 */
#define OSD_DISP_MODE           2       /* 显示OSD模式 */
#define OSD_ZERO_MODE           3       /* 零通道OSD模式 */
#define OSD_JPEG_MODE	        4       /*JPEG抓图OSD模式*/


/* 8×16点阵基址 */
#define FONT8_CODE_BASE         (OSD_TAG_CODE_BASE + OSD_TAG_CNT + 10)

/* 16×16点阵基址 */
#define FONT16_CODE_BASE        (FONT8_CODE_BASE + 88 * OSD_MAX_LINE * 4)

/* 传输字库的大小，按16行来算为22k大小 */
#define FONT_LIB_SIZE           (MAX_CHAR_PER_LINE * OSD_MAX_LINE * OSD_CHAR_MAX_H)

#define OSD_4CIF_IMG_SIZE (32 * 32 * 22 * 2)
#define OSD_CIF_IMG_SIZE (16 * 16 * 22 * 2)
#define OSD_QCIF_IMG_SIZE (8 * 8 * 22 * 2)
#define OSD_DISP_IMG_SIZE (64 * 64 * 30 * 2)

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
typedef enum
{
	ENC_PREV_OSD,
	ENC_PREV_LOGO,
	DISP_PREV_OSD,
	DISP_PREV_LOGO,
	ENC_PREV_MASK,
	DEC_PREV_LOGO,
	DEC_EXT_PREV_LOGO,
	DEC_PREV_OSD,
} PREV_OSD_TYPE_E;


/* OSD字符参数 */
typedef struct
{
    /* 用户初始化的代码，在初始化后不再使用，为了考虑到以后需要对OSD做扩展，将其保留 */
    UINT32        code;

    /* 新位置，OSD设置时X、Y、W、H可能随着编码分辨率不同而改变；bit  0- 15:X(调整后)、bit  16-31:Y(调整后) */
    UINT32        pos;

    /* 旧位置，OSD设置时；bit 0-15:X(初始化值，会随编码分辨率而变化)、bit 16-31:Y(初始化值，会随编码分辨率而变化) */
    UINT32        top;
    UINT32        color;

    /* 字符大小，低16位为新参数，高16位为旧参数；bit: 0 - 7:W(调整后)、bit  8-15:H(调整后)、bit 16-23:W(初始化值，会随编码分辨率而变化)、bit 24-31:H(初始化值，会随编码分辨率而变化) */
    UINT32        size;

    /* 该字符在字库中的地址 */
    UINT8         *pFont;                       /* 字库地址，0时无效。 */
    UINT32        bChange;
    UINT32        charType;
} OSD_CHAR_S;


/*
OSD字符参数
code：用户初始化的代码，在初始化后不再使用，为了考虑到以后需要对OSD做扩展，将其保留
pos：位置，OSD设置时X、Y、W、H可能随着编码分辨率不同而改变
    bit  0- 15:X(调整后)
    bit  16-31:Y(调整后)
top：位置，OSD设置时
    bit 0-15:X(初始化值，会随编码分辨率而变化)
    bit 16-31:Y(初始化值，会随编码分辨率而变化)
size:大小。
    bit  0- 7:W(调整后)
    bit  8-15:H(调整后)
    bit 16-23:W(初始化值，会随编码分辨率而变化)
    bit 24-31:H(初始化值，会随编码分辨率而变化)
*/

typedef struct
{
    UINT32        charCnt;
    UINT32        charWidth;
    UINT32        flgUsed; /*enc|dec|disp*/

    OSD_CHAR_S    *pFirstChar[MAX_VENC_CHAN + MAX_VDEC_CHAN + MAX_DISP_CHAN];
} OSD_TAG_PARAM_S;

typedef struct
{
    PUINT8             pOsdScaleBuf;
    PhysAddr           osdScaleBufPhyAddr;
    UINT32             bufLen;
} OSD_SCALE_BUF_INFO_S;

/* OSD 管理信息 */
typedef struct
{
    void * pstRgnHdl[MAX_OVERLAY_REGION_PER_CHAN];
    BOOL bShow;
} OSD_RGN_S;


typedef struct tagEncOsdInfoSt
{
    UINT32 uiEncChnCnt;
    OSD_RGN_S stOsdRegion[OSD_MAX_CHAN];
}ENC_OSD_INFO_S;

typedef struct
{
    UINT32 charCnt;                       /* 一个OSD快的最大字符数 */
    UINT32 validCharCnt;                  /* 这块OSD区域中当前可用的更新的字符数 */
    UINT32 x, y, w, h;                    /* 该行内所有字符所在的矩形区域 */
    UINT32 osdScaleH;                     /* 水平缩放比例 */
    UINT32 osdScaleV;                     /* 垂直缩放比例 */

    void (*pDrawFunc)(PUINT8, PUINT8, UINT32 w, UINT32 h, UINT32 pitch, UINT32 color,UINT32 bgcolor);
    OSD_CHAR_S charData[MAX_CHAR_PER_LINE];
    BOOL bLineChange;
    //PUINT8 pPrevCharImg;
    PUINT8 p4CifCharImg;
    //PUINT8 p2CifCharImg;
    PUINT8 pCifCharImg;
    PUINT8 pQCifCharImg;
    //PUINT8 pVgaCharImg;
    //PUINT8 pQvgaCharImg;
    //PUINT8 p1080CharImg;
    //PUINT8 p720CharImg;
    //PUINT8 pPrevCharImgP;
    //PUINT8 pHighCharImgP;
    PUINT8 p4CifCharImgP;
    //PUINT8 p2CifCharImgP;
    PUINT8 pCifCharImgP;
    PUINT8 pQCifCharImgP;
    //PUINT8 pVgaCharImgP;
    //PUINT8 pQvgaCharImgP;
    //PUINT8 p1080CharImgP;
    //PUINT8 p720CharImgP;
} OSD_LINE_S;

/* 每块OSD的对应参数 */
typedef struct
{
    UINT32 u32Idx;                          /* 用于访问的索引下标 */
    UINT32 u32Color;
    UINT32 u32BgColor;
    UINT32 u32StringLen;
    ENCODING_FORMAT_E enEncFormat;          /* 字符串编码类型 */
    char *szString;
} OSD_SET_PRM_S;

/* OSD地址 */
typedef struct
{
    UINT8 *pu8VirAddr;
    UINT64 u64PhyAddr;
} OSD_ADDR_S;

/* OSD区域 */
typedef struct
{
    OSD_ADDR_S stAddr;
    UINT32 u32Width;                    /* 单位：像素点 */
    UINT32 u32Height;                   /* 单位：像素点 */
    UINT32 u32Stride;                   /* 跨距，单位：字节 */
    UINT32 u32Size;                     /* 内存大小 */
} OSD_REGION_S;

typedef struct
{
    OSD_REGION_S astLatRegion[OSD_LAT_PER_BLOCK];
    OSD_REGION_S astFontRegion[OSD_LAT_PER_BLOCK][OSD_FONT_PER_LAT];
    OSD_REGION_S astBgFontRegion[OSD_LAT_PER_BLOCK][OSD_FONT_PER_LAT];
} OSD_FONT_REGION_S;

/* 填充点阵函数定义 */
typedef INT32 (*OSD_FillLattice_F)(char *szString, OSD_REGION_S *pstRegion, UINT32 u32Size, ENCODING_FORMAT_E enEncForma);

/* OSD BLOCK参数 */
typedef struct
{
    BOOL bLatEnable;
    BOOL bFontEnable;
    UINT32 u32OsdNum;
    UINT32 u32StringLenMax;
    UINT32 u32LatNum;
    UINT32 au32FontNum[OSD_LAT_PER_BLOCK];
    UINT32 au32LatSize[OSD_LAT_PER_BLOCK];
    UINT32 au32FontSize[OSD_LAT_PER_BLOCK][OSD_FONT_PER_LAT];
    UINT32 au32LatSizeMax[OSD_LAT_PER_BLOCK];                      // 点阵大小的最大值，多个时，从小到大排布
    UINT32 au32FontSizeMax[OSD_LAT_PER_BLOCK][OSD_FONT_PER_LAT];   // 字体大小的最大值，多个时，从小到大排布
} OSD_BLOCK_PRM_S;


/* osd block参数 */
typedef struct
{
    pthread_rwlock_t stRWLock;              /* 字库和点阵的数据使用读写锁保护 */
    pthread_mutex_t stMutex;                /* 其他参数使用互斥锁保护 */
    OSD_FillLattice_F pfFillLat;
    UINT32 u32StringNum;
    OSD_BLOCK_PRM_S stBlockPrm;
    UINT32 au32IdxMap[OSD_STRING_NUM_MAX];
    UINT32 au32LatMap[OSD_LAT_SIZE_MAX + 1];
    UINT32 au32FontMap[OSD_LAT_PER_BLOCK][OSD_FONT_SIZE_MAX + 1];
    OSD_SET_PRM_S astSetPrm[OSD_STRING_NUM_MAX];
    OSD_FONT_REGION_S astFontRegion[OSD_STRING_NUM_MAX];
} OSD_BLOCK_S;

/* 动态OSD block参数 */
typedef struct
{
    UINT32 u32StringLenMax;
    UINT32 u32LatSizeMax;
    UINT32 u32FontSizeMax;
    UINT32 u32LatNum;
    UINT32 u32FontNum;
    OSD_FillLattice_F pfFillLat;
    OSD_REGION_S *pstLatRegion;
    OSD_REGION_S *pstFontRegion;
} OSD_VAR_BLOCK_S;

typedef struct
{
    BOOL bUse;
    pthread_mutex_t stMutex;
    OSD_VAR_BLOCK_S *apstOsdVarBlock[OSD_VAR_BLOCK_NUM_MAX];
} OSD_VAR_BLOCKS_S;

typedef struct
{
    UINT32 u32StringNum;                    /* 应该等于可以访问的索引下标的最大值 */
    OSD_SET_PRM_S *pstOsdPrm;               /* 指向u32StringNum个OSD_STRING_ADD_PRM_S结构体的空间 */
} OSD_SET_ARRAY_S;

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */
INT32 osd_func_FillFont(OSD_REGION_S *pstLat, UINT32 u32LatSize, OSD_REGION_S *pstFont, UINT32 u32FontSize, UINT16 u16Color, UINT16 u16BgColor);
INT32 osd_func_blockInit(UINT32 u32Block, OSD_BLOCK_PRM_S *pstBlockPrm);
INT32 osd_func_blockSet(UINT32 u32Block, OSD_SET_ARRAY_S *pstOsdArray);
INT32 osd_func_readStart(VOID);
INT32 osd_func_readEnd(VOID);
INT32 osd_func_writeStart(VOID);
INT32 osd_func_writeEnd(VOID);
OSD_SET_PRM_S *osd_func_getOsdPrm(UINT32 u32Block, UINT32 u32Osd);
OSD_REGION_S *osd_func_getLatRegion(UINT32 u32Block, UINT32 u32Osd, UINT32 u32LatSize);
OSD_REGION_S *osd_func_getFontRegion(UINT32 u32Block, UINT32 u32Osd, UINT32 u32LatSize, UINT32 u32FontSize, BOOL bBgEnable);
INT32 osd_func_updateFontSize(UINT32 u32Block, UINT32 u32LatSize, UINT32 u32FontNum, UINT32 *pu32FontSize);
INT32 osd_func_updateLatSize(UINT32 u32Block, OSD_BLOCK_PRM_S *pstSetPrm);
INT32 osd_func_alertNameBlockInit(UINT32 u32NameLen, UINT32 u32NameNum);
UINT32 osd_func_getFontSize(INT32 s32Size);
SAL_STATUS osd_func_asciiBlockInit(VOID);
INT32 osd_func_numBlockInit(VOID);
OSD_REGION_S *osd_func_getNumLatRegion(UINT32 u32Block, UINT32 u32Num, UINT32 u32SubNum, UINT32 u32LatSize);
INT32 osd_func_getFreeVarBlock(OSD_VAR_BLOCK_S *pstBlockPrm, UINT32 *pu32Block);
OSD_REGION_S *osd_func_getVarLatRegionSet(UINT32 u32Block, UINT32 u32Num);
OSD_REGION_S *osd_func_getVarFontRegionSet(UINT32 u32Block, UINT32 u32Num);
INT32 osd_freetype_FillLattice(char *szString, OSD_REGION_S *pstRegion, UINT32 u32Size, ENCODING_FORMAT_E enEncFormat);
UINT32 osd_func_getLatBuffLen(UINT32 u32LatSize, UINT32 u32StrLen);
UINT32 osd_func_getFontBuffLen(UINT32 u32FontSize, UINT32 u32StrLen);
INT32 osd_func_getFontByString(CHAR *szString, OSD_REGION_S *pstLat, OSD_REGION_S *pstFont, UINT32 u32Size, UINT16 u16Color, UINT32 u16BgColor, ENCODING_FORMAT_E enEncFormat);

/**
 * @function   osd_drv_getAscHzFontAddr
 * @brief      获取汉字或者ASCII字符的地址
 * @param[in]   UINT8 *pu8Char 字符地址
 * @param[in]   UINT32 *pu32CharLen 字符长度，汉字为2，ASCII为1
 * @param[out]  None
 * @return     PUINT8 字模存放地址指针
 */
PUINT8 osd_drv_getAscHzFontAddr(UINT8 *pu8Char, UINT32 *pu32CharLen);

/**
 * @function    osd_drv_refreshTag
 * @brief       根据标记位，分别刷新OSD的每一位{年月日星期时分秒等}
 * @param[in]   UINT32 flg 刷新标记
 * @param[in]   UINT32 u32EncOffset flg通道偏移
 * @param[in]   UINT32 u32Chan 通道号
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
void osd_drv_refreshTag(UINT32 flg, UINT32 u32Chan, UINT32 u32EncOffset);

/**
 * @function    osd_drv_getColor
 * @brief       计算OSD字体的颜色
 * @param[in]   OSD_LINE_S *pLine OSD行参数指针
 * @param[in]   PUINT8 pLumaAddr 图像Y分量地址
 * @param[in]   UINT32 imgW, UINT32 imgH 图像宽高
 * @param[in]   UINT32 stride 跨度
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
void osd_drv_getColor(OSD_LINE_S  *pLine, PUINT8 luma, UINT32 imgW, UINT32 imgH, UINT32 stride);

/**
 * @function    osd_drv_regionCreate
 * @brief       申请OSD RGN通道
 * @param[in]   UINT32 startIdx rgn 起始id
 * @param[in]   UINT32 regionNum rgn个数
 * @param[out]  OSD_RGN_S *pstRegion osd句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_regionCreate(UINT32 startIdx, UINT32 regionNum, OSD_RGN_S *pstRegion);

/**
 * @function    osd_drv_Init
 * @brief       模块初始化
 * @param[in]   MEMORY_BUFF_S * pHzLib 汉字字库
 * @param[in]   MEMORY_BUFF_S * pAscLib 英文字库
 * @param[in]   void * pEncHzLib 字库地址
 * @param[in]   void * pEncAscLib 字库地址
 * @param[in]   UINT32 isLattice 是否点阵字库
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_Init(MEMORY_BUFF_S *pHzLib, MEMORY_BUFF_S *pAscLib, void *pEncHzLib, void *pEncAscLib, UINT32 isLattice);

/**
 * @function    osd_drv_destroy
 * @brief       销毁指定的REGION，注意，调用此函数需要加锁pthread_mutex_lock(&muProcessOsd[chan])保护
 * @param[in]   UINT32 chan 通道号
 * @param[in]   UINT32 line 行号
 * @param[in]   OSD_RGN_S *pstRegion osd句柄
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_destroy(UINT32 chan, UINT32 line, OSD_RGN_S *pstRegion);

/**
 * @function    osd_drv_paramCheck
 * @brief       osd参数校验
 * @param[in]   UINT32 imgW, UINT32 imgH 图像宽高
 * @param[in]   OSD_LINE_S *pLine OSD行参数指针
 * @param[in]   UINT32 mode 通道类型
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_paramCheck(UINT32 imgW, UINT32 imgH, OSD_LINE_S *pLine, UINT32 mode);

/**
 * @function    osd_drv_refreshDateTime
 * @brief       刷新OSD时间. 通过获取系统当前时间判断字符是否需要刷新
 * @param[in]  UINT32 u32MaskFlag flag字段mask
 * @param[out]  UINT32 *pFlg OSD标记指针
 * @return      NoneL
 */
void osd_drv_refreshDateTime(UINT32 *pFlg, UINT32 u32MaskFlag);

/**
 * @function    osd_drv_draw
 * @brief       OSD点阵描绘
 * @param[in]   OSD_LINE_S *pLine OSD行参数指针
 * @param[in]   UINT32 mode 通道类型
 * @param[out]  BOOL *pFlgChange 是否更新字符
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_draw(OSD_LINE_S *pLine, UINT32  mode, BOOL *pFlgChange);

/**
 * @function    osd_drv_process
 * @brief       OSD叠加
 * @param[in]   UINT32 chan 通道号
 * @param[in]   UINT32 resolution 分辨率
 * @param[in]   UINT32 line 行号
 * @param[in]   OSD_RGN_S *pRegion OSD句柄
 * @param[in]   OSD_LINE_S *pLine OSD行参数指针
 * @param[in]   UINT32 bTranslucent 半透明标记
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_process(UINT32 chan, UINT32 resolution, UINT32 line, OSD_RGN_S *pRegion, OSD_LINE_S *pLine, UINT32 bTranslucent);

/**
 * @function    osd_drv_clearTag
 * @brief       清除osd tag标志
 * @param[in]   UINT32 chan 通道号
 * @param[in]   UINT32 mode 通道类型
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
void osd_drv_clearTag(UINT32 chan, UINT32 mode);

/**
 * @function    osd_drv_charInit
 * @brief       初始化OSD字符
 * @param[in]   UINT32 chan 通道号
 * @param[in]   UINT32 u32ChanOffset 通道偏移
 * @param[in]   PUINT8 pLib 字库指针
 * @param[in]   OSD_LINE_S *pLine OSD行参数指针
 * @param[in]   UINT32 code 要处理的字符
 * @param[in]   UINT32 *pLeft 字符水平位置
 * @param[in]   UINT32 top 字符高度 
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_charInit(UINT32 chan, UINT32 u32ChanOffset,PUINT8 pLib, OSD_LINE_S *pLine, UINT32 code, UINT32 *pLeft, UINT32 top);

/**
 * @function    osd_drv_setLineParam
 * @brief       设置OSD行参数，eg，缩放比例等
 * @param[in]   OSD_LINE_S *pLine OSD行参数指针
 * @param[in]   UINT32 osdScaleV 水平缩放系数
 * @param[in]   UINT32 osdScaleH 垂直缩放系数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_setLineParam(OSD_LINE_S *pLine,  UINT32 osdScaleV, UINT32 osdScaleH);

/**
 * @function    osd_drv_clearDisp
 * @brief       清除预览OSD、LOGO等
 * @param[in]   UINT32 chan 通道号
 * @param[in]   UINT32 dispChan
 * @param[in]   OSD_LINE_S *pLine
 * @param[in]   PREV_OSD_TYPE_E mode
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
void osd_drv_clearDisp(UINT32 chan,UINT32 dispChan,  OSD_LINE_S *pLine, PREV_OSD_TYPE_E mode);

/**
 * @function    osd_drv_disp
 * @brief       处理预览OSD、LOGO等
 * @param[in]   UINT32 chan 通道号
 * @param[in]   UINT32 dispChan
 * @param[in]   OSD_LINE_S *pLine
 * @param[in]   PREV_OSD_TYPE_E mode
 * @param[in]   void *pRect
 * @param[in]   PUINT8 pBuf
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
void osd_drv_disp(UINT32 chan, UINT32 dispChan, OSD_LINE_S *pLine, PREV_OSD_TYPE_E mode, void *pRect, PUINT8 pBuf);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*_OSD_DRV_H_*/

