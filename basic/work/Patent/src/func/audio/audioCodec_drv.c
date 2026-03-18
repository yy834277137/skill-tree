/**
 * @file   audioCodec_drv.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---编解码接口
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取编解码接口
 */


/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal.h"
#include "sal_mem_new.h"
#include "audio_common.h"
#include "audioCodec_drv_api.h"

#include "audio_codec_common.h"
#include "G711_Codec.h"
#include "aaccodec.h"
#include "platform_hal.h"

/*****************************************************************************
                         结构定义
*****************************************************************************/
typedef struct audAdtsHeader
{
    unsigned int syncword;  /* 12 bit ~{M,2=WV~} '1111 1111 1111'~{#,K5CwR;8v~}ADTS~{V!5D?*J<~} */
    unsigned int id;        /* 1 bit MPEG ~{1jJ>7{#,~} 0 for MPEG-4~{#,~}1 for MPEG-2 */
    unsigned int layer;     /* 2 bit ~{W\JG~}'00' */
    unsigned int protectionAbsent;  /* 1 bit 1~{1mJ>C;SP~}crc~{#,~}0~{1mJ>SP~}crc */
    unsigned int profile;           /* 1 bit ~{1mJ>J9SCDD8v<61p5D~}AAC */
    unsigned int samplingFreqIndex; /* 4 bit ~{1mJ>J9SC5D2IQyF5BJ~} */
    unsigned int privateBit;        /* 1 bit */
    unsigned int channelCfg; /* 3 bit ~{1mJ>Iy5@J}~} */
    unsigned int originalCopy;         /* 1 bit */
    unsigned int home;                  /* 1 bit */

    /*~{OBCf5DN*8D1d5D2NJ}<4C?R;V!6<2;M,~}*/
    unsigned int copyrightIdentificationBit;   /* 1 bit */
    unsigned int copyrightIdentificationStart; /* 1 bit */
    unsigned int aacFrameLength;               /* 13 bit ~{R;8v~}ADTS~{V!5D3$6H0|@(~}ADTS~{M7:M~}AAC~{T-J<Aw~} */
    unsigned int adtsBufferFullness;           /* 11 bit 0x7FF ~{K5CwJGBkBJ?I1d5DBkAw~} */

    /* number_of_raw_data_blocks_in_frame
     * ~{1mJ>~}ADTS~{V!VPSP~}number_of_raw_data_blocks_in_frame + 1~{8v~}AAC~{T-J<V!~}
     * ~{KyRTK5~}number_of_raw_data_blocks_in_frame == 0
     * ~{1mJ>K5~}ADTS~{V!VPSPR;8v~}AAC~{J}>]?i2"2;JGK5C;SP!#~}(~{R;8v~}AAC~{T-J<V!0|:,R;6NJ1<dDZ~}1024~{8v2IQy<0O`9XJ}>]~})
     */
    unsigned int numberOfRawDataBlockInFrame; /* 2 bit */
} AUDIO_ADTS_HEADER_S;


/*****************************************************************************
                        函数
*****************************************************************************/

/**
 * @function   audioCodec_g711Enc
 * @brief      G711音频编码
 * @param[in]  void *pEncoder 无效参数，可为NULL
 * @param[in]  AUDIO_INTERFACE_INFO_S *pstInterfaceInfo 编码参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audioCodec_g711Enc(void *pEncoder, AUDIO_INTERFACE_INFO_S *pstInterfaceInfo)
{
    UINT32 u32SampleNum = 0;
    UINT8 *pSrc = NULL;
    UINT8 *pDst = NULL;
    AUDIO_ENC_TYPE enEncType = 0;

    if (NULL == pstInterfaceInfo)
    {
        AUD_LOGE("null ptr\n");
        return SAL_FAIL;
    }

    u32SampleNum = pstInterfaceInfo->uiInputLen / 2;
    pSrc = pstInterfaceInfo->pInputBuf;
    pDst = pstInterfaceInfo->pOutputBuf;
    enEncType = pstInterfaceInfo->enDstType;

    G711ENC_Encode(enEncType, u32SampleNum, pSrc, pDst);

    pstInterfaceInfo->uiOutputLen = pstInterfaceInfo->uiInputLen / 2;

    return SAL_SOK;
}

/**
 * @function   audioCodec_g711Enc
 * @brief      G711音频解码
 * @param[in]  void *pDecoder 无效参数，可为NULL
 * @param[in]  AUDIO_INTERFACE_INFO_S *pstInterfaceInfo 编码参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audioCodec_g711Dec(void *pDecoder, AUDIO_INTERFACE_INFO_S *pstInterfaceInfo)
{
    UINT32 u32SampleNum = 0;
    UINT8 *pSrc = NULL;
    UINT8 *pDst = NULL;
    AUDIO_ENC_TYPE enEncType = 0;

    if (NULL == pstInterfaceInfo)
    {
        AUD_LOGE("null ptr\n");
        return SAL_FAIL;
    }

    u32SampleNum = pstInterfaceInfo->uiInputLen;

    pSrc = pstInterfaceInfo->pInputBuf;
    pDst = pstInterfaceInfo->pOutputBuf;
    enEncType = pstInterfaceInfo->enDstType;

    G711DEC_Decode(enEncType, u32SampleNum, pDst, pSrc);

    pstInterfaceInfo->uiOutputLen = pstInterfaceInfo->uiInputLen * 2;
    /*SAL_INFO("type %d inlen %d \n", enEncType, u32SampleNum); */

    return SAL_SOK;
}

/**
 * @function   audioCodec_aacEncInit
 * @brief      AAC音频编码器创建
 * @param[in]  None
 * @param[out] void **ppHandle 编码器指针
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audioCodec_aacEncInit(void **ppHandle)
{
#if 1
    UINT32 u32Version = 0;
    AUDIOENC_INFO stEncInfo = {0};
    AUDIOENC_PARAM stEncParam = {0};
    MEM_TAB stMemTab[1] = {0};
    void *hEncoder = NULL;
    INT32 s32Ret = SAL_FAIL;

    if (NULL == ppHandle)
    {
        AUD_LOGE("init param inv !!!!! \n");
        return SAL_FAIL;
    }

    stEncParam.sample_rate = 8000;
    stEncParam.num_channels = 1;
    stEncParam.bitrate = 128000;
    stEncParam.bit_ctrl_level = LEVEL2;

    u32Version = HIK_AACCODEC_GetVersion();

    s32Ret = HIK_AACENC_GetInfoParam(&stEncInfo);
    if (s32Ret != HIK_AUDIOCODEC_LIB_S_OK)
    {
        AUD_LOGE("get info param failed, s32Ret:0x%x !!!!! \n", s32Ret);
        return SAL_FAIL;
    }

    /* pstAacEncPrm->uiAacEncFrmInLen  = stEncInfo.in_frame_size ; */
    /* pstAacEncPrm->pu8AacEncFrmInBuf = SAL_memMalloc(pstAacEncPrm->uiAacEncFrmInLen + 640, "audio", NULL); */
    /* if( NULL == pstAacEncPrm->pu8AacEncFrmInBuf ) */
    /* { */
    /*    AUD_LOGE(" allocate AAC Enc input buffer failed !!!!!!\n"); */
    /*    return SAL_FAIL; */
    /* } */

    AUD_LOGW(" ---u32Version %d--- stEncInfo.in_frame_size:%d ---------\n", u32Version, stEncInfo.in_frame_size);

    s32Ret = HIK_AACENC_GetMemSize(&stEncParam, stMemTab);
    if (s32Ret != HIK_AUDIOCODEC_LIB_S_OK)
    {
        AUD_LOGE("get memory size failed, s32Ret:0x%x !!!!\n", s32Ret);
        return SAL_FAIL;
    }

    stMemTab[0].base = (void *)SAL_memAlign(stMemTab[0].alignment, stMemTab[0].size, "audio", "codec");

    s32Ret = HIK_AACENC_Create(&stEncParam, stMemTab, &hEncoder);
    if (s32Ret != HIK_AUDIOCODEC_LIB_S_OK)
    {
        SAL_memfree(stMemTab[0].base, "audio", "codec");
        AUD_LOGE("HIK_AACENC_Create s32Ret: %x \n", s32Ret);
        return SAL_FAIL;
    }

    *ppHandle = hEncoder;

    /* AUD_LOGI("===== size of mem: %d \n", stMemTab[0].size); */
#endif
    return SAL_SOK;

}

/**
 * @function   audioCodec_aacEncProc
 * @brief      AAC音频编码
 * @param[in]  void *pEncoder 编码器指针
 * @param[in]  AUDIO_INTERFACE_INFO_S *pstInterfaceInfo 编码参数
 * @param[out] AUDIO_INTERFACE_INFO_S *pstInterfaceInfo 编码参数
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audioCodec_aacEncProc(void *pEncoder, AUDIO_INTERFACE_INFO_S *pstInterfaceInfo)
{
#if 1
    AUDIOENC_PROCESS_PARAM stProcPrm = {0};
    INT32 s32Ret = SAL_FAIL;

    if (NULL == pEncoder || NULL == pstInterfaceInfo)
    {
        AUD_LOGE("hdl %p,prm %p\n", pEncoder, pstInterfaceInfo);
        return SAL_FAIL;
    }

    if (pstInterfaceInfo->uiInTmpLen + pstInterfaceInfo->uiInputLen > pstInterfaceInfo->uiInTmpBufLen)
    {
        AUD_LOGE("enc data overflow %d %d\n", pstInterfaceInfo->uiInTmpLen, pstInterfaceInfo->uiInputLen);
        return SAL_FAIL;
    }

    memcpy(pstInterfaceInfo->pInTmpBuf + pstInterfaceInfo->uiInTmpLen, pstInterfaceInfo->pInputBuf, pstInterfaceInfo->uiInputLen);
    pstInterfaceInfo->uiInTmpLen += pstInterfaceInfo->uiInputLen;

    if (pstInterfaceInfo->uiInTmpLen < AAC_ENC_SRC_FRM_LEN)
    {
        AUD_LOGD("Need more data !!!!!\n");
        return AUDIO_CODEC_NEED_MORE_DATA;
    }

    stProcPrm.in_buf = pstInterfaceInfo->pInTmpBuf;
    stProcPrm.out_buf = pstInterfaceInfo->pOutputBuf;

    s32Ret = HIK_AACENC_Encode(pEncoder, &stProcPrm);
    if (HIK_AUDIOCODEC_LIB_S_OK != s32Ret)
    {
        AUD_LOGE(" HIK_AACENC_Encode failed , s32Ret:0x%x !!!!\n", s32Ret);
        return SAL_FAIL;
    }

    pstInterfaceInfo->uiInTmpLen -= AAC_ENC_SRC_FRM_LEN;
    if (0 < pstInterfaceInfo->uiInTmpLen)
    {
        memmove(pstInterfaceInfo->pInTmpBuf, pstInterfaceInfo->pInTmpBuf + AAC_ENC_SRC_FRM_LEN, pstInterfaceInfo->uiInTmpLen);
    }

    pstInterfaceInfo->uiOutputLen = stProcPrm.out_frame_size;

#endif

    return SAL_SOK;
}

/**
 * @function   audioCodec_aacDecInit
 * @brief      创建aac解码器
 * @param[in]  None
 * @param[out] void **ppHandle 解码器指针
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audioCodec_aacDecInit(void **ppHandle)
{
#if 1
    MEM_TAB stMemTab[1] = {0};
    AUDIODEC_PARAM stDecPrm = {0};
    void *handle = NULL;
    INT32 s32Ret = SAL_FAIL;

    if (NULL == ppHandle)
    {
        AUD_LOGE("init param inv !!!!! \n");
        return SAL_FAIL;
    }

    s32Ret = HIK_AACDEC_GetMemSize(&stDecPrm, stMemTab); /* ~{7VEdDZ4f~} */

    stMemTab[0].base = (void *)SAL_memAlign(stMemTab[0].alignment, stMemTab[0].size, "audio", "codec");
    if (stMemTab[0].base == NULL)
    {
        AUD_LOGE("enc_param.buf alloc failed(%d)\n", stMemTab[0].size);
        return SAL_FAIL;
    }

    s32Ret = HIK_AACDEC_Create(&stDecPrm, stMemTab, &handle);
    if (s32Ret != HIK_AUDIOCODEC_LIB_S_OK)
    {
        AUD_LOGE("HIK_AACDEC_Create with s32Ret:0x%x !!!!!!\n", s32Ret);
        SAL_memfree(stMemTab[0].base, "audio", "codec");
        return SAL_FAIL;
    }

    *ppHandle = handle;
#endif
    return SAL_SOK;
}

/**
 * @function   audioCodec_parseAdtsHeader
 * @brief      AAC音频ADTS头解析
 * @param[in]  UINT8 *pBuf 输入码流
 * @param[out] AUDIO_ADTS_HEADER_S *pstAdtsHeader adts头参数指针
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audioCodec_parseAdtsHeader(UINT8 *pBuf, AUDIO_ADTS_HEADER_S *pstAdtsHeader)
{
    if (NULL == pBuf || NULL == pstAdtsHeader)
    {
        AUD_LOGE("pBuf put %p,out %p\n", pBuf, pstAdtsHeader);
        return SAL_FAIL;
    }

    memset(pstAdtsHeader, 0, sizeof(*pstAdtsHeader));

    if ((pBuf[0] == 0xFF) && ((pBuf[1] & 0xF0) == 0xF0))
    {
        pstAdtsHeader->id = ((unsigned int) pBuf[1] & 0x08) >> 3;
        /* printf("adts:id  %d\n", pstAdtsHeader->id); */
        pstAdtsHeader->layer = ((unsigned int) pBuf[1] & 0x06) >> 1;
        /* printf( "adts:layer  %d\n", pstAdtsHeader->layer); */
        pstAdtsHeader->protectionAbsent = (unsigned int) pBuf[1] & 0x01;
        /* printf( "adts:protection_absent  %d\n", pstAdtsHeader->protectionAbsent); */
        pstAdtsHeader->profile = ((unsigned int) pBuf[2] & 0xc0) >> 6;
        /* printf( "adts:profile  %d\n", pstAdtsHeader->profile); */
        pstAdtsHeader->samplingFreqIndex = ((unsigned int) pBuf[2] & 0x3c) >> 2;
        /* printf( "adts:sf_index  %d\n", pstAdtsHeader->samplingFreqIndex); */
        pstAdtsHeader->privateBit = ((unsigned int) pBuf[2] & 0x02) >> 1;
        /* printf( "adts:pritvate_bit  %d\n", pstAdtsHeader->privateBit); */
        pstAdtsHeader->channelCfg = ((((unsigned int) pBuf[2] & 0x01) << 2) | (((unsigned int) pBuf[3] & 0xc0) >> 6));
        /* printf( "adts:channel_configuration  %d\n", pstAdtsHeader->channelCfg); */
        pstAdtsHeader->originalCopy = ((unsigned int) pBuf[3] & 0x20) >> 5;
        /* printf( "adts:original  %d\n", pstAdtsHeader->originalCopy); */
        pstAdtsHeader->home = ((unsigned int) pBuf[3] & 0x10) >> 4;
        /* printf( "adts:home  %d\n", pstAdtsHeader->home); */
        pstAdtsHeader->copyrightIdentificationBit = ((unsigned int) pBuf[3] & 0x08) >> 3;
        /* printf( "adts:copyright_identification_bit  %d\n", pstAdtsHeader->copyrightIdentificationBit); */
        pstAdtsHeader->copyrightIdentificationStart = (unsigned int) pBuf[3] & 0x04 >> 2;
        /* printf( "adts:copyright_identification_start  %d\n", pstAdtsHeader->copyrightIdentificationStart); */
        pstAdtsHeader->aacFrameLength = (((((unsigned int) pBuf[3]) & 0x03) << 11)
                                         | (((unsigned int)pBuf[4] & 0xFF) << 3)
                                         | ((unsigned int)pBuf[5] & 0xE0) >> 5);
        /* printf( "adts:aac_frame_length  %d\n", pstAdtsHeader->aacFrameLength); */
        pstAdtsHeader->adtsBufferFullness = (((unsigned int) pBuf[5] & 0x1f) << 6
                                             | ((unsigned int) pBuf[6] & 0xfc) >> 2);
        /* printf( "adts:adts_buffer_fullness  %d\n", pstAdtsHeader->adtsBufferFullness); */
        pstAdtsHeader->numberOfRawDataBlockInFrame = ((unsigned int) pBuf[6] & 0x03);
        /* printf( "adts:no_raw_data_blocks_in_frame  %d\n", pstAdtsHeader->numberOfRawDataBlockInFrame); */

        return 0;
    }
    else
    {
        AUD_LOGE("failed to parse adts header\n");
        return SAL_FAIL;
    }
}

/**
 * @function   aacCodec_aacDecProc
 * @brief      AAC音频解码
 * @param[in]  void *pDecoder 解码器指针
 * @param[out] AUDIO_INTERFACE_INFO_S *pstInterfaceInfo 解码参数
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 aacCodec_aacDecProc(void *pDecoder, AUDIO_INTERFACE_INFO_S *pstInterfaceInfo)
{
#if 1
    AUDIO_ADTS_HEADER_S stAacAdts = {0};
    AUDIODEC_PROCESS_PARAM stDecFrame = {0};
    UINT32 u32MinDecFrmLen = AO_FRAME_LEN / 2;
    INT32 s32LibRet = HIK_AUDIOCODEC_LIB_S_OK;
    INT32 s32Ret = SAL_FAIL;
    INT32 s32TotalDecLen = 0;
    INT32 s32RestLen = 0;

    if (NULL == pDecoder || NULL == pstInterfaceInfo)
    {
        AUD_LOGE("hdl %p,prm %p\n", pDecoder, pstInterfaceInfo);
        return SAL_FAIL;
    }

    if (pstInterfaceInfo->uiInTmpLen + pstInterfaceInfo->uiInputLen > pstInterfaceInfo->uiInTmpBufLen)
    {
        AUD_LOGE("data overflow %d %d\n", pstInterfaceInfo->uiInTmpLen, pstInterfaceInfo->uiInputLen);
        return SAL_FAIL;
    }

    memcpy(pstInterfaceInfo->pInTmpBuf + pstInterfaceInfo->uiInTmpLen, pstInterfaceInfo->pInputBuf, pstInterfaceInfo->uiInputLen);
    pstInterfaceInfo->uiInTmpLen += pstInterfaceInfo->uiInputLen;

    if (u32MinDecFrmLen > pstInterfaceInfo->uiInTmpLen)
    {
        AUD_LOGD("Need more data !!!!!\n");
        return AUDIO_CODEC_NEED_MORE_DATA;
    }

    if (pstInterfaceInfo->pInTmpBuf[0] == 0xff && ((pstInterfaceInfo->pInTmpBuf[1] & 0xf0) == 0xf0))
    {
        audioCodec_parseAdtsHeader(pstInterfaceInfo->pInTmpBuf, &stAacAdts);
    }

    stDecFrame.in_buf = pstInterfaceInfo->pInTmpBuf;
    stDecFrame.in_data_size = pstInterfaceInfo->uiInTmpLen; /* pstInterfaceInfo->uiInputLen;//AO_FRAME_LEN; */
    stDecFrame.out_buf = pstInterfaceInfo->pOutputBuf;
    do
    {
        /* if (framenum == 186 && stDecFrame.in_data_size < 200) */
        /* { */
        /*  framenum = framenum; */
        /* } */
        /* decode one AAC frame */
        s32LibRet = HIK_AACDEC_Decode(pDecoder, &stDecFrame);
        /* AUD_LOGE("===== HIK_AACDEC_Decode with %#x ====\n", s32LibRet); */
        if (s32LibRet == HIK_AUDIOCODEC_LIB_S_OK)
        {
            pstInterfaceInfo->framenum++;
            pstInterfaceInfo->uiOutputLen += stDecFrame.out_frame_size;
            stDecFrame.out_buf = pstInterfaceInfo->pOutputBuf + pstInterfaceInfo->uiOutputLen;

            s32TotalDecLen += stDecFrame.proc_data_size;
            s32RestLen = pstInterfaceInfo->uiInTmpLen - s32TotalDecLen;

            /*剩余数据不够或者解码输出缓冲区小于2K*/
            if ((u32MinDecFrmLen <= s32RestLen) && (pstInterfaceInfo->uiOutputLen + AAC_ENC_SRC_FRM_LEN < pstInterfaceInfo->uiOutputBufLen))
            {
                stDecFrame.in_buf = pstInterfaceInfo->pInTmpBuf + s32TotalDecLen;
                stDecFrame.in_data_size = s32RestLen; /* s32RestLen */
            }
            else
            {
                if (s32RestLen > 0)
                {
                    memmove(pstInterfaceInfo->pInTmpBuf, pstInterfaceInfo->pInTmpBuf + s32TotalDecLen, s32RestLen);
                    pstInterfaceInfo->uiInTmpLen = s32RestLen;
                    /*SAL_INFO("audio rest len %d\n", s32RestLen);*/
                }
                else
                {
                    pstInterfaceInfo->uiInTmpLen = 0;
                }

                s32Ret = SAL_SOK;
                break;
            }
        }
        else
        {
            if (0 < pstInterfaceInfo->framenum)
            {
                pstInterfaceInfo->uiInTmpLen = 0;  /* give up */
                s32Ret = SAL_SOK;
            }
            else
            {
                s32Ret = SAL_FAIL;
                pstInterfaceInfo->uiInTmpLen = 0;
                AUD_LOGE("===== HIK_AACDEC_Decode s32LibRet with %#x ====\n", s32LibRet);
            }

            break;
        }
    }
    while (1);  /* playStatus == Play */

    return s32Ret;
    #endif

    return 0;
}

/**
 * @function   audioCodec_clearData
 * @brief      清除解码buf数据
 * @param[in]  AUDIO_INTERFACE_INFO_S *pstInterfaceInfo 解码参数
 * @param[out] None
 * @return     void
 */
static INT32 audioCodec_clearData(AUDIO_INTERFACE_INFO_S *pstInterfaceInfo)
{
    if (NULL == pstInterfaceInfo)
    {
        AUD_LOGE("hdl %p\n", pstInterfaceInfo);
        return SAL_FAIL;
    }

    pstInterfaceInfo->uiOutputLen = 0;
    pstInterfaceInfo->uiInTmpLen = 0;

    return SAL_SOK;
}

/**
 * @function   audioCodec_drv_encRegister
 * @brief      注册音频编码函数
 * @param[in]  UINT32 u32ArraySize 数组大小
 * @param[in]  CODEC_OPERATION_S *pstOperate 编码相关函数指针数组
 * @param[out] None
 * @return     void
 */
void audioCodec_drv_encRegister(UINT32 u32ArraySize, CODEC_OPERATION_S *pstOperate)
{
    if (NULL == pstOperate
        || AAC >= u32ArraySize)
    {
        AUD_LOGE("u32ArraySize %d opt %p\n", u32ArraySize, pstOperate);
        return;
    }

    /* Enc processer register */
    pstOperate[G711_MU].bNeedHndl = 0;
    pstOperate[G711_MU].hndl = NULL;
    pstOperate[G711_MU].Init = NULL;
    pstOperate[G711_MU].DeInit = NULL;
    pstOperate[G711_MU].PreProc = audioCodec_clearData;
    pstOperate[G711_MU].Proc = audioCodec_g711Enc;

    pstOperate[G711_A].bNeedHndl = 0;
    pstOperate[G711_A].hndl = NULL;
    pstOperate[G711_A].Init = NULL;
    pstOperate[G711_A].DeInit = NULL;
    pstOperate[G711_A].PreProc = audioCodec_clearData;
    pstOperate[G711_A].Proc = audioCodec_g711Enc;

    pstOperate[G722_1].bNeedHndl = 0;
    pstOperate[G722_1].hndl = NULL;
    pstOperate[G722_1].Init = NULL;     /* */
    pstOperate[G722_1].DeInit = NULL;
    pstOperate[G722_1].PreProc = NULL;
    pstOperate[G722_1].Proc = NULL;

    pstOperate[AAC].bNeedHndl = 1;
    pstOperate[AAC].hndl = NULL;
    pstOperate[AAC].Init = audioCodec_aacEncInit;
    pstOperate[AAC].DeInit = NULL;
    pstOperate[AAC].PreProc = audioCodec_clearData;
    pstOperate[AAC].Proc = audioCodec_aacEncProc;
}

/**
 * @function   audioCodec_drv_decRegister
 * @brief      注册音频解码函数
 * @param[in]  UINT32 u32ArraySize 数组大小
 * @param[in]  CODEC_OPERATION_S *pstOperate 解码相关函数指针数组
 * @param[out] None
 * @return     void
 */
void audioCodec_drv_decRegister(UINT32 u32ArraySize, CODEC_OPERATION_S *pstOperate)
{
    if (NULL == pstOperate
        || AAC >= u32ArraySize)
    {
        AUD_LOGE("u32ArraySize %d opt %p\n", u32ArraySize, pstOperate);
        return;
    }

    /* Dec processer register */
    pstOperate[G711_MU].bNeedHndl = 0;
    pstOperate[G711_MU].hndl = NULL;
    pstOperate[G711_MU].Init = NULL;
    pstOperate[G711_MU].DeInit = NULL;
    pstOperate[G711_MU].PreProc = audioCodec_clearData;
    pstOperate[G711_MU].Proc = audioCodec_g711Dec;

    pstOperate[G711_A].bNeedHndl = 0;
    pstOperate[G711_A].hndl = NULL;
    pstOperate[G711_A].Init = NULL;
    pstOperate[G711_A].DeInit = NULL;
    pstOperate[G711_A].PreProc = audioCodec_clearData;
    pstOperate[G711_A].Proc = audioCodec_g711Dec;

    pstOperate[G722_1].bNeedHndl = 0;   /* when need to add one new type, update here */
    pstOperate[G722_1].hndl = NULL;
    pstOperate[G722_1].Init = NULL;     /* */
    pstOperate[G722_1].DeInit = NULL;
    pstOperate[G722_1].PreProc = NULL;
    pstOperate[G722_1].Proc = NULL;

    pstOperate[AAC].bNeedHndl = 1;
    pstOperate[AAC].hndl = NULL;
    pstOperate[AAC].Init = audioCodec_aacDecInit;
    pstOperate[AAC].DeInit = NULL;
    pstOperate[AAC].PreProc = audioCodec_clearData;
    pstOperate[AAC].Proc = aacCodec_aacDecProc;
}

