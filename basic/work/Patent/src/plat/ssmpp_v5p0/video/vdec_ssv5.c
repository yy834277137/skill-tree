/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: vdec_hiv5.c
   Description:
   Author:
   Date:
   Modification History:
******************************************************************************/
#include "vdec_ssv5.h"
#include "sal_mem_new.h"
#include "vdec_hal_inter.h"
#include <platform_sdk.h>

#define VDEC_HAL_W_MIM	114
#define VDEC_HAL_H_MIM	114

#define VDEC_HAL_W_MAX	1920
#define VDEC_HAL_H_MAX	1920

typedef struct _VDEC_HIV5_VIDEOATTR_
{
    ot_video_dec_mode dec_mode;
    td_u32 ref_frame_num;
    ot_data_bit_width bit_width;
} VDEC_HIV5_VIDEOATTR;
typedef struct _VDEC_HIV5_PICATTR_
{
    ot_pixel_format pixel_format;
    td_u32 alpha;
} VDEC_HIV5_PICATTR;

typedef struct _VDEC_VB_BUF_SIZE_
{
    td_u32 pic_buf_size;
    td_u32 tmv_buf_size;
    td_bool pic_buf_alloc;
    td_bool tmv_buf_alloc;
} VDEC_VB_BUF_SIZE;


/*解码器属性，平台差异化部分*/
typedef struct _VDEC_HIV5_SPEC_
{
    ot_vb_src vbsrc;
    ot_payload_type type;
    ot_vdec_send_mode mode;
    td_u32 u32width;
    td_u32 u32height;
    td_u32 u32frame_buf_cnt;
    td_u32 u32display_frame_num;
    ot_vb_pool g_pic_vb_pool;
    ot_vb_pool g_tmv_vb_pool;
    union
    {
        VDEC_HIV5_VIDEOATTR vdec_video; /* structure with video (h265/h264) */
        VDEC_HIV5_PICATTR vdec_picture; /* structure with picture (jpeg/mjpeg) */
    };
} VDEC_HIV5_SPEC;

typedef struct _VDEC_HAL_PLT_HANDLE_
{
    INT32 decChan;                                            /*对应实际的解码器通道号*/
    INT32 vdecOutStatue;                                      /* 解码器输出创建1，解码器输出未创建0, 2 正在使用 */
    VDEC_HIV5_SPEC vdechiv5Hdl;
} VDEC_HAL_PLT_HANDLE;

static VDEC_HAL_FUNC vdecFunc;

/******************************************************************
   Function:   vdec_hiv5_MemAlloc
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
VOID *vdec_hiv5_MemAlloc(UINT32 u32Format, void *prm)
{
    return NULL;
}

/******************************************************************
   Function:   vdec_hiv5_MemFree
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 vdec_hiv5_MemFree(void *ptr, void *prm)
{
    INT32 s32Ret = SAL_SOK;

    return s32Ret;
}

/**
 * @function   vdec_hiv5_ChangePixFormat
 * @brief	  映射平台相关格式
 * @param[in]  INT32 pixFormat 数据像素格式
 * @param[out] None
 * @return	  INT32
 */

static INT32 vdec_hiv5_ChangePixFormat(INT32 pixFormat)
{
    INT32 format = 0;

    switch (pixFormat)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        {
            format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
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
 * @function   vdec_hiv5_TransPixFormat
 * @brief	  映射业务相关格式
 * @param[in]  INT32 pixFormat 数据像素格式
 * @param[out] None
 * @return	  INT32
 */

static INT32 vdec_hiv5_TransPixFormat(INT32 pixFormat)
{
    INT32 format = 0;

    switch (pixFormat)
    {
        case OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420:
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
 * @function   vdec_hiv5_TransEncType
 * @brief	  映射业务相关编码格式
 * @param[in]  INT32 encType 编码格式
 * @param[out] None
 * @return	  INT32
 */

INT32 vdec_hiv5_TransEncType(INT32 encType)
{
    INT32 type = 0;

    switch (encType)
    {
        case OT_PT_H264:
        {
            type = H264;
            break;
        }
        case OT_PT_H265:
        {
            type = H265;
            break;
        }
        case OT_PT_JPEG:
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
 * @function   vdec_hiv5_ChangeEncType
 * @brief	  映射平台相关编码格式
 * @param[in]  INT32 encType 编码格式
 * @param[out] None
 * @return	  INT32
 */
static INT32 vdec_hiv5_ChangeEncType(INT32 encType)
{
    INT32 type = 0;

    switch (encType)
    {
        case H264:
        {
            type = OT_PT_H264;
            break;
        }
        case H265:
        {
            type = OT_PT_H265;
            break;
        }
        case MJPEG:
        {
            type = OT_PT_MJPEG;
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
 * @function   vdec_hiv5_SetModParam
 * @brief	  设置模式
 * @param[in]  ot_vb_src vbModule 模式
 * @param[out] None
 * @return	  INT32
 */
static INT32 vdec_hiv5_SetModParam(ot_vb_src vbModule)
{
    INT32 s32Ret = TD_SUCCESS;
    ot_vdec_mod_param mod_param;

    memset(&mod_param, 0x00, sizeof(ot_vdec_mod_param));

    s32Ret = ss_mpi_vdec_get_mod_param(&mod_param);
    if (TD_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error. ss_mpi_vdec_get_mod_param \n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    if (mod_param.vb_src != vbModule)
    {
        mod_param.vb_src = vbModule;

        s32Ret = ss_mpi_vdec_set_mod_param(&mod_param);
        if (TD_SUCCESS != s32Ret)
        {
            VDEC_LOGE("error. ss_mpi_vdec_set_mod_param\n");
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
        }
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_DeleteVBPool
 * @brief	  删除vb内存
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_DeleteVBPool(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = SAL_SOK;

    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    if (pVdecHalHdl->vdechiv5Hdl.vbsrc == OT_VB_SRC_MOD)
    {
        s32Ret = ss_mpi_vb_exit_mod_common_pool(OT_VB_UID_VDEC);
        if (s32Ret != TD_SUCCESS)
        {
            VDEC_LOGE("ss_mpi_vb_exit_mod_common_pool err 0x%x\n", s32Ret);
        }
    }
    else if (pVdecHalHdl->vdechiv5Hdl.vbsrc == OT_VB_SRC_USER)
    {
        if (pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool != OT_VB_INVALID_POOL_ID)
        {
            s32Ret = ss_mpi_vb_destroy_pool(pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool);
            if (s32Ret != TD_SUCCESS)
            {
                VDEC_LOGE("ss_mpi_vb_destroy_pool g_pic_vb_pool err 0x%x\n", s32Ret);
            }

            pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool = OT_VB_INVALID_POOL_ID;
        }

        if (pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool != OT_VB_INVALID_POOL_ID)
        {
            s32Ret = ss_mpi_vb_destroy_pool(pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool);
            if (s32Ret != TD_SUCCESS)
            {
                VDEC_LOGE("ss_mpi_vb_destroy_pool g_tmv_vb_pool err 0x%x\n", s32Ret);
            }

            pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool = OT_VB_INVALID_POOL_ID;
        }
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_InitVbPool
 * @brief	  初始化vb内存
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_InitVbPool(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = SAL_SOK;
    ot_pic_buf_attr buf_attr = { 0 };
    ot_vb_cfg vb_conf;
    ot_vb_pool_cfg vb_pool_cfg;
    VDEC_VB_BUF_SIZE vdec_buf = {0};

    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    (td_void)memset_s(&vb_conf, sizeof(ot_vb_cfg), 0, sizeof(ot_vb_cfg));

    buf_attr.align = 2;
    buf_attr.height = pVdecHalHdl->vdechiv5Hdl.u32height;
    buf_attr.width = pVdecHalHdl->vdechiv5Hdl.u32width;

    if (vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_H265)
    {
        buf_attr.bit_width = pVdecHalHdl->vdechiv5Hdl.vdec_video.bit_width;
        buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        vdec_buf.pic_buf_size = ot_vdec_get_pic_buf_size(vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type), &buf_attr);
        vdec_buf.tmv_buf_size
            = ot_vdec_get_tmv_buf_size(vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type), pVdecHalHdl->vdechiv5Hdl.u32width, pVdecHalHdl->vdechiv5Hdl.u32height);
    }
    else if (vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_H264)
    {
        buf_attr.bit_width = pVdecHalHdl->vdechiv5Hdl.vdec_video.bit_width;
        buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        vdec_buf.pic_buf_size = ot_vdec_get_pic_buf_size(vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type), &buf_attr);
        if (pVdecHalHdl->vdechiv5Hdl.vdec_video.dec_mode == OT_VIDEO_DEC_MODE_IPB)
        {
            vdec_buf.tmv_buf_size
                = ot_vdec_get_tmv_buf_size(vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type), pVdecHalHdl->vdechiv5Hdl.u32width, pVdecHalHdl->vdechiv5Hdl.u32height);
        }
    }
    else if (vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_MJPEG)
    {
        buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
        buf_attr.pixel_format = pVdecHalHdl->vdechiv5Hdl.vdec_picture.pixel_format;
        vdec_buf.pic_buf_size = ot_vdec_get_pic_buf_size(vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type), &buf_attr);
    }
    else
    {
        VDEC_LOGE("invalid type %d\n", vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type));
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    if (pVdecHalHdl->vdechiv5Hdl.vbsrc == OT_VB_SRC_MOD)
    {
    	vb_conf.common_pool[0].blk_size = vdec_buf.pic_buf_size;
	    vb_conf.common_pool[0].blk_cnt += 2 * pVdecHalHdl->vdechiv5Hdl.u32frame_buf_cnt;
	    vb_conf.common_pool[1].blk_size = vdec_buf.tmv_buf_size;
	    vb_conf.common_pool[1].blk_cnt += 2 * (pVdecHalHdl->vdechiv5Hdl.vdec_video.ref_frame_num + 1);
	    vb_conf.max_pool_cnt = 2;
        ss_mpi_vb_exit_mod_common_pool(OT_VB_UID_VDEC);
        s32Ret = ss_mpi_vb_set_mod_pool_cfg(OT_VB_UID_VDEC, &vb_conf);
        s32Ret = ss_mpi_vb_init_mod_common_pool(OT_VB_UID_VDEC);
        if (s32Ret != TD_SUCCESS)
        {
            VDEC_LOGE("ss_mpi_vb_exit_mod_common_pool fail for 0x%x\n", s32Ret);
            ss_mpi_vb_exit_mod_common_pool(OT_VB_UID_VDEC);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
        }
    }
    else if (pVdecHalHdl->vdechiv5Hdl.vbsrc == OT_VB_SRC_USER)
    {
    	if (vdec_buf.pic_buf_size > 0 && pVdecHalHdl->vdechiv5Hdl.u32frame_buf_cnt > 0)
    	{
    		memset(&vb_pool_cfg,0x00,sizeof(ot_vb_pool_cfg));
	        vb_pool_cfg.blk_size = vdec_buf.pic_buf_size;
	        vb_pool_cfg.blk_cnt = pVdecHalHdl->vdechiv5Hdl.u32frame_buf_cnt;
	        vb_pool_cfg.remap_mode = OT_VB_REMAP_MODE_NONE;
			pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool = ss_mpi_vb_create_pool(&vb_pool_cfg);
			//VDEC_LOGW("pic poold %d size %d\n",pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool,vb_pool_cfg.blk_size);
	        if (pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool == OT_VB_INVALID_POOL_ID)
	        {
	        	VDEC_LOGE("create pic pool %d failed \n",pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool);
	            goto fail;
	        }
    	}
		
		if (vdec_buf.tmv_buf_size > 0)
		{
			memset(&vb_pool_cfg,0x00,sizeof(ot_vb_pool_cfg));
	        vb_pool_cfg.blk_size = vdec_buf.tmv_buf_size;
	        vb_pool_cfg.blk_cnt = pVdecHalHdl->vdechiv5Hdl.vdec_video.ref_frame_num + 1;
	        vb_pool_cfg.remap_mode = OT_VB_REMAP_MODE_NONE;
			pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool = ss_mpi_vb_create_pool(&vb_pool_cfg);
			//VDEC_LOGW("tmv poold %d size %d\n",pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool,vb_pool_cfg.blk_size);
	        if (pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool == OT_VB_INVALID_POOL_ID)
	        {
	        	VDEC_LOGE("create tmv pool %d failed\n",pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool);
	            goto fail;
	        }
		}
		
		VDEC_LOGI("pollid %d %d\n",pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool,pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool);
    }

    return s32Ret;
fail:
    if (pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool != OT_VB_INVALID_POOL_ID)
    {
        ss_mpi_vb_destroy_pool(pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool);
        pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool = OT_VB_INVALID_POOL_ID;
    }

    if (pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool != OT_VB_INVALID_POOL_ID)
    {
        ss_mpi_vb_destroy_pool(pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool);
        pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool = OT_VB_INVALID_POOL_ID;
    }

    return SAL_FAIL;
}

/**
 * @function   vdec_hiv5_ChnCreate
 * @brief	  解码通道创建
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_ChnCreate(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = 0;
    ot_pic_buf_attr buf_attr;
    ot_vdec_chn_attr chn_attr;
    ot_vdec_chn_pool pool;
    ot_vdec_chn_param chn_param;
/*    ot_vdec_mod_param mod_param; */
    ot_low_delay_info ldy_attr;

    chn_attr.type = vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type);
    chn_attr.mode = pVdecHalHdl->vdechiv5Hdl.mode;
    chn_attr.pic_width = pVdecHalHdl->vdechiv5Hdl.u32width;
    chn_attr.pic_height = pVdecHalHdl->vdechiv5Hdl.u32height;
    chn_attr.stream_buf_size = chn_attr.pic_width * chn_attr.pic_height * 3 / 2 ;
    chn_attr.frame_buf_cnt = pVdecHalHdl->vdechiv5Hdl.u32frame_buf_cnt;

    buf_attr.align = 2;
    buf_attr.width = pVdecHalHdl->vdechiv5Hdl.u32width;
    buf_attr.height = pVdecHalHdl->vdechiv5Hdl.u32height;

    if (vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_H264 || vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_H265)
    {
        buf_attr.bit_width = pVdecHalHdl->vdechiv5Hdl.vdec_video.bit_width;
        buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        chn_attr.video_attr.ref_frame_num = pVdecHalHdl->vdechiv5Hdl.vdec_video.ref_frame_num;
        chn_attr.video_attr.temporal_mvp_en = 1;

        if ((vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_H264)
            && (pVdecHalHdl->vdechiv5Hdl.vdec_video.dec_mode != OT_VIDEO_DEC_MODE_IPB))
        {
            chn_attr.video_attr.temporal_mvp_en = 0;
        }

        chn_attr.frame_buf_size = ot_vdec_get_pic_buf_size(chn_attr.type, &buf_attr);
    }
    else if (vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_JPEG || vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_MJPEG)
    {
        chn_attr.mode = OT_VDEC_SEND_MODE_FRAME;
        buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
        buf_attr.pixel_format = pVdecHalHdl->vdechiv5Hdl.vdec_picture.pixel_format;
        chn_attr.frame_buf_size = ot_vdec_get_pic_buf_size(chn_attr.type, &buf_attr);
    }

    s32Ret = ss_mpi_vdec_create_chn(pVdecHalHdl->decChan, &chn_attr);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d create chn failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    if (pVdecHalHdl->vdechiv5Hdl.vbsrc == OT_VB_SRC_USER)
    {
        pool.pic_vb_pool = pVdecHalHdl->vdechiv5Hdl.g_pic_vb_pool;
        pool.tmv_vb_pool = pVdecHalHdl->vdechiv5Hdl.g_tmv_vb_pool;
        s32Ret = ss_mpi_vdec_attach_vb_pool(pVdecHalHdl->decChan, &pool);
        if (s32Ret != TD_SUCCESS)
        {
            VDEC_LOGE("chn %d attach vb chn failed 0x%x poolid [%d %d]\n", pVdecHalHdl->decChan, s32Ret, pool.pic_vb_pool, pool.tmv_vb_pool);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
        }
    }

    s32Ret = ss_mpi_vdec_get_chn_param(pVdecHalHdl->decChan, &chn_param);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d get vdec chn param failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    if (vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_H264 || vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_H265
        || vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type) == OT_PT_MP4VIDEO)
    {
        chn_param.video_param.dec_mode = pVdecHalHdl->vdechiv5Hdl.vdec_video.dec_mode;
        chn_param.video_param.compress_mode = OT_COMPRESS_MODE_NONE;
        chn_param.video_param.video_format = OT_VIDEO_FORMAT_LINEAR;
        if (chn_param.video_param.dec_mode == OT_VIDEO_DEC_MODE_IPB)
        {
            chn_param.video_param.out_order = OT_VIDEO_OUT_ORDER_DISPLAY;
        }
        else
        {
            chn_param.video_param.out_order = OT_VIDEO_OUT_ORDER_DEC;
        }

        chn_param.video_param.slice_input_en = TD_FALSE;
    }
    else
    {
        chn_param.pic_param.pixel_format = pVdecHalHdl->vdechiv5Hdl.vdec_picture.pixel_format;
        chn_param.pic_param.alpha = pVdecHalHdl->vdechiv5Hdl.vdec_picture.alpha;
    }

    chn_param.display_frame_num = pVdecHalHdl->vdechiv5Hdl.u32display_frame_num;

    s32Ret = ss_mpi_vdec_set_chn_param(pVdecHalHdl->decChan, &chn_param);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d set vdec chn param failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

	s32Ret = ss_mpi_vdec_set_display_mode(pVdecHalHdl->decChan, OT_VIDEO_DISPLAY_MODE_PREVIEW);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d set_display_mode failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    if (0)
    {
        s32Ret = ss_mpi_vdec_get_low_delay_attr(pVdecHalHdl->decChan, &ldy_attr);
        if (s32Ret != TD_SUCCESS)
        {
            VDEC_LOGE("chn %d get_low_delay failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
        }

        ldy_attr.enable = TD_TRUE;
        ldy_attr.line_cnt = chn_attr.pic_height / 4 * 3;

        s32Ret = ss_mpi_vdec_set_low_delay_attr(pVdecHalHdl->decChan, &ldy_attr);
        if (s32Ret != TD_SUCCESS)
        {
            VDEC_LOGE("chn %d set_low_delay failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
        }

        s32Ret = ss_mpi_vdec_set_display_mode(pVdecHalHdl->decChan, OT_VIDEO_DISPLAY_MODE_PREVIEW);
        if (s32Ret != TD_SUCCESS)
        {
            VDEC_LOGE("chn %d set_display_mode failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
        }
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_VdecCreate
 * @brief	  解码器创建
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecCreate(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm)
{
    INT32 s32Ret = SAL_SOK;
    INT32 cnt = 10;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(pVdecHalPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)SAL_memMalloc(sizeof(VDEC_HAL_PLT_HANDLE), "VDEC", "mallocOs");
    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl->decChan = u32VdecChn;
    pVdecHalHdl->vdechiv5Hdl.vbsrc = OT_VB_SRC_USER;
    pVdecHalHdl->vdechiv5Hdl.u32width = pVdecHalPrm->vdecPrm.decWidth;
    pVdecHalHdl->vdechiv5Hdl.u32height = pVdecHalPrm->vdecPrm.decHeight;
    pVdecHalHdl->vdechiv5Hdl.type = pVdecHalPrm->vdecPrm.encType;
    pVdecHalHdl->vdechiv5Hdl.mode = OT_VDEC_SEND_MODE_FRAME;
    pVdecHalHdl->vdechiv5Hdl.u32display_frame_num = 5;

    pVdecHalHdl->vdechiv5Hdl.vdec_video.dec_mode = OT_VIDEO_DEC_MODE_IP;
    pVdecHalHdl->vdechiv5Hdl.vdec_video.bit_width = OT_DATA_BIT_WIDTH_8;

    if (OT_PT_MJPEG == vdec_hiv5_ChangeEncType(pVdecHalPrm->vdecPrm.encType))
    {
        pVdecHalHdl->vdechiv5Hdl.vdec_video.ref_frame_num = 0;
        pVdecHalHdl->vdechiv5Hdl.u32frame_buf_cnt = pVdecHalHdl->vdechiv5Hdl.u32display_frame_num + 1;
        pVdecHalHdl->vdechiv5Hdl.vdec_picture.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        pVdecHalHdl->vdechiv5Hdl.vdec_picture.alpha = 255;
    }
    else
    {
        pVdecHalHdl->vdechiv5Hdl.vdec_video.ref_frame_num = 3;
        pVdecHalHdl->vdechiv5Hdl.u32frame_buf_cnt = pVdecHalHdl->vdechiv5Hdl.u32display_frame_num + 1 + pVdecHalHdl->vdechiv5Hdl.vdec_video.ref_frame_num;
    }

    vdec_hiv5_SetModParam(pVdecHalHdl->vdechiv5Hdl.vbsrc);

    s32Ret = vdec_hiv5_InitVbPool(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("3403 vb pool init failed, s32Ret: 0x%x\n", s32Ret);
        return s32Ret;
    }

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

    s32Ret = vdec_hiv5_ChnCreate(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_CONFIG);
    }

    pVdecHalHdl->vdecOutStatue = 1;
    pVdecHalHdl->decChan = u32VdecChn;
    pVdecHalPrm->decPlatHandle = (void *)pVdecHalHdl;

    VDEC_LOGI("chn %d w %d h %d type %d\n", u32VdecChn, pVdecHalHdl->vdechiv5Hdl.u32width, pVdecHalHdl->vdechiv5Hdl.u32height, vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type));
    return s32Ret;
}

/**
 * @function   vdec_hisi_VdecStart
 * @brief	   启动解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecStart(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = ss_mpi_vdec_start_recv_stream(pVdecHalHdl->decChan);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d start recv stream failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_VdecDeinit
 * @brief	   释放解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecDeinit(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = ss_mpi_vdec_destroy_chn(pVdecHalHdl->decChan);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d destory failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = vdec_hiv5_DeleteVBPool(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }

    SAL_memfree(pVdecHalHdl, "VDEC", "mallocOs");

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_VdecSetPrm
 * @brief	   设置解码器参数
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 解码参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecSetPrm(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm)
{
    INT32 s32Ret = SAL_SOK;
/*	ot_vdec_chn_attr chn_attr; */
/*	ot_vdec_chn_pool pool; */
/*	ot_vdec_chn_param chn_param; */

    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(pVdecHalPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)pVdecHalPrm->decPlatHandle;


    VDEC_LOGI("decChan %d ok\n", pVdecHalHdl->decChan);

    return s32Ret;
}

/**
 * @function   vdec_hiv5_VdecClear
 * @brief	   清除解码器数据
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecClear(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = ss_mpi_vdec_stop_recv_stream(pVdecHalHdl->decChan);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d stop recv stream failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vdec_reset_chn(pVdecHalHdl->decChan);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d reset chn failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vdec_start_recv_stream(pVdecHalHdl->decChan);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d start recv stream failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_VdecStop
 * @brief	   停止解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecStop(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = ss_mpi_vdec_stop_recv_stream(pVdecHalHdl->decChan);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d stop recv stream failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vdec_reset_chn(pVdecHalHdl->decChan);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("chn %d reset chn failed 0x%x\n", pVdecHalHdl->decChan, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_UpdatVdecChn
 * @brief	   更新解码器
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_UpdatVdecChn(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
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

    s32Ret = vdec_hiv5_VdecStop(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }

    s32Ret = vdec_hiv5_VdecDeinit(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }

    s32Ret = vdec_hiv5_InitVbPool(pVdecHalHdl);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_INIT);
    }

    s32Ret = vdec_hiv5_ChnCreate(pVdecHalHdl);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_INIT);
    }

    pVdecHalHdl->vdecOutStatue = 1;

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_VdecDecframe
 * @brief	   清除解码器数据
 * @param[in]  void *handle handle
 * @param[in]  SAL_VideoFrameBuf *pFrameIn 数据源
 * @param[in]  SAL_VideoFrameBuf *pFrameOut 输出数据
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecDecframe(void *handle, SAL_VideoFrameBuf *pFrameIn, SAL_VideoFrameBuf *pFrameOut)
{
    INT32 s32Ret = SAL_SOK;
    td_s32 s32MilliSec = 1000;
    ot_vdec_stream stStream = {0};
    ot_vdec_chn_status stStatus = {0};
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pFrameIn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pFrameOut, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    if (pFrameIn->encodeType != MJPEG)
    {
        if (pVdecHalHdl->vdechiv5Hdl.u32width <= MIN_VDEC_WIDTH || pVdecHalHdl->vdechiv5Hdl.u32height <= MIN_VDEC_HEIGHT
            || pVdecHalHdl->vdechiv5Hdl.u32width > MAX_VDEC_WIDTH || pVdecHalHdl->vdechiv5Hdl.u32height > MAX_VDEC_HEIGHT)
        {
            VDEC_LOGE("vdec chn %d width %d height %d is error\n", pVdecHalHdl->decChan, pVdecHalHdl->vdechiv5Hdl.u32width, pVdecHalHdl->vdechiv5Hdl.u32height);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
        }

        if (pFrameIn->frameParam.width != pVdecHalHdl->vdechiv5Hdl.u32width || pFrameIn->frameParam.height != pVdecHalHdl->vdechiv5Hdl.u32height
            || vdec_hiv5_ChangeEncType(pFrameIn->frameParam.encodeType) != vdec_hiv5_ChangeEncType(pVdecHalHdl->vdechiv5Hdl.type))
        {
            VDEC_LOGE("vdec chn %d width %d height %d type %d is error dec[w %d h %d type %d]\n", pVdecHalHdl->decChan, pFrameIn->frameParam.width, pFrameIn->frameParam.height,
                      pFrameIn->frameParam.encodeType, pVdecHalHdl->vdechiv5Hdl.u32width, pVdecHalHdl->vdechiv5Hdl.u32height, pVdecHalHdl->vdechiv5Hdl.type);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
        }

        s32Ret = ss_mpi_vdec_query_status(pVdecHalHdl->decChan, &stStatus);
        if (TD_SUCCESS != s32Ret)
        {
            VDEC_LOGW("will reset vdec\n");
            s32Ret = vdec_hiv5_VdecClear(pVdecHalHdl);
            if (SAL_SOK != s32Ret)
            {
                VDEC_LOGE("error %#x\n", s32Ret);
                return s32Ret;
            }
        }
    }

    stStream.pts = 0;
    stStream.addr = (UINT8 *)pFrameIn->virAddr[0];
    stStream.len = pFrameIn->bufLen;
    stStream.end_of_frame = TD_TRUE;
    stStream.end_of_stream = 0;
    stStream.need_display = 1;

    s32Ret = ss_mpi_vdec_send_stream(pVdecHalHdl->decChan, &stStream, s32MilliSec);
    if (TD_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error %#x\n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

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
static INT32 TdeQuickCopyYuv(ot_video_frame_info *pstDstframe, ot_video_frame_info *pstSrcframe)
{
    INT32 s32Ret = 0;
    td_s32 s32Handle;
    ot_tde_rect src_rect = {0};
    ot_tde_rect dst_rect = {0};
    ot_tde_surface src_surface = {0};
    ot_tde_surface dst_surface = {0};
    ot_tde_single_src single_src = {0};

    s32Handle = ss_tde_begin_job();
    if (OT_ERR_TDE_INVALID_HANDLE == s32Handle || OT_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        VDEC_LOGE("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    memset(&src_surface, 0, sizeof(ot_tde_surface));
    memset(&dst_surface, 0, sizeof(ot_tde_surface));
    memset(&src_rect, 0, sizeof(ot_tde_rect));
    memset(&dst_rect, 0, sizeof(ot_tde_rect));

    src_surface.alpha_max_is_255 = TD_TRUE;
    src_surface.color_format = OT_TDE_COLOR_FORMAT_BYTE;
    dst_surface.alpha_max_is_255 = TD_TRUE;
    dst_surface.color_format = OT_TDE_COLOR_FORMAT_BYTE;

    /* Y分量 */
    src_surface.width = pstSrcframe->video_frame.width;
    src_surface.height = pstSrcframe->video_frame.height;
    src_surface.stride = pstSrcframe->video_frame.stride[0];
    src_surface.phys_addr = pstSrcframe->video_frame.phys_addr[0];
    src_rect.pos_x = 0;
    src_rect.pos_y = 0;
    src_rect.width = src_surface.width;
    src_rect.height = src_surface.height;

    dst_surface.width = pstDstframe->video_frame.width;
    dst_surface.height = pstDstframe->video_frame.height;
    dst_surface.stride = pstDstframe->video_frame.stride[0];
    dst_surface.phys_addr = pstDstframe->video_frame.phys_addr[0];
    dst_rect.pos_x = 0;
    dst_rect.pos_y = 0;
    dst_rect.width = dst_surface.width;
    dst_rect.height = dst_surface.height;

    single_src.src_surface = &src_surface;
    single_src.dst_surface = &dst_surface;
    single_src.src_rect = &src_rect;
    single_src.dst_rect = &dst_rect;

    s32Ret = ss_tde_quick_copy(s32Handle, &single_src);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("HI_TDE2_QuickCopy failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        ss_tde_cancel_job(s32Handle);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    /* UV分量 */
    src_surface.width = pstSrcframe->video_frame.width;
    src_surface.height = pstSrcframe->video_frame.height / 2;
    src_surface.stride = pstSrcframe->video_frame.stride[1];
    src_surface.phys_addr = pstSrcframe->video_frame.phys_addr[1];
    src_rect.pos_x = 0;
    src_rect.pos_y = 0;
    src_rect.width = src_surface.width;
    src_rect.height = src_surface.height;

    dst_surface.width = pstDstframe->video_frame.width;
    dst_surface.height = pstDstframe->video_frame.height / 2;
    dst_surface.stride = pstDstframe->video_frame.stride[1];
    dst_surface.phys_addr = pstDstframe->video_frame.phys_addr[1];
    dst_rect.pos_x = 0;
    dst_rect.pos_y = 0;
    dst_rect.width = dst_surface.width;
    dst_rect.height = dst_surface.height;

    single_src.src_surface = &src_surface;
    single_src.dst_surface = &dst_surface;
    single_src.src_rect = &src_rect;
    single_src.dst_rect = &dst_rect;

    s32Ret = ss_tde_quick_copy(s32Handle, &single_src);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("HI_TDE2_QuickCopy failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        ss_tde_cancel_job(s32Handle);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    s32Ret = ss_tde_end_job(s32Handle, TD_FALSE, TD_TRUE, 100);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("HI_TDE2_EndJob failed %#x!\n", s32Ret);
        ss_tde_cancel_job(s32Handle);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_VdecMakeframe
 * @brief	   生成帧数据
 * @param[in]  PIC_FRAME_PRM *pPicFramePrm 数据内存
 * @param[in]  void *pstVideoFrame 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecMakeframe(PIC_FRAME_PRM *pPicFramePrm, void *pstVideoFrame)
{
    ot_video_frame_info *pstFrame = NULL;

    CHECK_PTR_NULL(pPicFramePrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pstFrame = (ot_video_frame_info *)pstVideoFrame;

    pstFrame->pool_id = pPicFramePrm->poolId;
    pstFrame->video_frame.width = pPicFramePrm->width;
    pstFrame->video_frame.height = pPicFramePrm->height;
    pstFrame->video_frame.field = OT_VIDEO_FIELD_FRAME;
    pstFrame->video_frame.video_format = OT_VIDEO_FORMAT_LINEAR;
    pstFrame->video_frame.pixel_format = vdec_hiv5_ChangePixFormat(pPicFramePrm->videoFormat);
    pstFrame->video_frame.phys_addr[0] = (td_phys_addr_t)pPicFramePrm->phy[0];
    pstFrame->video_frame.phys_addr[1] = (td_phys_addr_t)pPicFramePrm->phy[1];
    pstFrame->video_frame.virt_addr[0] = (pPicFramePrm->addr[0]);
    pstFrame->video_frame.virt_addr[1] = (pPicFramePrm->addr[1]);
    pstFrame->video_frame.stride[0] = pPicFramePrm->stride;
    pstFrame->video_frame.stride[1] = pPicFramePrm->stride;

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_VdecCpyframe
 * @brief	   数据拷贝
 * @param[in]  VDEC_PIC_COPY_EN *copyEn 拷贝方式
 * @param[in]  void *pstSrcframe 数据源
 * @param[in]  void *pstDstframe 目的
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecCpyframe(VDEC_PIC_COPY_EN *copyEn, void *pstSrcVideoFrame, void *pstDstVideoFrame)
{
    INT32 s32Ret = SAL_SOK;
    ot_video_frame_info *pstSrcFrame = NULL;
    ot_video_frame_info *pstDstFrame = NULL;
    INT32 i = 0;

    CHECK_PTR_NULL(copyEn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstSrcVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstDstVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pstSrcFrame = (ot_video_frame_info *)pstSrcVideoFrame;
    pstDstFrame = (ot_video_frame_info *)pstDstVideoFrame;

    if (copyEn->copyMeth == VDEC_DATA_ADDR_COPY)
    {
        memcpy(pstDstFrame, pstSrcFrame, sizeof(ot_video_frame_info));
    }
    else if (copyEn->copyMeth == VDEC_DATA_COPY)
    {
        pstDstFrame->video_frame.time_ref = pstSrcFrame->video_frame.time_ref;
        pstDstFrame->video_frame.pts = pstSrcFrame->video_frame.pts;
        pstDstFrame->video_frame.width = pstSrcFrame->video_frame.width;
        pstDstFrame->video_frame.height = pstSrcFrame->video_frame.height;
        pstDstFrame->video_frame.field = pstSrcFrame->video_frame.field;
        pstDstFrame->video_frame.video_format = pstSrcFrame->video_frame.video_format;
        pstDstFrame->video_frame.pixel_format = pstSrcFrame->video_frame.pixel_format;
        if (copyEn->copyth == 1)
        {
            for (i = 0; i < pstSrcFrame->video_frame.height; i++)
            {
                memcpy(HI_ADDR_U642P(pstDstFrame->video_frame.virt_addr[0]) + i * pstDstFrame->video_frame.stride[0], HI_ADDR_U642P(pstSrcFrame->video_frame.virt_addr[0]) + i * pstSrcFrame->video_frame.stride[0], pstSrcFrame->video_frame.width);
                if (i % 2 == 0)
                {
                    memcpy(HI_ADDR_U642P(pstDstFrame->video_frame.virt_addr[1] + i * pstDstFrame->video_frame.stride[1] / 2), HI_ADDR_U642P(pstSrcFrame->video_frame.virt_addr[1] + i * pstSrcFrame->video_frame.stride[1] / 2), pstSrcFrame->video_frame.width);
                }
            }
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
 * @function   vdec_hiv5_VdecGetframe
 * @brief	   获取解码器数据帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecGetframe(void *handle, UINT32 dstChn, void *pstframe)
{
    return SAL_FAIL;
}

/**
 * @function   vdec_hiv5_VdecReleaseframe
 * @brief	   释放解码帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecReleaseframe(void *handle, UINT32 dstChn, void *pstframe)
{
    return SAL_FAIL;
}

/**
 * @function   vdec_hiv5_FreeYuvPoolBlockMem
 * @brief	   释放yuv内存
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_FreeYuvPoolBlockMem(HAL_MEM_PRM *addrPrm)
{
    td_s32 s32Ret = TD_SUCCESS;
    ot_vb_pool pool = OT_VB_INVALID_POOL_ID;
    ot_vb_blk VbBlk = OT_VB_INVALID_HANDLE;
    td_u8 *p64VirAddr = NULL;
    td_s32 u32Size = 0;

    CHECK_PTR_NULL(addrPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pool = addrPrm->poolId;
    p64VirAddr = addrPrm->virAddr;
    VbBlk = addrPrm->vbBlk;
    u32Size = addrPrm->memSize;

    if (NULL != p64VirAddr)
    {
        s32Ret = ss_mpi_sys_munmap(p64VirAddr, u32Size);
        if (TD_SUCCESS != s32Ret)
        {
            VENC_LOGE("err %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }

    if (OT_VB_INVALID_HANDLE != VbBlk)
    {
        s32Ret = ss_mpi_vb_release_blk(VbBlk);
        if (TD_SUCCESS != s32Ret)
        {
            VENC_LOGE("err %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }

    if (OT_VB_INVALID_POOL_ID != pool)
    {
        s32Ret = ss_mpi_vb_destroy_pool(pool);
        if (TD_SUCCESS != s32Ret)
        {
            VENC_LOGE("err %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hiv5_GetYuvPoolBlockMem
 * @brief	   申请yuv平台相关内存
 * @param[in]  UINT32 imgW 宽
 * @param[in]  UINT32 imgH 高
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_GetYuvPoolBlockMem(UINT32 imgW, UINT32 imgH, HAL_MEM_PRM *pOutFrm)
{
/*	char mmzName[10] = "VDEC"; */
    ot_vb_pool pool	= OT_VB_INVALID_POOL_ID;
    UINT64 u64BlkSize = (UINT64)((UINT64)imgW * (UINT64)imgH * 3 / 2);
    ot_vb_blk VbBlk	= OT_VB_INVALID_HANDLE;
    UINT64 u64PhyAddr = 0;
    UINT64 *p64VirAddr = NULL;
    UINT32 u32LumaSize = 0;
    UINT32 u32ChrmSize = 0;
    UINT64 u64Size = 0;
/*	UINT32 u32Ret = SAL_SOK; */
    ot_vb_pool_cfg stVbPoolCfg;

    if (OT_VB_INVALID_POOL_ID == pool)
    {
        SAL_clear(&stVbPoolCfg);
        stVbPoolCfg.blk_size = u64BlkSize;
        stVbPoolCfg.blk_cnt	= 1;
        stVbPoolCfg.remap_mode = OT_VB_REMAP_MODE_NONE;

/*	    sprintf(stVbPoolCfg.acMmzName, "%s", mmzName); */
        pool = ss_mpi_vb_create_pool(&stVbPoolCfg);

        if (OT_VB_INVALID_POOL_ID == pool)
        {
            DUP_LOGE("HI_MPI_VB_CreatePool failed size %llu!\n", u64BlkSize);
            return SAL_FAIL;
        }
    }

    u32LumaSize = (imgW * imgH);
    u32ChrmSize = (imgW * imgH / 4);
    u64Size	= (UINT64)(u32LumaSize + (u32ChrmSize << 1));

    VbBlk = ss_mpi_vb_get_blk(pool, u64Size, NULL);
    if (OT_VB_INVALID_HANDLE == VbBlk)
    {
        DUP_LOGE("HI_MPI_VB_CreatePool failed size %llu!\n", u64Size);
        goto failed;
    }

    u64PhyAddr = ss_mpi_vb_handle_to_phys_addr(VbBlk);
    if (0 == u64PhyAddr)
    {
        DUP_LOGE("HI_MPI_VB_Handle2PhysAddr failed size %llu!\n", u64Size);
        goto failed;
    }

    p64VirAddr = (UINT64 *) ss_mpi_sys_mmap(u64PhyAddr, u64Size);
    if (NULL == p64VirAddr)
    {
        DUP_LOGE("ss_mpi_sys_mmap failed size %llu!\n", u64Size);
        goto failed;
    }

    pOutFrm->phyAddr = u64PhyAddr;
    pOutFrm->virAddr = p64VirAddr;
    pOutFrm->memSize = u64BlkSize;
    pOutFrm->poolId = pool;

    return SAL_SOK;

failed:
    if (pool != OT_VB_INVALID_POOL_ID)
    {
        ss_mpi_vb_destroy_pool(pool);
    }

    if (VbBlk != OT_VB_INVALID_HANDLE)
    {
        ss_mpi_vb_release_blk(VbBlk);
    }

    return SAL_FAIL;
}

/**
 * @function   vdec_hiv5_VdecMmap
 * @brief	   帧映射
 * @param[in]  void *pVideoFrame 数据帧
 * @param[in]  PIC_FRAME_PRM *pPicprm  映射地址
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hiv5_VdecMmap(void *pVideoFrame, PIC_FRAME_PRM *pPicprm)
{
    ot_video_frame_info *pstFrame = NULL;

    CHECK_PTR_NULL(pVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pstFrame = (ot_video_frame_info *)pVideoFrame;
/*	void *virAddr = NULL; */

    if (pstFrame->video_frame.virt_addr[0] == 0 && OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420 == pstFrame->video_frame.pixel_format)
    {
        pstFrame->video_frame.virt_addr[0] = ss_mpi_sys_mmap(pstFrame->video_frame.phys_addr[0], pstFrame->video_frame.width * pstFrame->video_frame.height);
        pstFrame->video_frame.virt_addr[1] = ss_mpi_sys_mmap(pstFrame->video_frame.phys_addr[1], pstFrame->video_frame.width * pstFrame->video_frame.height / 2);
    }

    pPicprm->width = pstFrame->video_frame.width;
    pPicprm->height = pstFrame->video_frame.height;
    pPicprm->stride = pstFrame->video_frame.stride[0];
    pPicprm->videoFormat = vdec_hiv5_TransPixFormat(pstFrame->video_frame.pixel_format);
    pPicprm->poolId = pstFrame->pool_id;
    pPicprm->addr[0] = (void *)pstFrame->video_frame.virt_addr[0];
    pPicprm->addr[1] = (void *)pstFrame->video_frame.virt_addr[1];
    pPicprm->phy[0] = pstFrame->video_frame.phys_addr[0];
    pPicprm->phy[1] = pstFrame->video_frame.phys_addr[1];

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
    memset(&vdecFunc, 0, sizeof(VDEC_HAL_FUNC));

    vdecFunc.VdecCreate = vdec_hiv5_VdecCreate;
    vdecFunc.VdecDeinit = vdec_hiv5_VdecDeinit;
    vdecFunc.VdecClear = vdec_hiv5_VdecClear;
    vdecFunc.VdecSetPrm = vdec_hiv5_VdecSetPrm;
    vdecFunc.VdecStart = vdec_hiv5_VdecStart;
    vdecFunc.VdecStop = vdec_hiv5_VdecStop;
    vdecFunc.VdecDecframe = vdec_hiv5_VdecDecframe;
    vdecFunc.VdecGetframe = vdec_hiv5_VdecGetframe;
    vdecFunc.VdecReleaseframe = vdec_hiv5_VdecReleaseframe;
    vdecFunc.VdecCpyframe = vdec_hiv5_VdecCpyframe;
    vdecFunc.VdecFreeYuvPoolBlockMem = vdec_hiv5_FreeYuvPoolBlockMem;
    vdecFunc.VdecMallocYUVMem = vdec_hiv5_GetYuvPoolBlockMem;
    vdecFunc.VdecMakeframe = vdec_hiv5_VdecMakeframe;
    vdecFunc.VdecMmap = vdec_hiv5_VdecMmap;

    return &vdecFunc;
}

