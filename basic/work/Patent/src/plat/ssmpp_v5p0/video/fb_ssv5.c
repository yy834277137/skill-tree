/**
 * @file   fb_ssv5.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  图形层hiv5p抽象层接口
 * @author liuxianying
 * @date   2021/8/5
 * @note
 * @note \n History
   1.日    期: 2021/8/5
     作    者: liuxianying
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include <platform_sdk.h>
#include "fb_ssv5.h"
#include "../../hal/hal_inc_inter/fb_hal_inter.h"



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */



/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

static struct fb_bitfield s_r16 = {10, 5, 0};
static struct fb_bitfield s_g16 = {5, 5, 0};
static struct fb_bitfield s_b16 = {0, 5, 0};
static struct fb_bitfield s_a16 = {15, 1, 0};

static struct fb_bitfield s_a32 = {24, 8, 0};
static struct fb_bitfield s_r32 = {16, 8, 0};
static struct fb_bitfield s_g32 = {8,  8, 0};
static struct fb_bitfield s_b32 = {0,  8, 0};

static FB_PLAT_OPS g_stFbPlatOps;
extern FB_COMMON g_fb_common[FB_DEV_NUM_MAX];


/**
 * @function   fb_ssv5MallocFb
 * @brief      申请mmz内存初始化pfb_chn本地全局变量
 * @param[in]  UINT32 uiChn        fb设备号
 * @param[in]  FB_COMMON *pfb_chn  fb信息
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_ssv5MallocFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{
    UINT32 uiSize = 0;

    if(uiChn >= FB_DEV_NUM_MAX)
    {
        SAL_ERROR("uiChn not exist, chan = %d!!\n", uiChn);
        return SAL_FAIL;
    }

    uiSize = pfb_chn->fb_dev.uiWidth * pfb_chn->fb_dev.uiHeight * pfb_chn->u32BytePerPixel * FB_BUFF_CNT;
    uiSize = SAL_align(uiSize ,16);

    if(uiSize == 0x00)
    {
        SAL_ERROR("error size = %d!!\n", uiSize);
        return SAL_FAIL;
    }
    
    SAL_INFO("Dev %d,Get DSP Buff uiWidth %d uiHeight %d size %d !!!\n",uiChn, pfb_chn->fb_dev.uiWidth, pfb_chn->fb_dev.uiHeight,uiSize);

    if(uiSize == 0x00)
    {
        SAL_ERROR("uiSize %d!!\n", uiSize);
        return SAL_FAIL;
    }
    
    if (TD_FAILURE == ss_mpi_sys_mmz_alloc((td_phys_addr_t *)&pfb_chn->fb_dev.ui64CanvasAddr, ((void**)&pfb_chn->fb_dev.pBuf), NULL, NULL, uiSize))
    {
        SAL_ERROR("allocate memory uiSize %d  failed\n",uiSize);
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }

    pfb_chn->fb_dev.offset = SAL_align(uiSize / FB_BUFF_CNT ,16);

    SAL_INFO("FB dev %d MMZ ADDR : %lu offset %d SIZE : 0x%x !!!\n",uiChn,(UINTL)pfb_chn->fb_dev.ui64CanvasAddr, pfb_chn->fb_dev.offset, uiSize);

    memset(pfb_chn->fb_dev.pBuf, 0x00, uiSize);

    pfb_chn->fb_dev.uiSize = uiSize;
        
    return SAL_SOK;
}

/**
 * @function   fb_ssv5RefreshFb
 * @brief      刷新图形层
 * @param[in]  UINT32 uiChn        fb设备号
 * @param[in]  FB_COMMON *pfb_chn  fb信息
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_ssv5RefreshFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{
    INT32                     s32Ret       = TD_SUCCESS;
    ot_fb_color_format         enClrFmt     = OT_FB_FORMAT_ARGB8888;
    static UINT32               cnt[FB_DEV_NUM_MAX] = {0};
    ot_fb_buf stCanvasBuf  = {0};


    if (cnt[uiChn] >= FB_BUFF_CNT)
    {
       cnt[uiChn] = 0;
    }
    
    switch (pfb_chn->bitWidth)
    {
        case 32:
        {
            enClrFmt = OT_FB_FORMAT_ARGB8888;
            break;
        }
        case 16:
        {
            enClrFmt = OT_FB_FORMAT_ARGB1555;
            break;
        }
        default:
        {
            enClrFmt = OT_FB_FORMAT_ARGB8888;
            break;
        }
    }



    stCanvasBuf.canvas.phys_addr = pfb_chn->fb_dev.ui64CanvasAddr + (cnt[uiChn] * pfb_chn->fb_dev.offset);
    stCanvasBuf.canvas.height  = pfb_chn->fb_dev.uiHeight;
    stCanvasBuf.canvas.width   = pfb_chn->fb_dev.uiWidth;
    stCanvasBuf.canvas.pitch   = pfb_chn->fb_dev.uiWidth * (pfb_chn->u32BytePerPixel);
    stCanvasBuf.canvas.format      = enClrFmt;
    
    stCanvasBuf.update_rect.x = 0;
    stCanvasBuf.update_rect.y = 0;
    stCanvasBuf.update_rect.width = pfb_chn->fb_dev.uiWidth;
    stCanvasBuf.update_rect.height = pfb_chn->fb_dev.uiHeight;
        
    s32Ret = ioctl(pfb_chn->fd, FBIO_REFRESH, &stCanvasBuf);
    if (s32Ret < 0)
    {
        SAL_ERROR("REFRESH failed!\n");
        return TD_NULL;
    }

    cnt[uiChn]++;
    
    return SAL_SOK;
}

/**
 * @function   fb_ssv5CreateDevFb3
 * @brief      创建G3显示，鼠标层显示设备
 * @param[in]  UINT32 uiChn                   fb设备号
 * @param[in]  FB_COMMON *pfb_chn             fb设备信息
 * @param[in]  FB_INIT_ATTR_ST *pstHalFbAttr  fb坐标区域
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_ssv5CreateDevFb3(UINT32 uiChn,FB_COMMON *pfb_chn, FB_INIT_ATTR_ST *pstHalFbAttr)
{
    
    INT32                     s32Ret       = TD_SUCCESS;
    BOOL                    bShow        = TD_TRUE;
    UINT32                     bindChn      = 0;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo stVarInfo;
    FB_COMMON *pfb0_chn = NULL;   
    ot_vo_layer s32Layer = OT_VO_LAYER_G3;
    ot_vo_csc stCSC = {0};

    if (NULL == pstHalFbAttr)
    {
        SAL_ERROR("pstHalFbAttr is null !!\n");
        return SAL_FAIL;
    }    

    bindChn            = pfb_chn->mouseNewChn;
    pfb0_chn = &g_fb_common[bindChn];
    
    //鼠标必须在UI的FB MAP后才允许MAP
    if(pfb0_chn->DevMMAP != SAL_TRUE && pfb0_chn->AppMMAP != SAL_TRUE)
    {
        SAL_ERROR("Fb%d is Umap !\n",bindChn);
        return SAL_FAIL;
    }

    if (pfb_chn->fd >= 0)
    {
        SAL_WARN("chn %d,fd %d!\n", uiChn,pfb_chn->fd);
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
    }


    s32Ret = ss_mpi_vo_unbind_layer(s32Layer, 0);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_ERROR("UnBinds32Layer failed with 0x%x!\n", s32Ret);
        if (pfb_chn->fd >= 0)
        {
            if(close(pfb_chn->fd) < 0)
            {
               SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
               close(pfb_chn->fd);
            } 
        }
        
        ss_mpi_vo_unbind_layer(s32Layer, 0);
        //return s32Ret;
    }

    s32Ret = ss_mpi_vo_bind_layer(s32Layer, pfb_chn->mouseNewChn);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_ERROR("Binds32Layer failed with 0x%x!\n", s32Ret);
        return s32Ret;
    }

    pfb_chn->fb_dev.uiWidth  = pstHalFbAttr->uiW;
    pfb_chn->fb_dev.uiHeight = pstHalFbAttr->uiH;
    pfb_chn->mouseLastChn    = pfb_chn->mouseNewChn;
    SAL_INFO("pfb_chn->fb_dev.uiWidth %d pfb_chn->fb_dev.uiHeight %d mouseNewChn %d\n", pfb_chn->fb_dev.uiWidth, pfb_chn->fb_dev.uiHeight,pfb_chn->mouseNewChn);

    sprintf((char *)pfb_chn->fbName, "/dev/fb%d", uiChn);

    /*************************************
    * 1. open framebuffer device overlay 0
    ****************************************/

    pfb_chn->fd = open((char *)pfb_chn->fbName, O_RDWR, 0);
    if (pfb_chn->fd < 0)
    {
        SAL_ERROR("open %s failed!\n", pfb_chn->fbName);
        return SAL_FAIL;
    }

    SAL_WARN("open chn %d,fd %d!\n", uiChn,pfb_chn->fd);

    
    if (pfb_chn->mouseNewChn == GRAPHICS_LAYER_G1)
    {
        s32Ret = ss_mpi_vo_get_layer_csc(s32Layer, &stCSC);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_ERROR("OT_MPI_VO_GetVideoLayerCSC failed!s32Ret 0x%x \n",s32Ret);
            if(close(pfb_chn->fd) < 0)
            {
               SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
               close(pfb_chn->fd);
            } 
            pfb_chn->fd = -1;
            return SAL_FAIL;
        }
        
        //stCSC.csc_matrix = VO_CSC_MATRIX_IDENTITY;  /* 新sdk没有该选择  调试时确认选项*/
        stCSC.luma = 80;
        stCSC.contrast = 40;
        s32Ret = ss_mpi_vo_set_layer_csc(s32Layer, &stCSC);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_ERROR("OT_MPI_VO_SetVideoLayerCSC failed!s32Ret 0x%x \n",s32Ret);
            if(close(pfb_chn->fd) < 0)
            {
               SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
               close(pfb_chn->fd);
            } 
            pfb_chn->fd = -1;
            return SAL_FAIL;
        }
    }

    bShow = TD_FALSE;
    if (ioctl(pfb_chn->fd, FBIOPUT_SHOW_GFBG, &bShow) < 0)
    {
        SAL_ERROR("FBIOPUT_SHOW_GFBG!\n");
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }
    
    
    s32Ret = ioctl(pfb_chn->fd, FBIOGET_VSCREENINFO, &stVarInfo);
    if (s32Ret < 0)
    {
        SAL_ERROR("GET_VSCREENINFO failed!\n");
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }

    switch (pfb_chn->bitWidth)
    {
        case 32:
        {
            stVarInfo.transp = s_a32;
            stVarInfo.red    = s_r32;
            stVarInfo.green  = s_g32;
            stVarInfo.blue   = s_b32;            
            stVarInfo.bits_per_pixel = 32;
            break;
        }
        case 16:
        {
            stVarInfo.transp = s_a16;
            stVarInfo.red    = s_r16;
            stVarInfo.green  = s_g16;
            stVarInfo.blue   = s_b16;
            stVarInfo.bits_per_pixel = 16;
            break;
        }
        default:
        {
            stVarInfo.transp = s_a32;
            stVarInfo.red    = s_r32;
            stVarInfo.green  = s_g32;
            stVarInfo.blue   = s_b32;            
            stVarInfo.bits_per_pixel = 32;
            break;
        }
    }

    pfb_chn->u32BytePerPixel = stVarInfo.bits_per_pixel/8;
    
    SAL_INFO("u32BytePerPixel %d bits_per_pixel %d \n", pfb_chn->u32BytePerPixel, stVarInfo.bits_per_pixel);

    stVarInfo.activate = FB_ACTIVATE_NOW;
    stVarInfo.xres_virtual = pfb_chn->fb_dev.uiWidth;
    stVarInfo.yres_virtual = pfb_chn->fb_dev.uiHeight;   
    
    stVarInfo.xres = pfb_chn->fb_dev.uiWidth;
    stVarInfo.yres = pfb_chn->fb_dev.uiHeight;   

    s32Ret = ioctl(pfb_chn->fd, FBIOPUT_VSCREENINFO, &stVarInfo);
    if (s32Ret < 0)
    {
        SAL_ERROR("PUT_VSCREENINFO failed s32Ret = 0x%x !\n",s32Ret);
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }

    if (ioctl(pfb_chn->fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
        SAL_ERROR("FBIOGET_FSCREENINFO failed!\n");
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }
    
    //OT_U32 u32FixScreenStride = fix.line_length;   /*fix screen stride*/

    pfb_chn->fb_dev.pShowScreen = mmap(TD_NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, pfb_chn->fd, 0);
    if (MAP_FAILED == pfb_chn->fb_dev.pShowScreen)
    {
        SAL_ERROR("mmap framebuffer failed!\n");
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }
    pfb_chn->fb_dev.ui64CanvasAddr  = fix.smem_start;
    pfb_chn->fb_dev.uiSmem_len      = fix.smem_len;

    pfb_chn->fb_dev.pBuf = NULL;
    
    memset(pfb_chn->fb_dev.pShowScreen, 0x0, fix.smem_len);

    /* time to play*/
    bShow = TD_TRUE;
    if (ioctl(pfb_chn->fd, FBIOPUT_SHOW_GFBG, &bShow) < 0)
    {
        SAL_ERROR("FBIOPUT_SHOW_GFBG failed!\n");
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }
    
    SAL_INFO("INIT fb %d success(%p,len %d)\n",uiChn, pfb_chn->fb_dev.pShowScreen, pfb_chn->fb_dev.uiSmem_len);

    return SAL_SOK;
}

/**
 * @function   fb_ssv5DeleteFb
 * @brief      释放/销毁显示FB
 * @param[in]  UINT32 uiChn                   fb设备号
 * @param[in]  FB_COMMON *pfb_chn             fb设备信息
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ssv5DeleteFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{
    INT32 s32Ret = 0;

    if(FB_DEV_NUM_MAX <= uiChn)
    {
        SAL_ERROR("uiChn is err, uiChn = %d\n",uiChn);
        return SAL_FAIL;
    }
    
    if(uiChn == GRAPHICS_LAYER_G0 || uiChn == GRAPHICS_LAYER_G1)
    {
        if( pfb_chn->fb_dev.ui64CanvasAddr != 0x00)
        {            
            SAL_INFO("UMAP Fb %d OT_MPI_SYS_MmzFree !\n",uiChn);

            s32Ret = ss_mpi_sys_mmz_free(pfb_chn->fb_dev.ui64CanvasAddr, pfb_chn->fb_dev.pBuf);
            if(TD_SUCCESS != s32Ret)
            {
                SAL_ERROR("chan %d error %x\n",uiChn,s32Ret);
                return SAL_FAIL;
            }
        }
    }
    else if(uiChn == GRAPHICS_LAYER_G3)
    {   
        if( pfb_chn->fb_dev.pShowScreen)
        {
            munmap(pfb_chn->fb_dev.pShowScreen, pfb_chn->fb_dev.uiSmem_len);
        }

    }

    pfb_chn->fb_dev.ui64CanvasAddr = 0x00;

    pfb_chn->fb_dev.pBuf = NULL;

    if (ioctl(pfb_chn->fd, FBIO_DESTROY_LAYER) < 0)
    {
        SAL_ERROR("uiChn %d,FBIO_DESTROY_LAYER error(%s)!\n",uiChn,strerror(errno));
        close(pfb_chn->fd);
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }

    if(close(pfb_chn->fd) < 0)
    {
       SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
       close(pfb_chn->fd);
    } 
    pfb_chn->fd = -1;
    SAL_INFO("fb_ssv5DeleteFb Fb %d Success !\n",uiChn);

    return SAL_SOK;
}

/**
 * @function   fb_ssv5CreateDevFb
 * @brief      创建普通显示设备
 * @param[in]  UINT32 uiChn                   fb设备号
 * @param[in]  FB_COMMON *pfb_chn             fb设备信息
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ssv5CreateDevFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{   
    INT32                     s32Ret       = TD_SUCCESS;
    BOOL                    bShow        = TD_TRUE;
    ot_fb_colorkey            stColorKey   = {0};
    ot_fb_layer_info          stLayerInfo  = {0};
    struct fb_var_screeninfo   stVarInfo;
    ot_vo_layer s32Layer = -1;
    ot_vo_csc stCSC = {0};

    sprintf((char *)pfb_chn->fbName, "/dev/fb%d", uiChn);

    /*************************************
    * 1. open framebuffer device overlay 0
    ****************************************/
    pfb_chn->fd = open((char *)pfb_chn->fbName, O_RDWR, 0);
    if (pfb_chn->fd < 0)
    {
        SAL_ERROR("open %s failed!\n", pfb_chn->fbName);
        return SAL_FAIL;
    }
   SAL_INFO("fb %d chn %d\n",pfb_chn->fd,uiChn);

    if (uiChn == GRAPHICS_LAYER_G1)
    {
        s32Layer = uiChn + OT_VO_MAX_PHYS_VIDEO_LAYER_NUM;
        s32Ret = ss_mpi_vo_get_layer_csc(s32Layer, &stCSC);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_ERROR("OT_MPI_VO_GetVideoLayerCSC failed!s32Ret 0x%x \n",s32Ret);
            if(close(pfb_chn->fd) < 0)
            {
               SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
               close(pfb_chn->fd);
            } 
            pfb_chn->fd = -1;
            return SAL_FAIL;
        }
        
      //  stCSC.csc_matrix = VO_CSC_MATRIX_IDENTITY;   /*lxy 20210805 没有该枚举类型*/
        stCSC.luma = 80;
        stCSC.contrast = 40;
        s32Ret = ss_mpi_vo_set_layer_csc(s32Layer, &stCSC);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_ERROR("OT_MPI_VO_SetVideoLayerCSC failed!s32Ret 0x%x \n",s32Ret);
            if(close(pfb_chn->fd) < 0)
            {
               SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
               close(pfb_chn->fd);
            } 
            pfb_chn->fd = -1;
            return SAL_FAIL;
        }
    }

    /*all layer surport colorkey*/
    stColorKey.enable = TD_FALSE;
    stColorKey.value = 0x0;
    if (ioctl(pfb_chn->fd, FBIOPUT_COLORKEY_GFBG, &stColorKey) < 0)
    {
        SAL_ERROR("FBIOPUT_COLORKEY_GFBG!\n");
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }

    s32Ret = ioctl(pfb_chn->fd, FBIOGET_VSCREENINFO, &stVarInfo);
    if (s32Ret < 0)
    {
        SAL_ERROR("GET_VSCREENINFO failed!\n");
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }


    switch (pfb_chn->bitWidth)
    {
        case 32:
        {
            stVarInfo.transp = s_a32;
            stVarInfo.red    = s_r32;
            stVarInfo.green  = s_g32;
            stVarInfo.blue   = s_b32;
            stVarInfo.bits_per_pixel = 32;
            break;
        }
        case 16:
        {
            stVarInfo.transp = s_a16;
            stVarInfo.red    = s_r16;
            stVarInfo.green  = s_g16;
            stVarInfo.blue   = s_b16;
            stVarInfo.bits_per_pixel = 16;
            break;
        }
        default:
        {
            stVarInfo.transp = s_a32;
            stVarInfo.red    = s_r32;
            stVarInfo.green  = s_g32;
            stVarInfo.blue   = s_b32;            
            stVarInfo.bits_per_pixel = 32;
            break;
        }
    }

    pfb_chn->u32BytePerPixel = stVarInfo.bits_per_pixel/8;
    
    SAL_INFO("u32BytePerPixel %d bits_per_pixel %d \n", pfb_chn->u32BytePerPixel, stVarInfo.bits_per_pixel);

    stVarInfo.activate = FB_ACTIVATE_NOW;
    stVarInfo.xres     = stVarInfo.xres_virtual = pfb_chn->fb_dev.uiWidth;
    stVarInfo.yres     = stVarInfo.yres_virtual = pfb_chn->fb_dev.uiHeight;
    s32Ret = ioctl(pfb_chn->fd, FBIOPUT_VSCREENINFO, &stVarInfo);
    if (s32Ret < 0)
    {
        SAL_ERROR("PUT_VSCREENINFO failed!\n");
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }


    #if 1
    stLayerInfo.buf_mode = OT_FB_LAYER_BUF_NONE;
    stLayerInfo.mask = OT_FB_LAYER_MASK_BUF_MODE;
    #else
    stLayerInfo.buf_mode = OT_FB_LAYER_BUF_DOUBLE_IMMEDIATE;    
    stLayerInfo.mask = OT_FB_LAYER_MASK_BUF_MODE;
    #endif
    s32Ret = ioctl(pfb_chn->fd, FBIOPUT_LAYER_INFO, &stLayerInfo);
    if (s32Ret < 0)
    {
        SAL_ERROR("PUT_LAYER_INFO failed!\n");
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }
    bShow = TD_TRUE;
    if (ioctl(pfb_chn->fd, FBIOPUT_SHOW_GFBG, &bShow) < 0)
    {
        SAL_ERROR("FBIOPUT_SHOW_GFBG failed!\n");
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }

    /* DSP buff  */
    s32Ret = fb_ssv5MallocFb(uiChn, pfb_chn);
    if(s32Ret != SAL_SOK)
    {                                    
        SAL_ERROR("fb_ssv5MallocFb failed !!\n");
    
        s32Ret = fb_ssv5DeleteFb(uiChn, pfb_chn);
        if(s32Ret != SAL_SOK)
        {
            SAL_ERROR("fb_ssv5DeleteFb fb %d fail !\n",uiChn);                
            //return SAL_FAIL;
        }
    
        return SAL_FAIL;
    }
        
    SAL_INFO("Dev fb %d Init Success\n",uiChn);

    return SAL_SOK;
}

/**
 * @function   fb_ssv5CreateAppFb
 * @brief      创建给应用的显示缓存
 * @param[in]  UINT32 uiChn                   fb设备号
 * @param[in]  FB_COMMON *pfb_chn             fb设备信息
 * @param[in]  SCREEN_ATTR *pstHalFbAtt       fb坐标区域
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ssv5CreateAppFb(UINT32 uiChn,FB_COMMON *pfb_chn, SCREEN_ATTR *pstHalFbAttr)
{
    UINT32 uiSize = 0;

    if(uiChn >= FB_DEV_NUM_MAX)
    {
        SAL_ERROR("uiChn not exist, chan = %d!!\n", uiChn);
        return SAL_FAIL;
    }
    
    pfb_chn->fb_app.uiWidth   = pstHalFbAttr->width;
    pfb_chn->fb_app.uiHeight  = pstHalFbAttr->height;
        
    uiSize = pfb_chn->fb_app.uiWidth * pfb_chn->fb_app.uiHeight * pfb_chn->u32BytePerPixel;

    if((GRAPHICS_LAYER_G3 != uiChn) || (pfb_chn->AppMMAP == SAL_TRUE))
    {
        uiSize = SAL_align(uiSize ,16);
    }
    else
    {
        if(uiSize == 0x00)
        {
             SAL_ERROR("uiSize %d!!\n", uiSize);
             return SAL_FAIL;
         }
    }

    SAL_INFO("MMAP : App Get Buff uiWidth %d uiHeight %d size %d !!!\n", pfb_chn->fb_app.uiWidth, pfb_chn->fb_app.uiHeight,uiSize);

    if (TD_FAILURE == ss_mpi_sys_mmz_alloc((td_phys_addr_t *)&pfb_chn->fb_app.ui64Phyaddr, ((void**)&pfb_chn->fb_app.Viraddr), NULL, NULL, uiSize))
    {
        SAL_ERROR("OT_MPI_SYS_MmzAlloc error uiSize %d  failed\n",uiSize);
        (void)ss_mpi_sys_mmz_free(pfb_chn->fb_dev.ui64CanvasAddr, pfb_chn->fb_dev.pBuf);
        pfb_chn->fb_dev.ui64CanvasAddr = 0;
        if(close(pfb_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
           close(pfb_chn->fd);
        } 
        pfb_chn->fd = -1;
        return SAL_FAIL;
    }

    memset(pfb_chn->fb_app.Viraddr, 0x00, uiSize);
    pfb_chn->fb_app.uiSize = uiSize;
    pstHalFbAttr->srcAddr  = pfb_chn->fb_app.Viraddr;
    pstHalFbAttr->srcSize  = uiSize;
    pstHalFbAttr->isUse    = 1;
    
    SAL_INFO("FB%d App Buff MMZ ADDR : %lu SIZE : 0x%x !!!\n",uiChn, (UINTL)pfb_chn->fb_app.Viraddr, uiSize);

    return SAL_SOK;
}

/**
 * @function   fb_ssv5DeleteAppFb
 * @brief      释放给应用的显示缓存
 * @param[in]  UINT32 uiChn                   fb设备号
 * @param[in]  FB_COMMON *pfb_chn             fb设备信息
 * @param[in]  SCREEN_ATTR *pstHalFbAtt       fb坐标区域
 * @param[out] None
 * @return     static INT32 SAL_SOK  成功
 *                          SAL_FAIL 失败
 */
static INT32 fb_ssv5DeleteAppFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{
    INT32 s32Ret = 0;

    if(FB_DEV_NUM_MAX <= uiChn)
    {
        SAL_ERROR("uiChn is err, uiChn = %d\n",uiChn);
        return SAL_FAIL;
    }
    
    if(pfb_chn->fb_app.ui64Phyaddr)
    {
        s32Ret = ss_mpi_sys_mmz_free(pfb_chn->fb_app.ui64Phyaddr, pfb_chn->fb_app.Viraddr);    
        if(TD_SUCCESS != s32Ret)
        {
            SAL_ERROR("chan %d error %x\n",uiChn,s32Ret);
            return SAL_FAIL;
        }
    }
    
    pfb_chn->fb_app.ui64Phyaddr = 0;
    
    return SAL_SOK;
}


/**
 * @function   fb_ssv5SetMouseFbChn
 * @brief      切换鼠标通道绑定到对用VO设备通道
 * @param[in]  UINT32 uiChn        设备号
 * @param[in]  UINT32 uiBindChn    绑定的设备号
 * @param[in]  FB_COMMON *pfb_chn  设备信息
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_ssv5SetMouseFbChn(UINT32 uiChn, UINT32 uiBindChn, FB_COMMON *pfb_chn)
{
    
    INT32                     s32Ret       = TD_SUCCESS;
    BOOL                    bShow        = TD_TRUE;
    UINT32                     sleTime      = 100; 
    struct fb_fix_screeninfo fix = {0};
    struct fb_var_screeninfo stVarInfo = {0};
    FB_COMMON *pfb3_chn = &g_fb_common[GRAPHICS_LAYER_G3];      /*默认G3为鼠标*/
    FB_COMMON *pfb0_chn = pfb_chn;   
    ot_vo_layer s32Layer = OT_VO_LAYER_G3;
    ot_vo_csc stCSC = {0};
    
    if (uiBindChn == pfb3_chn->mouseLastChn)
    {
        SAL_ERROR("uiBindChn %d is sam\n",pfb3_chn->mouseLastChn);
        return SAL_SOK;
    }
    
    pfb3_chn->mouseNewChn = uiBindChn; 

    /*鼠标必须在UI的FB MAP后才允许MAP*/
    if(pfb0_chn->DevMMAP != SAL_TRUE && pfb0_chn->AppMMAP != SAL_TRUE)
    {
        SAL_ERROR("Fb0 is Umap !\n");
        return SAL_FAIL;
    }

    if( pfb3_chn->fb_dev.pShowScreen)
    {
        munmap(pfb3_chn->fb_dev.pShowScreen, pfb3_chn->fb_dev.uiSmem_len);
    }

    pfb3_chn->fb_dev.ui64CanvasAddr = 0x00;

    if (pfb3_chn->fd >= 0)
    {
        if (ioctl(pfb3_chn->fd, FBIO_DESTROY_LAYER) < 0)
        {
            SAL_ERROR("GRAPHICS_LAYER_G3->FBIO_DESTROY_LAYER error(%s)!\n",strerror(errno));
            //close是异步的先等待100毫秒
            usleep(sleTime * 1000);
            if (ioctl(pfb3_chn->fd, FBIO_DESTROY_LAYER) < 0)
            {
                SAL_ERROR("GRAPHICS_LAYER_G3->FBIO_DESTROY_LAYER error(%s)!\n",strerror(errno));
            }
        }
        SAL_WARN("chn %d,fd %d!\n", GRAPHICS_LAYER_G3,pfb3_chn->fd);
        if(close(pfb3_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",GRAPHICS_LAYER_G3,pfb3_chn->fd,strerror(errno));
           close(pfb3_chn->fd);
        }

        pfb3_chn->fdcopy = pfb3_chn->fd;
        pfb3_chn->fd = -1;
    }
    else
    {
        SAL_WARN("chan %d fd %d(have close)\n",GRAPHICS_LAYER_G3,pfb3_chn->fd);
    }
     
    s32Ret = ss_mpi_vo_unbind_layer(s32Layer, pfb3_chn->mouseLastChn);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_ERROR("UnBinds32Layer failed with 0x%x!,mouseLastChn %d,mouseNewChn %d,fdcopy %d\n", s32Ret,pfb3_chn->mouseLastChn,pfb3_chn->mouseNewChn,pfb3_chn->fdcopy);
        if (pfb3_chn->fdcopy > 0)
        {
            if(close(pfb3_chn->fdcopy) < 0)
            {
               SAL_ERROR("chan %d fd %d,errno %s\n",OT_VO_LAYER_G3,pfb3_chn->fd,strerror(errno));
               close(pfb3_chn->fdcopy);
            } 
        }
        
        ss_mpi_vo_unbind_layer(s32Layer, pfb3_chn->mouseLastChn);
        //return s32Ret;
    }

    s32Ret = ss_mpi_vo_bind_layer(s32Layer, pfb3_chn->mouseNewChn);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_ERROR("Binds32Layer failed with 0x%x!\n", s32Ret);
        return s32Ret;
    }
    
    pfb3_chn->mouseLastChn    = pfb3_chn->mouseNewChn;
    SAL_INFO("pfb_chn->fb_dev.uiWidth %d pfb_chn->fb_dev.uiHeight %d mouseNewChn %d\n", pfb3_chn->fb_dev.uiWidth, pfb3_chn->fb_dev.uiHeight,pfb3_chn->mouseNewChn);

    sprintf((char *)pfb3_chn->fbName, "/dev/fb%d", GRAPHICS_LAYER_G3);

    /*************************************
    * 1. open framebuffer device overlay 0
    ****************************************/

    pfb3_chn->fd = open((char *)pfb3_chn->fbName, O_RDWR, 0);
    if (pfb3_chn->fd < 0)
    {
        SAL_ERROR("open %s failed!\n", pfb3_chn->fbName);
        return SAL_FAIL;
    }

    pfb3_chn->fdcopy = pfb3_chn->fd;
    SAL_WARN("open chn %d,fd %d!\n", GRAPHICS_LAYER_G3,pfb3_chn->fd);

    if (pfb3_chn->mouseNewChn == GRAPHICS_LAYER_G1)
    {
        s32Ret = ss_mpi_vo_get_layer_csc(s32Layer, &stCSC);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_ERROR("OT_MPI_VO_GetVideoLayerCSC failed!s32Ret 0x%x \n",s32Ret);
            if(close(pfb3_chn->fd) < 0)
            {
               SAL_ERROR("chan %d fd %d,errno %s\n",GRAPHICS_LAYER_G3,pfb3_chn->fd,strerror(errno));
               close(pfb3_chn->fd);
            } 
            pfb3_chn->fd = -1;
            return SAL_FAIL;
        }
        
       // stCSC.csc_matrix = VO_CSC_MATRIX_IDENTITY; /*lxy 没有VO_CSC_MATRIX_IDENTITY枚举类型*/
        stCSC.luma = 80;
        stCSC.contrast = 40;
        s32Ret = ss_mpi_vo_set_layer_csc(s32Layer, &stCSC);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_ERROR("OT_MPI_VO_SetVideoLayerCSC failed!s32Ret 0x%x \n",s32Ret);
            if(close(pfb3_chn->fd) < 0)
            {
               SAL_ERROR("chan %d fd %d,errno %s\n",GRAPHICS_LAYER_G3,pfb3_chn->fd,strerror(errno));
               close(pfb3_chn->fd);
            } 
            pfb3_chn->fd = -1;
            return SAL_FAIL;
        }
    }


    bShow = TD_FALSE;
    if (ioctl(pfb3_chn->fd, FBIOPUT_SHOW_GFBG, &bShow) < 0)
    {
        SAL_ERROR("FBIOPUT_SHOW_GFBG!\n");
        if(close(pfb3_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",GRAPHICS_LAYER_G3,pfb3_chn->fd,strerror(errno));
           close(pfb3_chn->fd);
        } 
        pfb3_chn->fd = -1;
        return SAL_FAIL;
    }
    
    s32Ret = ioctl(pfb3_chn->fd, FBIOGET_VSCREENINFO, &stVarInfo);
    if (s32Ret < 0)
    {
        SAL_ERROR("GET_VSCREENINFO failed!\n");
        if(close(pfb3_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",GRAPHICS_LAYER_G3,pfb3_chn->fd,strerror(errno));
           close(pfb3_chn->fd);
        } 
        pfb3_chn->fd = -1;
        return SAL_FAIL;
    }

    switch (pfb3_chn->bitWidth)
    {
        case 32:
        {
            stVarInfo.transp = s_a32;
            stVarInfo.red    = s_r32;
            stVarInfo.green  = s_g32;
            stVarInfo.blue   = s_b32;            
            stVarInfo.bits_per_pixel = 32;
            break;
        }
        case 16:
        {
            stVarInfo.transp = s_a16;
            stVarInfo.red    = s_r16;
            stVarInfo.green  = s_g16;
            stVarInfo.blue   = s_b16;
            stVarInfo.bits_per_pixel = 16;
            break;
        }
        default:
        {
            stVarInfo.transp = s_a32;
            stVarInfo.red    = s_r32;
            stVarInfo.green  = s_g32;
            stVarInfo.blue   = s_b32;            
            stVarInfo.bits_per_pixel = 32;
            break;
        }
    }

    pfb3_chn->u32BytePerPixel = stVarInfo.bits_per_pixel/8;
    
    SAL_INFO("u32BytePerPixel %d bits_per_pixel %d \n", pfb3_chn->u32BytePerPixel, stVarInfo.bits_per_pixel);

    stVarInfo.activate = FB_ACTIVATE_NOW;
    stVarInfo.xres_virtual = pfb3_chn->fb_dev.uiWidth;
    stVarInfo.yres_virtual = pfb3_chn->fb_dev.uiHeight;   
    
    stVarInfo.xres = pfb3_chn->fb_dev.uiWidth;
    stVarInfo.yres = pfb3_chn->fb_dev.uiHeight;   

    s32Ret = ioctl(pfb3_chn->fd, FBIOPUT_VSCREENINFO, &stVarInfo);
    if (s32Ret < 0)
    {
        SAL_ERROR("PUT_VSCREENINFO failed s32Ret = 0x%x !\n",s32Ret);
        if(close(pfb3_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",GRAPHICS_LAYER_G3,pfb3_chn->fd,strerror(errno));
           close(pfb3_chn->fd);
        } 
        pfb3_chn->fd = -1;
        return SAL_FAIL;
    }

    if (ioctl(pfb3_chn->fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
        SAL_ERROR("FBIOGET_FSCREENINFO failed!\n");
        if(close(pfb3_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",GRAPHICS_LAYER_G3,pfb3_chn->fd,strerror(errno));
           close(pfb3_chn->fd);
        } 
        pfb3_chn->fd = -1;
        return SAL_FAIL;
    }

    pfb3_chn->fb_dev.pShowScreen = mmap(TD_NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, pfb3_chn->fd, 0);
    if (MAP_FAILED == pfb3_chn->fb_dev.pShowScreen)
    {
        SAL_ERROR("mmap framebuffer failed!\n");
        if(close(pfb3_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",GRAPHICS_LAYER_G3,pfb3_chn->fd,strerror(errno));
           close(pfb3_chn->fd);
        } 
        pfb3_chn->fd = -1;
        return SAL_FAIL;
    }
    pfb3_chn->fb_dev.ui64CanvasAddr  = fix.smem_start;
    pfb3_chn->fb_dev.uiSmem_len      = fix.smem_len;

    pfb3_chn->fb_dev.pBuf = NULL;
    
    memset(pfb3_chn->fb_dev.pShowScreen, 0x0, fix.smem_len);

    /* time to play*/
    bShow = TD_TRUE;
    if (ioctl(pfb3_chn->fd, FBIOPUT_SHOW_GFBG, &bShow) < 0)
    {
        SAL_ERROR("FBIOPUT_SHOW_GFBG failed!\n");
        if(close(pfb3_chn->fd) < 0)
        {
           SAL_ERROR("chan %d fd %d,errno %s\n",GRAPHICS_LAYER_G3,pfb3_chn->fd,strerror(errno));
           close(pfb3_chn->fd);
        } 
        pfb3_chn->fd = -1;
        return SAL_FAIL;
    }
    
    SAL_INFO("INIT fb %d success(%p,len %d)\n",GRAPHICS_LAYER_G3, pfb3_chn->fb_dev.pShowScreen, pfb3_chn->fb_dev.uiSmem_len);

    return SAL_SOK;
}

/**
 * @function   fb_ssv5Refresh
 * @brief      刷新图形层界面
 * @param[in]  UINT32 uiChn               设备号
 * @param[in]  FB_COMMON *pfb_chn         fb设备信息
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  显示坐标
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ssv5Refresh(UINT32 uiChn,FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr)
{
    INT32 s32Ret = SAL_FAIL;

    if(FB_DEV_NUM_MAX <= uiChn)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }

    if(pfb_chn == NULL)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }

    if(pfb_chn->AppMMAP != SAL_TRUE || pfb_chn->DevMMAP != SAL_TRUE)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }
    
    if(uiChn == GRAPHICS_LAYER_G0 || uiChn == GRAPHICS_LAYER_G1)
    {
        s32Ret = fb_ssv5RefreshFb(uiChn, pfb_chn);
        if(s32Ret != SAL_SOK)
        {
            SAL_ERROR("error\n");
            return SAL_FAIL;
        }
    }

          
    return SAL_SOK;
}

/**
 * @function   fb_ssv5SetOrgin
 * @brief      设置显示启始坐标
 * @param[in]  UINT32 uiChn               
 * @param[in]  FB_COMMON *pfb_chn         
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ssv5SetOrgin(UINT32 uiChn,FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr)
{
    ot_fb_point stPoint = {0, 0};

    if(FB_DEV_NUM_MAX <= uiChn)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }

    if(pfb_chn == NULL)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }

    if(pfb_chn->AppMMAP != SAL_TRUE || pfb_chn->DevMMAP != SAL_TRUE)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }

    if(uiChn == GRAPHICS_LAYER_G3)
    {    
        if(NULL == pstHalFbAttr)
        {
            SAL_ERROR("error\n");
            return SAL_FAIL;
        }
        
        stPoint.x_pos = pstHalFbAttr->stScreenAttr[0].x;
        stPoint.y_pos = pstHalFbAttr->stScreenAttr[0].y;
        if (ioctl(pfb_chn->fd, FBIOPUT_SCREEN_ORIGIN_GFBG, &stPoint) < 0)
        {
            SAL_ERROR("FBIOPUT_SCREEN_ORIGIN_GFBG!\n");
            if(close(pfb_chn->fd) < 0)
            {
               SAL_ERROR("chan %d fd %d,errno %s\n",uiChn,pfb_chn->fd,strerror(errno));
               close(pfb_chn->fd);
            } 
            pfb_chn->fd = -1;
            return SAL_FAIL;
        }
        
    }

     return SAL_SOK;
    
}

/**
 * @function   fb_ssv5CreateFb
 * @brief      创建显示设备
 * @param[in]  UINT32 uiChn               设备号
 * @param[in]  FB_COMMON *pfb_chn         设备信息
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  显示参数
 * @param[in]  FB_INIT_ATTR_ST *pstFbPrm  图形界面信息
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ssv5CreateFb(UINT32 uiChn, FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr, FB_INIT_ATTR_ST *pstFbPrm)
{

    INT32 i = 0;
    INT32 s32Ret = SAL_SOK;
    FB_INIT_ATTR_ST  stFbHalMmapAttr;
    memset(&stFbHalMmapAttr, 0, sizeof(FB_INIT_ATTR_ST));

    /* 获取显示设备设置的分辨率 */      
    if(pstFbPrm == NULL)
    {
        SAL_ERROR("error !!\n");
        return SAL_FAIL;
    }

    for(i = 0; i < pstHalFbAttr->uiCnt; i++)
    {        
        stFbHalMmapAttr.uiW      = pstHalFbAttr->stScreenAttr[i].width;
        stFbHalMmapAttr.uiH      = pstHalFbAttr->stScreenAttr[i].height;      
        pfb_chn->bitWidth        = pstHalFbAttr->stScreenAttr[i].bitWidth;
        
        SAL_INFO("MMAP: New App Fb %d (%d x %d ) bitWidth %d !\n",uiChn,stFbHalMmapAttr.uiW,stFbHalMmapAttr.uiH,pfb_chn->bitWidth);
        
       if(stFbHalMmapAttr.uiW == 0x00 || stFbHalMmapAttr.uiH == 0x00 || pfb_chn->bitWidth == 0x00)
       {
          SAL_ERROR("error fb w(%d) h(%d) !!\n",stFbHalMmapAttr.uiW,stFbHalMmapAttr.uiH);
          return SAL_FAIL;
       }
        
        SAL_INFO("MMAP: New Dev Fb %d (%d x %d ) Already exist fb %d (%d x %d )\n",
          uiChn,pstFbPrm->uiW,pstFbPrm->uiH,
          uiChn,pfb_chn->fb_dev.uiWidth,pfb_chn->fb_dev.uiHeight);

         if(pfb_chn->DevMMAP == SAL_TRUE)
        {                
            if(pfb_chn->fb_dev.uiWidth != pstFbPrm->uiW || pfb_chn->fb_dev.uiHeight != pstFbPrm->uiH)
            {
                SAL_WARN("MMAP: Dev Fb %d Already exist,UMAP first !\n",uiChn);
                
                s32Ret = fb_ssv5DeleteFb(uiChn, pfb_chn);
                if(s32Ret != SAL_SOK)
                {
                    SAL_ERROR("fb_ssv5DeleteFb fb %d fail !\n",uiChn);                
                    return SAL_FAIL;
                }
                
                pfb_chn->DevMMAP = SAL_FALSE;
            }
            else
            {
                SAL_WARN("MMAP: Dev Fb %d Already MMAP !\n",uiChn);
            }
        }
        
        pfb_chn->fb_dev.uiWidth  = pstFbPrm->uiW;
        pfb_chn->fb_dev.uiHeight = pstFbPrm->uiH;
        
        if(pfb_chn->DevMMAP == SAL_FALSE)
        {   
            /* fb ui */
            if(uiChn == GRAPHICS_LAYER_G0 || uiChn == GRAPHICS_LAYER_G1) /* fb ui */
            {
                /* vo init */
                s32Ret = fb_ssv5CreateDevFb(uiChn,  pfb_chn);
                if(SAL_SOK != s32Ret)
                {
                    SAL_ERROR("fb_ssv5CreateDevFb failed !!\n");
                    return SAL_FAIL;
                }
    
            }
            else if(uiChn == GRAPHICS_LAYER_G3) /* 鼠标 */
            {                
                /*是否合理默认写0*/
                pfb_chn->mouseNewChn = pstHalFbAttr->stScreenAttr[0].mouseBindChn;
                s32Ret = fb_ssv5CreateDevFb3(uiChn, pfb_chn,  &stFbHalMmapAttr);
                if(SAL_SOK != s32Ret)
                {
                    SAL_ERROR("fb_ssv5CreateDevFb3 failed !!\n");
                    return SAL_FAIL;
                }
                
            }
                                  
            pfb_chn->DevMMAP = SAL_TRUE;
        }
    
        if(pfb_chn->AppMMAP == SAL_TRUE)
        {   
            SAL_INFO("MMAP: New App Fb %d (%d x %d ) srcAddr %lu srcSize %d , Already exist Fb %d (%d x %d ) Viraddr %lu uiSize 0x%x !\n",
                uiChn,stFbHalMmapAttr.uiW,stFbHalMmapAttr.uiH,(UINTL)pstHalFbAttr->stScreenAttr[i].srcAddr,pstHalFbAttr->stScreenAttr[i].srcSize,
                uiChn,pfb_chn->fb_app.uiWidth,pfb_chn->fb_app.uiHeight,(UINTL)pfb_chn->fb_app.Viraddr,pfb_chn->fb_app.uiSize);
    
            if(stFbHalMmapAttr.uiW != pfb_chn->fb_app.uiWidth || stFbHalMmapAttr.uiH != pfb_chn->fb_app.uiHeight)
            {
                SAL_WARN("MMAP: App Fb %d , Already exist,UMAP first !\n",uiChn);
                
                s32Ret = fb_ssv5DeleteAppFb(uiChn, pfb_chn);
                if(s32Ret != SAL_SOK)
                {
                    SAL_ERROR("UMAP: Already exist fb %d fail !\n",uiChn);                
                    return SAL_FAIL;
                }
                
                pfb_chn->AppMMAP = SAL_FALSE;
            }
            else
            {                
                SAL_WARN("MMAP: App Fb %d Already MMAP !\n",uiChn);
                
                pstHalFbAttr->stScreenAttr[i].srcAddr = pfb_chn->fb_app.Viraddr;
                pstHalFbAttr->stScreenAttr[i].srcSize = pfb_chn->fb_app.uiSize;
            }
        }
        
        if(pfb_chn->AppMMAP == SAL_FALSE)
        {             
                /* UI fb */
            s32Ret = fb_ssv5CreateAppFb(uiChn, pfb_chn, &pstHalFbAttr->stScreenAttr[i]);
            if(SAL_SOK != s32Ret)
            {                   
                SAL_ERROR("fb_ssv5CreateAppFb failed !!\n");
    
                s32Ret = fb_ssv5DeleteFb(uiChn, pfb_chn);
                if(s32Ret != SAL_SOK)
                {
                    SAL_ERROR("fb_ssv5DeleteFb fb %d fail !\n",uiChn);                
                    return SAL_FAIL;
                }
                
                pfb_chn->DevMMAP = SAL_FALSE;
    
                return SAL_FAIL;
            }                        
            
            pfb_chn->AppMMAP = SAL_TRUE;            
        }
    }
    return SAL_SOK;
}


/**
 * @function   fb_ssv5ReleasFb
 * @brief      释放显示设备
 * @param[in]  UINT32 uiChn        设备号
 * @param[in]  FB_COMMON *pfb_chn  设备信息
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_ssv5ReleasFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{
    INT32 s32Ret = 0;

    if(pfb_chn->DevMMAP == SAL_TRUE)
    {
        s32Ret = fb_ssv5DeleteFb(uiChn, pfb_chn);
        if(SAL_SOK != s32Ret)
        {
            SAL_ERROR("error \n");
            return SAL_FAIL;
        }

        pfb_chn->DevMMAP = SAL_FALSE;
        
        SAL_INFO("Dev Fb %d Umap Success !\n",uiChn);
    }
    else
    {
        SAL_WARN("Dev Fb %d Already Umap !\n",uiChn);
    }

    if(pfb_chn->AppMMAP == SAL_TRUE)
    {
        s32Ret = fb_ssv5DeleteAppFb(uiChn, pfb_chn);
        if(SAL_SOK != s32Ret)
        {
            SAL_ERROR("error \n");
            return SAL_FAIL;
        }
        
        pfb_chn->AppMMAP = SAL_FALSE;

        SAL_INFO("App Fb %d Umap Success !\n",uiChn);
    }
    else
    {
    
        SAL_WARN("App Fb %d Already umap !\n",uiChn);
    }    
        
    return SAL_SOK;
}

/**
 * @function   fb_ssv5GetFbNum
 * @brief      获取设备最大显示设备数
 * @param[in]  void  
 * @param[out] None
 * @return     static INT32 FB_DEV_NUM_MAX 设备数量
 *                      
 */
static INT32 fb_ssv5GetFbNum(void)
{
    return FB_DEV_NUM_MAX;
}

/**
 * @function   fb_halRegister
 * @brief      初始化fb成员函数
 * @param[in]  void  
 * @param[out] None
 * @return     FB_PLAT_OPS * 返回函数结构体指针
 */
FB_PLAT_OPS *fb_halRegister(void)
{
    memset(&g_stFbPlatOps, 0,sizeof(FB_PLAT_OPS));

    g_stFbPlatOps.createFb   = fb_ssv5CreateFb;
    g_stFbPlatOps.releaseFb  = fb_ssv5ReleasFb;
    g_stFbPlatOps.refreshFb  = fb_ssv5Refresh;
    g_stFbPlatOps.setOrgin  =  fb_ssv5SetOrgin;
    g_stFbPlatOps.setFbChn   = fb_ssv5SetMouseFbChn;
    g_stFbPlatOps.getFbNum   = fb_ssv5GetFbNum;

    return &g_stFbPlatOps;
    
}


