/*******************************************************************************
 * jpeg_soft.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : wangweiqin <wangweiqin@hikvision.com.cn>
 * Version: V1.0.0  2018年01月16日 Create
 *
 * Description : JPEG 软件解码
 * Modification:
 *******************************************************************************/

#include "jdec_soft.h"
#include "dspcommon.h"


/*****************************************************************************
                            宏定义
*****************************************************************************/
#define UP_ALIGN(_VAL_,_A_) (((_VAL_) + _A_ -1) & (~(_A_ -1)))


/*****************************************************************************
                            全局结构体
*****************************************************************************/

/*****************************************************************************
                            函数
*****************************************************************************/


void FreeMemTab(unsigned char * pBuf[], HKA_MEM_TAB stMemTab[], int uiMemSize)
{
    int i = 0;

    for (i = 0; i < uiMemSize; i++)
    {
        if (NULL != pBuf[i])
        {
            SAL_memfree(pBuf[i], "jdec_soft", "dec_buf");
            pBuf[i] = NULL;
        }
    }

    return;
}


void MallocMemTab(unsigned char * pBuf[], HKA_MEM_TAB stMemTab[], int uiMemSize)
{
    int i    = 0;
    //int size = 0;

    for (i = 0; i < uiMemSize; i++)
    {
        if (0 != stMemTab[i].size)
        {
            pBuf[i] = (unsigned char*)SAL_memMalloc(UP_ALIGN(stMemTab[i].size, 128), "jdec_soft", "dec_buf");
            if (NULL == pBuf[i])
            {
                SAL_ERROR("dec memTab alloc failed!\n");
                return;
            }

            memset(pBuf[i],0x0,stMemTab[i].size);
        }
    }
    return;
}


/*****************************************************************************
 函 数 名  : HikJpgDec
 功能描述  : JPEG 软解
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年01月18日
    作    者   : wwq
    修改内容   : 从 TX1 移植
*****************************************************************************/
int HikJpgDec(unsigned char* inBuf,unsigned int inSize,unsigned char* outBuf,unsigned int *outbuflen,unsigned int* width,unsigned int* height)
{
    //int                i    = 0;
    unsigned char*     m_pDecBuf[2];
    static unsigned char* m_pFrameBuf      = NULL;
    //static unsigned int   m_uiFrameBufLen = 0;
    void*              m_hDecoder = NULL;
    unsigned int       nYLen;
    HKA_STATUS         nRet = HIK_VIDEO_DEC_LIB_S_OK;
    HKAJPGD_ABILITY    stAbility = { 0 };
    HKA_MEM_TAB        stMemTab[HKA_MEM_TAB_NUM] = { 0 };
    HKAJPGD_STREAM     stStream;
    //HKAJPGD_IMAGE_INFO stImageInfo = { 0 };

    HKAJPGD_OUTPUT stOutput    = { 0 };

    if((NULL == inBuf) || (0 == inSize))
    {
        SAL_ERROR("InSize %d \n", inSize);
        return SAL_FAIL;
    }

    if (NULL == outBuf)
    {
        SAL_ERROR("outBuf is NULL! \n");
        return SAL_FAIL;
    }

    stStream.stream_buf = inBuf;
    stStream.stream_len = inSize;
    nRet = HKAJPGD_GetImageInfo(&stStream, &stAbility.image_info);
    if (nRet != HIK_VIDEO_DEC_LIB_S_OK)
    {
        SAL_ERROR("HKAJPGD_GetImageInfo err %x\n",nRet);
        return SAL_FAIL;
    }

    if ((stAbility.image_info.img_size.width > HIK_DIC_MAX_WIDTH) || (stAbility.image_info.img_size.height > HIK_DIC_MAX_HEIGHT))
    {
        SAL_ERROR("Image is Large : Width %d Height %d !!!\n", stAbility.image_info.img_size.width , stAbility.image_info.img_size.height);
        return SAL_FAIL;
    }

    nRet = HKAJPGD_GetMemSize(&stAbility, stMemTab);
    if (nRet != HIK_VIDEO_DEC_LIB_S_OK)
    {
        SAL_ERROR("HKAJPGD_GetMemSize err %x\n",nRet);
        return SAL_FAIL;
    }

    MallocMemTab(m_pDecBuf, stMemTab, 2);

    stMemTab[0].base = m_pDecBuf[0];
    stMemTab[1].base = m_pDecBuf[1];

    nRet = HKAJPGD_Create(&stAbility, stMemTab, &m_hDecoder);
    if (nRet != HIK_VIDEO_DEC_LIB_S_OK)
    {
        SAL_ERROR("HKAJPGD_Create failed\n");
        FreeMemTab(m_pDecBuf, stMemTab, 2);
        return SAL_FAIL;
    }

    nYLen = UP_ALIGN(stAbility.image_info.img_size.width,2) * UP_ALIGN(stAbility.image_info.img_size.height,2);

    m_pFrameBuf = outBuf;

    stOutput.image_out.data[0] = m_pFrameBuf;
    stOutput.image_out.data[1] = m_pFrameBuf + nYLen;
    stOutput.image_out.data[2] = m_pFrameBuf + nYLen * 5 / 4;

    nRet = HKAJPGD_Process(m_hDecoder, &stStream, sizeof(HKAJPGD_STREAM), &stOutput, sizeof(HKAJPGD_OUTPUT));
    if (HIK_VIDEO_DEC_LIB_S_OK == nRet)
    {
        if (((stAbility.image_info.img_size.width & (~1)) != stOutput.image_out.width) ||
            ((stAbility.image_info.img_size.height & (~1)) != stOutput.image_out.height) ||
            HKA_IMG_YUV_I420 != stOutput.image_out.format)
        {
            SAL_ERROR("resolution or format err! %d %d ,%d %d,%d \n",stAbility.image_info.img_size.width,stAbility.image_info.img_size.height,
            stOutput.image_out.width,stOutput.image_out.height,stOutput.image_out.format);
            FreeMemTab(m_pDecBuf, stMemTab, 2);
            return SAL_FAIL;
        }
    }
    else
    {
        SAL_INFO("HKAJPGD_Process err %x\n",nRet);
        FreeMemTab(m_pDecBuf, stMemTab, 2);
        return SAL_FAIL;
    }

    *outbuflen = nYLen * 3 / 2;
    *width     = stOutput.image_out.width;
    *height    = stOutput.image_out.height;

    FreeMemTab(m_pDecBuf, stMemTab, 2);

    return SAL_SOK;
}

