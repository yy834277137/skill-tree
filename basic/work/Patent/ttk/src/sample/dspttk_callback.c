/**
 * @file    dspttk_callback.c
 * @brief   DSP回调函数的实现
 * @note
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_util.h"
#include "dspttk_fop.h"
#include "dspttk_devcfg.h"
#include "dspcommon.h"
#include "dspttk_init.h"
#include "dspttk_callback.h"
#include "dspttk_cmd_xray.h"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
#define SQL_TYPE_INT2STR_LEN        24      // 整型转成字符串的最大长度，单位：字节
#define SQL_TYPE_DATE2STR_LEN       32      // 日期转成字符串的最大长度，单位：字节
/**
 * KEYI: KEY datatype is Interger
 * KEYS: KEY datatype is String/Text 
 * Key名长度不超过SQL_KEY_NAME_LEN_MAX 
 */
// 对应结构体DB_TABLE_SLICE
#define KEYI_SLICE_NO           "no"        // 条带序号
#define KEYI_SLICE_WIDTH        "width"     // 条带宽
#define KEYI_SLICE_HEIGHT       "height"    // 条带高
#define KEYI_SLICE_SIZE         "size"      // 条带NRAW数据大小，单位：字节
#define KEYS_SLICE_PATH         "path"      // 条带NRAW数据保存位置
#define KEYI_SLICE_TIME         "time"      // 条带输入时间
#define KEYI_SLICE_CONTENT      "content"   // 包裹条带或空白条带，参考枚举XSP_SLICE_CONTENT

// 对应结构体DB_TABLE_PACKAGE
#define KEYI_PACKAGE_ID             "id"            // 包裹序号
#define KEYI_PACKAGE_WIDTH          "width"         // 包裹宽
#define KEYI_PACKAGE_HEIGHT         "height"        // 包裹高
#define KEYI_PACKAGE_SLICE_START    "slicestart"    // 包裹起始条带号
#define KEYI_PACKAGE_SLICE_END      "sliceend"      // 包裹结束条带号
#define KEYI_PACKAGE_TIME_END       "timeend"       // 包裹结束时间，系统启动后的相对时间
#define KEYI_PACKAGE_TIME_CB        "timecb"        // 包裹回调时间，系统启动后的相对时间
#define KEYI_PACKAGE_AIDGR_NUM      "aidgr"         // AI危险品识别结果数
#define KEYI_PACKAGE_XRIDT_NUM      "xridt"         // XSP难穿透&可疑有机物识别结果数
#define KEYI_PACKAGE_DIRECTION      "direction"     // 过包显示方向
#define KEYI_PACKAGE_VMIRROR        "vmirror"       // 是否垂直镜像
#define KEYS_PACKAGE_INFO_PATH      "infopath"      // 包裹信息的保存路径
#define KEYS_PACKAGE_JPG_PATH       "jpgpath"       // 包裹JPG图片保存路径


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */
extern VOID dspttk_xray_trans_package(UINT32 u32Chan);
pthread_t gThrPidTrans[MAX_XRAY_CHAN] = {0,0};

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
/* 数据库表slice属性与Key结构 */
DB_TABLE_ATTR gstDbSliceAttr[MAX_XRAY_CHAN] = {0};
SQL_KEY_PAIR gastDbSliceKey[] = {
    {KEYI_SLICE_NO, SQL_TYPE_INTEGER}, // Primary Key
    {KEYI_SLICE_WIDTH, SQL_TYPE_INTEGER},
    {KEYI_SLICE_HEIGHT, SQL_TYPE_INTEGER},
    {KEYI_SLICE_SIZE, SQL_TYPE_INTEGER},
    {KEYS_SLICE_PATH, SQL_TYPE_TEXT},
    {KEYI_SLICE_TIME, SQL_TYPE_INTEGER},
    {KEYI_SLICE_CONTENT, SQL_TYPE_INTEGER},
};

/* 数据库表package属性与Key结构 */
DB_TABLE_ATTR gstDbPackageAttr[MAX_XRAY_CHAN] = {0};
SQL_KEY_PAIR gastDbPackageKey[] = {
    {KEYI_PACKAGE_ID, SQL_TYPE_INTEGER}, // Primary Key
    {KEYI_PACKAGE_WIDTH, SQL_TYPE_INTEGER},
    {KEYI_PACKAGE_HEIGHT, SQL_TYPE_INTEGER},
    {KEYI_PACKAGE_SLICE_START, SQL_TYPE_INTEGER},
    {KEYI_PACKAGE_SLICE_END, SQL_TYPE_INTEGER},
    {KEYI_PACKAGE_TIME_END, SQL_TYPE_INTEGER},
    {KEYI_PACKAGE_TIME_CB, SQL_TYPE_INTEGER},
    {KEYI_PACKAGE_AIDGR_NUM, SQL_TYPE_INTEGER},
    {KEYI_PACKAGE_XRIDT_NUM, SQL_TYPE_INTEGER},
    {KEYI_PACKAGE_DIRECTION, SQL_TYPE_INTEGER},
    {KEYI_PACKAGE_VMIRROR, SQL_TYPE_INTEGER},
    {KEYS_PACKAGE_INFO_PATH, SQL_TYPE_TEXT},
    {KEYS_PACKAGE_JPG_PATH, SQL_TYPE_TEXT},
};

static SEGMENT_CB_FOR_WEB gstSegForWeb[MAX_XRAY_CHAN] = {0};

/**
 * @function:   dspttk_get_seg_prm_for_web
 * @brief:     分片参数回调
 * @param[in]:  void
 * @param[out]: None
 * @return:     SEGMENT_CB_FOR_WEB* 通用参数指针
 */
SEGMENT_CB_FOR_WEB *dspttk_get_seg_prm_for_web(UINT32 u32Chan)
{
    return &gstSegForWeb[u32Chan];
}

/**
 * @fn      dspttk_slice_storage_init
 * @brief   初始化存储条带信息的数据库
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：初始化失败
 */
SAL_STATUS dspttk_slice_storage_init(void)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 i = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DB_TABLE_ATTR *pstDbSlice = NULL;
    CHAR sDbFile[128] = {0};

    for (i = 0; i < pstShareParam->xrayChanCnt; i++)
    {
        pstDbSlice = &gstDbSliceAttr[i];
        if (SAL_SOK != dspttk_rwlock_init(&pstDbSlice->rwlock))
        {
            PRT_INFO("init 'rwlock' failed, chan: %u\n", i);
            return SAL_FAIL;
        }

        pstDbSlice->pHandle = dspttk_db_init();
        if (NULL == pstDbSlice->pHandle)
        {
            PRT_INFO("oops, dspttk_db_init failed, chan: %u\n", i);
            return SAL_FAIL;
        }

        snprintf(sDbFile, sizeof(sDbFile), "%s/%u/slice.db", pstDevCfg->stSettingParam.stEnv.stOutputDir.sliceNraw, i);
        sRet = dspttk_db_open_conn(pstDbSlice->pHandle, sDbFile);
        if (SAL_SOK != sRet)
        {
            PRT_INFO("dspttk_db_open_conn failed, table name: %s, chan: %u\n", sDbFile, i);
            return SAL_FAIL;
        }

        snprintf(pstDbSlice->sName, sizeof(pstDbSlice->sName), "slice%u", i);
        sRet = dspttk_db_create_table(&pstDbSlice->pHandle, pstDbSlice->sName, gastDbSliceKey, SAL_arraySize(gastDbSliceKey));
        if (SAL_SOK != sRet)
        {
            PRT_INFO("dspttk_db_create_table failed, table name: %s, chan: %u\n", pstDbSlice->sName, i);
            dspttk_db_close_conn(pstDbSlice->pHandle);
            pstDbSlice->pHandle = NULL;
            return SAL_FAIL;
        }

        /* 获取条带号的最大值，本次运行过程中以此为基数累加 */
        sRet = dspttk_db_select_max(pstDbSlice->pHandle, pstDbSlice->sName, KEYI_SLICE_NO, &pstDbSlice->u64PrimKeyBase);
        if (SAL_SOK == sRet)
        {
            pstDbSlice->u64PrimKeyBase = ((pstDbSlice->u64PrimKeyBase >> 32) + 1) << 32;
        }
        else
        {
            PRT_INFO("dspttk_db_select_max for '%s' failed, chan: %u\n", KEYI_SLICE_NO, i);
            dspttk_db_close_conn(pstDbSlice->pHandle);
            dspttk_db_destroy(pstDbSlice->pHandle);
            pstDbSlice->pHandle = NULL;
            return SAL_FAIL;
        }

        PRT_INFO("init '%s' success, Handle: %p, PrimKeyBase: 0x%llx\n", pstDbSlice->sName, pstDbSlice->pHandle, pstDbSlice->u64PrimKeyBase);
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_package_storage_init
 * @brief   初始化存储包裹信息的数据库
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：初始化失败
 */
SAL_STATUS dspttk_package_storage_init(void)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 i = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DB_TABLE_ATTR *pstDbPackage = NULL;
    CHAR sDbFile[128] = {0};

    for (i = 0; i < pstShareParam->xrayChanCnt; i++)
    {
        pstDbPackage = &gstDbPackageAttr[i];
        if (SAL_SOK != dspttk_rwlock_init(&pstDbPackage->rwlock))
        {
            PRT_INFO("init 'rwlock' failed, chan: %u\n", i);
            return SAL_FAIL;
        }

        pstDbPackage->pHandle = dspttk_db_init();
        if (NULL == pstDbPackage->pHandle)
        {
            PRT_INFO("oops, dspttk_db_init failed, chan: %u\n", i);
            return SAL_FAIL;
        }

        snprintf(sDbFile, sizeof(sDbFile), "%s/%u/package.db", pstDevCfg->stSettingParam.stEnv.stOutputDir.packageImg, i);
        sRet = dspttk_db_open_conn(pstDbPackage->pHandle, sDbFile);
        if (SAL_SOK != sRet)
        {
            PRT_INFO("dspttk_db_open_conn failed, table name: %s, chan: %u\n", sDbFile, i);
            return SAL_FAIL;
        }

        snprintf(pstDbPackage->sName, sizeof(pstDbPackage->sName), "package%u", i);
        sRet = dspttk_db_create_table(&pstDbPackage->pHandle, pstDbPackage->sName, gastDbPackageKey, SAL_arraySize(gastDbPackageKey));
        if (SAL_SOK != sRet)
        {
            PRT_INFO("dspttk_db_create_table failed, table name: %s, chan: %u\n", pstDbPackage->sName, i);
            dspttk_db_close_conn(pstDbPackage->pHandle);
            pstDbPackage->pHandle = NULL;
            return SAL_FAIL;
        }

        /* 获取包裹ID的最大值，本次运行过程中以此为基数累加 */
        sRet = dspttk_db_select_max(pstDbPackage->pHandle, pstDbPackage->sName, KEYI_PACKAGE_ID, &pstDbPackage->u64PrimKeyBase);
        if (SAL_SOK == sRet)
        {
            pstDbPackage->u64PrimKeyBase = ((pstDbPackage->u64PrimKeyBase >> 32) + 1) << 32;
        }
        else
        {
            PRT_INFO("dspttk_db_select_max for '%s' failed, chan: %u\n", KEYI_PACKAGE_ID, i);
            dspttk_db_close_conn(pstDbPackage->pHandle);
            dspttk_db_destroy(pstDbPackage->pHandle);
            pstDbPackage->pHandle = NULL;
            return SAL_FAIL;
        }

        PRT_INFO("init '%s' success, Handle: %p, PrimKeyBase: 0x%llx\n", pstDbPackage->sName, pstDbPackage->pHandle, pstDbPackage->u64PrimKeyBase);
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_data_storage_init
 * @brief   初始化回调数据的存储功能
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：初始化失败
 */
SAL_STATUS dspttk_data_storage_init(void)
{
    SAL_STATUS sRet = SAL_SOK;

    sRet |= dspttk_slice_storage_init();
    sRet |= dspttk_package_storage_init();

    return sRet;
}


/**
 * @fn      dspttk_query_slice
 * @brief   从条带数据库中查询指定条件的条带记录 
 * @warning 该接口内部有申请内存（pstSlice）的操作，使用完成后需要调用dspttk_query_slice_free()释放 
 * 
 * @param   [IN] pstDbAttr 数据库表属性
 * @param   [OUT] pstSlice 查询得到的条带记录，数组形式，内存中以DB_TABLE_SLICE连续存放
 * @param   [OUT] pu32SliceCnt 查询得到的条带记录条目数
 * @param   [IN] pSelectCond 查询条件，比如WHERE、ORDER BY、LIKE等，可为NULL 
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
static SAL_STATUS dspttk_query_slice(DB_TABLE_ATTR *pstDbAttr, DB_TABLE_SLICE **pstSlice, UINT32 *pu32SliceCnt, const CHAR *pSelectCond)
{
	SAL_STATUS sRet = 0;
	UINT32 i = 0, u32SelectNum = 0, u32SliceCnt = 0;
    UINT32 u32SingleResultStrLen = sizeof(DB_TABLE_SLICE);
	CHAR *pResultsStr = NULL, **pResultsArray = NULL;
    CHAR *pColsValue = NULL, **pColsArray = NULL;
	DB_TABLE_SLICE *pstSliceSel = NULL, *pstSliceSingle = NULL;

    for (i = 0; i < SAL_arraySize(gastDbSliceKey); i++)
    {
        if (gastDbSliceKey[i].type == SQL_TYPE_INTEGER || gastDbSliceKey[i].type == SQL_TYPE_REAL || gastDbSliceKey[i].type == SQL_TYPE_NUMERIC)
        {
            u32SingleResultStrLen += SQL_TYPE_INT2STR_LEN;
        }
        else if (gastDbSliceKey[i].type == SQL_TYPE_DATETIME)
        {
            u32SingleResultStrLen += SQL_TYPE_DATE2STR_LEN;
        }
    }

    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
	sRet = dspttk_db_select(pstDbAttr->pHandle, pstDbAttr->sName, NULL, pSelectCond, NULL, &u32SelectNum);
	if (SAL_SOK == sRet)
	{
		if (u32SelectNum > 0)
		{
            pResultsStr = (CHAR *)malloc(u32SingleResultStrLen * u32SelectNum);
            pResultsArray = (CHAR **)malloc(sizeof(CHAR *) * u32SelectNum);
            if (NULL != pResultsStr && NULL != pResultsArray)
            {
                memset(pResultsStr, 0, u32SingleResultStrLen * u32SelectNum);
                for (i = 0; i < u32SelectNum; i++)
                {
                    pResultsArray[i] = pResultsStr + i * u32SingleResultStrLen;
                }
            }
            else
            {
                PRT_INFO("malloc for pResultsStr(%p) OR pResultsArray(%p) failed\n", pResultsStr, pResultsArray);
                sRet = SAL_FAIL;
                goto EXIT;
            }

            sRet = dspttk_db_select(pstDbAttr->pHandle, pstDbAttr->sName, NULL, pSelectCond, pResultsArray, &u32SelectNum);
            if (SAL_SOK == sRet)
            {
                pColsValue = (CHAR *)malloc(SQL_VAL_STR_LEN_MAX * SAL_arraySize(gastDbSliceKey));
                pColsArray = (CHAR **)malloc(sizeof(CHAR *) * SAL_arraySize(gastDbSliceKey));
                if (NULL != pColsValue && NULL != pColsArray)
                {
                    for (i = 0; i < SAL_arraySize(gastDbSliceKey); i++)
                    {
                        pColsArray[i] = pColsValue + i * SQL_VAL_STR_LEN_MAX;
                    }
                }
                else
                {
                    PRT_INFO("malloc for pColsValue(%p) OR pColsArray(%p) failed\n", pColsValue, pColsArray);
                    sRet = SAL_FAIL;
                    goto EXIT;
                }

                pstSliceSel = (DB_TABLE_SLICE *)malloc(sizeof(DB_TABLE_SLICE) * u32SelectNum);
                if (NULL != pstSliceSel)
                {
                    memset(pstSliceSel, 0, sizeof(DB_TABLE_SLICE) * u32SelectNum);
                    pstSliceSingle = pstSliceSel;
                }
                else
                {
                    PRT_INFO("malloc for pstSliceSel failed, buffer size: %zu\n", sizeof(DB_TABLE_SLICE) * u32SelectNum);
                    sRet = SAL_FAIL;
                    goto EXIT;
                }

                for (i = 0; i < u32SelectNum; i++)
                {
                    memset(pColsValue, 0, sizeof(SQL_VAL_STR_LEN_MAX * SAL_arraySize(gastDbSliceKey)));
                    sRet = dspttk_db_parse_value_row(pstDbAttr->pHandle, pstDbAttr->sName, pResultsArray[i], pColsArray);
                    if (SAL_SOK == sRet)
                    {
                        pstSliceSingle->u64No = strtoul(pColsArray[0], NULL, 0);
                        pstSliceSingle->u32Width = strtoul(pColsArray[1], NULL, 0);
                        pstSliceSingle->u32Height = strtoul(pColsArray[2], NULL, 0);
                        pstSliceSingle->u32Size = strtoul(pColsArray[3], NULL, 0);
                        strcpy(pstSliceSingle->sPath, pColsArray[4]);
                        pstSliceSingle->u64Time = strtoul(pColsArray[5], NULL, 0);
                        pstSliceSingle->enContent = strtol(pColsArray[6], NULL, 0);
                        pstSliceSingle++;
                        u32SliceCnt++;
                    }
                    else
                    {
                        PRT_INFO("dspttk_db_parse_value_row failed, idx: %u/%u, ResultsStr: %s\n", i, u32SelectNum, pResultsArray[i]);
                    }
                }
            }
            else
            {
                PRT_INFO("dspttk_db_select failed, condition: %s\n", (pSelectCond==NULL)?"NULL":pSelectCond);
            }
		}
	}
	else
	{
		PRT_INFO("dspttk_db_select failed, condition: %s\n", (pSelectCond==NULL)?"NULL":pSelectCond);
	}

EXIT:
    if (NULL != pResultsStr)
    {
        free(pResultsStr);
    }
    if (NULL != pResultsArray)
    {
        free(pResultsArray);
    }
    if (NULL != pColsValue)
    {
        free(pColsValue);
    }
    if (NULL != pColsArray)
    {
        free(pColsArray);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    if (SAL_SOK == sRet)
	{
		*pstSlice = pstSliceSel;
		*pu32SliceCnt = u32SliceCnt;
	}
	else
	{
		*pstSlice = NULL;
		*pu32SliceCnt = 0;
		if (NULL != pstSliceSel)
		{
			free(pstSliceSel);
		}
	}

	return sRet;
}


/**
 * @fn      dspttk_query_slice_free
 * @brief   释放dspttk_query_slice_xxx()类查询接口输出的条带记录Buffer
 * 
 * @param   [IN] pstSlice dspttk_query_slice_xxx()类查询接口输出的条带记录Buffer
 */
void dspttk_query_slice_free(DB_TABLE_SLICE *pstSlice)
{
	if (NULL != pstSlice)
	{
		free(pstSlice);
        pstSlice = NULL;
	}

	return;
}


/**
 * @fn      dspttk_query_slice_all
 * @brief   从条带数据库中查询所有的条带记录 
 * @warning 该接口内部有申请内存（pstSlice）的操作，使用完成后需要调用dspttk_query_slice_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstSlice 查询得到的条带记录，数组形式，内存中以DB_TABLE_SLICE连续存放
 * @param   [OUT] pu32Num 查询得到的条带记录条目数
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_slice_all(UINT32 chan, DB_TABLE_SLICE **pstSlice, UINT32 *pu32Num)
{
    DB_TABLE_ATTR *pstDbAttr = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    if (chan < pstShareParam->xrayChanCnt)
    {
        pstDbAttr = &gstDbSliceAttr[chan];
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, pstShareParam->xrayChanCnt);
        return SAL_FAIL;
    }

    if (NULL == pstSlice || NULL == pu32Num)
    {
        PRT_INFO("chan %u, pstSlice(%p) OR pu32Num(%p) is NULL\n", chan, pstSlice, pu32Num);
        return SAL_FAIL;
    }

    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
    if (SAL_SOK != dspttk_query_slice(pstDbAttr, pstSlice, pu32Num, NULL))
    {
        PRT_INFO("dspttk_query_slice failed, table: %s\n", pstDbAttr->sName);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    return SAL_SOK;
}


/**
 * @fn      dspttk_query_slice_lastest_n
 * @brief   从条带数据库中查询最新N个条带记录 
 * @warning 该接口内部有申请内存（pstSlice）的操作，使用完成后需要调用dspttk_query_slice_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstSlice 查询得到的条带记录，数组形式，内存中以DB_TABLE_SLICE连续存放
 * @param   [IN/OUT] pu32Num 输入需要查询的记录条目数（需大于0），输出查询得到的条带记录条目数
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_slice_lastest_n(UINT32 chan, DB_TABLE_SLICE **pstSlice, UINT32 *pu32Num)
{
    CHAR sSelectCond[128] = {0};
    DB_TABLE_ATTR *pstDbAttr = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    if (chan < pstShareParam->xrayChanCnt)
    {
        pstDbAttr = &gstDbSliceAttr[chan];
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, pstShareParam->xrayChanCnt);
        return SAL_FAIL;
    }

    if (NULL == pstSlice || NULL == pu32Num)
    {
        PRT_INFO("chan %u, pstSlice(%p) OR pu32Num(%p) is NULL\n", chan, pstSlice, pu32Num);
        return SAL_FAIL;
    }
    if (0 == *pu32Num)
    {
        PRT_INFO("chan %u, the u32Num is ZERO\n", chan);
        return SAL_FAIL;
    }

    snprintf(sSelectCond, sizeof(sSelectCond), "ORDER BY %s DESC LIMIT %u", KEYI_SLICE_NO, *pu32Num);
    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
    if (SAL_SOK != dspttk_query_slice(pstDbAttr, pstSlice, pu32Num, sSelectCond))
    {
        PRT_INFO("dspttk_query_slice failed, table: %s, SelectCond: %s\n", pstDbAttr->sName, sSelectCond);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    return SAL_SOK;
}


/**
 * @fn      dspttk_query_slice_assigned_n
 * @brief   从条带数据库中查询指定条带号开始的N个条带记录（不包含该条带本身）
 * @warning 该接口内部有申请内存（pstSlice）的操作，使用完成后需要调用dspttk_query_slice_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstSlice 查询得到的条带记录，数组形式，内存中以DB_TABLE_SLICE连续存放
 * @param   [IN/OUT] pu32Num 输入需要查询的记录条目数（需大于0），输出查询得到的条带记录条目数
 * @param   [IN] u64NoStart 开始条带号，查询结果中不包含该条带本身
 * @param   [IN] bReversed 是否逆序查询，TRUE：逆序，查询条带号小于u64NoStart的记录，FALSE：顺序，查询条带号大于u64NoStart的记录
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_slice_assigned_n(UINT32 chan, DB_TABLE_SLICE **pstSlice, UINT32 *pu32Num, UINT64 u64NoStart, BOOL bReversed)
{
    CHAR sSelectCond[128] = {0};
    DB_TABLE_ATTR *pstDbAttr = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    if (chan < pstShareParam->xrayChanCnt)
    {
        pstDbAttr = &gstDbSliceAttr[chan];
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, pstShareParam->xrayChanCnt);
        return SAL_FAIL;
    }

    if (NULL == pstSlice || NULL == pu32Num)
    {
        PRT_INFO("chan %u, pstSlice(%p) OR pu32Num(%p) is NULL\n", chan, pstSlice, pu32Num);
        return SAL_FAIL;
    }
    if (0 == *pu32Num)
    {
        PRT_INFO("chan %u, the u32Num is ZERO\n", chan);
        return SAL_FAIL;
    }

    if (bReversed) // 逆序，选择条带号小于u64NoStart的
    {
        snprintf(sSelectCond, sizeof(sSelectCond), "WHERE %s < %llu ORDER BY %s DESC LIMIT %u", KEYI_SLICE_NO, u64NoStart, KEYI_SLICE_NO, *pu32Num);
    }
    else // 顺序，选择条带号大于u64NoStart的
    {
        snprintf(sSelectCond, sizeof(sSelectCond), "WHERE %s > %llu ORDER BY %s ASC LIMIT %u", KEYI_SLICE_NO, u64NoStart, KEYI_SLICE_NO, *pu32Num);
    }

    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
    if (SAL_SOK != dspttk_query_slice(pstDbAttr, pstSlice, pu32Num, sSelectCond))
    {
        PRT_INFO("dspttk_query_slice failed, table: %s, SelectCond: %s\n", pstDbAttr->sName, sSelectCond);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    return SAL_SOK;
}


/**
 * @fn      dspttk_query_slice_assigned_n
 * @brief   从条带数据库中查询条带号范围内的记录
 * @warning 该接口内部有申请内存（pstSlice）的操作，使用完成后需要调用dspttk_query_slice_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstSlice 查询得到的条带记录，数组形式，内存中以DB_TABLE_SLICE连续存放
 * @param   [OUT] pu32Num 查询得到的条带记录条目数
 * @param   [IN] u64NoStart 起始条带号，可大于u64NoEnd，当大于u64NoEnd时，逆序查询
 * @param   [IN] u64NoEnd 结束条带号，可小于u64NoStart，当小于u64NoStart时，逆序查询
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_slice_by_range(UINT32 chan, DB_TABLE_SLICE **pstSlice, UINT32 *pu32Num, UINT64 u64NoStart, UINT64 u64NoEnd)
{
    CHAR sSelectCond[128] = {0};
    DB_TABLE_ATTR *pstDbAttr = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    if (chan < pstShareParam->xrayChanCnt)
    {
        pstDbAttr = &gstDbSliceAttr[chan];
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, pstShareParam->xrayChanCnt);
        return SAL_FAIL;
    }

    if (NULL == pstSlice || NULL == pu32Num)
    {
        PRT_INFO("chan %u, pstSlice(%p) OR pu32Num(%p) is NULL\n", chan, pstSlice, pu32Num);
        return SAL_FAIL;
    }

    if (u64NoStart <= u64NoEnd)
    {
        snprintf(sSelectCond, sizeof(sSelectCond), "WHERE %s BETWEEN %llu AND %llu ORDER BY %s ASC", KEYI_SLICE_NO, u64NoStart, u64NoEnd, KEYI_SLICE_NO);
    }
    else
    {
        snprintf(sSelectCond, sizeof(sSelectCond), "WHERE %s BETWEEN %llu AND %llu ORDER BY %s DESC", KEYI_SLICE_NO, u64NoEnd, u64NoStart, KEYI_SLICE_NO);
    }

    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
    if (SAL_SOK != dspttk_query_slice(pstDbAttr, pstSlice, pu32Num, sSelectCond))
    {
        PRT_INFO("dspttk_query_slice failed, table: %s, SelectCond: %s\n", pstDbAttr->sName, sSelectCond);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    return SAL_SOK;
}


/**
 * @fn      dspttk_query_package
 * @brief   从包裹数据库中查询指定条件的包裹记录 
 * @warning 该接口内部有申请内存（pstPackage）的操作，使用完成后需要调用dspttk_query_package_free()释放 
 * 
 * @param   [IN] pstDbAttr 数据库表属性
 * @param   [OUT] pstPackage 查询得到的包裹记录，数组形式，内存中以DB_TABLE_PACKAGE连续存放
 * @param   [OUT] pu32PackageCnt 查询得到的包裹记录条目数
 * @param   [IN] pSelectCond 查询条件，比如WHERE、ORDER BY、LIKE等，可为NULL 
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
static SAL_STATUS dspttk_query_package(DB_TABLE_ATTR *pstDbAttr, DB_TABLE_PACKAGE **pstPackage, UINT32 *pu32PackageCnt, const CHAR *pSelectCond)
{
	SAL_STATUS sRet = 0;
	UINT32 i = 0, u32SelectNum = 0, u32PackageCnt = 0;
    UINT32 u32SingleResultStrLen = sizeof(DB_TABLE_PACKAGE);
	CHAR *pResultsStr = NULL, **pResultsArray = NULL;
    CHAR *pColsValue = NULL, **pColsArray = NULL;
	DB_TABLE_PACKAGE *pstPackageSel = NULL, *pstPackageSingle = NULL;

    for (i = 0; i < SAL_arraySize(gastDbPackageKey); i++)
    {
        if (gastDbPackageKey[i].type == SQL_TYPE_INTEGER || gastDbPackageKey[i].type == SQL_TYPE_REAL || gastDbPackageKey[i].type == SQL_TYPE_NUMERIC)
        {
            u32SingleResultStrLen += SQL_TYPE_INT2STR_LEN;
        }
        else if (gastDbPackageKey[i].type == SQL_TYPE_DATETIME)
        {
            u32SingleResultStrLen += SQL_TYPE_DATE2STR_LEN;
        }
    }

    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
	sRet = dspttk_db_select(pstDbAttr->pHandle, pstDbAttr->sName, NULL, pSelectCond, NULL, &u32SelectNum);
	if (SAL_SOK == sRet)
	{
		if (u32SelectNum > 0)
		{
            pResultsStr = (CHAR *)malloc(u32SingleResultStrLen * u32SelectNum);
            pResultsArray = (CHAR **)malloc(sizeof(CHAR *) * u32SelectNum);
            if (NULL != pResultsStr && NULL != pResultsArray)
            {
                memset(pResultsStr, 0, u32SingleResultStrLen * u32SelectNum);
                for (i = 0; i < u32SelectNum; i++)
                {
                    pResultsArray[i] = pResultsStr + i * u32SingleResultStrLen;
                }
            }
            else
            {
                PRT_INFO("malloc for pResultsStr(%p) OR pResultsArray(%p) failed\n", pResultsStr, pResultsArray);
                sRet = SAL_FAIL;
                goto EXIT;
            }

            sRet = dspttk_db_select(pstDbAttr->pHandle, pstDbAttr->sName, NULL, pSelectCond, pResultsArray, &u32SelectNum);
            if (SAL_SOK == sRet)
            {
                pColsValue = (CHAR *)malloc(SQL_VAL_STR_LEN_MAX * SAL_arraySize(gastDbPackageKey));
                pColsArray = (CHAR **)malloc(sizeof(CHAR *) * SAL_arraySize(gastDbPackageKey));
                if (NULL != pColsValue && NULL != pColsArray)
                {
                    for (i = 0; i < SAL_arraySize(gastDbPackageKey); i++)
                    {
                        pColsArray[i] = pColsValue + i * SQL_VAL_STR_LEN_MAX;
                    }
                }
                else
                {
                    PRT_INFO("malloc for pColsValue(%p) OR pColsArray(%p) failed\n", pColsValue, pColsArray);
                    sRet = SAL_FAIL;
                    goto EXIT;
                }

                pstPackageSel = (DB_TABLE_PACKAGE *)malloc(sizeof(DB_TABLE_PACKAGE) * u32SelectNum);
                if (NULL != pstPackageSel)
                {
                    memset(pstPackageSel, 0, sizeof(DB_TABLE_PACKAGE) * u32SelectNum);
                    pstPackageSingle = pstPackageSel;
                }
                else
                {
                    PRT_INFO("malloc for pstPackageSel failed, buffer size: %zu\n", sizeof(DB_TABLE_PACKAGE) * u32SelectNum);
                    sRet = SAL_FAIL;
                    goto EXIT;
                }

                for (i = 0; i < u32SelectNum; i++)
                {
                    memset(pColsValue, 0, sizeof(SQL_VAL_STR_LEN_MAX * SAL_arraySize(gastDbPackageKey)));
                    sRet = dspttk_db_parse_value_row(pstDbAttr->pHandle, pstDbAttr->sName, pResultsArray[i], pColsArray);
                    if (SAL_SOK == sRet)
                    {
                        pstPackageSingle->u64Id = strtoul(pColsArray[0], NULL, 0); // 包裹序号
                        pstPackageSingle->u32Width = strtoul(pColsArray[1], NULL, 0); // 包裹宽
                        pstPackageSingle->u32Height = strtoul(pColsArray[2], NULL, 0); // 包裹高
                        pstPackageSingle->u64SliceStart = strtoul(pColsArray[3], NULL, 0); // 包裹起始条带号
                        pstPackageSingle->u64SliceEnd = strtoul(pColsArray[4], NULL, 0); // 包裹结束条带号
                        pstPackageSingle->u64TimeEnd = strtoul(pColsArray[5], NULL, 0); // 包裹结束时间，系统启动后的相对时间
                        pstPackageSingle->u64TimeCb = strtoul(pColsArray[6], NULL, 0); // 包裹回调时间，系统启动后的相对时间
                        pstPackageSingle->u32AiDgrNum = strtol(pColsArray[7], NULL, 0); // AI危险品识别结果数
                        pstPackageSingle->u32XrIdtNum = strtol(pColsArray[8], NULL, 0); // XSP难穿透&可疑有机物识别结果数
                        pstPackageSingle->enDir = strtol(pColsArray[9], NULL, 0); // 过包显示方向
                        pstPackageSingle->bVMirror = strtol(pColsArray[10], NULL, 0); // 是否垂直镜像
                        strcpy(pstPackageSingle->sInfoPath, pColsArray[11]); // 包裹信息的保存路径
                        strcpy(pstPackageSingle->sJpgPath, pColsArray[12]); // 包裹JPG图片保存路径
                        pstPackageSingle++;
                        u32PackageCnt++;
                    }
                    else
                    {
                        PRT_INFO("dspttk_db_parse_value_row failed, idx: %u/%u, ResultsStr: %s\n", i, u32SelectNum, pResultsArray[i]);
                    }
                }
            }
            else
            {
                PRT_INFO("dspttk_db_select failed, condition: %s\n", (pSelectCond==NULL)?"NULL":pSelectCond);
            }
		}
	}
	else
	{
		PRT_INFO("dspttk_db_select failed, condition: %s\n", (pSelectCond==NULL)?"NULL":pSelectCond);
	}

EXIT:
    if (NULL != pResultsStr)
    {
        free(pResultsStr);
    }
    if (NULL != pResultsArray)
    {
        free(pResultsArray);
    }
    if (NULL != pColsValue)
    {
        free(pColsValue);
    }
    if (NULL != pColsArray)
    {
        free(pColsArray);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    if (SAL_SOK == sRet)
	{
		*pstPackage = pstPackageSel;
		*pu32PackageCnt = u32PackageCnt;
	}
	else
	{
		*pstPackage = NULL;
		*pu32PackageCnt = 0;
		if (NULL != pstPackageSel)
		{
			free(pstPackageSel);
		}
	}

	return sRet;
}


/**
 * @fn      dspttk_query_package_free
 * @brief   释放dspttk_query_package_xxx()类查询接口输出的包裹记录Buffer
 * 
 * @param   [IN] pstPackage dspttk_query_package_xxx()类查询接口输出的包裹记录Buffer
 */
void dspttk_query_package_free(DB_TABLE_PACKAGE *pstPackage)
{
	if (NULL != pstPackage)
	{
		free(pstPackage);
        pstPackage = NULL;
	}

	return;
}


/**
 * @fn      dspttk_query_package_all
 * @brief   从包裹数据库中查询所有的包裹记录 
 * @warning 该接口内部有申请内存（pstPackage）的操作，使用完成后需要调用dspttk_query_package_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstPackage 查询得到的包裹记录，数组形式，内存中以DB_TABLE_PACKAGE连续存放
 * @param   [OUT] pu32Num 查询得到的包裹记录条目数
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_package_all(UINT32 chan, DB_TABLE_PACKAGE **pstPackage, UINT32 *pu32Num)
{
    DB_TABLE_ATTR *pstDbAttr = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    if (NULL != pstShareParam && chan < pstShareParam->xrayChanCnt)
    {
        pstDbAttr = &gstDbPackageAttr[chan];
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, (NULL != pstShareParam) ? pstShareParam->xrayChanCnt : 0);
        return SAL_FAIL;
    }

    if (NULL == pstPackage || NULL == pu32Num)
    {
        PRT_INFO("chan %u, pstPackage(%p) OR pu32Num(%p) is NULL\n", chan, pstPackage, pu32Num);
        return SAL_FAIL;
    }

    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
    if (SAL_SOK != dspttk_query_package(pstDbAttr, pstPackage, pu32Num, NULL))
    {
        PRT_INFO("dspttk_query_package failed, table: %s\n", pstDbAttr->sName);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    return SAL_SOK;
}


/**
 * @fn      dspttk_query_package_lastest_n
 * @brief   从包裹数据库中查询最新N个包裹记录 
 * @warning 该接口内部有申请内存（pstPackage）的操作，使用完成后需要调用dspttk_query_package_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstPackage 查询得到的包裹记录，数组形式，内存中以DB_TABLE_PACKAGE连续存放
 * @param   [IN/OUT] pu32Num 输入需要查询的记录条目数（需大于0），输出查询得到的包裹记录条目数
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_package_lastest_n(UINT32 chan, DB_TABLE_PACKAGE **pstPackage, UINT32 *pu32Num)
{
    CHAR sSelectCond[128] = {0};
    DB_TABLE_ATTR *pstDbAttr = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    if (NULL != pstShareParam && chan < pstShareParam->xrayChanCnt)
    {
        pstDbAttr = &gstDbPackageAttr[chan];
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, (NULL != pstShareParam) ? pstShareParam->xrayChanCnt : 0);
        return SAL_FAIL;
    }

    if (NULL == pstPackage || NULL == pu32Num)
    {
        PRT_INFO("chan %u, pstPackage(%p) OR pu32Num(%p) is NULL\n", chan, pstPackage, pu32Num);
        return SAL_FAIL;
    }
    if (0 == *pu32Num)
    {
        PRT_INFO("chan %u, the u32Num is ZERO\n", chan);
        return SAL_FAIL;
    }

    snprintf(sSelectCond, sizeof(sSelectCond), "ORDER BY %s DESC LIMIT %u", KEYI_PACKAGE_ID, *pu32Num);
    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
    if (SAL_SOK != dspttk_query_package(pstDbAttr, pstPackage, pu32Num, sSelectCond))
    {
        PRT_INFO("dspttk_query_package failed, table: %s, SelectCond: %s\n", pstDbAttr->sName, sSelectCond);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    return SAL_SOK;
}


/**
 * @fn      dspttk_query_package_assigned_n
 * @brief   从包裹数据库中查询指定包裹号开始的N个包裹记录（不包含该包裹本身）
 * @warning 该接口内部有申请内存（pstPackage）的操作，使用完成后需要调用dspttk_query_package_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstPackage 查询得到的包裹记录，数组形式，内存中以DB_TABLE_PACKAGE连续存放
 * @param   [IN/OUT] pu32Num 输入需要查询的记录条目数（需大于0），输出查询得到的包裹记录条目数
 * @param   [IN] u64IdStart 开始包裹号，查询结果中不包含该包裹本身
 * @param   [IN] bReversed 是否逆序查询，TRUE：逆序，查询包裹号小于u64IdStart的记录，FALSE：顺序，查询包裹号大于u64IdStart的记录
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_package_assigned_n(UINT32 chan, DB_TABLE_PACKAGE **pstPackage, UINT32 *pu32Num, UINT64 u64IdStart, BOOL bReversed)
{
    CHAR sSelectCond[128] = {0};
    DB_TABLE_ATTR *pstDbAttr = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    if (NULL != pstShareParam && chan < pstShareParam->xrayChanCnt)
    {
        pstDbAttr = &gstDbPackageAttr[chan];
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, (NULL != pstShareParam) ? pstShareParam->xrayChanCnt : 0);
        return SAL_FAIL;
    }

    if (NULL == pstPackage || NULL == pu32Num)
    {
        PRT_INFO("chan %u, pstPackage(%p) OR pu32Num(%p) is NULL\n", chan, pstPackage, pu32Num);
        return SAL_FAIL;
    }
    if (0 == *pu32Num)
    {
        PRT_INFO("chan %u, the u32Num is ZERO\n", chan);
        return SAL_FAIL;
    }

    if (bReversed) // 逆序，选择包裹ID小于u64IdStart的
    {
        snprintf(sSelectCond, sizeof(sSelectCond), "WHERE %s < %llu ORDER BY %s DESC LIMIT %u", KEYI_PACKAGE_ID, u64IdStart, KEYI_PACKAGE_ID, *pu32Num);
    }
    else // 顺序，选择包裹ID大于u64IdStart的
    {
        snprintf(sSelectCond, sizeof(sSelectCond), "WHERE %s > %llu ORDER BY %s ASC LIMIT %u", KEYI_PACKAGE_ID, u64IdStart, KEYI_PACKAGE_ID, *pu32Num);
    }

    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
    if (SAL_SOK != dspttk_query_package(pstDbAttr, pstPackage, pu32Num, sSelectCond))
    {
        PRT_INFO("dspttk_query_package failed, table: %s, SelectCond: %s\n", pstDbAttr->sName, sSelectCond);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    return SAL_SOK;
}


/**
 * @fn      dspttk_query_package_by_sliceno
 * @brief   从包裹数据库中查询指定条带号范围内涉及的所有包裹记录 
 *          比如#1包裹条带号范围[10, 20]，#2包裹条带号范围[30, 40]，查找条带号范围[15, 32]，则输出#1和#2包裹 
 * @warning 该接口内部有申请内存（pstPackage）的操作，使用完成后需要调用dspttk_query_package_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstPackage 查询得到的包裹记录，数组形式，内存中以DB_TABLE_PACKAGE连续存放
 * @param   [OUT] pu32Num 查询得到的包裹记录条目数
 * @param   [IN] u64SliceNoStart 起始条带号，可大于u64SliceNoEnd，当大于u64SliceNoEnd时，逆序查询
 * @param   [IN] u64SliceNoEnd 结束条带号，可小于u64SliceNoStart，当小于u64SliceNoStart时，逆序查询
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_package_by_sliceno(UINT32 chan, DB_TABLE_PACKAGE **pstPackage, UINT32 *pu32Num, UINT64 u64SliceNoStart, UINT64 u64SliceNoEnd)
{
    CHAR sSelectCond[128] = {0};
    DB_TABLE_ATTR *pstDbAttr = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    if (NULL != pstShareParam && chan < pstShareParam->xrayChanCnt)
    {
        pstDbAttr = &gstDbPackageAttr[chan];
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, (NULL != pstShareParam) ? pstShareParam->xrayChanCnt : 0);
        return SAL_FAIL;
    }

    if (NULL == pstPackage || NULL == pu32Num)
    {
        PRT_INFO("chan %u, pstPackage(%p) OR pu32Num(%p) is NULL\n", chan, pstPackage, pu32Num);
        return SAL_FAIL;
    }

    if (u64SliceNoStart <= u64SliceNoEnd)
    {
        snprintf(sSelectCond, sizeof(sSelectCond), "WHERE %s >= %llu AND %s <= %llu ORDER BY %s ASC", KEYI_PACKAGE_SLICE_END, u64SliceNoStart, 
                 KEYI_PACKAGE_SLICE_START, u64SliceNoEnd, KEYI_PACKAGE_ID);
    }
    else
    {
        snprintf(sSelectCond, sizeof(sSelectCond), "WHERE %s >= %llu AND %s <= %llu ORDER BY %s DESC", KEYI_PACKAGE_SLICE_END, u64SliceNoEnd, 
                 KEYI_PACKAGE_SLICE_START, u64SliceNoStart, KEYI_PACKAGE_ID);
    }

    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
    if (SAL_SOK != dspttk_query_package(pstDbAttr, pstPackage, pu32Num, sSelectCond))
    {
        PRT_INFO("dspttk_query_package failed, table: %s, SelectCond: %s\n", pstDbAttr->sName, sSelectCond);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    return SAL_SOK;
}


/**
 * @fn      dspttk_query_package_by_packageid
 * @brief   从包裹数据库中查询指定包裹ID的记录 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstPackage 查询得到的单个包裹记录
 * @param   [IN] u64PackageId 包裹ID号
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_package_by_packageid(UINT32 chan, DB_TABLE_PACKAGE *pstPackage, UINT64 u64PackageId)
{
    SAL_STATUS sRet = SAL_FAIL;
    CHAR sSelectCond[128] = {0};
    UINT32 u32Num = 0;
    DB_TABLE_ATTR *pstDbAttr = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DB_TABLE_PACKAGE *pstPackageTmp = NULL;

    if (NULL != pstShareParam && chan < pstShareParam->xrayChanCnt)
    {
        pstDbAttr = &gstDbPackageAttr[chan];
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, (NULL != pstShareParam) ? pstShareParam->xrayChanCnt : 0);
        return SAL_FAIL;
    }

    if (NULL == pstPackage)
    {
        PRT_INFO("chan %u, pstPackage is NULL\n", chan);
        return SAL_FAIL;
    }

    snprintf(sSelectCond, sizeof(sSelectCond), "WHERE %s == %llu", KEYI_PACKAGE_ID, u64PackageId);

    dspttk_rwlock_rdlock(&pstDbAttr->rwlock, SAL_TIMEOUT_FOREVER);
    sRet = dspttk_query_package(pstDbAttr, &pstPackageTmp, &u32Num, sSelectCond);
    if (SAL_SOK == sRet && pstPackageTmp != NULL)
    {
        memcpy(pstPackage, pstPackageTmp, sizeof(DB_TABLE_PACKAGE));
    }
    else
    {
        PRT_INFO("dspttk_query_package failed, table: %s, SelectCond: %s\n", pstDbAttr->sName, sSelectCond);
    }
    dspttk_rwlock_unlock(&pstDbAttr->rwlock);

    if (NULL != pstPackageTmp)
    {
        free(pstPackageTmp);
    }

    return sRet;
}


/**
 * @fn      dspttk_slice_nraw_store
 * @brief   存储条带NRaw数据，并将条带信息插入到条带数据库中 
 * @note    该接口以线程形式调用，在dspttk_slice_nraw_cb()中发起，使回调立即返回 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [IN] pstSliceNraw 条带数据，以堆内存形式传入，使用完后需要释放
 */
static void dspttk_slice_nraw_store(UINT32 chan, XSP_SLICE_NRAW *pstSliceNraw)
{
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DB_TABLE_ATTR *pstDbSlice = NULL;
    CHAR sSqlIns[256] = {0}, *pSqlIns = sSqlIns;
    CHAR sSlicePath[128] = {0};
    SAL_STATUS sRet = SAL_SOK;
    DSPTTK_XRAY_RT_SLICE_STATUS *pstRtSliceStatus = NULL;

    if (NULL == pstSliceNraw)
    {
        PRT_INFO("the pstSliceNraw is NULL\n");
        return;
    }

    if (NULL != pstShareParam && chan < pstShareParam->xrayChanCnt)
    {
        pstDbSlice = &gstDbSliceAttr[chan];
        pstRtSliceStatus = &gstGlobalStatus.stRtSliceStatus[chan];
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, (NULL != pstShareParam) ? pstShareParam->xrayChanCnt : 0);
        goto EXIT;
    }

    pstRtSliceStatus->u64SliceNo = pstDbSlice->u64PrimKeyBase | pstSliceNraw->uiColNo;
    pstRtSliceStatus->u32SliceHeight = pstSliceNraw->u32Hight;
    if ((pstRtSliceStatus->u64SliceNoL2R < pstRtSliceStatus->u64SliceNoR2L && pstSliceNraw->uiColNo <= pstRtSliceStatus->u64SliceNoL2R) ||
        (pstRtSliceStatus->u64SliceNoL2R > pstRtSliceStatus->u64SliceNoR2L && pstSliceNraw->uiColNo > pstRtSliceStatus->u64SliceNoR2L))
    {   // 回调的条带在L2R段
        pstRtSliceStatus->enDir = XRAY_DIRECTION_RIGHT;
    }
    else
    {   // 回调的条带在R2L段
        pstRtSliceStatus->enDir = XRAY_DIRECTION_LEFT;
    }

    snprintf(sSlicePath, sizeof(sSlicePath), "%s/%u/%llu.nraw", pstDevCfg->stSettingParam.stEnv.stOutputDir.sliceNraw,
             chan, pstDbSlice->u64PrimKeyBase | pstSliceNraw->uiColNo);
    sRet = dspttk_write_file(sSlicePath, 0, SEEK_SET, pstSliceNraw->pSliceNrawAddr, pstSliceNraw->u32SliceNrawSize);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_write_file '%s' failed\n", sSlicePath);
        goto EXIT;
    }

    dspttk_rwlock_wrlock(&pstDbSlice->rwlock, SAL_TIMEOUT_FOREVER);
    snprintf(sSqlIns, sizeof(sSqlIns), "%llu, %u, %u, %u, %s, %llu, %u,", \
             pstDbSlice->u64PrimKeyBase | pstSliceNraw->uiColNo, pstSliceNraw->u32Width, pstSliceNraw->u32Hight, \
             pstSliceNraw->u32SliceNrawSize, sSlicePath, pstSliceNraw->u64SyncTime, pstSliceNraw->enSliceCont);
    sRet = dspttk_db_insert(pstDbSlice->pHandle, pstDbSlice->sName, &pSqlIns, 1);
    if (sRet != SAL_SOK)
    {
        PRT_INFO("dspttk_db_insert failed, table: %s, SqlIns: %s\n", pstDbSlice->sName, sSqlIns);
    }
    dspttk_rwlock_unlock(&pstDbSlice->rwlock);

EXIT:
    free(pstSliceNraw);
    return;
}


/**
 * @fn      dspttk_slice_nraw_cb
 * @brief   条带NRaw数据的回调接口，发起存储条带数据操作 
 * @note    该回调需要立即返回，存储写文件与插入数据库操作在单独线程中执行 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [IN] pstSliceNraw 回调的条带数据
 */
static void dspttk_slice_nraw_cb(UINT32 chan, XSP_SLICE_NRAW *pstSliceNraw)
{
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    VOID *pDataCbBuf = NULL;
    UINT8 *pu8SliceData = NULL;

    if (NULL != pstShareParam && chan >= pstShareParam->xrayChanCnt)
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, (NULL != pstShareParam) ? pstShareParam->xrayChanCnt : 0);
        return;
    }

    pDataCbBuf = malloc(sizeof(XSP_SLICE_NRAW) + pstSliceNraw->u32SliceNrawSize);
    if (NULL != pDataCbBuf)
    {
        // pDataCbBuf中，前部分存结构体XSP_SLICE_NRAW，后部分存条带的NRAW数据
        pu8SliceData = (UINT8 *)pDataCbBuf + sizeof(XSP_SLICE_NRAW);
        memcpy(pDataCbBuf, pstSliceNraw, sizeof(XSP_SLICE_NRAW));
        memcpy(pu8SliceData, pstSliceNraw->pSliceNrawAddr, pstSliceNraw->u32SliceNrawSize);
        ((XSP_SLICE_NRAW *)pDataCbBuf)->pSliceNrawAddr = pu8SliceData;
        dspttk_pthread_spawn(NULL, DSPTTK_PTHREAD_SCHED_RR, 50, 0x100000, dspttk_slice_nraw_store, 2, chan, (XSP_SLICE_NRAW *)pDataCbBuf);
    }
    else
    {
        PRT_INFO("chan %u, malloc for 'pDataCbBuf' failed, buffer size: %zu\n", 
                 chan, sizeof(XSP_SLICE_NRAW) + pstSliceNraw->u32SliceNrawSize);
    }

    return;
}


/**
 * @fn      dspttk_set_tip_cb
 * @brief   tip插入回调函数
 * 
 * @param   [IN] chan XRay通道号
 * @param   [IN] XSP_TIP_RESULT pstTipResult 回调的包裹数据
 */
static void dspttk_set_tip_cb(UINT32 chan, XSP_TIP_RESULT *pstTipResult)
{
    if (NULL != pstTipResult && pstTipResult->result == 1)
    {
        PRT_INFO("chan %u, TIP insert successfully!!\n", chan);
    }
    else
    {
        PRT_INFO("chan %u, TIP insert failed!!\n", chan);
    }

    return;

}


/**
 * @fn      dspttk_package_cb
 * @brief   包裹数据的回调接口，存储包裹JPG与包裹信息，包裹信息以.bin文件形式存储，并插入到包裹数据库中 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [IN] pstPackage 回调的包裹数据
 */
static void dspttk_package_cb(UINT32 chan, XPACK_RESULT_ST *pstPackage)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 i = 0;
    UINT64 u64PackageId = 0;
    BOOL bTrans = SAL_FALSE;
    CHAR sSqlIns[480] = {0}, *pSqlIns = sSqlIns;
    CHAR sImgFile[128] = {0}, sPackageInfo[128] = {0};
    UINT64 *pu64FirstPackageId = NULL;
    DSPTTK_NODE *pNode = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XRAY_PACKAGE_INFO *pstPackPrm = NULL;
    DB_TABLE_ATTR *pstDbSlice = NULL, *pstDbPackage = NULL;
    if (NULL == pstPackage)
    {
        PRT_INFO("the pstPackage is NULL\n");
        return;
    }

    if (NULL != pstShareParam && chan < pstShareParam->xrayChanCnt)
    {
        pstDbSlice = &gstDbSliceAttr[chan];
        pstDbPackage = &gstDbPackageAttr[chan];
        u64PackageId = pstDbPackage->u64PrimKeyBase | pstPackage->packIndx;
        if (pstPackage->packIndx == 1) /* 存储首个包裹id */
        {
            pu64FirstPackageId = dspttk_get_first_trans_package_id(chan);
            *pu64FirstPackageId = u64PackageId;
        }
        // PRT_INFO("id %llu base %llu idx %u first %llu\n", u64PackageId, pstDbPackage->u64PrimKeyBase, pstPackage->packIndx, *pu64FirstPackageId);
    }
    else
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, (NULL != pstShareParam) ? pstShareParam->xrayChanCnt : 0);
        return;
    }

    /* 未超分的nraw回调 */
    #if 0
    snprintf(sImgFile, sizeof(sImgFile), "%s/%u/%llu_w%u_h%u.nraw", pstDevCfg->stSettingParam.stEnv.stOutputDir.packageImg, 
             chan, u64PackageId, (UINT32)((FLOAT32)pstPackage->width/pstDevCfg->stInitParam.stXray.resizeFactor), 
             (UINT32)((FLOAT32)pstPackage->hight/pstDevCfg->stInitParam.stXray.resizeFactor));
    sRet = dspttk_write_file(sImgFile, 0, SEEK_SET, pstPackage->cXrayAddr, pstPackage->uiOriXraySize);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_write_file '%s' failed\n", sImgFile);
    }
    #endif
    snprintf(sImgFile, sizeof(sImgFile), "%s/%u/%llu.jpg", pstDevCfg->stSettingParam.stEnv.stOutputDir.packageImg, chan, u64PackageId);
    sRet = dspttk_write_file(sImgFile, 0, SEEK_SET, pstPackage->stJpgResultOut.cJpegAddr, pstPackage->stJpgResultOut.uiJpegSize);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_write_file '%s' failed\n", sImgFile);
    }

    snprintf(sPackageInfo, sizeof(sPackageInfo), "%s/%u/%llu.bin", pstDevCfg->stSettingParam.stEnv.stOutputDir.packageImg, chan, u64PackageId);
    sRet = dspttk_write_file(sPackageInfo, 0, SEEK_SET, pstPackage, sizeof(XPACK_RESULT_ST));
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_write_file '%s' failed\n", sPackageInfo);
    }

    dspttk_rwlock_wrlock(&pstDbPackage->rwlock, SAL_TIMEOUT_FOREVER);
    snprintf(sSqlIns, sizeof(sSqlIns), "%llu, %u, %u, %llu, %llu, %llu, %llu, %u, %u, %u, %u, %s, %s,", \
             u64PackageId, pstPackage->width, pstPackage->hight, pstDbSlice->u64PrimKeyBase | pstPackage->igmPrm.stTransferInfo.uiColStartNo, 
             pstDbSlice->u64PrimKeyBase | pstPackage->igmPrm.stTransferInfo.uiColEndNo, pstPackage->igmPrm.stTransferInfo.uiSyncTime, dspttk_get_clock_ms(), 
             pstPackage->stPackSavPrm.target_num, pstPackage->igmPrm.stResultData.stUnpenSent.uiNum+pstPackage->igmPrm.stResultData.stZeffSent.uiNum,
             pstPackage->igmPrm.stTransferInfo.uiNoramlDirection, pstPackage->igmPrm.stTransferInfo.uiIsVerticalFlip, sPackageInfo, sImgFile);
    sRet = dspttk_db_insert(pstDbPackage->pHandle, pstDbPackage->sName, &pSqlIns, 1);
    if (sRet != SAL_SOK)
    {
        PRT_INFO("dspttk_db_insert failed, table: %s, SqlIns: %s\n", pstDbPackage->sName, sSqlIns);
    }
    dspttk_rwlock_unlock(&pstDbPackage->rwlock);

    pstPackPrm = dspttk_get_global_trans_prm(chan);
    if (pstPackPrm == NULL)
    {
        PRT_INFO("error pstPackPrm %p\n", pstPackPrm);
        return;
    }
    /* 首次创建 */
    if (pstPackPrm->lstTranPack == NULL)
    {
        if (NULL == (pstPackPrm->lstTranPack = dspttk_lst_init(TRANS_NODE_NUM)))
        {
            PRT_INFO("chan %u, init 'lstTranPack' failed\n", chan);
        }
        if (SAL_SOK != dspttk_cond_init(&pstPackPrm->lstTranPack->sync))
        {
            PRT_INFO("dspttk_cond_init 'lstTranPack->sync' failed\n");
        }
        for (i = 0; i < TRANS_NODE_NUM; i++)
        {
            if (NULL != (pNode = dspttk_lst_get_idle_node(pstPackPrm->lstTranPack)))
            {
                pNode->pAdData = pstPackPrm->u64PackIdNode + i;
                dspttk_lst_push(pstPackPrm->lstTranPack, pNode);
            }
        }
        for (i = 0; i < TRANS_NODE_NUM; i++)
        {
            dspttk_lst_pop(pstPackPrm->lstTranPack);
        }
    }

    for ( i = 0; i < XRAY_TRANS_BUTT; i++)
    {
       bTrans |= pstDevCfg->stSettingParam.stXray.bTransTypeEn[i];
    }
    if(bTrans)
    {
        dspttk_mutex_lock(&pstPackPrm->lstTranPack->sync.mid, SAL_TIMEOUT_NONE, __FUNCTION__, __LINE__);
        pNode = dspttk_lst_get_idle_node(pstPackPrm->lstTranPack);
        if (pNode != NULL)
        {
            *(UINT64 *)pNode->pAdData = u64PackageId;
            dspttk_lst_push(pstPackPrm->lstTranPack, pNode);
        }
        if (dspttk_sem_give(&pstPackPrm->semEnTrans, __FUNCTION__, __LINE__) == SAL_FAIL)
        {
            PRT_INFO("dspttk_xray_simulate_func failed %p\n", &pstPackPrm->semEnTrans);
            return;
        }
        dspttk_mutex_unlock(&pstPackPrm->lstTranPack->sync.mid,  __FUNCTION__, __LINE__);
    }
    // 自动转存测试
    if (gThrPidTrans[chan] == 0)
    {
        dspttk_pthread_spawn(&gThrPidTrans[chan], DSPTTK_PTHREAD_SCHED_RR, 30, 0x100000, dspttk_xray_trans_package, 2, chan);
    }

    return;
}


/**
 * @fn      dspttk_packseg_cb
 * @brief   包裹分片数据的回调接口
 * 
 * @param   [IN] chan XRay通道号
 * @param   [IN] pstPackage 回调的包裹分片数据
 */
static void dspttk_packseg_cb(UINT32 chan, XPACK_DSP_SEGMENT_OUT *pstPackSeg)
{
    UINT32 j = 0;
    static UINT32 u32CurIdx[2] = {0};
    DSPTTK_NODE *pNode = NULL;
    DSPTTK_SEG_DATA_NODE *pstNodeData = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    CHAR sSegDir[80] = {0}, sImgFile[128] = {0};
    SAL_STATUS sRet = SAL_SOK;
    CHAR sPackFlag[][16] = {"start", "middle", "end"};
    CHAR sImgFmt[][8] = {"raw", "bmp", "jpg"}; // 图像文件后缀

    if (NULL == pstPackSeg)
    {
        PRT_INFO("the pstPackSeg is NULL\n");
        return;
    }

    if (NULL != pstShareParam && chan >= pstShareParam->xrayChanCnt)
    {
        PRT_INFO("the chan(%u) is invalid, should less than %u\n", chan, (NULL != pstShareParam) ? pstShareParam->xrayChanCnt : 0);
        return;
    }

    /* 将分片的id存入节点中最多300个，始终保持最新的300个包裹 */
    if (NULL == gstSegForWeb[chan].lstSeg)
    {
        gstSegForWeb[chan].lstSeg = dspttk_lst_init(DSPTTK_SEG_CNT_FOR_WEB);
        if (NULL != gstSegForWeb[chan].lstSeg)
        {
            for (j = 0; j < DSPTTK_SEG_CNT_FOR_WEB; j++)
            {
                if (NULL != (pNode = dspttk_lst_get_idle_node(gstSegForWeb[chan].lstSeg)))
                {
                    pNode->pAdData = gstSegForWeb[chan].stSegNode + j;
                    dspttk_lst_push(gstSegForWeb[chan].lstSeg, pNode);
                }
            }
            for (j = 0; j < DSPTTK_SEG_CNT_FOR_WEB; j++)
            {
                dspttk_lst_pop(gstSegForWeb[chan].lstSeg);
            }
        }
    }

    snprintf(sSegDir, sizeof(sSegDir), "%s/%u", pstDevCfg->stSettingParam.stEnv.stOutputDir.packageSeg, chan);
    if (NULL != opendir(sSegDir)) // 存在主目录才存储
    {
        snprintf(sSegDir+strlen(sSegDir), sizeof(sSegDir)-strlen(sSegDir), "/%u", pstPackSeg->u32PackIndx);
        if (NULL == opendir(sSegDir)) // 不存在子目录则创建
        {
            if (0 != mkdir(sSegDir, 0777))
            {
                PRT_INFO("mkdir '%s' failed, errno %d, %s\n", sSegDir, errno, strerror(errno));
                return;
            }
        }
        snprintf(sImgFile, sizeof(sImgFile), "%s/%02u_%s_%s_fs%d_w%u_h%u.%s", sSegDir, pstPackSeg->u32SegmentIndx, 
                 (XRAY_DIRECTION_RIGHT==pstPackSeg->Direction)?"l2r":"r2l", sPackFlag[SAL_MIN(pstPackSeg->flag, 2)], pstPackSeg->uiIsForcedToSeparate, 
                 pstPackSeg->stSegmentDataPrm.u32SegmentWidth, pstPackSeg->stSegmentDataPrm.u32SegmentHeight, 
                 sImgFmt[SAL_MIN(pstPackSeg->stSegmentDataPrm.SegmentDataTpye, XPACK_SAVE_BUTT-1)]);
        sRet = dspttk_write_file(sImgFile, 0, SEEK_SET, pstPackSeg->stSegmentDataPrm.pOutBuff, pstPackSeg->stSegmentDataPrm.u32DataSize);
        if (SAL_SOK != sRet)
        {
            PRT_INFO("dspttk_write_file '%s' failed\n", sImgFile);
        }

        /* 更新至链表 */
        if (u32CurIdx[chan] != pstPackSeg->u32PackIndx)
        {
            if (dspttk_lst_get_count(gstSegForWeb[chan].lstSeg) == DSPTTK_SEG_CNT_FOR_WEB)
            {
                /* 从头删除 */
                dspttk_lst_pop(gstSegForWeb[chan].lstSeg);
            }
            /* 从尾部插入 */
            pNode = dspttk_lst_get_idle_node(gstSegForWeb[chan].lstSeg);
            if (NULL != pNode)
            {
                pstNodeData = (DSPTTK_SEG_DATA_NODE *)pNode->pAdData;
                pstNodeData->u32SegPackNo = pstPackSeg->u32PackIndx;
                pstNodeData->u64SegPackTime= dspttk_get_clock_ms();
                pstNodeData->stSegImgData[pstPackSeg->u32SegmentIndx].u32SegmentIndx = pstPackSeg->u32SegmentIndx;
                pstNodeData->Direction = pstPackSeg->Direction;
                pstNodeData->stSegImgData[pstPackSeg->u32SegmentIndx].u32SegmentWidth = pstPackSeg->stSegmentDataPrm.u32SegmentWidth;
                pstNodeData->stSegImgData[pstPackSeg->u32SegmentIndx].u32SegmentHeight = pstPackSeg->stSegmentDataPrm.u32SegmentHeight;
                strcpy(pstNodeData->sSegDir, sSegDir);
                strcpy(pstNodeData->stSegImgData[pstPackSeg->u32SegmentIndx].sSegImgPath, sImgFile);
                dspttk_lst_push(gstSegForWeb[chan].lstSeg, pNode);
            }
        }
        else
        {
            pNode = dspttk_lst_get_tail(gstSegForWeb[chan].lstSeg);
            if (NULL != pNode)
            {
                pstNodeData = (DSPTTK_SEG_DATA_NODE *)pNode->pAdData;
                pstNodeData->stSegImgData[pstPackSeg->u32SegmentIndx].u32SegmentWidth = pstPackSeg->stSegmentDataPrm.u32SegmentWidth;
                pstNodeData->stSegImgData[pstPackSeg->u32SegmentIndx].u32SegmentHeight = pstPackSeg->stSegmentDataPrm.u32SegmentHeight;
                pstNodeData->stSegImgData[pstPackSeg->u32SegmentIndx].SegmentDataTpye = pstPackSeg->stSegmentDataPrm.SegmentDataTpye;
                pstNodeData->stSegImgData[pstPackSeg->u32SegmentIndx].u32SegmentIndx = pstPackSeg->u32SegmentIndx;
                strcpy(pstNodeData->stSegImgData[pstPackSeg->u32SegmentIndx].sSegImgPath, sImgFile);
            }
        }
        // PRT_INFO("idx %u path %s \n", pstPackSeg->u32SegmentIndx, pstNodeData->stSegImgData[pstPackSeg->u32SegmentIndx].sSegImgPath);
        u32CurIdx[chan] = pstPackSeg->u32PackIndx;
    }

    return;
}


/**
 * @fn      dspttk_data_cb
 * @brief   向DSP注册的数据回调接口
 * 
 * @param   [IN] pstElem 回调数据的类型
 * @param   [IN] pBuf 回调数据的Buffer
 * @param   [IN] u32BufLen 回调数据的Buffer长度
 */
void dspttk_data_cb(STREAM_ELEMENT *pstElem, void *pBuf, UINT32 u32BufLen)
{
    if (NULL == pstElem)
    {
        PRT_INFO("the pstElem is NULL\n");
        return;
    }

    switch (pstElem->type)
    {
    case STREAM_ELEMENT_JPEG_IMG:  /* 安检机IPC通道用于人脸JPEG抓图 */
        PRT_INFO("Not support now.\n");
        break;

    case STREAM_ELEMENT_DEC_BMP:
        PRT_INFO("Not support now.\n");
        break;

    case STREAM_ELEMENT_SAVE_BMP:
        PRT_INFO("Not support now.\n");
        break;

    case STREAM_ELEMENT_XRAY_RAW:
        break;

    case STREAM_ELEMENT_PACKAGE_RESULT:
        dspttk_package_cb(pstElem->chan, (XPACK_RESULT_ST *)pBuf);
        break;

    case STREAM_ELEMENT_SLICE_NRAW:
        dspttk_slice_nraw_cb(pstElem->chan, (XSP_SLICE_NRAW *)pBuf);
        break;

    case STREAM_ELEMENT_SEGMENT_XPACK:
        dspttk_packseg_cb(pstElem->chan, (XPACK_DSP_SEGMENT_OUT *)pBuf);
        break;

    case STREAM_ELEMENT_TIP:
        dspttk_set_tip_cb(pstElem->chan, (XSP_TIP_RESULT *)pBuf);
        break;

    default:
        PRT_INFO("unsupport this callback type: %d\n", pstElem->type);
        break;
    }
}
