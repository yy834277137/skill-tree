
/*******************************************************************************
* char_code.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : cuifeng5
* Version: V1.0.0  2020年12月16日 Create
*
* Description : 字符编码格式相关操作
* Modification:
*******************************************************************************/

#include <iconv.h>

#include "sal.h"

/*******************************************************************************
* 函数名  : CodeConvert
* 描  述  : 编码格式转换，不做参数检查，上层保证入参有效
* 输  入  : char *szSrcCode : 源编码格式
          char *szDstCode : 目标编码格式
          char *szGbBuff : 源缓存
          UINT32 u32GbSize : 源字符串长度
          char *szUniBuff : 目标输出缓存
          UINT32 u32UniSize : 目标缓存大小
* 输  出  : 无
* 返回值  : 输出缓存地址
*******************************************************************************/
static char *CodeConvert(char *szSrcCode, char *szDstCode, char *szSrcBuff, UINT32 u32SrcSize, char *szDstBuff, UINT32 u32DstSize)
{
    iconv_t cd = NULL;
    char *szSrc = szSrcBuff;
    char *szDst = szDstBuff;
    size_t src  = u32SrcSize;
    size_t dst  = u32DstSize;
    
    cd = libiconv_open(szDstCode, szSrcCode);
    if ((iconv_t)-1 == cd)
    {
        ICONV_LOGE("iconv open %s to UCS-2LE fail\n", szSrcCode);
        return NULL;
    }
    
    if ((size_t)(-1) == libiconv(cd, &szSrc, &src, &szDst, &dst))
    {
        ICONV_LOGE("iconv fail:%s\n", strerror(errno));
        iconv_close(cd);
        return NULL;
    }

    libiconv_close(cd);
    return szDstBuff;
}

/*******************************************************************************
* 函数名  : UCS2LE_GetCodeNum
* 描  述  : 获取UCS-2LE字符串的字符数
* 输  入  : UINT8 *pu8String : UCS-2LE字符串，字符串尾部预留2个字节的0x00防止越界
* 输  出  : 无
* 返回值  : 字符个数，一个文字算一个字符
*******************************************************************************/
UINT32 UCS2LE_GetCodeNum(UINT8 *pu8String)
{
    UINT32 u32CodeNum = 0;
    UINT16 *pu16String = (UINT16 *)pu8String;

    while (*pu16String)
    {
        u32CodeNum++;
        pu16String++;
    }

    return u32CodeNum;
}

/*******************************************************************************
* 函数名  : Tran2Unicode
* 描  述  : 国标和UTF-8转Unicode，不做参数检查，上层保证入参有效
* 输  入  : enEncFormat :源数据编码类型
            char *szSrcBuff : 源字符串缓存
            UINT32 u32SrcSize : 源字符串长度
            char *szUniBuff : unicode输出缓存
            UINT32 u32UniSize : unicode缓存大小
* 输  出  : 无
* 返回值  : 输出缓存地址
*******************************************************************************/
char *Tran2Unicode(ENCODING_FORMAT_E enEncFormat,char *szSrcBuff, UINT32 u32SrcSize, char *szUniBuff, UINT32 u32UniSize)
{
    if(enEncFormat == ENC_FMT_UTF8)
    {
        return CodeConvert("UTF-8", "UCS-2LE", szSrcBuff, u32SrcSize, szUniBuff, u32UniSize);
    }
    else if(enEncFormat == ENC_FMT_GB2312)
    {
        return CodeConvert("GB2312", "UCS-2LE", szSrcBuff, u32SrcSize, szUniBuff, u32UniSize);
    }
    else if(enEncFormat == ENC_FMT_ISO_8859_6)
    {
        return CodeConvert("ISO-8859-6", "UCS-2LE", szSrcBuff, u32SrcSize, szUniBuff, u32UniSize);
    }
    else
    {
        FREETYPE_LOGE("enEncFormat err! enEncFormat:%d\n", enEncFormat);
        return NULL;
    }
}

/*******************************************************************************
* 函数名  : GB_GetCodeNum
* 描  述  : 获取国标编码字符串的字符数，不做参数检查，上层保证入参有效
* 输  入  : UINT8 *pu8String : 国标编码的字符串
* 输  出  : 无
* 返回值  : 字符个数，一个汉字算一个字符
*******************************************************************************/
UINT32 GB_GetCodeNum(UINT8 *pu8String)
{
    UINT32 u32CodeNum = 0;

    while (*pu8String)
    {
        pu8String += (*pu8String > 0x7F) ? 2 : 1;
        u32CodeNum++;
    }

    return u32CodeNum;
}



