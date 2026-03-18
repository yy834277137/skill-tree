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
   2.Date        : 2021/07/09
     Author      : liuxianying
     Modification: 组件开发，整理接口
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <sal.h>
#include "osd_drv_api.h"
#include "drawChar.h"
#include "freetype.h"
#include "freetype_func.h"
#include "color_space.h"
#include "platform_hal.h"
#include "system_prm_api.h"


#define OSD_MEM_NAME "osd_mem"
#define OSD_FUNC_NAME "osd_func"

static OSD_BLOCK_S *g_pastOsdBlock[OSD_BLOCK_NUM_MAX] = {NULL};
static OSD_VAR_BLOCKS_S g_stOsdVarBlocks = {0};
#ifdef DSP_ISA
static const char g_stringLanguageMainDux[3][2][16]={
                                                     {"UDF","UDF"},
                                                     {"主", "侧"},
                                                     {"MAIN", "AUX"},
};
#endif
extern PUINT8 osd_drv_getAscHzFontAddr(UINT8 *pu8Char, UINT32 *pu32CharLen);



/*******************************************************************************
* 函数名  : osd_Hzk_FillLattice
* 描  述  : 根据Hzk字库及字符串填充点阵，从pstRegion->u32Width处开始
* 输  入  : char *szString : 输入字符串，国标编码
          OSD_REGION_S *pstRegion : 点阵的区域
          UINT32 u32Size : 字体大小
          ENCODING_FORMAT_E enEncFormat : 编码类型
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
static INT32 osd_Hzk_FillLattice(char *szString, OSD_REGION_S *pstRegion, UINT32 u32Size, ENCODING_FORMAT_E enEncFormat)
{
    UINT8 *pu8FontAddr = NULL;
    UINT8 *pu8Buff = NULL;
    UINT32 u32Stride = 0;
    UINT32 u32CharLen = 0;
    UINT32 i = 0;

    if ((NULL == szString) || (NULL == pstRegion))
    {
        OSD_LOGE("hz fill lattice null pointer:%p %p\n", szString, pstRegion);
        return SAL_FAIL;
    }

    SAL_UNUSED(u32Size);

    pu8Buff = pstRegion->stAddr.pu8VirAddr;
    u32Stride = pstRegion->u32Stride;
    pstRegion->u32Width = 0;

    while (*szString)
    {
        pu8FontAddr = osd_drv_getAscHzFontAddr((UINT8 *)szString, &u32CharLen);
        if (NULL == pu8FontAddr)
        {
            OSD_LOGE("hz get font add fail\n");
            return SAL_FAIL;
        }

        /* 中文字库一个字符大小为16*16，ASCII为16*8 */
        for (i = 0; i < 16; i++)
        {
            *(pu8Buff + u32Stride * i) = *pu8FontAddr++;
            if (2 == u32CharLen)
            {
                *(pu8Buff + u32Stride * i + 1) = *pu8FontAddr++;
            }
        }

        szString += u32CharLen;
        pu8Buff += u32CharLen;
        pstRegion->u32Width += (u32CharLen << 3);       /* width单位为像素点 */
    }

    pstRegion->u32Height = 16;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_freetype_FillLattice
* 描  述  : 通过Freetype填充点阵
* 输  入  : char *szString : 输入字符串，国标编码
          OSD_REGION_S *pstRegion : 点阵的区域
          UINT32 u32Size : 字体大小
          ENCODING_FORMAT_E enEncFormat : 编码类型
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_freetype_FillLattice(char *szString, OSD_REGION_S *pstRegion, UINT32 u32Size, ENCODING_FORMAT_E enEncFormat)
{
    INT32 s32Ret = SAL_SOK;
    FREETYPE_BLOCK_S stBlock;

    if ((NULL == szString) || (NULL == pstRegion))
    {
        OSD_LOGE("freetype fill lattice null pointer:%p %p\n", szString, pstRegion);
        return SAL_FAIL;
    }

    stBlock.pu8Buffer = pstRegion->stAddr.pu8VirAddr;
    stBlock.u32Size   = pstRegion->u32Size;

    s32Ret = Freetype_DrawByString(&stBlock, szString, u32Size, enEncFormat);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("freetype fill lattice fail! string:%s\n",szString);
        return SAL_FAIL;
    }

    pstRegion->u32Width = stBlock.u32Width;
    pstRegion->u32Height = stBlock.u32Height;
    pstRegion->u32Stride = stBlock.u32Stride;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_func_FillFont
* 描  述  : 根据点阵填充字体
* 输  入  : OSD_REGION_S *pstLat : 点阵区域
          UINT32 u32LatSize : 点阵大小
          OSD_REGION_S *pstFont : 字体区域
          UINT32 u32FontSize : 字体大小
          UINT16 u16Color : 字体颜色
          UINT16 u16BgColor : 字体北京颜色
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_FillFont(OSD_REGION_S *pstLat, UINT32 u32LatSize, OSD_REGION_S *pstFont, UINT32 u32FontSize, UINT16 u16Color, UINT16 u16BgColor)
{
    UINT32 u32Scale = 0;
    UINT32 u32Bytes = 0;
    UINT32 u32Left = 0;
    UINT8 u8Tmp = 0;
    UINT16 u16Tmp = 0;
    UINT16 *pu16FontBuff = NULL;
    UINT32 i = 0, j = 0, k = 0, m = 0, n = 0;

    if ((NULL == pstLat) || (NULL == pstFont))
    {
        OSD_LOGE("lattice[%p] or font[%p] is null\n", pstLat, pstFont);
        return SAL_FAIL;
    }

    u32Scale = u32FontSize / u32LatSize;
    if ((0 != u32FontSize % u32LatSize) || (0 == u32Scale))
    {
        OSD_LOGE("invalid size:font[%u], lat[%u]\n", u32FontSize, u32LatSize);
        return SAL_FAIL;
    }

    pstFont->u32Width = SAL_align(pstLat->u32Width, 2) * u32Scale;
    pstFont->u32Height = pstLat->u32Height * u32Scale;
    pstFont->u32Stride = pstFont->u32Width * 2;
    if (pstFont->u32Stride * pstFont->u32Height > pstFont->u32Size)
    {
        OSD_LOGE("osd font size:%u is too smalll, stride:%u height:%u\n", pstFont->u32Size, pstFont->u32Stride, pstFont->u32Height);
        return SAL_FAIL;
    }

    u32Bytes = pstLat->u32Width >> 3;
    for (i = 0; i < pstLat->u32Height; i++)
    {
        for (j = 0; j < u32Bytes; j++)
        {
            u8Tmp = *(pstLat->stAddr.pu8VirAddr + (i * pstLat->u32Stride) + j);
            for (k = 0; k < 8; k++)
            {
                /* 乘2是RGB1555一个像素点2个字节 */
                pu16FontBuff = (UINT16 *)(pstFont->stAddr.pu8VirAddr + (i * u32Scale * pstFont->u32Stride) + ((j * 8 + k) * u32Scale) * 2);
                u16Tmp = (u8Tmp & 0x80) ? u16Color : u16BgColor;
                for (m = 0; m < u32Scale; m++)
                {
                    for (n = 0; n < u32Scale; n++)
                    {
                        *(pu16FontBuff + n) = u16Tmp;
                    }

                    pu16FontBuff += (pstFont->u32Stride / 2);
                }

                u8Tmp <<= 1;
            }
        }
    }

    u32Left = pstLat->u32Width & 0x07;
    u32Left = SAL_align(u32Left, 2);        /* VGS画OSD需要二对齐 */
    for (i = 0; i < pstLat->u32Height; i++)
    {
        u8Tmp = *(pstLat->stAddr.pu8VirAddr + (i * pstLat->u32Stride) + u32Bytes);
        for (k = 0; k < u32Left; k++)
        {
            /* 乘2是RGB1555一个像素点2个字节 */
            pu16FontBuff = (UINT16 *)(pstFont->stAddr.pu8VirAddr + (i * u32Scale * pstFont->u32Stride) + ((u32Bytes * 8 + k) * u32Scale) * 2);
            u16Tmp = (u8Tmp & 0x80) ? u16Color : u16BgColor;
            for (m = 0; m < u32Scale; m++)
            {
                for (n = 0; n < u32Scale; n++)
                {
                    *(pu16FontBuff + n) = u16Tmp;
                }

                pu16FontBuff += pstFont->u32Stride;
            }

            u8Tmp <<= 1;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_blockInit
* 描  述  : osd block初始化
* 输  入  : UINT32 u32Block : block索引号
          OSD_BLOCK_PRM_S *pstBlockPrm : 初始化参数
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
static INT32 osd_blockInit(UINT32 u32Block, OSD_BLOCK_PRM_S *pstBlockPrm)
{
    OSD_BLOCK_S *pstBlock = NULL;
    OSD_FONT_REGION_S *pstFontRegion = NULL;
    UINT32 u32StringLen = 0;
    UINT32 *pu32Addr = NULL;
    UINT8 *pu8Addr = NULL;
    UINT8 *pu8AddrTmp = NULL;
    UINT32 u32Size = 0;
    UINT32 i = 0, j = 0, k = 0;
    UINT32 u32OsdNum = 0;

    if ((u32Block >= OSD_BLOCK_NUM_MAX) || (NULL == pstBlockPrm))
    {
        OSD_LOGE("osd block init invalid para: block:%u[max:%d],  %p\n",
                 u32Block, OSD_BLOCK_NUM_MAX, pstBlockPrm);
        return SAL_FAIL;
    }

    if (NULL != g_pastOsdBlock[u32Block])
    {
        OSD_LOGW("osd block[%u] has been initialized\n", u32Block);
        return SAL_SOK;
    }

    u32OsdNum = pstBlockPrm->u32OsdNum;

    g_pastOsdBlock[u32Block] =  SAL_memZalloc(sizeof(OSD_BLOCK_S), OSD_FUNC_NAME, OSD_MEM_NAME);
    if (NULL == g_pastOsdBlock[u32Block])
    {
        OSD_LOGE("osd block[%u] malloc fail\n", u32Block);
        return SAL_FAIL;
    }

    pstBlock = g_pastOsdBlock[u32Block];
    memset(pstBlock, 0, sizeof(OSD_BLOCK_S));

    (VOID)SAL_RwlockInit(&pstBlock->stRWLock);
    (VOID)pthread_mutex_init(&pstBlock->stMutex, NULL);

    pstBlock->pfFillLat = (OSD_FONT_TRUETYPE == capb_get_osd()->enFontType) ?
                          osd_freetype_FillLattice : osd_Hzk_FillLattice;

    memcpy(&pstBlock->stBlockPrm, pstBlockPrm, sizeof(*pstBlockPrm));
    u32StringLen = SAL_align(pstBlock->stBlockPrm.u32StringLenMax + 1, 8);

    /* 用于存放OSD字符 */
    pu8Addr = SAL_memZalloc(u32OsdNum * u32StringLen, OSD_FUNC_NAME, OSD_MEM_NAME);
    if (NULL == pu8Addr)
    {
        OSD_LOGE("osd block[%u] malloc string buffer\n", u32Block);
        SAL_memfree(g_pastOsdBlock[u32Block], OSD_FUNC_NAME, OSD_MEM_NAME);
        g_pastOsdBlock[u32Block] = NULL;
        return SAL_FAIL;
    }

    memset(pu8Addr, 0, u32OsdNum * u32StringLen);

    /* OSD字符串映射表置为无效值 */
    pu32Addr = (UINT32 *)pstBlock->au32IdxMap;
    for (i = 0; i < u32OsdNum; i++)
    {
        *pu32Addr++ = OSD_IDX_INVALID;
    }

    /* 点阵映射表置为无效值 */
    pu32Addr = (UINT32 *)pstBlock->au32LatMap;
    for (i = 0; i < sizeof(pstBlock->au32LatMap) / sizeof(pstBlock->au32LatMap[0]); i++)
    {
        *pu32Addr++ = OSD_IDX_INVALID;
    }

    /* 字体映射表置为无效值 */
    pu32Addr = (UINT32 *)pstBlock->au32FontMap;
    for (i = 0; i < sizeof(pstBlock->au32FontMap) / sizeof(pstBlock->au32FontMap[0][0]); i++)
    {
        *pu32Addr++ = OSD_IDX_INVALID;
    }

    /* 初始化OSD字符串索引值 */
    pu8AddrTmp = pu8Addr;
    
    for (i = 0; i < u32OsdNum; i++)
    {
        pstBlock->astSetPrm[i].szString = (char *)pu8AddrTmp;
        pu8AddrTmp += u32StringLen;

        pstFontRegion = pstBlock->astFontRegion + i;
        for (j = 0; (j < pstBlock->stBlockPrm.u32LatNum) && (j < OSD_LAT_PER_BLOCK); j++)
        {
            pstBlock->stBlockPrm.au32LatSizeMax[j] = SAL_align(pstBlock->stBlockPrm.au32LatSizeMax[j], 2);
            pstBlock->stBlockPrm.au32LatSize[j] = SAL_align(pstBlock->stBlockPrm.au32LatSize[j], 2);

            if ((pstBlock->stBlockPrm.au32LatSizeMax[j] > OSD_LAT_SIZE_MAX) || (pstBlock->stBlockPrm.au32LatSize[j] > pstBlock->stBlockPrm.au32LatSizeMax[j]))
            {
                OSD_LOGE("osd lattice size[%u %u] is bigger than max[%d]\n", pstBlock->stBlockPrm.au32LatSizeMax[j],
                         pstBlock->stBlockPrm.au32LatSize[j],
                         OSD_LAT_SIZE_MAX);
                goto exit;
            }

            pstBlock->au32LatMap[pstBlock->stBlockPrm.au32LatSize[j]] = j;
            pstFontRegion->astLatRegion[j].u32Size = SAL_align(osd_func_getLatBuffLen(pstBlock->stBlockPrm.au32LatSizeMax[j], pstBlock->stBlockPrm.u32StringLenMax), 16);

            /* 初始化OSD字体索引值 */
            for (k = 0; (k < pstBlock->stBlockPrm.au32FontNum[j]) && (k < OSD_FONT_PER_LAT); k++)
            {
                pstBlock->stBlockPrm.au32FontSizeMax[j][k] = SAL_align(pstBlock->stBlockPrm.au32FontSizeMax[j][k], 2);
                pstBlock->stBlockPrm.au32FontSize[j][k] = SAL_align(pstBlock->stBlockPrm.au32FontSize[j][k], 2);

                u32Size = pstBlock->stBlockPrm.au32FontSize[j][k];

                /* 字体过大或者字体大小不是点阵大小的整数倍 */
                if ((pstBlock->stBlockPrm.au32FontSizeMax[j][k] > OSD_FONT_SIZE_MAX)
                    || (u32Size > pstBlock->stBlockPrm.au32FontSizeMax[j][k])
                    || (0 != u32Size % pstBlock->stBlockPrm.au32LatSize[j]))
                {
                    OSD_LOGE("osd font size[%u %u max:%d] is invalid:latttice size[%u]\n", u32Size, pstBlock->stBlockPrm.au32FontSizeMax[j][k],
                             OSD_FONT_SIZE_MAX, pstBlock->stBlockPrm.au32LatSize[j]);
                    goto exit;
                }

                pstBlock->au32FontMap[j][pstBlock->stBlockPrm.au32FontSize[j][k]] = k;
                pstFontRegion->astFontRegion[j][k].u32Size = SAL_align(osd_func_getFontBuffLen(pstBlock->stBlockPrm.au32FontSizeMax[j][k], pstBlock->stBlockPrm.u32StringLenMax), 16);
                pstFontRegion->astBgFontRegion[j][k].u32Size = pstFontRegion->astFontRegion[j][k].u32Size;
            }
        }
    }

    return SAL_SOK;

exit:
    
    SAL_memfree(pu8Addr, OSD_FUNC_NAME, OSD_MEM_NAME);
    SAL_memfree(g_pastOsdBlock[u32Block], OSD_FUNC_NAME, OSD_MEM_NAME);
    g_pastOsdBlock[u32Block] = NULL;

    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : osd_func_blockSet
* 描  述  : osd block内容设置
* 输  入  : UINT32 u32Block : block索引号
          OSD_SET_ARRAY_S pstOsdArray : 配置参数
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_blockSet(UINT32 u32Block, OSD_SET_ARRAY_S *pstOsdArray)
{
    INT32 s32Ret = SAL_SOK;
    char szMmzName[20];
    //char *szZoneName = (DRAW_MOD_DSP == capb_get_osd()->enDrawMod) ? "dsp" : NULL;      /* dsp画OSD只需要点阵 */
    char *szZoneName = NULL;
    OSD_ADDR_S stLatAddr;
    OSD_ADDR_S stFontAddr;
    OSD_BLOCK_S *pstBlock = NULL;
    OSD_BLOCK_PRM_S *pstBlockPrm = NULL;
    OSD_FONT_REGION_S *pstFontRegion = NULL;
    OSD_SET_PRM_S *pstOsdPrm = NULL;
    OSD_SET_PRM_S *pstSetPrm = NULL;
    OSD_REGION_S *pstRegion = NULL;
    UINT32 u32Idx = 0;
    UINT32 u32LatSize = 0;
    UINT32 u32FontSize = 0;
    UINT32 u32NewNum = 0;
    UINT32 u32Argb1555 = 0;
    UINT32 u32BgArgb1555 = 0;
    UINT32 i = 0, j = 0, k = 0;
    
    memset(&stLatAddr, 0, sizeof(stLatAddr));
    memset(&stFontAddr, 0, sizeof(stFontAddr));

    if (u32Block >= OSD_BLOCK_NUM_MAX)
    {
        OSD_LOGE("block idx[%u] should less than %u\n", u32Block, OSD_BLOCK_NUM_MAX);
        return SAL_FAIL;
    }

    if ((NULL == pstOsdArray) || (NULL == pstOsdArray->pstOsdPrm))
    {
        OSD_LOGE("input pointer is null\n");
        return SAL_FAIL;
    }

    if (NULL == g_pastOsdBlock[u32Block])
    {
        OSD_LOGE("osd block[%u] has not initialized\n", u32Block);
        return SAL_FAIL;
    }

    pstBlock = g_pastOsdBlock[u32Block];
    pstBlockPrm = &pstBlock->stBlockPrm;

    /* 根据索引值判断是否有新增的索引值 */
    for (i = 0; i < pstOsdArray->u32StringNum; i++)
    {
        if (pstOsdArray->pstOsdPrm[i].u32Idx >= pstBlockPrm->u32OsdNum)
        {
            OSD_LOGW("osd block[%u] string idx[%u] is bigger than max[%d]\n",
                     u32Block, pstOsdArray->pstOsdPrm[i].u32Idx, pstBlockPrm->u32OsdNum);
            continue;
        }

        if (OSD_IDX_INVALID == pstBlock->au32IdxMap[pstOsdArray->pstOsdPrm[i].u32Idx])
        {
            ++u32NewNum;
        }
    }

    if (u32NewNum > pstBlockPrm->u32OsdNum)
    {
        OSD_LOGE("osd string[%u %u %u] is too much\n",u32NewNum, pstOsdArray->u32StringNum, pstBlockPrm->u32OsdNum);
        return SAL_FAIL;
    }

    if (0 != u32NewNum)
    {
        u32LatSize = 0;
        u32FontSize = 0;

        pstFontRegion = &pstBlock->astFontRegion[0];
        for (i = 0; (i < pstBlockPrm->u32LatNum) && (i < OSD_LAT_PER_BLOCK); i++)
        {
            /* 点阵大小 */
            u32LatSize += pstFontRegion->astLatRegion[i].u32Size;

            for (j = 0; (j < pstBlockPrm->au32FontNum[i]) && (j < OSD_FONT_PER_LAT); j++)
            {
                /* 字体大小 */
                u32FontSize += pstFontRegion->astFontRegion[i][j].u32Size;
                u32FontSize += pstFontRegion->astBgFontRegion[i][j].u32Size;
            }
        }

        u32LatSize *= u32NewNum;
        u32FontSize *= u32NewNum;

        if (SAL_TRUE == pstBlockPrm->bLatEnable)
        {
            sprintf(szMmzName, "osd_lat%u", u32Block);
            szZoneName = NULL;
            s32Ret = mem_hal_mmzAlloc(u32LatSize, OSD_FUNC_NAME, szMmzName, szZoneName, SAL_FALSE, &stLatAddr.u64PhyAddr, (VOID **)&stLatAddr.pu8VirAddr);
            if (SAL_SOK != s32Ret)
            {
                OSD_LOGE("alloc osd lattice buff[%u] fail:0x%X\n", u32LatSize, s32Ret);
                return SAL_FAIL;
            }
        }

        if (SAL_TRUE == pstBlockPrm->bFontEnable)
        {
            sprintf(szMmzName, "osd_font%u", u32Block);
            szZoneName = NULL;
            s32Ret = mem_hal_mmzAlloc(u32FontSize, OSD_FUNC_NAME, szMmzName, szZoneName, SAL_FALSE, &stFontAddr.u64PhyAddr, (VOID **)&stFontAddr.pu8VirAddr);
            if (SAL_SOK != s32Ret)
            {
                OSD_LOGE("alloc osd font buff[%u] fail:0x%X\n", u32FontSize, s32Ret);
                if (stLatAddr.u64PhyAddr)
                {
                    mem_hal_mmzFree(stLatAddr.pu8VirAddr, "osd_func", szMmzName);
                }
                return SAL_FAIL;
            }
        }
    }

    for (i = 0; i < pstOsdArray->u32StringNum; i++)
    {
        pstSetPrm = pstOsdArray->pstOsdPrm + i;

        /* 只检测越界，不检测重复，上层应保证索引值不重复 */
        if (pstSetPrm->u32Idx >= pstBlockPrm->u32OsdNum)
        {
            OSD_LOGW("osd block[%u] invalid idx[%u]\n", u32Block, pstSetPrm->u32Idx);
            continue;
        }

        if (OSD_IDX_INVALID == pstBlock->au32IdxMap[pstSetPrm->u32Idx])
        {
            /* 对上层传递下来的索引值重新做一次映射 */
            pstBlock->au32IdxMap[pstSetPrm->u32Idx] = pstBlock->u32StringNum;
            u32Idx = pstBlock->au32IdxMap[pstSetPrm->u32Idx];
            pstBlock->astSetPrm[u32Idx].u32Idx = pstSetPrm->u32Idx;
            pstBlock->u32StringNum++;

            if (SAL_TRUE == pstBlockPrm->bLatEnable)
            {
                for (j = 0; (j < pstBlockPrm->u32LatNum) && (j < OSD_LAT_PER_BLOCK); j++)
                {
                    pstRegion = pstBlock->astFontRegion[u32Idx].astLatRegion + j;

                    /* 分配点阵内存 */
                    pstRegion->stAddr.pu8VirAddr = stLatAddr.pu8VirAddr;
                    pstRegion->stAddr.u64PhyAddr = stLatAddr.u64PhyAddr;
                    stLatAddr.pu8VirAddr += pstRegion->u32Size;
                    stLatAddr.u64PhyAddr += pstRegion->u32Size;

                    if (SAL_TRUE == pstBlockPrm->bFontEnable)
                    {
                        for (k = 0; (k < pstBlockPrm->au32FontNum[j]) && (k < OSD_FONT_PER_LAT); k++)
                        {
                            /* 分配字体内存 */
                            pstRegion = &pstBlock->astFontRegion[u32Idx].astFontRegion[j][k];
                            pstRegion->stAddr.pu8VirAddr = stFontAddr.pu8VirAddr;
                            pstRegion->stAddr.u64PhyAddr = stFontAddr.u64PhyAddr;
                            stFontAddr.pu8VirAddr += pstRegion->u32Size;
                            stFontAddr.u64PhyAddr += pstRegion->u32Size;

                            pstRegion = &pstBlock->astFontRegion[u32Idx].astBgFontRegion[j][k];
                            pstRegion->stAddr.pu8VirAddr = stFontAddr.pu8VirAddr;
                            pstRegion->stAddr.u64PhyAddr = stFontAddr.u64PhyAddr;
                            stFontAddr.pu8VirAddr += pstRegion->u32Size;
                            stFontAddr.u64PhyAddr += pstRegion->u32Size;
                        }
                    }
                }
            }
        }

        u32Idx = pstBlock->au32IdxMap[pstSetPrm->u32Idx];
        pstOsdPrm = pstBlock->astSetPrm + u32Idx;
        pstFontRegion = pstBlock->astFontRegion + u32Idx;

        /* 有效字符串更新 */
        if (NULL != pstSetPrm->szString)
        {
            strcpy(pstOsdPrm->szString, pstSetPrm->szString);
            pstOsdPrm->u32StringLen = strlen(pstOsdPrm->szString);
            pstOsdPrm->enEncFormat = pstSetPrm->enEncFormat;
        }

        /* 有效颜色更新 */
        if (OSD_COLOR_INVALID != pstSetPrm->u32Color)
        {
            pstOsdPrm->u32Color = pstSetPrm->u32Color;
        }

        if (OSD_COLOR_INVALID != pstSetPrm->u32BgColor)
        {
            pstOsdPrm->u32BgColor = pstSetPrm->u32BgColor;
        }

        for (j = 0; (j < pstBlockPrm->u32LatNum) && (j < OSD_LAT_PER_BLOCK); j++)
        {
            /* 填充点阵 */
            if (SAL_TRUE == pstBlockPrm->bLatEnable)
            {
                pstRegion = &pstFontRegion->astLatRegion[j];
                memset(pstRegion->stAddr.pu8VirAddr, 0, pstRegion->u32Size);
                s32Ret = pstBlock->pfFillLat(pstOsdPrm->szString, pstRegion, pstBlockPrm->au32LatSize[j], pstOsdPrm->enEncFormat);
                if (SAL_SOK != s32Ret)
                {
                    OSD_LOGW("osd block[%u] fill lattice fail, size[%u] idx[%u]\n", u32Block, pstBlockPrm->au32LatSize[j], pstSetPrm->u32Idx);
                }
            }

            if (SAL_TRUE == pstBlockPrm->bFontEnable)
            {
                (VOID)SAL_RGB24ToARGB1555(pstOsdPrm->u32BgColor, &u32BgArgb1555, 1);

                for (k = 0; (k < pstBlockPrm->au32FontNum[j]) && (k < OSD_FONT_PER_LAT); k++)
                {
                    /* 填充字库 */
                    (VOID)SAL_RGB24ToARGB1555(pstOsdPrm->u32Color, &u32Argb1555, 1);
                    memset(pstFontRegion->astFontRegion[j][k].stAddr.pu8VirAddr, 0, pstFontRegion->astFontRegion[j][k].u32Size);
                    s32Ret = osd_func_FillFont(&pstFontRegion->astLatRegion[j], pstBlockPrm->au32LatSize[j],
                                          &pstFontRegion->astFontRegion[j][k], pstBlockPrm->au32FontSize[j][k],
                                          u32Argb1555, OSD_BACK_COLOR);
                    if (SAL_SOK != s32Ret)
                    {
                        OSD_LOGW("osd block[%u] fill font[%u] form lattice [%u] fail\n", u32Block, pstBlockPrm->au32LatSize[j], pstBlockPrm->au32FontSize[j][k]);
                    }

                    /* 填充带背景色的字库 */
                    (VOID)SAL_RGB24ToARGB1555(pstOsdPrm->u32Color, &u32Argb1555, 1);
                    memset(pstFontRegion->astBgFontRegion[j][k].stAddr.pu8VirAddr, 0, pstFontRegion->astBgFontRegion[j][k].u32Size);
                    s32Ret = osd_func_FillFont(&pstFontRegion->astLatRegion[j], pstBlockPrm->au32LatSize[j],
                                          &pstFontRegion->astBgFontRegion[j][k], pstBlockPrm->au32FontSize[j][k],
                                          u32BgArgb1555, u32Argb1555);
                    if (SAL_SOK != s32Ret)
                    {
                        OSD_LOGW("osd block[%u] fill bg font[%u] form lattice [%u] fail\n", u32Block, pstBlockPrm->au32LatSize[j], pstBlockPrm->au32FontSize[j][k]);
                    }
                }
            }
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_readLock
* 描  述  : 读锁
* 输  入  : UINT32 u32Block : OSD block索引号
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
static INT32 osd_readLock(UINT32 u32Block)
{
    OSD_BLOCK_S *pstBlock = NULL;

    if (u32Block >= OSD_BLOCK_NUM_MAX)
    {
        OSD_LOGE("block[%u] should less than %u\n", u32Block, OSD_BLOCK_NUM_MAX);
        return SAL_FAIL;
    }

    pstBlock = g_pastOsdBlock[u32Block];
    if (NULL == pstBlock)
    {
        OSD_LOGE("block[%u] has not initialized\n", u32Block);
        return SAL_FAIL;
    }

    (VOID)SAL_RwlockRdlock(&pstBlock->stRWLock, SAL_TIMEOUT_FOREVER, NULL, 0);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_writeLock
* 描  述  : 写锁
* 输  入  : UINT32 u32Block : OSD block索引号
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
static INT32 osd_writeLock(UINT32 u32Block)
{
    OSD_BLOCK_S *pstBlock = NULL;

    if (u32Block >= OSD_BLOCK_NUM_MAX)
    {
        OSD_LOGE("block[%u] should less than %u\n", u32Block, OSD_BLOCK_NUM_MAX);
        return SAL_FAIL;
    }

    pstBlock = g_pastOsdBlock[u32Block];
    if (NULL == pstBlock)
    {
        OSD_LOGE("block[%u] has not initialized\n", u32Block);
        return SAL_FAIL;
    }

    (VOID)SAL_RwlockWrlock(&pstBlock->stRWLock, SAL_TIMEOUT_FOREVER, NULL, 0);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_unlock
* 描  述  : 解锁
* 输  入  : UINT32 u32Block : OSD block索引号
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
static INT32 osd_unlock(UINT32 u32Block)
{
    OSD_BLOCK_S *pstBlock = NULL;

    if (u32Block >= OSD_BLOCK_NUM_MAX)
    {
        OSD_LOGE("block[%u] should less than %u\n", u32Block, OSD_BLOCK_NUM_MAX);
        return SAL_FAIL;
    }

    pstBlock = g_pastOsdBlock[u32Block];
    if (NULL == pstBlock)
    {
        OSD_LOGE("block[%u] has not initialized\n", u32Block);
        return SAL_FAIL;
    }

    (VOID)SAL_RwlockUnlock(&pstBlock->stRWLock);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_rwEnd
* 描  述  : 解锁
* 输  入  :
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
static INT32 osd_rwEnd(VOID)
{
#ifdef DSP_ISA
    (VOID)osd_unlock(OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL);
#endif
    (VOID)osd_unlock(OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT);
    (VOID)osd_unlock(OSD_BLOCK_IDX_NUM_MUL);
    (VOID)osd_unlock(OSD_BLOCK_IDX_NUM_PAREN);
    (VOID)osd_unlock(OSD_BLOCK_IDX_ASCII);
    (VOID)osd_unlock(OSD_BLOCK_IDX_STRING);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_func_readStart
* 描  述  : 开始读操作，加锁保护
* 输  入  :
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_readStart(VOID)
{
    (VOID)osd_readLock(OSD_BLOCK_IDX_STRING);
    (VOID)osd_readLock(OSD_BLOCK_IDX_ASCII);
    (VOID)osd_readLock(OSD_BLOCK_IDX_NUM_PAREN);
    (VOID)osd_readLock(OSD_BLOCK_IDX_NUM_MUL);
    (VOID)osd_readLock(OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT);
#ifdef DSP_ISA
    (VOID)osd_readLock(OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL);
#endif
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_func_readEnd
* 描  述  : 解锁
* 输  入  :
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_readEnd(VOID)
{
    return osd_rwEnd();
}

/*******************************************************************************
* 函数名  : osd_func_writeStart
* 描  述  : 开始写操作，加锁保护
* 输  入  :
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_writeStart(VOID)
{
    (VOID)osd_writeLock(OSD_BLOCK_IDX_STRING);
    (VOID)osd_writeLock(OSD_BLOCK_IDX_ASCII);
    (VOID)osd_writeLock(OSD_BLOCK_IDX_NUM_PAREN);
    (VOID)osd_writeLock(OSD_BLOCK_IDX_NUM_MUL);
    (VOID)osd_writeLock(OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT);
#ifdef DSP_ISA
    (VOID)osd_writeLock(OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL);
#endif
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_func_writeEnd
* 描  述  : 解锁
* 输  入  :
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_writeEnd(VOID)
{
    return osd_rwEnd();
}

/*******************************************************************************
* 函数名  : osd_func_getOsdPrm
* 描  述  : 获取OSD相关参数
* 输  入  : UINT32 u32Block : OSD block索引号
          OSD_BLOCK_PRM_S *pstSetPrm : 下发参数
* 输  出  :
* 返回值  : NULL  : 失败
          非NULL : 成功
*******************************************************************************/
OSD_SET_PRM_S *osd_func_getOsdPrm(UINT32 u32Block, UINT32 u32Osd)
{
    OSD_BLOCK_S *pstBlock = NULL;
    OSD_SET_PRM_S *pstOsd = NULL;
    UINT32 u32OsdIdx = 0;

    if (u32Block >= OSD_BLOCK_NUM_MAX)
    {
        OSD_LOGE("block[%u] should less than %u\n", u32Block, OSD_BLOCK_NUM_MAX);
        return NULL;
    }

    pstBlock = g_pastOsdBlock[u32Block];
    if (NULL == pstBlock)
    {
        OSD_LOGE("block[%u] has not initialized\n", u32Block);
        return NULL;
    }

    u32OsdIdx = pstBlock->au32IdxMap[u32Osd];
    if (u32OsdIdx >= pstBlock->u32StringNum)
    {
        OSD_LOGE("block[%u] osd idx[%u] must less than num[%u]\n", u32Block, u32OsdIdx, pstBlock->u32StringNum);
        return NULL;
    }

    pstOsd = &pstBlock->astSetPrm[u32OsdIdx];

    return (u32Osd == pstOsd->u32Idx) ? pstOsd : NULL;
}

/*******************************************************************************
* 函数名  : osd_func_getLatRegion
* 描  述  : 获取点阵的地址
* 输  入  : UINT32 u32BlockIdx : block索引号
          UINT32 u32OsdIdx : OSD索引号
          UINT32 u32LatSize : 点阵大小
* 输  出  :
* 返回值  : NULL  : 失败
          非NULL : 成功
*******************************************************************************/
OSD_REGION_S *osd_func_getLatRegion(UINT32 u32Block, UINT32 u32Osd, UINT32 u32LatSize)
{
    OSD_BLOCK_S *pstBlock = NULL;
    UINT32 u32OsdIdx = 0;
    UINT32 u32LatIdx = 0;

    if (u32Block >= OSD_BLOCK_NUM_MAX)
    {
        OSD_LOGE("block[%u] should less than %u\n", u32Block, OSD_BLOCK_NUM_MAX);
        return NULL;
    }

    pstBlock = g_pastOsdBlock[u32Block];
    if (NULL == pstBlock)
    {
        OSD_LOGE("block[%u] has not initialized\n", u32Block);
        return NULL;
    }

    if (SAL_TRUE != pstBlock->stBlockPrm.bLatEnable)
    {
        OSD_LOGE("block[%u] lat not enable\n", u32Block);
        return NULL;
    }

    u32OsdIdx = pstBlock->au32IdxMap[u32Osd];
    if (u32OsdIdx >= pstBlock->u32StringNum)
    {
        OSD_LOGE("block[%u] osd idx[%u] must less than num[%u]\n", u32Block, u32OsdIdx, pstBlock->u32StringNum);
        return NULL;
    }

    u32LatIdx = pstBlock->au32LatMap[u32LatSize];
    if (u32LatIdx > pstBlock->stBlockPrm.u32LatNum)
    {
        OSD_LOGE("block[%u] lat idx[%u %u %u] invalid\n", u32Block, u32LatIdx, pstBlock->stBlockPrm.u32LatNum, u32LatSize);
        return NULL;
    }

    return &pstBlock->astFontRegion[u32OsdIdx].astLatRegion[u32LatIdx];
}

/*******************************************************************************
* 函数名  : osd_func_getFontRegion
* 描  述  : 获取字库的地址
* 输  入  : UINT32 u32Block : block索引号
          UINT32 u32Osd : OSD索引号
          UINT32 u32LatSize : 点阵大小
          UINT32 u32FontSize : 字体大小
          BOOL bBgEnable : 是否带背景色
* 输  出  :
* 返回值  : NULL  : 失败
          非NULL : 成功
*******************************************************************************/
OSD_REGION_S *osd_func_getFontRegion(UINT32 u32Block, UINT32 u32Osd, UINT32 u32LatSize, UINT32 u32FontSize, BOOL bBgEnable)
{
    OSD_BLOCK_S *pstBlock = NULL;
    UINT32 u32OsdIdx = 0;
    UINT32 u32LatIdx = 0;
    UINT32 u32FontIdx = 0;

    if (u32Block >= OSD_BLOCK_NUM_MAX)
    {
        OSD_LOGE("block[%u] should less than %u\n", u32Block, OSD_BLOCK_NUM_MAX);
        return NULL;
    }

    pstBlock = g_pastOsdBlock[u32Block];
    if (NULL == pstBlock)
    {
        OSD_LOGE("block[%u] has not initialized\n", u32Block);
        return NULL;
    }

    if ((SAL_TRUE != pstBlock->stBlockPrm.bLatEnable) || (SAL_TRUE != pstBlock->stBlockPrm.bFontEnable))
    {
        OSD_LOGE("block[%u] lat[%u] or font[%d] not enable\n", u32Block, pstBlock->stBlockPrm.bLatEnable, pstBlock->stBlockPrm.bFontEnable);
        return NULL;
    }

    u32OsdIdx = pstBlock->au32IdxMap[u32Osd];
    if (u32OsdIdx >= pstBlock->u32StringNum)
    {
        OSD_LOGE("block[%u] osd idx[%u] must less than num[%u]\n", u32Block, u32OsdIdx, pstBlock->u32StringNum);
        return NULL;
    }

    u32LatIdx = pstBlock->au32LatMap[u32LatSize];
    u32FontIdx = pstBlock->au32FontMap[u32LatIdx][u32FontSize];

    if ((u32LatIdx >= pstBlock->stBlockPrm.u32LatNum) || (u32FontIdx >= pstBlock->stBlockPrm.au32FontNum[u32LatIdx]))
    {
        OSD_LOGE("block[%u] lat idx[%u %u] or font idx[%u %u] invalid\n", u32Block, u32LatIdx, pstBlock->stBlockPrm.u32LatNum,
                 u32FontIdx, pstBlock->stBlockPrm.au32FontNum[u32LatIdx]);
        return NULL;
    }

    return (SAL_TRUE == bBgEnable) ? &pstBlock->astFontRegion[u32OsdIdx].astBgFontRegion[u32LatIdx][u32FontIdx]
           : &pstBlock->astFontRegion[u32OsdIdx].astFontRegion[u32LatIdx][u32FontIdx];
}

/*******************************************************************************
* 函数名  : osd_func_updateFontSize
* 描  述  : 更新字体大小
* 输  入  : UINT32 u32Block : block索引号
          UINT32 u32LatSize : 点阵大小
          UINT32 u32FontNum : 更新后的字体个数
          UINT32 *pu32FontSize : 更新后的字体大小
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_updateFontSize(UINT32 u32Block, UINT32 u32LatSize, UINT32 u32FontNum, UINT32 *pu32FontSize)
{
    INT32 s32Ret = 0;
    OSD_BLOCK_S *pstBlock = NULL;
    OSD_SET_PRM_S *pstOsdPrm = NULL;
    OSD_BLOCK_PRM_S *pstBlockPrm = NULL;
    OSD_REGION_S *pstLatRegion = NULL;
    OSD_REGION_S *pstFontRegion = NULL;
    UINT32 u32LatIdx = 0;
    UINT32 u32OsdNum = 0;
    UINT32 u32UpNum = 0;
    UINT32 u32Argb1555 = 0;
    UINT32 u32BgArgb1555 = 0;
    UINT32 i = 0, j = 0;

    if (u32Block >= OSD_BLOCK_NUM_MAX)
    {
        OSD_LOGE("block[%u] should less than %u\n", u32Block, OSD_BLOCK_NUM_MAX);
        return SAL_FAIL;
    }

    pstBlock = g_pastOsdBlock[u32Block];
    if (NULL == pstBlock)
    {
        OSD_LOGE("block[%u] has not initialized\n", u32Block);
        return SAL_FAIL;
    }

    if ((SAL_TRUE != pstBlock->stBlockPrm.bLatEnable) || (SAL_TRUE != pstBlock->stBlockPrm.bFontEnable))
    {
        OSD_LOGE("block[%u] lat[%u] or font[%d] not enable\n", u32Block, pstBlock->stBlockPrm.bLatEnable, pstBlock->stBlockPrm.bFontEnable);
        return SAL_FAIL;
    }

    u32LatIdx = pstBlock->au32LatMap[u32LatSize];
    if (u32LatIdx >= pstBlock->stBlockPrm.u32LatNum)
    {
        OSD_LOGE("block[%u] lat idx[%u %u] invalid\n", u32Block, u32LatIdx, pstBlock->stBlockPrm.u32LatNum);
        return SAL_FAIL;
    }

    u32OsdNum = pstBlock->u32StringNum;
    pstBlockPrm = &pstBlock->stBlockPrm;

    for (i = 0; (i < u32FontNum) && (i < pstBlockPrm->au32FontNum[u32LatIdx]); i++)
    {
        if ((*pu32FontSize > pstBlockPrm->au32FontSizeMax[u32LatIdx][i]) || (0 != *pu32FontSize % u32LatSize))
        {
            OSD_LOGW("osd block[%u] font size[%u] is invalid max[%u] lat[%u]\n", u32Block, *pu32FontSize, pstBlockPrm->au32FontSizeMax[u32LatIdx][i], u32LatSize);
            continue;
        }

        /* 不等于原来字体大小的时候才更新 */
        if (*pu32FontSize != pstBlockPrm->au32FontSize[u32LatIdx][i])
        {
            for (j = 0; j < u32OsdNum; j++)
            {
                pstOsdPrm = pstBlock->astSetPrm + j;
                pstLatRegion = &pstBlock->astFontRegion[j].astLatRegion[u32LatIdx];

                (VOID)SAL_RGB24ToARGB1555(pstOsdPrm->u32BgColor, &u32BgArgb1555, 1);

                pstFontRegion = &pstBlock->astFontRegion[j].astFontRegion[u32LatIdx][i];
                (VOID)SAL_RGB24ToARGB1555(pstOsdPrm->u32Color, &u32Argb1555, 0);
                s32Ret = osd_func_FillFont(pstLatRegion, u32LatSize, pstFontRegion, *pu32FontSize, u32Argb1555, OSD_BACK_COLOR);
                if (SAL_SOK != s32Ret)
                {
                    OSD_LOGW("block[%u] update osd[%u] to size[%u] fail\n", u32Block, pstOsdPrm->u32Idx, *pu32FontSize);
                }

                pstFontRegion = &pstBlock->astFontRegion[j].astBgFontRegion[u32LatIdx][i];
                (VOID)SAL_RGB24ToARGB1555(pstOsdPrm->u32Color, &u32Argb1555, 1);
                s32Ret = osd_func_FillFont(pstLatRegion, u32LatSize, pstFontRegion, *pu32FontSize, u32BgArgb1555, u32Argb1555);
                if (SAL_SOK != s32Ret)
                {
                    OSD_LOGW("block[%u] update osd[%u] to size[%u] fail\n", u32Block, pstOsdPrm->u32Idx, *pu32FontSize);
                }
            }
        }

        pstBlock->au32FontMap[u32LatIdx][*pu32FontSize] = i;
        pstBlock->au32FontMap[u32LatIdx][pstBlockPrm->au32FontSize[u32LatIdx][i]] = OSD_IDX_INVALID;
        pstBlockPrm->au32FontSize[u32LatIdx][i] = *pu32FontSize;
        pu32FontSize++;
        u32UpNum++;
    }

    for (i = u32UpNum; i < u32FontNum; i++)
    {
        OSD_LOGW("block[%u] update font to size[%u] fail\n", u32Block, *pu32FontSize++);
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : OSD_GetFontAddr
* 描  述  : 获取字库的地址
* 输  入  : UINT32 u32Block : OSD block索引号
          OSD_BLOCK_PRM_S *pstSetPrm : 下发参数
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_updateLatSize(UINT32 u32Block, OSD_BLOCK_PRM_S *pstSetPrm)
{
    INT32 s32Ret = 0;
    OSD_BLOCK_S *pstBlock = NULL;
    OSD_SET_PRM_S *pstOsdPrm = NULL;
    OSD_BLOCK_PRM_S *pstBlockPrm = NULL;
    OSD_REGION_S *pstLatRegion = NULL;
    UINT32 u32OsdNum = 0;
    UINT32 u32UpNum = 0;
    UINT32 *pu32LatSize = NULL;
    UINT32 i = 0, j = 0;
    ENCODING_FORMAT_E enEncFormat = ENC_FMT_GB2312;

    if (u32Block == OSD_BLOCK_IDX_STRING) /* 违禁品名称点阵应用下发，字符串编码应用给出 */
    {
        enEncFormat = SystemPrm_getDspInitPara()->stFontLibInfo.enEncFormat;
    }
    else
    {
        enEncFormat = ENC_FMT_GB2312; /* 其余点阵名称代码写死，编码为文件编码格式 */
    }

    if (u32Block >= OSD_BLOCK_NUM_MAX)
    {
        OSD_LOGE("block[%u] should less than %u\n", u32Block, OSD_BLOCK_NUM_MAX);
        return SAL_FAIL;
    }

    pstBlock = g_pastOsdBlock[u32Block];
    if (NULL == pstBlock)
    {
        OSD_LOGE("block[%u] has not initialized\n", u32Block);
        return SAL_FAIL;
    }

    if (SAL_TRUE != pstBlock->stBlockPrm.bLatEnable)
    {
        OSD_LOGE("block[%u] lat not enable\n", u32Block);
        return SAL_FAIL;
    }

    u32OsdNum = pstBlock->u32StringNum;
    pstBlockPrm = &pstBlock->stBlockPrm;
    pu32LatSize = pstSetPrm->au32LatSize;
    for (i = 0; (i < pstSetPrm->u32LatNum) && (i < pstBlockPrm->u32LatNum); i++)
    {
        if (0 != *pu32LatSize % 2)
        {
            *pu32LatSize += 1;
        }

        if (*pu32LatSize > pstBlockPrm->au32LatSizeMax[i])
        {
            OSD_LOGW("osd block[%u] lat size[%u] is bigger than max[%u]\n", u32Block, *pu32LatSize, pstBlockPrm->au32LatSizeMax[i]);
            continue;
        }

        /* 不等于原来的字体大小才更新 */
        if (*pu32LatSize != pstBlockPrm->au32LatSize[i])
        {
            /* 填充点阵 */
            for (j = 0; j < u32OsdNum; j++)
            {
                pstOsdPrm = pstBlock->astSetPrm + j;
                pstLatRegion = &pstBlock->astFontRegion[j].astLatRegion[i];

                /* 填充点阵 */
                memset(pstLatRegion->stAddr.pu8VirAddr, 0, pstLatRegion->u32Size);
                s32Ret = pstBlock->pfFillLat(pstOsdPrm->szString, pstLatRegion, *pu32LatSize, enEncFormat);
                if (SAL_SOK != s32Ret)
                {
                    OSD_LOGW("osd block[%u] fill lattice fail, size[%u]\n", u32Block, *pu32LatSize);
                }
            }

            pstBlock->au32LatMap[pstBlockPrm->au32LatSize[i]] = OSD_IDX_INVALID;
            pstBlock->au32LatMap[*pu32LatSize] = i;
            pstBlockPrm->au32LatSize[i] = *pu32LatSize;

            if (SAL_TRUE == pstBlock->stBlockPrm.bFontEnable)
            {
                /* 更新字体 */
                s32Ret = osd_func_updateFontSize(u32Block, *pu32LatSize, pstSetPrm->au32FontNum[u32UpNum], pstSetPrm->au32FontSize[u32UpNum]);
                if (SAL_SOK != s32Ret)
                {
                    OSD_LOGW("osd block[%u] update font size fail:lat size[%u]\n", u32Block, *pu32LatSize);
                }
            }
        }

        pu32LatSize++;
        u32UpNum++;
    }

    for (i = u32UpNum; i < pstSetPrm->u32LatNum; i++)
    {
        OSD_LOGW("block[%u] update lat to size[%u] fail\n", u32Block, *pu32LatSize++);
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : osd_func_getFontSize
* 描  述  : 
* 输  入  :
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
inline UINT32 osd_func_getFontSize(INT32 s32Size)
{
    INT32 s32Tmp = s32Size - 1;
    static UINT32 au32Size[] = {18, 24, 30};

    if (s32Tmp < 0)
    {
        s32Tmp = 0;
    }

    if (s32Tmp >= sizeof(au32Size)/sizeof(au32Size[0]))
    {
        s32Tmp = sizeof(au32Size)/sizeof(au32Size[0]) - 1;
    }

    return au32Size[s32Tmp];
}

/*******************************************************************************
* 函数名  : osd_func_alertNameBlockInit
* 描  述  : 违禁品名称的OSD初始化
* 输  入  :
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_alertNameBlockInit(UINT32 u32NameLen, UINT32 u32NameNum)
{
    INT32 s32Ret = SAL_SOK;
    OSD_BLOCK_PRM_S stBlockPrm;
    UINT32 i = 0, j = 0;
    CAPB_OSD *pstCapbOsd = capb_get_osd();

    stBlockPrm.u32StringLenMax = u32NameLen;
    stBlockPrm.bLatEnable = SAL_TRUE;
    stBlockPrm.bFontEnable = SAL_TRUE;
    stBlockPrm.u32OsdNum = u32NameNum;

    stBlockPrm.u32LatNum = (OSD_FONT_TRUETYPE == pstCapbOsd->enFontType) ? 3 : 1;
    for (; i < stBlockPrm.u32LatNum; i++)
    {
        if (OSD_FONT_TRUETYPE == pstCapbOsd->enFontType)
        {
            stBlockPrm.au32LatSize[i] = pstCapbOsd->TrueTypeSize[i];
            stBlockPrm.au32LatSizeMax[i] = pstCapbOsd->TrueTypeSize[i];
            stBlockPrm.au32FontNum[i] = 1;
            stBlockPrm.au32FontSize[i][0] = pstCapbOsd->TrueTypeSize[i];
            stBlockPrm.au32FontSizeMax[i][0] = pstCapbOsd->TrueTypeSize[i];
        }
        else
        {
            stBlockPrm.au32LatSize[0] = 16;
            stBlockPrm.au32LatSizeMax[0] = 16;
            stBlockPrm.au32FontNum[0] = 3;

            for (; j < stBlockPrm.au32FontNum[0]; j++)
            {
                stBlockPrm.au32FontSize[0][i] = 16 * (i + 1);
                stBlockPrm.au32FontSizeMax[0][i] = 16 * (i + 1);
            }
        }
    }

    s32Ret = osd_blockInit(OSD_BLOCK_IDX_STRING, &stBlockPrm);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd block[%u] init fail\n", OSD_BLOCK_IDX_STRING);
        return SAL_FAIL;
    }

    return SAL_SOK;
}



/**
 * @fn      osd_func_asciiBlockInit
 * @brief   OSD ASCII字符 Block初始化
 * 
 * @return  SAL_STATUS 
 */
SAL_STATUS osd_func_asciiBlockInit(VOID)
{
    INT32 s32Ret = SAL_SOK;
    OSD_BLOCK_PRM_S stBlockPrm = {0};
    UINT32 i = 0, j = 0, u32PrtedAscNum = OSD_CHAR_ASC_PRT_END - OSD_CHAR_ASC_PRT_START + 1;
    CAPB_OSD *pstCapbOsd = capb_get_osd();
    OSD_SET_ARRAY_S stOsdArray = {0};
    OSD_SET_PRM_S *pstOsdPrm = NULL;
    CHAR aszString[u32PrtedAscNum][2];

    if(pstCapbOsd == NULL)
    {
        return SAL_FAIL;
    }

    stBlockPrm.bLatEnable = SAL_TRUE;
    stBlockPrm.bFontEnable = SAL_TRUE;
    stBlockPrm.u32LatNum = (OSD_FONT_TRUETYPE == pstCapbOsd->enFontType) ? 3 : 1; // 矢量字体每种尺寸都需要申请
    for (; i < stBlockPrm.u32LatNum; i++)
    {
        if (OSD_FONT_TRUETYPE == pstCapbOsd->enFontType)
        {
            stBlockPrm.au32LatSize[i] = pstCapbOsd->TrueTypeSize[i];
            stBlockPrm.au32LatSizeMax[i] = pstCapbOsd->TrueTypeSize[i];
            stBlockPrm.au32FontNum[i] = 1;
            stBlockPrm.au32FontSize[i][0] = pstCapbOsd->TrueTypeSize[i];
            stBlockPrm.au32FontSizeMax[i][0] = pstCapbOsd->TrueTypeSize[i];
        }
        else
        {
            stBlockPrm.au32LatSize[0] = 16;
            stBlockPrm.au32LatSizeMax[0] = 16;
            stBlockPrm.au32FontNum[0] = 3;

            for (; j < stBlockPrm.au32FontNum[0]; j++)
            {
                stBlockPrm.au32FontSize[0][i] = 16 * (i + 1);
                stBlockPrm.au32FontSizeMax[0][i] = 16 * (i + 1);
            }
        }
    }

    /* 初始化ASCII码的OSD Block */
    stBlockPrm.u32StringLenMax = 2; // 单个ASCII字符，预留1
    stBlockPrm.u32OsdNum = u32PrtedAscNum; // 仅统计可打印字符的数量
    s32Ret = osd_blockInit(OSD_BLOCK_IDX_ASCII, &stBlockPrm);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd_blockInit OSD_BLOCK_IDX_ASCII(%d) failed\n", OSD_BLOCK_IDX_ASCII);
        return SAL_FAIL;
    }

    stOsdArray.pstOsdPrm = SAL_memZalloc(stBlockPrm.u32OsdNum * sizeof(OSD_SET_PRM_S), OSD_FUNC_NAME, OSD_MEM_NAME);
    if (NULL == stOsdArray.pstOsdPrm)
    {
        OSD_LOGE("SAL_memZalloc failed, buffer size: %zu\n", stBlockPrm.u32OsdNum * sizeof(OSD_SET_PRM_S));
        return SAL_FAIL;
    }

    /* 填充ASCII码的OSD Block */
    stOsdArray.u32StringNum = u32PrtedAscNum; // 可打印的ASCII码
    memset(aszString, 0, sizeof(aszString));
    for (i = 0; i < stOsdArray.u32StringNum; i++)
    {
        pstOsdPrm = stOsdArray.pstOsdPrm + i;
        sprintf(aszString[i], "%c", i+OSD_CHAR_ASC_PRT_START);
        pstOsdPrm->u32Idx = i;
        pstOsdPrm->szString = aszString[i];
        pstOsdPrm->u32Color = OSD_COLOR_INVALID;
        pstOsdPrm->u32BgColor = OSD_COLOR_INVALID;
        pstOsdPrm->enEncFormat = ENC_FMT_GB2312;
    }
    s32Ret = osd_func_blockSet(OSD_BLOCK_IDX_ASCII, &stOsdArray);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd_func_blockSet OSD_BLOCK_IDX_ASCII(%d) failed\n", OSD_BLOCK_IDX_ASCII);
    }

    SAL_memfree(stOsdArray.pstOsdPrm, OSD_FUNC_NAME, OSD_MEM_NAME);

    return s32Ret;
}


/*******************************************************************************
* 函数名  : osd_func_numBlockInit
* 描  述  : 初始化数字的OSD
* 输  入  :
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_numBlockInit(VOID)
{
    INT32 s32Ret = SAL_SOK;
    OSD_BLOCK_PRM_S stBlockPrm;
    OSD_SET_ARRAY_S stOsdArray;
    OSD_SET_PRM_S *pstOsdPrm;
    char aszString[OSD_IDX_NUM_MAX*OSD_IDX_NUM_MAX][32];
    UINT32 i = 0, j = 0;
    CAPB_OSD *pstCapbOsd = capb_get_osd();

    stOsdArray.pstOsdPrm = SAL_memZalloc(OSD_IDX_NUM_MAX * OSD_IDX_NUM_MAX * sizeof(OSD_SET_PRM_S), OSD_FUNC_NAME, OSD_MEM_NAME);
    if (NULL == stOsdArray.pstOsdPrm)
    {
        OSD_LOGE("osd malloc fail\n");
        return SAL_FAIL;
    }

    stBlockPrm.bLatEnable = SAL_TRUE;
    stBlockPrm.bFontEnable = SAL_FALSE;
    stBlockPrm.u32LatNum = (OSD_FONT_TRUETYPE == pstCapbOsd->enFontType) ? 3 : 1;
    for (; i < stBlockPrm.u32LatNum; i++)
    {
        if (OSD_FONT_TRUETYPE == pstCapbOsd->enFontType)
        {
            stBlockPrm.au32LatSize[i] = pstCapbOsd->TrueTypeSize[i];
            stBlockPrm.au32LatSizeMax[i] = pstCapbOsd->TrueTypeSize[i];
            stBlockPrm.au32FontNum[i] = 1;
            stBlockPrm.au32FontSize[i][0] = pstCapbOsd->TrueTypeSize[i];
            stBlockPrm.au32FontSizeMax[i][0] = pstCapbOsd->TrueTypeSize[i];
        }
        else
        {
            stBlockPrm.au32LatSize[0] = 16;
            stBlockPrm.au32LatSizeMax[0] = 16;
            stBlockPrm.au32FontNum[0] = 3;

            for (; j < stBlockPrm.au32FontNum[0]; j++)
            {
                stBlockPrm.au32FontSize[0][i] = 16 * (i + 1);
                stBlockPrm.au32FontSizeMax[0][i] = 16 * (i + 1);
            }
        }
    }

    stBlockPrm.u32StringLenMax = strlen("(  %)");
    stBlockPrm.u32OsdNum = OSD_IDX_NUM_MAX;
    s32Ret = osd_blockInit(OSD_BLOCK_IDX_NUM_PAREN, &stBlockPrm);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd block[%u] init fail\n", OSD_BLOCK_IDX_NUM_PAREN);
        SAL_memfree(stOsdArray.pstOsdPrm, OSD_FUNC_NAME, OSD_MEM_NAME);
        return SAL_FAIL;
    }

    stOsdArray.u32StringNum = 0;
    for (i = 0; i < OSD_IDX_NUM_MAX; i++)
    {
        pstOsdPrm = stOsdArray.pstOsdPrm + stOsdArray.u32StringNum++;
        sprintf(aszString[i], "(%2u%s)", i, "%");
        pstOsdPrm->u32Idx = i;
        pstOsdPrm->szString = aszString[i];
        pstOsdPrm->u32Color = OSD_COLOR_INVALID;
        pstOsdPrm->u32BgColor = OSD_COLOR_INVALID;
        pstOsdPrm->enEncFormat = ENC_FMT_GB2312;
    }

    s32Ret = osd_func_blockSet(OSD_BLOCK_IDX_NUM_PAREN, &stOsdArray);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd block[%u] set fail\n", OSD_BLOCK_IDX_NUM_PAREN);
        SAL_memfree(stOsdArray.pstOsdPrm, OSD_FUNC_NAME, OSD_MEM_NAME);
        return SAL_FAIL;
    }

    stBlockPrm.u32StringLenMax = 3;
    stBlockPrm.u32OsdNum = OSD_IDX_NUM_MAX;
    s32Ret = osd_blockInit(OSD_BLOCK_IDX_NUM_MUL, &stBlockPrm);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd block[%u] init fail\n", OSD_BLOCK_IDX_NUM_MUL);
        SAL_memfree(stOsdArray.pstOsdPrm, OSD_FUNC_NAME, OSD_MEM_NAME);
        return SAL_FAIL;
    }

    stOsdArray.u32StringNum = 0;
    for (i = 0; i < OSD_IDX_NUM_MAX; i++)
    {
        pstOsdPrm = stOsdArray.pstOsdPrm + stOsdArray.u32StringNum++;
        sprintf(aszString[i], "X%-2u", i);
        pstOsdPrm->u32Idx     = i;
        pstOsdPrm->szString   = aszString[i];
        pstOsdPrm->u32Color   = OSD_COLOR_INVALID;
        pstOsdPrm->u32BgColor = OSD_COLOR_INVALID;
        pstOsdPrm->enEncFormat = ENC_FMT_GB2312;
    }

    s32Ret = osd_func_blockSet(OSD_BLOCK_IDX_NUM_MUL, &stOsdArray);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd block[%u] set fail\n", OSD_BLOCK_IDX_NUM_MUL);
        SAL_memfree(stOsdArray.pstOsdPrm, OSD_FUNC_NAME, OSD_MEM_NAME);
        return SAL_FAIL;
    }

    stBlockPrm.u32StringLenMax = strlen("(  .  %)");
    stBlockPrm.u32OsdNum = OSD_IDX_NUM_MAX * OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT_IDX_SUBNUM_MAX;
    s32Ret = osd_blockInit(OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT, &stBlockPrm);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd block[%u] init fail\n", OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT);
        SAL_memfree(stOsdArray.pstOsdPrm, OSD_FUNC_NAME, OSD_MEM_NAME);
        return SAL_FAIL;
    }

    stOsdArray.u32StringNum = 0;
    for (i = 0; i < OSD_IDX_NUM_MAX; i++)
    {
        for (j = 0; j < OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT_IDX_SUBNUM_MAX; j++)
        {
            pstOsdPrm = stOsdArray.pstOsdPrm + stOsdArray.u32StringNum++;
            sprintf(aszString[i+j*OSD_IDX_NUM_MAX], "(%02u.%02u%s)", i, j, "%");
            pstOsdPrm->u32Idx = i+j*OSD_IDX_NUM_MAX;
            pstOsdPrm->szString = aszString[i+j*OSD_IDX_NUM_MAX];
            pstOsdPrm->u32Color = OSD_COLOR_INVALID;
            pstOsdPrm->u32BgColor = OSD_COLOR_INVALID;
            pstOsdPrm->enEncFormat = ENC_FMT_GB2312;
        }
    }

    s32Ret = osd_func_blockSet(OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT, &stOsdArray);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd block[%u] set fail\n", OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT);
        SAL_memfree(stOsdArray.pstOsdPrm, OSD_FUNC_NAME, OSD_MEM_NAME);
        return SAL_FAIL;
    }

#ifdef DSP_ISA
    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();
    if (pstDspInfo->languageType >= LANGUAGE_TYPE_NUM)
    {
        OSD_LOGE("language is err %d\n",pstDspInfo->languageType);
    }

    stBlockPrm.u32StringLenMax = strlen("(MAINX  ,AUXX  )");
    stBlockPrm.u32OsdNum = OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX * OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX;
    s32Ret = osd_blockInit(OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL, &stBlockPrm);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd block[%u] init fail\n", OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL);
        SAL_memfree(stOsdArray.pstOsdPrm, OSD_FUNC_NAME, OSD_MEM_NAME);
        return SAL_FAIL;
    }

    stOsdArray.u32StringNum = 0;
    for (i = 0; i < OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX; i++)
    {
        for (j = 0; j < OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX; j++)
        {
            pstOsdPrm = stOsdArray.pstOsdPrm + stOsdArray.u32StringNum++;
            if(0 == i && 0 != j)
            {
                snprintf(aszString[i+j*OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX], 32 , "(%sX%2u)",g_stringLanguageMainDux[pstDspInfo->languageType][1], j);
            }
            else if(0 != i && 0 == j)
            {
                snprintf(aszString[i+j*OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX], 32 , "(%sX%2u)",g_stringLanguageMainDux[pstDspInfo->languageType][0], i);
            }
            else
            {
                snprintf(aszString[i+j*OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX], 32 , "(%sX%2u,%sX%2u)",g_stringLanguageMainDux[pstDspInfo->languageType][0], i,g_stringLanguageMainDux[pstDspInfo->languageType][1], j);
            }
            pstOsdPrm->u32Idx = i+j*OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX;
            pstOsdPrm->szString = aszString[i+j*OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX];
            pstOsdPrm->u32Color = OSD_COLOR_INVALID;
            pstOsdPrm->u32BgColor = OSD_COLOR_INVALID;
            pstOsdPrm->enEncFormat = ENC_FMT_GB2312;
        }
    }

    s32Ret = osd_func_blockSet(OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL, &stOsdArray);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("osd block[%u] set fail\n", OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL);
        SAL_memfree(stOsdArray.pstOsdPrm, OSD_FUNC_NAME, OSD_MEM_NAME);
        return SAL_FAIL;
    }
#endif

    SAL_memfree(stOsdArray.pstOsdPrm, OSD_FUNC_NAME, OSD_MEM_NAME);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_func_getNumLatRegion
* 描  述  : 获取数字点阵的地址
* 输  入  : UINT32 u32Num : 数字，小于100
          UINT32 u32LatSize : 点阵大小
          BOOL bParen : 是否带括号
          UINT32 u32SubNum : 辅数字，小于OSD_IDX_SUBNUM_MAX
* 输  出  : OSD_REGION_S **ppstRegion : 返回点阵的region
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
inline OSD_REGION_S *osd_func_getNumLatRegion(UINT32 u32Block,UINT32 u32Num, UINT32 u32SubNum, UINT32 u32LatSize)
{
    UINT32 u32Osd = 0;

    switch (u32Block) 
    {
        case OSD_BLOCK_IDX_NUM_PAREN:
        case OSD_BLOCK_IDX_NUM_MUL:
        {
            u32Num = (u32Num < OSD_IDX_NUM_MAX) ? u32Num : OSD_IDX_NUM_MAX;
            u32Osd = u32Num;
            break;
        }
        case OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT:
        {
            /* (%u.%u) */
            u32Num = (u32Num < OSD_IDX_NUM_MAX) ? u32Num : OSD_IDX_NUM_MAX;
            u32SubNum = (u32SubNum < OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT_IDX_SUBNUM_MAX) ? u32SubNum : 0;
            u32Osd = u32Num + (u32SubNum * OSD_IDX_NUM_MAX);
            break;
        }
#ifdef DSP_ISA
        case OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL:
        {
            /* (主X%u，侧X%u) */
            u32Num = (u32Num < OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX) ? u32Num : OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX;
            u32SubNum = (u32SubNum < OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX) ? u32SubNum : OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX;
            u32Osd = u32Num + (u32SubNum * OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL_IDX_NUM_MAX);
            break;
        }
#endif
        default:
        {
            OSD_LOGE("no block %u\n", u32Block);
            break;
        }
    }
    return osd_func_getLatRegion(u32Block, u32Osd, u32LatSize);
}

/*******************************************************************************
* 函数名  : osd_initVarBlock
* 描  述  : 初始化OSD内容动态变化的block
* 输  入  : UINT32 u32Block : block索引号
          OSD_VAR_BLOCK_S *pstBlockPrm : block参数
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
static INT32 osd_initVarBlock(UINT32 u32Block, OSD_VAR_BLOCK_S *pstBlockPrm)
{
    INT32 s32Ret = 0;
    char szMmzName[20];
    //char *szZoneName = (DRAW_MOD_DSP == capb_get_osd()->enDrawMod) ? "dsp" : NULL;
    char *szZoneName = NULL;
    OSD_ADDR_S stAddr;
    OSD_VAR_BLOCK_S *pstBlock = NULL;
    OSD_REGION_S *pstRegion = NULL;
    UINT32 u32Size = 0;
    UINT8 *pu8Addr = NULL;
    UINT32 i = 0;

    if ((u32Block >= OSD_VAR_BLOCK_NUM_MAX) || (NULL == pstBlockPrm))
    {
        OSD_LOGE("osd var block init invalid para: block:%u[max:%d],  %p\n",
                 u32Block, OSD_VAR_BLOCK_NUM_MAX, pstBlockPrm);
        return SAL_FAIL;
    }

    if (NULL != g_stOsdVarBlocks.apstOsdVarBlock[u32Block])
    {
        OSD_LOGW("osd block[%u] has been initialized\n", u32Block);
        return SAL_SOK;
    }

    g_stOsdVarBlocks.apstOsdVarBlock[u32Block] = SAL_memZalloc(sizeof(OSD_VAR_BLOCK_S), OSD_FUNC_NAME, OSD_MEM_NAME);
    if (NULL == g_stOsdVarBlocks.apstOsdVarBlock[u32Block])
    {
        OSD_LOGE("osd block[%u] malloc fail\n", u32Block);
        return SAL_FAIL;
    }

    pstBlock = g_stOsdVarBlocks.apstOsdVarBlock[u32Block];
    memcpy(pstBlock, pstBlockPrm, sizeof(*pstBlockPrm));
    pstBlock->pfFillLat = (OSD_FONT_TRUETYPE == capb_get_osd()->enFontType) ?
                          osd_freetype_FillLattice : osd_Hzk_FillLattice;
    pstBlock->pstLatRegion = NULL;
    pstBlock->pstFontRegion = NULL;

    /* 点阵 */
    u32Size = SAL_align(osd_func_getLatBuffLen(pstBlock->u32LatSizeMax, pstBlock->u32StringLenMax), 16);
    if (0 != pstBlock->u32LatNum)
    {
        pu8Addr = SAL_memZalloc(sizeof(OSD_REGION_S) * pstBlock->u32LatNum, OSD_FUNC_NAME, OSD_MEM_NAME);
        if (NULL == pu8Addr)
        {
            OSD_LOGE("osd block[%u] malloc osd region buff[%zu] fail\n", u32Block, sizeof(OSD_REGION_S) * pstBlock->u32LatNum);
            SAL_memfree(g_stOsdVarBlocks.apstOsdVarBlock[u32Block], OSD_FUNC_NAME, OSD_MEM_NAME);
            g_stOsdVarBlocks.apstOsdVarBlock[u32Block] = NULL;
            return SAL_FAIL;
        }

        sprintf(szMmzName, "osd_var_lat%u", u32Block);
        szZoneName = NULL;
        s32Ret = mem_hal_mmzAlloc(u32Size * pstBlock->u32LatNum, OSD_FUNC_NAME, szMmzName, szZoneName, SAL_FALSE, &stAddr.u64PhyAddr, (VOID **)&stAddr.pu8VirAddr);
        if (SAL_SOK != s32Ret)
        {
            OSD_LOGE("alloc osd lattice buff[%u] fail:0x%X\n", u32Size * pstBlock->u32LatNum, s32Ret);
            SAL_memfree(pu8Addr, OSD_FUNC_NAME, OSD_MEM_NAME);
            SAL_memfree(g_stOsdVarBlocks.apstOsdVarBlock[u32Block], OSD_FUNC_NAME, OSD_MEM_NAME);
            g_stOsdVarBlocks.apstOsdVarBlock[u32Block] = NULL;
            return SAL_FAIL;
        }

        pstBlock->pstLatRegion = (OSD_REGION_S *)pu8Addr;
        pstRegion = pstBlock->pstLatRegion;
        for (i = 0; i < pstBlock->u32LatNum; i++, pstRegion++)
        {
            pstRegion->stAddr.pu8VirAddr = stAddr.pu8VirAddr;
            pstRegion->stAddr.u64PhyAddr = stAddr.u64PhyAddr;
            pstRegion->u32Size = u32Size;

            stAddr.pu8VirAddr += u32Size;
            stAddr.u64PhyAddr += u32Size;
        }
    }

    /* 字体 */
    u32Size = SAL_align(osd_func_getFontBuffLen(pstBlock->u32FontSizeMax, pstBlock->u32StringLenMax), 16);
    if (0 != pstBlock->u32FontNum)
    {
        pstRegion = pstBlock->pstLatRegion;

        pu8Addr = SAL_memZalloc(sizeof(OSD_REGION_S) * pstBlock->u32FontNum, OSD_FUNC_NAME, OSD_MEM_NAME);
        if (NULL == pu8Addr)
        {
            OSD_LOGE("osd block[%u] malloc osd region buff[%zu] fail\n", u32Block, sizeof(OSD_REGION_S) * pstBlock->u32FontNum);
            if (0 != pstBlock->u32LatNum)
            {
                mem_hal_mmzFree(pstRegion->stAddr.pu8VirAddr, OSD_FUNC_NAME, szMmzName);
                SAL_memfree(pstBlock->pstLatRegion, OSD_FUNC_NAME, OSD_MEM_NAME);
                pstBlock->pstLatRegion = NULL;
            }

            free(g_stOsdVarBlocks.apstOsdVarBlock[u32Block]);
            g_stOsdVarBlocks.apstOsdVarBlock[u32Block] = NULL;

            return SAL_FAIL;
        }

        sprintf(szMmzName, "osd_var_font%u", u32Block);
        s32Ret = mem_hal_mmzAlloc(u32Size * pstBlock->u32FontNum, OSD_FUNC_NAME, szMmzName, NULL, SAL_FALSE, &stAddr.u64PhyAddr, (VOID **)&stAddr.pu8VirAddr);
        if (SAL_SOK != s32Ret)
        {
            OSD_LOGE("alloc osd lattice buff[%u] fail:0x%X\n", u32Size * pstBlock->u32FontNum, s32Ret);
            if (0 != pstBlock->u32LatNum)
            {
                mem_hal_mmzFree(pstRegion->stAddr.pu8VirAddr, OSD_FUNC_NAME, szMmzName);
                SAL_memfree(pstBlock->pstLatRegion, OSD_FUNC_NAME, OSD_MEM_NAME);
                pstBlock->pstLatRegion = NULL;
            }

            SAL_memfree(pu8Addr, OSD_FUNC_NAME, OSD_MEM_NAME);
            SAL_memfree(g_stOsdVarBlocks.apstOsdVarBlock[u32Block], OSD_FUNC_NAME, OSD_MEM_NAME);
            g_stOsdVarBlocks.apstOsdVarBlock[u32Block] = NULL;
            return SAL_FAIL;
        }

        pstBlock->pstFontRegion = (OSD_REGION_S *)pu8Addr;
        pstRegion = pstBlock->pstFontRegion;
        for (i = 0; i < pstBlock->u32FontNum; i++, pstRegion++)
        {
            pstRegion->stAddr.pu8VirAddr = stAddr.pu8VirAddr;
            pstRegion->stAddr.u64PhyAddr = stAddr.u64PhyAddr;
            pstRegion->u32Size = u32Size;

            stAddr.pu8VirAddr += u32Size;
            stAddr.u64PhyAddr += u32Size;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : osd_func_getFreeVarBlock
* 描  述  : 获取未使用的block号并初始化
* 输  入  :
* 输  出  :
* 返回值  : SAL_SOK   : 成功
          SAL_FAIL : 失败
*******************************************************************************/
INT32 osd_func_getFreeVarBlock(OSD_VAR_BLOCK_S *pstBlockPrm, UINT32 *pu32Block)
{
    OSD_VAR_BLOCKS_S *pstBlocks = &g_stOsdVarBlocks;
    UINT32 i = 0;

    if (NULL == pu32Block)
    {
        OSD_LOGE("null pointer\n");
        return SAL_FAIL;
    }

    if (SAL_FALSE == pstBlocks->bUse)
    {
        (VOID)pthread_mutex_init(&pstBlocks->stMutex, NULL);
        pstBlocks->bUse = SAL_TRUE;
        memset(pstBlocks->apstOsdVarBlock, 0, sizeof(pstBlocks->apstOsdVarBlock));
    }

    (VOID)pthread_mutex_lock(&pstBlocks->stMutex);
    for (i = 0; i < OSD_VAR_BLOCK_NUM_MAX; i++)
    {
        if (NULL == pstBlocks->apstOsdVarBlock[i])
        {
            *pu32Block = i;
            break;
        }
    }

    if (i >= OSD_VAR_BLOCK_NUM_MAX)
    {
        OSD_LOGE("osd var block is not enough\n");
        (VOID)pthread_mutex_unlock(&pstBlocks->stMutex);
        return SAL_FAIL;
    }

    if (SAL_SOK != osd_initVarBlock(*pu32Block, pstBlockPrm))
    {
        OSD_LOGE("osd block init fail\n");
        (VOID)pthread_mutex_unlock(&pstBlocks->stMutex);
        return SAL_FAIL;
    }

    (VOID)pthread_mutex_unlock(&pstBlocks->stMutex);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : OSD_VarGetLatRegion
* 描  述  : 获取动态变化的OSD的集合
* 输  入  : UINT32 u32Block : block索引值
          UINT32 u32Num : 获取的个数
* 输  出  :
* 返回值  : NULL : 失败
          非NULL : 成功
*******************************************************************************/
OSD_REGION_S *osd_func_getVarLatRegionSet(UINT32 u32Block, UINT32 u32Num)
{
    OSD_VAR_BLOCK_S *pstBlock = NULL;

    if ((u32Block >= OSD_VAR_BLOCK_NUM_MAX) || (0 == u32Num))
    {
        OSD_LOGE("invalid input para, block[%u], num[%u]\n", u32Block, u32Num);
        return NULL;
    }

    if (NULL == g_stOsdVarBlocks.apstOsdVarBlock[u32Block])
    {
        OSD_LOGW("osd block[%u] has not been initialized\n", u32Block);
        return NULL;
    }

    pstBlock = g_stOsdVarBlocks.apstOsdVarBlock[u32Block];
    if (u32Num > pstBlock->u32LatNum)
    {
        OSD_LOGE("invalid input para, block[%u], num[%u], max[%u]\n", u32Block, u32Num, pstBlock->u32LatNum);
        return NULL;
    }

    return pstBlock->pstLatRegion;
}

/**
 * @function   osd_func_getVarFontRegionSet
 * @brief      获取动态变化的OSD的集合
 * @param[in]  UINT32 u32Block block索引值
 * @param[in]  UINT32 u32Num 获取的个数
 * @param[out]  None
 * @return     None
 */
OSD_REGION_S *osd_func_getVarFontRegionSet(UINT32 u32Block, UINT32 u32Num)
{
    OSD_VAR_BLOCK_S *pstBlock = NULL;

    if (u32Block >= OSD_VAR_BLOCK_NUM_MAX)
    {
        OSD_LOGE("invalid input para, block[%u]\n", u32Block);
        return NULL;
    }

    if (NULL == g_stOsdVarBlocks.apstOsdVarBlock[u32Block])
    {
        OSD_LOGW("osd block[%u] has not been initialized\n", u32Block);
        return NULL;
    }

    pstBlock = g_stOsdVarBlocks.apstOsdVarBlock[u32Block];
    if (u32Num > pstBlock->u32FontNum)
    {
        OSD_LOGE("invalid input para, block[%u], num[%u], max[%u]\n", u32Block, u32Num, pstBlock->u32FontNum);
        return NULL;
    }

    return pstBlock->pstFontRegion;
}

/**
 * @function   osd_func_getLatBuffLen
 * @brief      获取点阵的缓存大小
 * @param[in]  UINT32 u32LatSize
 * @param[in]  UINT32 u32StrLen
 * @return     None
 */
inline UINT32 osd_func_getLatBuffLen(UINT32 u32LatSize, UINT32 u32StrLen)
{
    return ((u32LatSize * u32LatSize * u32StrLen + 7) >> 3) + u32LatSize * 2;
}

/**
 * @function   osd_func_getFontBuffLen
 * @brief      获取字体的缓存大小
 * @param[in]  UINT32 u32FontSize
 * @param[in]  UINT32 u32StrLen
 * @return     None
 */
inline UINT32 osd_func_getFontBuffLen(UINT32 u32FontSize, UINT32 u32StrLen)
{
    return u32FontSize * u32FontSize * u32StrLen * 2;
}

/**
 * @function   osd_func_getFontByString
 * @brief      
 * @param[in]  CHAR *szString           字符串
 * @param[in]  OSD_REGION_S *pstLat     点阵缓存，调用前对size和addr赋值
 * @param[in]  OSD_REGION_S *pstFont    字体缓存，调用前对size和addr赋值
 * @param[in]  UINT32 u32Size           字体大小
 * @param[in]  UINT16 u16Color          字体颜色
 * @param[in]  UINT32 u16BgColor        背景色
 * @param[out] OSD_REGION_S *pstLat     点阵缓存，返回宽，高，跨距
 * @param[out] OSD_REGION_S *pstFont    字体缓存，返回宽，高，跨距
 * @param[out] ENCODING_FORMAT_E enEncFormat : 编码类型
 * @return     None
 */
INT32 osd_func_getFontByString(CHAR *szString, OSD_REGION_S *pstLat, OSD_REGION_S *pstFont, UINT32 u32Size, UINT16 u16Color, UINT32 u16BgColor, ENCODING_FORMAT_E enEncFormat)
{
    if ((NULL == szString) || (NULL == pstLat) || (0 == u32Size))
    {
        OSD_LOGE("invalid input para, string:%p, lat rgn:%p, size:%u\n", szString, pstLat, u32Size);
        return SAL_FAIL;
    }

    if (SAL_SOK != osd_freetype_FillLattice(szString, pstLat, u32Size, enEncFormat))
    {
        OSD_LOGE("%s osd fill lattice fail\n", szString);
        return SAL_FAIL;
    }

    if (NULL != pstFont)
    {
        if (SAL_SOK != osd_func_FillFont(pstLat, u32Size, pstFont, u32Size, u16Color, u16BgColor))
        {
            OSD_LOGE("%s osd fill font fail\n", szString);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}
