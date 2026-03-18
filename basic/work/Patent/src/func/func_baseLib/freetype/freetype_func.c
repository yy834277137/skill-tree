
/*******************************************************************************
* freetype.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : cuifeng5
* Version: V1.0.0  2020年12月11日 Create
*
* Description : freetype渲染字体相关接口
* Modification:
*******************************************************************************/

#include <ft2build.h>
#include FT_FREETYPE_H

#include "sal.h"
#include "ft2build.h"
#include "freetype_func.h"
#include "harfbuzz_func.h"
#include "hb.h"
#include "char_code.h"

#define FREETYPE_FACE_SAFE_DONE(face)       do {(VOID)FT_Done_Face(face); face = NULL;}while (0)
#define FREETYPE_SAFE_DONE(lib)             do {(VOID)FT_Done_FreeType(lib); lib = NULL;}while (0)

static FT_Library g_ftLibrary = NULL;
static FT_Face g_ftFace = NULL;

extern DSPINITPARA *SystemPrm_getDspInitPara(void);

/*******************************************************************************
* 函数名  : Freetype_SetFace
* 描  述  : 设置字库
* 输  入  : FREETYPE_FACE_PRM_S *pstFace：字体存放位置
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 Freetype_SetFace(FREETYPE_FACE_PRM_S *pstFace)
{
	FT_Error ftError = FT_Err_Ok;

    if (NULL == pstFace)
    {
        FREETYPE_LOGE("freetype init prm null\n");
        return SAL_FAIL;
    }

    HB_Init(pstFace->stMemory.pvAddr, pstFace->stMemory.u32Size);

    /* 如果已经指定了字库，销毁原来字库，更新字库 */
    if (NULL != g_ftFace)
    {
        FREETYPE_FACE_SAFE_DONE(g_ftFace);
    }
    if (NULL != g_ftLibrary)
    {
        FREETYPE_SAFE_DONE(g_ftLibrary);
    }

    ftError = FT_Init_FreeType(&g_ftLibrary);
    if (FT_Err_Ok != ftError)
    {
        FREETYPE_LOGE("freetype init fail:%s\n", FT_Error_String(ftError));
        return SAL_FAIL;
    }

    if (FREETYPE_FACE_LOAD_TYPE_FILE == pstFace->enLoadType)
    {
        ftError = FT_New_Face(g_ftLibrary, pstFace->szFontName, 0, &g_ftFace);
        if (FT_Err_Ok != ftError)
        {
            FREETYPE_LOGE("freetype load font file[%s] fail:%s\n", pstFace->szFontName, FT_Error_String(ftError));
            goto error_lib;
        }
    }
    else if (FREETYPE_FACE_LOAD_TYPE_MEMORY == pstFace->enLoadType)
    {
        ftError = FT_New_Memory_Face(g_ftLibrary, (const FT_Byte*)pstFace->stMemory.pvAddr, 
                                     pstFace->stMemory.u32Size, 0, &g_ftFace);
        if (FT_Err_Ok != ftError)
        {
            FREETYPE_LOGE("freetype load font memory[%p] fail:%s\n", pstFace->stMemory.pvAddr, FT_Error_String(ftError));
            goto error_lib;
        }
    }

    return SAL_SOK;
/*
error_face:
    (VOID)FT_Done_Face(g_ftFace);
    g_ftFace = NULL;
*/

error_lib:
    FREETYPE_SAFE_DONE(g_ftLibrary);
    
    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Freetype_DeInit
* 描  述  : 销毁freetype相关资源
* 输  入  : 
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 Freetype_DeInit(VOID)
{
    FREETYPE_FACE_SAFE_DONE(g_ftFace);
    FREETYPE_SAFE_DONE(g_ftLibrary);
    HB_Deinit();

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Freetype_SetCharSize
* 描  述  : 设置字体大小
* 输  入  : UINT32 u32Size:单位:像素点
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 Freetype_SetCharSize(UINT32 u32Size)
{
    FT_Error ftError = FT_Err_Ok;

    ftError = FT_Set_Pixel_Sizes(g_ftFace, 0, u32Size);
    if (FT_Err_Ok != ftError)
    {
        FREETYPE_LOGE("freetype set pixel size fail:%s\n", FT_Error_String(ftError));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Freetype_GetCharFont
* 描  述  : 获取对应字符的图形
* 输  入  : UINT32 u32Code: unicode
* 输  出  : 无
* 返回值  : 对应字符的图形，失败返回NULL
*******************************************************************************/
static FT_GlyphSlot Freetype_GetCharFont(UINT32 u32Code, hb_glyph_info_t *pInfo)
{
    FT_Error ftError = FT_Err_Ok;
    FT_Face ftFace = g_ftFace;
    // FT_UInt ftGlyphIdx = 0;

    // ftGlyphIdx = FT_Get_Char_Index(ftFace, u32Code);
    // if (0 == ftGlyphIdx)
    // {
    //     FREETYPE_LOGE("freetype get char idx[%u] fail:%s\n", u32Code, FT_Error_String(ftError));
    //     return NULL;
    // }

    ftError = FT_Load_Glyph(ftFace, pInfo->codepoint, FT_LOAD_DEFAULT);
    if (FT_Err_Ok != ftError)
    {
        FREETYPE_LOGE("freetype load glyph fail:%s\n", FT_Error_String(ftError));
        return NULL;
    }
#if 0
    FT_Outline_Embolden(&(ftFace->glyph->outline), -32);
    if (FT_Err_Ok != ftError)
    {
        FREETYPE_LOGE("freetype Outline_Embolden fail:%s\n", FT_Error_String(ftError));
        return NULL;
    }
#endif
    if (FT_GLYPH_FORMAT_BITMAP != ftFace->glyph->format)
    {
        ftError = FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_MONO);
        if (FT_Err_Ok != ftError)
        {
            FREETYPE_LOGE("freetype render glyph fail:%s\n", FT_Error_String(ftError));
            return NULL;
        }

    }
#if 0
	ftError = FT_Bitmap_Embolden(g_ftLibrary, &(ftFace->glyph->bitmap), 32, 32);
	if (FT_Err_Ok != ftError)
		{
			printf("bitmap_buf=%p,bitmap_width=%d\n",ftFace->glyph->bitmap.buffer,ftFace->glyph->bitmap.width);
			printf("ftError=0x%x\n",ftError);
			FREETYPE_LOGE("freetype Bitmap_Embolden fail:%s\n", FT_Error_String(ftError));
			return NULL;
		}
#endif
    return ftFace->glyph;
}

/*******************************************************************************
* 函数名  : Freetype_DrawBitMap
* 描  述  : 将位图拷贝到对应内存中
* 输  入  : FT_GlyphSlot ftGlyph : 对应的位图
          UINT32 u32BaseLine : 基准线，水平方向
          FREETYPE_BLOCK_S *pstBlock : 拷贝的目标内存块
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 Freetype_DrawBitMap(FT_GlyphSlot ftGlyph, UINT32 u32BaseLine, FREETYPE_BLOCK_S *pstBlock)
{
    UINT32 i = 0, j = 0;
    UINT32 u32Pitch  = ftGlyph->bitmap.pitch;
    UINT32 u32Stride = pstBlock->u32Stride << 3;
    UINT32 u32Width  = ftGlyph->bitmap.width;
    UINT32 u32Bytes  = (u32Width + 7) >> 3;
    UINT32 u32Height = ftGlyph->bitmap.rows;
    UINT32 u32XStart = pstBlock->u32Width + ftGlyph->bitmap_left;
    INT32 s32YStart = (INT32)u32BaseLine - 1 > ftGlyph->bitmap_top ? (INT32)u32BaseLine - 1 - ftGlyph->bitmap_top : 0;
    UINT8 *pu8BitMap = NULL;
    UINT8 *pu8Buff   = NULL;
    FREETYPE_PEN_S stPen;

    if (u32XStart + u32Width > u32Stride)
    {
        FREETYPE_LOGE("freetype char pos[%u] exceed rect width[%u]\n",
                      u32XStart + u32Width, u32Stride);
        return SAL_FAIL;
    }

    if (s32YStart + u32Height > pstBlock->u32Height || s32YStart < 0)
    {
        FREETYPE_LOGE("freetype char exceed rect:baseline[%u], top[%d], char height[%u] rgn height[%u]\n",
                      u32BaseLine, ftGlyph->bitmap_top, u32Height, pstBlock->u32Height);
        return SAL_FAIL;
    }

    /* 计算起始点X坐标 */
    stPen.u32XOffset = (u32XStart >> 3);
    stPen.u8XShift   = (u32XStart & 0x07);

    /* 拷贝位图到缓存中 */
    for (i = 0; i < u32Height; i++)
    {
        pu8BitMap = ftGlyph->bitmap.buffer + i * u32Pitch;
        pu8Buff   = pstBlock->pu8Buffer + (((i + s32YStart) * u32Stride + u32XStart) >> 3);
        for (j = 0; j < u32Bytes; j++)
        {
            *pu8Buff++ |= (*pu8BitMap >> stPen.u8XShift);
            *pu8Buff   |= (*pu8BitMap++ << (8 - stPen.u8XShift));
        }
    }

    pstBlock->u32Width += (ftGlyph->advance.x >> 6);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : GetEncFormatName
* 描  述  : 获取危险品名称编码类型枚举对应的编码类型名称
* 输  入  : ENCODING_FORMAT_E enEncFormat : 编码类型
          CHAR *pname ：输入枚举类型名称字符串地址
          UINT32 u32NameLen : pname指向地址大小
* 输  出  : CHAR *pname ：输出枚举类型名称字符串
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 GetEncFormatName(ENCODING_FORMAT_E enEncFormat, CHAR *pname, UINT32 u32NameLen)
{
    switch (enEncFormat)
    {
        case ENC_FMT_GB2312:
            snprintf(pname, u32NameLen, "GB2312");
            break;
        case ENC_FMT_UTF8:
            snprintf(pname, u32NameLen, "UTF-8");
            break;
        case ENC_FMT_ISO_8859_6:
            snprintf(pname, u32NameLen, "ISO-8859-6");
            break;
        default:
            snprintf(pname, u32NameLen, "NOT SUPPORT TYPE");
            break;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : Freetype_DrawByString
* 描  述  : 生成对应字符串的点阵位图
* 输  入  : FREETYPE_BLOCK_S *pstBlock : 保存位图的目标缓存块
          char *szString : 输入字符串，字符串长度小于128
          UINT32 u32Size : 字体大小
          ENCODING_FORMAT_E enEncFormat : 编码类型
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 Freetype_DrawByString(FREETYPE_BLOCK_S *pstBlock, char *szString, UINT32 u32Size, ENCODING_FORMAT_E enEncFormat)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32Baseline = 0;
    UINT16 u16Code = 0;
    UINT16 *pu16Code = 0;
    UINT32 u32CodeNum = 0;
    FT_GlyphSlot ftGlyph = NULL;
    UINT8 *pu8Unicode = NULL;
    UINT32 u32UnicodeLen = 0;
    UINT32 u32WidthMax = 0;
    UINT32 i = 0;
    hb_glyph_info_t *pInfo = NULL;
    hb_glyph_info_t *infos = NULL;
    

    if (enEncFormat >= ENC_FMT_MAX_CNT)
    {
        FREETYPE_LOGE("sva_name encoding format err! enEncFormat:%d\n", enEncFormat);
        return SAL_FAIL;
    }

    if ((NULL == pstBlock) || (szString == NULL))
    {
        FREETYPE_LOGE("null pointer:%p %p\n", pstBlock, szString);
        return SAL_FAIL;
    }

    if (NULL == g_ftFace)
    {
        FREETYPE_LOGE("freetype face has not init\n");
        return SAL_FAIL;
    }

    /* 设置字体大小 */
    s32Ret = Freetype_SetCharSize(u32Size);
    if (SAL_SOK != s32Ret)
    {
        FREETYPE_LOGE("freetype set char size[%u] fail\n", u32Size);
        return SAL_FAIL;
    }

    /* 字符串水平移动，得到基准线的Y坐标 */
    u32Baseline = g_ftFace->size->metrics.ascender >> 6;
    pstBlock->u32Height = SAL_align(g_ftFace->size->metrics.height >> 6, 2);
    pstBlock->u32Stride = pstBlock->u32Size / pstBlock->u32Height;
    u32WidthMax = pstBlock->u32Stride << 3;

    /* UCS-2LE采用小端模式以2字节表示Unicode中的一个字符,预留2字节防止UCS2LE_GetCodeNum函数出错 */
    u32UnicodeLen = SVA_ALERT_NAME_LEN * 2 + 2;
    pu8Unicode = (UINT8 *)SAL_memMalloc(u32UnicodeLen, "freetype_func", "draw");
    if (NULL == pu8Unicode)
    {
        FREETYPE_LOGE("freetype malloc unicode buff %u bytes fail\n", u32UnicodeLen);
        return SAL_FAIL;
    }
    memset(pu8Unicode, 0x00, u32UnicodeLen);

    if (NULL == Tran2Unicode(enEncFormat, szString, strlen(szString), (char *)pu8Unicode, u32UnicodeLen))
    {
        if (szString[SVA_ALERT_NAME_LEN - 1] != '\0')
        {
            FREETYPE_LOGE("string[%d] is %0x\n", SVA_ALERT_NAME_LEN - 1, szString[SVA_ALERT_NAME_LEN - 1]);
            szString[SVA_ALERT_NAME_LEN - 1] = '\0';
        }
        GetEncFormatName(enEncFormat, (char *)pu8Unicode, u32UnicodeLen); /* 信息打印临时使用pu8Unicode存储编码类型名 */
        FREETYPE_LOGE("string:%s convert %s to unicode fail\n", szString, pu8Unicode);
        SAL_memfree(pu8Unicode, "freetype_func", "draw");
        return SAL_FAIL;
    }

    pstBlock->u32Width = 0;
    pu16Code = (UINT16 *)pu8Unicode;
    u32CodeNum = UCS2LE_GetCodeNum(pu8Unicode); /* 计算字符个数 */

    /* 字符整形 */
    HB_PlasticCode((char *)pu8Unicode, &u32CodeNum, &infos, enEncFormat);

    for (i = 0; i < u32CodeNum; i++)
    {
        u16Code = *pu16Code++;
        pInfo = &infos[i];
        ftGlyph = Freetype_GetCharFont((UINT32)u16Code, pInfo);
        if (NULL == ftGlyph)
        {
            FREETYPE_LOGE("freetype get char[0x%x] font fail\n", u16Code);
            SAL_memfree(pu8Unicode, "freetype_func", "draw");
            return SAL_FAIL;
        }

        if (pstBlock->u32Width + (ftGlyph->advance.x >> 6) > u32WidthMax)
        {
            FREETYPE_LOGE("buf size:%u is too small, stride << 3:%u, width:%u + advance:%lu, height:%u\n",
                          pstBlock->u32Size, pstBlock->u32Stride << 3, pstBlock->u32Width, ftGlyph->advance.x >> 6, pstBlock->u32Height);
            free(pu8Unicode);
            return SAL_FAIL;
        }

        FREETYPE_LOGD("0x%x: width:%u, height:%u, left:%d, top:%d, advance:x:%ld, y:%ld\n",
                      u16Code, ftGlyph->bitmap.width, ftGlyph->bitmap.rows, ftGlyph->bitmap_left, 
                      ftGlyph->bitmap_top, ftGlyph->advance.x >> 6, ftGlyph->advance.y >> 6);
        
        s32Ret = Freetype_DrawBitMap(ftGlyph, u32Baseline, pstBlock);
        if (SAL_SOK != s32Ret)
        {
            FREETYPE_LOGE("freetype draw bitmap[0x%x] fail\n", u16Code);
            SAL_memfree(pu8Unicode, "freetype_func", "draw");
            return SAL_FAIL;
        }
    }
    pstBlock->u32Width = SAL_align(pstBlock->u32Width,4); /* NT平台osg画OSD需要OSD宽度4对齐高度2对齐 */
    SAL_memfree(pu8Unicode, "freetype_func", "draw");

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : Freetype_Print
* 描  述  : 测试代码，打印点阵
* 输  入  : FREETYPE_BLOCK_S *pstBlock
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
VOID Freetype_Print(FREETYPE_BLOCK_S *pstBlock)
{
    UINT32 u32Bytes = 0;
    UINT32 u32Bits  = 0;
    UINT8 *pu8Tmp = NULL;
    UINT8 u8Tmp = 0;
    UINT32 i = 0, j = 0, k = 0;

    u32Bytes = pstBlock->u32Width >> 3;
    u32Bits  = (pstBlock->u32Width & 0x07);
    
    for (i = 0; i < pstBlock->u32Height; i++)
    {
        pu8Tmp = pstBlock->pu8Buffer + (i * pstBlock->u32Stride);
        for (j = 0; j < u32Bytes; j++)
        {
            u8Tmp = *pu8Tmp++;
            for (k = 0; k < 8; k++)
            {
                printf("%c", (u8Tmp & 0x80) ? '*' : ' ');
                u8Tmp <<= 1;
            }
        }

        u8Tmp = *pu8Tmp;
        for (k = 0; k < u32Bits; k++)
        {
            printf("%c", (u8Tmp & 0x80) ? '*' : ' ');
            u8Tmp <<= 1;
        }
        
        printf("\n");
    }
}


/*******************************************************************************
* 函数名  : Freetype_Test
* 描  述  : 测试代码
* 输  入  : 无
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
VOID Freetype_Test(VOID)
{
    INT32 s32Ret = SAL_SOK;
    FREETYPE_FACE_PRM_S stFace;
    FREETYPE_BLOCK_S stBlock;
    
    
    char szBuff[128];
    UINT32 u32SizeStart = 16;
    UINT32 u32SizeEnd   = 48;
    UINT32 u32SizeStep  = 16;
    UINT32 u32Size      = 0;

    stFace.enLoadType = FREETYPE_FACE_LOAD_TYPE_FILE;
    stFace.szFontName = "./msyh.ttf";
    
    s32Ret = Freetype_SetFace(&stFace);
    if (SAL_SOK != s32Ret)
    {
        printf("freetype set face fail\n");
        return;
    }

    stBlock.u32Size = 64 * 1024;
    stBlock.pu8Buffer = SAL_memMalloc(stBlock.u32Size, "freetype_func", "test");
    if (NULL == stBlock.pu8Buffer)
    {
        printf("malloc fail\n");
        return;
    }

    strcpy(szBuff, "海康威视");
    for (u32Size = u32SizeStart; u32Size <= u32SizeEnd; u32Size += u32SizeStep)
    {
        stBlock.u32Width = 0;
        memset(stBlock.pu8Buffer, 0, stBlock.u32Size);
        s32Ret = Freetype_DrawByString(&stBlock, szBuff, u32Size, ENC_FMT_GB2312);
        if (SAL_SOK != s32Ret)
        {
            printf("freetype set face fail\n");
            SAL_memfree(stBlock.pu8Buffer, "freetype_func", "test");
            return;
        }

        Freetype_Print(&stBlock);
    }

    strcpy(szBuff, "abcdefghijklmnopqrstuvwxyz");
    for (u32Size = u32SizeStart; u32Size <= u32SizeEnd; u32Size += u32SizeStep)
    {
        stBlock.u32Width = 0;
        memset(stBlock.pu8Buffer, 0, stBlock.u32Size);
        s32Ret = Freetype_DrawByString(&stBlock, szBuff, u32Size, ENC_FMT_GB2312);
        if (SAL_SOK != s32Ret)
        {
            printf("freetype set face fail\n");
            SAL_memfree(stBlock.pu8Buffer, "freetype_func", "test");
            return;
        }

        Freetype_Print(&stBlock);
    }
    
    strcpy(szBuff, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    for (u32Size = u32SizeStart; u32Size <= u32SizeEnd; u32Size += u32SizeStep)
    {
        stBlock.u32Width = 0;
        memset(stBlock.pu8Buffer, 0, stBlock.u32Size);
        s32Ret = Freetype_DrawByString(&stBlock, szBuff, u32Size, ENC_FMT_GB2312);
        if (SAL_SOK != s32Ret)
        {
            printf("freetype set face fail\n");
            SAL_memfree(stBlock.pu8Buffer, "freetype_func", "test");
            return;
        }

        Freetype_Print(&stBlock);
    }
    
    strcpy(szBuff, "0123456789");
    for (u32Size = u32SizeStart; u32Size <= u32SizeEnd; u32Size += u32SizeStep)
    {
        stBlock.u32Width = 0;
        memset(stBlock.pu8Buffer, 0, stBlock.u32Size);
        s32Ret = Freetype_DrawByString(&stBlock, szBuff, u32Size, ENC_FMT_GB2312);
        if (SAL_SOK != s32Ret)
        {
            printf("freetype set face fail\n");
            SAL_memfree(stBlock.pu8Buffer, "freetype_func", "test");
            return;
        }

        Freetype_Print(&stBlock);
    }
    
    strcpy(szBuff, "~!#$%^&*()_+-=:;,./<>?");
    for (u32Size = u32SizeStart; u32Size <= u32SizeEnd; u32Size += u32SizeStep)
    {
        stBlock.u32Width = 0;
        memset(stBlock.pu8Buffer, 0, stBlock.u32Size);
        s32Ret = Freetype_DrawByString(&stBlock, szBuff, u32Size, ENC_FMT_GB2312);
        if (SAL_SOK != s32Ret)
        {
            printf("freetype set face fail\n");
            SAL_memfree(stBlock.pu8Buffer, "freetype_func", "test");
            return;
        }

        Freetype_Print(&stBlock);
    }
    
    SAL_memfree(stBlock.pu8Buffer, "freetype_func", "test");
    (VOID)Freetype_DeInit();

    return;
}

