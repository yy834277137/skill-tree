/*******************************************************************************
* icf_common.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2022年3月18日 Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef _ICF_COMMON_H_
#define _ICF_COMMON_H_

#include "sal.h"
#include "ia_common.h"

#define ENGINE_HAL_CHECK_RET(s32Ret, loop, str) \
    { \
        if (s32Ret) \
        { \
            SAL_LOGE("%s, ret: 0x%x \n", str, s32Ret); \
            goto loop; \
        } \
    }

#define ENGINE_HAL_CHECK_RET_NO_LOOP(s32Ret, str) \
    { \
        if (s32Ret) \
        { \
            SAL_LOGE("%s, ret: 0x%x \n", str, s32Ret); \
        } \
    }

#define ENGINE_COMM_CHECK_CTRL_PRM(func, p, loop) \
    { \
        if (NULL == func) \
        { \
            SAL_LOGE("ctrl func null! \n"); \
            goto loop; \
        } \
        if (NULL == p) \
        { \
            SAL_LOGE("ctrl data p null! \n"); \
            goto loop; \
        } \
    }

#define ENGINE_COMM_GET_NODE_CTRL_PRM_CHECK(node, func, p, loop) \
    { \
        if (NULL == func) \
        { \
            SAL_LOGE("node cb func null! node: %d \n", node); \
            goto loop; \
        } \
        if (NULL == p) \
        { \
            SAL_LOGE("user buf null! node: %d \n", node); \
            goto loop; \
        } \
    }

#define ENGINE_COMM_SQUARE_VAR(a)	(a * a)       /* 平方数 */
#define ENGINE_COMM_GET_ABS(a, b)	(a < b ? b - a : a - b)

#define ENGINE_COMM_MAX_USER_CHN_NUM			(2)        /* engine封装层,支持最大创建的引擎通道个数，暂定2 */
#define ENGINE_COMM_INPUT_DATA_IMG_MAX_NUM		(16)       /* engine封装层,单次输入最大图片个数 */
#define ENGINE_COMM_INPUT_VIEW_NUM				(2)        /* engine封装层,最大视角个数，数据维度 */
#define ENGINE_COMM_INPUT_SINGLE_VIEW_IMG_NUM	(1)        /* engine封装层,单个视角最大图像个数，数据维度 */
#define ENGINE_COMM_MAX_RSLT_NODE_NUM			(16)       /* engine封装层,单次获取算法节点结果时的最大节点个数，当前默认单次调用获取单个节点信息 */

typedef enum _ENGINE_COMM_CALLBACK_TYPE_
{
    ENGINE_CALLBACK_OUTPUT         = 0,
    ENGINE_CALLBACK_RELEASE_SOURCE = 1,
    ENGINE_CALLBACK_RELEASE_ALG    = 2,
    ENGINE_CALLBACK_ABNORMAL_ALARM = 3,
    ENGINE_CALLBACK_END,
} ENGINE_COMM_CALLBACK_TYPE_E;

typedef enum _ENGINE_COMM_NODE_TYPE_
{
	/* 非法节点类型 */
	ENGINE_COMM_INVALID_NODE = 0,
	/* xsi节点 */
    ENGINE_COMM_XSI_NODE = 1,
} ENGINE_COMM_NODE_TYPE_E;

typedef struct _ENGINE_COMM_INIT_CFG_FILE_
{
    char *pAlgCfgPath;                 /* 算法配置文件路径 */
    char *pTaskCfgPath;                /* 算子任务配置文件路径 */
    char *pToolkitCfgPath;             /* 框架工具配置文件路径 */
} ENGINE_COMM_INIT_CFG_FILE_S;

typedef void (*ENGINE_COMM_RSLT_CB_FUNC)(unsigned int nNodeID,
			                             int nCallBackType,
			                             void *pstOutput,
			                             unsigned int nSize,
			                             void *pUsr,
			                             int nUserSize);

typedef INT32 (*ENGINE_COMM_MEM_ALLOC_CB_FUNC)(IA_MEM_TYPE_E enMemType,/* TODO: 后续取消使用ia_common.c中的数据结构 */
                                               UINT32 u32MemSize,
                                               IA_MEM_PRM_S *pstMemBuf);

typedef INT32 (*ENGINE_COMM_MEM_RLS_CB_FUNC)(IA_MEM_TYPE_E enMemType,
                                             UINT32 u32MemSize,
                                             IA_MEM_PRM_S *pstMemBuf);

typedef struct _ENGINE_COMM_INIT_MEM_CONFIG_
{
    UINT32 u32MemSize[IA_MEM_TYPE_NUM];               /* 初始化的内存信息，Byte */
    ENGINE_COMM_MEM_ALLOC_CB_FUNC pMemAllocFunc;         /* 内存申请回调接口 */
    ENGINE_COMM_MEM_RLS_CB_FUNC pMemRlsFunc;             /* 内存释放回调接口 */
} ENGINE_COMM_INIT_MEM_CONFIG;

typedef struct _ENGINE_COMM_CHN_ATTR_
{
    char *pAppParamCfgPath;               /* 引擎应用层配置文件 */
	UINT32 u32GraphType;                  /* graph类型 */
	UINT32 u32GraphId;                    /* graph Id*/
    UINT32 u32NodeId;                     /* 节点id */
    ENGINE_COMM_CALLBACK_TYPE_E u32CallBackType;  /* 结果回调类型 */
} ENGINE_COMM_CHN_ATTR;

typedef struct _ENGINE_COMM_INIT_CHNL_OUTPUT_
{
    UINT32 u32ChnNum;                                /* 创建的通道数，ICF_COMM_MAX_USER_CHN_NUM */
    VOID *pChnHandle[ENGINE_COMM_MAX_USER_CHN_NUM];     /* 当前最大支持创建2路引擎通道 */
} ENGINE_COMM_INIT_CHNL_OUTPUT;

typedef struct _ENGINE_COMM_INIT_CHNL_CONFIG_
{
    UINT32 u32ChnNum;                                            /* 创建的通道数，ICF_COMM_MAX_USER_CHN_NUM */
    ENGINE_COMM_CHN_ATTR astChnAttr[ENGINE_COMM_MAX_USER_CHN_NUM];     /* 当前最大支持创建2路引擎通道 */
} ENGINE_COMM_INIT_CHNL_CONFIG;

typedef struct _ENGINE_COMM_INIT_PRM_
{
    /* icf_engine_init使用 */
    ENGINE_COMM_INIT_MEM_CONFIG stInitMemCfg;            /* 初始化的内存大小 */
    ENGINE_COMM_INIT_CFG_FILE_S stCfgFileInfo;           /* 引擎使用的配置文件信息 */

    /* icf_engine_create使用 */
    ENGINE_COMM_INIT_CHNL_CONFIG stInitChnCfg;           /* 初始化引擎通道信息 */
    ENGINE_COMM_RSLT_CB_FUNC pRsltCbFunc;                /* 结果回调函数 */

    /* icf inner chn使用 */
    UINT32 u32IaPrvtDataSize;                         /* 与特定智能模块相关，对应ICF_INPUT_DATA->ICF_SOURCE_BLOB->pData */
} ENGINE_COMM_INIT_PRM_S;

typedef struct _ENGINE_COMM_INIT_OUTPUT_PRM_
{
    INT32 s32InitStatus;     /* szl_todo: 后续扩充底层错误码，当前仅支持成功SAL_SOK, 失败SAL_FAIL */
    ENGINE_COMM_INIT_CHNL_OUTPUT stInitChnOutput;     /* 初始化通道输出，用于上层通道隔离 */
} ENGINE_COMM_INIT_OUTPUT_PRM;

typedef struct _ENGINE_COMM_USER_CONFIG_PRM_
{
    VOID *pChnHandle;            /* 引擎通道句柄 */

    UINT32 u32NodeId;            /* 引擎节点信息 */
    UINT32 u32CfgKey;            /* 引擎配置键 */
    VOID *pData;                 /* 存放配置结果 */
    UINT32 u32DataSize;          /* 配置结果结构体大小，用于内部校验 */
} ENGINE_COMM_USER_CONFIG_PRM;

typedef enum _ENGINE_COMM_INPUT_DATA_IMG_TYPE_
{
    YUV_NV21,
    IMG_TYPE_NUM,
} ENGINE_COMM_INPUT_DATA_IMG_TYPE;

typedef struct _ENGINE_COMM_INPUT_DATA_IMG_PRM_
{
    UINT32 u32W;
    UINT32 u32H;
    UINT32 u32S[3];

    VOID *pY_CompVa;
    VOID *pU_CompVa;
    VOID *pV_CompVa;
} ENGINE_COMM_INPUT_DATA_IMG_PRM;

typedef struct _ENGINE_COMM_INPUT_LCD_DATA_IMG_PRM_
{
    char *InFeatRaw;                                          // 输入特征图,按顺序依次为Le He Zmap(均为16bit 2字节)
    int   FeatW;                                              // 特征宽
    int   FeatH;                                              // 特征高
} ENGINE_COMM_INPUT_LCD_DATA_IMG_PRM;


typedef enum _ENGINE_COMM_INPUT_DATA_MEM_TYPE_
{
    HISI_MMZ_CACHE,
    NT_FEATURE,
    RK_MALLOC,

    INPUT_DATA_MEM_NUM,
} ENGINE_COMM_INPUT_DATA_MEM_TYPE;

typedef struct _ENGINE_COMM_INPUT_DATA_IMG_INFO_
{
    UINT64 u64FrameNum;

    ENGINE_COMM_INPUT_DATA_MEM_TYPE enInputDataMemType;        /* 输入数据使用内存类型 */
    ENGINE_COMM_INPUT_DATA_IMG_TYPE enImgType;                 /* 图像类型 */

    UINT32 u32SourYuvRectValid;                             /* 是否使能图像裁剪 */
    float fYuvRectX;
    float fYuvRectY;
    float fYuvRectW;
    float fYuvRectH;

    UINT32 u32W;
    UINT32 u32H;
    UINT32 u32S[3];

    UINT32 u32MainViewImgNum;
    ENGINE_COMM_INPUT_DATA_IMG_PRM stMainViewImg[ENGINE_COMM_INPUT_SINGLE_VIEW_IMG_NUM];

    UINT32 u32SideViewImgNum;
    ENGINE_COMM_INPUT_DATA_IMG_PRM stSideViewImg[ENGINE_COMM_INPUT_SINGLE_VIEW_IMG_NUM];

    ENGINE_COMM_INPUT_LCD_DATA_IMG_PRM stInLcdData[ENGINE_COMM_INPUT_SINGLE_VIEW_IMG_NUM];
} ENGINE_COMM_INPUT_DATA_IMG_INFO;

/* 回调函数，输出icf框架内部数据到外部，此为抽象化接口，无任何智能相关信息 */
typedef INT32 (*ENGINE_COMM_COPY_TO_OUTER)(VOID *pSrc, VOID *pDst);
typedef INT32 (*ENGINE_COMM_RELEASE_USER_PRVT_INFO)(VOID *pUserPrvtInfo);

typedef struct _ENGINE_COMM_USER_PRVT_CTRL_PRM_
{
    BOOL bUsed;             /* 标志位，是否存在用户透传数据，默认0 */
    VOID *pUsrData;
    ENGINE_COMM_RELEASE_USER_PRVT_INFO pFuncRlsUserPrvtInfo;    /* 输入用户私有数据，暂不需要处理函数 */
} ENGINE_COMM_USER_PRVT_CTRL_PRM;

/* 回调函数，处理engine框架内的私有资源 */
typedef INT32 (*ENGINE_COMM_ENGINE_PROC_PRVT_DATA)(VOID *pEngPrvtData);

/* 此处用于存放引擎框架依赖的buf信息，透传用于释放 */
typedef struct _ENGINE_COMM_ENGINE_PRVT_CTRL_PRM_
{
    VOID *pEngineInnerChnPrvtData;                         /* 引擎通道缓存，当前仅有ICF_INNER_CHN_BUF */
    ENGINE_COMM_ENGINE_PROC_PRVT_DATA pFuncProcEngPrvtData;   /* 用于处理引擎框架内部的私有数据，当前仅有释放pInnerChnBuf实现 */
} ENGINE_COMM_ENGINE_PRVT_CTRL_PRM;

/* engine框架通用用户输入数据 */
typedef struct _ENGINE_COMM_INPUT_DATA_USER_INFO_
{
    ENGINE_COMM_USER_PRVT_CTRL_PRM stUsrPrvtCtrlPrm;          /* 用户透传的私有数据结构 */
    ENGINE_COMM_ENGINE_PRVT_CTRL_PRM stEnginePrvtInfo;        /* engine框架的私有信息，目前仅有通道buf信息 */
} ENGINE_COMM_INPUT_DATA_USER_INFO;

/* 拷贝外部智能模块的图像数据、算法参数到icf框架结构体中 */
typedef INT32 (*ENGINE_COMM_PROC_IA_IMG_FUNC)(VOID *pIaImgInfo, VOID *pIcfOutInfo);
typedef INT32 (*ENGINE_COMM_PROC_IA_PRM_FUNC)(VOID *pIaPrvtPrm, VOID *pIcfOutInfo);

/* 对于智能模块私有图像数据处理的封装结构，包括数据和操作函数指针 */
typedef struct _ENGINE_COMM_IA_IMG_PRVT_CTRL_PRM_
{
    UINT32 u32InputDataImgNum;
    ENGINE_COMM_INPUT_DATA_IMG_INFO stInputDataImg[ENGINE_COMM_INPUT_DATA_IMG_MAX_NUM];     /* 图像数据，对应底层blob张量 */

    ENGINE_COMM_PROC_IA_IMG_FUNC pFuncProcIaImg;       /* 回调函数，外部智能模块赋值。将图像信息转化为底层icf框架支持的数据结构 */
} ENGINE_COMM_IA_IMG_PRVT_CTRL_PRM;

/* 对于智能模块私有算法配置参数处理的封装结构，包括数据和操作函数指针 */
typedef struct _ENGINE_COMM_IA_PRM_PRVT_CTRL_PRM_
{
    VOID *pIaAlgPrm;                                    /* 智能模块需要传入的算法私有参数 */
    ENGINE_COMM_PROC_IA_PRM_FUNC pFuncProcIaPrm;           /* 回调函数，外部智能模块赋值。用于将算法智能参数拷贝到底层icf框架支持的数据结构 */
} ENGINE_COMM_IA_PRM_PRVT_CTRL_PRM;

typedef struct _ENGINE_COMM_INPUT_DATA_INFO_
{
    VOID *pChnHandle;                                    /* 引擎通道句柄 */

    ENGINE_COMM_IA_IMG_PRVT_CTRL_PRM stIaImgPrvtCtrlPrm;    /* 智能模块图像信息，私有控制参数，包含数据及数据支持的行为 */
    ENGINE_COMM_IA_PRM_PRVT_CTRL_PRM stIaPrmPrvtCtrlPrm;    /* 智能模块算法参数，私有控制参数，包含数据及数据支持的行为 */

    ENGINE_COMM_INPUT_DATA_USER_INFO stInputUserInfo;       /* engine封装层支持的用户通用参数结构体 */
} ENGINE_COMM_INPUT_DATA_INFO;

typedef struct _ENGINE_COMM_OUTPUT_CTRL_PRM_
{
    ENGINE_COMM_NODE_TYPE_E enNodeId;                             /* 待获取算法结果的节点id */
    VOID *pAlgRsltInfo;                           /* 根据传入的节点控制信息返回的算法结果，此处对结果透明化处理，在外部模块具象化 */
    ENGINE_COMM_COPY_TO_OUTER pFuncGetNodeRslt;      /* 回调函数，用于将节点算法结果具象化到外部模块 */

    VOID *pUserInfo;                              /* 外部智能模块存放用户透传信息 */
    ENGINE_COMM_COPY_TO_OUTER pFuncGetUserInfo;      /* 回调函数，用于将透传信息具象化到外部模块 */
} ENGINE_COMM_OUTPUT_CTRL_PRM;

typedef struct _ENGINE_COMM_OUTPUT_DATA_S_
{
    ENGINE_COMM_OUTPUT_CTRL_PRM stOutputCtlPrm;                /* 待获取的节点控制信息 */
} ENGINE_COMM_OUTPUT_DATA_S;

typedef struct _ENGINE_COMM_VERSION_
{
	/* 主版本号 */
    UINT32 u32MajorVersion;
	/* 子版本号 */
    UINT32 u32SubVersion;
	/* 修正版本号 */
    UINT32 u32RevisVersion;
	/* 版本号-年 */
    UINT32 u32VersionYear;
	/* 版本号-月 */
    UINT32 u32VersionMonth;
	/* 版本号-日 */
    UINT32 u32VersionDay;
} ENGINE_COMM_VERSION_S;

typedef INT32 (*ENGINE_COMM_INIT_FUNC)(const ENGINE_COMM_INIT_PRM_S *, ENGINE_COMM_INIT_OUTPUT_PRM *);
typedef INT32 (*ENGINE_COMM_DEINIT_FUNC)(VOID);
typedef INT32 (*ENGINE_COMM_GET_ICF_VERSION)(ENGINE_COMM_VERSION_S *, UINT32);
typedef INT32 (*ENGINE_COMM_GET_ICF_APP_VERSION)(ENGINE_COMM_VERSION_S *, UINT32);
typedef INT32 (*ENGINE_COMM_SET_CONFIG_FUNC)(ENGINE_COMM_USER_CONFIG_PRM *);
typedef INT32 (*ENGINE_COMM_GET_CONFIG_FUNC)(ENGINE_COMM_USER_CONFIG_PRM *);
typedef INT32 (*ENGINE_COMM_INPUT_DATA_FUNC)(ENGINE_COMM_INPUT_DATA_INFO *);
typedef INT32 (*ENGINE_COMM_GET_RSLT_FUNC)(VOID *, ENGINE_COMM_OUTPUT_DATA_S *);

typedef struct _ENGINE_COMM_FUNC_P_
{
    ENGINE_COMM_INIT_FUNC init;
    ENGINE_COMM_DEINIT_FUNC deinit;
    ENGINE_COMM_GET_ICF_VERSION get_icf_version;
    ENGINE_COMM_GET_ICF_APP_VERSION get_icf_app_version;
    ENGINE_COMM_SET_CONFIG_FUNC set_config;
    ENGINE_COMM_GET_CONFIG_FUNC get_config;
    ENGINE_COMM_INPUT_DATA_FUNC input_data;
    ENGINE_COMM_GET_RSLT_FUNC get_rslt;
} ENGINE_COMM_FUNC_P;

#endif

