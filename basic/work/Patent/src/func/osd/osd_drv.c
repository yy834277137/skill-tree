/**
 * @file   osd_drv.c
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

#include <sal.h>
#include "osd_drv_api.h"
#include "drawChar.h"
#include "freetype_func.h"
#include "platform_hal.h"
#include "system_prm_api.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

/*最小的区域宽高值*/
#define RGN_MIN_W	(4)
#define RGN_MIN_H	(4)

#define MAX_IMG_W	(704)
#define MAX_IMG_H	(576)
#define MIN(_1_, _2_)	((_1_) > (_2_) ? (_2_) : (_1_))
#define MAX(_1_, _2_)	((_1_) > (_2_) ? (_1_) : (_2_))

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* 字库 */
UINT32 *fontAscLib = NULL;
UINT32 *fontHzLib = NULL;

UINT8 daysInYear[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const UINT8 weeksNameEng[7][3] =
{
    {'S', 'u', 'n'},
    {'M', 'o', 'n'},
    {'T', 'u', 'e'},
    {'W', 'e', 'd'},
    {'T', 'h', 'u'},
    {'F', 'r', 'i'},
    {'S', 'a', 't'},
};
/* 中文星期 */
static UINT32 weeksNameChn[7] =
{
    0xc8d5,
    0xd2bb,
    0xb6fe,
    0xc8fd,
    0xcbc4,
    0xcee5,
    0xc1f9,
};
/* 动态字库星期 */
static const UINT8 weeks_2_8[7][2] =
{
    {0, 1},
    {2, 3},
    {4, 5},
    {6, 7},
    {8, 9},
    {10, 11},
    {12, 13},
};
#if 0
static const UINT8 weeks_3_8[7][3] =
{
    {0, 1, 2},
    {3, 4, 5},
    {6, 7, 8},
    {9, 10, 11},
    {12, 13, 14},
    {15, 16, 17},
    {18, 19, 20},
};
#endif
static const UINT8 weeks_5_8[7][5] =
{
    {0, 1, 2, 3, 4},
    {5, 6, 7, 8, 9},
    {10, 11, 12, 13, 14},
    {15, 16, 17, 18, 19},
    {20, 21, 22, 23, 24},
    {25, 26, 27, 28, 29},
    {30, 31, 32, 33, 34},
};
#if 0
static const UINT8 weeks_1_16[7][1] =
{
    {0},
    {2},
    {4},
    {6},
    {8},
    {10},
    {12},
};
#endif
static const UINT8 monthNameEng[12][3] =
{
    {'J', 'a', 'n'}, {'F', 'e', 'b'}, {'M', 'a', 'r'}, {'A', 'p', 'r'}, {'M', 'a', 'y'}, {'J', 'u', 'n'},
    {'J', 'u', 'l'}, {'A', 'u', 'g'}, {'S', 'e', 'p'}, {'O', 'c', 't'}, {'N', 'o', 'v'}, {'D', 'e', 'c'},
};

/* 特殊字符结构 */
OSD_TAG_PARAM_S g_stOsdTagParam[OSD_TAG_CNT] =
{
    /* charCnt,     charWidth,      flgEncUsed, */
    {4, OSD_CHAR_W_ASC, 0},             /* 0,year4 */
    {2, OSD_CHAR_W_ASC, 0},             /* 1,year2 */
    {3, OSD_CHAR_W_ASC, 0},             /* 2,Month3 */
    {2, OSD_CHAR_W_ASC, 0},             /* 3,month2 */
    {2, OSD_CHAR_W_ASC, 0},             /* 4,day */
    {3, OSD_CHAR_W_ASC, 0},             /* 5,week3 */
    {1, OSD_CHAR_W_HZ, 0},              /* 6,cweek1 */
    {2, OSD_CHAR_W_ASC, 0},             /* 7,hour24 */
    {2, OSD_CHAR_W_ASC, 0},             /* 8,hour12 */
    {2, OSD_CHAR_W_ASC, 0},             /* 9,minute */
    {2, OSD_CHAR_W_ASC, 0},             /* 10,second */
    {2, OSD_CHAR_W_ASC, 0},             /* 11,荷兰星期 */
    {3, OSD_CHAR_W_ASC, 0},             /* 12,法国星期 */
    {2, OSD_CHAR_W_ASC, 0},             /* 13,德国星期 */
    {3, OSD_CHAR_W_ASC, 0},             /* 14,意大利星期 */
    {3, OSD_CHAR_W_ASC, 0},             /* 15,波兰星期 */
    {3, OSD_CHAR_W_ASC, 0},             /* 16,西班牙星期 */
    {3, OSD_CHAR_W_ASC, 0},             /* 17,葡萄牙星期 */
    {2, OSD_CHAR_W_ASC, 0},             /* 18,俄语星期 */
    {3, OSD_CHAR_W_ASC, 0},             /* 19,希腊语星期 */
    {1, OSD_CHAR_W_HZ, 0},              /* 20,日本星期 */
    {3, OSD_CHAR_W_ASC, 0},             /* 21,匈牙利星期 */
    {2, OSD_CHAR_W_ASC, 0},             /* 22, 捷克语星期 */
    {2, OSD_CHAR_W_ASC, 0},             /* 23,斯洛伐克语星期 */
    {3, OSD_CHAR_W_ASC, 0},             /* 24,土耳其语星期 */
    {2, OSD_CHAR_W_ASC, 0},             /* 25,AMPM */
    {3, OSD_CHAR_W_ASC, 0},             /* 26,西班牙星期 */
    {3, OSD_CHAR_W_ASC, 0},             /* 27,葡萄牙星期 */
    {2, OSD_CHAR_W_ASC, 0},             /* 28,week_2_8 */
    {5, OSD_CHAR_W_ASC, 0},             /* 29,week_5_8 */
};

static DATE_TIME dtOld = {0};
static DATE_TIME now = {0};


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function   osd_getCharType
 * @brief      判断字模地址(地址范围已经预先设定好)，得到字模类型索引
 * @param[in]  UINT32 x 字模地址
 * @param[out]  None
 * @return     UINT32 字模类型索引
 */
static UINT32 osd_getCharType(UINT32 x)
{
#if 0 /* 待修改 */
    if (pDspInitParam->NetOsdType & OSD_TYPE_DYNAMIC)
    {
        if (x >= OSD_TAG_CODE_BASE && x < OSD_TAG_CODE_BASE + OSD_TAG_CNT)
        {
            return OSD_CHAR_TYPE_TAG;
        }
        /* 8x8点阵字库 */
        else if (x >= FONT8_CODE_BASE
                 && x < (FONT8_CODE_BASE + MAX_CHAR_PER_LINE * OSD_MAX_LINE))
        {
            return OSD_CHAR_TYPE_FONT8;
        }
        /* 16x16点阵字库 */
        else if (x >= FONT16_CODE_BASE
                 && x < (FONT16_CODE_BASE + MAX_CHAR_PER_LINE * OSD_MAX_LINE / 2))
        {
            return OSD_CHAR_TYPE_FONT16;
        }
        /* ASCII 字符表 */
        else if (x <= 0xff)
        {
            return OSD_CHAR_TYPE_ASC;
        }
        else
        {
            return 0xFF;
        }
    }
    else
#endif
    {
        if (x >= OSD_TAG_CODE_BASE && x < OSD_TAG_CODE_BASE + OSD_TAG_CNT)
        {
            return OSD_CHAR_TYPE_TAG;
        }
        else if (x < 0xff)
        {
            return OSD_CHAR_TYPE_ASC;
        }
        else
        {
            return OSD_CHAR_TYPE_FONT16;
        }
    }
}

/**
 * @function   osd_getCharFontAddr
 * @brief      根据ASCII码值得到字模存放地址
 * @param[in]   UINT32 code 字符码偏移
 * @param[out]  None
 * @return     PUINT8 字模存放地址指针
 */
static PUINT8 inline osd_getCharFontAddr(UINT32 code)
{
    UINT32 charType = osd_getCharType(code);
    UINT8 *addr = NULL;

    if (charType == OSD_CHAR_TYPE_ASC)
    {
        addr = (PUINT8)fontAscLib + code * 16;
    }
    else
    {
        addr = (PUINT8)fontHzLib + GET_HZ_OFFSET(code);
    }

    return addr;
}

/**
 * @function   osd_drv_getAscHzFontAddr
 * @brief      获取汉字或者ASCII字符的地址
 * @param[in]   UINT8 *pu8Char 字符地址
 * @param[in]   UINT32 *pu32CharLen 字符长度，汉字为2，ASCII为1
 * @param[out]  None
 * @return     PUINT8 字模存放地址指针
 */
PUINT8 osd_drv_getAscHzFontAddr(UINT8 *pu8Char, UINT32 *pu32CharLen)
{
    UINT32 u32Code = 0;

    if (*pu8Char < 0x80)
    {
        u32Code = (UINT32)(*pu8Char);
        *pu32CharLen = 1;
    }
    else if (*pu8Char > 0xA0)
    {
        u32Code = (pu8Char[0] << 8) | (pu8Char[1] & 0xFF);
        *pu32CharLen = 2;
    }
    else
    {
        return NULL;
    }

    return osd_getCharFontAddr(u32Code);
}

/**
 * @function   osd_getCharFontCode
 * @brief      根据字模地址得到ASCII值
 * @param[in]   PUINT8 addr 字模地址
 * @param[out]  None
 * @return     UINT32 该字模的ASCII值、点阵字库地址
 */
UINT32 osd_getCharFontCode(PUINT8 addr)
{
    UINT32 charType;
    UINT32 code = 0;

    #if 0 /* 待修改 */
    if (pDspInitParam->NetOsdType & OSD_TYPE_DYNAMIC)
    {
        if (((UINT32)addr >= (UINT32)dspParam.fontAscLib)
            && ((UINT32)addr <= ((UINT32)dspParam.fontAscLib + 4096)))
        {
            charType = OSD_CHAR_TYPE_ASC;
        }
        else
        {
            charType = OSD_CHAR_TYPE_FONT16;
        }

        if (charType == OSD_CHAR_TYPE_ASC)
        {
            code = (UINT32)((UINT32)addr - (UINT32)dspParam.fontAscLib) / 16;
        }
    }
    else
    #endif
    {
        if ((PhysAddr)addr >= (PhysAddr)fontHzLib)
        {
            charType = OSD_CHAR_TYPE_FONT16;
        }
        else
        {
            charType = OSD_CHAR_TYPE_ASC;
        }

        if (charType == OSD_CHAR_TYPE_ASC)
        {
            code = (UINT32)((PhysAddr)addr - (PhysAddr)fontAscLib) / 16;
        }
    }

    return code;
}

/**
 * @function   osd_updateWeekTagChar
 * @brief      根据刷新模式的不同，刷新特殊字符
 * @param[in]  OSD_TAG_PARAM_S *pstTag 特殊字符结构指针
 * @param[in]  UINT32 *pCode 特殊字符指针
 * @param[in]  UINT32 u32ChanOffset 通道偏移(输入/回放/预览)
 * @param[in]  UINT32 u32Chan 通道索引
 * @param[out]  None
 * @return     None
 */
static void osd_updateWeekTagChar(OSD_TAG_PARAM_S *pstTag, UINT32 *pCode, UINT32 u32ChanOffset, UINT32 u32Chan)
{
    UINT32 i = 0;
    OSD_CHAR_S *pChar = NULL;

    if (pstTag->flgUsed & (1 << (u32Chan + u32ChanOffset)))
    {
        pChar = pstTag->pFirstChar[u32Chan + u32ChanOffset];
        for (i = 0; i < pstTag->charCnt; i++)
        {
            pChar[i].code = pCode[i];
            pChar[i].pFont = osd_getCharFontAddr(pCode[i]);

            pChar[i].bChange = SAL_TRUE;
        }
    }
}

/**
 * @function   osd_updateTagChar
 * @brief      根据刷新模式的不同，刷新特殊字符
 * @param[in]  OSD_TAG_PARAM_S *pTag 特殊字符结构指针
 * @param[in]  UINT32 *pCode 特殊字符指针
 * @param[in]  UINT32 u32ChanOffset 通道偏移(输入/回放/预览)
 * @param[in]  UINT32 u32Chan 通道索引
 * @param[out]  None
 * @return     None
 */
static void osd_updateTagChar(OSD_TAG_PARAM_S *pTag, UINT32 *pCode, UINT32 u32ChanOffset, UINT32 u32Chan)
{
    UINT32 i = 0;
    OSD_CHAR_S *pChar = NULL;

    if (pTag->flgUsed & (1 << (u32Chan + u32ChanOffset)))
    {
        pChar = pTag->pFirstChar[u32Chan + u32ChanOffset];

        for (i = 0; i < pTag->charCnt; i++)
        {
            pChar[i].code = pCode[i];
            pChar[i].pFont = osd_getCharFontAddr(pCode[i]);

            pChar[i].bChange = SAL_TRUE;
        }
    }

    return;
}

/**
 * @function   osd_refreshYearTag
 * @brief      根据年份标记位刷新字符
 * @param[in]  UINT32 flg 刷新标记
 * @param[in]  UINT32 u32ChanOffset 通道偏移(输入/回放/预览)
 * @param[in]  UINT32 u32Chan 通道索引
 * @param[out]  None
 * @return     None
 */
static void osd_refreshYearTag(UINT32 flg, UINT32 u32ChanOffset, UINT32 u32Chan)
{
    UINT32 code[4] = {0};

    if (flg & (1 << (OSD_YEAR2 - OSD_TAG_CODE_BASE)))
    {
        code[0] = ('0' + GET_TEN(now.year));
        code[1] = ('0' + GET_ONE(now.year));

        osd_updateTagChar(&g_stOsdTagParam[OSD_YEAR2 - OSD_TAG_CODE_BASE], code, u32ChanOffset, u32Chan);
    }

    if (flg & (1 << (OSD_YEAR4 - OSD_TAG_CODE_BASE)))
    {
        code[0] = ('0' + GET_THOUSAND(now.year));
        code[1] = ('0' + GET_HUNDRED(now.year));
        code[2] = ('0' + GET_TEN(now.year));
        code[3] = ('0' + GET_ONE(now.year));

        osd_updateTagChar(&g_stOsdTagParam[OSD_YEAR4 - OSD_TAG_CODE_BASE], code, u32ChanOffset, u32Chan);
    }

    return;
}

/**
 * @function   osd_refreshMonthTag
 * @brief      根据月份标记位刷新字符
 * @param[in]  UINT32 flg 刷新标记
 * @param[in]  UINT32 u32ChanOffset 通道偏移(输入/回放/预览)
 * @param[in]  UINT32 u32Chan 通道索引
 * @param[out]  None
 * @return     None
 */
static void osd_refreshMonthTag(UINT32 flg, UINT32 u32ChanOffset, UINT32 u32Chan)
{
    UINT32 code[4] = {0};

    if (flg & (1 << (OSD_MONTH2 - OSD_TAG_CODE_BASE)))
    {
        code[0] = ('0' + GET_TEN(now.month));
        code[1] = ('0' + GET_ONE(now.month));

        osd_updateTagChar(&g_stOsdTagParam[OSD_MONTH2 - OSD_TAG_CODE_BASE], code, u32ChanOffset, u32Chan);
    }

    if (flg & (1 << (OSD_MONTH3 - OSD_TAG_CODE_BASE)))
    {
        code[0] = monthNameEng[now.month - 1][0];
        code[1] = monthNameEng[now.month - 1][1];
        code[2] = monthNameEng[now.month - 1][2];

        osd_updateTagChar(&g_stOsdTagParam[OSD_MONTH3 - OSD_TAG_CODE_BASE], code, u32ChanOffset, u32Chan);
    }

    return;
}

/**
 * @function   osd_refreshDayTag
 * @brief      根据日标记位刷新字符
 * @param[in]  UINT32 flg 刷新标记
 * @param[in]  UINT32 u32ChanOffset 通道偏移(输入/回放/预览)
 * @param[in]  UINT32 u32Chan 通道索引
 * @param[out]  None
 * @return     None
 */
static void osd_refreshDayTag(UINT32 flg, UINT32 u32ChanOffset, UINT32 u32Chan)
{
    UINT32 code[2] = {0};

    if (flg & (1 << (OSD_DAY - OSD_TAG_CODE_BASE)))
    {
        code[0] = ('0' + GET_TEN(now.day));
        code[1] = ('0' + GET_ONE(now.day));

        osd_updateTagChar(&g_stOsdTagParam[OSD_DAY - OSD_TAG_CODE_BASE], code, u32ChanOffset, u32Chan);
    }

    return;
}

/**
 * @function   osd_refreshWeekTag
 * @brief      根据星期标记位刷新字符
 * @param[in]  UINT32 flg 刷新标记
 * @param[in]  UINT32 u32ChanOffset 通道偏移(输入/回放/预览)
 * @param[in]  UINT32 u32Chan 通道索引
 * @param[out]  None
 * @return     None
 */
static void osd_refreshWeekTag(UINT32 flg, UINT32 u32ChanOffset, UINT32 u32Chan)
{
    UINT32 code[5] = {0};

    if (flg & (1 << (OSD_WEEK2 - OSD_TAG_CODE_BASE)))
    {
        code[0] = weeks_2_8[now.dayOfWeek][0];
        code[1] = weeks_2_8[now.dayOfWeek][1];
        osd_updateWeekTagChar(&g_stOsdTagParam[OSD_WEEK2 - OSD_TAG_CODE_BASE], code, u32ChanOffset,
                              u32Chan);
    }

    if (flg & (1 << (OSD_WEEK3 - OSD_TAG_CODE_BASE)))
    {
        code[0] = weeksNameEng[now.dayOfWeek][0];
        code[1] = weeksNameEng[now.dayOfWeek][1];
        code[2] = weeksNameEng[now.dayOfWeek][2];

        osd_updateWeekTagChar(&g_stOsdTagParam[OSD_WEEK3 - OSD_TAG_CODE_BASE], code, u32ChanOffset,
                              u32Chan);
    }

    if (flg & (1 << (OSD_WEEK5 - OSD_TAG_CODE_BASE)))
    {
        code[0] = weeks_5_8[now.dayOfWeek][0];
        code[1] = weeks_5_8[now.dayOfWeek][1];
        code[2] = weeks_5_8[now.dayOfWeek][2];
        code[3] = weeks_5_8[now.dayOfWeek][3];
        code[4] = weeks_5_8[now.dayOfWeek][4];
        osd_updateWeekTagChar(&g_stOsdTagParam[OSD_WEEK5 - OSD_TAG_CODE_BASE], code, u32ChanOffset,
                              u32Chan);
    }

    if (flg & (1 << (OSD_CWEEK1 - OSD_TAG_CODE_BASE)))
    {
        code[0] = weeksNameChn[now.dayOfWeek];

        osd_updateWeekTagChar(&g_stOsdTagParam[OSD_CWEEK1 - OSD_TAG_CODE_BASE], code, u32ChanOffset,
                              u32Chan);
    }

    return;
}

/**
 * @function   osd_refreshHourTag
 * @brief      根据小时标记位刷新字符
 * @param[in]  UINT32 flg 刷新标记
 * @param[in]  UINT32 u32ChanOffset 通道偏移(输入/回放/预览)
 * @param[in]  UINT32 u32Chan 通道索引
 * @param[out]  None
 * @return     None
 */
static void osd_refreshHourTag(UINT32 flg, UINT32 u32ChanOffset, UINT32 u32Chan)
{
    UINT32 code[2] = {0};

    if (flg & (1 << (OSD_HOUR24 - OSD_TAG_CODE_BASE)))
    {
        code[0] = ('0' + GET_TEN(now.hour));
        code[1] = ('0' + GET_ONE(now.hour));

        osd_updateTagChar(&g_stOsdTagParam[OSD_HOUR24 - OSD_TAG_CODE_BASE], code, u32ChanOffset, u32Chan);
    }

    if (flg & (1 << (OSD_HOUR12 - OSD_TAG_CODE_BASE)))
    {
        if (now.hour > 12)
        {
            code[0] = ('0' + GET_TEN(now.hour - 12));
            code[1] = ('0' + GET_ONE(now.hour - 12));
        }
        else if (now.hour == 0)                 /* 12小时制没有0点，只有1到12点 */
        {
            code[0] = ('0' + GET_TEN(12));
            code[1] = ('0' + GET_ONE(12));
        }
        else
        {
            code[0] = ('0' + GET_TEN(now.hour));
            code[1] = ('0' + GET_ONE(now.hour));
        }

        osd_updateTagChar(&g_stOsdTagParam[OSD_HOUR12 - OSD_TAG_CODE_BASE], code, u32ChanOffset, u32Chan);
    }

    return;
}

/**
 * @function   osd_refreshMinuteTag
 * @brief      根据分钟标记位刷新字符
 * @param[in]  UINT32 flg 刷新标记
 * @param[in]  UINT32 u32ChanOffset 通道偏移(输入/回放/预览)
 * @param[in]  UINT32 u32Chan 通道索引
 * @param[out]  None
 * @return     None
 */
static void osd_refreshMinuteTag(UINT32 flg, UINT32 u32ChanOffset, UINT32 u32Chan)
{
    UINT32 code[2] = {0};

    if (flg & (1 << (OSD_MINUTE - OSD_TAG_CODE_BASE)))
    {
        code[0] = ('0' + GET_TEN(now.minute));
        code[1] = ('0' + GET_ONE(now.minute));

        osd_updateTagChar(&g_stOsdTagParam[OSD_MINUTE - OSD_TAG_CODE_BASE], code, u32ChanOffset, u32Chan);
    }

    return;
}

/**
 * @function   osd_refreshSecondTag
 * @brief      根据秒标记位刷新字符
 * @param[in]  UINT32 flg 刷新标记
 * @param[in]  UINT32 u32ChanOffset 通道偏移(输入/回放/预览)
 * @param[in]  UINT32 u32Chan 通道索引
 * @param[out]  None
 * @return     None
 */
static void osd_refreshSecondTag(UINT32 flg, UINT32 u32ChanOffset, UINT32 u32Chan)
{
    UINT32 code[2] = {0};

    if (flg & (1 << (OSD_SECOND - OSD_TAG_CODE_BASE)))
    {
        code[0] = ('0' + GET_TEN(now.second));
        code[1] = ('0' + GET_ONE(now.second));

        osd_updateTagChar(&g_stOsdTagParam[OSD_SECOND - OSD_TAG_CODE_BASE], code, u32ChanOffset, u32Chan);
    }

    return;
}

/**
 * @function   osd_refreshAMPMTag
 * @brief      根据AMPM标记位刷新字符
 * @param[in]  UINT32 flg 刷新标记
 * @param[in]  UINT32 u32ChanOffset 通道偏移(输入/回放/预览)
 * @param[in]  UINT32 u32Chan 通道索引
 * @param[out]  None
 * @return     None
 */
static void osd_refreshAMPMTag(UINT32 flg, UINT32 u32ChanOffset, UINT32 u32Chan)
{
    UINT32 code[2] = {0};

    if (flg & (1 << (OSD_AMPM - OSD_TAG_CODE_BASE)))
    {
        if (now.hour < 12)
        {
            code[0] = 'A';
        }
        else
        {
            code[0] = 'P';
        }

        code[1] = 'M';

        osd_updateTagChar(&g_stOsdTagParam[OSD_AMPM - OSD_TAG_CODE_BASE], code, u32ChanOffset, u32Chan);
    }

    return;
}

/**
 * @function   osd_refreshFlgYear
 * @brief      刷新年标记
 * @param[in]  UINT32 u32MaskFlag flag字段mask
 * @param[out] UINT32 *pFlg OSD标记指针
 * @return     None
 */
static void osd_refreshFlgYear(UINT32 *pFlg, UINT32 u32MaskFlag)
{
    if (g_stOsdTagParam[OSD_YEAR2 - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_YEAR2 - OSD_TAG_CODE_BASE));
    }

    if (g_stOsdTagParam[OSD_YEAR4 - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_YEAR4 - OSD_TAG_CODE_BASE));
    }
}

/**
 * @function   osd_refreshFlgMonth
 * @brief      刷新月标记
 * @param[in]  UINT32 u32MaskFlag flag字段mask
 * @param[out] UINT32 *pFlg OSD标记指针
 * @return     None
 */
static void osd_refreshFlgMonth(UINT32 *pFlg, UINT32 u32MaskFlag)
{
    if (g_stOsdTagParam[OSD_MONTH2 - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_MONTH2 - OSD_TAG_CODE_BASE));
    }

    if (g_stOsdTagParam[OSD_MONTH3 - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_MONTH3 - OSD_TAG_CODE_BASE));
    }
}

/**
 * @function   osd_refreshDay
 * @brief      刷新日标记
 * @param[in]  UINT32 u32MaskFlag flag字段mask
 * @param[out] UINT32 *pFlg OSD标记指针
 * @return     None
 */
static void osd_refreshDay(UINT32 *pFlg, UINT32 u32MaskFlag)
{
    if (g_stOsdTagParam[OSD_DAY - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_DAY - OSD_TAG_CODE_BASE));
    }
}

/**
 * @function   osd_refreshWeek
 * @brief      刷新星期标记
 * @param[in]  UINT32 u32MaskFlag flag字段mask
 * @param[out] UINT32 *pFlg OSD标记指针
 * @return     None
 */
static void osd_refreshWeek(UINT32 *pFlg, UINT32 u32MaskFlag)
{
    if (g_stOsdTagParam[OSD_WEEK2 - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_WEEK2 - OSD_TAG_CODE_BASE));
    }

    if (g_stOsdTagParam[OSD_WEEK3 - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_WEEK3 - OSD_TAG_CODE_BASE));
    }

    if (g_stOsdTagParam[OSD_CWEEK1 - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_CWEEK1 - OSD_TAG_CODE_BASE));
    }

    if (g_stOsdTagParam[OSD_WEEK5 - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_WEEK5 - OSD_TAG_CODE_BASE));
    }
}

/**
 * @function   osd_refreshHour
 * @brief      刷新小时标记
 * @param[in]  UINT32 u32MaskFlag flag字段mask
 * @param[out] UINT32 *pFlg OSD标记指针
 * @return     None
 */
static void osd_refreshHour(UINT32 *pFlg, UINT32 u32MaskFlag)
{
    if (g_stOsdTagParam[OSD_HOUR24 - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_HOUR24 - OSD_TAG_CODE_BASE));
    }

    if (g_stOsdTagParam[OSD_HOUR12 - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_HOUR12 - OSD_TAG_CODE_BASE));
    }
}

/**
 * @function   osd_refreshMinute
 * @brief      刷新分钟标记
 * @param[in]  UINT32 u32MaskFlag flag字段mask
 * @param[out] UINT32 *pFlg OSD标记指针
 * @return     None
 */
static void osd_refreshMinute(UINT32 *pFlg, UINT32 u32MaskFlag)
{
    if (g_stOsdTagParam[OSD_MINUTE - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_MINUTE - OSD_TAG_CODE_BASE));
    }
}

/**
 * @function   osd_refreshSecond
 * @brief      刷新秒标记
 * @param[in]  UINT32 u32MaskFlag flag字段mask
 * @param[out] UINT32 *pFlg OSD标记指针
 * @return     None
 */
static void osd_refreshSecond(UINT32 *pFlg, UINT32 u32MaskFlag)
{
    if (g_stOsdTagParam[OSD_SECOND - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_SECOND - OSD_TAG_CODE_BASE));
    }
}

/**
 * @function   osd_refreshAMPM
 * @brief      刷新AMPM标记
 * @param[in]  UINT32 u32MaskFlag flag字段mask
 * @param[out] UINT32 *pFlg OSD标记指针
 * @return     None
 */
static void osd_refreshAMPM(UINT32 *pFlg, UINT32 u32MaskFlag)
{
    if (g_stOsdTagParam[OSD_AMPM - OSD_TAG_CODE_BASE].flgUsed & u32MaskFlag)
    {
        *pFlg |= (1 << (OSD_AMPM - OSD_TAG_CODE_BASE));
    }
}

/**
 * @function    osd_drv_refreshDateTime
 * @brief       刷新OSD时间. 通过获取系统当前时间判断字符是否需要刷新
 * @param[in]  UINT32 u32MaskFlag flag字段mask
 * @param[out]  UINT32 *pFlg OSD标记指针
 * @return      NoneL
 */
void osd_drv_refreshDateTime(UINT32 *pFlg, UINT32 u32MaskFlag)
{
    UINT32 u32Flg = 0;
    
    if (NULL == pFlg)
    {
        return;
    }

    /* 获取支持夏令时的时间 */
    SAL_getDateTime_DST(&now);

    if (0 == memcmp(&now, &dtOld, sizeof(DATE_TIME)))
    {
        return;
    }

    if (dtOld.year != now.year)
    {
        /* 刷新年标记 */
        osd_refreshFlgYear(&u32Flg, u32MaskFlag);
    }

    if (dtOld.month != now.month)
    {
        /* 刷新月标记 */
        osd_refreshFlgMonth(&u32Flg, u32MaskFlag);
    }

    if (dtOld.dayOfWeek != now.dayOfWeek)
    {
        /* 刷新月标记 */
        osd_refreshWeek(&u32Flg, u32MaskFlag);
    }

    if (dtOld.day != now.day)
    {
        /* 刷新日标记 */
        osd_refreshDay(&u32Flg, u32MaskFlag);
    }

    if (dtOld.hour != now.hour)
    {
        /* 刷新小时标记 */
        osd_refreshHour(&u32Flg, u32MaskFlag);
        /* 刷新AMPM标记 */
        osd_refreshAMPM(&u32Flg, u32MaskFlag);
    }

    if (dtOld.minute != now.minute)
    {
        /* 刷新分钟标记 */
        osd_refreshMinute(&u32Flg, u32MaskFlag);
    }

    if (dtOld.second != now.second)
    {
        /* 刷新秒标记 */
        osd_refreshSecond(&u32Flg, u32MaskFlag);
    }

    memcpy(&dtOld, &now, sizeof(DATE_TIME));

    *pFlg = u32Flg;

    return;
}

/**
 * @function    osd_getWHFormat
 * @brief       获取图像宽高信息
 * @param[in]   UINT32 u32Res 分辨率信息
 * @param[out]  UINT32 *puiW, UINT32 *puiH 图像宽高
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 osd_getWHFormat(UINT32 u32Res, UINT32 *puiW, UINT32 *puiH)
{
    if ((NULL == puiW) || (NULL == puiH))
    {
        OSD_LOGE("!!!\n");
        return SAL_FAIL;
    }

    switch (u32Res)
    {
        case QQCIF_FORMAT:
        {
            *puiW = 96;
            *puiH = 80;
            break;
        }
        case QNCIF_FORMAT:
        {
            *puiW = 160;
            *puiH = 120;
            break;
        }
        case QCIF_FORMAT:
        {
            *puiW = 176;
            *puiH = 144;
            break;
        }
        case CIF_FORMAT:
        {
            *puiW = 352;
            *puiH = 288;
            break;
        }
        case VGA_FORMAT:
        {
            *puiW = 640;
            *puiH = 480;
            break;
        }
        case FCIF_FORMAT:
        {
            *puiW = 704;
            *puiH = 576;
            break;
        }
        case D1_FORMAT:
        {
            *puiW = 720;
            *puiH = 576;
            break;
        }
        case HD720p_FORMAT:
        {
            *puiW = 1280;
            *puiH = 720;
            break;
        }
        case XVGA_FORMAT:
        {
            *puiW = 1280;
            *puiH = 960;
            break;
        }
        case UXGA_FORMAT:
        {
            *puiW = 1600;
            *puiH = 1200;
            break;
        }

        case RES_1440_900_FORMAT:
        {
            *puiW = 1440;
            *puiH = 900;
            break;
        }

        case HD1080p_FORMAT:
        {
            *puiW = 1920;
            *puiH = 1080;
            break;
        }
        case RES_1080_1920_FORMAT:
        {
            *puiW = 1080;
            *puiH = 1920;
            break;
        }
        case RES_720_1280_FORMAT:
        {
            *puiW = 720;
            *puiH = 1280;
            break;
        }
        case RES_360_640_FORMAT:
        {
            *puiW = 360;
            *puiH = 640;
            break;
        }
        case RES_1080_720_FORMAT:
        {
            *puiW = 1080;
            *puiH = 720;
            break;
        }

        case SXGA_FORMAT:
        {
            *puiW = 1280;
            *puiH = 1024;
            break;
        }

        case XGA_FORMAT:
        {
            *puiW = 1024;
            *puiH = 768;
            break;
        }

        case SVGA_FORMAT:
        {
            *puiW = 800;
            *puiH = 600;
            break;
        }

        case RES_1366_768_FORMAT:
        {
            *puiW = 1366;
            *puiH = 768;
            break;
        }

        default:
        {
            OSD_LOGE("Not Support Res, %x !!!\n", u32Res);
            *puiW = 1280;
            *puiH = 720;
            break;
        }
    }

    /* OSD_LOGI("w:%d,h:%d\n",*puiW,*puiH); */
    return SAL_SOK;
}

/**
 * @function    osd_drv_refreshTag
 * @brief       根据标记位，分别刷新OSD的每一位{年月日星期时分秒等}
 * @param[in]   UINT32 flg 刷新标记
 * @param[in]   UINT32 u32ChanOffset 通道偏移
 * @param[in]   UINT32 u32Chan 通道号
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
void osd_drv_refreshTag(UINT32 flg, UINT32 u32Chan, UINT32 u32ChanOffset)
{
    /* 刷新年份 */
    osd_refreshYearTag(flg, u32ChanOffset, u32Chan);

    /* 刷新月份 */
    osd_refreshMonthTag(flg, u32ChanOffset, u32Chan);

    /* 刷新日期 */
    osd_refreshDayTag(flg, u32ChanOffset, u32Chan);

    /* 刷新星期 */
    osd_refreshWeekTag(flg, u32ChanOffset, u32Chan);

    /* 刷新小时 */
    osd_refreshHourTag(flg, u32ChanOffset, u32Chan);

    /* 刷新分钟 */
    osd_refreshMinuteTag(flg, u32ChanOffset, u32Chan);

    /* 刷新秒 */
    osd_refreshSecondTag(flg, u32ChanOffset, u32Chan);

    /* 刷新AMPM */
    osd_refreshAMPMTag(flg, u32ChanOffset, u32Chan);
}

/**
 * @function    osd_drv_getColor
 * @brief       计算OSD字体的颜色
 * @param[in]   OSD_LINE_S *pstLine OSD行参数指针
 * @param[in]   PUINT8 pLumaAddr 图像Y分量地址
 * @param[in]   UINT32 u32ImgW, UINT32 u32ImgH 图像宽高
 * @param[in]   UINT32 u32Stride 跨度
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
void osd_drv_getColor(OSD_LINE_S *pstLine, PUINT8 pLumaAddr, UINT32 u32ImgW, UINT32 u32ImgH, UINT32 u32Stride)
{
    OSD_CHAR_S *pstChar = NULL;
    UINT32 x = 0;
    UINT32 y = 0;
    UINT32 w = 0;
    UINT32 h = 0;
    UINT32 u32Total = 0;
    UINT32 u32Avg = 0;
    INT32 i = 0;
    INT32 j = 0;
    INT32 k = 0;

    if ((NULL == pstLine) || (NULL == pLumaAddr))
    {
        OSD_LOGE("null ptr!!pstLine %p pLumaAddr %p\n", pstLine, pLumaAddr);
        return;
    }

    /* 编码通道更新分辨率，需要加锁 同步处理 */
    /* pthread_mutex_lock(&pEncCtrl->muProcessOsd); */
    for (i = 0; i < pstLine->validCharCnt; i++)
    {
        pstChar = pstLine->charData + i;
        w = (pstChar->size & 0xff);
        h = ((pstChar->size >> 8) & 0xff);
        y = (pstChar->pos >> 16);
        x = (pstChar->pos & 0xffff);

        if (u32ImgW < 704)
        {
            x /= 2;
            w /= 2;
        }

        if (u32ImgH < 480)
        {
            y /= 2;
            h /= 2;
        }

        if (((x + w) > u32ImgW) || ((y + h) > u32ImgH))                 /* 防止切换制式时出现坐标超出的情况 */
        {
            OSD_LOGW("GetOsdColor invalid Date!! x=%d,y=%d,w=%d,h=%d u32ImgW=%d u32ImgH=%d\n", x, y, w, h, u32ImgW, u32ImgH);
            continue;
        }

        u32Total = 0;

        for (j = 0; j < h; j++)
        {
            for (k = 0; k < w; k++)
            {
                u32Total += pLumaAddr[(y + j) * u32Stride + x + k];
            }
        }

        /*避免除数为0*/
        if (h == 0)
        {
            continue;
        }

        if (w == 0)
        {
            continue;
        }

        u32Avg = u32Total / h / w;

        if (u32Avg > 128)
        {
            if (pstChar->color != OSD_COLOR_BLACK)
            {
                pstChar->color = OSD_COLOR_BLACK;
                pstChar->bChange = 1;
            }
        }
        else
        {
            if (pstChar->color != OSD_COLOR_WHITE)
            {
                pstChar->color = OSD_COLOR_WHITE;
                pstChar->bChange = 1;
            }
        }
    }

    /* pthread_mutex_unlock(&pEncCtrl->muProcessOsd); */
}

/**
 * @function    osd_drv_regionCreate
 * @brief       申请OSD RGN通道
 * @param[in]   UINT32 u32StartIdx rgn 起始id
 * @param[in]   UINT32 u32RegionNum rgn个数
 * @param[out]  OSD_RGN_S *pstRegion osd句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_regionCreate(UINT32 u32StartIdx, UINT32 u32RegionNum, OSD_RGN_S *pstRegion)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT32 idx = u32StartIdx;
    UINT32 u32RgnSize = 0;
    PUINT8 pBuf = NULL;

    u32RgnSize = osd_hal_getMemSize();

    if (u32RgnSize)
    {
        for (i = 0; i < u32RegionNum; i++)
        {
            pBuf = SAL_memMalloc(u32RgnSize, "osd", "drv");
            if (NULL == pBuf)
            {
                OSD_LOGE("alloc mem fail\n");
                return SAL_FAIL;
            }

            s32Ret = osd_hal_create(u32StartIdx, idx, &pstRegion->pstRgnHdl[i], pBuf);
            if (SAL_SOK != s32Ret)
            {
                OSD_LOGE("osd_hal_create fail\n");
                return SAL_FAIL;
            }

            idx++;
        }
    }

    pstRegion->bShow = SAL_FALSE;

    return SAL_SOK;
}

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
INT32 osd_drv_Init(MEMORY_BUFF_S *pHzLib, MEMORY_BUFF_S *pAscLib, void *pEncHzLib, void *pEncAscLib, UINT32 isLattice)
{
    DSPINITPARA *pDspInitParm = SystemPrm_getDspInitPara();
    FREETYPE_FACE_PRM_S stFace = {0};

    if ((NULL == pEncHzLib) || (NULL == pEncAscLib))
    {
        OSD_LOGE("invalid font[pEncHzLib:%p, pEncAscLib:%p]\n", pEncHzLib, pEncAscLib);
        return SAL_FAIL;
    }

    if (SAL_FALSE == isLattice)
    {
        if (pDspInitParm->stFontLibInfo.enEncFormat == ENC_FMT_GB2312) /* 中文编码使用alipuhui.ttf字库 */
        {
            if ((NULL == pHzLib->pVirAddr) || (0 == pHzLib->uiSize))
            {
                OSD_LOGE("invalid font[addr:%p, size:%u]\n", pHzLib->pVirAddr, pHzLib->uiSize);
                return SAL_FAIL;
            }

            stFace.stMemory.pvAddr = (VOID *)pHzLib->pVirAddr;
            stFace.stMemory.u32Size = pHzLib->uiSize;
        }
        else /* 其他语言使用arial.ttf字库 */
        {
            if ((NULL == pAscLib->pVirAddr) || (0 == pAscLib->uiSize))
            {
                OSD_LOGE("invalid font[addr:%p, size:%u]\n", pAscLib->pVirAddr, pAscLib->uiSize);
                return SAL_FAIL;
            }

            stFace.stMemory.pvAddr = (VOID *)pAscLib->pVirAddr;
            stFace.stMemory.u32Size = pAscLib->uiSize;
        }

        stFace.enLoadType = FREETYPE_FACE_LOAD_TYPE_MEMORY;
        if (SAL_SOK != Freetype_SetFace(&stFace))
        {
            OSD_LOGE("freetype set face fail\n");
            return SAL_FAIL;
        }
    }

    /* TODO: 编码OSD字库先采用HZK16和ASC16，后续可改成freetype */
    fontAscLib = (UINT32 *)pEncAscLib;
    fontHzLib = (UINT32 *)pEncHzLib;

    return SAL_SOK;
}

/**
 * @function    osd_drv_destroy
 * @brief       销毁指定的REGION，注意，调用此函数需要加锁pthread_mutex_lock(&muProcessOsd[u32Chan])保护
 * @param[in]   UINT32 u32Chan plt层编码通道号,上层无需校验
 * @param[in]   UINT32 u32LineId 行号
 * @param[in]   OSD_RGN_S *pstRegion osd句柄
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_destroy(UINT32 u32Chan, UINT32 u32LineId, OSD_RGN_S *pstRegion)
{
    INT32 s32Ret = SAL_SOK;

    if ((NULL == pstRegion) || (MAX_OVERLAY_REGION_PER_CHAN <= u32LineId))
    {
        OSD_LOGE("u32Chan %d,u32LineId %d\n", u32Chan, u32LineId);
        return SAL_FAIL;
    }

    s32Ret = osd_hal_destroy(u32Chan, pstRegion->pstRgnHdl[u32LineId]);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("HI MPI RGN Destroy is error 0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    pstRegion->bShow = SAL_FALSE;

    OSD_LOGI("u32LineId %d\n", u32LineId);
    return SAL_SOK;
}

/**
 * @function    osd_drv_paramCheck
 * @brief       osd参数校验
 * @param[in]   UINT32 u32ImgW, UINT32 u32ImgH 图像宽高
 * @param[in]   OSD_LINE_S *pstLine OSD行参数指针
 * @param[in]   UINT32 u32Mode 通道类型
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_paramCheck(UINT32 u32ImgW, UINT32 u32ImgH, OSD_LINE_S *pstLine, UINT32 u32Mode)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 x = 0;
    UINT32 y = 0;
    UINT32 w = 0;
    UINT32 h = 0;
    UINT32 u32StartX = 0;
    UINT32 u32ScaleH = 0;
    UINT32 u32ScaleV = 0;
    OSD_CHAR_S *pstChar = NULL;

    pstLine->validCharCnt = 0;
    pstLine->x = pstLine->y = pstLine->w = pstLine->h = 0;

    if (u32Mode == OSD_DISP_MODE)
    {
        u32ScaleH = 2;
        u32ScaleV = 2;
    }
    else
    {
        u32ScaleH = 1;
        u32ScaleV = 1;
    }

    for (j = 0; j < pstLine->charCnt; j++)
    {
        pstChar = pstLine->charData + j;
        x = (pstChar->top) & 0xffff;
        y = pstChar->top >> 16;

        x = SAL_align(x, 8);
        y = SAL_align(y, 8);
        w = (pstChar->size >> 16) & 0xff;
        h = pstChar->size >> 24;

        /* 第一个字符的左上角是该行所有字符区域的左上角 */
        if (j == 0)
        {
            pstLine->x = x;
            pstLine->y = y;
        }

        /* y为绝对位置，x、w、h随编码分辨率变化 */
        x <<= u32ScaleH;
        w <<= u32ScaleH;
        h <<= u32ScaleV;

        if (j == 0)
        {
            u32StartX = x;
        }

        x = pstLine->x + x - u32StartX;


        if ((x + w > u32ImgW) || (y + h > u32ImgH)) /* */
        {
            return SAL_FAIL;
        }

        /* 首末字符之间的宽度是该行所有字符区域的宽度 */
        pstLine->w = x + w - pstLine->x;
        if (h > pstLine->h)
        {
            pstLine->h = h;
        }

        /* 更新字符在当前分辨率下的位置 */
        pstChar->pos = y << 16 | x;
        pstChar->size = (pstChar->size & 0xffff0000) | (h << 8) | w;
        pstChar->bChange = SAL_TRUE;
        /* OSD_LOGI("x,y,w,h\n",x,y,w,h); */
        pstChar->color = OSD_COLOR_BLACK;
        pstLine->validCharCnt++;
    }

    OSD_LOGI("i %d, x %d, y %d\n", i, pstLine->x, pstLine->y);

    if (u32Mode == OSD_ENC_MODE)
    {
        memset(pstLine->p4CifCharImg, 0x00, OSD_4CIF_IMG_SIZE);
        memset(pstLine->pCifCharImg, 0x00, OSD_CIF_IMG_SIZE);
        memset(pstLine->pQCifCharImg, 0x00, OSD_QCIF_IMG_SIZE);
    }
    else
    {
        memset(pstLine->p4CifCharImg, 0x00, OSD_DISP_IMG_SIZE);
    }

    return SAL_SOK;
}

/**
 * @function    osd_drv_draw
 * @brief       OSD点阵描绘
 * @param[in]   OSD_LINE_S *pstLine OSD行参数指针
 * @param[in]   UINT32 u32Mode 通道类型
 * @param[out]  BOOL *pFlgChange 是否更新字符
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_draw(OSD_LINE_S *pstLine, UINT32 u32Mode, BOOL *pFlgChange)
{
    UINT32 j = 0;
    UINT32 x = 0;
    UINT32 y = 0;
    UINT32 w = 0;
    UINT32 h = 0;
    UINT32 pitch = 0;
    UINT32 dstY = 0;
    UINT32 code = 0;
    OSD_CHAR_S *pChar = NULL;
    PUINT8 pDst = NULL;
    PUINT8 pDst1 = NULL;;

    if (NULL == pstLine)
    {
        OSD_LOGI("inv prm");
        return SAL_FAIL;
    }

    for (j = 0; j < pstLine->validCharCnt; j++)
    {
        pChar = pstLine->charData + j;

        if (u32Mode == OSD_ENC_MODE)
        {

        }

        if (!pChar->bChange)
        {
            continue;
        }

        if (pFlgChange)
        {
            *pFlgChange = SAL_TRUE;
        }

        if (u32Mode == OSD_ENC_MODE)
        {
            w = (pChar->size & 0xff);
            h = ((pChar->size >> 8) & 0xff);
            y = (pChar->pos >> 16);
            x = (pChar->pos & 0xffff);
            pitch = pstLine->w;
            dstY = 0;

            if ((PUINT8)NULL == pstLine->p4CifCharImg
                || (PUINT8)NULL == pstLine->pCifCharImg
                || (PUINT8)NULL == pChar->pFont)
            {
                OSD_LOGE("y %d\n", y);
                return SAL_SOK;
            }

            pDst = (PUINT8)(pstLine->p4CifCharImg + x * 2 + dstY * 704 * 2);
            DrawCharH2V2(pChar->pFont, (PUINT8)pDst, w, h, pitch, pChar->color, OSD_BACK_COLOR);
            /* DrawCharH2V2(ascFont, (PUINT8)pDst, w, h, pitch, pChar->color); */
            x /= 2;
            dstY /= 2;
            pitch /= 2;
            w /= 2;
            h /= 2;
            pDst = (PUINT8)(pstLine->pCifCharImg + x * 2 + dstY * 352 * 2);
            DrawCharH1V1(pChar->pFont, (PUINT8)pDst, w, h, pitch, pChar->color, OSD_BACK_COLOR);

            code = osd_getCharFontCode(pChar->pFont);
            pDst1 = (PUINT8)(pstLine->pQCifCharImg + x + dstY / 2 * 352);

            if (code == 58) /* 冒号单独画，不然下面那个点会丢失 */
            {
                DrawCharH0V0Colon((PUINT8)pDst, (PUINT8)pDst1, w, h, pitch, pChar->color);
            }
            else
            {
                DrawCharH0V0((PUINT8)pDst, (PUINT8)pDst1, w, h, pitch, pChar->color, OSD_BACK_COLOR);
            }
        }
        else if (u32Mode == OSD_DISP_MODE)
        {
            w = (pChar->size & 0xff);
            h = ((pChar->size >> 8) & 0xff);
            y = (pChar->pos >> 16);
            x = (pChar->pos & 0xffff);
            pitch = pstLine->w;
            pDst = (PUINT8)(pstLine->p4CifCharImg + (x - pstLine->x) * 2);
            pstLine->pDrawFunc(pChar->pFont, (PUINT8)pDst, w, h, pitch, pChar->color, OSD_BACK_COLOR);
        }

        pChar->bChange = SAL_FALSE;
    }

    return SAL_SOK;
}

/**
 * @function    osd_drv_process
 * @brief       OSD叠加
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 u32Resolution 分辨率
 * @param[in]   UINT32 u32LineId 行号
 * @param[in]   OSD_RGN_S *pstRegion OSD句柄
 * @param[in]   OSD_LINE_S *pstLine OSD行参数指针
 * @param[in]   UINT32 bTranslucent 半透明标记
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_process(UINT32 u32Chan, UINT32 u32Resolution, UINT32 u32LineId, OSD_RGN_S *pstRegion, OSD_LINE_S *pstLine, UINT32 bTranslucent)
{
    UINT32 wDiv = 1;
    UINT32 hDiv = 1;
    UINT32 wMul = 1;
    UINT32 hMul = 1;
    UINT32 xDiv = 1;
    UINT32 yDiv = 1;
    UINT32 xMul = 1;
    UINT32 yMul = 1;
    PUINT8 pCharImgBuf = NULL;
    BOOL bScalingOsdBuf = SAL_FALSE;
    UINT32 uiW = 0;
    UINT32 uiH = 0;
    float xRatio = 0.0;
    float yRatio = 0.0;
    INT32 dstW = 0;
    INT32 dstH = 0;
    INT32 dstX = 0;
    INT32 dstY = 0;
    INT32 s32Ret = SAL_FAIL;
    UINT64 u64PhyAddr = 0;

    if ((NULL == pstRegion) || (NULL == pstLine) || (MAX_OVERLAY_REGION_PER_CHAN <= u32LineId))
    {
        OSD_LOGI("prm err,u32LineId %d\n", u32LineId);
        return SAL_FAIL;
    }

    /* 需要获取编码分辨率 */
    if (u32Resolution == HD1080p_FORMAT || u32Resolution == UXGA_FORMAT
        || u32Resolution == SXGA_FORMAT || u32Resolution == RES_1440_900_FORMAT)
    {
        if (pstLine->osdScaleH == 0 && pstLine->osdScaleV == 0)
        {
            wDiv = 4;
            hDiv = 4;
        }
        else if (pstLine->osdScaleH == 1 && pstLine->osdScaleV == 1)
        {
            wDiv = 1;
            hDiv = 1;
        }

        pCharImgBuf = (PUINT8)((PhysAddr)pstLine->p4CifCharImg + pstLine->x * 2);
        u64PhyAddr = (PhysAddr)pstLine->p4CifCharImgP + pstLine->x * 2;
        bScalingOsdBuf = SAL_FALSE;
    }
    else if (u32Resolution == XGA_FORMAT || u32Resolution == HD720p_FORMAT)
    {
        if (pstLine->osdScaleH == 0 && pstLine->osdScaleV == 0)
        {
            wDiv = 4;
            hDiv = 4;
        }
        else if (pstLine->osdScaleH == 1 && pstLine->osdScaleV == 1)
        {
            wDiv = 2;
            hDiv = 2;
        }

        pCharImgBuf = (PUINT8)((PhysAddr)pstLine->pCifCharImg + pstLine->x);
        u64PhyAddr = (PhysAddr)pstLine->pCifCharImgP + pstLine->x;
        bScalingOsdBuf = SAL_FALSE;
    }
    else
    {
        if (pstLine->osdScaleH == 0 && pstLine->osdScaleV == 0)
        {
            wDiv = 4;
            hDiv = 4;
        }
        else if (pstLine->osdScaleH == 1 && pstLine->osdScaleV == 1)
        {
            wDiv = 2;
            hDiv = 2;
        }

        pCharImgBuf = (PUINT8)((PhysAddr)pstLine->pCifCharImg + pstLine->x);
        u64PhyAddr = (PhysAddr)pstLine->pCifCharImgP + pstLine->x;
        bScalingOsdBuf = SAL_FALSE;
    }

    dstW = (pstLine->w * wMul / wDiv);
    dstH = (pstLine->h * hMul / hDiv);
    dstX = (pstLine->x * xMul / xDiv);
    dstY = (pstLine->y * yMul / yDiv);

    osd_getWHFormat(u32Resolution, &uiW, &uiH);
    xRatio = (uiW * 1.0) / (MAX_IMG_W * 1.0);
    yRatio = (uiH * 1.0) / (MAX_IMG_H * 1.0);
    dstX = dstX * 1.0 * xRatio;
    dstY = dstY * 1.0 * yRatio;

    dstW = MAX(dstW, RGN_MIN_W);
    dstH = MAX(dstH, RGN_MIN_H);

    if (dstX + dstW > uiW)
    {
        dstX -= (dstX + dstW - uiW);
    }

    if (dstY + dstH > uiH)
    {
        dstY -= (dstY + dstH - uiH);
    }

    dstW = dstW & (~3);
    dstH = dstH & (~3);
    dstX = dstX > 0 ? (dstX & (~3)) : 0;
    dstY = dstY > 0 ? (dstY & (~3)) : 0;

    s32Ret = osd_hal_process(u32Chan,
                             pstRegion->pstRgnHdl[u32LineId],
                             pCharImgBuf,
                             u64PhyAddr,
                             dstW,
                             dstH,
                             dstX,
                             dstY,
                             OSD_BACK_COLOR,
                             bTranslucent);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("!!!\n");
        return SAL_FAIL;
    }

    if (u32LineId > 0)
    {
        OSD_LOGI("u32Chan %d,bScalingOsdBuf %d\n", u32Chan, bScalingOsdBuf);
    }

    return SAL_SOK;
}

/**
 * @function    osd_drv_clearTag
 * @brief       清除osd tag标志
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 u32ChnOffset 通道偏移
 * @param[out]  None
 * @return      void
 */
void osd_drv_clearTag(UINT32 u32Chan, UINT32 u32ChnOffset)
{
    UINT32 i = 0;

    /* 清掉所有使用的特殊字符 */
    for (i = 0; i < OSD_TAG_CNT; i++)
    {
        g_stOsdTagParam[i].flgUsed &= ~(1 << (u32Chan + u32ChnOffset));
    }

    return;
}

/**
 * @function    osd_drv_charInit
 * @brief       初始化OSD字符
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 u32ChanOffset 通道偏移
 * @param[in]   PUINT8 pLib 字库指针
 * @param[in]   OSD_LINE_S *pstLine OSD行参数指针
 * @param[in]   UINT32 u32Code 要处理的字符
 * @param[in]   UINT32 *pLeft 字符水平位置
 * @param[in]   UINT32 u32Top 字符高度
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_charInit(UINT32 u32Chan, UINT32 u32ChanOffset, PUINT8 pLib, OSD_LINE_S *pstLine, UINT32 u32Code, UINT32 *pLeft, UINT32 u32Top)
{
    UINT32 u32CharCnt = 0;
    UINT32 u32CharType = 0;
    UINT32 u32TagIdx = 0;
    UINT32 u32CharW = 0;
    UINT32 u32CharH = 0;
    PUINT8 pFontAddr = NULL;
    OSD_CHAR_S *pstChar = NULL;

    /*pLib当前没用，不用check*/
    if ((NULL == pstLine) || (NULL == pLeft))
    {
        OSD_LOGE("prm is NULL\n");
        return SAL_FAIL;
    }

    u32CharType = osd_getCharType(u32Code);

    if (u32CharType == OSD_CHAR_TYPE_TAG)
    {
        u32TagIdx = u32Code - OSD_TAG_CODE_BASE;
        u32CharCnt = g_stOsdTagParam[u32TagIdx].charCnt;
        u32CharW = g_stOsdTagParam[u32TagIdx].charWidth;
        u32CharH = 16;  
        pstLine->bLineChange = SAL_TRUE;
        if (g_stOsdTagParam[u32TagIdx].flgUsed & (1 << (u32Chan + u32ChanOffset)))
        {
            *pLeft += u32CharW * u32CharCnt;
            return SAL_SOK;
        }
    }
    else
    {
        /*
        if (u32Code == ' ')
        {
            *pLeft += OSD_CHAR_W_ASC;
            return SAL_SOK;
        }
        */

        u32CharCnt = 1;
        #if 0 /* 待修改 */
        if (pDspInitP->NetOsdType & OSD_TYPE_DYNAMIC)
        {
            if (u32CharType == OSD_CHAR_TYPE_FONT8)
            {
                u32CharH = 16;
                u32CharW = 8;
                pFontAddr = pLib + (u32Code - FONT8_CODE_BASE) * 16;
            }
            else if (u32CharType == OSD_CHAR_TYPE_FONT16)
            {
                u32CharH = 16;
                u32CharW = 16;
                pFontAddr = pLib + (u32Code - FONT16_CODE_BASE) * 32;
            }
            else if (u32CharType == OSD_CHAR_TYPE_ASC)
            {
                u32CharW = OSD_CHAR_W_ASC;
                u32CharH = 16;
                pFontAddr = (PUINT8)dspParam.fontAscLib + u32Code * 16;
            }
        }
        else
        #endif
        {
            if (u32CharType == OSD_CHAR_TYPE_ASC)
            {
                u32CharW = OSD_CHAR_W_ASC;
                u32CharH = 16;
                pFontAddr = (PUINT8)fontAscLib + u32Code * 16;
            }
            else
            {
                u32CharH = 16;
                u32CharW = OSD_CHAR_W_HZ;
                /* if (!(FONT_HZ_SIMPL_LEN >= GET_HZ_OFFSET(u32Code),GET_HZ_OFFSET(u32Code))) */
                if (!(FONT_HZ_SIMPL_LEN >= GET_HZ_OFFSET(u32Code)))
                {
                    OSD_LOGE("\n");
                    return SAL_FAIL;
                }

                pFontAddr = (PUINT8)fontHzLib + GET_HZ_OFFSET(u32Code);
            }
        }
    }

    if ((pstLine->charCnt + u32CharCnt) > MAX_CHAR_PER_LINE)
    {
        OSD_LOGE("pLib %p\n", pLib);
        return SAL_FAIL;
    }

    if ((*pLeft + u32CharW) > MAX_IMG_W)
    {
        OSD_LOGE("\n");
        return SAL_FAIL;
    }

    pstChar = &pstLine->charData[pstLine->charCnt];
    if (u32CharType == OSD_CHAR_TYPE_TAG)
    { 
        g_stOsdTagParam[u32TagIdx].flgUsed |= (1 << (u32Chan + u32ChanOffset));
        g_stOsdTagParam[u32TagIdx].pFirstChar[u32Chan + u32ChanOffset] = pstChar;
    }

    while (u32CharCnt--)
    {
        pstChar->top = (u32Top << 16) | ((*pLeft));
        pstChar->size = (u32CharH << 24) + (u32CharW << 16);
        pstChar->pFont = pFontAddr;
        pstChar->color = OSD_COLOR_BLACK;
        pstChar->bChange = SAL_TRUE;
        pstChar->charType = u32CharType;
        *pLeft += u32CharW;
        pstLine->charCnt++;
        pstChar++;
    }

    return SAL_SOK;
}

/**
 * @function    osd_drv_setLineParam
 * @brief       设置OSD行参数，eg，缩放比例等
 * @param[in]   OSD_LINE_S *pstLine OSD行参数指针
 * @param[in]   UINT32 u32ScaleV 水平缩放系数
 * @param[in]   UINT32 u32ScaleH 垂直缩放系数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_drv_setLineParam(OSD_LINE_S *pstLine, UINT32 u32ScaleV, UINT32 u32ScaleH)
{
    if (NULL == pstLine)
    {
        OSD_LOGE("prm is NULL\n");
        return SAL_FAIL;
    }

    pstLine->charCnt = 0;
    pstLine->validCharCnt = 0;

    if (u32ScaleH == 0 && u32ScaleV == 0)
    {
        pstLine->pDrawFunc = DrawCharH1V1;
    }
    else if (u32ScaleH == 0 && u32ScaleV == 1)
    {
        pstLine->pDrawFunc = DrawCharH1V2;
    }
    else if (u32ScaleH == 1 && u32ScaleV == 1)
    {
        pstLine->pDrawFunc = DrawCharH2V2;
    }
    else if (u32ScaleH == 2 && u32ScaleV == 2)
    {
        pstLine->pDrawFunc = DrawCharH4V4;
    }
    else
    {
        pstLine->pDrawFunc = DrawCharH2V2;
    }

    pstLine->osdScaleH = 1;
    pstLine->osdScaleV = 1;
    pstLine->bLineChange = SAL_FALSE;

    return SAL_SOK;
}

/**
 * @function    osd_drv_clearDisp
 * @brief       清除预览OSD、LOGO等
 * @param[in]   UINT32 u32EncChan 通道号
 * @param[in]   UINT32 u32DispChan 通道号
 * @param[in]   OSD_LINE_S *pstLine
 * @param[in]   PREV_OSD_TYPE_E u32Mode
 * @param[out]  None
 * @return      void
 */
void osd_drv_clearDisp(UINT32 u32EncChan, UINT32 u32DispChan, OSD_LINE_S *pstLine, PREV_OSD_TYPE_E u32Mode)
{
    /*TODO:osd_hal_clearDisp*/
    return;
}

/**
 * @function    osd_drv_disp
 * @brief       处理预览OSD、LOGO等
 * @param[in]   UINT32 u32EncChan 通道号
 * @param[in]   UINT32 u32DispChan
 * @param[in]   OSD_LINE_S *pstLine
 * @param[in]   PREV_OSD_TYPE_E u32Mode
 * @param[in]   void *pRect
 * @param[in]   PUINT8 pBuf
 * @param[out]  None
 * @return      void
 */
void osd_drv_disp(UINT32 u32EncChan, UINT32 u32DispChan, OSD_LINE_S *pstLine, PREV_OSD_TYPE_E u32Mode, void *pRect, PUINT8 pBuf)
{
    /*TODO:osd_hal_disp*/

    return;
}

