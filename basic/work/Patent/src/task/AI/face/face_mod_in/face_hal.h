/*******************************************************************************
* face_hal.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : zongkai5 <zongkai5@hikvision.com>
* Version: V2.0.0  2021Äê10ÔÂ30ÈÕ Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef _FACE_HAL_H_
#define _FACE_HAL_H_

/* ========================================================================== */
/*                          Í·ÎÄ¼şÇø									   */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"
#include "system_prm_api.h"
#include "ia_common.h"
#include "jpgd_lib.h"
#include "vdec_tsk_api.h"

/* ´Ë´¦°üº¬µÄµ±Ç°°æ±¾icfµÄËùÓĞÍ·ÎÄ¼ş¼°ÒıÇæÓ¦ÓÃ²ãÍ·ÎÄ¼ş */
#include "ICF_error_code_v2.h"
#include "ICF_Interface_v2.h"
#include "ICF_toolkit_v2.h"
#include "ICF_type_v2.h"

/* ÈËÁ³Ïà¹ØÍ·ÎÄ¼ş */
#include "alg_error_code.h"
#include "alg_type_base.h"
#include "alg_type_face.h"
#include "alg_type_vca.h"

/* ========================================================================== */
/*                          ºê¶¨ÒåÇø									   */
/* ========================================================================== */
#define FACE_TEATURE_HEADER_LENGTH	(sizeof(UINT32))
#define FACE_LOGIN_IMG_WIDTH		(960)
#define FACE_LOGIN_IMG_HEIGHT		(540)
#define FACE_CAP_IMG_WIDTH			(960)
#define FACE_CAP_IMG_HEIGHT			(540)
#define FACE_REGISTER_MAX_WIDTH		(1920)
#define FACE_REGISTER_MAX_HEIGHT	(1080)
#define FACE_MAX_DET_NUM			(10)
#define FACE_JDEC_BUF_NUM			(1)
#define FACE_MAX_CHAN_NUM			(3)
#define FACE_MAX_REGISTER_BUF_NUM	(3)
#define FACE_INPUT_DATA_NUM			(16)
#define FACE_BATCH_NUM				(1)        /* Åú´¦Àí */
#define FACE_VPSS_CHAN_ID			(2)
#define FACE_JPEG_SIZE				(4 * FACE_CAP_IMG_WIDTH * FACE_CAP_IMG_HEIGHT)
#define FACE_LOGIN_THR				(0.7)
#define FACE_CAP_THR				(0.85)
#define FACE_SEND_PIC_THR			(0.75)
#define FACE_EYE_DIS_THR			(0.02)

#define FD_TRACK_MODEL_PATH			("./face/model/Track_v5.2.0_RK3588_FP32_gen20200225.bin")
#define FD_QUALITY_MODEL_PATH		("./face/model/QualityFD_v7.0_v1_GPU_RK3588_INT8_SIG_QUAN_gen20220727.bin")
#define DFR_DETECT_MODEL_PATH		("./face/model/DetectFD_v5.2.0_v1_RK3588_INT8_gen20220728.bin")
#define DFR_LANDMARK_MODEL_PATH		("./face/model/Landmark_v7.0.0_RK3588_gen20220718.bin")
#define DFR_QUALITY_MODEL_PATH		("./face/model/QualityFR_v7.0_v1_GPU_RK3588_INT8_gen20220727.bin")
#define DFR_LIVENESS_MODEL_PATH		("./face/model/Liveness_v7.0.0_rk3588_INT8_gen20220726.bin")
#define DFR_ATTRIBUTE_MODEL_PATH	("./face/model/Attribute_v7.0.0_v1_GPU_RK3588_INT8_gen20220726.bin")
#define DFR_FEATURE_MODEL_PATH		("./face/model/Feature_v5.2.0_RK3588_FP16_gen20220519.bin")
#define DFR_COMPARE_MODEL_PATH		("./face/model/Compare_v5.2.0_RK3588_INT8_gen20210308_CHAR.bin")

#define ICF_CHECK_ERROR(sts, ret)                                                                           \
    {                                                                                                           \
        if (sts)                                                                                                \
        {                                                                                                       \
            FACE_LOGE("\nerror !!!: %s, %s, line: %d, return 0x%x\n\n", __FILE__, __FUNCTION__, __LINE__, ret);    \
            return (ret);                                                                                       \
        }                                                                                                       \
    }

/* ========================================================================== */
/*                          Êı¾İ½á¹¹Çø									   */
/* ========================================================================== */
typedef enum tagFaceAnaMode
{
    FACE_PICTURE_MODE = 0,      /* Í¼Æ¬Ä£Ê½ Í¨µÀ */
    FACE_VIDEO_LOGIN_MODE = 1,    /* ÈËÁ³µÇÂ¼Í¨µÀ */
    FACE_VIDEO_CAP_MODE = 2,    /* ÈËÁ³×¥ÅÄÍ¨µÀ */
    FACE_MODE_MAX_NUM = 3
} FACE_ANA_MODE_E;

typedef struct _FACE_PIC_REGISTER_OUT_
{
    /* ±¾´Î´¦Àí½á¹û×´Ì¬ */
    FACE_DSP_REG_RESULT_FLAG enProcSts;
    /* ÈËÁ³½¨Ä£Êı¾İ³¤¶È */
    UINT32 u32FeatDataLen;
    /* ÈËÁ³½¨Ä£Êı¾İ */
    UINT8 acFeaureData[FACE_FEATURE_LENGTH];
} FACE_PIC_REGISTER_OUT_S;

typedef struct _FACE_VIDEO_LOGIN_OUT_
{
    /* ±¾´Î´¦Àí½á¹û×´Ì¬ */
    FACE_DSP_REG_RESULT_FLAG enProcSts;
    /* ÈËÁ³½¨Ä£Êı¾İ³¤¶È */
    UINT32 u32FeatDataLen;
    /* ÈËÁ³½¨Ä£Êı¾İ */
    UINT8 acFeaureData[FACE_FEATURE_LENGTH];
} FACE_VIDEO_LOGIN_OUT_S;

typedef struct _FACE_VIDEO_CAPTURE_OUT_
{
    /* ±¾´Î´¦Àí½á¹û×´Ì¬ */
    FACE_DSP_REG_RESULT_FLAG enProcSts;

	/* ÈËÁ³¸öÊı */
	UINT32 u32FaceNum;
    /* ÈËÁ³½¨Ä£Êı¾İ³¤¶È */
    UINT32 au32FeatDataLen[MAX_CAP_FACE_NUM];
    /* ÈËÁ³½¨Ä£Êı¾İ */
    UINT8 acFeaureData[MAX_CAP_FACE_NUM][FACE_FEATURE_LENGTH];

    /* ÈËÁ³ÊôĞÔ */
	UINT32 u32FaceAttrNum;
    FACE_ATTRIBUTE_DSP_OUT astFaceAttribute[MAX_CAP_FACE_NUM];
} FACE_VIDEO_CAPTURE_OUT_S;

typedef union _FACE_PROC_OUT_
{
    /* Í¼Æ¬×¢²áÒµÎñÏß´¦Àí½á¹û */
    FACE_PIC_REGISTER_OUT_S stPicProcOut;
    /* ÈËÁ³µÇÂ¼ÒµÎñÏß´¦Àí½á¹û */
    FACE_VIDEO_LOGIN_OUT_S stVideoLoginProcOut;
    /* ÈËÁ³×¥ÅÄÒµÎñÏß´¦Àí½á¹û */
    FACE_VIDEO_CAPTURE_OUT_S stVideoCapProcOut;
} FACE_PROC_OUT_U;

typedef struct tagFaceAnaBuf
{
    sem_t sem;                                 /* sem signal */

    /* ÈËÁ³¸÷ÒµÎñÏß´¦Àí½á¹û */
    FACE_PROC_OUT_U uFaceProcOut;

    UINT32 uiMaxBufNum;                        /* Total Buf Count */
    UINT32 uiFrameNum;                         /* Frame Num Marking New Frame */

    /* Ë½ÓĞÖ¡ĞòºÅ£¬µ±Ç°½öÈËÁ³×¥ÅÄÒµÎñÏßµÄ½Úµã¶şĞèÒªÊ¹ÓÃ£¬ÓÃÓÚ»ñÈ¡ÈËÁ³ÊôĞÔºÍÈËÁ³½¨Ä£½á¹û */
    UINT32 uiFrameNum_priv;

    UINT32 uiBufIdx;                           /* Save Current Buf Index */
    UINT32 uiRlsFlag[FACE_INPUT_DATA_NUM];                      /* Release Flag */
    ICF_INPUT_DATA_V2 stFaceBufData[FACE_INPUT_DATA_NUM];          /* Buf Data */
    SAE_FACE_IN_DATA_INPUT_T stFaceData[FACE_INPUT_DATA_NUM];             /*Face Data*/
} FACE_ANA_BUF_INFO;

typedef struct _FACE_INIT_ENGINE_CHANNEL_PRM_
{
    /* ³õÊ¼»¯µÄÒıÇæ¿ò¼Ü¾ä±ú */
    VOID *pIcfInitHandle;

    /* ÒµÎñÏßid£¬ÒıÇæÄÚ²¿¶¨Òå */
    UINT32 u32GraphId;
    /* ¾ßÌåÒµÎñÀàĞÍ */
    UINT32 u32GraphType;
    /* ÓÃ»§²ÎÊı£¬¶ÔÓ¦²»Í¬ÒµÎñÏßĞèÒªÆôÓÃµÄÄÜÁ¦¼¶ */
    ICF_APP_PARAM_INFO_V2 *pstAppParam;
    /* post½Úµãid */
    UINT32 u32PostNodeId;
    /* ½á¹û»Øµ÷´¦Àíº¯Êı */
    VOID *pCallBackFunc;
} FACE_INIT_ENGINE_CHANNEL_PRM_S;

typedef struct tagFaceBgraFrm
{
    UINT32 uiUseFlag;    /* ¸ÃÄÚ´æÊÇ·ñ±»Õ¼ÓÃ */
    UINT64 PhyAddr[3];   /* ÎïÀíµØÖ· */
    UINT64 VirAddr[3];   /* ĞéÄâµØÖ· */
} BGRA_FRAME_BUF;

typedef struct tagFaceJdecBuf
{
    UINT32 uiUseFlag;    /* ¸ÃÄÚ´æÊÇ·ñ±»Õ¼ÓÃ */
    UINT32 uiBufSize;    /* »º´æ´óĞ¡ */
    UINT64 PhyAddr;      /* ÎïÀíµØÖ· */
    UINT64 VirAddr;      /* ĞéÄâµØÖ· */
} FACE_JDEC_BUF;

typedef struct tagFaceJdecBufInfo
{
    UINT32 uiIdx;            /* BGRAÉêÇëµÄBUF¸öÊı */
    FACE_JDEC_BUF stBufData[FACE_JDEC_BUF_NUM];   /* BGRAÄÚ´æ */
} FACE_JDEC_BUF_INFO;

typedef struct tagFaceVerData
{
    unsigned int nMajorVersion;
    unsigned int nSubVersion;
    unsigned int nRevisVersion;
    unsigned int nVersionYear;
    unsigned int nVersionMonth;
    unsigned int nVersionDay;
} FACE_VERSION_DATA;

typedef struct tagFaceVersionInfo
{
    FACE_VERSION_DATA stAppVerInfo;   /* ÈËÁ³Ó¦ÓÃ»ùÏß°æ±¾ĞÅÏ¢ */
} FACE_VERSION_INFO;

/* ÔËĞĞÊ±¼ÓÔØ¶¯Ì¬¿â·ûºÅ£¬´Ë´¦½ö»ñÈ¡º¯ÊıµØÖ· */
typedef int (*FACE_ICF_INIT_F)(void *pInitParam,
                               unsigned int nInitParamSize,
                               void *pInitOutParam,
                               unsigned int nInitOutParamSize);
typedef int (*FACE_ICF_LOAD_MODEL_F)(void *pLMInParam,
                                     unsigned int nLMInParamSize,
                                     void *pLMOutParam,
                                     unsigned int nLMOutParamSize);
typedef int (*FACE_ICF_UNLOAD_MODEL_F)(void *pULMInParam,
                                       unsigned int nULMInParamSize);
typedef int (*FACE_ICF_CREATE_F)(void *pCreateParam,
                                 unsigned int nCreateParamSize,
                                 void *pCreateHandle,
                                 unsigned int nHandleSize);
typedef int (*FACE_ICF_DESTROY_F)(void *pCreateHandle,
                                  unsigned int nHandleSize);
typedef int (*FACE_ICF_INPUT_DATA_F)(void *pChannelHandle,
                                     void *pInputData,
                                     unsigned int nDataSize);
typedef int (*FACE_ICF_SET_CONFIG_F)(void *pChannelHandle,
                                     void *pConfigParam,
                                     unsigned int nConfigParamSize);
typedef int (*FACE_ICF_GET_CONFIG_F)(void *pChannelHandle,
                                     void *pConfigParam,
                                     unsigned int nConfigParamSize);
typedef int (*FACE_ICF_SET_CALLBACK_F)(void *pChannelHandle,
                                       void *pCBParam,
                                       unsigned int nCBParamSize);
typedef int (*FACE_ICF_GET_VERSION_F)(void *pEngineVersionParam,
                                      int nSize);
typedef int (*SAE_GET_APP_VERSION_F)(void *pVersionParam,
                                     int nSize);
typedef int (*FACE_ICF_FINIT_F)(void *pFinitInParam,
                                unsigned int nFinitInParamSize);
typedef int (*FACE_ICF_GET_MEMPOOL_STATUS)(void *pChannelHandle);
typedef int (*FACE_ICF_GET_PACKAGE_STATE)(void *pPackageHandle);
typedef void * (*FACE_ICF_GET_PACKAGE_DATA_PTR)(void *pPackageHandle, int nDataType);

typedef struct _FACE_ICF_FUNC_P_
{
    FACE_ICF_INIT_F IcfInit;
    FACE_ICF_FINIT_F IcfFinit;
    FACE_ICF_LOAD_MODEL_F IcfLoadModel;
    FACE_ICF_UNLOAD_MODEL_F IcfUnloadModel;
    FACE_ICF_CREATE_F IcfCreate;
    FACE_ICF_DESTROY_F IcfDestroy;
    FACE_ICF_SET_CONFIG_F IcfSetConfig;
    FACE_ICF_GET_CONFIG_F IcfGetConfig;
    FACE_ICF_SET_CALLBACK_F IcfSetCallback;
    FACE_ICF_GET_VERSION_F IcfGetVersion;
    SAE_GET_APP_VERSION_F SaeGetAppVersion;               /* »ñÈ¡app°æ±¾ºÅ½Ó¿ÚÀ´×Ôlibalg.so£¬ÆäËû¾ùÊÇlibicf.soÖĞ»ñÈ¡ */
    FACE_ICF_INPUT_DATA_F IcfInputData;
    FACE_ICF_GET_PACKAGE_STATE IcfGetPackageStatus;
    FACE_ICF_GET_PACKAGE_DATA_PTR IcfGetPackageDataPtr;
    FACE_ICF_GET_MEMPOOL_STATUS IcfGetMemPoolStatus;
} FACE_ICF_FUNC_P;

typedef struct _FACE_PROC_LINE_ICF_HANDLE_
{
    /* Ä£ĞÍ¾ä±ú */
    ICF_MODEL_HANDLE_V2 stIcfModelHandle;
    /* ÒıÇæÍ¨µÀ¾ä±ú£¬µ±Ç°½öÈËÁ³×¥ÅÄÒµÎñÏßÊ¹ÓÃ1£¬ÓÃÓÚ¶ÔÑ¡Ö¡½á¹û½øĞĞÈËÁ³½¨Ä£ */
    ICF_CREATE_HANDLE_V2 stIcfCreateHandle[2];
} FACE_PROC_LINE_ICF_HANDLE_S;

typedef struct tagFaceCommonParam
{
    /* Ä£¿é³õÊ¼»¯×´Ì¬ */
    BOOL bInit;

    /* µ÷¶È¾ä±ú */
    VOID *pSchedulerHdl;
    /* Ëã·¨½âÃÜ¾ä±ú */
    VOID *pAlgEncryptHdl;
    /* libicf.soµÄ¼ÓÔØ¾ä±ú */
    VOID *pIcfLibHandle;
    /* libSae.soµÄ¼ÓÔØ¾ä±ú */
    VOID *pIcfAlgHandle;
    /* ICF¿ò¼Ü¶¯Ì¬ÔËĞĞ¿âº¯Êı£¬Í¨¹ıdlopen´ò¿ª */
    FACE_ICF_FUNC_P stIcfFuncP;
    /* ÒıÇæ¼ÆËã¿ò¼Ü³õÊ¼»¯È«¾Ö¾ä±ú */
    VOID *pInitHandle;
    /* ÈËÁ³Ã¿¸öÒµÎñÏß´´½¨µÄÍ¨µÀ¾ä±ú£¬µÚÒ»Î¬Ë÷ÒıÎªÒµÎñÏß¸öÊı£¬µÚ¶şÎ¬Ë÷ÒıÎª¸ÃÒµÎñÏßÏÂ´´½¨µÄÍ¨µÀ¸öÊı£¬µ±Ç°×î´óÎª2 */
    VOID *pEngineChnHandle[FACE_MODE_MAX_NUM][2];
    /* extern±È¶Ô¿âÊ¹ÓÃµÄÄÚ´æ³Ø¾ä±ú */
    VOID *pMemPoolExtCmp;
    /* extern±È¶Ô¿âÊ¹ÓÃµÄ¾ä±ú */
    VOID *pExternCompare;
    /* Í¼Æ¬×¢²á¡¢ÈËÁ³µÇÂ¼¡¢ÈËÁ³×¥ÅÄÈı¸öÒµÎñÏßµÄ³õÊ¼»¯¾ä±ú */
    FACE_PROC_LINE_ICF_HANDLE_S astProcLineHandle[FACE_MODE_MAX_NUM];

    /* °æ±¾ºÅĞÅÏ¢ */
    FACE_VERSION_INFO stVersionInfo;

    /* ÓÃÓÚ¿½±´½âÂë½á¹ûÊı¾İ£¬ÁÙÊ±±äÁ¿ */
    SYSTEM_FRAME_INFO stVdecCpyTmpFrameInfo;
    /* »¥³âËø£¬ÓÃÓÚ¹ÜÀí½âÂë¿½±´Ö¡Êı¾İÁÙÊ±±äÁ¿£¬Ä¿Ç°ÓĞµÇÂ¼ºÍ×¥ÅÄÁ½¸öµØ·½ÔÚÊ¹ÓÃ */
    VOID *pVdecCpyFrmMutex;

    /* Í¼Æ¬½âÂë»º´æÊı¾İ */
    FACE_JDEC_BUF_INFO stJdecBufInfo;

    /* ËÍÈëÒıÇæµÄÈËÁ³»º´æÊı¾İµÈÏà¹ØĞÅÏ¢ */
    FACE_ANA_BUF_INFO astFaceBufData[FACE_MODE_MAX_NUM];

    /* ÈËÁ³×¥ÅÄ½¨Ä£½ÚµãÊ¹ÓÃµÄ»¥³âËø */
    pthread_mutex_t stFaceRepoLock;
} FACE_COMMON_PARAM;

typedef struct tagFaceModelDataBase
{
    UINT32 uiModelCnt;                      /* Êı¾İ¿âÖĞÒÑÓĞÄ£ĞÍ¸öÊı */
    UINT32 uiMaxModelCnt;                   /* Êı¾İ¿âµ±Ç°Ö§³Ö×î´óµÄÈËÁ³Êı£¬µ±Ç°×î´óÎª53 */
    UINT64 *pFeatureData[MAX_FACE_NUM];     /* Ä£ĞÍÌØÕ÷Êı¾İ */
} FACE_MODEL_DATA_BASE;

typedef enum
{
    TOP_FID_MODE = 0,               /* Top field. */
    BOTTOM_FID_MODE	= 1,            /* Bottom field. */
    PROGRESSIVE_FRAME_MODE = 2,     /* Progressive frame*/
    INTERLACED_FRAME_MODE = 3,      /* Interlaced frame */
    SEPARATE_FRAME_MODE	= 4,        /* Separete frame */
    FRAME_MODE_BUTT,
} Vdec_Frame_Mode;

/* YUVÍ¼ÏñµÄĞÅÏ¢À©Õ¹ */
typedef struct _DSP_YUV_FRAME_EX_
{
    UINT32 width;
    UINT32 height;
    UINT32 pitchY;
    UINT32 pitchUv;
    PUINT8 yTopAddr;
    PUINT8 yBotAddr;
    PUINT8 uTopAddr;
    PUINT8 uBotAddr;
    PUINT8 vTopAddr;
    PUINT8 vBotAddr;
    Vdec_Frame_Mode frameMode;
} FACE_YUV_FRAME_EX;

/* µ×¿âÍ· */
typedef struct _FEAT_REPO_HEADER_
{
    unsigned int ele_cnt;                                        /* åå•åº“é‡Œå…ƒç´ çš„ä¸ªæ•° */
    unsigned int ele_pad_cnt;                                    /* åå•åº“é‡Œè¡¥å……å…ƒç´ çš„ä¸ªæ•°ä¸€èˆ¬1000 */
    unsigned int ele_all_cnt;                                    /* åå•åº“é‡Œæ‰€æœ‰å…ƒç´ çš„ä¸ªæ•° */
    unsigned int feat_len;                                       /* åå•åº“é‡Œæ¯ä¸ªå…ƒç´ ä¸­äººè„¸æ¨¡å‹çš„é•¿åº¦ */
    unsigned int addinfo_len;                                    /* åå•åº“é‡Œæ¯ä¸ªå…ƒç´ ä¸­åº”ç”¨é™„åŠ çš„é•¿åº¦ */
    unsigned int elesize;                                        /* åå•åº“é‡Œæ¯ä¸ªå…ƒç´ çš„å†…å­˜å¤§å° */
    unsigned int memsize;                                        /* åå•åº“é‡Œçš„æ€»å†…å­˜å¤§å° */
    unsigned int headsize;                                       /* åå•åº“å¤´çš„ç»“æ„ä½“å¤§å° */

    unsigned int pool_version;                                   /* åå•åº“ç‰ˆæœ¬å·ï¼Œåº”ç”¨å±‚å¡«å†™ï¼Œå¼•æ“è¯»å– */
    unsigned char *virthead;                                     /* åå•åº“é‡Œå¤´çš„è™šé¦–åœ°å€ */
    unsigned char *physhead;                                     /* åå•åº“é‡Œå¤´çš„ç‰©ç†é¦–åœ°å€ */
    unsigned char *virtpool;                                     /* åå•åº“é‡Œeleçš„è™šé¦–åœ°å€ */
    unsigned char *physpool;                                     /* åå•åº“é‡Œeleçš„ç‰©ç†é¦–åœ°å€ */
    unsigned char reserve[12];                                   /* åå•åº“é‡Œçš„ä¿ç•™å­—æ®µ */
} FEAT_REPO_HEADER;

/* ========================================================================== */
/*                          º¯Êı¶¨ÒåÇø									   */
/* ========================================================================== */


void *Face_HalGetVaceHandle(FACE_ANA_MODE_E mode);
void *Face_HalGetICFHandle(FACE_ANA_MODE_E mode, UINT32 u32NodeIdx);
UINT64 Face_HalgetTimeMilli(void);
void Face_HalDebugDumpData(CHAR *pData, CHAR *pPath, UINT32 uiSize);
FACE_ANA_BUF_INFO *Face_HalGetAnaDataTab(UINT32 mode);
UINT32 Face_HalGetAnaFreeBuf(FACE_ANA_MODE_E mode, void *pIdx);

FACE_JDEC_BUF_INFO *Face_HalGetJdecBuf(VOID);
FACE_COMMON_PARAM *Face_HalGetComPrm(VOID);
FACE_MODEL_DATA_BASE *Face_HalGetDataBase(VOID);
UINT32 Face_HalGetFreeBgraMem(UINT32 *pIdx);
INT32 Face_HalI420ToNv21(FACE_YUV_FRAME_EX *pYuvFrm,
                         UINT32 yDstStride,
                         UINT32 uvDstStride,
                         PUINT8 pDstLumaAddr,
                         PUINT8 pDstChromaAddr);
VOID Face_HalMemFree(VOID *buf);
INT32 Face_HalFreeMemTab(HKA_MEM_TAB *mem_tab, INT32 tab_num);
VOID *Face_HalMemAlloc(HKA_SZT size, INT32 align);
INT32 Face_HalAllocMemTab(HKA_MEM_TAB *mem_tab, HKA_S32 tab_num);

UINT32 Face_HalYuv2Bgra(UINT64 yDataPhy, UINT64 uvDataPhy, UINT64 bgraDataPhy, UINT32 width, UINT32 height, UINT32 pitch);
UINT32 Face_HalSetConfig(UINT32 chan, void *pParam);
UINT32 Face_HalCompare(UINT32 chan, void *pFeatData, float fSim, UINT32 mode);
INT32 Face_HalGetFrame(UINT32 uiVdecChn, void *pFrame);
INT32 Face_HalLoginProc(UINT32 chan, float fSim, void *pFrame, void *pOutInfo);
UINT32 Face_HalUpdateCompareLib(void);
INT32 Face_HalGetVersion(void);
INT32 Face_HalInit(void);
INT32 Face_HalDeInit(void);
INT32 Face_HalFree(VOID *p);
INT32 Face_HalMalloc(VOID **pp, UINT32 uiSize);
VOID Face_HalDumpNv21(CHAR *pData, UINT32 u32DataSize, CHAR *pStrHead, UINT32 u32StrTailIdx, UINT32 u32W, UINT32 u32H);

/**
 * @function   Face_HalDumpFaceFeature
 * @brief      ToDo
 * @param[in]  CHAR *pData
 * @param[in]  UINT32 u32DataSize
 * @param[in]  CHAR *pStrHead
 * @param[in]  UINT32 u32StrTailIdx
 * @param[in]  UINT32 u32W
 * @param[in]  UINT32 u32H
 * @param[out] None
 * @return     VOID
 */
VOID Face_HalDumpFaceFeature(CHAR *pData, UINT32 u32DataSize, CHAR *pStrHead, UINT32 u32StrTailIdx);

/**
 * @function:   Face_HalInitRtld
 * @brief:      baÄ£¿é¼ÓÔØ¶¯Ì¬¿â·ûºÅ
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Face_HalInitRtld(VOID);

#endif

