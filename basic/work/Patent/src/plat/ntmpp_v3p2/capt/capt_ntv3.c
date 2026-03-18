/**
 * @file   capt_ntv3.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台采集模块
 * @author liuxianying
 * @date   2022/4/13
 * @note
 * @note \n History
   1.日    期: 2022/4/13
     作    者: liuxianying
     修改历史: 创建文件
 */

#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include "capbility.h"
#include "capt_ntv3.h"
#include "../../hal/hal_inc_inter/capt_hal_inter.h"

/*****************************************************************************
                               宏定义
*****************************************************************************/

#define VGA_OFFSET_CHECK_PIXEL      (4)     // 计算VGA图像偏移的像素点个数
#define VENDOR_AD_PLAT_CHIP_VI_MAX	    8//4:31x, 32x  8:33x
#define VENDOR_AD_PLAT_VCAP_VI_MAX                      4
#define VENDOR_AD_PLAT_VI_TO_CHIP_ID(x)                 ((x)/VENDOR_AD_PLAT_CHIP_VI_MAX)
#define VENDOR_AD_PLAT_VI_TO_CHIP_VCAP_ID(x)            (((x)%VENDOR_AD_PLAT_CHIP_VI_MAX)/VENDOR_AD_PLAT_VCAP_VI_MAX)
#define VENDOR_AD_PLAT_VI_TO_VCAP_VI_ID(x)              ((x)%VENDOR_AD_PLAT_VCAP_VI_MAX)

/*****************************************************************************
                               结构体定义
*****************************************************************************/
typedef struct _USER_VIDEOCAP {
	HD_VIDEO_FRAME videocap_out_buffer;
	VOID  *videocap_out_buffer_va;
	HD_PATH_ID videocap_path;   /* video capture module */

} USER_VIDEOCAP;

/*****************************************************************************
                               全局变量
*****************************************************************************/
CAPT_MOD_PRM g_stCaptModPrm = {0};
static CAPT_PLAT_OPS g_stCaptPlatOps;
static INT ssenif_fd_user_cnt  = 0;
static INT ssenif_fd  = -1;

/**
 * @function   capt_ntv3CaptIsOffset
 * @brief      计算图像是否有偏移
 * @param[in]  UINT32 uiDev  输入通道
 * @param[out] None
 * @return     static BOOL  SAL_TRUE
 *                          SAL_FALSE
 */
static BOOL capt_ntv3CaptIsOffset(UINT32 uiDev)
{

#if 0   /*暂时未实现*/
    INT32 s32Ret = TD_SUCCESS;
    ot_video_frame_info stFrame;
    ot_vi_pipe viPipe    = 0;
    UINT64 u64PhyAddr = 0;
    void *pvAddr   = NULL;
    UINT8 *pu8Tmp     = NULL;
    UINT32 u32Stride  = 0;
    BOOL bLeft = SAL_TRUE, bRight = SAL_TRUE;
    BOOL bUp = SAL_TRUE, bDown = SAL_TRUE;
    UINT32 i = 0, j = 0;

    if (SAL_SOK != capt_ntv3GetPipeId(uiDev, &viPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FALSE;
    }

    s32Ret = ss_mpi_vi_get_chn_frame(viPipe, 0, &stFrame, 200);
    if (SAL_SOK != s32Ret)
    {
        CAPT_LOGE("capt chn[%u] get frame fail s32Ret = 0x%x \n", uiDev, s32Ret);
        return SAL_FALSE;
    }

    u64PhyAddr = stFrame.video_frame.phys_addr[0];
    u32Stride  = stFrame.video_frame.stride[0];

    pvAddr = ss_mpi_sys_mmap(u64PhyAddr, u32Stride * stFrame.video_frame.height * 3 / 2);
    if (NULL == pvAddr)
    {
        CAPT_LOGE("capt chn[%u] map addr[0x%llx] fail\n", uiDev, u64PhyAddr);
        if (TD_SUCCESS != (s32Ret = ss_mpi_vi_release_chn_frame(viPipe, 0, &stFrame)))
        {
            CAPT_LOGW("release vi pipe[%d] frame fail:0x%X\n", viPipe, s32Ret);
        }
        return SAL_FALSE;
    }

    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += VGA_OFFSET_CHECK_PIXEL * u32Stride;
    for (i = VGA_OFFSET_CHECK_PIXEL; i < stFrame.video_frame.height - VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride;
        /* 最左边的一列不校验 */
        for (j = 1; j < VGA_OFFSET_CHECK_PIXEL; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 左边没有黑边 */
                bLeft = SAL_FALSE;
                goto right;
            }
        }
    }

right:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += (stFrame.video_frame.width - VGA_OFFSET_CHECK_PIXEL);
    pu8Tmp += VGA_OFFSET_CHECK_PIXEL * u32Stride;
    for (i = VGA_OFFSET_CHECK_PIXEL; i < stFrame.video_frame.height - VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride;
        /* 最右边的一列不校验 */
        for (j = 0; j < VGA_OFFSET_CHECK_PIXEL - 1; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 右边没有黑边 */
                bRight = SAL_FALSE;
                goto up;
            }
        }
    }

up:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += u32Stride;
    /* 最上面的一行不校验 */
    for (i = 1; i < VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride;
        for (j = VGA_OFFSET_CHECK_PIXEL; j < stFrame.video_frame.width - VGA_OFFSET_CHECK_PIXEL; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 上面没有黑边 */
                bUp = SAL_FALSE;
                goto down;
            }
        }
    }

down:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += (stFrame.video_frame.height - VGA_OFFSET_CHECK_PIXEL) * u32Stride;
    /* 最下面的一行不校验 */
    for (i = 0; i < VGA_OFFSET_CHECK_PIXEL - 1; i++)
    {
        pu8Tmp += u32Stride;
        for (j = VGA_OFFSET_CHECK_PIXEL; j < stFrame.video_frame.width - VGA_OFFSET_CHECK_PIXEL; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 下面没有黑边 */
                bDown = SAL_FALSE;
                goto end;
            }
        }
    }

end:
    if (TD_SUCCESS != (s32Ret = ss_mpi_sys_munmap(pvAddr, u32Stride * stFrame.video_frame.height * 3 / 2)))
    {
        CAPT_LOGW("ummap viraddr[%p] size[%u] fail:0x%X\n", pvAddr, u32Stride * stFrame.video_frame.height * 3 / 2, s32Ret);
    }
    
    if (TD_SUCCESS != (s32Ret = ss_mpi_vi_release_chn_frame(viPipe, 0, &stFrame)))
    {
        CAPT_LOGW("release vi pipe[%d] frame fail:0x%X\n", viPipe, s32Ret);
    }

    /* 上下都有黑边或左右都有黑边时无需调整 */
    return ((bLeft ^ bRight) || (bUp ^ bDown));
 #endif
    return SAL_FALSE;
}

/**
 * @function   capt_ntv3GetUserPicStatue
 * @brief      采集模块使用用户图片，用于无信号时编码
 * @param[in]  UINT32 uiDev  设备号
 * @param[out] None
 * @return     static UINT32 SAL_SOK
 *                           SAL_FAIL
 */
static UINT32 capt_ntv3GetUserPicStatue(UINT32 uiDev)
{ 
    NT_CAPT_CHN_PRM *pstCaptHalChnPrm = NULL;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("uiChn %d error\n", uiDev);
        return SAL_FAIL;
    }
    
    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (pstCaptHalChnPrm->userPic)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @function   capt_ntv3SetCaptUserPic
 * @brief      采集模块配置用户图片，用于无信号时编码
 * @param[in]  UINT32 uiDev                   设备号
 * @param[in]  CAPT_CHN_USER_PIC *pstUserPic  用户数据
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ntv3SetCaptUserPic(UINT32 uiDev, CAPT_CHN_USER_PIC *pstUserPic)
{

   /*不支持送暂时返回ok*/
    return SAL_SOK;
}

/**
 * @function   capt_ntv3EnableCaptUserPic
 * @brief      使能采集模块使用用户图片，用于无信号时编-
               码
 * @param[in]  UINT32 uiDev    输入设备号
 * @param[in]  UINT32 uiEnble  是否使能
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ntv3EnableCaptUserPic(UINT32 uiDev, UINT32 uiEnble)
{

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }


    return SAL_SOK;
}

/**
 * @function   capt_ntv3SetChnRotate
 * @brief      设置采集通道选择
 * @param[in]  UINT32 uiDev            设备号
 * @param[in]  UINT32 uiChn            通道号
 * @param[in]  CAPT_ROTATION rotation  旋转属性
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ntv3SetChnRotate(UINT32 uiDev, UINT32 uiChn ,CAPT_ROTATION rotation)
{
    return SAL_SOK;
}

/**
 * @function   capt_ntv3GetDevStatus
 * @brief      查询采集通道状态
 * @param[in]  UINT32 uiChn                    设备号
 * @param[in]  CAPT_HAL_CHN_STATUS *pstStatus  通道状态
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ntv3GetDevStatus(UINT32 uiChn, CAPT_HAL_CHN_STATUS *pstStatus)
{
    INT32             s32Ret = SAL_SOK;
  
    NT_CAPT_CHN_PRM *pstCaptHalChnPrm = NULL;

    if (CAPT_CHN_NUM_MAX <= uiChn)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");
        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    if (0 == pstCaptHalChnPrm->uiStarted)
    {
        return SAL_FAIL;
    }

    //解决不同pipe可以支持不同图片后再打开
    pstStatus->captStatus= 0;  /*0:视频正常,默认视频正常*/

    CAPT_LOGD("capt_ntv3GetDevStatus s32Ret=%d \n", s32Ret);

    return SAL_SOK;
}

/**
 * @function   capt_ntv3stopCaptDev
 * @brief      Camera采集通道停止
 * @param[in]  UINT32 uiDev  设备号
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ntv3stopCaptDev(UINT32 uiDev)
{
    CAPT_LOGI("Capt Stop Chn %d !!!\n", uiDev);
    return SAL_SOK;

    /* 规避代码静态检测缺陷 */
#if 0
    NT_CAPT_CHN_PRM *pstCaptHalChnPrm = NULL;
    INT32  s32Ret  = 0;
    CAPT_LOGI("Capt Stop Chn %d !!!\n", uiDev);
    
     return SAL_SOK;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];


    s32Ret = hd_videocap_stop(pstCaptHalChnPrm->uiPath);
    if (s32Ret < 0)
    {
        CAPT_LOGW("hd_videocap_stop failed s32Ret = 0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    pstCaptHalChnPrm->uiStarted = 0;
    pstCaptHalChnPrm->viDevEn = 0;

    //pstCaptHalChnPrm->uiStarted = 0;

    CAPT_LOGI("Capt Stop Chn %d !!!\n", uiDev);

    return SAL_SOK;
#endif
}


/**
 * @function   capt_ntv3StartVcapDev
 * @brief      Camera采集通道开始
 * @param[in]  UINT32 uiDev  设备号
 * @param[out] None
 * @return     INT32 SAL_SOK
 *                   SAL_FAIL
 */
static int capt_ntv3StartVcapDev(UINT32 uiDev)
{

    unsigned int ioc_ver;
    const char *dev_name = "/dev/vcap0";
    INT32 dev_fd = 0;
    INT32  retval;
    struct vcap_ioc_md_ctrl_t ioc_md_ctrl = {0};
         
     
     /* open vcap316 device node */
    dev_fd = open(dev_name, O_RDONLY);
    if (dev_fd ==  -1) 
    {
        CAPT_LOGE("Capt Hal Chn is Not Support0 !!!\n");
        return SAL_FAIL;
    }

       /* get and check ioctl control version */
    retval = ioctl(dev_fd, VCAP_IOC_VER, &ioc_ver);
    if (retval < 0) 
    {
        CAPT_LOGE("Capt Hal Chn is Not Support1 !!!\n");
        return SAL_FAIL;
    }

    if(ioc_ver != VCAP_IOC_VERSION) 
    {
        CAPT_LOGE("Capt Hal Chn is Not Support2 !!!\n");
        return SAL_FAIL;
    }

    ioc_md_ctrl.version = ioc_ver;
    ioc_md_ctrl.chip    = 0;
    ioc_md_ctrl.vcap    = 0;
    ioc_md_ctrl.vi      = 2;
    ioc_md_ctrl.ch      = 0;
    ioc_md_ctrl.enable  = 1;
    retval = ioctl(dev_fd, VCAP_IOC_MD_CTRL, &ioc_md_ctrl);
    if (retval < 0) 
    {
        CAPT_LOGE("Capt Hal Chn is Not Support5 !!!\n");
        return SAL_FAIL;
    }

    close(dev_fd);

    return SAL_SOK;
}

/**
 * @function   capt_ntv3StartCaptDev
 * @brief      Camera采集通道开始
 * @param[in]  UINT32 uiDev  设备号
 * @param[out] None
 * @return     INT32 SAL_SOK
 *                   SAL_FAIL
 */
static INT32 capt_ntv3StartCapt(UINT32 uiDev)
{
    NT_CAPT_CHN_PRM *pstCaptHalChnPrm = NULL;
    HD_VIDEOCAP_PATH_CONFIG cap_config;
	HD_VIDEOCAP_OUT cap_out;
    INT32  s32Ret  = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (1 == pstCaptHalChnPrm->uiStarted)
    {
        CAPT_LOGW("is started\n");
        return SAL_SOK;
    }

	memset(&cap_config, 0x0, sizeof(HD_VIDEOCAP_PATH_CONFIG));
	cap_config.data_pool[0].mode = HD_VIDEOCAP_POOL_ENABLE;
	cap_config.data_pool[0].ddr_id = DDR_ID1;
	cap_config.data_pool[0].counts = HD_VIDEOCAP_SET_COUNT(5, 0);
	cap_config.data_pool[1].mode = HD_VIDEOCAP_POOL_DISABLE;
	cap_config.data_pool[2].mode = HD_VIDEOCAP_POOL_DISABLE;
	cap_config.data_pool[3].mode = HD_VIDEOCAP_POOL_DISABLE;

	s32Ret = hd_videocap_set(pstCaptHalChnPrm->uiPath, HD_VIDEOCAP_PARAM_PATH_CONFIG, &cap_config);
	if (s32Ret != HD_OK) {
		CAPT_LOGE("hd_videocap_set HD_VIDEOCAP_PARAM_PATH_CONFIG\n");
	//	break;
	}

	cap_out.dim.w = 1920;
	cap_out.dim.h = 1080;
	cap_out.pxlfmt = HD_VIDEO_PXLFMT_YUV420_NVX3;//cap_syscaps.pxlfmt;
		cap_out.depth = 0;
	cap_out.frc = HD_VIDEO_FRC_RATIO(1, 1);
	s32Ret = hd_videocap_set(pstCaptHalChnPrm->uiPath, HD_VIDEOCAP_PARAM_OUT, &cap_out);
	if (s32Ret != HD_OK) {
		CAPT_LOGE("hd_videocap_set HD_VIDEOCAP_PARAM_OUT\n");
	//	break;
	}
    s32Ret = hd_videocap_start(pstCaptHalChnPrm->uiPath);
    if (s32Ret < 0)
    {
        CAPT_LOGW("hd_videocap_start failed!s32Ret = %#x, viPath=%d\n", s32Ret, pstCaptHalChnPrm->uiPath);
        return SAL_FAIL;
    }

    s32Ret = capt_ntv3StartVcapDev(uiDev);
    if (s32Ret < 0)
    {
        CAPT_LOGW("capt_ntv3StartVcapDev failed!s32Ret = %#x, uiDev=%d\n", s32Ret, uiDev);
        return SAL_FAIL;
    }

    pstCaptHalChnPrm->uiStarted = 1;

    CAPT_LOGI("Capt Start Chn %d:%x !!!\n", uiDev, pstCaptHalChnPrm->uiPath);

    return SAL_SOK;
}


/**
 * @function   capt_ntv3ReSetMipi
 * @brief      采集模块初重新始化Mipi接口
 * @param[in]  UINT32 uiChn   通道
 * @param[in]  UINT32 width   宽
 * @param[in]  UINT32 height  高
 * @param[in]  UINT32 fps     帧率
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ntv3ReSetMipi(UINT32 uiChn,UINT32 width, UINT32 height, UINT32 fps)
{
    return SAL_SOK;
}


/* @function   capt_ssenif_init
 * @brief      采集模Mipi接口开启
 * @param[in]  UINT32 ssenif_dev_name   名称
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static HD_RESULT capt_ssenif_init(CHAR *ssenif_dev_name)
{
	HD_RESULT ret = HD_OK;

	/* open ssenif device node */
	if (ssenif_fd_user_cnt == 0) {
		ssenif_fd = open(ssenif_dev_name, O_RDONLY);
		if (ssenif_fd < 0) {
			ssenif_fd = -1;
			printf("open %s failed(%s)\n", ssenif_dev_name, strerror(errno));
			ret = HD_ERR_SYS;
			goto exit;
		}
	}
	ssenif_fd_user_cnt++;

exit:
	return ret;
}

/* @function   capt_ntv3mipiInit
 * @brief      采集模块初重新始化Mipi接口
 * @param[in]  UINT32 uiChn   通道
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ntv3mipiInit(UINT32 uiChn)
{
    unsigned int csi_id = 2;
    INT32 s32Ret = SAL_FAIL;
    struct csi_drv_sensor_type  csi_sensor_type;
    struct csi_drv_dlane_number csi_dlane_num;
    struct csi_field_en csi_field;
    struct csi_hd_gating csi_hd_gate;
    struct csi_pxclk_sel csi_pxclk;
    struct csi_drv_vc_id csi_vc;
    struct csi_drv_vc_valid_height csi_valid_height;
    int  k;
     
    capt_ssenif_init("/dev/nvt_slvsec0");

            
    /* CSI engine stop  */
    s32Ret = ioctl(ssenif_fd, SSENIF_IOC_CSI_STOP, &csi_id);
    if (s32Ret < 0) {
        printf("CSI#%u SSENIF_IOC_CSI_STOP ioctl...error\n", csi_id);
        s32Ret = HD_ERR_NG;
         close(ssenif_fd);
         return s32Ret;
    }

    /* CSI sensor type */
    csi_sensor_type.eng_id      = csi_id;
    csi_sensor_type.sensor_type = 6;     ///< others for MIPI AD
    s32Ret = ioctl(ssenif_fd, SSENIF_IOC_SET_CSI_SENSORTYPE, &csi_sensor_type);
    if (s32Ret < 0) {
        printf("CSI#%u SSENIF_IOC_SET_CSI_SENSORTYPE ioctl...error\n", csi_id);
        s32Ret = HD_ERR_NG;
        close(ssenif_fd);
        return s32Ret;
    }

    /* CSI data lane number */
    csi_dlane_num.eng_id       = csi_id;
    csi_dlane_num.dlane_number = 4;   
    s32Ret = ioctl(ssenif_fd, SSENIF_IOC_SET_CSI_DLANE_NUMBER, &csi_dlane_num);
    if (s32Ret < 0) {
        printf("CSI#%u SSENIF_IOC_SET_CSI_DLANE_NUMBER ioctl...error\n", csi_id);
        s32Ret = HD_ERR_NG;
        close(ssenif_fd);
        return s32Ret;
    }

    /* CSI field status enable for interlace signal */
    csi_field.eng_id    = csi_id;
    csi_field.field_en  = 1;
    csi_field.field_bit = 0;    ///< short packet WC field as frame number, odd field(1), even field(2)....
    s32Ret = ioctl(ssenif_fd, SSENIF_IOC_SET_CSI_FIELD_EN, &csi_field);
    if (s32Ret < 0) {
        printf("CSI#%u SSENIF_IOC_SET_CSI_FIELD_EN ioctl...error\n", csi_id);
         close(ssenif_fd);
        return s32Ret;
    }

    /* CSI HD gating disable */
    csi_hd_gate.eng_id       = csi_id;
    csi_hd_gate.hd_gating_en = 0;           ///< HW issue HD pulses each time when the valid HD detected (Valid header or LineStart Short Packet)
    s32Ret = ioctl(ssenif_fd, SSENIF_IOC_SET_CSI_HD_GATING, &csi_hd_gate);
    if (s32Ret < 0) {
        printf("CSI#%u SSENIF_IOC_SET_CSI_HD_GATING ioctl...error\n", csi_id);
        s32Ret = HD_ERR_NG;
        close(ssenif_fd);
        return s32Ret;
    }

    /* CSI output pixel clock selection */
    csi_pxclk.eng_id    = csi_id;
    csi_pxclk.pxclk_sel = 1;    ///< 0:300MHz 1:400MHz 2:350MHz,  clock freq must be large than (lane_speed_Mbps x lane_num)/8(bit)/2(pixel) to prevent CSI FIFO overflow
    csi_pxclk.pxclk_en  = 1;
    s32Ret = ioctl(ssenif_fd, SSENIF_IOC_SET_CSI_PXCLK_SEL, &csi_pxclk);
    if (s32Ret < 0) {
        printf("CSI#%u SSENIF_IOC_SET_CSI_PXCLK_SEL ioctl...error\n", csi_id);
        s32Ret = HD_ERR_NG;
         close(ssenif_fd);
        return s32Ret;
    }

    /* CSI virtual channel number and valid height for frame end detect */
    for (k=0; k<4; k++) 
    {
        csi_vc.eng_id       = csi_id;
        csi_vc.vch_index    = k;
        csi_vc.id           = k;
        s32Ret = ioctl(ssenif_fd, SSENIF_IOC_SET_CSI_VC_ID, &csi_vc);
        if (s32Ret < 0) 
        {
            printf("CSI#%u-VC#%d SSENIF_IOC_SET_CSI_VC_ID ioctl...error\n", csi_id, k);
            s32Ret = HD_ERR_NG;
             close(ssenif_fd);
             return s32Ret;
        }

        csi_valid_height.eng_id       = csi_id;
        csi_valid_height.vch_index    = k;
        csi_valid_height.valid_height = 64;
        s32Ret = ioctl(ssenif_fd, SSENIF_IOC_SET_CSI_VC_VALID_HEIGHT, &csi_valid_height);
        if (s32Ret < 0) 
        {
            printf("CSI#%u-VC#%d SSENIF_IOC_SET_CSI_VC_VALID_HEIGHT ioctl...error\n", csi_id, k);
            s32Ret = HD_ERR_NG;
            close(ssenif_fd);
        return s32Ret;
        }
    }

    /* CSI engine start */
    s32Ret = ioctl(ssenif_fd, SSENIF_IOC_CSI_START, &csi_id);
    if (s32Ret < 0) {
        printf("CSI#%u SSENIF_IOC_CSI_START ioctl...error\n", csi_id);
        s32Ret = HD_ERR_NG;
         close(ssenif_fd);
        return s32Ret;
    }
    CAPT_LOGW("CSI#%u SSENIF_IOC_CSI_START ioctl..\n", csi_id);

     close(ssenif_fd);
     return s32Ret;
}

/**
 * @function   capt_ntv3InitViDev
 * @brief      Camera采集通道创建
 * @param[in]  UINT32 uiDev               采集设备
 * @param[in]  CAPT_CHN_ATTR *pstChnAttr  采集通道参数
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ntv3InitViDev(UINT32 uiDev, CAPT_CHN_ATTR *pstChnAttr)
{

    NT_CAPT_CHN_PRM *pstCaptHalChnPrm = NULL;
    INT32 s32Ret = SAL_FAIL;


    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if(pstCaptHalChnPrm->viDevEn == 1)
    {
        CAPT_LOGW("Capt Hal Chn %d is started !!!\n",uiDev);
        return SAL_SOK;
    }
        
    pstCaptHalChnPrm->uiChn   = uiDev;
    pstCaptHalChnPrm->uiIspId = uiDev;
    pstCaptHalChnPrm->uiSnsId = uiDev;
    pstCaptHalChnPrm->uiIspInited = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    /* 配置 vi dev */
   /* open videocap module */
    s32Ret =  hd_videocap_open(HD_VIDEOCAP_IN(0, 0), HD_VIDEOCAP_OUT(0, 0), &pstCaptHalChnPrm->uiPath);
    if (s32Ret != HD_OK) 
    {
        CAPT_LOGE("hd_videocap_open fail\n");
        return SAL_FAIL;
    }

    pstCaptHalChnPrm->viDevEn = 1;
    CAPT_LOGI("hd_videocap_open fail %d \n", uiDev);

    return SAL_SOK;
}

/**
 * @function   capt_ntv3DeInitViDev
 * @brief      Camera采集通道销毁 
 * @param[in]  UINT32 uiChn  设备通道号
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ntv3DeInitViDev(UINT32 uiChn)
{
    NT_CAPT_CHN_PRM *pstCaptHalChnPrm = NULL;
    INT32 s32Ret = SAL_FAIL;
    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    s32Ret = hd_videocap_close(pstCaptHalChnPrm->uiPath);
    if (s32Ret != HD_OK) 
    {
        CAPT_LOGE("hd_videocap_close fail\n");
    }


    s32Ret = hd_videocap_uninit();
    if (s32Ret != HD_OK)
    {
        CAPT_LOGE("hd_videocap_uninit fail\n");
    }
    
    pstCaptHalChnPrm->uiStarted = 0;
    memset(pstCaptHalChnPrm, 0, sizeof(NT_CAPT_CHN_PRM));

    return SAL_SOK;
}
 
static INT32 capt_ntv3Init(void)
{
    INT32 s32Ret = SAL_FAIL;
    INT32 i = 0, j = 0;
    HD_VIDEOCAP_VI_ID stVcapId = {0};
    HD_VIDEOCAP_HOST_ID stHostId;
    HD_VIDEOCAP_HOST stHost;
    HD_VIDEOCAP_VI stVcap;
    HD_VIDEOCAP_VI_CH_NORM3 stVideoNorm;
    HD_VIDEOCAP_VI_CH_PARAM stViParam;

    capt_ntv3mipiInit(0);

   /* for(i = 0;i < NT_CAPT_DEV_NUM;  i++)
    {
         for (j = 0; j < NT_AD_PLAT_CHIP_VI_MAX; j++)
        {
            stVcapId.chip = i;
            stVcapId.vcap = 0;VENDOR_AD_PLAT_VI_TO_CHIP_ID
            stVcapId.vi = 2;//(j % NT_AD_PLAT_VCAP_VI_MAX);
            s32Ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_DEREGISTER_VI, &stVcapId);
            if (s32Ret != HD_OK)
            {
                CAPT_LOGE("set VENDOR_AD_PARAM_DEREGISTER_VCAP_VI failedi = %d, j = %d, s32Ret = %d\n", i, j, s32Ret);
                return s32Ret;
            }
        }
    }*/

    for (i=0; i< HD_VIDEOCAP_VI_MAX; i++) 
    {
		stVcapId.chip    = VENDOR_AD_PLAT_VI_TO_CHIP_ID(i);
		stVcapId.vcap    = VENDOR_AD_PLAT_VI_TO_CHIP_VCAP_ID(i);
		stVcapId.vi      = VENDOR_AD_PLAT_VI_TO_VCAP_VI_ID(i);
		s32Ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_DEREGISTER_VI, &stVcapId);
		if (s32Ret != HD_OK) 
        {
			 CAPT_LOGE("set VENDOR_AD_PARAM_DEREGISTER_VCAP_VI failedi = %d, j = %d, s32Ret = %d\n", i, j, s32Ret);
                return s32Ret;
		}
	}

     /*销毁vi host*/
    stHostId.host = 0;
    s32Ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_UNINIT_HOST, &stHostId);
    if (s32Ret < 0)
    {
        CAPT_LOGE("set VENDOR_AD_PARAM_UNINIT_VCAP_HOST failed, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }
    /*初始化vi模块*/
    /* vcap host init, to specify vcap system vi usage and prepare requirement memory */
    memset(&stHost, 0, sizeof(HD_VIDEOCAP_HOST));
    stHost.host = 0;
    stHost.md.enable = 1;
    stHost.md.mb_x_num_max = 128;
    stHost.md.mb_y_num_max = 64;
    stHost.md.buf_src = 0;      /*上层分配内存*/


    for (i = 0; i < NT_CAPT_DEV_NUM; i++)
    {
        for (j = 0; j < NT_AD_PLAT_CHIP_VI_MAX; j++)
        {
            if (stHost.nr_of_vi >= HD_VIDEOCAP_VI_MAX)
            {
                CAPT_LOGE("stHost.nr_of_vi = %d\n", stHost.nr_of_vi);
                 return s32Ret;
            }

            stHost.vi[stHost.nr_of_vi].chip = 0;
            stHost.vi[stHost.nr_of_vi].vcap = 0;
            stHost.vi[stHost.nr_of_vi].vi = 2;//j % NT_AD_PLAT_VCAP_VI_MAX;
            stHost.vi[stHost.nr_of_vi].mode = HD_VIDEOCAP_VI_MODE_4CH;
            stHost.nr_of_vi++;
        }
    }

    s32Ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_INIT_HOST, &stHost);
    if (s32Ret != HD_OK)
    {
        CAPT_LOGE("set VENDOR_AD_PARAM_INIT_VCAP_HOST failed, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }


    /* VCAP VI Register */
        /*注册vi dev*/
    for (i = 0; i < NT_CAPT_DEV_NUM; i++)
    {
        for (j = 0; j < NT_AD_PLAT_CHIP_VI_MAX; j++)
        {
            stVcap.chip = i;
            stVcap.vcap = 0;
            stVcap.vi = 2;
            stVcap.global.src = HD_VIDEOCAP_VI_SRC_CSI2;
            
            stVcap.global.format = HD_VIDEOCAP_VI_FMT_CSI_16BIT;  /*MIPI-CSI输入*/

            stVcap.global.tdm = HD_VIDEOCAP_VI_TDM_1CH_BYPASS;

            /*是否通过chnid区分复用的不同通道*/
            stVcap.global.id_extract = HD_VIDEOCAP_VI_CHID_EAV_SAV;

            stVcap.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_DUAL;     /* 双边延采样; */

           s32Ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_REGISTER_VI, &stVcap);
           if (s32Ret != HD_OK) 
           {
              VDEC_LOGE("VENDOR_AD_PARAM_REGISTER_VCAP_VI failed s32Ret = 0x%x\n", s32Ret);
               return s32Ret;
           }

            /*
               此处注释仅做参考，目前此处修改会被内核过滤掉，
               如确实需要修改，建议联系内核同事修改GPIO复用关系。
               [X_CAP#0]
               clk_pin => 0: X_CAP0_CLK (GPIO_1#1)
               [X_CAP#1]
               clk_pin => 0: X_CAP1_CLK (GPIO_1#10)
               [X_CAP#2]
               clk_pin => 0: X_CAP2_CLK (GPIO_3#4)  1: GPIO_2#26
               [X_CAP#3]
               clk_pin => 0: X_CAP3_CLK (GPIO3#13)  1: GPIO_3#4
             */
             #if 0
            stVcap.vport[0].clk_dly = 0;
            stVcap.vport[0].clk_pdly = 0;
            stVcap.vport[0].clk_pin = 0;
            stVcap.vport[0].data_swap = 0;       /* /< VI-P0(rising) */

            if (stVcap.global.latch_edge == HD_VIDEOCAP_VI_LATCH_EDGE_DUAL)
            {
                stVcap.vport[1].data_swap = stVcap.vport[0].data_swap;          /* /< VI-P1(falling) */
            }

            /*注册VI设备*/
            s32Ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_REGISTER_VI, &stVcap);
            if (s32Ret != HD_OK)
            {
                VDEC_LOGE("VENDOR_AD_PARAM_REGISTER_VCAP_VI failed s32Ret = 0x%x\n", s32Ret);
                 return s32Ret;
            }
            #endif
        }

    }

    /*设定通道对应关系*/
    for (i = 0; i < NT_CAPT_DEV_NUM; i++)
    {
        for (j = 0; j < NT_AD_PLAT_CHIP_VI_MAX; j++)
        {
            stViParam.chip = i;
            stViParam.vcap = 0;
            stViParam.vi = 2;//j % NT_AD_PLAT_VCAP_VI_MAX;
            stViParam.ch = 0;
            stViParam.value =0;// (i * NT_AD_PLAT_CHIP_VI_MAX + j) * NT_VI_DEV_MAX_CHN_NUM; ///< Device vin video channel index
            /*设置通道的工作模式*/
            stViParam.pid = HD_VIDEOCAP_VI_CH_PARAM_VCH_ID;
            s32Ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_VI_CH_PARAM, &stViParam);
            if (s32Ret != HD_OK)
            {
                VDEC_LOGE("VENDOR_AD_PARAM_VCAP_VI_CH_SET_PARAM faile, i = %d, s32Ret = %d\n", i, s32Ret);
                return s32Ret;
            }

            /* CSI_VC, don't care if ch not CSI format */
            stViParam.chip = i;
            stViParam.vcap = 0;
            stViParam.vi = 2;//j % NT_AD_PLAT_VCAP_VI_MAX;
            stViParam.ch = 0;
            stViParam.value = 0;    ///< Device vin csi virtual channel id,  VC#  :0 ~ 3
            /*设置通道的工作模式*/
            stViParam.pid = HD_VIDEOCAP_VI_CH_PARAM_CSI_VC;
            s32Ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_VI_CH_PARAM, &stViParam);
            if (s32Ret != HD_OK)
            {
                VDEC_LOGE("HD_VIDEOCAP_DRV_PARAM_VI_CH_PARAM faile, i = %d, s32Ret = %d\n", i, s32Ret);
                return s32Ret;
            }


            /* get video norm */
            stVideoNorm.chip = i;
            stVideoNorm.vcap = 0;
            stVideoNorm.vi = 2;//j % NT_AD_PLAT_VCAP_VI_MAX;

            stVideoNorm.ch = 0;

            stVideoNorm.org_width = 1920;
            stVideoNorm.org_height = 1080;
            stVideoNorm.fps = 60;
            stVideoNorm.prog = 1;
            stVideoNorm.cap_width = 1920;
            stVideoNorm.cap_height = 1080;
            stVideoNorm.format = HD_VIDEOCAP_VI_CH_FMT_CSI_16BIT;
            stVideoNorm.data_rate = HD_VIDEOCAP_VI_CH_DATA_RATE_SINGLE;
     
            stVideoNorm.data_latch = HD_VIDEOCAP_VI_CH_DATA_LATCH_SINGLE;   
 


            stVideoNorm.horiz_dup = HD_VIDEOCAP_VI_CH_HORIZ_DUP_OFF;
            stVideoNorm.data_src   = HD_VIDEOCAP_VI_CH_DATA_SRC_VI;

            /*设置vi通道的默认参数*/
            s32Ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_VI_CH_NORM3, &stVideoNorm);
            if (s32Ret != HD_OK)
            {
                VDEC_LOGE("VENDOR_AD_PARAM_VCAP_VI_CH_SET_NORM chip = %d dev = %d s32Ret = 0x%x\n", i, j,  s32Ret);
                return s32Ret;
            }
             VDEC_LOGI("VENDOR_AD_PARAM_VCAP_VI_CH_SET_PARAM, i = %d, s32Ret = %d\n", i, s32Ret);
            
        }
    }


    s32Ret = hd_videocap_init();
    if (s32Ret != SAL_SOK) 
    {
        CAPT_LOGE("hd_videocap_init fail\n");
        return s32Ret;
    }
    
    return s32Ret;
}


/**
 * @function   capt_ntv3Interface
 * @brief      采集模块初始化Mipi接口
 * @param[in]  UINT32 uiChn  通道号
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ntv3Interface(UINT32 uiChn)
{
    INT32  s32Ret  = 0;

    if(0)
    {
         s32Ret =  capt_ntv3Init();
        if(SAL_SOK != s32Ret)
        {
            VDEC_LOGE("hd_videodec_init fail!!!\n");
        }
    }
    return SAL_SOK;
}

/**
 * @function   capt_halRegister
 * @brief      注册hisi disp显示函数
 * @param[in]  void  
 * @param[out] None
 * @return     CAPT_PLAT_OPS * 返回capt函数指针
 */
CAPT_PLAT_OPS *capt_halRegister(void)
{
     INT32 s32Ret = SAL_SOK;
     
    memset(&g_stCaptPlatOps, 0, sizeof(CAPT_PLAT_OPS));
    VDEC_LOGW("hd_videodec_init ok!!!\n");

    g_stCaptPlatOps.initCaptInterface =   capt_ntv3Interface;
    g_stCaptPlatOps.reSetCaptInterface =  capt_ntv3ReSetMipi;
    
    g_stCaptPlatOps.initCaptDev   =       capt_ntv3InitViDev;
    g_stCaptPlatOps.deInitCaptDev =       capt_ntv3DeInitViDev;
    g_stCaptPlatOps.startCaptDev  =       capt_ntv3StartCapt;
    g_stCaptPlatOps.stopCaptDev   =       capt_ntv3stopCaptDev;
    g_stCaptPlatOps.getCaptDevState =     capt_ntv3GetDevStatus;
    g_stCaptPlatOps.setCaptChnRotate  =   capt_ntv3SetChnRotate;

    g_stCaptPlatOps.enableCaptUserPic =   capt_ntv3EnableCaptUserPic;
    g_stCaptPlatOps.setCaptUserPic    =   capt_ntv3SetCaptUserPic;
    g_stCaptPlatOps.getCaptUserPicStatus = capt_ntv3GetUserPicStatue;
    
    g_stCaptPlatOps.checkCaptIsOffset =    capt_ntv3CaptIsOffset;


    s32Ret =  capt_ntv3Init();
    if(SAL_SOK != s32Ret)
    {
        VDEC_LOGE("hd_videodec_init fail!!!\n");
    }
    
    return &g_stCaptPlatOps;
}


