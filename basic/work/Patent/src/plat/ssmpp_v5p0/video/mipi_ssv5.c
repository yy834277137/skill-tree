/**
 * @file   mipi_ssv5.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  显示mipi_tx模块
 * @author liuxianying
 * @date   2021/8/4
 * @note
 * @note \n History
   1.日    期: 2021/8/4
     作    者: liuxianying
     修改历史: 创建文件
 */



/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <platform_sdk.h>
#include <platform_hal.h>
#include "capbility.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define DDM_DEV_REGINTER_NAME "/proc/reginterface"

#define DDM_IOC_DP_REG_SET	_IOW(DDM_IOC_DP_MNUM, 2, UINT32)
#define DDM_IOC_DP_REG_GET	_IOW(DDM_IOC_DP_MNUM, 3, UINT32)

#define MIPI_TX_LANE_NUM (4)

static INT32 s_s32SampleMemDev = -1;

#define SAMPLE_MEM_DEV_OPEN if (s_s32SampleMemDev <= 0) \
    { \
        s_s32SampleMemDev = open("/dev/mem", O_RDWR | O_SYNC); \
        if (s_s32SampleMemDev < 0) \
        { \
            perror("Open dev/mem error"); \
            return NULL; \
        } \
    } \

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function   Mpi_Tx_Start
 * @brief      mipi输出参数配置，启动mipi输出
 * @param[in]  const BT_TIMING_S *pstTiming
 * @param[out] None
 * @return     void
 */
void Mpi_Tx_Start(const BT_TIMING_S *pstTiming)
{
    INT32 fd = -1;
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    combo_dev_cfg_t stMipiTxConfig;

    if (NULL == pstTiming)
    {
        SAL_LOGE("invalid timing\n");
        return;
    }

    stMipiTxConfig.devno = 0;
    for (i = 0; i < LANE_MAX_NUM; i++)
    {
        if (i < MIPI_TX_LANE_NUM)
        {
            stMipiTxConfig.lane_id[i] = i;
        }
        else
        {
            stMipiTxConfig.lane_id[i] = -1;
        }
    }

    stMipiTxConfig.out_mode = OUT_MODE_DSI_VIDEO;
    stMipiTxConfig.video_mode = BURST_MODE;
    stMipiTxConfig.out_format = OUT_FORMAT_RGB_24BIT;
    stMipiTxConfig.sync_info.hsa_pixels = pstTiming->u32HSync;
    stMipiTxConfig.sync_info.hbp_pixels = pstTiming->u32HBackPorch;
    stMipiTxConfig.sync_info.hact_pixels = pstTiming->u32Width;
    stMipiTxConfig.sync_info.hfp_pixels =  pstTiming->u32HFrontPorch;
    stMipiTxConfig.sync_info.vsa_lines = pstTiming->u32VSync;
    stMipiTxConfig.sync_info.vbp_lines = pstTiming->u32VBackPorch;
    stMipiTxConfig.sync_info.vfp_lines = pstTiming->u32VFrontPorch;
    stMipiTxConfig.sync_info.vact_lines = pstTiming->u32Height;
    stMipiTxConfig.phy_data_rate = pstTiming->u32PixelClock * 24 / MIPI_TX_LANE_NUM / 1000000 + 50;               /* 24一个像素点24bit，50是增加一个固定的裕量 */
    stMipiTxConfig.pixel_clk = pstTiming->u32PixelClock / 1000;

    fd = open("/dev/ot_mipi_tx", O_RDWR);
    if (fd < 0)
    {
        SAL_LOGE("open ot_mipi_tx dev failed\n");
        return;
    }

    /*
     * step 1 : Config mipi_tx controller.
     */
    s32Ret = ioctl(fd, OT_MIPI_TX_SET_DEV_CFG, &stMipiTxConfig);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("MIPI_TX SET_DEV_CONFIG failed\n");
        close(fd);
        return;
    }

    /*
     * NOTICE!!! Do it yourself: change SAMPLE_VO_USE_DEFAULT_MIPI_TX to 0.
     * step 2 : Init screen or other peripheral unit.
     */

    usleep(10000);

    /*
     * step 3 : enable mipi_tx controller.
     */
    s32Ret = ioctl(fd, OT_MIPI_TX_ENABLE);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("MIPI_TX enable failed\n");
        close(fd);
        return;
    }

    close(fd);
}

/**
 * @function   Mpi_Tx_Start
 * @brief      mipi输出参数配置，启动mipi输出
 * @param[in]  const BT_TIMING_S *pstTiming
 * @param[out] None
 * @return     void
 */
void Mpi_Tx_Stop(void)
{
    INT32 fd = -1;
    INT32 s32Ret = SAL_SOK;
    
    fd = open("/dev/ot_mipi_tx", O_RDWR);
    if (fd < 0)
    {
        SAL_LOGE("open ot_mipi_tx dev failed\n");
        return;
    }

    s32Ret = ioctl(fd, OT_MIPI_TX_DISABLE);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("MIPI_TX enable failed\n");
    }

    close(fd);
}

/**
 * @function   Mpi_Tx_IOMmap
 * @brief      物理地址映射虚拟地址
 * @param[in]  UINT64 u64PhyAddr  物理地址
 * @param[in]  UINT32 u32Size
 * @param[out] None
 * @return     VOID * pPageAddr 映射后虚拟地址
 */
VOID *Mpi_Tx_IOMmap(UINT64 u64PhyAddr, UINT32 u32Size)
{
    UINT32 u32Diff;
    UINT64 u64PagePhy;
    UINT32 *pPageAddr;
    UINTL ulPageSize;

    SAMPLE_MEM_DEV_OPEN;

    /**********************************************************
       PageSize will be 0 when u32size is 0 and u32Diff is 0,
       and then mmap will be error (error: Invalid argument)
     ***********************************************************/
    if (!u32Size)
    {
        printf("Func: %s u32Size can't be 0.\n", __FUNCTION__);
        return NULL;
    }

    /* The mmap address should align with page */
    u64PagePhy = u64PhyAddr & 0xfffffffffffff000ULL;
    u32Diff = u64PhyAddr - u64PagePhy;

    /* The mmap size shuld be mutliples of 1024 */
    ulPageSize = ((u32Size + u32Diff - 1) & 0xfffff000UL) + 0x1000;

    pPageAddr = mmap((void *)0, ulPageSize, PROT_READ | PROT_WRITE,
                     MAP_SHARED, s_s32SampleMemDev, u64PagePhy);
    if (MAP_FAILED == pPageAddr)
    {
        perror("mmap error");
        return NULL;
    }

    return (VOID *) (pPageAddr + u32Diff);
}

/**
 * @function   Mpi_Tx_Munmap
 * @brief      虚拟地址释放
 * @param[in]  VOID* pVirAddr 虚拟地址
 * @param[in]  UINT32 u32Size
 * @param[out] None
 * @return     INT32
 */
INT32 Mpi_Tx_Munmap(VOID *pVirAddr, UINT32 u32Size)
{
    UINT64 u64PageAddr;
    UINT32 u32PageSize;
    UINT32 u32Diff;

    u64PageAddr = (((UINTL)pVirAddr) & 0xfffffffffffff000ULL);
    u32Diff = (UINTL)pVirAddr - u64PageAddr;
    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000UL) + 0x1000;

    return munmap((VOID *)(UINTL)u64PageAddr, u32PageSize);
}

/**
 * @function   Mpi_Tx_SetReg
 * @brief      设置寄存器值
 * @param[in]  UINT64 u64Addr   寄存器地址
 * @param[in]  UINT32 u32Value  设置参数
 * @param[out] None
 * @return     INT32
 */
INT32 Mpi_Tx_SetReg(UINT64 u64Addr, UINT32 u32Value)
{
    UINT32 *pu32RegAddr = NULL;
    UINT32 u32MapLen = sizeof(u32Value);

    pu32RegAddr = (UINT32 *)Mpi_Tx_IOMmap(u64Addr, u32MapLen);
    if (NULL == pu32RegAddr)
    {
        return TD_FAILURE;
    }

    *pu32RegAddr = u32Value;

    return Mpi_Tx_Munmap(pu32RegAddr, u32MapLen);
}

/**
 * @function   Mpi_Tx_GetReg
 * @brief      获取寄存器值
 * @param[in]  UINT64 u64Addr     寄存器地址
 * @param[in]  UINT32 *pu32Value  寄存器值
 * @param[out] None
 * @return     INT32  SAL_SOK  : 成功
 *                     SAL_FAIL : 失败
 */
INT32 Mpi_Tx_GetReg(UINT64 u64Addr, UINT32 *pu32Value)
{
    UINT32 *pu32RegAddr = NULL;
    UINT32 u32MapLen;

    if (NULL == pu32Value)
    {
        return OT_ERR_SYS_NULL_PTR;
    }

    u32MapLen = sizeof(*pu32Value);
    pu32RegAddr = (UINT32 *)Mpi_Tx_IOMmap(u64Addr, u32MapLen);
    if (NULL == pu32RegAddr)
    {
        return TD_FAILURE;
    }

    *pu32Value = *pu32RegAddr;

    return Mpi_Tx_Munmap(pu32RegAddr, u32MapLen);
}

/**
 * @function   Mpi_Tx_SetMipi
 * @brief      设置mipi控制器
 * @param[in]  void
 * @param[out] None
 * @return     int SAL_SOK  : 成功
 *           SAL_FAIL : 失败
 */
INT32 Mpi_Tx_SetMipi(void)
{
    int fd;
    int ret;
    UINT32 value = 0;
    DDM_dpGet dp_info;
    int i = 0;
    /* UINT32  *p64VirAddr    = NULL; */
    UINTL VID_MODE_CFG = 0x11170038;
    CAPB_DISP *pstDispCab = capb_get_disp();

    if (NULL == pstDispCab)
    {
        SAL_LOGE("get disp capblity fail\n");
        return SAL_FAIL;
    }

    fd = open(DDM_DEV_REGINTER_NAME, O_RDWR);
    if (fd < 0)
    {
        SAL_LOGE("Open %s error \n", DDM_DEV_REGINTER_NAME);
        return SAL_FAIL;
    }

    ret = ioctl(fd, DDM_IOC_DP_REG_GET, &value);
    if (ret < 0)
    {
        SAL_LOGE("ioctl %lu error \n", (UINTL)DDM_IOC_DP_REG_GET);
    }

    SAL_LOGI("SRC is 0x%x\n", value);

    value = value | (1 << 8) | (1 << 25);

    SAL_LOGI("DST is 0x%x\n", value);
    ret = ioctl(fd, DDM_IOC_DP_REG_SET, &value);
    if (ret < 0)
    {
        SAL_LOGE("ioctl %lu error \n", (UINTL)DDM_IOC_DP_REG_SET);
    }

    ret = ioctl(fd, DDM_IOC_DP_REG_GET, &value);
    if (ret < 0)
    {
        SAL_LOGE("ioctl %lu error \n", (UINTL)DDM_IOC_DP_REG_SET);
    }

    close(fd);

    if (MIPI_TX_CHIP_LT8912B == pstDispCab->disp_mipi_chip)
    {
        /* 安检机配置9812b 寄存器 防止屏幕闪屏 */
        fd = open(DDM_DEV_DP_NAME, O_RDWR);
        if (fd < 0)
        {
            SAL_LOGE("Open %s error \n", DDM_DEV_DP_NAME);
            return SAL_FAIL;
        }

        memset(&dp_info, 0x00, sizeof(dp_info));
        dp_info.channel = VO_DEV_DHD1;
        ret = ioctl(fd, DDM_IOC_DP_GET_PARAM, &dp_info);
        if (ret < 0)
        {
            SAL_LOGE("DDM_IOC_DP_GET_PARAM failed:%d\n", errno);
        }

        if (dp_info.chip_count)
        {
            for (i = 0; i < dp_info.chip_count; i++)
            {
                if (0 == strcmp(dp_info.chip_name[i], "lt9812b"))
                {
                    Mpi_Tx_GetReg(VID_MODE_CFG, &value);
                    value = 0xf02;
                    ret = Mpi_Tx_SetReg(VID_MODE_CFG, value);
                    if (SAL_SOK != ret)
                    {
                        close(fd);
                        SAL_LOGE("umap %#x\n", ret);
                        return SAL_FAIL;
                    }
                }
            }

            value = dp_info.channel;
            ret = ioctl(fd, DDM_IOC_DP_RX_RESET, &value);
            if (ret < 0)
            {
                SAL_LOGE("DDM_IOC_DP_RX_RESET failed: %d\n", errno);
            }
            else
            {
                SAL_LOGI("DDM_IOC_DP_RX_RESET successful \n");
            }

            usleep(10 * 1000);
            ret = ioctl(fd, DDM_IOC_DP_DISPLAY_ENABLE, &value);
            if (ret < 0)
            {
                printf("DDM_IOC_DP_DISPLAY_ENABLE failed:%d", errno);
            }
            else
            {
                SAL_LOGI("DDM_IOC_DP_DISPLAY_ENABLE successful \n");
            }
        }

        close(fd);
    }

    return SAL_SOK;
}

