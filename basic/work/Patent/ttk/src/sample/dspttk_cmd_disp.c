/**
 * @file    dspttk_cmd_disp.c
 * @brief
 * @note
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_devcfg.h"
#include "dspttk_init.h"
#include "dspttk_util.h"
#include "dspttk_cmd_disp.h"
#include "dspcommon.h"
#include "dspttk_fop.h"
#include "dspttk_cmd.h"
/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
#define GUI_CURSOR_BGRA ("/cursor_w32_h32.bgra")    /* bgra格式鼠标图片默认路径 */
#define GUI_ASCII_SRC_BMP ("/asc_src.bmp")          /* ascii码表bmp资源 */
#define GUI_ASCII_SRC_BGRA ("/asc_src.bgra")        /* ascii码表bgra资源 */
#define GUI_ASCII_SPACE (32)                        /* ascii码表"空格"的ascii编码, 第一个可显示的ascii字符 */
#define GUI_ASCII_TILDE (126)                       /* ascii码表"~"的ascii编码, 最后一个可显示的ascii字符*/
#define GUI_BIT_WIDTH (32)                          /* GUI图片位宽 */
#define MAX_FILENAME_SIZE (256)                     /* 最大文件名长度 */
#define GUI_BGRA_BIT_WIDTH (4)                      /* bgra格式图片每个像素点占四个字节 */
#define GUI_BMP_BIT_WIDTH (3)                       /* BMP格式图片每个像素点占三个字节 */
#define BMP_PIC_HEAD_SIZE (54)                      /* BMP数据头所占字节个数 */
#define ASCII_SINGLE_ELEMENT_SIZE (20 * 15 * 4)     /* 每个元素占最大字节数 (每个像素占4个字节, 每个元素最大由20*15个像素点组成) */
#define BG_PIXEL_COLOR (0xFF20262C)                 /* BGRA图片菜单栏背景色由0x2C 0x26 0x20 0xFF四个字节组成一个像素点 */
#define BG_LINE_PIXEL_COLOR (0x64)                  /* 分割线颜色 */
#define BG_SINGLE_ELEMENT_MAX_WIDTH (15)            /* 单个元素最大宽度 */
#define BG_SINGLE_ELEMENT_MAX_HEIGHT (20)           /* 单个元素最大高度 */
#define BG_ASCII_AVERAGE_WIDTH (7)                  /* ascii表中元素的平均宽度 */
#define DISPLAY_DEVICE_WIDTH (1920)                 /* 显示器宽度 */
#define DISPLAY_DEVICE_HEIGHT (1080)                /* 显示器高度 */
#define MAX_DISPLAY_STRING_SIZE (40)                /* 最大字符串长度 */
#define DISPLAY_SPLIT_LINE_1 (600)                  /* 显示菜单栏中第一个分割线的横坐标 */
#define DISPLAY_SPLIT_LINE_2 (1600)                 /* 显示菜单栏中第二个分割线的横坐标 */
#define DISPLAY_GAP_WORD_SPLIT (20)                 /* 显示中字符分割单位(每20个像素点一个分割单位) */
#define DISPLAY_MENU_HEIGHT (1000)                  /* 显示菜单栏中的高度坐标 */
#define MAX_DISPLAY_ENLARGE_CHANNEL (8)             /* enlarge配置中最大可配置通道数 */
#define DISPLAY_MAX_STRING_CNT  (3)                 /* 显示菜单栏中每行最多的字符串个数为3 */
#define BMP_HEAD_INFO_WIDTH     (18)                /* bmp图片中宽度信息在图片数据中相对于首地址的偏移位置 */
#define BMP_HEAD_INFO_HEIGHT    (22)                /* bmp图片中高度信息在图片数据中相对于首地址的偏移位置 */
#define BMP_HEAD_INFO_WIDTH_HEIGHT_BITWIDTH     (4) /* bmp图片中宽高信息在图片数据中所占的位宽(4个字节) */

/* ASCII码单个元素控制信息 */
typedef struct DSPTTK_ELEMENT_FEATURE
{
    UINT32 u32Key;              //  ASCII码的值(例如空格为32)
    UINT32 u32Width;            //  在显示中所占像素宽度
    UINT32 u32WidthBegin;       //  bmp图中像素点横坐标的起始位置
    CHAR *pAddr;                //  bgra数据在内存中的起始地址
} DSPTTK_ELEMENT_FEATURE;

/* DSP库版本信息 */
typedef struct DSP_VERSION_PARAM
{
    CHAR szOrigin[80];                               //FUN_HOST_CMD_DSP_GET_VERSION 函数对应原始信息
    CHAR szBuildTime[MAX_DISPLAY_STRING_SIZE];       // 编译时间
    CHAR szDSPVersion[MAX_DISPLAY_STRING_SIZE];      // 版本号
    CHAR szBuilder[MAX_DISPLAY_STRING_SIZE];         // 编译者
    CHAR szCurrentTime[MAX_DISPLAY_STRING_SIZE];     // 当前时间
} DSP_VERSION_PARAM;

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
SCREEN_CTRL stScreenParam[MAX_VODEV_CNT] = {{0}, {0}}; // GUI FBshow和mmap使用

pthread_t g_FBInfoPthread = 0;      //用于菜单栏显示信息的线程

static CHAR g_szXspFunc[][10] = {"anti,", "ee :", "ue :", "lp,", "hp,", "le,", "absor :", "lum:", "pseudo :", "dispmode :"}; // 保存不同状态对应的字符串信息
static DSPTTK_ELEMENT_FEATURE stCtlAscElement[95] = {0};    //保存ASCII元素的控制信息
pthread_mutex_t gLockCtlAscElement;         // 对ASCII元素控制信息的锁

/* 保存xsp func状态对应的序号, 与g_szXspFunc序号对应 */
typedef enum EnXspFunc
{
    XSP_FUNC_ANTI = 0,
    XSP_FUNC_EE = 1,
    XSP_FUNC_UE = 2,
    XSP_FUNC_LP = 3,
    XSP_FUNC_HP = 4,
    XSP_FUNC_LE = 5,
    XSP_FUNC_ABSOR = 6,
    XSP_FUNC_LUM = 7,
    XSP_FUNC_PSEUDO = 8,
    XSP_FUNC_DISPMODE = 9
} EnXspFunc;

/**
 * @fn      dspttk_disp_is_useful_bg_pixel
 * @brief   判断当前像素点是否含有可用信息 (除背景色外的信息)
 *
 * @param   [IN] pSrcAsciiFile      源数据地址
 * @param   [IN] u32LineH           源数据在图片中的纵坐标(单位pixel)
 * @param   [IN] u32LineW           源数据在图片中的横坐标(单位pixel)
 * @param   [IN] u32PicWidth        源数据图片宽度
 * @return  含有有用信息:SAL_TRUE 不含有用信息:SAL_FALSE
 */
BOOL dspttk_disp_is_useful_bg_pixel(CHAR *pSrcAsciiFile, UINT32 u32LineH, UINT32 u32LineW, UINT32 u32PicWidth)
{
    UINT32 i = 0;
    CHAR ch = 0;
    CHAR szBG[3] = {(0xFF & BG_PIXEL_COLOR), ((0xFF << 16) & BG_PIXEL_COLOR) >> 16, ((0xFF << 8) & BG_PIXEL_COLOR) >> 8};   //背景色为0x2c 0x20 0x26

    if (NULL == pSrcAsciiFile)
    {
        PRT_INFO("pSrcAsciiFile is NULL\n");
        return SAL_FAIL;
    }

    for (i = 0; i < GUI_BMP_BIT_WIDTH; i++)
    {
        ch = *(pSrcAsciiFile + u32LineH * u32PicWidth * GUI_BMP_BIT_WIDTH + u32LineW * GUI_BMP_BIT_WIDTH + BMP_PIC_HEAD_SIZE + i);

        if (ch == szBG[0] || ch == szBG[1] || ch == szBG[2])
        {
            return SAL_FAIL;
        }
    }

    return SAL_TRUE;
}

/**
 * @fn      dspttk_get_element_feature
 * @brief   获取ascii码控制信息 (除在内存中地址外的所有信息, 对应DSPTTK_ELEMENT_FEATURE 结构体)
 *
 * @param   [IN] pSrcAsciiFile      源数据地址
 * @param   [IN] u32PicWidth        源数据图片宽度
 * @param   [IN] u32PicHeight       源数据图片高度
 * @return  成功:SAL_SOK 失败:SAL_FAIL
 */
SAL_STATUS dspttk_get_element_feature(CHAR *pSrcAsciiFile, UINT32 u32PicWidth, UINT32 u32PicHeight)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 u32Left = 0, u32Right = 0, i = 0, u32AsciiCnt = 1, u32Flag = 0, u32Width = 0, u32FileSize = 0;
    CHAR *pSrcBMPBuffer = NULL;

    if (NULL == pSrcAsciiFile)
    {
        PRT_INFO("pSrcAsciiFile is NULL\n");
        return SAL_FALSE;
    }

    stCtlAscElement[0].u32Key = GUI_ASCII_SPACE;
    stCtlAscElement[0].u32Width = 8;
    stCtlAscElement[0].u32WidthBegin = 0;

    u32FileSize = dspttk_get_file_size(pSrcAsciiFile);

    pSrcBMPBuffer = (CHAR *)malloc(u32FileSize);

    sRet = dspttk_read_file(pSrcAsciiFile, 0, pSrcBMPBuffer, &u32FileSize);

    while (u32Right < u32PicWidth)
    {
        for (i = 0; i < u32PicHeight; i++)
        {
            sRet = dspttk_disp_is_useful_bg_pixel(pSrcBMPBuffer, i, u32Right, u32PicWidth);
            if (SAL_TRUE == sRet)
            {
                break;
            }
        }

        if (SAL_TRUE == sRet)
        {
            if (SAL_FALSE == u32Flag)
            {
                u32Left = u32Right;
            }
            u32Flag = SAL_TRUE;
        }

        if (SAL_FALSE == sRet)
        {
            if (SAL_TRUE == u32Flag)
            {
                u32Width = u32Right - u32Left;
                stCtlAscElement[u32AsciiCnt].u32Key = u32AsciiCnt + GUI_ASCII_SPACE;
                stCtlAscElement[u32AsciiCnt].u32Width = u32Width;
                stCtlAscElement[u32AsciiCnt].u32WidthBegin = u32Left;

                u32AsciiCnt++;
            }
            u32Flag = SAL_FALSE;
        }
        u32Right++;
    }

    free(pSrcBMPBuffer);

    return sRet;
}


/**
 * @fn      dspttk_disp_get_ascii_src_from_bmp
 * @brief   从bmp图片中截取ascii码像素信息并保存在bgra文件中
 *
 * @param   [IN] pFileNameSrc      源数据地址
 * @param   [IN] pFileNameDst      目的数据地址
 * @param   [IN] u32PicWidth       源数据图片宽度
 * @return  成功:SAL_SOK 失败:SAL_FAIL
 */
SAL_STATUS dspttk_disp_get_ascii_src_from_bmp(CHAR *pFileNameSrc, CHAR *pFileNameDst, UINT32 u32PicWidth)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 u32SrcFileSize = 0, i = 0, j = 0, k = 0, u32ReadSize = 0;
    UINT64 u64SrcFileIdx = BMP_PIC_HEAD_SIZE, u64SingleAsciiBufIdx = 0, u64Total = 0, u32AscCnt = 0;
    CHAR *pSrcAsciiBuf = NULL;
    CHAR *pSingleAsciiBuf = NULL;
    CHAR szDstLineByte[BG_SINGLE_ELEMENT_MAX_WIDTH * GUI_BGRA_BIT_WIDTH] = {0};
    CHAR szSrcLineByte[BG_SINGLE_ELEMENT_MAX_WIDTH * GUI_BMP_BIT_WIDTH] = {0};

    if (NULL == pFileNameSrc || NULL == pFileNameDst)
    {
        PRT_INFO("param in error. srcfilename is %s, dstfilename is %s\n", pFileNameSrc, pFileNameDst);
        return SAL_FAIL;
    }

    u32SrcFileSize = dspttk_get_file_size(pFileNameSrc);

    if (u32SrcFileSize <= 0)
    {
        PRT_INFO("dspttk_get_file_size failed. file %s size is %d, ret is %d\n", pFileNameSrc, u32SrcFileSize, sRet);
        return SAL_FAIL;
    }

    pSrcAsciiBuf = (CHAR *)malloc(sizeof(CHAR) * u32SrcFileSize - BMP_PIC_HEAD_SIZE);

    if (NULL == pSrcAsciiBuf)
    {
        PRT_INFO("malloc memory for ascii buffer failed.\n");
        return SAL_FAIL;
    }

    pSingleAsciiBuf = (CHAR *)malloc(sizeof(CHAR) * GUI_BGRA_BIT_WIDTH * BG_SINGLE_ELEMENT_MAX_HEIGHT * BG_SINGLE_ELEMENT_MAX_WIDTH);

    sRet = dspttk_read_file(pFileNameSrc, BMP_PIC_HEAD_SIZE, pSrcAsciiBuf, &u32SrcFileSize);
    if (SAL_FAIL == sRet)
    {
        PRT_INFO("srcfilename is %s, filesize is %d\n", pFileNameSrc, u32SrcFileSize);
        free(pSrcAsciiBuf);
        return sRet;
    }

    u32AscCnt = sizeof(stCtlAscElement) / sizeof(DSPTTK_ELEMENT_FEATURE);

    for (i = 0; i < u32AscCnt; i++) // 一共需要对(126-32 +1 = 94 +1 == 95)个元素进行处理
    {
        u32ReadSize = stCtlAscElement[i].u32Width * GUI_BMP_BIT_WIDTH;                            // 每一次读取该元素一整行的像素点信息
        u64SrcFileIdx = stCtlAscElement[i].u32WidthBegin * GUI_BMP_BIT_WIDTH + BMP_PIC_HEAD_SIZE; // 每到下一个元素需要对初始位置进行更新

        for (j = 0; j < BG_SINGLE_ELEMENT_MAX_HEIGHT; j++) // 当前循环结束表示一个完整的元素已被处理完成
        {

            memset(szDstLineByte, 0xFF, sizeof(szDstLineByte));                                // 初始化置数组所有元素为0xFF,后续向对应数据位置进行memcpy
            sRet = dspttk_read_file(pFileNameSrc, u64SrcFileIdx, szSrcLineByte, &u32ReadSize); // 先从bmp文件中读取像素点信息到srcPerLine中 (bmp24位图中每个像素点信息只有三个字节(BGR), BGRA背景图中需要四个字节)
            for (k = 0; k < stCtlAscElement[i].u32Width; k++)                                  // 当前循环结束表示一个元素中的一整行已被处理完成
            {
                memcpy(szDstLineByte + k * GUI_BGRA_BIT_WIDTH, szSrcLineByte + k * GUI_BMP_BIT_WIDTH, GUI_BMP_BIT_WIDTH); // 将bmp图片中像素信息每三字节拷贝一次,存放在Dst的前三个字节中, Dst每隔四个字节赋值依次,第四个字节即BGRA的Alpha透明度,初始化memset为0xFF
            }
            memcpy(pSingleAsciiBuf + u64SingleAsciiBufIdx, szDstLineByte, stCtlAscElement[i].u32Width * GUI_BGRA_BIT_WIDTH);
            u64SingleAsciiBufIdx += stCtlAscElement[i].u32Width * GUI_BGRA_BIT_WIDTH; // 每完成一行就将当前行的像素信息拷贝到元素的完整信息中,待j==元素的高度时, 该元素的信息已按行存储在szSingleAsciiBuf中, u64SingleAsciiBufIdx不仅保存szSingleAsciiBuf的索引信息, 还保存当前元素所占空间大小
            u64SrcFileIdx += u32PicWidth * GUI_BMP_BIT_WIDTH;                         // 对bmp中文件索引进行更新, j*stCtlAscElement[i].u32Width*GUI_BMP_BIT_WIDTH相当于跳转到下一行的对应位置
        }
        sRet = dspttk_write_file(pFileNameDst, 0, SEEK_END, pSingleAsciiBuf, stCtlAscElement[i].u32Width * GUI_BGRA_BIT_WIDTH * BG_SINGLE_ELEMENT_MAX_HEIGHT); // 当前元素已处理完成, 将其相关数据填充在目标文件中
        u64Total += u64SingleAsciiBufIdx;
        u64SingleAsciiBufIdx = 0;
    }

    return sRet;
}

/**
 * @fn      dspttk_disp_write_ascii_to_bgra
 * @brief   将ascii元素写入fb内存区域做显示
 *
 * @param   [IN] pDstData      目的数据地址
 * @param   [IN] pSrcData      源数据地址
 * @param   [IN] u32PixelGap   每个显示元素之间的间隔(单位pixel)
 * @return  成功:SAL_SOK 失败:SAL_FAIL
 */
SAL_STATUS dspttk_disp_write_ascii_to_bgra(UINT8 *pDstData, CHAR *pSrcData, UINT32 u32PixelGap)
{
    SAL_STATUS sRet = SAL_SOK;
    CHAR *pPerLineData = NULL;
    UINT32 u32ElementCnt = 0, u32AsciiNum = 0, i = 0, j = 0, u32BufferSize = 0;
    UINT64 u64Idx = 0;
    UINT32 szAsciiIdx[MAX_DISPLAY_STRING_SIZE] = {0};

    if (NULL == pDstData || NULL == pSrcData)
    {
        PRT_INFO("param in error. DstDataAddr is %p, SrcDataAddr is %p", pDstData, pSrcData);
        return SAL_FAIL;
    }
    if(pPerLineData == NULL)
    {
        pPerLineData = (CHAR *)malloc(MAX_DISPLAY_STRING_SIZE * BG_SINGLE_ELEMENT_MAX_WIDTH * GUI_BGRA_BIT_WIDTH);
        if (NULL == pPerLineData)
        {
            PRT_INFO("PerLineData memory malloc failed.\n");
            return SAL_FAIL;
        }
    }

    u32ElementCnt = strlen(pSrcData);

    for (i = 0; i < u32ElementCnt; i++)
    {
        u32AsciiNum = (UINT32) * (pSrcData + i);
        if (u32AsciiNum < GUI_ASCII_SPACE || u32AsciiNum > GUI_ASCII_TILDE)
        {
            PRT_INFO("in string %s, idx is %d, u32AsciiNum is %d, ascii num cannot be showed. \n", pSrcData, i, u32AsciiNum);
            return SAL_FALSE;
        }
        else
        {
            szAsciiIdx[i] = u32AsciiNum - GUI_ASCII_SPACE;
        }
    }

    u32BufferSize = MAX_DISPLAY_STRING_SIZE * BG_SINGLE_ELEMENT_MAX_WIDTH;

    /* 对当前背景Buffer初始化 */
    for (i = 0; i < u32BufferSize; i++)
    {
        memset(pPerLineData + i * GUI_BGRA_BIT_WIDTH, (0xFF & BG_PIXEL_COLOR), 1);
        memset(pPerLineData + i * GUI_BGRA_BIT_WIDTH + 1, ((0xFF << 8) & BG_PIXEL_COLOR) >> 8, 1);
        memset(pPerLineData + i * GUI_BGRA_BIT_WIDTH + 2, ((0xFF << 16) & BG_PIXEL_COLOR) >> 16, 1);
        memset(pPerLineData + i * GUI_BGRA_BIT_WIDTH + 3, ((0xFF << 24) & BG_PIXEL_COLOR) >> 24, 1);
    }

    /* 将元素信息写入buffer并拷贝到fb中送显示 */
    for (i = 0; i < BG_SINGLE_ELEMENT_MAX_HEIGHT; i++)
    {
        for (j = 0; j < u32ElementCnt; j++)
        {
            memcpy(pPerLineData + u64Idx,
                   stCtlAscElement[szAsciiIdx[j]].pAddr + i * stCtlAscElement[szAsciiIdx[j]].u32Width * GUI_BGRA_BIT_WIDTH,
                   stCtlAscElement[szAsciiIdx[j]].u32Width * GUI_BGRA_BIT_WIDTH);
            u64Idx += stCtlAscElement[szAsciiIdx[j]].u32Width * GUI_BGRA_BIT_WIDTH;
            u64Idx += u32PixelGap * GUI_BGRA_BIT_WIDTH;
        }
        memcpy(pDstData + i * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH, pPerLineData, u64Idx);
        u64Idx = 0;
    }

    if(pPerLineData != NULL)
    {
        free(pPerLineData);
        pPerLineData = NULL;
    }

    return sRet;
}

/**
 * @fn      dspttk_disp_get_time
 * @brief   获取当前系统时间
 *
 * @param   [IN] u32Len   pBuffer长度
 * @param   [OUT] pBuffer   将获取到的时间以字符串形式写入pBuffer
 * @return  成功:SAL_SOK 失败:SAL_FAIL
 */
SAL_STATUS dspttk_disp_get_time(CHAR *pBuffer, UINT32 u32Len)
{
    SAL_STATUS sRet = SAL_FAIL;
    struct timeval stTimeValue = {0};
    struct tm *pstTm = NULL;
    CHAR szTime[MAX_DISPLAY_STRING_SIZE] = {0};

    if (NULL == pBuffer)
    {
        PRT_INFO("ParamIn pBuffer is NULL.\n");
        return SAL_FAIL;
    }

    if (u32Len < sizeof(szTime))
    {
        PRT_INFO("Src String Length (%d) Less Than Time String Length(%ld).\n", u32Len, sizeof(szTime));
        return SAL_FAIL;
    }

    sRet = gettimeofday(&stTimeValue, NULL);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("gettimeofday failed.\n");
        return SAL_FAIL;
    }

    pstTm = localtime(&(stTimeValue.tv_sec));
    if (NULL == pstTm)
    {
        PRT_INFO("get localtime failed.\n");
        return SAL_FAIL;
    }

    strftime(szTime, sizeof(szTime), "%Y.%m.%d %H:%M:%S", pstTm); // 输出格式为: 2023.03.08 09:40:00
    memcpy(pBuffer, szTime, sizeof(szTime));

    return sRet;
}

/**
 * @fn      dspttk_disp_additional_info
 * @brief   获取附加信息并送显示
 * @param   [IN] pBGAddr   目的数据地址(即fb首地址)
 * @return  无
 */
void dspttk_disp_additional_info(UINT8 *pBGAddr)
{
    SAL_STATUS sRet = SAL_SOK;
    DSP_VERSION_PARAM stDspVersionParam = {0};
    CHAR szAdditionalAddr[4][MAX_FILENAME_SIZE] = {{0}, {0}, {0}, {0}};

    if (NULL == pBGAddr)
    {
        PRT_INFO("pBGAddr is NULL\n");
        return;
    }

    sRet = SendCmdToDsp(HOST_CMD_DSP_GET_SYS_VERISION, 0, NULL, &stDspVersionParam);

    if (SAL_SOK != sRet)
    {
        PRT_INFO("HOST_CMD_DSP_GET_SYS_VERSION failed. sRet is %d\n", sRet);
        return;
    }

    dspttk_disp_get_time(stDspVersionParam.szCurrentTime, sizeof(stDspVersionParam.szCurrentTime));

    memcpy(szAdditionalAddr[0], stDspVersionParam.szCurrentTime, sizeof(stDspVersionParam.szCurrentTime));

    memcpy(szAdditionalAddr[1], stDspVersionParam.szBuilder, sizeof(stDspVersionParam.szBuilder));
    
    memcpy(szAdditionalAddr[2], stDspVersionParam.szDSPVersion, sizeof(stDspVersionParam.szDSPVersion));

    memcpy(szAdditionalAddr[3], stDspVersionParam.szBuildTime, sizeof(stDspVersionParam.szBuildTime));

    dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH + 
                                    (DISPLAY_SPLIT_LINE_2 + DISPLAY_GAP_WORD_SPLIT) * GUI_BGRA_BIT_WIDTH, 
                                    szAdditionalAddr[0], 2);

    dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT + DISPLAY_GAP_WORD_SPLIT * 1) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH + 
                                    (DISPLAY_SPLIT_LINE_2 + DISPLAY_GAP_WORD_SPLIT) * GUI_BGRA_BIT_WIDTH, 
                                    szAdditionalAddr[1], 2);

    dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT + DISPLAY_GAP_WORD_SPLIT * 2) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH + 
                                    (DISPLAY_SPLIT_LINE_2 + DISPLAY_GAP_WORD_SPLIT) * GUI_BGRA_BIT_WIDTH, 
                                    szAdditionalAddr[2], 2);

    dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT + DISPLAY_GAP_WORD_SPLIT * 3) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH + 
                                    (DISPLAY_SPLIT_LINE_2 + DISPLAY_GAP_WORD_SPLIT) * GUI_BGRA_BIT_WIDTH, 
                                    szAdditionalAddr[3], 2);
    return;
}

/**
 * @fn      dspttk_disp_xsp_status_info
 * @brief   获取xsp信息并送显示
 * @param   [IN] pBGAddr   目的数据地址(即fb首地址)
 * @return  无
 */
void dspttk_disp_xsp_status_info(UINT8 *pBGAddr)
{
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    UINT32 i = 0, u32StatusCnt = 0, u32HeightCnt = 0;
    CHAR szXSPStatusCtl[][MAX_DISPLAY_STRING_SIZE] = {{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}};
    CHAR szXSPPipeInfo[MAX_DISPLAY_STRING_SIZE] = {0};

    if (NULL == pBGAddr)
    {
        PRT_INFO("pBGAddr is NULL\n");
        return;
    }

    // 0.反色
    u32StatusCnt = 0;
    if (pstShareParam->stXspPicPrm.stAntiColor.bEnable)
    {
        snprintf(szXSPStatusCtl[u32StatusCnt], sizeof(szXSPStatusCtl[u32StatusCnt]), "%s ", g_szXspFunc[XSP_FUNC_ANTI]);
        u32StatusCnt++;
    }

    // 1.边缘增强(level)
    if (pstShareParam->stXspPicPrm.stEe.bEnable)
    {
        snprintf(szXSPStatusCtl[u32StatusCnt], sizeof(szXSPStatusCtl[u32StatusCnt]), "%s %d,",
                 g_szXspFunc[XSP_FUNC_EE],
                 pstShareParam->stXspPicPrm.stEe.uiLevel);
        u32StatusCnt++;
    }

    // 2.超增(level)
    if (pstShareParam->stXspPicPrm.stUe.bEnable)
    {
        snprintf(szXSPStatusCtl[u32StatusCnt], sizeof(szXSPStatusCtl[u32StatusCnt]), "%s %d,",
                 g_szXspFunc[XSP_FUNC_UE],
                 pstShareParam->stXspPicPrm.stUe.uiLevel);
        u32StatusCnt++;
    }

    // 3.低穿
    if (pstShareParam->stXspPicPrm.stLp.bEnable)
    {
        snprintf(szXSPStatusCtl[u32StatusCnt], sizeof(szXSPStatusCtl[u32StatusCnt]), "%s ",
                 g_szXspFunc[XSP_FUNC_LP]);
        u32StatusCnt++;
    }

    // 4.高穿
    if (pstShareParam->stXspPicPrm.stHp.bEnable)
    {
        snprintf(szXSPStatusCtl[u32StatusCnt], sizeof(szXSPStatusCtl[u32StatusCnt]), "%s ",
                 g_szXspFunc[XSP_FUNC_HP]);
        u32StatusCnt++;
    }

    // 5.局增
    if (pstShareParam->stXspPicPrm.stLe.bEnable)
    {
        snprintf(szXSPStatusCtl[u32StatusCnt], sizeof(szXSPStatusCtl[u32StatusCnt]), "%s ",
                 g_szXspFunc[XSP_FUNC_LE]);
        u32StatusCnt++;
    }

    // 6.可变吸收率(level)
    if (pstShareParam->stXspPicPrm.stVarAbsor.bEnable)
    {
        snprintf(szXSPStatusCtl[u32StatusCnt], sizeof(szXSPStatusCtl[u32StatusCnt]), "%s %d,",
                 g_szXspFunc[XSP_FUNC_ABSOR],
                 pstShareParam->stXspPicPrm.stVarAbsor.uiLevel);
        u32StatusCnt++;
    }

    // 7.设置亮度(level)
    if (pstShareParam->stXspPicPrm.stLum.bEnable)
    {
        snprintf(szXSPStatusCtl[u32StatusCnt], sizeof(szXSPStatusCtl[u32StatusCnt]), "%s %d,",
                 g_szXspFunc[XSP_FUNC_LUM],
                 pstShareParam->stXspPicPrm.stLum.uiLevel);
        u32StatusCnt++;
    }

    // 8.伪彩(level)
    snprintf(szXSPStatusCtl[u32StatusCnt], sizeof(szXSPStatusCtl[u32StatusCnt]), "%s %d,",
             g_szXspFunc[XSP_FUNC_PSEUDO], 
             pstShareParam->stXspPicPrm.stPseudoColor.pseudo_color);
    u32StatusCnt++;

    // 9. 显示模式(level)
    snprintf(szXSPStatusCtl[u32StatusCnt], sizeof(szXSPStatusCtl[u32StatusCnt]), "%s %d,",
             g_szXspFunc[XSP_FUNC_DISPMODE],
             pstShareParam->stXspPicPrm.stDispMode.disp_mode);
    u32StatusCnt++;

    for (i = 0; i < u32StatusCnt; i++)
    {
        if (i == DISPLAY_MAX_STRING_CNT)
        {
            u32HeightCnt = 1;
        }

        dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT + 
                                        u32HeightCnt * DISPLAY_GAP_WORD_SPLIT) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH + 
                                        (i % DISPLAY_MAX_STRING_CNT) * DISPLAY_GAP_WORD_SPLIT * GUI_BGRA_BIT_WIDTH * 5,
                                        szXSPStatusCtl[i], 1);
    }

    snprintf(szXSPPipeInfo, sizeof(szXSPPipeInfo), "timeProcPipe1: %03lld ms", 
                pstShareParam->stTimePram.timeProcPipe0E - 
                pstShareParam->stTimePram.timeProcPipe0S);

    dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT)*DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH + DISPLAY_GAP_WORD_SPLIT * 20 * GUI_BGRA_BIT_WIDTH,
                                    szXSPPipeInfo, 1);
    memset(szXSPPipeInfo, 0, sizeof(szXSPPipeInfo));

    snprintf(szXSPPipeInfo, sizeof(szXSPPipeInfo), "timeProcPipe2: %03lld ms", pstShareParam->stTimePram.timeProcPipe1E - 
                pstShareParam->stTimePram.timeProcPipe1S);

    dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT + DISPLAY_GAP_WORD_SPLIT) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH + 
                                        DISPLAY_GAP_WORD_SPLIT * 20 * GUI_BGRA_BIT_WIDTH,
                                        szXSPPipeInfo, 1);
    memset(szXSPPipeInfo, 0, sizeof(szXSPPipeInfo));

    snprintf(szXSPPipeInfo, sizeof(szXSPPipeInfo), "timeProcPipe3: %03lld ms", pstShareParam->stTimePram.timeProcPipe2E - 
                pstShareParam->stTimePram.timeProcPipe2S);

    dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT + 2 * DISPLAY_GAP_WORD_SPLIT) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH + 
                                    DISPLAY_GAP_WORD_SPLIT * 20 * GUI_BGRA_BIT_WIDTH,
                                    szXSPPipeInfo, 1);
    memset(szXSPPipeInfo, 0, sizeof(szXSPPipeInfo));

    snprintf(szXSPPipeInfo, sizeof(szXSPPipeInfo), "timeProcPipeSum: %03lld ms", pstShareParam->stTimePram.timeAfterProc - 
                pstShareParam->stTimePram.timeBeforeProc);

    dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT + 3 * DISPLAY_GAP_WORD_SPLIT) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH + 
                                    DISPLAY_GAP_WORD_SPLIT * 20 * GUI_BGRA_BIT_WIDTH,
                                    szXSPPipeInfo, 1);
    memset(szXSPPipeInfo, 0, sizeof(szXSPPipeInfo));
}

/**
 * @fn      dspttk_disp_enlarge_info
 * @brief   获取enlarge信息并送显示
 * @param   [IN] pBGAddr   目的数据地址(即fb首地址)
 * @return  无
 */
void dspttk_disp_enlarge_info(UINT8 *pBGAddr)
{
    SAL_STATUS sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstInitPrm = dspttk_get_share_param();
    DSPTTK_SETTING_DISP *pstDispCfg = &pstDevCfg->stSettingParam.stDisp;

    UINT32 i = 0, j = 0, u32HeightCnt = 0, u32Width = 0;
    BOOL bEnlarge = SAL_FALSE;
    CHAR szEnlargeString[MAX_DISPLAY_STRING_SIZE] = {0};
    CHAR szEnlargeInfo[MAX_DISPLAY_STRING_SIZE] = {0};

    if (NULL == pBGAddr)
    {
        PRT_INFO("pBGAddr is NULL\n");
        return;
    }

    for (i = 0; i < pstDevCfg->stInitParam.stDisplay.voDevCnt; i++)
    {
        for (j = 0; j < MAX_DISP_CHAN; j++)
        {

            if (0 != pstDispCfg->stGlobalEnlargePrm[i][j].ratio || 0 != pstDispCfg->stLocalEnlargePrm[i][j].ratio)
            {
                if (SAL_TRUE == bEnlarge)
                {
                    u32HeightCnt++;
                }

                if (((DISPLAY_DEVICE_HEIGHT - DISPLAY_MENU_HEIGHT) / BG_SINGLE_ELEMENT_MAX_HEIGHT) == u32HeightCnt)
                {
                    u32HeightCnt = 0;
                }

                u32Width = 0;

                snprintf(szEnlargeInfo, sizeof(szEnlargeInfo), "(vodev -- %d window -- %d): ", i, j);
                sRet = dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT + u32HeightCnt * DISPLAY_GAP_WORD_SPLIT) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH +
                                                           (DISPLAY_SPLIT_LINE_1 + DISPLAY_GAP_WORD_SPLIT) * GUI_BGRA_BIT_WIDTH,
                                                       szEnlargeInfo, 1);
                u32Width += sizeof(szEnlargeInfo);

                bEnlarge = SAL_TRUE;

                if (0 != pstInitPrm->GlobalEnlarge[i][j].ratio)
                {
                    snprintf(szEnlargeString, sizeof(szEnlargeString), "GlobalEnlarge x:%d, y:%d, ratio:%.1f; ",
                            pstInitPrm->GlobalEnlarge[i][j].x, pstInitPrm->GlobalEnlarge[i][j].y,
                            pstInitPrm->GlobalEnlarge[i][j].ratio);
                    sRet = dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT + u32HeightCnt * DISPLAY_GAP_WORD_SPLIT) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH +
                                                           (DISPLAY_SPLIT_LINE_1 + u32Width * BG_ASCII_AVERAGE_WIDTH) * GUI_BGRA_BIT_WIDTH,
                                                            szEnlargeString, 1);
                    if (SAL_SOK != sRet)
                    {
                        PRT_INFO("dspttk_disp_write_ascii_to_bgra failed. string is %s\n", szEnlargeString);
                    }
                }

                u32Width += strlen(szEnlargeString) + 10;

                if (0 != pstDispCfg->stLocalEnlargePrm[i][j].ratio)
                {
                    snprintf(szEnlargeString, sizeof(szEnlargeString), "LocalEnlarge x:%d, y:%d, ratio:%.1f;",
                            pstDispCfg->stLocalEnlargePrm[i][j].x, pstDispCfg->stLocalEnlargePrm[i][j].y,
                             pstDispCfg->stLocalEnlargePrm[i][j].ratio);
                    sRet = dspttk_disp_write_ascii_to_bgra(pBGAddr + (DISPLAY_MENU_HEIGHT + u32HeightCnt * DISPLAY_GAP_WORD_SPLIT) * DISPLAY_DEVICE_WIDTH * GUI_BGRA_BIT_WIDTH + (DISPLAY_SPLIT_LINE_1 + u32Width * BG_ASCII_AVERAGE_WIDTH) * GUI_BGRA_BIT_WIDTH,
                                                       szEnlargeString, 1);
                    if (SAL_SOK != sRet)
                    {
                        PRT_INFO("dspttk_disp_write_ascii_to_bgra failed. string is %s\n", szEnlargeString);
                    }
                }

            }
            else
            {
                bEnlarge = SAL_FALSE;
            } 
        }
    }
}

/**
 * @fn      dspttk_disp_generate_bgImage
 * @brief   根据配置信息生成对应背景图片
 *
 * @param   [IN] pSrcAddr  生成的背景图片名
 * @param   [IN] u32Width  背景图片宽(默认1920)
 * @param   [IN] u32Height  背景图片高(默认1080)
 *
 * @return  SAL_STATUS 成功:SAL_SOK 失败:SAL_FAIL
 */
SAL_STATUS dspttk_disp_generate_bgImage(UINT8 *pSrcAddr, UINT32 u32Width, UINT32 u32Height)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 i = 0, j = 0;

    UINT32 u32MenuHeight = DISPLAY_DEVICE_HEIGHT - DISPLAY_MENU_HEIGHT;

    if (NULL == pSrcAddr || u32Width <= 0 || u32Height <= 0)
    {
        PRT_INFO("Param In Error. pSrcAddr is %p, u32Width is %d, u32Height is %d\n", pSrcAddr, u32Width, u32Height);
        return SAL_FAIL;
    }

    memset(pSrcAddr, 0, u32Height * u32Width * GUI_BGRA_BIT_WIDTH);

    for (i = (u32Height - u32MenuHeight); i < u32Height; i++)
    {
        for (j = 0; j < u32Width; j++)
        {
            memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH, (0xFF & BG_PIXEL_COLOR), 1);
            memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH + 1, ((0xFF << 8) & BG_PIXEL_COLOR) >> 8, 1);
            memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH + 2, ((0xFF << 16) & BG_PIXEL_COLOR) >> 16, 1);
            memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH + 3, ((0xFF << 24) & BG_PIXEL_COLOR) >> 24, 1);
        }
    }

    for (i = (u32Height - u32MenuHeight); i < u32Height; i++)
    {
        j = DISPLAY_SPLIT_LINE_1;
        memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH, BG_LINE_PIXEL_COLOR, 1);
        memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH + 1, BG_LINE_PIXEL_COLOR, 1);
        memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH + 2, BG_LINE_PIXEL_COLOR, 1);
        memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH + 3, ((0xFF << 24) & BG_PIXEL_COLOR) >> 24, 1);
    }

    for (i = (u32Height - u32MenuHeight); i < u32Height; i++)
    {
        j = DISPLAY_SPLIT_LINE_2;
        memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH, BG_LINE_PIXEL_COLOR, 1);
        memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH + 1, BG_LINE_PIXEL_COLOR, 1);
        memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH + 2, BG_LINE_PIXEL_COLOR, 1);
        memset(pSrcAddr + i * u32Width * GUI_BGRA_BIT_WIDTH + j * GUI_BGRA_BIT_WIDTH + 3, ((0xFF << 24) & BG_PIXEL_COLOR) >> 24, 1);
    }

    return sRet;
}



/**
 * @fn      dspttk_disp_operate_menu_info
 * @brief   每300ms更新菜单栏信息状态并刷新fb
 * @return  无
 */
void dspttk_disp_operate_menu_info(UINT8 *pBGAddr, UINT32 u32Vodev)
{

    while (SAL_TRUE)
    {
        dspttk_disp_generate_bgImage(stScreenParam[u32Vodev].stScreenAttr->srcAddr, DISPLAY_DEVICE_WIDTH, DISPLAY_DEVICE_HEIGHT);
        dspttk_disp_additional_info(pBGAddr);
        dspttk_disp_xsp_status_info(pBGAddr);
        dspttk_disp_enlarge_info(pBGAddr);
        SendCmdToDsp(HOST_CMD_DISP_FB_SHOW, u32Vodev, NULL, NULL);
        dspttk_msleep(300);
    }
}

/**
 * @fn      dspttk_disp_set_vi
 * @brief   配置采集预览功能
 *
 * @param   [IN] u32Vodev  显示器编号
 * @param   [IN] u32WinNo  窗口编号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_set_vi(UINT32 u32Vodev, UINT32 u32WinNo)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSP_DISP_RCV_PARAM stDispRegion = {0};
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DISP *pstDispCfg = NULL;

    if (u32Vodev > MAX_VODEV_CNT - 1 || u32WinNo > MAX_DISP_CHAN - 1)
    {
        PRT_INFO("Vodev Num is %d, Supposed to be 0 or 1. WinNo is %d , Supposed the WinNo in [0,16)\n", u32Vodev, u32WinNo);
        return CMD_RET_MIX(HOST_CMD_SET_VI_DISP, sRet);
    }

    pstDispCfg = &pstDevCfg->stSettingParam.stDisp;

    /* 采集预览功能参数配置:
     *
     * stDispRegion.uiCnt: 配置个数(该接口每次只配置一个预览窗口)
     * stDispRegion.addr[0]: 采集编号(DUP编号)或者编码通道
     * stDispRegion.out[0]: 显示器上显示通道编号(0-N)
     *
     */

    stDispRegion.uiCnt = 1;
    stDispRegion.addr[0] = pstDispCfg->stInSrcParam[u32Vodev][u32WinNo].u32SrcChn;
    stDispRegion.out[0] = pstDispCfg->stWinParam[u32Vodev][u32WinNo].stDispRegion.uiChan;

    sRet = SendCmdToDsp(HOST_CMD_SET_VI_DISP, u32Vodev, NULL, &stDispRegion);

    return CMD_RET_MIX(HOST_CMD_SET_VI_DISP, sRet);
}

/**
 * @fn      dspttk_disp_set_dec
 * @brief   配置解码预览功能
 *
 * @param   [IN] u32Vodev  显示器编号
 * @param   [IN] u32WinNo  窗口编号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */

UINT64 dspttk_disp_set_dec(UINT32 u32Vodev, UINT32 u32WinNo)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSP_DISP_RCV_PARAM stDispRegion = {0};
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DISP *pstDispCfg = NULL;

    if (u32Vodev > MAX_VODEV_CNT - 1 || u32WinNo > MAX_DISP_CHAN - 1)
    {
        PRT_INFO("Vodev Num is %d, Supposed to be 0 or 1. WinNo is %d , Supposed the WinNo in [0,16)\n", u32Vodev, u32WinNo);
        return CMD_RET_MIX(HOST_CMD_SET_DEC_DISP, sRet);
    }

    pstDispCfg = &pstDevCfg->stSettingParam.stDisp;

    stDispRegion.uiCnt = 1;

    stDispRegion.addr[0] = pstDispCfg->stInSrcParam[u32Vodev][u32WinNo].u32SrcChn;
    stDispRegion.out[0] = pstDispCfg->stWinParam[u32Vodev][u32WinNo].stDispRegion.uiChan;
    stDispRegion.draw[0] = 0;

    sRet = SendCmdToDsp(HOST_CMD_SET_DEC_DISP, u32Vodev, NULL, &stDispRegion);

    return CMD_RET_MIX(HOST_CMD_SET_DEC_DISP, sRet);
}

/**
 * @fn      dspttk_disp_set_thumbnail
 * @brief   配置缩略图预览功能
 *
 * @param   [IN] u32Vodev  显示器编号
 * @param   [IN] u32WinNo  窗口编号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */

UINT64 dspttk_disp_set_thumbnail(UINT32 u32Vodev, UINT32 u32WinNo)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DISP *pstDispCfg = NULL;
    pstDispCfg = &pstDevCfg->stSettingParam.stDisp;
    DISP_POS_CTRL stDispPrm = {0};
    UINT32 u32FillColor = 0, u32BorderColor = 0;
    CHAR cFillColorStr[20] = {0};
    CHAR cBorderColorStr[20] = {0};

    stDispPrm.enable = pstDispCfg->stWinParam[u32Vodev][u32WinNo].enable;
    stDispPrm.voChn = pstDispCfg->stWinParam[u32Vodev][u32WinNo].stDispRegion.uiChan;
    stDispPrm.voDev = u32Vodev;
    stDispPrm.stDispRegion.x = pstDispCfg->stWinParam[u32Vodev][u32WinNo].stDispRegion.x;
    stDispPrm.stDispRegion.y = pstDispCfg->stWinParam[u32Vodev][u32WinNo].stDispRegion.y;
    stDispPrm.stDispRegion.w = pstDispCfg->stWinParam[u32Vodev][u32WinNo].stDispRegion.w;
    stDispPrm.stDispRegion.h = pstDispCfg->stWinParam[u32Vodev][u32WinNo].stDispRegion.h;
    stDispPrm.stDispRegion.bDispBorder = pstDispCfg->stWinParam[u32Vodev][u32WinNo].stDispRegion.bDispBorder;
    stDispPrm.stDispRegion.BorDerLineW = pstDispCfg->stWinParam[u32Vodev][u32WinNo].stDispRegion.BorDerLineW;

    if (pstDispCfg->stWinColorPrm[u32Vodev][u32WinNo].cFillWinColor[0] == '#')
    {
        strcpy(cFillColorStr, "ff");
        strcat(cFillColorStr, &pstDispCfg->stWinColorPrm[u32Vodev][u32WinNo].cFillWinColor[1]);
    }
    else
    {
        strcpy(cFillColorStr, "ff");
        strcat(cFillColorStr, &pstDispCfg->stWinColorPrm[u32Vodev][u32WinNo].cFillWinColor[0]);
    }
    u32FillColor = strtoul(cFillColorStr, NULL, 16);
    stDispPrm.stDispRegion.color = u32FillColor;

    if (pstDispCfg->stWinColorPrm[u32Vodev][u32WinNo].cBorderLineColor[0] == '#')
    {
        strcpy(cBorderColorStr, "ff");
        strcat(cBorderColorStr, &pstDispCfg->stWinColorPrm[u32Vodev][u32WinNo].cBorderLineColor[1]);
    }
    else
    {
        strcpy(cBorderColorStr, "ff");
        strcat(cBorderColorStr, &pstDispCfg->stWinColorPrm[u32Vodev][u32WinNo].cBorderLineColor[0]);
    }
    u32BorderColor = strtoul(cBorderColorStr, NULL, 16);
    stDispPrm.stDispRegion.BorDerColor = u32BorderColor;

    sRet = SendCmdToDsp(HOST_CMD_SET_THUMBNAIL_PRM, u32Vodev, NULL, &stDispPrm);

    return CMD_RET_MIX(HOST_CMD_SET_THUMBNAIL_PRM, sRet);
}

/**
 * @fn      dspttk_screen_ctrl_set
 * @brief   设置SCREEN_CTRL参数
 *
 * @param   [IN] u32Vodev 显示设备号
 * @param   [IN] pFileName 需要通过文件名获取长宽
 * @param   [OUT] pstScreen 已设置好长宽和位宽等参数的SCREEN_CTRL结构体,HOST_CMD_DISP_FB_MMAP命令对应参数
 *
 * @return  32位表示错误号
 */
SAL_STATUS dspttk_screen_ctrl_set(UINT32 u32Vodev, CHAR *pFileName, SCREEN_CTRL *pstScreen)
{
    SAL_STATUS sRet = SAL_FAIL;
    CHAR *pSubString = NULL;
    CHAR szSubString[MAX_FILENAME_SIZE] = {0};
    pSubString = szSubString;
    if (u32Vodev > MAX_VODEV_CNT || pFileName == NULL || pstScreen == NULL)
    {
        PRT_INFO("Param Error. u32Vodev is %d, pFileName is %p, pstScreen is %p.", u32Vodev, pFileName, pstScreen);
        return SAL_FAIL;
    }

    pstScreen->uiCnt = 1;
    pstScreen->stScreenAttr[0].bitWidth = GUI_BIT_WIDTH;
    pstScreen->stScreenAttr[0].x = 0;
    pstScreen->stScreenAttr[0].y = 0;

    pSubString = strstr(pFileName, "_w");

    if (pSubString == NULL)
    {
        PRT_INFO("pSubString is NULL.\n");
        return SAL_FAIL;
    }

    sRet = sscanf(pSubString, "%*[^w]w%u[^_]", &pstScreen->stScreenAttr[0].width);

    if (sRet < 0)
    {
        PRT_INFO("sscanf width from %s failed.\n", pSubString);
        return SAL_FAIL;
    }

    sRet = sscanf(pSubString, "%*[^h]h%u[^_]", &pstScreen->stScreenAttr[0].height);

    if (sRet < 0)
    {
        PRT_INFO("sscanf height from %s failed.\n", pSubString);
        return SAL_FAIL;
    }

    sRet = SAL_SOK;

    pSubString = NULL;
    return sRet;
}

/**
 * @fn      dspttk_cursor_fb_mmap
 * @brief   设置FrameBuffer参数,对鼠标图片进行mmap
 *
 * @param   [IN] u32Vodev 显示设备号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_cursor_fb_mmap(UINT32 u32Vodev)
{
    SAL_STATUS sRet = SAL_FAIL;
    SCREEN_CTRL stScreen = {0};
    CHAR szGuiName[256] = {0};
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENV *pstCfgEnv = &pstDevCfg->stSettingParam.stEnv;
    DSPTTK_SETTING_ENV_INPUT *pstInput = &pstCfgEnv->stInputDir;

    if (u32Vodev != SCREEN_LAYER_MAX - 1) // FB2用来对cursor进行mmap和show
    {
        PRT_INFO("Vodev Num is %d, Supposed to be SCREEN_LAYER_MAX-1.\n", u32Vodev);
        return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, SAL_FAIL);
    }

    snprintf(szGuiName, sizeof(szGuiName), "%s%s", pstInput->gui, GUI_CURSOR_BGRA);

    sRet = dspttk_screen_ctrl_set(u32Vodev, szGuiName, &stScreen);

    if (sRet != SAL_SOK)
    {
        PRT_INFO("Get SCREEN_CTRL at %s, %d failed!\n", __func__, __LINE__);
        return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, SAL_FAIL);
    }

    sRet = SendCmdToDsp(HOST_CMD_DISP_FB_MMAP, u32Vodev, NULL, &stScreen);
    return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, sRet);
}

/**
 * @fn      dspttk_gui_fb_mmap
 * @brief   设置FrameBuffer参数,对背景图片进行mmap
 *
 * @param   [IN] u32Vodev 显示设备号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_gui_fb_mmap(UINT32 u32Vodev)
{
    SAL_STATUS sRet = SAL_FAIL;

    if (u32Vodev > MAX_VODEV_CNT - 1)
    {
        PRT_INFO("Vodev Num Error. Vodev Num is %d, Supposed to be 0 or 1.\n", u32Vodev);
        return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, SAL_FAIL);
    }

    stScreenParam[u32Vodev].uiCnt = 1;
    stScreenParam[u32Vodev].stScreenAttr[0].bitWidth = GUI_BIT_WIDTH;
    stScreenParam[u32Vodev].stScreenAttr[0].x = 0;
    stScreenParam[u32Vodev].stScreenAttr[0].y = 0;
    stScreenParam[u32Vodev].stScreenAttr[0].width = DISPLAY_DEVICE_WIDTH;
    stScreenParam[u32Vodev].stScreenAttr[0].height = DISPLAY_DEVICE_HEIGHT;

    sRet = SendCmdToDsp(HOST_CMD_DISP_FB_MMAP, u32Vodev, NULL, &stScreenParam[u32Vodev]);

    if (NULL == stScreenParam[u32Vodev].stScreenAttr[0].srcAddr)
    {
        PRT_INFO("mmap failed. srcAddr is NULL. vodev is %d\n", u32Vodev);
        return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, sRet);
    }

    memset(stScreenParam[u32Vodev].stScreenAttr[0].srcAddr, 0xFF, stScreenParam[u32Vodev].stScreenAttr[0].srcSize);

    return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, sRet);
}

/**
 * @fn      dspttk_disp_fb_show
 * @brief   对FB缓冲区刷新, 当前仅对GUI 背景图片进行刷新显示
 *
 * @param   [IN] u32Vodev 显示设备号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_fb_show(UINT32 u32Vodev)
{
    SAL_STATUS sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENV_INPUT *pstEnvInput = &pstDevCfg->stSettingParam.stEnv.stInputDir;
    UINT32 u32FileSize = 0, i = 0, u32AsciiCnt = 0;
    UINT64 u64AddrIdxSrc = 0;
    CHAR *pSrcAscii = NULL;
    CHAR szAscBgraFileFullPath[MAX_FILENAME_SIZE] = {0};

    snprintf(szAscBgraFileFullPath, sizeof(szAscBgraFileFullPath), "%s%s", pstEnvInput->gui, GUI_ASCII_SRC_BGRA);

    u32FileSize = dspttk_get_file_size(szAscBgraFileFullPath);

    if (u32FileSize <= 0)
    {
        PRT_INFO("get %s file size failed. filesize is %d\n", szAscBgraFileFullPath, u32FileSize);
        return CMD_RET_MIX(HOST_CMD_DISP_FB_SHOW, SAL_FAIL);
    }

    pSrcAscii = (CHAR *)malloc(u32FileSize);
    if (NULL == pSrcAscii)
    {
        PRT_INFO("malloc memory for SrcAscii failed.\n");
        return SAL_FAIL;
    }

    sRet = dspttk_read_file(szAscBgraFileFullPath, 0, pSrcAscii, &u32FileSize);

    u32AsciiCnt = sizeof(stCtlAscElement) / sizeof(DSPTTK_ELEMENT_FEATURE);

    for (i = 0; i < u32AsciiCnt; i++)
    {
        stCtlAscElement[i].pAddr = pSrcAscii + u64AddrIdxSrc;
        u64AddrIdxSrc += stCtlAscElement[i].u32Width * GUI_BGRA_BIT_WIDTH * BG_SINGLE_ELEMENT_MAX_HEIGHT;
    }

    sRet = dspttk_disp_generate_bgImage(stScreenParam[u32Vodev].stScreenAttr->srcAddr, DISPLAY_DEVICE_WIDTH, DISPLAY_DEVICE_HEIGHT);

    dspttk_pthread_spawn(&g_FBInfoPthread, DSPTTK_PTHREAD_SCHED_OTHER, 0, 0x100000, (void *)dspttk_disp_operate_menu_info, 2, stScreenParam[u32Vodev].stScreenAttr->srcAddr, u32Vodev);
    return CMD_RET_MIX(HOST_CMD_DISP_FB_SHOW, sRet);
}

/**
 * @fn      dspttk_disp_fb_set
 * @brief   设置FrameBuffer参数,对背景图片进行绑定和show
 *
 * @param   [IN] u32Vodev 显示设备号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_fb_set(UINT32 u32Vodev)
{
    UINT64 sRet = CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, SAL_FAIL);

    if (u32Vodev > MAX_VODEV_CNT)
    {
        PRT_INFO("Param In Vodev Error. Vodev is %d. Supposed the Vodev in [0, %d).\n", u32Vodev, MAX_VODEV_CNT);
        return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, sRet);
    }

    // 当u32Vodev为2时,mmap为cursor;其他情况为GUI
    if (u32Vodev == SCREEN_LAYER_MAX - 1)
    {
        sRet = dspttk_cursor_fb_mmap(u32Vodev);
        sRet = CMD_RET_OF(sRet);
        if (sRet != SAL_SOK)
        {
            PRT_INFO("dspttk_cursor_fb_mmap error.");
            return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, sRet);
        }
    }
    else
    {
        sRet = dspttk_gui_fb_mmap(u32Vodev);
        sRet = CMD_RET_OF(sRet);
        if (sRet != SAL_SOK)
        {
            PRT_INFO("dspttk_gui_fb_mmap error.\n");
            return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, sRet);
        }

        sRet = dspttk_disp_fb_show(u32Vodev);
        sRet = CMD_RET_OF(sRet);
        if (sRet != SAL_SOK)
        {
            PRT_INFO("dspttk_disp_fb_show error.");
            return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, sRet);
        }
    }

    return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, sRet);
}

/**
 * @fn      dspttk_disp_fb_mouse_bind
 * @brief   设置鼠标参数
 *
 * @param   [IN] u32Vodev vodev号
 * @param   [IN] DSPTTK_SETTING_ENV 隐含入参，设置gui路径
 * @param   [IN] DSPTTK_SETTING_DISP 隐含入参，设置鼠标包括使能、坐标参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_fb_mouse_bind(UINT32 u32Vodev)
{

    SAL_STATUS sRet = SAL_FAIL;
    SCREEN_CTRL stScreen = {0};
    UINT32 u32BufSize = 0;
    DSPTTK_DEVCFG *pstDevCfg = NULL;
    DSPTTK_SETTING_ENV *pstCfgEnv = NULL;
    DSPTTK_SETTING_ENV_INPUT *pstInput = NULL;
    DSPTTK_SETTING_DISP *pstDispCfg = NULL;
    DSPTTK_INIT_PARAM *pstCfgInit = NULL;
    CHAR szCursorName[256] = {0};
    CHAR szSubString[256] = {0};
    CHAR *pSubString = NULL;
    pSubString = szSubString;

    if (u32Vodev > MAX_VODEV_CNT)
    {
        PRT_INFO("Vodev is %d Error.\n", u32Vodev);
        return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, sRet);
    }

    pstDevCfg = dspttk_devcfg_get();
    pstCfgEnv = &pstDevCfg->stSettingParam.stEnv;
    pstInput = &pstCfgEnv->stInputDir;
    pstDispCfg = &pstDevCfg->stSettingParam.stDisp;
    pstCfgInit = &pstDevCfg->stInitParam;

    stScreen.uiCnt = 1;
    stScreen.stScreenAttr[0].bitWidth = GUI_BIT_WIDTH;
    stScreen.stScreenAttr[0].x = 0;
    stScreen.stScreenAttr[0].y = 0;

    /* 首先获取ENV-SETTING页面下gui的相对路径，再将相对路径和文件名进行拼接 */
    memcpy(szCursorName, pstInput->gui, sizeof(szCursorName));
    strcat(szCursorName, GUI_CURSOR_BGRA);

    snprintf(szCursorName, sizeof(szCursorName), "%s%s", pstInput->gui, GUI_CURSOR_BGRA);

    // 从文件名中获取cursor的宽高
    pSubString = strstr(szCursorName, "_w");
    sRet = sscanf(pSubString, "%*[^w]w%u[^_]", &stScreen.stScreenAttr[0].width);
    sRet = sscanf(pSubString, "%*[^h]h%u[^_]", &stScreen.stScreenAttr[0].height);

    sRet = SendCmdToDsp(HOST_CMD_DISP_FB_MMAP, SCREEN_LAYER_MAX - 1, NULL, &stScreen);
    if (sRet != SAL_SOK)
    {
        return CMD_RET_MIX(HOST_CMD_DISP_FB_MMAP, sRet);
    }

    stScreen.stScreenAttr[0].x = pstDispCfg->stFBParam[0].stMousePos.x;
    stScreen.stScreenAttr[0].y = pstDispCfg->stFBParam[0].stMousePos.y;

    if (stScreen.stScreenAttr[0].x < 0 || stScreen.stScreenAttr[0].x > pstCfgInit->stDisplay.stVoDevAttr[u32Vodev].width ||
        stScreen.stScreenAttr[0].y < 0 || stScreen.stScreenAttr[0].y > pstCfgInit->stDisplay.stVoDevAttr[u32Vodev].height)
    {
        sRet = SAL_FAIL;
        PRT_INFO("Mouse Position Param Error, x is %d, y is %d.\n x should below %d, above 0. y should below %d, above 0.\n",
                 stScreen.stScreenAttr[0].x, stScreen.stScreenAttr[0].y, pstCfgInit->stDisplay.stVoDevAttr[u32Vodev].width,
                 pstCfgInit->stDisplay.stVoDevAttr[u32Vodev].height);
        return CMD_RET_MIX(HOST_CMD_DISP_FB_SHOW, sRet);
    }

    u32BufSize = dspttk_get_file_size(szCursorName);

    if (u32BufSize == 0)
    {
        PRT_INFO("dspttk_get_file_size Error. \n");
    }

    sRet = dspttk_read_file(szCursorName, 0, stScreen.stScreenAttr[0].srcAddr, &u32BufSize);
    if (sRet != SAL_SOK)
    {
        PRT_INFO("Read File Error.\n");
        return CMD_RET_MIX(HOST_CMD_DISP_FB_SHOW, sRet);
    }

    sRet = SendCmdToDsp(HOST_CMD_DISP_FB_SHOW, SCREEN_LAYER_MAX - 1, NULL, &stScreen);

    pSubString = NULL;
    return CMD_RET_MIX(HOST_CMD_DISP_FB_SHOW, sRet);
}

/**
 * @fn      dspttk_disp_vodev_win_set
 * @brief   设置输出窗口参数
 *
 * @param   [IN] u32Vodev vodev号
 * @param   [IN] DSPTTK_SETTING_DISP 隐含入参,设置窗口通道、初始坐标及窗口大小
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_vodev_win_set(UINT32 u32Num)
{
    SAL_STATUS sRet = SAL_SOK;
    DISP_CTRL stDispCtrl = {0};
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DISP *pstDispCfg = &pstDevCfg->stSettingParam.stDisp;
    UINT32 u32Vodev = 0, u32WinNum = 0, u32FillColor = 0, u32BorderColor = 0;
        CHAR cFillColorStr[50] = {0};
    CHAR cBorderColorStr[50] = {0};
    DSP_DISP_RCV_PARAM stDispRegion = {0};
    DISP_CLEAR_CTRL stClearCtrl = {0};

    u32Vodev = U32_FIRST_OF(u32Num);
    u32WinNum = U32_LAST_OF(u32Num);

    if (0 == pstDispCfg->stWinParam[u32Vodev][u32WinNum].enable)
    {
        stDispRegion.uiCnt = 1;
        stDispRegion.out[0] = u32WinNum;
        sRet = SendCmdToDsp(HOST_CMD_STOP_DISP, u32Vodev, NULL, &stDispRegion);
        if (sRet != SAL_SOK)
        {
            return CMD_RET_MIX(HOST_CMD_STOP_DISP, sRet);
        }

        stClearCtrl.uiChan[0] = u32WinNum;
        stClearCtrl.uiCnt = 1;
        sRet = SendCmdToDsp(HOST_CMD_DISP_CLEAR, u32Vodev, NULL, &stClearCtrl);
        return CMD_RET_MIX(HOST_CMD_DISP_CLEAR, sRet);
        
    }

    if (pstDispCfg->stLocalEnlargePrm[u32Vodev][u32WinNum].ratio)
    {
        return dspttk_disp_enlarge_local(u32Num);
    }

    if (pstDispCfg->stInSrcParam[u32Vodev][u32WinNum].enInSrcMode == DSPTTK_INPUT_SOURCE_VPSS)
    {
        return dspttk_disp_set_thumbnail(u32Vodev, u32WinNum);
    }

    if (u32Vodev > MAX_VODEV_CNT - 1)
    {
        PRT_INFO("Vodev Num Error, Error Num is %d, Supposed vodev in [0,1]\n", u32Vodev);
        return CMD_RET_MIX(HOST_CMD_ALLOC_DISP_REGION, sRet);
    }

    stDispCtrl.uiCnt = 1;
    stDispCtrl.stDispRegion[0].uiChan = pstDispCfg->stWinParam[u32Vodev][u32WinNum].stDispRegion.uiChan;
    stDispCtrl.stDispRegion[0].x = pstDispCfg->stWinParam[u32Vodev][u32WinNum].stDispRegion.x;
    stDispCtrl.stDispRegion[0].y = pstDispCfg->stWinParam[u32Vodev][u32WinNum].stDispRegion.y;
    stDispCtrl.stDispRegion[0].w = pstDispCfg->stWinParam[u32Vodev][u32WinNum].stDispRegion.w;
    stDispCtrl.stDispRegion[0].h = pstDispCfg->stWinParam[u32Vodev][u32WinNum].stDispRegion.h;
    PRT_INFO("Vodev %u, u32WinNum %u, w %u, h %u\n", u32Vodev, u32WinNum, stDispCtrl.stDispRegion[0].w, stDispCtrl.stDispRegion[0].h);
    stDispCtrl.stDispRegion[0].bDispBorder = pstDispCfg->stWinParam[u32Vodev][u32WinNum].stDispRegion.bDispBorder;
    stDispCtrl.stDispRegion[0].BorDerLineW = pstDispCfg->stWinParam[u32Vodev][u32WinNum].stDispRegion.BorDerLineW;

    if (pstDispCfg->stWinColorPrm[u32Vodev][u32WinNum].cFillWinColor[0] == '#')
    {
        strcpy(cFillColorStr, "ff");
        strcat(cFillColorStr, &pstDispCfg->stWinColorPrm[u32Vodev][u32WinNum].cFillWinColor[1]);
    }
    else
    {
        strcpy(cFillColorStr, "ff");
        strcat(cFillColorStr, &pstDispCfg->stWinColorPrm[u32Vodev][u32WinNum].cFillWinColor[0]);
    }
    u32FillColor = strtoul(cFillColorStr, NULL, 16);
    stDispCtrl.stDispRegion[0].color = u32FillColor;

    if (pstDispCfg->stWinColorPrm[u32Vodev][u32WinNum].cBorderLineColor[0] == '#')
    {
        strcpy(cBorderColorStr, "ff");
        strcat(cBorderColorStr, &pstDispCfg->stWinColorPrm[u32Vodev][u32WinNum].cBorderLineColor[1]);
    }
    else
    {
        strcpy(cBorderColorStr, "ff");
        strcat(cBorderColorStr, &pstDispCfg->stWinColorPrm[u32Vodev][u32WinNum].cBorderLineColor[0]);
    }
    u32BorderColor = strtoul(cBorderColorStr, NULL, 16);
    stDispCtrl.stDispRegion[0].BorDerColor = u32BorderColor;

    sRet = SendCmdToDsp(HOST_CMD_ALLOC_DISP_REGION, u32Vodev, NULL, &stDispCtrl);
    if (sRet != SAL_SOK)
    {
        PRT_INFO("HOST_CMD_ALLOC_DISP_REGION Error. sRet is %d.\n", sRet);
        return CMD_RET_MIX(HOST_CMD_ALLOC_DISP_REGION, SAL_FAIL);
    }

    return CMD_RET_MIX(HOST_CMD_ALLOC_DISP_REGION, sRet);
}

/**
 * @fn      dspttk_disp_input_src_set
 * @brief   设置输入源参数
 *
 * @param   [IN] u32Num 高16位输出设备号,低16位窗口号
 * @param   [IN] DSPTTK_SETTING_DISP 隐含入参,获取VI通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_input_src_set(UINT32 u32Num)
{
    UINT64 sRet = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DISP *pstDispCfg = &pstDevCfg->stSettingParam.stDisp;
    UINT32 u32Vodev = 0, u32WinNo = 0;

    u32Vodev = U32_FIRST_OF(u32Num);
    u32WinNo = U32_LAST_OF(u32Num);

    if (u32Vodev > MAX_VODEV_CNT - 1 || u32WinNo > MAX_DISP_CHAN - 1)
    {
        PRT_INFO("Vodev Num Or Win Num Error. Vodev Num is %d, Supposed to be 0 or 1. WinNo is %d , Supposed the WinNo in [0,16)\n", u32Vodev, u32WinNo);
        return CMD_RET_MIX(HOST_CMD_SET_VI_DISP, SAL_FAIL);
    }

    switch (pstDispCfg->stInSrcParam[u32Vodev][u32WinNo].enInSrcMode)
    {
    case DSPTTK_INPUT_SOURCE_VI:
    {
        sRet = dspttk_disp_set_vi(u32Vodev, u32WinNo);
    }
    break;

    case DSPTTK_INPUT_SOURCE_DECODE:
    {
        sRet = dspttk_disp_set_dec(u32Vodev, u32WinNo);
    }
    break;

    case DSPTTK_INPUT_SOURCE_VPSS:
    {

        sRet = dspttk_disp_set_thumbnail(u32Vodev, u32WinNo);
    }
    break;

    default:
        PRT_INFO("INPUT SOURCE MODE ERROR! Current mode num is %d. Please refer to enum DSPTTK_INPUT_SOURCE!\n", pstDispCfg->stInSrcParam[u32Vodev][u32WinNo].enInSrcMode);
        return CMD_RET_MIX(HOST_CMD_ALLOC_DISP_REGION, SAL_FAIL);
        break;
    }

    return sRet;
}

/**
 * @fn      dspttk_disp_enlarge_global
 * @brief   设置全局放大参数
 *
 * @param   [IN] DSPTTK_DEVCFG 获取Disp默认配置
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_enlarge_global(UINT32 u32Num)
{
    SAL_STATUS sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DISP *pstDispCfg = &pstDevCfg->stSettingParam.stDisp;
    DISP_DPTZ stGlobalEnlargeCtl = {0};
    UINT32 u32Vodev = 0, u32Chan = 0;
    u32Vodev = U32_FIRST_OF(u32Num);
    u32Chan = U32_LAST_OF(u32Num);

    if ((u32Vodev > MAX_VODEV_CNT - 1) || (u32Chan > MAX_DISP_CHAN - 1))
    {
        PRT_INFO("ParamIn error. vodev is %d, channel is %d\n", u32Vodev, u32Chan);
        return CMD_RET_MIX(HOST_CMD_DPTZ_DISP_PRM, SAL_FAIL);
    }

    stGlobalEnlargeCtl.mode = DPTZ_ZOOM_GLOBAL;
    stGlobalEnlargeCtl.voDev = u32Vodev;
    stGlobalEnlargeCtl.voChn = u32Chan;
    stGlobalEnlargeCtl.x = pstDispCfg->stGlobalEnlargePrm[u32Vodev][u32Chan].x;
    stGlobalEnlargeCtl.y = pstDispCfg->stGlobalEnlargePrm[u32Vodev][u32Chan].y;
    stGlobalEnlargeCtl.ratio = pstDispCfg->stGlobalEnlargePrm[u32Vodev][u32Chan].ratio;

    sRet = SendCmdToDsp(HOST_CMD_DPTZ_DISP_PRM, u32Vodev, NULL, &stGlobalEnlargeCtl);

    return CMD_RET_MIX(HOST_CMD_DPTZ_DISP_PRM, sRet);
}

/**
 * @fn      dspttk_disp_enlarge_local
 * @brief   设置局部放大参数
 *
 * @param   [IN] DSPTTK_DEVCFG 获取Disp默认配置
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_enlarge_local(UINT32 u32Num)
{
    SAL_STATUS sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DISP *pstDispCfg = &pstDevCfg->stSettingParam.stDisp;
    DISP_DPTZ stDispCtl = {0};
    DISP_POS_CTRL stDispPosCtl = {0};
    UINT32 u32Vodev = 0, u32Chan = 0, u32VoChn = 0, u32InputSrcChan = 0;

    u32Vodev = U32_FIRST_OF(u32Num);
    u32Chan = U32_LAST_OF(u32Num);
    u32InputSrcChan = pstDispCfg->stInSrcParam[u32Vodev][u32Chan].u32SrcChn;

    if ((u32Vodev > MAX_VODEV_CNT - 1) || (u32Chan > MAX_DISP_CHAN - 1))
    {
        PRT_INFO("ParamIn error. vodev is %d, channel is %d\n", u32Vodev, u32Chan);
        return CMD_RET_MIX(HOST_CMD_DPTZ_DISP_PRM, SAL_FAIL);
    }

    

    u32VoChn = u32Chan;

    stDispPosCtl.voDev = u32Vodev;
    stDispPosCtl.voChn = u32VoChn;
    
    stDispPosCtl.stDispRegion.w = pstDispCfg->stWinParam[u32Vodev][u32Chan].stDispRegion.w; // 新开的窗口宽
    stDispPosCtl.stDispRegion.h = pstDispCfg->stWinParam[u32Vodev][u32Chan].stDispRegion.h; // 新开的窗口高

    stDispPosCtl.stDispRegion.x = pstDispCfg->stLocalEnlargePrm[u32Vodev][u32Chan].x > stDispPosCtl.stDispRegion.w / 2 ?
                                    ((pstDispCfg->stLocalEnlargePrm[u32Vodev][u32Chan].x + stDispPosCtl.stDispRegion.w / 2) > 
                                        pstDispCfg->stWinParam[u32Vodev][u32InputSrcChan].stDispRegion.w ?  
                                        (pstDispCfg->stWinParam[u32Vodev][u32InputSrcChan].stDispRegion.w - stDispPosCtl.stDispRegion.w)
                                        : pstDispCfg->stLocalEnlargePrm[u32Vodev][u32Chan].x) : 0;  // 窗口的左上角x坐标
    pstDispCfg->stWinParam[u32Vodev][u32Chan].stDispRegion.x = stDispPosCtl.stDispRegion.x;

    stDispPosCtl.stDispRegion.y = pstDispCfg->stLocalEnlargePrm[u32Vodev][u32Chan].y > stDispPosCtl.stDispRegion.h / 2 ?
                                    ((pstDispCfg->stLocalEnlargePrm[u32Vodev][u32Chan].y + stDispPosCtl.stDispRegion.h / 2) > 
                                        pstDispCfg->stWinParam[u32Vodev][u32InputSrcChan].stDispRegion.h ?  
                                        (pstDispCfg->stWinParam[u32Vodev][u32InputSrcChan].stDispRegion.h - stDispPosCtl.stDispRegion.h)
                                        : pstDispCfg->stLocalEnlargePrm[u32Vodev][u32Chan].y) : 0;  // 窗口的左上角y坐标
    pstDispCfg->stWinParam[u32Vodev][u32Chan].stDispRegion.y = stDispPosCtl.stDispRegion.y;


    stDispPosCtl.stDispRegion.uiChan = pstDispCfg->stInSrcParam[u32Vodev][u32Chan].u32SrcChn;

    stDispCtl.voDev = u32Vodev;
    stDispCtl.voChn = u32VoChn;
    stDispCtl.x = pstDispCfg->stLocalEnlargePrm[u32Vodev][u32Chan].x;   // 放大中心点x坐标
    stDispCtl.y = pstDispCfg->stLocalEnlargePrm[u32Vodev][u32Chan].y;   // 放大中心点y坐标

    switch((UINT32) pstDispCfg->stLocalEnlargePrm[u32Vodev][u32Chan].ratio)
    {
        case 1:
            stDispCtl.ratio = 1.25;
            break;
        
        case 2:
            stDispCtl.ratio = 1.5;
            break;
        
        case 3:
            stDispCtl.ratio = 1.75;
            break;
        
        default:
            stDispCtl.ratio = 0;
            break;
    }

    stDispCtl.w = stDispPosCtl.stDispRegion.w;
    stDispCtl.h = stDispPosCtl.stDispRegion.h;
    stDispCtl.mode = DPTZ_ZOOM_LOCAL;
    stDispPosCtl.enable =  stDispCtl.ratio == 0 ? SAL_FALSE : SAL_TRUE;

    sRet = SendCmdToDsp(HOST_CMD_DPTZ_DISP_PRM, u32Vodev, NULL, &stDispCtl);

    if (SAL_SOK != sRet)
    {
        PRT_INFO("HOST_CMD_DPTZ_DISP_PRM failed. channel is %d.\n", u32Vodev);
        return CMD_RET_MIX(HOST_CMD_DPTZ_DISP_PRM, sRet);
    }

    sRet = SendCmdToDsp(HOST_CMD_PIP_DISP_REGION, u32Vodev, NULL, &stDispPosCtl);

    return CMD_RET_MIX(HOST_CMD_PIP_DISP_REGION, sRet);
}

SAL_STATUS dspttk_get_bmp_width_height(CHAR *pFileName, UINT32 *u32Width, UINT32 *u32Height)
{
    SAL_STATUS sRet = 0;
    UINT32 u32FileSize = BMP_HEAD_INFO_WIDTH_HEIGHT_BITWIDTH;

    sRet = dspttk_read_file(pFileName, BMP_HEAD_INFO_WIDTH, u32Width, &u32FileSize);

    sRet = dspttk_read_file(pFileName, BMP_HEAD_INFO_HEIGHT, u32Height, &u32FileSize);

    return sRet;
}

/**
 * @fn      dspttk_disp_set_init
 * @brief   对Display Setting作初始化
 *
 * @param   [IN] DSPTTK_DEVCFG 获取Disp默认配置
 *
 * @return  UINT32 32位表示错误号
 */
SAL_STATUS dspttk_disp_set_init(DSPTTK_DEVCFG *pstDevCfg)
{
    DSPTTK_SETTING_DISP *pstDispCfg = &pstDevCfg->stSettingParam.stDisp;
    DSPTTK_SETTING_ENV_INPUT *pstEnvInput = &pstDevCfg->stSettingParam.stEnv.stInputDir;
    UINT64 sRet = CMD_RET_MIX(HOST_CMD_ALLOC_DISP_REGION, SAL_SOK);
    SAL_STATUS u32Ret = SAL_SOK;
    UINT32 i = 0, j = 0, u32PicWidth = 0, u32PicHeight = 0, u32Combine = 0;
    CHAR szBMPFileFullPath[MAX_FILENAME_SIZE] = {0};
    CHAR szAsciiBGRAFullPath[MAX_FILENAME_SIZE] = {0};

    if (NULL == pstDevCfg)
    {
        PRT_INFO("pstDevCfg is NULL\n");
        return SAL_FALSE;
    }

    snprintf(szBMPFileFullPath, sizeof(szBMPFileFullPath), "%s%s", pstEnvInput->gui, GUI_ASCII_SRC_BMP);
    snprintf(szAsciiBGRAFullPath, sizeof(szAsciiBGRAFullPath), "%s%s", pstEnvInput->gui, GUI_ASCII_SRC_BGRA);

    u32Ret = dspttk_get_bmp_width_height(szBMPFileFullPath, &u32PicWidth, &u32PicHeight);
    u32Ret = dspttk_get_element_feature(szBMPFileFullPath, u32PicWidth, u32PicHeight);

    u32Ret = access(szAsciiBGRAFullPath, F_OK);   //判断bgra文件是否存在, 若不存在需要重新生成
    if (SAL_SOK != u32Ret)
    {
        PRT_INFO("%s not exist. generate it from %s\n", szAsciiBGRAFullPath, szBMPFileFullPath);
        u32Ret = dspttk_disp_get_ascii_src_from_bmp(szBMPFileFullPath, szAsciiBGRAFullPath, u32PicWidth);
        if (SAL_SOK != u32Ret)
        {
            PRT_INFO("dspttk_disp_get_ascii_src_from_bmp error. BMP path %s, BGRA path %s, PicWidth %d.\n", szBMPFileFullPath, szAsciiBGRAFullPath, u32PicWidth);
            return u32Ret;
        }
    }

    if (pstDevCfg == NULL)
    {
        PRT_INFO("Param Error. pstDevCfg is %p.", pstDevCfg);
        return SAL_FAIL;
    }

    for (i = 0; i < MAX_VODEV_CNT; i++)
    {
        if (pstDispCfg->stFBParam[i].bFBEn)
        {
            sRet = dspttk_disp_fb_set(i);
            if (CMD_RET_OF(sRet) != SAL_SOK)
            {
                PRT_INFO("dspttk_disp_fb_set error! vodev num is %d\n", i);
                return CMD_RET_OF(sRet);
            }
        }

        if (pstDispCfg->stFBParam[i].bMouseEn)
        {
            sRet = dspttk_disp_fb_mouse_bind(i);
            if (CMD_RET_OF(sRet) != SAL_SOK)
            {
                PRT_INFO("dspttk_disp_fb_mouse_bind error! vodev num is %d\n", i);
                return CMD_RET_OF(sRet);
            }
        }

        for (j = 0; j < MAX_DISP_CHAN; j++)
        {
            if (SAL_TRUE == pstDispCfg->stWinParam[i][j].enable)
            {
                U32_COMBINE(i, j, u32Combine);
                sRet = dspttk_disp_vodev_win_set(u32Combine);
                if (CMD_RET_OF(sRet) != SAL_SOK)
                {
                    PRT_INFO("dspttk_disp_vodev_win_set error! vodev num is %d, win num is %d.\n", i, j);
                    return CMD_RET_OF(sRet);
                }
                sRet = dspttk_disp_input_src_set(u32Combine);
                if (CMD_RET_OF(sRet) != SAL_SOK)
                {
                    PRT_INFO("dspttk_disp_input_src_set error! vodev num is %d, win num is %d.\n", i, j);
                    return CMD_RET_OF(sRet);
                }
            }
        }
    }

    return CMD_RET_OF(sRet);
}
