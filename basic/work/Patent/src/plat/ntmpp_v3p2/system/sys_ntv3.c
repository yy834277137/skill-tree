/**
 * @file   sys_ntv3.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  sys NT平台抽象层接口
 * @author liuxianying
 * @date   2021/11/26
 * @note
 * @note \n History
   1.日    期: 2021/11/26
     作    者: liuxianying
     修改历史: 创建文件
   2.日    期: 2022/02/09
     作    者: yeyanzhong
     修改历史: 与vpe绑定时，vpe采用一进一出的方式
 */
#include "platform_hal.h"
#include "platform_sdk.h"
#include "sys_hal_inter.h"
#include "fmtConvert_ntv3.h"
#include "vpe_nt9833x.h"
#include <errno.h>

#define DDM_PCP "/dev/DDM/pcp"

static SYS_PLAT_OPS g_stSysPlatOps;
extern HD_RESULT bd_bind(HD_DAL src_deviceid, HD_IO out_id, HD_DAL dst_deviceid, HD_IO in_id);
extern HD_RESULT bd_unbind(HD_DAL self_deviceid, HD_IO out_id);


/*****************************************************************************
                            宏定义
*****************************************************************************/
static HD_COMMON_MEM_POOL_INFO pool_info[] = {
    /* ddr_id, type, blk_size, cnt, addr, shared_pool */
    { 0, HD_COMMON_MEM_COMMON_POOL              , 70720   ,1 , 0, {0} },
    { 0, HD_COMMON_MEM_ENC_OUT_POOL             , 7784    ,1 , 0, {0} },
    { 0, HD_COMMON_MEM_ENC_REF_POOL             , 3368    ,1 , 0, {0} },
    { 0, HD_COMMON_MEM_ENC_SCL_OUT_POOL         , 9116    ,1 , 0, {0} },
    { 0, HD_COMMON_MEM_AU_ENC_AU_GRAB_OUT_POOL , 60      ,1 , 0, {0} },
    { 0, HD_COMMON_MEM_AU_DEC_AU_RENDER_IN_POOL , 60      ,1 , 0, {0} },

    { 0, HD_COMMON_MEM_CNN_POOL                 , 1248576  ,1 , 0, {0} },
    { 0, HD_COMMON_MEM_USER_BLK                 , 252168 ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_COMMON_POOL              , 47748   ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DISP_DEC_IN_POOL         , 119920  ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DISP_DEC_OUT_POOL        , 449896  ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DISP_DEC_OUT_RATIO_POOL  , 101184  ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DEC_TILE_POOL            , 14840   ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_ENC_CAP_OUT_POOL         , 27552   ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_ENC_REF_POOL             , 3368    ,4 , 0, {0} },
    { 1, HD_COMMON_MEM_ENC_REF_POOL             , 2328    ,4 , 0, {0} },
    { 1, HD_COMMON_MEM_ENC_SCL_OUT_POOL         , 36480   ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DISP0_IN_POOL            , 48600   ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DISP0_FB_POOL            , 8160    ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DISP1_IN_POOL            , 12240   ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DISP1_FB_POOL            , 8160    ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DISP2_IN_POOL            , 3240    ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DISP2_FB_POOL            , 812     ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_GLOBAL_MD_POOL           , 296     ,4 , 0, {0} },
    { 1, HD_COMMON_MEM_TMNR_MOTION_POOL         , 1024    ,4 , 0, {0} },
    { 1, HD_COMMON_MEM_OSG_POOL                 , 4096    ,1 , 0, {0} },
	{ 1, HD_COMMON_MEM_CNN_POOL                 , 1048576  ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_USER_BLK                 , 2097152  ,1 , 0, {0} },

};


static HD_COMMON_MEM_POOL_INFO pool_infoAnalyzerBox[] = {
    /* ddr_id, type, blk_size, cnt, addr, shared_pool */
    { 1, HD_COMMON_MEM_COMMON_POOL              , 3188    ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_DISP_DEC_OUT_POOL        , 44980   ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_ENC_CAP_OUT_POOL         , 31868   ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_ENC_REF_POOL             , 6072    ,12, 0, {0} },
    { 1, HD_COMMON_MEM_ENC_REF_POOL             , 4196    ,4 , 0, {0} },
    { 1, HD_COMMON_MEM_ENC_SCL_OUT_POOL         , 36480   ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_ENC_OUT_POOL             , 7784    ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_GLOBAL_MD_POOL           , 296     ,4 , 0, {0} },
    { 1, HD_COMMON_MEM_TMNR_MOTION_POOL         , 1024    ,4 , 0, {0} },
    { 1, HD_COMMON_MEM_OSG_POOL                 , 4096    ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_CNN_POOL                 , 286720  ,1 , 0, {0} },
    { 1, HD_COMMON_MEM_USER_BLK                 , 479308  ,1 , 0, {0} },

};



/**
 * @function   sys_ntv3_deInit
 * @brief      释放初始化过程
 * @param[in]  void  
 * @param[out] None
 * @return     static INT32
 */
static INT32 sys_ntv3_deInit(void)
{
    INT32 sRet = SAL_SOK;
    
    sRet = hd_common_mem_uninit();
    if (SAL_SOK  != sRet)
    {
       SYS_LOGE("hd_common_uninit fail\n");
    }

    sRet = hd_common_uninit();
    if (SAL_SOK  != sRet)
    {
       SYS_LOGE("hd_common_uninit fail\n");
    }

    return SAL_SOK;
}


/**
 * @function   sys_ntv3_MemAddr
 * @brief      addr设置
 * @param[in]  void  
 * @param[out] None
 * @return     static int
 */
static int sys_ntv3_memAddr(HD_COMMON_MEM_POOL_INFO *poolInfo, UINT32  poolInfoSize)
{
    int i, sys_fd, ddr_id, ddr_nr;
    UINTPTR ddr_paddr[DDR_ID_MAX];
    unsigned long ddr_total[DDR_ID_MAX], ddr_usage[DDR_ID_MAX];
    unsigned int pool_size = 0;
    struct nvtmem_hdal_base sys_hdal;
    HD_COMMON_MEM_POOL_INFO *pPoolInfo = poolInfo;
    UINT32 poolInfoLen = poolInfoSize /sizeof(HD_COMMON_MEM_POOL_INFO);

    sys_fd = open("/dev/nvtmem0", O_RDWR);
    if (sys_fd < 0) 
    {
        SYS_LOGE("Error: cannot open /dev/nvtmem0 device.\n");
        return SAL_FAIL;
    }
    if (ioctl(sys_fd, NVTMEM_GET_DTS_HDAL_BASE, &sys_hdal) < 0) 
    {
        SYS_LOGE("PCIE_SYS_IOC_HDALBASE! \n");
        close(sys_fd);
        return SAL_FAIL;
    }
    close(sys_fd);

    memset(ddr_paddr, 0, sizeof(ddr_paddr));
    memset(ddr_total, 0, sizeof(ddr_total));
    memset(ddr_usage, 0, sizeof(ddr_usage));
    for (ddr_nr = 0; ddr_nr < MAX_DDR_NR; ddr_nr++) 
    {
        ddr_id = sys_hdal.ddr_id[ddr_nr];
        if (sys_hdal.size[ddr_nr] != 0) 
        {
            if (ddr_total[ddr_id] != 0)  
            {  /* check duplicate ddr_id */
                SYS_LOGE("DDR%d: hdal_mem(%lx), size(0x%lx)\n", ddr_id, ddr_paddr[ddr_id], ddr_total[ddr_id]);
                SYS_LOGE("other DDR%ld: hdal_mem(%#lx), size(0x%lx)\n", sys_hdal.ddr_id[ddr_nr], sys_hdal.base[ddr_nr], sys_hdal.size[ddr_nr]);
                SYS_LOGE("please check the nvt-mem-tbl.dtsi file.\n");
                return SAL_FAIL;
            }
            ddr_paddr[ddr_id] = sys_hdal.base[ddr_nr];
            ddr_total[ddr_id] = sys_hdal.size[ddr_nr];
        }
    }

    for (ddr_id = 0; ddr_id < DDR_ID_MAX; ddr_id++) 
    {
        if (ddr_total[ddr_id] == 0) 
        {
            continue;
        }
        SYS_LOGI("DDR%d: hdal_mem %lx, 0x%lx\n", ddr_id, ddr_paddr[ddr_id], ddr_total[ddr_id]);

        for (i = 0; i < poolInfoLen ; i++) 
        {
            if (pPoolInfo[i].ddr_id != ddr_id) 
            {
                continue;
            }

            pPoolInfo[i].start_addr = ddr_paddr[ddr_id];
            pPoolInfo[i].blk_size = pPoolInfo[i].blk_size * 1024;
            pPoolInfo[i].blk_size = ALIGN_CEIL(pPoolInfo[i].blk_size, 4096);  //for 4K page handling
            pool_size =  pPoolInfo[i].blk_size * pPoolInfo[i].blk_cnt;
            ddr_paddr[ddr_id] += pool_size;
            ddr_usage[ddr_id] += pool_size;
        }

        SYS_LOGD("Hdal DDR%d Pool usage %ldKB (Total %ldKB Free %ldKB)\n", ddr_id,
               ddr_usage[ddr_id] / 1024, ddr_total[ddr_id] / 1024, (ddr_total[ddr_id] - ddr_usage[ddr_id]) / 1024);

        if (ddr_usage[ddr_id] > ddr_total[ddr_id]) 
        {
            SYS_LOGE("DDR%d Pool total size overflow %ldKB (hdal size %ldKB)\n", ddr_id,
                   ddr_usage[ddr_id] / 1024, ddr_total[ddr_id] / 1024);
            return SAL_FAIL;
        }
    }
    
    return SAL_SOK;
}

/**
 * @function   sys_ntv3_memInit
 * @brief      初始化媒体内存
 * @param[in]  void  
 * @param[out] None
 * @return     static int
 */
static INT32 sys_ntv3_memInit(void)
{
    HD_COMMON_MEM_INIT_CONFIG mem_cfg;
    INT32 ret = SAL_SOK;
    UINT32  u32BoardType = HARDWARE_GetBoardType();
    UINT32  poolInfoSize = 0;
    HD_COMMON_MEM_POOL_INFO *pPoolInfo = NULL;
    INT32 fd = -1;
    cpld_info cpld;

    if (u32BoardType == DB_RS20032_V1_0)
    {
        poolInfoSize = sizeof(pool_infoAnalyzerBox);
        pPoolInfo = pool_infoAnalyzerBox;
    }
    else
    {
        poolInfoSize = sizeof(pool_info);
        pPoolInfo = pool_info;

        /* 根据内存修改内存分配 */
        fd = open(DDM_PCP, O_RDWR);
        if (fd < 0)
        {
            SYS_LOGE("open %s fail:%s\n", DDM_PCP, strerror(errno));
            return SAL_FAIL;
        }

        ret = ioctl(fd, DDM_IOC_GET_HARDWRE_INFO, &cpld);
        if (ret < 0)
        {
            SYS_LOGE("DDM_IOC_GET_HARDWRE_INFO failed: %s\n", strerror(errno));
            close(fd);
            return SAL_FAIL;
        }
        close(fd);

        if (cpld.sys.bits.ddr_bitw == DDR_4G_4x8Gb || cpld.sys.bits.ddr_bitw == DDR_4G_2x16Gb) /* 板子内存为4G，需要修改内存分配 */
        {
            pool_info[0].blk_size = 30720;
            pool_info[6].blk_size = 435200;
            pool_info[7].blk_size = 25600;
            pool_info[9].blk_size = 51200;
            pool_info[10].blk_size = 184320;
            pool_info[11].blk_size = 40960;
            pool_info[26].blk_size = 1024;
            pool_info[27].blk_size = 1468006;
            SYS_LOGI("DDR is 4G!\n");
        }
        else if (cpld.sys.bits.ddr_bitw == DDR_8G_8x8Gb || cpld.sys.bits.ddr_bitw == DDR_8G_4x16Gb) /* 内存8G不修改，其他大小暂不支持 */
        {
            SYS_LOGI("DDR is 8G!\n");
        }
        else
        {
            SYS_LOGE("cpld.sys.bits.ddr_bitw not support!\n");
            return SAL_FAIL;
        }
    }

    memset(&mem_cfg, 0, sizeof(HD_COMMON_MEM_INIT_CONFIG));
    ret = sys_ntv3_memAddr(pPoolInfo, poolInfoSize);
    if (ret < 0)
    {
        SYS_LOGE("sys_ntv3_MemAddrinit fail ret:%#x\n", ret);
        return SAL_FAIL;
    }

    ret = hd_common_init(1);
    if (SAL_SOK != ret)
    {
        SYS_LOGE("common init fail\n");
        return SAL_FAIL;
    }

    memcpy(mem_cfg.pool_info, pPoolInfo, poolInfoSize);
    ret = hd_common_mem_init(&mem_cfg);
    if (ret != SAL_SOK)
    {
        SYS_LOGE("common init fail ret:%#x\n", ret);
        return SAL_FAIL;
    }

    return SAL_SOK;

}

/**
 * @function    sys_ntv3_allocVideoFrameInfoSt
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 sys_ntv3_init(UINT32 u32ViChnNum)
{
    INT32 sRet = SAL_SOK;

    sRet = sys_ntv3_memInit();   /*先使用sdk提供设备树方式进行分配内存*/
    if (SAL_SOK != sRet)
    {
        SYS_LOGE("sys_ntv3_memInit fail\n");
        return SAL_FAIL;
    }
    
    SYS_LOGI("common init ok\n");

    return sRet;
}

/**
 * @function    sys_ntv3_allocVideoFrameInfoSt
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 sys_ntv3_allocVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{

    HD_VIDEO_FRAME *pOutFrm = NULL;
    if (NULL == pstSystemFrameInfo)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    pOutFrm = (HD_VIDEO_FRAME *)SAL_memMalloc(sizeof(HD_VIDEO_FRAME), "sys_ntv3", "video_frame");
    if (!pOutFrm)
    {
        SYS_LOGE("alloc mem[mod:sys_ntv3, name:video_frame] fail \n");
        return SAL_FAIL;
    }

    memset(pOutFrm,0x00,sizeof(HD_VIDEO_FRAME));
    pstSystemFrameInfo->uiAppData = (PhysAddr)pOutFrm;

    return SAL_SOK;
}

/**
 * @function    sys_ntv3_buildVideoFrame
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 sys_ntv3_buildVideoFrame(SAL_VideoFrameBuf *pVideoFrame, SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{

    if (NULL == pstSystemFrameInfo || NULL == pVideoFrame)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }
    
    HD_VIDEO_FRAME *pVideoFrameInfo = (HD_VIDEO_FRAME *)pstSystemFrameInfo->uiAppData;

    if (!pVideoFrameInfo)
    {
        SYS_LOGE("pVideoFrameInfo     is NULL \n");
        return SAL_FAIL;
    }
    pVideoFrameInfo->sign = MAKEFOURCC('V','F','R','M');
    pVideoFrameInfo->blk  = pVideoFrame->vbBlk;
    pVideoFrameInfo->dim.w  = pVideoFrame->frameParam.width;
    pVideoFrameInfo->dim.h = pVideoFrame->frameParam.height;
    pVideoFrameInfo->pxlfmt = HD_VIDEO_PXLFMT_YUV420;
    /* 引擎只给了虚拟地址，所以这里需要从虚拟地址转为物理地址。DSP其他模块物理地址需要从上层传下来 */
    pVideoFrameInfo->phy_addr[0] =  pVideoFrame->phyAddr[0] - pVideoFrame->frameParam.crop.top * pVideoFrame->stride[0] - pVideoFrame->frameParam.crop.left;
    pVideoFrameInfo->phy_addr[1] =  pVideoFrame->phyAddr[1] - pVideoFrame->frameParam.crop.top * pVideoFrame->stride[0] / 2 - pVideoFrame->frameParam.crop.left;
    // pVideoFrameInfo->phy_addr[1] =  pVideoFrame->phyAddr[1];
    pVideoFrameInfo->ddr_id = DDR1_ADDR_START > pVideoFrame->phyAddr[0]?DDR_ID0:DDR_ID1;
    pVideoFrameInfo->timestamp = (UINT64)pVideoFrame->pts;
    pVideoFrameInfo->count =(UINT32)pVideoFrame->frameNum;
    pVideoFrameInfo->loff[0] = pVideoFrame->stride[0];
    pVideoFrameInfo->loff[1] = pVideoFrame->stride[1];
    pVideoFrameInfo->ph[0] = pVideoFrame->vertStride[0]; /* 记录buffer高度(跨距) */

    pstSystemFrameInfo->uiDataAddr = pVideoFrame->virAddr[0];
    pstSystemFrameInfo->uiDataWidth = pVideoFrameInfo->dim.w;
    pstSystemFrameInfo->uiDataHeight = pVideoFrameInfo->dim.h;
    pstSystemFrameInfo->uiDataLen = pVideoFrame->frameParam.width * pVideoFrame->frameParam.height * 3 / 2;
    pstSystemFrameInfo->u32LeftOffset = pVideoFrame->frameParam.crop.left;
    pstSystemFrameInfo->u32TopOffset = pVideoFrame->frameParam.crop.top;

    return SAL_SOK; 
}

/**
 * @function    sys_ntv3_getVideoFrameInfo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 sys_ntv3_getVideoFrameInfo(SYSTEM_FRAME_INFO *pstSystemFrameInfo, SAL_VideoFrameBuf *pVideoFrame)
{
    if (NULL == pstSystemFrameInfo || NULL == pVideoFrame)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }
    HD_VIDEO_FRAME *pVideoFrameInfo = (HD_VIDEO_FRAME *)pstSystemFrameInfo->uiAppData;
    if (NULL == pVideoFrameInfo)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    SAL_VideoDataFormat enSalPixFmt = SAL_VIDEO_DATFMT_YUV420SP_UV;
    
    fmtConvert_ntv3_plat2sal(pVideoFrameInfo->pxlfmt, &enSalPixFmt);
    //pVideoFrame->poolId = pVideoFrameInfo->blk;
    pVideoFrame->stride[0] = pVideoFrameInfo->loff[0];
    pVideoFrame->stride[1] = pVideoFrameInfo->loff[1];
    pVideoFrame->frameParam.crop.left = pstSystemFrameInfo->u32LeftOffset;
    pVideoFrame->frameParam.crop.top = pstSystemFrameInfo->u32TopOffset;
    pVideoFrame->vbBlk = pVideoFrameInfo->blk;
    pVideoFrame->frameParam.width = pVideoFrameInfo->dim.w;
    pVideoFrame->frameParam.height = pVideoFrameInfo->dim.h;
    pVideoFrame->frameParam.dataFormat = enSalPixFmt;
    pVideoFrame->phyAddr[0] = (PhysAddr)pVideoFrameInfo->phy_addr[0] + pVideoFrame->frameParam.crop.top * pVideoFrame->stride[0] + pVideoFrame->frameParam.crop.left;
    pVideoFrame->phyAddr[1] = (PhysAddr)pVideoFrameInfo->phy_addr[1] + pVideoFrame->frameParam.crop.top * pVideoFrame->stride[0] / 2 + pVideoFrame->frameParam.crop.left;
    pVideoFrame->phyAddr[2] = pVideoFrame->phyAddr[1];

    if(pstSystemFrameInfo->uiDataAddr != 0x00)
    {
        pVideoFrame->virAddr[0] = pstSystemFrameInfo->uiDataAddr;
        pVideoFrame->virAddr[1] = pVideoFrame->virAddr[0] + pVideoFrame->stride[0] * pVideoFrameInfo->dim.h;
        pVideoFrame->virAddr[2] = pVideoFrame->virAddr[1];
    }
    else
    {
         SAL_LOGE("u64PhyAddr = 0x%lx GetVirAddr err!!!\n",pVideoFrameInfo->phy_addr[0]);
    }


    pVideoFrame->frameNum =  (UINT64)pVideoFrameInfo->count;
    pVideoFrame->pts = (UINT64)pVideoFrameInfo->timestamp;


    return SAL_SOK; 
}

/**
 * @function    sys_ntv3_releaseVideoFrameInfoSt
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 sys_ntv3_releaseVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    if (NULL == pstSystemFrameInfo)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }
    
    HD_VIDEO_FRAME *pOutFrm = (HD_VIDEO_FRAME *)pstSystemFrameInfo->uiAppData;
    if (NULL == pOutFrm)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    SAL_memfree(pOutFrm, "sys_ntv3", "video_frame");
    pstSystemFrameInfo->uiAppData = 0;

    return SAL_SOK;    
}

/**
 * @function    sys_ntv3_setVideoTimeIfno
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 sys_ntv3_setVideoTimeIfno(SYSTEM_FRAME_INFO *pstSystemFrameInfo, UINT32 u32RefTime, UINT64 u64Pts)
{
    HD_VIDEO_FRAME *pstFrame = NULL;


    if (NULL == pstSystemFrameInfo)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    pstFrame = (HD_VIDEO_FRAME *)pstSystemFrameInfo->uiAppData;
    if (NULL == pstFrame)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    pstFrame->timestamp = u32RefTime;
    pstFrame->count = u64Pts;

    return SAL_SOK;
}

/**
 * @function    sys_ntv3_getPts
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 sys_ntv3_getPts(UINT64 *pCurPts)
{
    UINT64 p64CurPts = 0;
    
    if (NULL == pCurPts)
    {
        SYS_LOGE("error\n");
        return SAL_FAIL;
    }

    p64CurPts = hd_gettime_ms();
    if (p64CurPts == 0)
    {
        VDEC_LOGE("ss_mpi_sys_get_cur_pts error 0x%llx\n", p64CurPts);
        return SAL_FAIL;
    }

    *pCurPts = p64CurPts;

    return SAL_SOK;
}


INT32 sys_ntv3_bind(SYSTEM_MOD_ID_E uiSrcModId, UINT32 uiSrcDevId, UINT32 uiSrcChn, SYSTEM_MOD_ID_E uiDstModId, UINT32 uiDstDevId, UINT32 uiDstChn,UINT32 uiBind)
{
    HD_RESULT ret;
    HD_OUT_ID out_id;
    HD_IN_ID dest_in_id;

    HD_DAL self_id;
    HD_IO _out_id;
    HD_DAL dest_id;
    HD_IO _in_id;
    //UINT32 u32VpeDevId;

    if (SYSTEM_MOD_ID_DUP == uiSrcModId)
    {

#if 1   /*安检盒子使用绑定方式需要一进多出*/
        /* 一进多出的方式 */
        out_id = HD_VIDEOPROC_OUT(uiSrcDevId, uiSrcChn); 
#else  /*安检机送帧不会使用绑定方式*/
        /* 一进一出的方式 */
        u32VpeDevId = vpe_nt9833x_getVpeDevId(uiSrcDevId, uiSrcChn);
        if (u32VpeDevId != (UINT32)(-1))
        {
            out_id = HD_VIDEOPROC_OUT(u32VpeDevId, 0);
            vpe_nt9833x_setVpeMode(uiSrcDevId, uiSrcChn, VPE_FLOW_MODE);
        }
        else
        {
            SYS_LOGE("vpe_nt9833x_getVpeDevId failed! uiSrcDevId(%u), uiSrcChn(%u)\n", uiSrcDevId, uiSrcChn);
            return SAL_FAIL;
        }
#endif
    }
    else if (SYSTEM_MOD_ID_CAPT == uiSrcModId)
    {
        out_id = HD_VIDEOCAP_OUT(uiSrcDevId, uiSrcChn); //这里uiSrcChn传外面传进来的参数还是直接传0，还要具体看一下

    }
    else if (SYSTEM_MOD_ID_VDEC == uiSrcModId)
    {
        out_id = HD_VIDEODEC_OUT(uiSrcDevId, uiSrcChn);
    }
    else
    {
        SYS_LOGE("uiSrcModId %d not support \n", uiSrcModId);
        return SAL_FAIL;
    }

    if (SYSTEM_MOD_ID_DUP == uiDstModId)
    {
#if 1   /*安检盒子使用绑定方式需要一进多出*/
        dest_in_id = HD_VIDEOPROC_IN(uiDstDevId, uiDstChn);
#else
        /* 一进一出的方式 */
        u32VpeDevId = vpe_nt9833x_getVpeDevId(uiDstDevId, uiDstChn);
        if (u32VpeDevId != (UINT32)(-1))
        {
            dest_in_id = HD_VIDEOPROC_IN(u32VpeDevId, 0);
        }
        else
        {
            SYS_LOGE("vpe_nt9833x_getVpeDevId failed! uiDstDevId(%u), uiDstChn(%u)\n", uiDstDevId, uiDstChn);
            return SAL_FAIL;
        }
#endif
    }
    else if (SYSTEM_MOD_ID_DISP == uiDstModId)
    {
        dest_in_id = HD_VIDEOOUT_IN(uiDstDevId, uiDstChn);
    }
    else if (SYSTEM_MOD_ID_VENC == uiDstModId)
    {
        dest_in_id = HD_VIDEOENC_IN(uiDstDevId, uiDstChn);
    }
    else
    {
        SYS_LOGE("uiDstModId %d not support \n", uiDstModId);
        return SAL_FAIL;
    }

    self_id = HD_GET_DEV(out_id);
    _out_id = HD_GET_OUT(out_id);
    dest_id = HD_GET_DEV(dest_in_id);
    _in_id = HD_GET_IN(dest_in_id);

    if (1 == uiBind)
    {
        ret = bd_bind(self_id, _out_id, dest_id, _in_id);
        if (ret != HD_OK)
        {
            SYS_LOGE("hd_bind failed! src:      modid %u, devid(%#x) out(%u);         dst:modid %u, devid(%#x) in(%u)\n", uiSrcModId, self_id, _out_id, uiDstModId, dest_id, _in_id);
            return SAL_FAIL;
        }

        SYS_LOGI("hd_bind success! src:      modid %u, devid(%#x) out(%u);         dst:modid %u, devid(%#x) in(%u)\n", uiSrcModId, self_id, _out_id, uiDstModId, dest_id, _in_id);
    }
    else
    {

        ret = bd_unbind(self_id, _out_id);
        if (ret != HD_OK)
        {
            SYS_LOGE("hd_unbind failed! src:      modid %u, devid(%#x) out(%u);         dst:modid %u, devid(%#x) in(%u)\n", uiSrcModId, self_id, _out_id, uiDstModId, dest_id, _in_id);
            return SAL_FAIL;
        }

        SYS_LOGI("hd_unbind success! src:      modid %u, devid(%#x) out(%u);         dst:modid %u, devid(%#x) in(%u)\n", uiSrcModId, self_id, _out_id, uiDstModId, dest_id, _in_id);
    }
    return SAL_SOK;
}

/**
 * @function    sys_plat_register
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
const SYS_PLAT_OPS *sys_plat_register(void)
{
    INT32 sRet = SAL_SOK;
    
    g_stSysPlatOps.init = NULL;
    g_stSysPlatOps.deInit = sys_ntv3_deInit;
    g_stSysPlatOps.bind = sys_ntv3_bind;
    g_stSysPlatOps.getPts = sys_ntv3_getPts;
    g_stSysPlatOps.getVideoFrameInfo = sys_ntv3_getVideoFrameInfo;
    g_stSysPlatOps.buildVideoFrame = sys_ntv3_buildVideoFrame;
    g_stSysPlatOps.allocVideoFrameInfoSt = sys_ntv3_allocVideoFrameInfoSt;
    g_stSysPlatOps.releaseVideoFrameInfoSt = sys_ntv3_releaseVideoFrameInfoSt;
    g_stSysPlatOps.setVideoTimeInfo = sys_ntv3_setVideoTimeIfno;


    sRet = sys_ntv3_init(0);
    if (SAL_SOK != sRet)
    {
        SYS_LOGE("sys_ntv3_init init fail\n");
    }
    
    return &g_stSysPlatOps;
}


