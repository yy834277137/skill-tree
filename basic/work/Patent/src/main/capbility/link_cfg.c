/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: link_cfg.c
   Description: 不同产品可在该文件中进行配置，对系统中的模块进行绑定，
   从而搭建起产品特有的数据流
   Author: yeyanzhong
   Date: 2020.12.10
   Modification History:
        1、20220815，节点配置信息从link_task.c里拆分到capability目录下的link_cfg.c
******************************************************************************/
#include "sal.h"
#include "platform_hal.h"
#include "system_prm_api.h"
#include "link_task_api.h"
#include "link_drv_api.h"


/* 分析仪里视频从左到右时，用于包裹抓图的镜像处理 */
INST_CFG_S stVpDupInstCfg = 
{
    .szInstPreName = "DUP_VP", /* 实例名字的前缀，可以再加上后缀，比如通道号 */
    .u32NodeNum = 3, /* 这里节点个数指的是下面紧接着的NodeCfg*/
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "out_jpeg", NODE_BIND_TYPE_GET},
		{OUT_NODE_1, "out_yuv",   NODE_BIND_TYPE_GET},
    },
};

#ifdef NT98336  /* 当前只在安检机里使用，安检盒子不用解码 */
INST_CFG_S stVdecDupInstCfg = 
{
    .szInstPreName = "DUP_VDEC", /* 实例名字的前缀，可以再加上后缀，比如多路解码的话，可以加解码通道号 */
    .u32NodeNum = 5,
    .stNodeCfg = 
    {

        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_GET},  /* 从vdec获取送入vpss */
        {OUT_NODE_0, "out_disp0", NODE_BIND_TYPE_GET},
        {OUT_NODE_1, "out_disp1", NODE_BIND_TYPE_GET},
        {OUT_NODE_2, "out_jpeg",  NODE_BIND_TYPE_GET},
        {OUT_NODE_3, "out_ba",    NODE_BIND_TYPE_GET},
    },
};
#elif (defined(HI3559A_SPC030) || defined(RK3588) || defined(ss928))
INST_CFG_S stVdecDupInstCfg = 
{
    .szInstPreName = "DUP_VDEC", /* 实例名字的前缀，可以再加上后缀，比如多路解码的话，可以加解码通道号 */
    .u32NodeNum = 5,
    .stNodeCfg = 
    {

        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND}, /* vdec与vpss之间绑定 */
        {OUT_NODE_0, "out_disp0", NODE_BIND_TYPE_GET},
        {OUT_NODE_1, "out_disp1", NODE_BIND_TYPE_GET},
        {OUT_NODE_2, "out_jpeg",  NODE_BIND_TYPE_GET},  /*jpeg编码 和人脸 都会从该通道获取*/
        {OUT_NODE_3, "out_ba",    NODE_BIND_TYPE_GET},
    },
};
#else
    /* 新增平台时这里需要新增配置信息 */
#endif

/* ss928里vpss同一个时刻只有一个chan能做放大，所以需要新增一个FRONT VPSS做放大 */
INST_CFG_S stVdecFrontDupInstCfg = 
{
    .szInstPreName = "DUP_VDEC_FRONT", /* 实例名字的前缀，可以再加上后缀，比如多路解码的话，可以加解码通道号 */
    .u32NodeNum = 2,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "out_dupRear", NODE_BIND_TYPE_SDK_BIND},
    },
};


/* 人包关联使用的标定解码通道 */
INST_CFG_S stPpmVdecDupInstCfg = 
{
    .szInstPreName = "DUP_VDEC", /* 实例名字的前缀，可以再加上后缀，比如多路解码的话，可以加解码通道号 */
    .u32NodeNum = 5,
    .stNodeCfg = 
    {

        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "out_0", NODE_BIND_TYPE_GET},
        {OUT_NODE_1, "out_1", NODE_BIND_TYPE_GET},
        {OUT_NODE_2, "out_2",  NODE_BIND_TYPE_GET},
        {OUT_NODE_3, "out_3",    NODE_BIND_TYPE_GET},
    },
};

#if (defined(HI3559A_SPC030) && (defined(DSP_ISA) || defined(DSP_ISM)))
INST_CFG_S stFrontDupInstCfg = 
{
    .szInstPreName = "DUP_FRONT", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 5,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",        NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "out_dupRear", NODE_BIND_TYPE_SDK_BIND}, /* 分析仪里绑定后级；安检机里用于局部放大 */
        {OUT_NODE_1, "out_disp0", NODE_BIND_TYPE_GET},
        {OUT_NODE_2, "out_disp1", NODE_BIND_TYPE_GET},
        {OUT_NODE_3, "out_sva",   NODE_BIND_TYPE_GET},
    },
};

INST_CFG_S stRearDupInstCfg = 
{
    .szInstPreName = "DUP_REAR", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 5,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "out_venc0",   NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_1, "out_venc1",   NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_2, "out_jpeg",    NODE_BIND_TYPE_GET}, 
        {OUT_NODE_3, "out_thumbnail",   NODE_BIND_TYPE_GET},
    },
};

#elif (defined(NT98336) && defined(DSP_ISM))
INST_CFG_S stFrontDupInstCfg = 
{
    .szInstPreName = "DUP_FRONT", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 4,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",          NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "out_dupRear", NODE_BIND_TYPE_GET}, //局部放大
        {OUT_NODE_1, "out_disp0", NODE_BIND_TYPE_GET}, //NODE_BIND_TYPE_SDK_BIND
        {OUT_NODE_2, "out_disp1",   NODE_BIND_TYPE_GET},
        //{OUT_NODE_3, "out_thumbnail",   NODE_BIND_TYPE_GET},
    },
};

INST_CFG_S stRearDupInstCfg = 
{
    .szInstPreName = "DUP_REAR", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 4,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "out_venc0",   NODE_BIND_TYPE_GET},
        {OUT_NODE_1, "out_venc1",   NODE_BIND_TYPE_GET},
        {OUT_NODE_2, "out_thumbnail",   NODE_BIND_TYPE_GET}, //DUP_FRONT含有框，后续可调整到DUP_REAR
        //{OUT_NODE_2, "out_jpeg",    NODE_BIND_TYPE_GET},     //暂时没有用到，先注释掉

    },
};
        
#elif (defined(SS928) && defined(DSP_ISA))  /*ss928里vpss性能不够，第二个vpss没有绑定到第一个vpss */
INST_CFG_S stFrontDupInstCfg = 
{
    .szInstPreName = "DUP_FRONT", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 5,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",          NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "sva", NODE_BIND_TYPE_GET}, 
        {OUT_NODE_1, "out_disp0", NODE_BIND_TYPE_GET}, 
        {OUT_NODE_2, "out_disp1",   NODE_BIND_TYPE_GET},
        {OUT_NODE_3, "out_venc0", NODE_BIND_TYPE_SDK_BIND},
    },
};
#elif (defined(RK3588) && defined(DSP_ISM))
INST_CFG_S stFrontDupInstCfg = 
{
    .szInstPreName = "DUP_FRONT", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 4,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND},

        {OUT_NODE_0, "out_disp0", NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_1, "out_disp1", NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_2, "out_dupRear", NODE_BIND_TYPE_SDK_BIND}, //局部放大           
    },
};

INST_CFG_S stRearDupInstCfg = 
{
    .szInstPreName = "DUP_REAR", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 5,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "out_venc0", NODE_BIND_TYPE_SDK_BIND}, /* 对应的vpss chan会被改成USER模式，因为虚宽超过了venc使用的RGA的限制 */
        {OUT_NODE_1, "out_venc1", NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_2, "out_thumbnail",   NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_3, "out_jpeg",  NODE_BIND_TYPE_GET},
    },
};

#elif (defined(RK3588) && defined(DSP_ISA))
INST_CFG_S stFrontDupInstCfg = 
{
    .szInstPreName = "DUP_FRONT", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 5,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND},

        {OUT_NODE_0, "out_dupRear", NODE_BIND_TYPE_SDK_BIND}, //用于绑定后级DUP
        {OUT_NODE_1, "out_disp0", NODE_BIND_TYPE_GET},
        {OUT_NODE_2, "out_disp1", NODE_BIND_TYPE_GET},
        {OUT_NODE_3, "out_sva",   NODE_BIND_TYPE_GET},
    },
};

INST_CFG_S stRearDupInstCfg = 
{
    .szInstPreName = "DUP_REAR", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 4,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "out_venc0", NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_1, "out_venc1", NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_2, "out_jpeg",  NODE_BIND_TYPE_GET},
    },
};
#else
    /* 新增平台时这里需要新增配置信息 */
#endif

/* 实例名字用大写，节点名字用小写; astBinder_isa_s表示分析仪单路的绑定关系*/
static BIND_S astBinder_isa_s[] =  /* 单路，安检机和分析仪公用 */
{
    /* 源实例，源节点，目标实例，目标节点*/

    /*
    {"VI0",                 "out_0",                "DUP0",                     "in_0"},
    {"DUP_FRONT_0",         "out_dupRear",          "DUP_REAR_0",               "in_0"},   // 前后级两个DUP的绑定关系. 创建DUP时无法使用该绑定关系，因为该绑定关系还未建立
    */
    {"DUP_REAR_0",         "out_venc0",            "VENC_INDEV0_STRM0",        "in_0"},
    {"DUP_REAR_0",         "out_venc1",            "VENC_INDEV0_STRM1",        "in_0"}, /* ss928里由于vpss性能不够所以这一路其实没有数据 */
    {"DUP_REAR_0",         "out_jpeg",             "VENC_JPG0",                "in_0"}, /* ss928里由于vpss性能不够没有单独的VPSS chan获取帧用于JPEG */
    /*    
    {"DUP_REAR_0",          "out_disp0",            "DISP0_CHN0",               "in_0"},   //在app下发命令时绑定
    {"DUP_REAR_0",          "out_disp1",            "DISP1_CHN0",               "in_0"},   //在app下发命令时绑定
    {"DUP_REAR_0",          "out_sva",              "SVA0",                     "in_0"},   */
};

/* 实例名字用大写，节点名字用小写；astBinder_isa_d表示分析仪双路的绑定关系 */
static BIND_S astBinder_isa_d[] =   /* 双路，安检机和分析仪公用 */
{
    /* 源实例，源节点，目标实例，目标节点*/

    {"DUP_REAR_0",         "out_venc0",            "VENC_INDEV0_STRM0",        "in_0"},
    {"DUP_REAR_0",         "out_venc1",            "VENC_INDEV0_STRM1",        "in_0"},
    {"DUP_REAR_0",         "out_jpeg",             "VENC_JPG0",                "in_0"},

    {"DUP_REAR_1",         "out_venc0",             "VENC_INDEV1_STRM0",       "in_0"},
    {"DUP_REAR_1",         "out_venc1",             "VENC_INDEV1_STRM1",       "in_0"},
    {"DUP_REAR_1",         "out_jpeg",              "VENC_JPG1",               "in_0"},

};


/*安检盒子*/
static INST_CFG_S stFrontDupInstBoxCfg = 
{
    .szInstPreName = "DUP_FRONT", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 5,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",            NODE_BIND_TYPE_SDK_BIND},
        {OUT_NODE_0, "out_venc0",       NODE_BIND_TYPE_SDK_BIND}, //局部放大
        {OUT_NODE_1, "out_venc1",       NODE_BIND_TYPE_SDK_BIND}, //NODE_BIND_TYPE_SDK_BIND
        {OUT_NODE_2, "out_jpeg",        NODE_BIND_TYPE_GET},
        {OUT_NODE_3, "out_sva",         NODE_BIND_TYPE_GET},
    },
};


/*安检盒子*/
static INST_CFG_S stRearDupInstBoxCfg = 
{
    .szInstPreName = "DUP_REAR", /* 实例名字的前缀，可以再加上后缀，比如两路输入的话，可以加输入通道号 */
    .u32NodeNum = 3,
    .stNodeCfg = 
    {
        {IN_NODE_0,  "in_0",      NODE_BIND_TYPE_SDK_BIND},
    },
};
/* 安检盒子 实例名字用大写，节点名字用小写; astBinder_isabox_s表示分析仪单盒子路的绑定关系*/
static BIND_S astBinder_isabox_s[] =
{
    /* 源实例，源节点，目标实例，目标节点*/
    {"DUP_FRONT_0",         "out_venc0",            "VENC_INDEV0_STRM0",        "in_0"},
    {"DUP_FRONT_0",         "out_venc1",            "VENC_INDEV0_STRM1",        "in_0"}, 

};

/*
*   Function:   link_task_getDupInstCfg
*   Description: 获取dup实例inst配置信息
*   Input:
*   Output: pFrontDupInstCfg  前级DUP配置信息
*           pRearDupInstCfg   后级DUP配置信息
*   Return:   OK or ERR Information
*/
INT32 link_task_getDupInstCfg(INST_CFG_S **pFrontDupInstCfg, INST_CFG_S **pRearDupInstCfg)
{
    UINT32  u32BoardType = HARDWARE_GetBoardType();

    if(u32BoardType == DB_RS20032_V1_0)
    {
       *pFrontDupInstCfg = &stFrontDupInstBoxCfg;
       *pRearDupInstCfg  = &stRearDupInstBoxCfg;
    }
    else
    {
       *pFrontDupInstCfg = &stFrontDupInstCfg;
       *pRearDupInstCfg  = &stRearDupInstCfg;
    }

    return SAL_SOK;
}

/*
*   Function:   link_cfg_getBinderInfo
*   Description: 获取节点间的绑定信息
*   Input:
*   Output: ppstBinderInfo  绑定信息
*           pu32BinderCnt   绑定信息的条例个数
*   Return:   OK or ERR Information
*/
INT32 link_cfg_getBinderInfo(BIND_S **ppstBinderInfo, UINT32 *pu32BinderCnt)
{
    UINT32 u32Cnt = 0;
    BIND_S *pstBinderInfo  =  NULL;


    DSPINITPARA *pstDspParam = SystemPrm_getDspInitPara();
    UINT32  u32BoardType = HARDWARE_GetBoardType();

    if(u32BoardType == DB_RS20032_V1_0)
    {
         u32Cnt = sizeof(astBinder_isabox_s) / sizeof(astBinder_isabox_s[0]);
         pstBinderInfo = astBinder_isabox_s;
    }
    else
    {
        if (1 == pstDspParam->stViInitInfoSt.uiViChn)
        {
            u32Cnt = sizeof(astBinder_isa_s) / sizeof(astBinder_isa_s[0]);
            pstBinderInfo = astBinder_isa_s;
        }
        else if (2 == pstDspParam->stViInitInfoSt.uiViChn)
        {
            u32Cnt = sizeof(astBinder_isa_d) / sizeof(astBinder_isa_d[0]);
            pstBinderInfo = astBinder_isa_d;
        }
        else
        {
            LINK_LOGE("uiViChn %d is illegal \n", pstDspParam->stViInitInfoSt.uiViChn);
            return SAL_FAIL;
        }
    }
    *pu32BinderCnt = u32Cnt;
    *ppstBinderInfo = pstBinderInfo;

    return SAL_SOK;

}

