/**
 * @file   vca_unpack.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  vca解封装函数接口
 * @author huangshuxin
 * @date   2019年5月24日 Create
 *
 * @note \n History
   1.Date        : 2019年5月24日 Create
     Author      : huangshuxin
     Modification: 新建文件
   2.Date        : 2021/08/19
     Author      : yindongping
     Modification: 组件开发，整理接口
 */


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <arpa/inet.h>

#include <dspcommon.h>
#include "sal.h"

#include "IVS_SYS_lib.h"
#include "vca_common.h"
#include "type_dscrpt_common.h"
#include "libdemux.h"

#include "system_prm_api.h"
#include "sva_out.h"
#include "vca_unpack.h"
#include "vca_unpack_api.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define BUFFER_SIZE					(1024 * 1024)
#define TEST_TARGET_NUM				64
#define CMMD_POS_SYNC_FLAG			(1 << 0) /* /<POS同步标记 */
#define CMMD_VIDEO_PRIV_SYNC_FLAG	(1 << 1) /* /<智能信息采用新的更精确阈值进行同步匹配以及匹配处理--此功能已废弃，改用IVS解析的type表示 */

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
/* *****************控制信息私有类型**************** / */
typedef struct _TI_OSD_LINE_T_
{
    unsigned int offsetx;       /* 单行字符横坐标(归一化，乘以1000的值) */

    unsigned int offsety;       /* 单行字符纵坐标(归一化，乘以1000的值) */

    /* length = sizeof(TI_OSD_LINE_T)+ROUND_UP(strlen(code),0x10) */
    unsigned short length;        /* 字符长度 */

    unsigned char alignment;    /* 对齐字段 */

    unsigned char res[17];      /* 保留字段 */

    unsigned int check_sum;     /* 校验码 (从横坐标到保留字段的总和) */

    unsigned char osd_name[SVA_ALERT_NAME_LEN]; /* osd字符 */

} __attribute__((packed, aligned(1))) TI_OSD_LINE_T;

typedef struct _TI_OSD_INFO_T_
{
    unsigned int startcode;     /* 起始码，0x54584554 */

    unsigned short text_type;   /* 文本类型(默认填0) */

    unsigned short def_version; /* 版本信息，默认0x0001 */

    unsigned short language;    /* 语言类型 (该值非0才显示字符) */

    unsigned short color;       /* 字体颜色 */

    /* OSD line numbers */
    unsigned short line_nums;   /* 字符行数 (该值非0才显示字符) */

    unsigned char res[14];     /* 全部为0 */

    unsigned short char_width;  /* 字符宽度 */

    unsigned short char_height;  /* 字符高度 */

    unsigned int win_width;     /* 画面宽度 */

    unsigned int win_height;    /* 画面高度 */

    /* length = sizeof(TI_OSD_INFO_T)+line * (osd line chars) */
    unsigned int length;        /* 数据总长度 (包含48字节头) */

    unsigned int check_sum;     /* 校验码 (该值和数据前44字节各字段数值总和进行校验) */

    TI_OSD_LINE_T line[TEST_TARGET_NUM];

} TI_OSD_INFO_T;

typedef struct _VCA_UNPACK_
{
    IVS_SYS_PROC_PARAM stProcPrm;

    HIK_MULT_VCA_TARGET_LIST stTargetList;

    SVA_PROCESS_OUT stOut;

    SVA_PROCESS_OUT stOutResult;

    void *mDecMutexHdl;                                          /* 通道信号量*/

} VCA_UNPACK_ST;


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */


static VCA_UNPACK_ST gstVcaUnpack[MAX_VDEC_CHAN];

/**
 * @function   vca_unpack_init
 * @brief      vca 解析初始化
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] NONE
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_unpack_init(UINT32 u32Chan)
{
    VCA_UNPACK_ST *pstVcaDec = NULL;

    if (u32Chan < MAX_VDEC_CHAN)
    {
        pstVcaDec = &gstVcaUnpack[u32Chan];
        if (pstVcaDec == NULL)
        {
            VCA_LOGE("error\n");
            return SAL_FAIL;
        }

        pstVcaDec->stProcPrm.buf_size = BUFFER_SIZE;

        pstVcaDec->stProcPrm.buf = (unsigned char *)SAL_memMalloc(BUFFER_SIZE, "vca_venc", "unpack");
        if (pstVcaDec->stProcPrm.buf == NULL)
        {
            VCA_LOGE("error\n");
            return SAL_FAIL;
        }

        pstVcaDec->stTargetList.type = SHOW_TYPE_LINEBG_ONE_HALF | MATCH_TYPE_ACCURACY;
        pstVcaDec->stTargetList.target_num = TEST_TARGET_NUM;
        pstVcaDec->stTargetList.p_target = (HIK_MULT_VCA_TARGET *)SAL_memMalloc(TEST_TARGET_NUM * sizeof(HIK_MULT_VCA_TARGET), "vca_venc", "target");

        if (pstVcaDec->stTargetList.p_target == NULL)
        {
            VCA_LOGE("error\n");
            SAL_memfree(pstVcaDec->stProcPrm.buf, "vca_venc", "unpack");
            return SAL_FAIL;
        }

        SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstVcaDec->mDecMutexHdl);
    }
    else
    {
        VCA_LOGE("error\n");
        return SAL_FAIL;
    }

    VCA_LOGD("Chn %d UnPack Privt Init Success \n", u32Chan);

    return SAL_SOK;
}

/**
 * @function   vca_unpack_clearResult
 * @brief      清除vca解析结果
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] NONE
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_unpack_clearResult(UINT32 u32Chan)
{
    /* INT32 s32Ret        = SAL_FAIL; */
    VCA_UNPACK_ST *pstVcaDec = NULL;

    /* SVA_PROCESS_OUT *pstOut = NULL; */

    if (u32Chan > (MAX_VDEC_CHAN - 1))
    {
        VCA_LOGE("u32Chan %d error !\n", u32Chan);
        return SAL_FAIL;
    }

    pstVcaDec = &gstVcaUnpack[u32Chan];
    if (pstVcaDec == NULL)
    {
        VCA_LOGE("pstVcaDec error\n");
        return SAL_FAIL;
    }

    SAL_mutexLock(pstVcaDec->mDecMutexHdl);

    memset(&pstVcaDec->stOut, 0x00, sizeof(SVA_PROCESS_OUT));
    memset(&pstVcaDec->stOutResult, 0x00, sizeof(SVA_PROCESS_OUT));

    SAL_mutexUnlock(pstVcaDec->mDecMutexHdl);

    return SAL_SOK;
}

/**
 * @function   vca_unpack_updateResult
 * @brief      更新vca解析结果
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] NONE
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 vca_unpack_updateResult(UINT32 u32Chan)
{
    INT32 s32Ret = SAL_SOK;
    INT32 i = 0;
    VCA_UNPACK_ST *pstVcaDec = NULL;

    if (u32Chan > (MAX_VDEC_CHAN - 1))
    {
        VCA_LOGE("u32Chan %d error !\n", u32Chan);
        return SAL_FAIL;
    }

    pstVcaDec = &gstVcaUnpack[u32Chan];
    if (pstVcaDec == NULL)
    {
        VCA_LOGE("pstVcaDec error\n");
        return SAL_FAIL;
    }

    SAL_mutexLock(pstVcaDec->mDecMutexHdl);

    s32Ret = Sva_HalCpySvaResult(&pstVcaDec->stOut, &pstVcaDec->stOutResult);
    /* 任何包裹展示方式时，双视角目标合并的target直接忽略 */
    for (i = 0; i < pstVcaDec->stOut.target_num; i++)
    {
        if (SAL_TRUE == pstVcaDec->stOutResult.target[i].bAnotherViewTar)
        {
            memset(&pstVcaDec->stOutResult.target[i], 0x00, sizeof(SVA_TARGET));
        }
    }

    SAL_mutexUnlock(pstVcaDec->mDecMutexHdl);

    return s32Ret;
}

/**
 * @function   vca_unpack_getResult
 * @brief      获取vca解析结果
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] void *pstGetOut 输出结果buf
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_unpack_getResult(UINT32 u32Chan, void *pstGetOut)
{
    /* INT32 s32Ret        = SAL_FAIL; */
    UINT32 i= 0;
    VCA_UNPACK_ST *pstVcaDec = NULL;

    /* SVA_PROCESS_OUT *pstOut = NULL; */

    if (u32Chan > (MAX_VDEC_CHAN - 1))
    {
        VCA_LOGE("u32Chan %d error !\n", u32Chan);
        return SAL_FAIL;
    }

    pstVcaDec = &gstVcaUnpack[u32Chan];
    if (pstVcaDec == NULL)
    {
        VCA_LOGE("error\n");
        return SAL_FAIL;
    }

    SAL_mutexLock(pstVcaDec->mDecMutexHdl);

    /* 任何包裹展示方式时，双视角目标合并的target直接忽略 */
    for (i = 0; i < pstVcaDec->stOutResult.target_num; i++)
    {
        if (SAL_TRUE == pstVcaDec->stOutResult.target[i].bAnotherViewTar)
        {
            memset(&pstVcaDec->stOutResult.target[i], 0x00, sizeof(SVA_TARGET));
        }
    }
    memcpy(pstGetOut, &pstVcaDec->stOutResult, sizeof(SVA_PROCESS_OUT));

    SAL_mutexUnlock(pstVcaDec->mDecMutexHdl);

    return SAL_SOK;
}

/**
 * @function   vca_unpack_ivs
 * @brief      解析ivs信息
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] void *pstBuff 输出结果buf
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 vca_unpack_ivs(UINT32 u32Chan, void *pstBuff)
{
    UINT32 i = 0;

    DEM_BUFFER *pstDemBuf = NULL;
    HIK_MULT_VCA_TARGET_LIST *pstMetaData = NULL;
    IVS_SYS_PROC_PARAM *pstProcParam = NULL;
    SVA_PROCESS_OUT *pstOut = NULL;
    IS_PRIVT_INFO_CONTRABAND *pstPrivtInfo = NULL;
    VCA_UNPACK_ST *pstVcaDec = NULL;
    UINT8 *pLeft = NULL;
    UINT8 *pRight = NULL;

    pstVcaDec = &gstVcaUnpack[u32Chan];
    if (pstVcaDec == NULL)
    {
        VCA_LOGE("pstVcaDec error\n");
        return SAL_FAIL;
    }

    pstDemBuf = (DEM_BUFFER *)pstBuff;
    if (pstDemBuf == NULL)
    {
        VCA_LOGE("pstDemBuf error\n");
        return SAL_FAIL;
    }

    pstProcParam = &pstVcaDec->stProcPrm;
    pstMetaData = &pstVcaDec->stTargetList;
    if (pstMetaData == NULL || pstProcParam == NULL)
    {
        VCA_LOGE("pstMetaData || pstProcParam error\n");
        return SAL_FAIL;
    }

    memcpy(pstProcParam->buf, pstDemBuf->addr, pstDemBuf->datalen);

    pstProcParam->len = pstDemBuf->datalen;

    if (HIK_IVS_SYS_LIB_S_OK != IVS_MULT_META_DATA_sys_parse(pstMetaData, pstProcParam))
    {
        VCA_LOGE("IVS_META_DATA_sys_parse_com error\n");
        return SAL_FAIL;
    }

    pstOut = &pstVcaDec->stOut;
    if (pstOut == NULL)
    {
        VCA_LOGE("pstOut error\n");
        return SAL_FAIL;
    }

    memset(pstOut, 0x00, sizeof(SVA_PROCESS_OUT));

    pstOut->frame_stamp = pstDemBuf->timestamp;
    pstOut->target_num = pstMetaData->target_num;

    for (i = 0; i < pstOut->target_num; i++)
    {
        if (IS_PRIVT_COLOR_CONTRABAND == pstMetaData->p_target[i].privt_info.privt_type)
        {
            pstPrivtInfo = (IS_PRIVT_INFO_CONTRABAND *)pstMetaData->p_target[i].privt_info.privt_data;

            pstOut->target[i].color = (pstPrivtInfo->color.red << 16);
            pstOut->target[i].color += (pstPrivtInfo->color.green << 8);
            pstOut->target[i].color += (pstPrivtInfo->color.blue);
            pstOut->target[i].visual_confidence = pstPrivtInfo->confidence;
            pstOut->target[i].type = pstPrivtInfo->type;

            pLeft = (UINT8 *)strstr((char *)pstPrivtInfo->pos_data, "(");
            pRight = (UINT8 *)strstr((char *)pstPrivtInfo->pos_data, "%)");

            if (pLeft && pRight)
            {
                sal_memcpy_s(pstOut->target[i].sva_name, SVA_ALERT_NAME_LEN, pstPrivtInfo->pos_data, pLeft - pstPrivtInfo->pos_data);
            }
            else
            {
                sal_memcpy_s(pstOut->target[i].sva_name, SVA_ALERT_NAME_LEN, pstPrivtInfo->pos_data, strlen((char *)pstPrivtInfo->pos_data));
            }
        }

        pstOut->target[i].alarm_flg = pstMetaData->p_target[i].target.alarm_flg;
        pstOut->target[i].ID = pstMetaData->p_target[i].target.ID;
        pstOut->target[i].rect.x = pstMetaData->p_target[i].target.rect.x;
        pstOut->target[i].rect.y = pstMetaData->p_target[i].target.rect.y;
        pstOut->target[i].rect.width = pstMetaData->p_target[i].target.rect.width;
        pstOut->target[i].rect.height = pstMetaData->p_target[i].target.rect.height;
    }

    return SAL_SOK;
}

/**
 * @function   vca_unpack_pos
 * @brief      解析pos信息
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] void *pstBuff 输出结果buf
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_unpack_pos(UINT32 u32Chan, VOID *pstBuff)
{
    UINT32 i = 0;
    UINT8 *pLeft = NULL;
    UINT8 *pRight = NULL;
    DEM_BUFFER *pstDemBuf = NULL;
    TI_OSD_INFO_T *pstTiOsdInfo = NULL;
    TI_OSD_LINE_T *pstLine = NULL;
    SVA_PROCESS_OUT *pstOut = NULL;

    /* unsigned char name[32] = {0}; //osd字符 */

    VCA_UNPACK_ST *pstVcaDec = NULL;

    pstVcaDec = &gstVcaUnpack[u32Chan];
    if (pstVcaDec == NULL)
    {
        VCA_LOGE("pstVcaDec error\n");
        return SAL_FAIL;
    }

    pstDemBuf = (DEM_BUFFER *)pstBuff;
    if (pstDemBuf == NULL)
    {
        VCA_LOGE("pstDemBuf error\n");
        return SAL_FAIL;
    }

    pstTiOsdInfo = (TI_OSD_INFO_T *)pstDemBuf->addr;
    if (pstTiOsdInfo == NULL)
    {
        VCA_LOGE("pstTiOsdInfo error\n");
        return SAL_FAIL;
    }

    pstOut = &pstVcaDec->stOut;
    if (pstOut == NULL)
    {
        VCA_LOGE("pstOut error\n");
        return SAL_FAIL;
    }

    /* pstOut->stTargetPrm.name_cnt++; */

    for (i = 0; i < pstTiOsdInfo->line_nums; i++)
    {
        pstLine = &pstTiOsdInfo->line[i];
        if (pstLine == NULL)
        {
            VCA_LOGE("pstLine error\n");
            return SAL_FAIL;
        }

        pLeft = (UINT8 *)strstr((char *)pstLine->osd_name, "(");
        pRight = (UINT8 *)strstr((char *)pstLine->osd_name, "%)");

        if (pLeft && pRight)
        {
            memcpy(pstOut->target[i].sva_name, pstLine->osd_name, pLeft - pstLine->osd_name);
        }
        else
        {
            memcpy(pstOut->target[i].sva_name, pstLine->osd_name, strlen((char *)pstLine->osd_name));
        }
    }

    return SAL_SOK;
}

/**
 * @function   vca_unpack_process
 * @brief      vca信息解析处理
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] void *pstBuff 目标数据buf
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_unpack_process(UINT32 u32Chan, void *pstBuff)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 u32PrivtInfoType = 0;
    /* UINT32 privt_info_sub_type  = 0; */

    DEM_BUFFER *pstDemBuf = NULL;

    if (u32Chan > (MAX_VDEC_CHAN - 1))
    {
        VCA_LOGE("u32Chan %d error\n", u32Chan);
        return SAL_FAIL;
    }

    pstDemBuf = (DEM_BUFFER *)pstBuff;
    if (pstDemBuf == NULL)
    {
        VCA_LOGE("pstBuff error\n");
        return SAL_FAIL;
    }

    u32PrivtInfoType = pstDemBuf->privtType;
    /* privt_info_sub_type = pstDemBuf->privtSubType; */

    if (u32PrivtInfoType == HIK_PRIVT_IVS_INFO)
    {
        s32Ret = vca_unpack_ivs(u32Chan, pstDemBuf);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("vca_unpack_ivs u32Chan %d error\n", u32Chan);
            return SAL_FAIL;
        }

        s32Ret = vca_unpack_updateResult(u32Chan);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("vca_unpack_updateResult u32Chan %d error\n", u32Chan);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

