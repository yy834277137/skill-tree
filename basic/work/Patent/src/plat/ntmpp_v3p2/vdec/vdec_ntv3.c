/**
 * @file   vdec_ntv3.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  NT平台解码模块接口
 * @author liuxianying
 * @date   2021/12/27
 * @note
 * @note \n History
   1.日    期: 2021/12/27
     作    者: liuxianying
     修改历史: 创建文件
 */


#include "vdec_ntv3.h"
#include "sal_mem_new.h"
#include "vdec_hal_inter.h"
#include "mem_hal_api.h"
#include <platform_sdk.h>

#define VDEC_HAL_W_MIM	114
#define VDEC_HAL_H_MIM	114

#define VDEC_HAL_W_MAX	1920
#define VDEC_HAL_H_MAX	1920

#define VDEC_NUM_MAX   16
#define VDEC_QUE_NUM   8
#define _VDEC_PUSH_IN_   /*绑定和非绑定模式进行区别*/




/*解码器属性，平台差异化部分*/
typedef struct _VDEC_HIV5_SPEC_
{
    UINT32 type;
    UINT32 u32width;
    UINT32 u32height;

} VDEC_HIV5_SPEC;

typedef struct _NT_VDEC_FRAME_HAL_
{
    void *         pVirAddr;
    UINTPTR        u64PhyAddr;
    UINT32         size;
    UINTPTR        blk;
    HD_VIDEO_FRAME vfrm;
}NT_VDEC_FRAME_HAL;


typedef struct _NT_VDEC_ID_HAL_
{
    HD_PATH_ID     stVdecPathId;
    INT32          dec_ddr_id;
    DSA_QueHndl    pstFullQue;                                     /* 数据满队列 */
    DSA_QueHndl    pstTmpQue;                                     /* 预留对列 */
    DSA_QueHndl    pstEmptQue;                                     /* 数据空队列 */
}VDEC_ID_HAL;


typedef struct _VDEC_HAL_PLT_HANDLE_
{
    INT32 decChan;                                            /*对应实际的解码器通道号*/
    INT32 vdecOutStatue;                                      /* 解码器输出创建1，解码器输出未创建0, 2 正在使用 */
    VDEC_HIV5_SPEC vdechiv5Hdl;
    VDEC_ID_HAL stVdecId; 
} VDEC_HAL_PLT_HANDLE;



INT32 mem_hal_vbFree(Ptr pPtr, CHAR *szModName, CHAR *szMemName, UINT32 u32Size, UINT64 u64VbBlk, UINT32 u32PoolId);
INT32 mem_hal_vbAlloc(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, ALLOC_VB_INFO_S *pstVbInfo);
static VDEC_HAL_FUNC vdecFunc;


/**
 * @function   vdec_ntv3_ChangePixFormat
 * @brief	  映射平台相关格式
 * @param[in]  INT32 pixFormat 数据像素格式
 * @param[out] None
 * @return	  INT32
 */
static INT32 vdec_ntv3_ChangePixFormat(INT32 pixFormat)
{
    INT32 format = 0;

    switch (pixFormat)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        {
            format = HD_VIDEO_PXLFMT_YUV420;
            break;
        }

        default:
        {
            VDEC_LOGE("err format %d\n", format);
            return -1;
        }
    }

    return format;
}

/**
 * @function   vdec_ntv3_TransPixFormat
 * @brief	  映射业务相关格式
 * @param[in]  INT32 pixFormat 数据像素格式
 * @param[out] None
 * @return	  INT32
 */

static INT32 vdec_ntv3_TransPixFormat(INT32 pixFormat)
{
    INT32 format = 0;

    switch (pixFormat)
    {
        case HD_VIDEO_PXLFMT_YUV420:
        {
            format = SAL_VIDEO_DATFMT_YUV420SP_VU;
            break;
        }

        default:
        {
            VDEC_LOGE("err format %d pixFormat %d\n", format, pixFormat);
            return -1;
        }
    }

    return format;
}

/**
 * @function   vdec_ntv3_TransEncType
 * @brief	  映射业务相关编码格式
 * @param[in]  INT32 encType 编码格式
 * @param[out] None
 * @return	  INT32
 */

INT32 vdec_ntv3_TransEncType(INT32 encType)
{
    INT32 type = 0;

    switch (encType)
    {
        case HD_CODEC_TYPE_H264:
        {
            type = H264;
            break;
        }
        case HD_CODEC_TYPE_H265:
        {
            type = H265;
            break;
        }
        case HD_CODEC_TYPE_JPEG:
        {
            type = MJPEG;
            break;
        }
        default:
        {
            VDEC_LOGE("err type %d encType %d\n", type, encType);
            return -1;
        }
    }

    return type;
}



/**
* @function   vdec_ntv3_ChangeEncType
* @brief     申请解码缓存，NT解码需要对齐，直接在dec_out Pool中申请
* @param[in]   pVdecFrame 解码信息
* @param[out] None
* @return    INT32
*/
static INT32 vdec_ntv3_allocateBuffer(UINT64 pool, int size, HD_COMMON_MEM_DDR_ID ddr_id, void **virAddr,
            UINTPTR *phyAddr, HD_COMMON_MEM_VB_BLK *vblk)
{
    INT32 ret = 0;
    HD_COMMON_MEM_VB_BLK blk;
    UINTPTR pa;
    VOID *va = NULL;

    blk = hd_common_mem_get_block(pool, size, ddr_id);
    if (HD_COMMON_MEM_VB_INVALID_BLK == blk)
    {
        printf("hd_common_mem_get_block fail\r\n");
        ret =  -1;
        goto exit;
    }
    
    pa = hd_common_mem_blk2pa(blk);
    if (pa == 0) 
    {
        printf("hd_common_mem_blk2pa fail, blk = %#lx\r\n", blk);
        hd_common_mem_release_block(blk);
        ret =  -1;
        goto exit;
    }

    va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, pa, size);
    if (va == NULL) 
    {
        printf("hd_common_mem_mmap fail, blk = %#lx, pa = %#lx\r\n", blk, pa);
        hd_common_mem_release_block(blk);
        ret =  -1;
        goto exit;
    }
    
    *virAddr = va;
    *phyAddr = pa;
    *vblk = blk;
    

exit:
    return ret;
}


/**
* @function   vdec_ntv3_ChangeEncType
* @brief     释放解码缓存
* @param[in]   pVdecFrame 解码信息
* @param[out] None
* @return    INT32
*/
static INT32 vdec_ntv3_freeBuffer(NT_VDEC_FRAME_HAL *pVdecFrame)
{
    INT32 ret = SAL_SOK;
    HD_RESULT hd_ret;

    if (pVdecFrame->pVirAddr != NULL) {
        hd_ret = hd_common_mem_munmap(pVdecFrame->pVirAddr, pVdecFrame->size);
        if (hd_ret != SAL_SOK) {
            SAL_ERROR("hd_common_mem_munmap fail\r\n");
            ret =  SAL_FAIL;
            return ret;
        }
    }
    
    hd_ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)pVdecFrame->blk);
    if (hd_ret != SAL_SOK) {
        SAL_ERROR("hd_common_mem_release_block fail\r\n");
        ret =  SAL_FAIL;
        return ret;
    }
    
    return ret;
}



/**
 * @function   vdec_ntv3_ChangeEncType
 * @brief	  映射平台相关编码格式
 * @param[in]  INT32 encType 编码格式
 * @param[out] None
 * @return	  INT32
 */
static INT32 vdec_ntv3_ChangeEncType(INT32 encType)
{
    INT32 type = 0;

    switch (encType)
    {
        case H264:
        {
            type = HD_CODEC_TYPE_H264;
            break;
        }
        case H265:
        {
            type = HD_CODEC_TYPE_H265;
            break;
        }
        case MJPEG:
        {
            type = HD_CODEC_TYPE_JPEG;
            break;
        }
        default:
        {
            VDEC_LOGE("err type %d\n", type);
            return -1;
        }
    }

    return type;
}




/**
 * @function   vdec_ntv3_ChnCreate
 * @brief	  解码通道创建
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_ChnCreate(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = 0;
    HD_VIDEODEC_PATH_CONFIG dec_config = {0};
#ifdef _VDEC_PUSH_IN_
    INT32 i = 0;
    NT_VDEC_FRAME_HAL *pVdecFrame = NULL;
    HD_VIDEO_FRAME dec_out_buffer = {0};
    UINT32 frame_buf_size = 0;
#endif

    if ((s32Ret = hd_videodec_open(HD_VIDEODEC_IN(0, pVdecHalHdl->decChan), HD_VIDEODEC_OUT(0, pVdecHalHdl->decChan), &pVdecHalHdl->stVdecId.stVdecPathId)) != HD_OK) 
    {
        VDEC_LOGE("Error hd_videodec_open %u\n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    
    /* Set videodec parameters */
    memset(&dec_config, 0x0, sizeof(HD_VIDEODEC_PATH_CONFIG));
#ifdef  _VDEC_PUSH_IN_
    dec_config.max_mem.dim.w =  ALIGN_CEIL(pVdecHalHdl->vdechiv5Hdl.u32width, 64);  
    dec_config.max_mem.dim.h =  ALIGN_CEIL(pVdecHalHdl->vdechiv5Hdl.u32height, 64);
    dec_config.max_mem.frame_rate = 30;
    dec_config.max_mem.max_ref_num = 1;
    dec_config.max_mem.max_bitrate = 3 * 1024 * 1024;
    dec_config.max_mem.codec_type = vdec_ntv3_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type);
#else
    dec_config.max_mem.frame_rate = 30;          // 默认设置为30fps
    dec_config.max_mem.dim.w = ALIGN_CEIL(pVdecHalHdl->vdechiv5Hdl.u32width, 64);  
    dec_config.max_mem.dim.h = ALIGN_CEIL(pVdecHalHdl->vdechiv5Hdl.u32height, 64);
    dec_config.max_mem.bs_counts = 8;
    dec_config.max_mem.codec_type = vdec_ntv3_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type);
    dec_config.max_mem.max_ref_num = 2;
    dec_config.max_mem.max_bitrate = 3 * 1024 * 1024;
    dec_config.max_mem.ddr_id = pVdecHalHdl->stVdecId.dec_ddr_id;
    
    
    /* Set videodec out pool */
    dec_config.data_pool[0].mode = HD_VIDEODEC_POOL_ENABLE;
    dec_config.data_pool[0].ddr_id = pVdecHalHdl->stVdecId.dec_ddr_id;
    dec_config.data_pool[0].counts = HD_VIDEODEC_SET_COUNT(3, 0);  /*数据缓存区，根据实际测试更改*/
    dec_config.data_pool[1].mode = HD_VIDEODEC_POOL_DISABLE;       /*当前只开一路输出后续需要多个同源窗口再增加*/
    dec_config.data_pool[2].mode = HD_VIDEODEC_POOL_DISABLE;
    dec_config.data_pool[3].mode = HD_VIDEODEC_POOL_DISABLE;
#endif
    s32Ret = hd_videodec_set(pVdecHalHdl->stVdecId.stVdecPathId, HD_VIDEODEC_PARAM_PATH_CONFIG, &dec_config);
    if (s32Ret != SAL_SOK) 
    {
        return s32Ret;
    }

#ifdef _VDEC_PUSH_IN_
    /* 1. 创建管理满队列 */
    if (SAL_SOK != DSA_QueCreate(&pVdecHalHdl->stVdecId.pstFullQue, VDEC_QUE_NUM))
    {
        SVA_LOGE("error !!!\n");
        return SAL_FAIL;
    }

    /* 2. 创建管理空队列 */
    if (SAL_SOK != DSA_QueCreate(&pVdecHalHdl->stVdecId.pstEmptQue, VDEC_QUE_NUM))
    {
        SVA_LOGE("error !!!\n");
        return SAL_FAIL;
    }

    /* 3. 创建管理临时缓存列需要等待vpe结束释放*/
    if (SAL_SOK != DSA_QueCreate(&pVdecHalHdl->stVdecId.pstTmpQue, VDEC_QUE_NUM))
    {
        SVA_LOGE("error !!!\n");
        return SAL_FAIL;
    }

    switch(pVdecHalHdl->vdechiv5Hdl.type)
    {
        case MJPEG:
            dec_out_buffer.sign = MAKEFOURCC('J', 'P', 'G', 'D');
            
            break;
        case H264:
            dec_out_buffer.sign = MAKEFOURCC('2', '6', '4', 'D');
   
            break;
        case H265:
            dec_out_buffer.sign = MAKEFOURCC('2', '6', '5', 'D');
            break;
        default:
            break;
    }

    dec_out_buffer.dim.w  = dec_config.max_mem.dim.w;
    dec_out_buffer.dim.h  = dec_config.max_mem.dim.h;
    dec_out_buffer.ddr_id =  pVdecHalHdl->stVdecId.dec_ddr_id;
    frame_buf_size = hd_common_mem_calc_buf_size(&dec_out_buffer);

    for(i = 0; i < VDEC_QUE_NUM; i++)
    {
        pVdecFrame = (NT_VDEC_FRAME_HAL *)SAL_memMalloc(sizeof(NT_VDEC_FRAME_HAL), "vdec", "vdec_ntv3");
        if(pVdecFrame == NULL)
        {
            VDEC_LOGW("SAL_memMalloc dec_out fail\n");
            continue;
        }
        s32Ret = vdec_ntv3_allocateBuffer(HD_COMMON_MEM_DISP_DEC_OUT_POOL, frame_buf_size, dec_out_buffer.ddr_id,
                              &pVdecFrame->pVirAddr, &pVdecFrame->u64PhyAddr, &(pVdecFrame->blk));
        if (s32Ret < 0)
        {
            VDEC_LOGW("allocate dec_out fail\n");
             return SAL_FAIL;
            
        }

        pVdecFrame->size = frame_buf_size;

        if (SAL_SOK != DSA_QuePut(&pVdecHalHdl->stVdecId.pstEmptQue, (void *)pVdecFrame, SAL_TIMEOUT_NONE))
        {
            SVA_LOGE("error !!!\n");
            return SAL_FAIL;
        }
       
    }
#endif

    VDEC_LOGI("chn %d Create success!\n", pVdecHalHdl->decChan);
    
    return SAL_SOK;
}

/**
 * @function   vdec_ntv3_VdecCreate
 * @brief	  解码器创建
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecCreate(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm)
{
    INT32 s32Ret = SAL_SOK;
    INT32 cnt = 10;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(pVdecHalPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)SAL_memMalloc(sizeof(VDEC_HAL_PLT_HANDLE), "VDEC", "mallocOs");
    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl->decChan = u32VdecChn;
    pVdecHalHdl->vdechiv5Hdl.u32width = pVdecHalPrm->vdecPrm.decWidth;
    pVdecHalHdl->vdechiv5Hdl.u32height = pVdecHalPrm->vdecPrm.decHeight;
    pVdecHalHdl->vdechiv5Hdl.type = pVdecHalPrm->vdecPrm.encType;
    pVdecHalHdl->stVdecId.dec_ddr_id = DDR_ID1;    //后续通过sys接口获取；

    while (2 == pVdecHalHdl->vdecOutStatue)
    {
        usleep(100 * 1000);
        if (cnt <= 0)
        {
            VDEC_LOGW("vdecChn %d out(vpss) is use\n", u32VdecChn);
            break;
        }

        cnt--;
    }

    pVdecHalHdl->vdecOutStatue = 0;

    s32Ret = vdec_ntv3_ChnCreate(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_CONFIG);
    }

    pVdecHalHdl->vdecOutStatue = 1;
    pVdecHalHdl->decChan = u32VdecChn;
    pVdecHalPrm->decPlatHandle = (void *)pVdecHalHdl;

    VDEC_LOGI("chn %d w %d h %d type %d\n", u32VdecChn, pVdecHalHdl->vdechiv5Hdl.u32width, pVdecHalHdl->vdechiv5Hdl.u32height, vdec_ntv3_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type));
    return s32Ret;
}

/**
 * @function   vdec_hisi_VdecStart
 * @brief	   启动解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecStart(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

#ifdef _VDEC_PUSH_IN_
     s32Ret = hd_videodec_start(pVdecHalHdl->stVdecId.stVdecPathId);
#else
    s32Ret = hd_videodec_start_list(&pVdecHalHdl->stVdecId.stVdecPathId, 1);
#endif
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d start recv stream failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return SAL_FAIL;
    }

    VDEC_LOGI("chn %d start!\n", pVdecHalHdl->decChan);


    return SAL_SOK;
}

/**
* @function   vdec_ntv3_VdecFreeQue
* @brief     释放解码队列
* @param[in]   handle 解码句柄
* @param[out] None
* @return    INT32
*/
static INT32 vdec_ntv3_VdecFreeQue(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    NT_VDEC_FRAME_HAL *pVdecFrame = NULL;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;


    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;
     
    while(!DSA_QueIsEmpty(&pVdecHalHdl->stVdecId.pstFullQue))
    {    
         s32Ret = DSA_QueGet(&pVdecHalHdl->stVdecId.pstFullQue, (void **)&pVdecFrame, SAL_TIMEOUT_NONE);
         if(SAL_SOK == s32Ret)
         {
             if(pVdecFrame!= NULL)
             {
                 vdec_ntv3_freeBuffer(pVdecFrame);
                 SAL_memfree(pVdecFrame, "vdec", "vdec_ntv3");
             }
         }
    }
   
    while(!DSA_QueIsEmpty(&pVdecHalHdl->stVdecId.pstEmptQue))
    {  
        s32Ret = DSA_QueGet(&pVdecHalHdl->stVdecId.pstEmptQue, (void **)&pVdecFrame, SAL_TIMEOUT_NONE);
        if(SAL_SOK == s32Ret)
        {
            if(pVdecFrame!= NULL)
            {
                vdec_ntv3_freeBuffer(pVdecFrame);
                SAL_memfree(pVdecFrame, "vdec", "vdec_ntv3");
            }
        }
    }
    
    while(!DSA_QueIsEmpty(&pVdecHalHdl->stVdecId.pstTmpQue))
    { 
        s32Ret = DSA_QueGet(&pVdecHalHdl->stVdecId.pstTmpQue, (void **)&pVdecFrame, SAL_TIMEOUT_NONE);
        if(SAL_SOK == s32Ret)
        {
            if(pVdecFrame!= NULL)
            {
                vdec_ntv3_freeBuffer(pVdecFrame);
                SAL_memfree(pVdecFrame, "vdec", "vdec_ntv3");
            }
        }
    }

    DSA_QueDelete(&pVdecHalHdl->stVdecId.pstFullQue);
    DSA_QueDelete(&pVdecHalHdl->stVdecId.pstEmptQue);
    DSA_QueDelete(&pVdecHalHdl->stVdecId.pstTmpQue);

    return SAL_SOK;
}
/**
 * @function   vdec_ntv3_VdecDeinit
 * @brief	   释放解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecDeinit(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = vdec_ntv3_VdecFreeQue(handle);
    if ( s32Ret  != SAL_SOK) 
    {
        VDEC_LOGE("Error vdec_ntv3_VdecFreeQue %d\n", s32Ret);
    }

    s32Ret = hd_videodec_close(pVdecHalHdl->stVdecId.stVdecPathId);
    if ( s32Ret  != HD_OK) 
    {
        VDEC_LOGE("Error hd_videodec_close %d\n", s32Ret);
    }

    SAL_memfree(pVdecHalHdl, "VDEC", "mallocOs");

    return SAL_SOK;
}

/**
 * @function   vdec_ntv3_VdecSetPrm
 * @brief	   设置解码器参数
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 解码参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecSetPrm(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm)
{
    INT32 s32Ret = SAL_SOK;
/*	hi_vdec_chn_attr chn_attr; */
/*	hi_vdec_chn_pool pool; */
/*	hi_vdec_chn_param chn_param; */

    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(pVdecHalPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)pVdecHalPrm->decPlatHandle;


    VDEC_LOGI("decChan %d ok\n", pVdecHalHdl->decChan);


    return s32Ret;
}

/**
 * @function   vdec_ntv3_VdecClear
 * @brief	   清除解码器数据
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecClear(void *handle)
{
    return SAL_SOK;
}

/**
 * @function   vdec_ntv3_VdecStop
 * @brief	   停止解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecStop(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = hd_videodec_stop(pVdecHalHdl->stVdecId.stVdecPathId);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d stop recv stream failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   vdec_ntv3_UpdatVdecChn
 * @brief	   更新解码器
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_UpdatVdecChn(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = SAL_SOK;
    INT32 cnt = 10;

    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    while (2 == pVdecHalHdl->vdecOutStatue)
    {
        usleep(100 * 1000);
        if (cnt <= 0)
        {
            VDEC_LOGW("vdecChn  is use\n");
            break;
        }

        cnt--;
    }

    pVdecHalHdl->vdecOutStatue = 0;

    s32Ret = vdec_ntv3_VdecStop(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }

    s32Ret = vdec_ntv3_VdecDeinit(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }


    s32Ret = vdec_ntv3_ChnCreate(pVdecHalHdl);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_INIT);
    }

    pVdecHalHdl->vdecOutStatue = 1;

    return SAL_SOK;
}



	/* Open output file */

	/* Write buffer to output file */


/**
 * @function   vdec_ntv3_VdecDecframe
 * @brief	   数据帧送入解码器
 * @param[in]  void *handle handle
 * @param[in]  SAL_VideoFrameBuf *pFrameIn 数据源
 * @param[in]  SAL_VideoFrameBuf *pFrameOut 输出数据
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecDecframe(void *handle, SAL_VideoFrameBuf *pFrameIn, SAL_VideoFrameBuf *pFrameOut)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;
    INT32 wait_ms = 2000;
#ifndef _VDEC_PUSH_IN_  
    HD_VIDEODEC_SEND_LIST send_list = {0};
#else
    HD_VIDEODEC_BS bs_in_buffer = {0};
    HD_VIDEO_FRAME stVideoFrame = {0};
    HD_VIDEO_FRAME dec_out_buffer = {0};
    unsigned int frame_buf_size;
    NT_VDEC_FRAME_HAL *pVdecFrame = NULL;

#endif

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pFrameIn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pFrameOut, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;
    if ( NULL == pVdecHalHdl )
    {
        VENC_LOGE("handle is NULL err \n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    if (pFrameIn->encodeType != MJPEG)
    {
        if (pVdecHalHdl->vdechiv5Hdl.u32width <= MIN_VDEC_WIDTH || pVdecHalHdl->vdechiv5Hdl.u32height <= MIN_VDEC_HEIGHT
            || pVdecHalHdl->vdechiv5Hdl.u32width > MAX_VDEC_WIDTH || pVdecHalHdl->vdechiv5Hdl.u32height > MAX_VDEC_HEIGHT)
        {
            VDEC_LOGE("vdec chn %d width %d height %d is error\n", pVdecHalHdl->decChan, pVdecHalHdl->vdechiv5Hdl.u32width, pVdecHalHdl->vdechiv5Hdl.u32height);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
        }

        if (pFrameIn->frameParam.width != pVdecHalHdl->vdechiv5Hdl.u32width || pFrameIn->frameParam.height != pVdecHalHdl->vdechiv5Hdl.u32height
            || vdec_ntv3_ChangeEncType(pFrameIn->frameParam.encodeType) != vdec_ntv3_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type))
        {
            VDEC_LOGE("vdec chn %d width %d height %d type %d is error dec[w %d h %d type %d]\n", pVdecHalHdl->decChan, pFrameIn->frameParam.width, pFrameIn->frameParam.height,
                      pFrameIn->frameParam.encodeType, pVdecHalHdl->vdechiv5Hdl.u32width, pVdecHalHdl->vdechiv5Hdl.u32height, pVdecHalHdl->vdechiv5Hdl.type);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
        }

    }

#ifndef _VDEC_PUSH_IN_ 

    send_list.path_id = pVdecHalHdl->stVdecId.stVdecPathId;
    send_list.user_bs.p_bs_buf = (CHAR *)pFrameIn->virAddr[0];
    send_list.user_bs.bs_buf_size = pFrameIn->bufLen;

    s32Ret =  hd_videodec_send_list(&send_list, 1, wait_ms);
    if (s32Ret != HD_OK) 
    {
        VDEC_LOGE("hd_videodec_send_list fail\n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_DATA);
    }
#else

    if (pFrameIn->encodeType == MJPEG)
    {
        stVideoFrame.sign  = MAKEFOURCC('J', 'P', 'G', 'D');
        stVideoFrame.pxlfmt = HD_VIDEO_PXLFMT_YUV420_W8;
        dec_out_buffer.sign = MAKEFOURCC('J', 'P', 'G', 'D');
        dec_out_buffer.pxlfmt = HD_VIDEO_PXLFMT_YUV420_W8;
    }
    else
    {
        switch(pFrameIn->frameParam.encodeType)
        {
            case H264:
                stVideoFrame.sign = MAKEFOURCC('2', '6', '4', 'D');
                dec_out_buffer.sign = MAKEFOURCC('2', '6', '4', 'D');
                break;
            case H265:
                stVideoFrame.sign = MAKEFOURCC('2', '6', '5', 'D');
                dec_out_buffer.sign = MAKEFOURCC('2', '6', '5', 'D');
                break;
            default:
                stVideoFrame.sign = MAKEFOURCC('2', '6', '4', 'D');
                dec_out_buffer.sign = MAKEFOURCC('2', '6', '4', 'D');
                break;
        }
    }
    stVideoFrame.ddr_id = pVdecHalHdl->stVdecId.dec_ddr_id;
    stVideoFrame.dim.w = pVdecHalHdl->vdechiv5Hdl.u32width;
    stVideoFrame.dim.h = pVdecHalHdl->vdechiv5Hdl.u32height;

    frame_buf_size = hd_common_mem_calc_buf_size(&stVideoFrame);
    if(frame_buf_size == 0)
    {
         VDEC_LOGE("hd_common_mem_calc_buf_size fail\n");
    }

    /* 获取 buffer */
    s32Ret = DSA_QueGet(&pVdecHalHdl->stVdecId.pstEmptQue, (void **)&pVdecFrame, SAL_TIMEOUT_NONE);
    if (s32Ret != HD_OK)
    {
        s32Ret =  DSA_QueGet( &pVdecHalHdl->stVdecId.pstFullQue, (void **)&pVdecFrame, SAL_TIMEOUT_NONE);
        if (SAL_SOK != s32Ret)
        {
            VENC_LOGE("DSA_QueGet err %#x\n", s32Ret);
            return SAL_FAIL;
        }
        VDEC_LOGD("hd_videodec_push_in_buf fail %d\n", s32Ret);
    }

    dec_out_buffer.sign = stVideoFrame.sign;
    dec_out_buffer.dim.w = pVdecHalHdl->vdechiv5Hdl.u32width;
    dec_out_buffer.dim.h = pVdecHalHdl->vdechiv5Hdl.u32height;
    dec_out_buffer.phy_addr[0] = (UINTPTR)pVdecFrame->u64PhyAddr;
    dec_out_buffer.ddr_id = pVdecHalHdl->stVdecId.dec_ddr_id;

    bs_in_buffer.sign = stVideoFrame.sign;
    bs_in_buffer.ddr_id = pVdecHalHdl->stVdecId.dec_ddr_id;
    bs_in_buffer.size = pFrameIn->bufLen;
    bs_in_buffer.phy_addr = (UINTPTR)pFrameIn->phyAddr[0];


    s32Ret = hd_videodec_push_in_buf(pVdecHalHdl->stVdecId.stVdecPathId , &bs_in_buffer, &dec_out_buffer, wait_ms);
    if (s32Ret != HD_OK) 
    {
        VDEC_LOGE("hd_videodec_push_in_buf fail %x\n", s32Ret);
        DSA_QuePut(&pVdecHalHdl->stVdecId.pstEmptQue, (void *)pVdecFrame, SAL_TIMEOUT_NONE);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_DATA);
    }

    s32Ret = hd_videodec_pull_out_buf(pVdecHalHdl->stVdecId.stVdecPathId , &dec_out_buffer, wait_ms);
    if (s32Ret != HD_OK) 
    {
        hd_videodec_release_out_buf(pVdecHalHdl->stVdecId.stVdecPathId, &dec_out_buffer);
        VDEC_LOGE("hd_videodec_pull_out_buf fail %d \n", s32Ret);
        DSA_QuePut(&pVdecHalHdl->stVdecId.pstEmptQue, (void *)pVdecFrame, SAL_TIMEOUT_NONE);
        return SAL_FAIL;
    }


    
    memcpy(&(pVdecFrame->vfrm), (void *)&dec_out_buffer, sizeof(HD_VIDEO_FRAME));
    DSA_QuePut(&pVdecHalHdl->stVdecId.pstFullQue, (void *)pVdecFrame, SAL_TIMEOUT_NONE);


#endif
    return s32Ret;
}

/**
 * @function   TdeQuickCopyYuv
 * @brief	   数据帧tde拷贝
 * @param[in]  VIDEO_FRAME_INFO_S *pstDstframe 源数据
 * @param[in]  VIDEO_FRAME_INFO_S *pstSrcframe 目的
 * @param[out] None
 * @return	  INT32
 */
static INT32 TdeQuickCopyYuv(HD_VIDEO_FRAME *pstDstframe, HD_VIDEO_FRAME *pstSrcframe)
{
    INT32 s32Ret = 0;
    HD_GFX_COPY copy_param;


    copy_param.src_region.x = 0;
    copy_param.src_region.y = 0;
    copy_param.src_region.w = pstSrcframe->dim.w;
    copy_param.src_region.h = pstSrcframe->dim.h;

    copy_param.src_img.dim.w = pstSrcframe->dim.w;
    copy_param.src_img.dim.h = pstSrcframe->dim.h;
    copy_param.src_img.p_phy_addr[0] = pstSrcframe->phy_addr[0];
    copy_param.src_img.p_phy_addr[1] = pstSrcframe->phy_addr[0]+ pstSrcframe->dim.w*pstSrcframe->dim.h;
    copy_param.src_img.ddr_id = pstDstframe->ddr_id;
    copy_param.src_img.format = HD_VIDEO_PXLFMT_YUV420;
    copy_param.dst_pos.x = 0;
    copy_param.dst_pos.y = 0;
    copy_param.dst_img.dim.w = pstDstframe->dim.w;
    copy_param.dst_img.dim.h = pstDstframe->dim.h;
    copy_param.dst_img.p_phy_addr[0] =pstDstframe->phy_addr[0];
    copy_param.dst_img.p_phy_addr[1] =pstDstframe->phy_addr[0] + pstSrcframe->dim.w*pstSrcframe->dim.h;
    copy_param.dst_img.ddr_id = pstDstframe->ddr_id;
    copy_param.dst_img.format = HD_VIDEO_PXLFMT_YUV420;

        
    s32Ret = hd_gfx_copy(&copy_param);
    if (s32Ret != HD_OK) 
    {
        printf("hd_gfx_copy fail\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   vdec_ntv3_VdecMakeframe
 * @brief	   生成帧数据
 * @param[in]  PIC_FRAME_PRM *pPicFramePrm 数据内存
 * @param[in]  void *pstVideoFrame 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecMakeframe(PIC_FRAME_PRM *pPicFramePrm, void *pstVideoFrame)
{
    HD_VIDEO_FRAME *pstFrame = NULL;

    CHECK_PTR_NULL(pPicFramePrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pstFrame = (HD_VIDEO_FRAME *)pstVideoFrame;

  //  pstFrame-> = pPicFramePrm->poolId;
    pstFrame->dim.w = pPicFramePrm->width;
    pstFrame->dim.h = pPicFramePrm->height;
   // pstFrame->video_frame.field = HI_VIDEO_FIELD_FRAME;
   // pstFrame->video_frame.video_format = HI_VIDEO_FORMAT_LINEAR;
    pstFrame->pxlfmt = vdec_ntv3_ChangePixFormat(pPicFramePrm->videoFormat);
    pstFrame->phy_addr[0] = (UINTPTR)pPicFramePrm->addr[0];
    pstFrame->phy_addr[1] = (UINTPTR)pPicFramePrm->addr[1];
  
   // pstFrame->loff[0] = pPicFramePrm->stride;
   // pstFrame->loff[1] = pPicFramePrm->stride;
   // pstFrame->ddr_id = DDR1_ADDR_START > pPicFramePrm->phy[0]?DDR_ID0:DDR_ID1;
    
    return SAL_SOK;
}

/**
 * @function   vdec_ntv3_VdecCpyframe
 * @brief	   数据拷贝
 * @param[in]  VDEC_PIC_COPY_EN *copyEn 拷贝方式
 * @param[in]  void *pstSrcframe 数据源
 * @param[in]  void *pstDstframe 目的
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecCpyframe(VDEC_PIC_COPY_EN *copyEn, void *pstSrcVideoFrame, void *pstDstVideoFrame)
{
    INT32 s32Ret = SAL_SOK;
    HD_VIDEO_FRAME *pstSrcFrame = NULL;
    HD_VIDEO_FRAME *pstDstFrame = NULL;
    INT32 i = 0;
    void *srcVirtAddr[2] = {NULL};
    void *dtsVirtAddr[2] = {NULL};

    CHECK_PTR_NULL(copyEn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstSrcVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstDstVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pstSrcFrame = (HD_VIDEO_FRAME *)pstSrcVideoFrame;
    pstDstFrame = (HD_VIDEO_FRAME *)pstDstVideoFrame;

    if (copyEn->copyMeth == VDEC_DATA_ADDR_COPY)
    {
        memcpy(pstDstFrame, pstSrcFrame, sizeof(HD_VIDEO_FRAME));
    }
    else if (copyEn->copyMeth == VDEC_DATA_COPY)
    {
        pstDstFrame->ddr_id = pstSrcFrame->ddr_id;
        pstDstFrame->timestamp = pstSrcFrame->timestamp;
        pstDstFrame->dim.w = pstSrcFrame->dim.w;
        pstDstFrame->dim.h = pstSrcFrame->dim.h;
        pstDstFrame->pxlfmt = pstSrcFrame->pxlfmt;
        pstDstFrame->sign = pstSrcFrame->sign;
        pstDstFrame->count = pstSrcFrame->count;
        
        if (copyEn->copyth == 1)
        {
            srcVirtAddr[0] = (void *)pstSrcFrame->phy_addr[0];
            srcVirtAddr[1] = (void *)pstSrcFrame->phy_addr[1];
            dtsVirtAddr[0] = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, (ULONG)pstDstFrame->phy_addr[0], pstSrcFrame->dim.w * pstSrcFrame->dim.h*3/2); 
            dtsVirtAddr[1] = dtsVirtAddr[0] + pstSrcFrame->dim.w * pstSrcFrame->dim.h;
            for (i = 0; i < pstDstFrame->dim.h; i++)
            {
               
                memcpy(HI_ADDR_U642P(dtsVirtAddr[0]) + i * pstSrcFrame->dim.w, HI_ADDR_U642P(srcVirtAddr[0]) + i * pstSrcFrame->dim.w, pstSrcFrame->dim.w);
                if (i % 2 == 0)
                {
                    memcpy(HI_ADDR_U642P(dtsVirtAddr[1] + i * pstSrcFrame->dim.w / 2), HI_ADDR_U642P(srcVirtAddr[1] + i * pstSrcFrame->dim.w / 2), pstSrcFrame->dim.w);
                }
            }
            hd_common_mem_munmap(dtsVirtAddr[0], pstSrcFrame->dim.w * pstSrcFrame->dim.h*3/2);  /*释放虚拟内存*/
        }
        else
        {
            s32Ret = TdeQuickCopyYuv(pstDstFrame, pstSrcFrame);
            if (s32Ret != SAL_SOK)
            {
                VDEC_LOGE("tde copy failed\n");
                return s32Ret;
            }
        }
    }
    else
    {
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    return s32Ret;
}

/**
 * @function   vdec_ntv3_VdecGetframe
 * @brief	   获取解码器数据帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecGetframe(void *handle, UINT32 dstChn, void *pstframe)
{

    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;
    HD_VIDEO_FRAME *p_video_frame = (HD_VIDEO_FRAME *)pstframe; 
    NT_VDEC_FRAME_HAL *pVdecFrame = NULL;
    INT32 s32Ret = SAL_SOK;

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;
    if ( NULL == pVdecHalHdl )
    {
        VENC_LOGE("err %#x\n", s32Ret);
         return SAL_FAIL;
    }

    /*jpeg抓图获取一帧数据*/
    if(2 == dstChn)
    {
        s32Ret =  DSA_QueGet( &pVdecHalHdl->stVdecId.pstFullQue, (void **)&pVdecFrame, SAL_TIMEOUT_NONE);
        if (SAL_SOK != s32Ret)
        {
                s32Ret =  DSA_QueGet( &pVdecHalHdl->stVdecId.pstEmptQue, (void **)&pVdecFrame, SAL_TIMEOUT_NONE);
                if (SAL_SOK != s32Ret)
                {
                   VENC_LOGE("err %#x\n", s32Ret);
                   return SAL_FAIL;
                }

        }
    }
    else
    {
        s32Ret =  DSA_QueGet( &pVdecHalHdl->stVdecId.pstFullQue, (void **)&pVdecFrame, SAL_TIMEOUT_NONE);
        if (SAL_SOK != s32Ret)
        {
            VENC_LOGD("err %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }
    memcpy(p_video_frame, (void *)&(pVdecFrame->vfrm), sizeof(HD_VIDEO_FRAME));

    DSA_QuePut(&pVdecHalHdl->stVdecId.pstTmpQue, (void *)pVdecFrame, SAL_TIMEOUT_NONE);
    
    return SAL_SOK;

}

/**
 * @function   vdec_ntv3_VdecReleaseframe
 * @brief	   释放解码帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecReleaseframe(void *handle, UINT32 dstChn, void *pstframe)
{
    NT_VDEC_FRAME_HAL *pVdecFrame = NULL;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;
    INT32 s32Ret = SAL_SOK;

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    if ( NULL == pVdecHalHdl )
    {
        VENC_LOGE("err %#x\n", s32Ret);
         return SAL_FAIL;
    }
    
    s32Ret = DSA_QueGet( &pVdecHalHdl->stVdecId.pstTmpQue, (void **)&pVdecFrame, SAL_TIMEOUT_NONE);
    if (SAL_SOK != s32Ret)
    {
        VENC_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }
    
    DSA_QuePut(&pVdecHalHdl->stVdecId.pstEmptQue, (void *)pVdecFrame, SAL_TIMEOUT_NONE);

    return SAL_SOK;
}

/**
 * @function   vdec_ntv3_FreeYuvPoolBlockMem
 * @brief	   释放yuv内存
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_FreeYuvPoolBlockMem(HAL_MEM_PRM *addrPrm)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = mem_hal_vbFree(addrPrm->virAddr,"vdec", "vdec_hal",addrPrm->memSize,addrPrm->vbBlk, addrPrm->poolId);
    if (SAL_SOK != s32Ret)
    {
        VENC_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   vdec_ntv3_GetYuvPoolBlockMem
 * @brief	   申请yuv平台相关内存
 * @param[in]  UINT32 imgW 宽
 * @param[in]  UINT32 imgH 高
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_GetYuvPoolBlockMem(UINT32 imgW, UINT32 imgH, HAL_MEM_PRM *pOutFrm)
{
    UINT32 u32LumaSize = 0;
    UINT32 u32ChrmSize = 0;
    UINT64 u64Size = 0;
    INT32 s32Ret = SAL_SOK; 
    ALLOC_VB_INFO_S stVbInfo = {0};

    u32LumaSize = (imgW * imgH);
    u32ChrmSize = (imgW * imgH) >> 2;
    u64Size	= (UINT64)(u32LumaSize + (u32ChrmSize << 1));

    s32Ret = mem_hal_vbAlloc(u64Size, "vdec", "vdec_hal", NULL, SAL_FALSE, &stVbInfo);
    if (SAL_FAIL == s32Ret)
    {
        DISP_LOGE("mem_hal_vbAlloc failed size %llu !\n", u64Size);
        return SAL_FAIL;
    }

    pOutFrm->phyAddr = stVbInfo.u64PhysAddr;
    pOutFrm->virAddr = stVbInfo.pVirAddr;
    pOutFrm->memSize = u64Size;
    pOutFrm->poolId = 0;
    pOutFrm->vbBlk = stVbInfo.u64VbBlk;


    return SAL_SOK;

}

/**
 * @function   vdec_ntv3_VdecMmap
 * @brief	   帧映射
 * @param[in]  void *pVideoFrame 数据帧
 * @param[in]  PIC_FRAME_PRM *pPicprm  映射地址
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecMmap(void *pVideoFrame, PIC_FRAME_PRM *pPicprm)
{
    HD_VIDEO_FRAME *pstFrame = NULL;
    CHECK_PTR_NULL(pVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pstFrame = (HD_VIDEO_FRAME *)pVideoFrame;



    pPicprm->width = pstFrame->dim.w;
    pPicprm->height = pstFrame->dim.h;
   // pPicprm->stride = pstFrame->loff;
    pPicprm->videoFormat = vdec_ntv3_TransPixFormat(pstFrame->pxlfmt);
   //??? pPicprm->poolId = pstFrame->blk;
  //  pPicprm->addr[0] = (void *)pstFrame->phy_addr[0];
   // pPicprm->addr[1] = (void *)pstFrame->video_frame.virt_addr[1];
    pPicprm->phy[0] = (PhysAddr)pstFrame->phy_addr[0];
    pPicprm->phy[1] = (PhysAddr)pstFrame->phy_addr[1];

    return SAL_SOK;
}

/**
 * @function   vdec_ntv3_init
 * @brief      解码模块初始化
 * @param[in]  void  
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_ntv3_init(void)
{
    INT32 s32Ret = SAL_SOK;
    
    s32Ret = hd_videodec_init();
    if(SAL_SOK != s32Ret)
    {
        VDEC_LOGE("hd_videodec_init fail!!!\n");
    }

    return SAL_SOK;
}


/******************************************************************
   Function:   vdec_halRegister
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
VDEC_HAL_FUNC *vdec_halRegister()
{

    INT32 s32Ret = SAL_SOK;

    memset(&vdecFunc, 0, sizeof(VDEC_HAL_FUNC));

    vdecFunc.VdecCreate = vdec_ntv3_VdecCreate;
    vdecFunc.VdecDeinit = vdec_ntv3_VdecDeinit;
    vdecFunc.VdecClear = vdec_ntv3_VdecClear;
    vdecFunc.VdecSetPrm = vdec_ntv3_VdecSetPrm;
    vdecFunc.VdecStart = vdec_ntv3_VdecStart;
    vdecFunc.VdecStop = vdec_ntv3_VdecStop;
    vdecFunc.VdecDecframe = vdec_ntv3_VdecDecframe;
    vdecFunc.VdecGetframe = vdec_ntv3_VdecGetframe;
    vdecFunc.VdecReleaseframe = vdec_ntv3_VdecReleaseframe;
    vdecFunc.VdecCpyframe = vdec_ntv3_VdecCpyframe;
    vdecFunc.VdecFreeYuvPoolBlockMem = vdec_ntv3_FreeYuvPoolBlockMem;
    vdecFunc.VdecMallocYUVMem = vdec_ntv3_GetYuvPoolBlockMem;
    vdecFunc.VdecMakeframe = vdec_ntv3_VdecMakeframe;
    vdecFunc.VdecMmap = vdec_ntv3_VdecMmap;

    s32Ret = vdec_ntv3_init();
    if(SAL_SOK != s32Ret)
    {
       VDEC_LOGE("hd_videodec_init fail!!!\n");
    }


    return &vdecFunc;
}

