
#include "xray_preproc.h"
#include "xsp_wrap.h"

/* ============================ 宏/枚举 ============================ */
#define PIXEL_PER_XSENSOR_CARD          64  // 单块探测板的像素数
#define GAUSS_DIST_STAT_BIN             64  // 正态分布统计阶段数
#define DOSAGE_DIST_STAT_BIN            11
#define DOSAGE_STAT_RANGE_MIN           64
#define DOSAGE_STAT_RANGE_MAX           64000
#define SLICE_CONTOURS_NUM_MAX          64  // 单条带中最多轮廓数
#define XRAW_BINARY_PROC_NB             2   /**< 二值图处理的邻域，上下左右各外扩该长度 */
#define XRAW_BINARY_DS_SCALE            2   /**< 二值图下采样尺度，横向和纵向各缩小该倍数 */
#define XRAW_PACK_UNCERTAINTY_LOW       (DOSAGE_DIST_STAT_BIN / 2 - 1) // 中间区域Index[4, 5]为不确定
#define XRAW_PACK_UNCERTAINTY_HIGH      (DOSAGE_DIST_STAT_BIN / 2)

/* ========================= 结构体/联合体 ========================== */
// 横向边界线，横坐标从PointStart开始，长度为BorderLen，纵坐标不同
typedef struct
{
    UINT32 u32HorStart;
    UINT32 u32BorderLen;
    UINT32 *pu32VerYCoor; // 数组个数为xraw_width_max
} BORDER_HOR;

// 纵向边界线，横坐标从PointStart开始，长度为BorderLen，横坐标不同
typedef struct
{
    UINT32 u32VerStart;
    UINT32 u32BorderLen;
    UINT32 *pu32HorXCoor; // 数组个数为xraw_height_max
} BORDER_VER;

// 各方向外接边界线
typedef struct
{
    BORDER_HOR stTop;
    BORDER_HOR stBottom;
    BORDER_VER stLeft;
    BORDER_VER stRight;
} BORDER_CIRCUM;

typedef struct
{
    UINT32 u32XStart;
    UINT32 u32XEnd;
} LINE_TRANSVERSE;

typedef struct
{
    UINT32 u32IdxStart; // 索引起点
    UINT32 u32IdxEnd;   // 索引终点
} GROUP_RANGE;

typedef struct
{
    UINT32 u32ContourNo; // 轮廓编号，从1开始计
    UINT32 u32Area; // 轮廓面积
    INT32 s32ProbHist; // 根据轮廓直方图统计的包裹或空白置信度
    UINT32 u32PackMeas; // 包裹像素（超过归一化值的2000）数统计值
    INT32 s32ProbCompos; // 包裹或空白综合置信度，范围[-100, 100]，大于0倾向于包裹，小于0倾向于空白
    UINT32 u32RelevantCnt; // 轮廓的连续次数
    INT32 s32AvgXLeft; // 左边界均值，邻域不计算在内，-1表示无效值
    INT32 s32AvgXRight; // 右边界均值，邻域不计算在内，-1表示无效值
    LINE_TRANSVERSE stLineTop; // 紧贴条带上沿的边界起点与终点，邻域不计算在内，Start与End相等时无效
    LINE_TRANSVERSE stLineBot; // 紧贴条带下沿的边界起点与终点，邻域不计算在内，Start与End相等时无效
} CONTOUR_ATTR;

typedef struct
{
    UINT32 u32SliceNo;
    BOOL bPseudo; // 是否伪空白条带
    INT32 s32ProbEva; // 整条带是包裹或空白的概率评估
    UINT32 u32ContourNum; // 轮廓数量
    CONTOUR_ATTR astContAttr[SLICE_CONTOURS_NUM_MAX]; // 每个条带最多记录SLICE_CONTOURS_NUM_MAX个可能为包裹的轮廓
} SLICE_PROFILE;

typedef struct
{
    UINT32 u32CurPackNo;
    UINT32 u32SliceIdx; // 取值范围[0, XSP_PACK_HAS_SLICE_NUM_MAX-1]
    SLICE_PROFILE astProfile[XSP_PACK_HAS_SLICE_NUM_MAX];
} SLICE_SEQ;

typedef struct
{
    UINT32 u32HandleIdx;
    UINT32 u32BlankLeftCal;             /**< 左置白区, 根据xraw数据计算, 实际使用该值, 与u32BlankLeftSet比较取max */
    UINT32 u32BlankRightCal;            /**< 右置白区, 根据xraw数据计算, 实际使用该值, 与u32BlankRightSet比较取max*/
    UINT32 u32BlankLeftSet;             /**< 左置白区, 通过HOST_CMD设置 */
    UINT32 u32BlankRightSet;            /**< 右置白区, 通过HOST_CMD设置 */
    UINT16 u16FullCorrManualDetectorMax;/**< 手动满载校正低能最大值(探测器为单位) */
    BOOL *pBXsensorCardValid;           /**< 探测板是否有效，数组元素个数为所有探测板块数 */
    UINT16 *pu16FullCorrManual;         /**< 手动满载校正模板 */
    UINT16 *pu16FullCorrManualDetector; /**< 手动满载校正模板 (按探测板, 一块探测板占64个Pixel)  */
    UINT16 *pu16FullCorrAuto;           /**< 自动满载校正模板 */
    UINT32 *pu32PixelSum;               /**< 用于条带像素求和 */
    pthread_mutex_t mutexProc;          /**< 更新手动校正模板与计算自动校正模板的互斥锁 */
    UINT32 u32Sensitivity;              /**< 自动校正灵敏度，范围[0, 100]，越大越灵敏，默认值50 */

    XIMAGE_DATA_ST stSliceNormalize;    /**< 条带归一化图，DSP_IMG_DATFMT_SP16格式 */
    XIMAGE_DATA_ST stBinarySlice;       /**< 条带二值图，DSP_IMG_DATFMT_SP8格式 */
    XIMAGE_DATA_ST stBinaryDs;          /**< 条带二值图，下采样，基于stBinarySlice，上下各扩2个像素作为邻域，DSP_IMG_DATFMT_SP8格式 */
    XIMAGE_DATA_ST stBinaryErode;       /**< 条带二值图，腐蚀，基于stBinaryDs，上下各扩2个像素作为邻域，DSP_IMG_DATFMT_SP8格式 */
    XIMAGE_DATA_ST stBinaryDilate;      /**< 条带二值图，膨胀，基于stBinaryErode，上下左右各扩2个像素作为邻域，DSP_IMG_DATFMT_SP8格式 */
    UINT32 u32ContourGrpNum;            /**< 轮廓组数，计算过程中的临时变量 */
    GROUP_RANGE aContourGrpRange[SLICE_CONTOURS_NUM_MAX]; /**< 各轮廓在数组paContourPoints中，从哪个索引点开始，到哪个索引点结束 */
    POS_ATTR *paContourPoints;          /**< 轮廓点集，计算过程中的临时变量 */
    BORDER_CIRCUM stBorderCircum;       /**< 轮廓包围边界线，计算过程中的临时变量 */
    SLICE_SEQ stBlankSeq;               /**< 空白条带队列 */
} XRAY_BGAC_PARAM;


/* =================== 函数申明，static && extern =================== */


/* =================== 全局变量，static && extern =================== */
static const UINT32 au32DosageDistPoint[DOSAGE_DIST_STAT_BIN] = {30, 70, 130, 200, 300, 1000, 2000, 4000, 8000, 16000, 65535};
static const INT32 as32ProbCoeff[DOSAGE_DIST_STAT_BIN] = {-64, -28, -16, -10, -6, 0, 60, 240, 900, 3000, 8000};
static UINT32 gu32BgacHandleCnt = 0;


static VOID *xray_mod_malloc(UINT32 u32Size)
{
    return (VOID *)SAL_memMalloc(SAL_align(u32Size, 16), "xray", "preproc");
}


static UINT32 seq_binary_search_range(UINT32 u32Key, const UINT32 au32SeqSmall2Big[], UINT32 u32SeqSize)
{
    UINT32 i = 0;

    if (u32SeqSize <= 1 || NULL == au32SeqSmall2Big)
    {
        return 0;
    }

    for (i = 0; i < u32SeqSize; i++)
    {
        if (u32Key <= au32SeqSmall2Big[i])
        {
            return i;
        }
    }

    return u32SeqSize-1;
}


static void xraw_hist_2bin_2norm(UINT16 *pu16FullCorr, XIMAGE_DATA_ST *pstXRayLe, UINT32 u32BLeft, UINT32 u32BRight, 
                                 UINT32 au32Hist[], XIMAGE_DATA_ST *pstBinary, XIMAGE_DATA_ST *pstNormalize, UINT16 *pu16FullCorrDetector,
                                 UINT16 u16FullCorrDetectorMax)
{
    UINT32 i = 0, j = 0, k = 0;
    UINT32 u32NormVal = 0, u32BinIdx = 0;
    UINT16 *pu16Le = NULL, *pu16NormLine = NULL;
    UINT8 *pu8BinaryLine = NULL;
    UINT32 u32UncertaintyLow = au32DosageDistPoint[XRAW_PACK_UNCERTAINTY_LOW];
    //UINT32 u32UncertaintyHigh = au32DosageDistPoint[XRAW_PACK_UNCERTAINTY_HIGH];
    k = pstXRayLe->u32Width - u32BRight;
    for (j = 0; j < pstXRayLe->u32Height; j++)
    {
        pu16Le = (UINT16 *)pstXRayLe->pPlaneVir[0] + pstXRayLe->u32Width * j;
        pu8BinaryLine = (UINT8 *)pstBinary->pPlaneVir[0] + pstBinary->u32Width * j;
        pu16NormLine = (UINT16 *)pstNormalize->pPlaneVir[0] + pstNormalize->u32Width * j;
        for (i = u32BLeft; i < k; i++)
        {
            if (pu16Le[i] > DOSAGE_STAT_RANGE_MIN && pu16Le[i] < DOSAGE_STAT_RANGE_MAX)
            {
                u32NormVal = SAL_SUB_SAFE(pu16FullCorr[i], pu16Le[i]);
                u32BinIdx = seq_binary_search_range(u32NormVal, au32DosageDistPoint, DOSAGE_DIST_STAT_BIN);
                u32UncertaintyLow = pu16FullCorrDetector[i / PIXEL_PER_XSENSOR_CARD] * 
                                    au32DosageDistPoint[XRAW_PACK_UNCERTAINTY_LOW] / (u16FullCorrDetectorMax);
                pu8BinaryLine[i] = (u32NormVal > u32UncertaintyLow) ? 0 : 255;
                pu16NormLine[i] = (UINT16)u32NormVal;
                au32Hist[u32BinIdx]++;
            }
        }
    }

    return;
}


/* HIST:
   idx  lower  upper      prob
    0 -     0,    29,      -64, 
    1 -    30,    69,      -28, 
    2 -    70,   129,      -16, 
    3 -   130,   199,      -10, 
    4 -   200,   299,       -6, 
    5 -   300,   999,        0, 
    6 -  1000,  1999,       60, 
    7 -  2000,  3999,      240, 
    8 -  4000,  7999,      900, 
    9 -  8000, 15999,     3000, 
   10 - 16000, 65535,     8000,
*/
static INT32 xraw_nopack_prob_by_hist(UINT32 au32Hist[], UINT32 u32Sensitivity)
{
    UINT32 i = 0;
    INT32 s32Proc = 0;
    UINT32 u32BinSum = 0, u32PackSum = 0;
    UINT32 u32PackTh = 0, u32BlankTh = 0;

    u32Sensitivity = SAL_MIN(u32Sensitivity, 100);
    u32PackTh = (10000 + 600 * u32Sensitivity - 4 * u32Sensitivity * u32Sensitivity) / 10000;
    u32BlankTh = XRAW_PACK_UNCERTAINTY_LOW + u32PackTh - 3; // 3: MAX of u32PackTh

    for (i = 0; i < DOSAGE_DIST_STAT_BIN; i++)
    {
        if (i > XRAW_PACK_UNCERTAINTY_HIGH)
        {
            u32PackSum += au32Hist[i];
        }
        u32BinSum += au32Hist[i];
    }
    if (u32PackSum < u32BinSum * u32PackTh / 1000)
    {
        for (i = 0; i <= u32BlankTh; i++)
        {
            s32Proc += as32ProbCoeff[i] * au32Hist[i];
        }
        s32Proc /= (INT32)(u32BinSum - u32PackSum);
    }

    return s32Proc;
}


void set_position(POS_ATTR *p, UINT32 x, UINT32 y)
{
    if (NULL != p)
    {
        p->x = x;
        p->y = y;
    }

    return;
}


void set_pixel_val(XIMAGE_DATA_ST *pstBinary, POS_ATTR *p, UINT8 val)
{
    *((UINT8 *)pstBinary->pPlaneVir[0] + pstBinary->u32Stride[0] * p->y + p->x) = val;

    return;
}


UINT8 get_pixel_val(XIMAGE_DATA_ST *pstBinary, POS_ATTR *p)
{
    return *((UINT8 *)pstBinary->pPlaneVir[0] + pstBinary->u32Stride[0] * p->y + p->x);
}


// 8-step around a pixel counter-clockwise
static void step_ccw8(POS_ATTR *current, POS_ATTR *center)
{
	if (current->x > center->x && current->y >= center->y)      // 右下->右 或 右->右上
    {
        current->y--;
    }
	else if (current->x < center->x && current->y <= center->y) // 左上->左 或 左->左下
    {
        current->y++;
    }
	else if (current->x <= center->x && current->y > center->y) // 左下->下 或 下->右下
    {
        current->x++;
    }
	else if (current->x >= center->x && current->y < center->y) // 右上->上 或 上->左上
    {
		current->x--;
    }

    return;
}


// 8-step around a pixel clockwise
static void step_cw8(POS_ATTR *current, POS_ATTR *center)
{
	if (current->x > center->x && current->y <= center->y)      // 右上->右 或 右->右下
    {
        current->y++;
    }
	else if (current->x < center->x && current->y >= center->y) // 左下->左 或 左->左上
    {
        current->y--;
    }
	else if (current->x >= center->x && current->y > center->y) // 右下->下 或 下->左下
    {
        current->x--;
    }
	else if (current->x <= center->x && current->y < center->y) // 左上->上 或 上->右上
    {
		current->x++;
    }

    return;
}


static BOOL isTheSamePoint(POS_ATTR *pA, POS_ATTR *pB)
{
    return (pA->x == pB->x && pA->y == pB->y);
}


static void swapPoint(POS_ATTR *pA, POS_ATTR *pB)
{
    SAL_SWAP(pA->x, pB->x);
    SAL_SWAP(pA->y, pB->y);

    return;
}


static INT32 pointSortX(const void *pA, const void *pB)
{
    return (INT32)((INT32)((POS_ATTR *)pA)->x - (INT32)((POS_ATTR *)pB)->x);
}


static INT32 pointSortY(const void *pA, const void *pB)
{
    return (INT32)((INT32)((POS_ATTR *)pA)->y - (INT32)((POS_ATTR *)pB)->y);
}


/**
 * @fn      follow_contour
 * @brief   跟踪外轮廓
 * 
 * @param   [IN] pstBinary 二值图，这里是需要膨胀腐蚀后的
 * @param   [IN] pBorderOuter 判定起始点时的左侧点，在轮廓外，不属于轮廓
 * @param   [IN] pBorderInner 判定起始点时的右侧点，在轮廓内，属于轮廓
 * @param   [IN] paContourPoints 存储边界点的数组缓存
 * @param   [IN] u32ContourPixelVal 轮廓像素值，便于区分不同的轮廓
 * 
 * @return  UINT32 轮廓像素点数量，0则表示跟踪轮廓异常
 */
static UINT32 follow_contour(XIMAGE_DATA_ST *pstBinary, POS_ATTR *pBorderOuter, POS_ATTR *pBorderInner, POS_ATTR *paContourPoints, UINT32 u32ContourPixelVal)
{
    UINT32 i = 0, u32ContourPointsNum = 0;
    INT32 s32CycleCnt = 0;
    UINT8 u8PixelVal = 0;
    POS_ATTR pointCurrent = {0}, pointCenter = {0};
    POS_ATTR pointStart = {0};  // 指向border的起始点，在轮廓跟踪过程中，该指针始终不变，在判断跟踪是否结束的时候用到它
    POS_ATTR pointLast = {0};   // 指向border的最后一个点，在轮廓跟踪过程中，该指针始终不变，在判断跟踪是否结束的时候用到它
                                // 轮廓算法中首先找到起始点，紧接着就寻找这个点，找到这个点之后才开始寻找轮廓的第二个像素点
    #ifdef FOLLOW_CONTOUR_PRTDBG
    CHAR sDbgStr[131072] = {0}, sDbgName[64] = {0};
    #endif

    set_position(&pointStart, pBorderInner->x, pBorderInner->y);
    set_position(&pointCurrent, pBorderOuter->x, pBorderOuter->y);
    pointCenter = pointStart;

    s32CycleCnt = 8;
    while (s32CycleCnt-- > 0)
    {
        step_cw8(&pointCurrent, &pointCenter); // 以起点为中心，顺时针8-邻域寻找最后一个Border Point
        if (isTheSamePoint(&pointCurrent, pBorderOuter)) // pointStart为孤点，不认为是边缘
        {
            return 0;
        }
        else
        {
            if (get_pixel_val(pstBinary, &pointCurrent) == 0) // Bingo the Last Border Point
            {
                pointLast = pointCurrent;
                paContourPoints[u32ContourPointsNum++] = pointCenter;
                break;
            }
        }
    }
    if (s32CycleCnt <= 0) // 循环被动结束，非主动跳出
    {
        XSP_LOGW("8-step clockwise out, CycleCnt %d\n", s32CycleCnt);
        return 0;
    }
    #ifdef FOLLOW_CONTOUR_PRTDBG
    snprintf(sDbgStr, sizeof(sDbgStr), "start %d(%d, %d), last %d(%d, %d)\n", get_pixel_val(pstBinary, &pointStart), 
             pointStart.x, pointStart.y, get_pixel_val(pstBinary, &pointLast), pointLast.x, pointLast.y);
    #endif

    // 以pointStart为中心，逆时针8-邻域寻找下一个Border Point
    s32CycleCnt = 8;
    while (s32CycleCnt-- > 0)
    {
        #ifdef FOLLOW_CONTOUR_PRTDBG
        if (7 == s32CycleCnt)
        {
            snprintf(sDbgStr+strlen(sDbgStr), sizeof(sDbgStr)-strlen(sDbgStr), 
                     "(%d, %d), (%d, %d) -> ", pointCenter.x, pointCenter.y, pointCurrent.x, pointCurrent.y);
        }
        else
        {
            snprintf(sDbgStr+strlen(sDbgStr), sizeof(sDbgStr)-strlen(sDbgStr), 
                     "(%d, %d) -> ", pointCurrent.x, pointCurrent.y);
        }
        #endif
        step_ccw8(&pointCurrent, &pointCenter); // 采用弱连通算法，即对角线也视为连通
        u8PixelVal = get_pixel_val(pstBinary, &pointCurrent);
        if (u8PixelVal == 0 || u8PixelVal == u32ContourPixelVal) // Bingo the Next Border Point
        {
            if (isTheSamePoint(&pointStart, &pointCurrent) && isTheSamePoint(&pointLast, &pointCenter)) // 回到起始点，结束
            {
                #ifdef FOLLOW_CONTOUR_PRTDBG
                snprintf(sDbgStr+strlen(sDbgStr), sizeof(sDbgStr)-strlen(sDbgStr), "(%d, %d) End %d\n", pointCurrent.x, pointCurrent.y, s32CycleCnt);
                #endif
                set_pixel_val(pstBinary, &pointStart, u32ContourPixelVal);
                break;
            }
            else
            {
                #ifdef FOLLOW_CONTOUR_PRTDBG
                snprintf(sDbgStr+strlen(sDbgStr), sizeof(sDbgStr)-strlen(sDbgStr), "(%d, %d) Next %d\n", pointCurrent.x, pointCurrent.y, s32CycleCnt);
                #endif
                paContourPoints[u32ContourPointsNum++] = pointCurrent;
                set_pixel_val(pstBinary, &pointCurrent, u32ContourPixelVal); // 重置边界的值，便于从图像上区分各边界
                swapPoint(&pointCurrent, &pointCenter); // 交换pointCurrent与pointCenter
                s32CycleCnt = 8; // 重置循环次数
            }
        }
    }

    #ifdef FOLLOW_CONTOUR_PRTDBG
    snprintf(sDbgName, sizeof(sDbgName), "/mnt/dump/no%05u_t%llu_%d_%u.txt", pstBinary->stMbAttr.poolId, sal_get_tickcnt(), s32CycleCnt, u32ContourPixelVal);
    SAL_WriteToFile(sDbgName, 0, SEEK_SET, sDbgStr, strlen(sDbgStr));
    #endif

    if (s32CycleCnt > 0)
    {
        return u32ContourPointsNum;
    }
    else // 循环被动结束，非主动跳出
    {
        XSP_LOGW("slice %u, 8-step counter-clockwise out, CycleCnt %d\n", pstBinary->stMbAttr.poolId, s32CycleCnt);
        for (i = 0; i < u32ContourPointsNum; i++) // 回退边界值
        {
            set_pixel_val(pstBinary, paContourPoints+i, 0);
        }
        return 0;
    }
}


static void trace_border_hor(POS_ATTR *paContourPoints, UINT32 u32BorderPointsNum, BORDER_HOR *pstTop, BORDER_HOR *pstBottom)
{
    UINT32 j = 0, k = 0;
    POS_ATTR *pPointCur = NULL, *pPointNext = NULL;

    qsort(paContourPoints, u32BorderPointsNum, sizeof(POS_ATTR), pointSortX); // 对横坐标进行从小到大排序
    pstTop->u32HorStart = paContourPoints->x; // 取横坐标X最小值为起始点
    pstBottom->u32HorStart = paContourPoints->x;
    pstTop->u32BorderLen = 0;
    pstBottom->u32BorderLen = 0;

    for (j = 1; j < u32BorderPointsNum; j++)
    {
        k = 1; // 统计相同横坐标的Points数量
        pPointNext = paContourPoints + j;
        pPointCur = pPointNext - 1;
        while (pPointCur->x == pPointNext->x && j < u32BorderPointsNum) // 横坐标相同时，对纵坐标进行从小到大排序
        {
            pPointCur++;
            pPointNext++;
            k++;
            j++;
        }
        if (k > 1)
        {
            qsort(pPointNext - k, k, sizeof(POS_ATTR), pointSortY); // 对纵坐标进行从小到大排序
        }

        // 取纵坐标Y最小值为上沿
        pstTop->pu32VerYCoor[pstTop->u32BorderLen] = (pPointNext-k)->y;
        pstTop->u32BorderLen++;
        // 取纵坐标Y最大值为下沿
        pstBottom->pu32VerYCoor[pstBottom->u32BorderLen] = pPointCur->y;
        pstBottom->u32BorderLen++;
    }

    #ifdef TRACE_BORDER_PRTDBG
    printf("horizontal border top, x: %u, y(%u): ", pstTop->u32HorStart, pstTop->u32BorderLen);
    for (j = 0; j < pstTop->u32BorderLen; j++)
    {
        printf("%3d ", pstTop->pu32VerYCoor[j]);
    }
    printf("\n");
    printf("horizontal border bot, x: %u, y(%u): ", pstBottom->u32HorStart, pstBottom->u32BorderLen);
    for (j = 0; j < pstBottom->u32BorderLen; j++)
    {
        printf("%3d ", pstBottom->pu32VerYCoor[j]);
    }
    printf("\n");
    #endif

    return;
}


static void trace_border_ver(POS_ATTR *paContourPoints, UINT32 u32BorderPointsNum, BORDER_VER *pstLeft, BORDER_VER *pstRight, 
                             XIMAGE_DATA_ST *pstSliceNorm, UINT32 u32DsScale, UINT32 u32NbSize, UINT32 au32Hist[], UINT32 *pu32BinSum)
{
    UINT32 i = 0, j = 0, k = 0, m = 0, n = 0, z = 0;
    POS_ATTR *pPointCur = NULL, *pPointNext = NULL, *pPointStart = NULL, *pPointEnd = NULL;
    UINT16 *pu16NormLine = NULL;
    UINT32 u32BinSum = 0, u32BinIdx = 0;
    
    qsort(paContourPoints, u32BorderPointsNum, sizeof(POS_ATTR), pointSortY); // 对纵坐标进行从小到大排序
    pstLeft->u32VerStart = paContourPoints->y; // 取纵坐标Y最小值为起始点
    pstRight->u32VerStart = paContourPoints->y;
    pstLeft->u32BorderLen = 0;
    pstRight->u32BorderLen = 0;

    for (j = 1; j < u32BorderPointsNum; j++)
    {
        k = 1; // 统计相同纵坐标的Points数量
        pPointNext = paContourPoints + j;
        pPointCur = pPointNext - 1;
        while (pPointCur->y == pPointNext->y && j < u32BorderPointsNum) // 纵坐标相同时，对横坐标进行从小到大排序
        {
            pPointCur++;
            pPointNext++;
            k++;
            j++;
        }
        pPointStart = pPointNext - k;
        pPointEnd = pPointCur;

        if (k > 1)
        {
            qsort(pPointStart, k, sizeof(POS_ATTR), pointSortX); // 对横坐标进行从小到大排序

            pPointCur = pPointStart;
            pPointNext = pPointCur + 1;
            while (1)
            {
                if (pPointNext->x - pPointCur->x == 1) // 相邻
                {
                    while ((pPointNext < pPointEnd) && ((pPointNext+1)->x - pPointNext->x == 1)) // 从pPointCur->x到pPointNext->x连续相邻
                    {
                        pPointNext++;
                    }
                }

                // 分段统计
                m = (pPointCur->x - u32NbSize) * u32DsScale;
                n = (pPointNext->x - u32NbSize + 1) * u32DsScale;
                pu16NormLine = (UINT16 *)pstSliceNorm->pPlaneVir[0] + pstSliceNorm->u32Width * (pPointStart->y - u32NbSize) * u32DsScale;
                for (z = 0; z < u32DsScale; z++)
                {
                    for (i = m; i < n; i++)
                    {
                        u32BinIdx = seq_binary_search_range(pu16NormLine[i], au32DosageDistPoint, DOSAGE_DIST_STAT_BIN);
                        au32Hist[u32BinIdx]++;
                        u32BinSum++;
                    }
                    pu16NormLine += pstSliceNorm->u32Width;
                }

                if (pPointNext + 1 < pPointEnd) // 统计下一段
                {
                    pPointCur = pPointNext;
                    pPointNext += 2;
                }
                else
                {
                    break;
                }
            }
        }

        // 取横坐标X最小值为左沿
        pstLeft->pu32HorXCoor[pstLeft->u32BorderLen] = pPointStart->x;
        pstLeft->u32BorderLen++;
        // 取横坐标X最大值为右沿
        pstRight->pu32HorXCoor[pstRight->u32BorderLen] = pPointEnd->x;
        pstRight->u32BorderLen++;
    }

    *pu32BinSum = u32BinSum;

    #ifdef TRACE_BORDER_PRTDBG
    printf("vertical border left,  y: %u, x(%u): ", pstLeft->u32VerStart, pstLeft->u32BorderLen);
    for (j = 0; j < pstLeft->u32BorderLen; j++)
    {
        printf("%3d ", pstLeft->pu32HorXCoor[j]);
    }
    printf("\n");
    printf("vertical border right, y: %u, x(%u): ", pstRight->u32VerStart, pstRight->u32BorderLen);
    for (j = 0; j < pstRight->u32BorderLen; j++)
    {
        printf("%3d ", pstRight->pu32HorXCoor[j]);
    }
    printf("\n");
    #endif

    return;
}


static SAL_STATUS find_contours(XIMAGE_DATA_ST *pstSliceNorm, XIMAGE_DATA_ST *pstBinary, UINT32 u32NbSize, SLICE_PROFILE *pstProfile,
                                UINT32 *u32ContourGrpNum, GROUP_RANGE aContourGrpRange[], POS_ATTR *paContourPoints, BORDER_CIRCUM *pstBorderCircum)
{
    INT32 i = 0, j = 0, k = 0;
    UINT32 u32ContourPointOffset = 0, u32ContourPointsNum = 0;
    UINT32 u32ContourGrpIdx = 0;
    UINT32 u32NbRight = 0, u32NbBottom = 0;
    UINT32 u32BinSum = 0, au32Hist[DOSAGE_DIST_STAT_BIN] = {0};
    UINT32 u32AvgX = 0, u32StatTop = 0, u32StatBottom = 0;
    UINT8 *pu8Line = NULL;
    const UINT32 u32StickCloseRatioTh = 500; // 紧贴上下边缘判定阈值，Q10格式
    const UINT32 u32StickClosePixelTh = 3; // 紧贴上下边缘判定阈值，至少3个像素点
    POS_ATTR pointLeft = {0}, pointRight = {0};
    CONTOUR_ATTR *pstAttr = NULL;

    if (NULL == pstBinary || 0 == u32NbSize)
    {
        return SAL_FAIL;
    }

    u32NbRight = pstBinary->u32Width - u32NbSize;
    u32NbBottom = pstBinary->u32Height - u32NbSize;
    for (i = u32NbSize; i < u32NbBottom; i++)
    {
        pu8Line = (UINT8 *)pstBinary->pPlaneVir[0] + pstBinary->u32Width * i;
		for (j = u32NbSize; j < u32NbRight; j++)
        {
			if ((pu8Line[j - 1] == 0xFF) && (pu8Line[j] == 0)) // find the contour
            {
				set_position(&pointLeft, j - 1, i);
				set_position(&pointRight, j, i);
				u32ContourPointsNum = follow_contour(pstBinary, &pointLeft, &pointRight, paContourPoints+u32ContourPointOffset, u32ContourGrpIdx+1);
                if (u32ContourPointsNum > 0)
                {
                    memset(au32Hist, 0, sizeof(au32Hist));
                    trace_border_ver(paContourPoints+u32ContourPointOffset, u32ContourPointsNum, &pstBorderCircum->stLeft, &pstBorderCircum->stRight, 
                                     pstSliceNorm, XRAW_BINARY_DS_SCALE, u32NbSize, au32Hist, &u32BinSum);
                    if (u32BinSum < pstSliceNorm->u32Height * 2) // 过滤太小区域
                    {
                        goto NEXT;
                    }

                    if (pstProfile->u32ContourNum < SLICE_CONTOURS_NUM_MAX)
                    {
                        pstAttr = pstProfile->astContAttr + pstProfile->u32ContourNum++;
                        memset(pstAttr, 0, sizeof(CONTOUR_ATTR));
                        pstAttr->u32ContourNo = u32ContourGrpIdx + 1;
                        pstAttr->u32Area = u32BinSum;
                        pstAttr->s32AvgXLeft = -1; // 初始化为非法值
                        pstAttr->s32AvgXRight = -1;
                    }
                    else
                    {
                        XSP_LOGW("contours are too many...\n");
                        goto END; // 如果仅计算概率，是不需要跳出的，存不下也没有关系
                    }

                    // 统计上下边界紧贴边缘程度
                    trace_border_hor(paContourPoints+u32ContourPointOffset, u32ContourPointsNum, &pstBorderCircum->stTop, &pstBorderCircum->stBottom);

                    // 上边界
                    u32StatTop = 0;
                    for (k = 0; k < pstBorderCircum->stTop.u32BorderLen; k++)
                    {
                        if (pstBorderCircum->stTop.pu32VerYCoor[k] == u32NbSize)
                        {
                            u32StatTop++;
                        }
                    }
                    if (u32StatTop > u32StickClosePixelTh)
                    {
                        for (k = 0; k < pstBorderCircum->stTop.u32BorderLen; k++)
                        {
                            if (pstBorderCircum->stTop.pu32VerYCoor[k] == u32NbSize || 
                                pstBorderCircum->stTop.pu32VerYCoor[k] == u32NbSize + 1)
                            {
                                pstAttr->stLineTop.u32XStart = pstBorderCircum->stTop.u32HorStart + k - u32NbSize;
                                break;
                            }
                        }
                        for (k = pstBorderCircum->stTop.u32BorderLen-1; k >= 0; k--)
                        {
                            if (pstBorderCircum->stTop.pu32VerYCoor[k] == u32NbSize || 
                                pstBorderCircum->stTop.pu32VerYCoor[k] == u32NbSize + 1)
                            {
                                pstAttr->stLineTop.u32XEnd = pstBorderCircum->stTop.u32HorStart + k - u32NbSize;
                                break;
                            }
                        }
                    }

                    // 下边界
                    u32StatBottom = 0;
                    for (k = 0; k < pstBorderCircum->stBottom.u32BorderLen; k++)
                    {
                        if (pstBorderCircum->stBottom.pu32VerYCoor[k] == u32NbBottom - 1)
                        {
                            u32StatBottom++;
                        }
                    }
                    if (u32StatBottom > u32StickClosePixelTh)
                    {
                        for (k = 0; k < pstBorderCircum->stBottom.u32BorderLen; k++)
                        {
                            if (pstBorderCircum->stBottom.pu32VerYCoor[k] == u32NbBottom - 1 || 
                                pstBorderCircum->stBottom.pu32VerYCoor[k] == u32NbBottom - 2)
                            {
                                pstAttr->stLineBot.u32XStart = pstBorderCircum->stBottom.u32HorStart + k - u32NbSize;
                                break;
                            }
                        }
                        for (k = pstBorderCircum->stBottom.u32BorderLen-1; k >= 0; k--)
                        {
                            if (pstBorderCircum->stBottom.pu32VerYCoor[k] == u32NbBottom - 1 || 
                                pstBorderCircum->stBottom.pu32VerYCoor[k] == u32NbBottom - 2)
                            {
                                pstAttr->stLineBot.u32XEnd = pstBorderCircum->stBottom.u32HorStart + k - u32NbSize;
                                break;
                            }
                        }
                    }

                    // 根据轮廓内图像直方图判定包裹的概率
                    pstAttr->s32ProbHist = 0;
                    for (k = 0; k < DOSAGE_DIST_STAT_BIN; k++)
                    {
                        pstAttr->s32ProbHist += as32ProbCoeff[k] * (INT32)au32Hist[k];
                        if (k > DOSAGE_DIST_STAT_BIN / 2 + 1)
                        {
                            pstAttr->u32PackMeas += au32Hist[k];
                        }
                    }

                    if (pstAttr->s32ProbHist > 0 && pstAttr->u32PackMeas > SAL_MAX(u32BinSum / 200, 10)) // 体现包裹剂量的点超过0.5%，注：10个点以下忽略
                    {
                        pstAttr->s32ProbCompos = 99; // 十分确信是包裹
                    }
                    else
                    {
                        // 根据左右边界斜率，计算非包裹的置信度
                        if ((0 == pstBorderCircum->stLeft.u32VerStart - u32NbSize) && (u32NbBottom - u32NbSize == pstBorderCircum->stLeft.u32BorderLen) &&
                            (0 == pstBorderCircum->stRight.u32VerStart - u32NbSize) && (u32NbBottom - u32NbSize == pstBorderCircum->stRight.u32BorderLen))
                        {
                            if (u32StatTop > u32StickClosePixelTh && (u32StatTop << 10) / pstBorderCircum->stTop.u32BorderLen > u32StickCloseRatioTh &&
                                u32StatBottom > u32StickClosePixelTh && (u32StatBottom << 10) / pstBorderCircum->stBottom.u32BorderLen > u32StickCloseRatioTh)
                            {
                                // 计算左边界斜率
                                u32AvgX = 0;
                                for (k = 0; k < pstBorderCircum->stLeft.u32BorderLen; k++)
                                {
                                    u32AvgX += pstBorderCircum->stLeft.pu32HorXCoor[k] - u32NbSize;
                                }
                                pstAttr->s32AvgXLeft = (u32AvgX + pstBorderCircum->stLeft.u32BorderLen / 2) / pstBorderCircum->stLeft.u32BorderLen;

                                // 计算右边界斜率
                                u32AvgX = 0;
                                for (k = 0; k < pstBorderCircum->stRight.u32BorderLen; k++)
                                {
                                    u32AvgX += pstBorderCircum->stRight.pu32HorXCoor[k] - u32NbSize + 1;
                                }
                                pstAttr->s32AvgXRight = (u32AvgX + pstBorderCircum->stRight.u32BorderLen / 2) / pstBorderCircum->stRight.u32BorderLen;
                            }
                        }
                    }

                NEXT:
                    // 记录每组轮廓的点集
                    if (u32ContourGrpIdx >= *u32ContourGrpNum)
                    {
                        goto END;
                    }
   
                    aContourGrpRange[u32ContourGrpIdx].u32IdxStart = u32ContourPointOffset;
                    u32ContourPointOffset += u32ContourPointsNum;
                    aContourGrpRange[u32ContourGrpIdx].u32IdxEnd = u32ContourPointOffset - 1;

                    u32ContourGrpIdx++;
                    
                    for (k = u32NbRight-1; k > j; k--) // jump the current border
                    {
                        if (pu8Line[k] == pu8Line[j])
                        {
                            break;
                        }
                    }
                    j = k;
                }
                else
                {
                    pu8Line[j] = 0xFF; // 置当前点为背景
                }
            }
            else if ((pu8Line[j - 1] == 0xFF) && (pu8Line[j] > 0 && pu8Line[j] < 0xFF)) // jump the known border
            {
                for (k = u32NbRight-1; k > j; k--)
                {
                    if (pu8Line[k] == pu8Line[j])
                    {
                        break;
                    }
                }
                j = k;
            }
        }
	}

END:
    *u32ContourGrpNum = u32ContourGrpIdx;

    return SAL_SOK;
}

static void resize_binary_mem_param(XRAY_BGAC_PARAM *pstBgacParam, XIMAGE_DATA_ST *pstXrawCur)
{
    if (NULL == pstBgacParam || NULL == pstXrawCur)
    {
        XSP_LOGE("Param In Failed. BgacParam %p, Xraw %p\n", pstBgacParam, pstXrawCur);
        return;
    }
    
    pstBgacParam->stBinarySlice.u32Height = pstXrawCur->u32Height;

    pstBgacParam->stBinaryDs.u32Height = pstBgacParam->stBinarySlice.u32Height / XRAW_BINARY_DS_SCALE + 2 * XRAW_BINARY_PROC_NB;
    
    pstBgacParam->stBinaryErode.u32Height = pstBgacParam->stBinaryDs.u32Height;

    pstBgacParam->stBinaryDilate.u32Height = pstBgacParam->stBinaryDs.u32Height;

    return;
}

static INT32 calc_weight_prob(INT32 s32ProbCur, INT32 s32ProbPre)
{
    const INT32 as32GaussWeight[4] = {56, 34, 8, 2};
    INT32 s32Prob = s32ProbPre;
    INT32 s32WeightSum = as32GaussWeight[0] + as32GaussWeight[1];

    if (s32ProbCur * s32ProbPre < 0) // 属性相相反，做二次加权
    {
        s32Prob = s32ProbPre * as32GaussWeight[1] / s32WeightSum;
    }

    return (s32ProbCur * as32GaussWeight[0] + s32Prob * as32GaussWeight[1]) / s32WeightSum;
}


/**
 * @fn      xray_bg_auto_corr
 * @brief   背景自动校正处理
 * 
 * @param   [IN] pHandle 背景自动校正算法句柄，xray_bgac_init()返回
 * @param   [IN] pstXrawIn 采传的实时预览XRAW条带数据，包括XRAY_TYPE_NORMAL与XRAY_TYPE_PSEUDO_BLANK两种类型
 * @param   [IN] bPseudo 是否为伪空白条带，即XRAY_TYPE_PSEUDO_BLANK类型数据
 * @param   [IN] u32SliceNo 条带号，仅用于调试
 * @param   [OUT] pu16FullCorr 自适应的满载模板，仅1行数据
 * 
 * @return  SAL_STATUS SAL_SOK：满载模板有更新，SAL_FAIL：满载模板无更新
 */
SAL_STATUS xray_bg_auto_corr(void *pHandle, XIMAGE_DATA_ST *pstXrawIn, BOOL bPseudo, UINT32 u32SliceNo, UINT16 *pu16FullCorr)
{
    UINT32 au32Hist[DOSAGE_DIST_STAT_BIN] = {0};
    UINT8 *pL0 = NULL, *pL1 = NULL, *pL2 = NULL, *pDst = NULL;
    UINT32 i = 0, j = 0, k = 0, m = 0, n = 0, loop1 = 0, loop2 = 0;
    INT32 s32ProbSpatial = 0, s32ProbPre = 0, s32ProbCoupl = 0; // 包裹或空白置信度，范围[-100, 100]，大于0倾向于包裹，小于0倾向于空白
    UINT32 binTh = 0xF0 * (XRAW_BINARY_DS_SCALE * XRAW_BINARY_DS_SCALE - 1) / (XRAW_BINARY_DS_SCALE * XRAW_BINARY_DS_SCALE);
    UINT32 u32JumpPackCnt = 0;
    INT32 s32WeightA = 0, s32WeightB = 0; /* 新的自动满载模板与旧的加权值，百分制，A：After-新的，B：Before-旧的 */
    UINT16 *pu16XrawLine = NULL;
    FLOAT64 f64Alpha = 0;
    const INT32 s32BlankProbWeak = -4, s32BlankProbStrong = -20;
    const UINT32 u32PixelPreXCardDs = PIXEL_PER_XSENSOR_CARD / XRAW_BINARY_DS_SCALE;
    const INT32 s32PseudoBlankProb = -30; // 伪空白条带作为空白条带的概率，用于后条带的加权
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    CONTOUR_ATTR *pstContAttr = NULL;
    XRAY_BGAC_PARAM *pstBgacParam = (XRAY_BGAC_PARAM *)pHandle;
    SLICE_PROFILE *pstSliceCur = NULL, *pstSlicePre = NULL;

    if (NULL == pHandle || NULL == pstXrawIn)
    {
        XSP_LOGE("the pHandle(%p) OR pstXrawIn(%p) is NULL\n", pHandle, pstXrawIn);
        return SAL_FAIL;
    }

    SAL_mutexTmLock(&pstBgacParam->mutexProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

    pstSliceCur = pstBgacParam->stBlankSeq.astProfile + pstBgacParam->stBlankSeq.u32SliceIdx;
    pstSlicePre = pstBgacParam->stBlankSeq.astProfile + BACKWARD(pstBgacParam->stBlankSeq.u32SliceIdx, XSP_PACK_HAS_SLICE_NUM_MAX);
    memset(pstSliceCur, 0, sizeof(SLICE_PROFILE));
    if (bPseudo) // enProcType == XRAY_TYPE_PSEUDO_BLANK
    {
        if (!pstSlicePre->bPseudo) // 如果前一个条带是伪空白条带，则不重复记录
        {
            pstSliceCur->bPseudo = SAL_TRUE;
            pstSliceCur->u32SliceNo = u32SliceNo;
            pstSliceCur->s32ProbEva = s32PseudoBlankProb;
            pstBgacParam->stBlankSeq.u32SliceIdx = FORWARD(pstBgacParam->stBlankSeq.u32SliceIdx, XSP_PACK_HAS_SLICE_NUM_MAX);
        }
        SAL_mutexTmUnlock(&pstBgacParam->mutexProc, __FUNCTION__, __LINE__);
        return SAL_FAIL;
    }

    if (pstBgacParam->stBinarySlice.u32Height != pstXrawIn->u32Height)
    {
        resize_binary_mem_param(pHandle, pstXrawIn);
    }

    xraw_hist_2bin_2norm(pstBgacParam->pu16FullCorrManual, pstXrawIn, pstBgacParam->u32BlankLeftCal, pstBgacParam->u32BlankRightCal,
                         au32Hist, &pstBgacParam->stBinarySlice, &pstBgacParam->stSliceNormalize, pstBgacParam->pu16FullCorrManualDetector,
                         pstBgacParam->u16FullCorrManualDetectorMax);
    pstSliceCur->u32SliceNo = u32SliceNo;
    pstSliceCur->s32ProbEva = xraw_nopack_prob_by_hist(au32Hist, pstBgacParam->u32Sensitivity);
    if (pstSliceCur->s32ProbEva == 0) // 非强空白条带，根据轮廓属性继续判定
    {
        /**
         * 下采样 
         * 内存上下各预留2行作为邻域，目前实际仅使用1行邻域，有效数据从第2行（0、1、2）开始计
         */
        loop1 = pstBgacParam->stBinarySlice.u32Height / XRAW_BINARY_DS_SCALE;
        loop2 = pstBgacParam->stBinarySlice.u32Width / XRAW_BINARY_DS_SCALE;
        for (m = 0; m < loop1; m++)
        {
            pL0 = (UINT8 *)pstBgacParam->stBinarySlice.pPlaneVir[0] + (pstBgacParam->stBinarySlice.u32Width * m * XRAW_BINARY_DS_SCALE);
            pL1 = pL0 + pstBgacParam->stBinarySlice.u32Stride[0];
            pDst = (UINT8 *)pstBgacParam->stBinaryDs.pPlaneVir[0] + pstBgacParam->stBinaryDs.u32Width * (XRAW_BINARY_PROC_NB + m);
            for (n = 0; n < loop2; n++)
            {
                k = n * XRAW_BINARY_DS_SCALE;
                pDst[n] = ((UINT32)pL0[k] + pL0[k+1] + pL1[k] + pL1[k+1]) / (XRAW_BINARY_DS_SCALE * XRAW_BINARY_DS_SCALE);
            }
        }

        // Ds邻域，复制最后一行到下邻域XRAW_BINARY_PROC_NB
        pL0 = (UINT8 *)pstBgacParam->stBinaryDs.pPlaneVir[0] + 
            pstBgacParam->stBinaryDs.u32Width * (pstBgacParam->stBinaryDs.u32Height - XRAW_BINARY_PROC_NB - 1);
        for (m = 1; m <= XRAW_BINARY_PROC_NB; m++)
        {
            memcpy(pL0 + pstBgacParam->stBinaryDs.u32Width * m, pL0, pstBgacParam->stBinaryDs.u32Width);
        }

        // Erode
        loop1 = pstBgacParam->stBinaryDs.u32Height - XRAW_BINARY_PROC_NB;
        loop2 = pstBgacParam->stBinaryDs.u32Width - 1;
        for (m = XRAW_BINARY_PROC_NB; m < loop1; m++)
        {
            pL1 = (UINT8 *)pstBgacParam->stBinaryDs.pPlaneVir[0] + pstBgacParam->stBinaryDs.u32Width * m; // 卷积核-1
            pL0 = pL1 - pstBgacParam->stBinaryDs.u32Width; // 卷积核-0
            pL2 = pL1 + pstBgacParam->stBinaryDs.u32Width; // 卷积核-2
            pDst = (UINT8 *)pstBgacParam->stBinaryErode.pPlaneVir[0] + pstBgacParam->stBinaryErode.u32Width * m;
            for (n = 1; n < loop2; n++) // 横向没有扩边，边缘像素不参与计算
            {
                if (pL0[n-1] > binTh || pL0[n] > binTh || pL0[n+1] > binTh ||
                    pL1[n-1] > binTh || pL1[n] > binTh || pL1[n+1] > binTh ||
                    pL2[n-1] > binTh || pL2[n] > binTh || pL2[n+1] > binTh)
                {
                    pDst[n] = 0xFF;
                }
                else
                {
                    pDst[n] = 0;
                }
            }
        }

        // Erode邻域，复制最后一行到下邻域XRAW_BINARY_PROC_NB
        pL0 = (UINT8 *)pstBgacParam->stBinaryErode.pPlaneVir[0] + 
            pstBgacParam->stBinaryErode.u32Width * (pstBgacParam->stBinaryErode.u32Height - XRAW_BINARY_PROC_NB - 1);
        for (m = 1; m <= XRAW_BINARY_PROC_NB; m++)
        {
            memcpy(pL0 + pstBgacParam->stBinaryErode.u32Width * m, pL0, pstBgacParam->stBinaryErode.u32Width);
        }

        // Dilate
        loop1 = pstBgacParam->stBinaryErode.u32Height - XRAW_BINARY_PROC_NB;
        loop2 = pstBgacParam->stBinaryErode.u32Width - 1;
        for (m = XRAW_BINARY_PROC_NB; m < loop1; m++)
        {
            pL1 = (UINT8 *)pstBgacParam->stBinaryErode.pPlaneVir[0] + pstBgacParam->stBinaryErode.u32Width * m; // 卷积核-1
            pL0 = pL1 - pstBgacParam->stBinaryErode.u32Width; // 卷积核-0
            pL2 = pL1 + pstBgacParam->stBinaryErode.u32Width; // 卷积核-2
            pDst = (UINT8 *)pstBgacParam->stBinaryDilate.pPlaneVir[0] + pstBgacParam->stBinaryDilate.u32Width * m + XRAW_BINARY_PROC_NB;
            for (n = 1; n < loop2; n++)
            {
                if (pL0[n-1] < binTh || pL0[n] < binTh || pL0[n+1] < binTh ||
                    pL1[n-1] < binTh || pL1[n] < binTh || pL1[n+1] < binTh ||
                    pL2[n-1] < binTh || pL2[n] < binTh || pL2[n+1] < binTh)
                {
                    pDst[n] = 0;
                }
                else
                {
                    pDst[n] = 0xFF;
                }
            }
        }

        // ximg_dump(&pstBgacParam->stSliceNormalize, pstBgacParam->u32HandleIdx, "/mnt/dump/s", "slice", NULL, u32SliceNo);
        ximg_dump(&pstBgacParam->stBinaryDs, pstBgacParam->u32HandleIdx, "/mnt/dump/b", "ds", NULL, u32SliceNo);
        ximg_dump(&pstBgacParam->stBinaryErode, pstBgacParam->u32HandleIdx, "/mnt/dump/e", "erode", NULL, u32SliceNo);
        ximg_dump(&pstBgacParam->stBinaryDilate, pstBgacParam->u32HandleIdx, "/mnt/dump/d", "dilate", NULL, u32SliceNo);
        pstBgacParam->u32ContourGrpNum = SAL_arraySize(pstBgacParam->aContourGrpRange);
        find_contours(&pstBgacParam->stSliceNormalize, &pstBgacParam->stBinaryDilate, XRAW_BINARY_PROC_NB, pstSliceCur, 
                      &pstBgacParam->u32ContourGrpNum, pstBgacParam->aContourGrpRange, pstBgacParam->paContourPoints, &pstBgacParam->stBorderCircum);
        //ximg_dump(&pstBgacParam->stBinaryDilate, pstBgacParam->u32HandleIdx, "/mnt/dump/c", "contour", NULL, u32SliceNo);

        if (pstSliceCur->u32ContourNum > 0) // 轮廓分别计算
        {
            #ifdef BGAC_PRTDBG
            printf("<CJDBG> chan %u, slice %u, Contour %u: ", pstBgacParam->u32HandleIdx, u32SliceNo, pstSliceCur->u32ContourNum);
            #endif
            for (m = 0; m < pstSliceCur->u32ContourNum; m++)
            {
                pstContAttr = pstSliceCur->astContAttr + m;
                s32ProbCoupl = pstContAttr->s32ProbCompos;
                if (0 == pstContAttr->s32ProbCompos) // 悬而未决的轮廓
                {
                    if (pstContAttr->stLineTop.u32XEnd > pstContAttr->stLineTop.u32XStart && 
                        pstContAttr->stLineBot.u32XEnd > pstContAttr->stLineBot.u32XStart &&
                        pstContAttr->s32AvgXLeft >= 0 && pstContAttr->s32AvgXRight >= 0)
                    {
                        s32ProbSpatial = 0; // 空间域上的独立概率
                        if (0 == pstContAttr->s32AvgXLeft % u32PixelPreXCardDs) // 与探测板边界强齐平
                        {
                            s32ProbSpatial += s32BlankProbStrong;
                        }
                        else if ((1 == pstContAttr->s32AvgXLeft % u32PixelPreXCardDs) || // 与探测板边界弱齐平
                                 (u32PixelPreXCardDs - 1 == pstContAttr->s32AvgXLeft % u32PixelPreXCardDs))
                        {
                            s32ProbSpatial += s32BlankProbWeak;
                        }
  
                        if (0 == pstContAttr->s32AvgXRight % u32PixelPreXCardDs) // 与探测板边界强齐平
                        {
                            s32ProbSpatial = ((s32ProbSpatial < 0) ? 5 : 4) * (s32ProbSpatial + s32BlankProbStrong) >> 2; // 双边齐平，概率×1.25
                        }
                        else if ((1 == pstContAttr->s32AvgXRight % u32PixelPreXCardDs) || // 与探测板边界弱齐平
                                 (u32PixelPreXCardDs - 1 == pstContAttr->s32AvgXRight % u32PixelPreXCardDs))
                        {
                            s32ProbSpatial = ((s32ProbSpatial < 0) ? 5 : 4) * (s32ProbSpatial + s32BlankProbWeak) >> 2; // 双边齐平，概率×1.25
                        }

                        s32ProbPre = 0; // 记录前一连续轮廓概率，用于连续属性的概率增强
                        u32JumpPackCnt = 0;
                        for (k = 1; k < XSP_PACK_HAS_SLICE_NUM_MAX; k++) // 最多遍历次数
                        {
                            if (!pstSlicePre->bPseudo)
                            {
                                for (n = 0; n < pstSlicePre->u32ContourNum; n++)
                                {
                                    if (pstSlicePre->astContAttr[n].stLineBot.u32XEnd > pstSlicePre->astContAttr[n].stLineBot.u32XStart && 
                                        pstSlicePre->astContAttr[n].stLineTop.u32XEnd > pstSlicePre->astContAttr[n].stLineTop.u32XStart)
                                    {
                                        // 对于包裹和非包裹轮廓的重叠，这里的判断是不严格的
                                        if (pstSlicePre->astContAttr[n].s32ProbCompos > 0)
                                        {
                                            if ((pstSlicePre->astContAttr[n].stLineBot.u32XStart <= pstContAttr->s32AvgXLeft || // 左包围
                                                 pstSlicePre->astContAttr[n].stLineBot.u32XStart - pstContAttr->s32AvgXLeft <= 3) && // 或不超过3个像素的对齐
                                                (pstSlicePre->astContAttr[n].stLineBot.u32XEnd >= pstContAttr->s32AvgXRight || // 右包围
                                                 pstContAttr->s32AvgXRight - pstSlicePre->astContAttr[n].stLineBot.u32XEnd <= 3))
                                            {
                                                u32JumpPackCnt++;
                                                if (1 == k) // 记录前一条带中的连续轮廓，参考其概率
                                                {
                                                    s32ProbPre = pstSlicePre->astContAttr[n].s32ProbCompos;
                                                }
                                                break;
                                            }
                                        }
                                        else
                                        {
                                            if (pstSlicePre->astContAttr[n].s32AvgXLeft >= 0 && pstSlicePre->astContAttr[n].s32AvgXRight >= 0)
                                            {
                                                // 最多支持3个像素的误差，左对齐或右对齐
                                                if (SAL_SUB_ABS(pstSlicePre->astContAttr[n].s32AvgXLeft, pstContAttr->s32AvgXLeft) <= 3 || 
                                                    SAL_SUB_ABS(pstSlicePre->astContAttr[n].s32AvgXRight, pstContAttr->s32AvgXRight) <= 3)
                                                {
                                                    pstContAttr->u32RelevantCnt += u32JumpPackCnt + 1;
                                                    u32JumpPackCnt = 0;
                                                    if (1 == k) // 记录前一条带中的连续轮廓，参考其概率
                                                    {
                                                        s32ProbPre = pstSlicePre->astContAttr[n].s32ProbCompos;
                                                    }
                                                    if (pstSlicePre->astContAttr[n].u32RelevantCnt > 0)
                                                    {
                                                        pstContAttr->u32RelevantCnt += pstSlicePre->astContAttr[n].u32RelevantCnt;
                                                        k = XSP_PACK_HAS_SLICE_NUM_MAX; // try break out directory
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }

                                if (n >= pstSlicePre->u32ContourNum) // 轮廓从此断开，跳出循环：for (k = 1; k < XSP_PACK_HAS_SLICE_NUM_MAX; k++)
                                {
                                    break;
                                }
                            }
                            else
                            {
                                pstContAttr->u32RelevantCnt++;
                            }

                            pstSlicePre = pstBgacParam->stBlankSeq.astProfile + BACKWARD(pstSlicePre - pstBgacParam->stBlankSeq.astProfile, XSP_PACK_HAS_SLICE_NUM_MAX);
                        }

                        pstContAttr->s32ProbCompos = -1 * (INT32)SAL_SUB_SAFE(pstContAttr->u32RelevantCnt * pstXrawIn->u32Height, 500) / 100; // 连续600列开始估计脏图概率
                        #ifdef BGAC_PRTDBG
                        printf("{%u, Bin %u %d %u, Relev %u - %u, Prob %d + %d -> ", pstContAttr->u32ContourNo, pstContAttr->u32Area, pstContAttr->s32ProbHist, 
                               pstContAttr->u32PackMeas, pstContAttr->u32RelevantCnt, u32JumpPackCnt, s32ProbSpatial, pstContAttr->s32ProbCompos);
                        #endif
                        pstContAttr->s32ProbCompos = SAL_MAX(pstContAttr->s32ProbCompos, s32BlankProbStrong) + s32ProbSpatial;
                        #ifdef BGAC_PRTDBG
                        printf("%d -> ", pstContAttr->s32ProbCompos);
                        #endif

                        pstSlicePre = pstBgacParam->stBlankSeq.astProfile + BACKWARD(pstBgacParam->stBlankSeq.u32SliceIdx, XSP_PACK_HAS_SLICE_NUM_MAX);
                        if (pstSlicePre->u32ContourNum == 0)
                        {
                            s32ProbCoupl = calc_weight_prob(pstContAttr->s32ProbCompos, pstSlicePre->s32ProbEva);
                        }
                        else
                        {
                            // 前后相同属性相的轮廓，根据关联延续性做概率值增强
                            if (s32ProbPre * pstContAttr->s32ProbCompos > 0)
                            {
                                f64Alpha = log10((double)(pstContAttr->u32RelevantCnt * pstXrawIn->u32Height) / 100);
                                s32ProbCoupl = (INT32)((1.0 + SAL_MAX(f64Alpha, 0.1)) * pstContAttr->s32ProbCompos);
                            }
                            else
                            {
                                s32ProbCoupl = calc_weight_prob(pstContAttr->s32ProbCompos, s32ProbPre);
                            }
                        }
                        #ifdef BGAC_PRTDBG
                        printf("%d} ", s32ProbCoupl);
                        #endif
                    }
                }
                else
                {
                    #ifdef BGAC_PRTDBG
                    printf("{%u, Bin %u %u %d, Prob %d} ", pstContAttr->u32ContourNo, pstContAttr->u32Area, pstContAttr->s32ProbHist, 
                           pstContAttr->u32PackMeas, pstContAttr->s32ProbCompos);
                    #endif
                }

                if (0 != s32ProbCoupl) // 统计该条带的概率
                {
                    if (pstSliceCur->s32ProbEva != 0)
                    {
                        // 同属性相基于绝对值增强，异属性相直接取大值
                        if (pstSliceCur->s32ProbEva * s32ProbCoupl < 0)
                        {
                            pstSliceCur->s32ProbEva = SAL_MAX(pstSliceCur->s32ProbEva, s32ProbCoupl);
                        }
                        else
                        {
                            if (s32ProbCoupl < 0)
                            {
                                pstSliceCur->s32ProbEva = SAL_MIN(pstSliceCur->s32ProbEva, s32ProbCoupl) + 
                                    pstSliceCur->s32ProbEva * s32ProbCoupl / (pstSliceCur->s32ProbEva + s32ProbCoupl);
                            }
                            else
                            {
                                pstSliceCur->s32ProbEva = SAL_MAX(pstSliceCur->s32ProbEva, s32ProbCoupl) + 
                                    pstSliceCur->s32ProbEva * s32ProbCoupl / (pstSliceCur->s32ProbEva + s32ProbCoupl);
                            }
                        }
                    }
                    else
                    {
                        pstSliceCur->s32ProbEva = s32ProbCoupl;
                    }
                }
                pstSliceCur->s32ProbEva = SAL_CLIP(pstSliceCur->s32ProbEva, -64, 99);
            } // 遍历各轮廓
        }
        else // 竟然没有轮廓！！
        {
            XSP_LOGW("chan %u, SliceNo %u, ProbIndep is 0, and ContourNum is 0 too...\n", pstBgacParam->u32HandleIdx, u32SliceNo);
            pstSliceCur->s32ProbEva = s32BlankProbWeak;
        }
        #ifdef BGAC_PRTDBG
        printf(", Prob Summation: %d\n", pstSliceCur->s32ProbEva);
        #endif
    }
    else
    {
        #ifdef BGAC_PRTDBG
        printf("<CJDBG> chan %u, slice %u, Prob Independent: %d\n", pstBgacParam->u32HandleIdx, u32SliceNo, pstSliceCur->s32ProbEva);
        #endif
    }

    pstBgacParam->stBlankSeq.u32SliceIdx = (pstBgacParam->stBlankSeq.u32SliceIdx + 1) % XSP_PACK_HAS_SLICE_NUM_MAX;

    // 邻域，复制最后XRAW_BINARY_PROC_NB行到上邻域，给下个条带使用
    pL0 = (UINT8 *)pstBgacParam->stBinaryDs.pPlaneVir[0] + 
        pstBgacParam->stBinaryDs.u32Width * (pstBgacParam->stBinaryDs.u32Height - 2 * XRAW_BINARY_PROC_NB);
    memcpy(pstBgacParam->stBinaryDs.pPlaneVir[0], pL0, pstBgacParam->stBinaryDs.u32Width * XRAW_BINARY_PROC_NB);
    pL0 = (UINT8 *)pstBgacParam->stBinaryErode.pPlaneVir[0] + 
        pstBgacParam->stBinaryErode.u32Width * (pstBgacParam->stBinaryErode.u32Height - 2 * XRAW_BINARY_PROC_NB);
    memcpy(pstBgacParam->stBinaryErode.pPlaneVir[0], pL0, pstBgacParam->stBinaryErode.u32Width * XRAW_BINARY_PROC_NB);

    if (pstSliceCur->s32ProbEva < 0)
    {
        memset(pstBgacParam->pu32PixelSum, 0, pstXrawIn->u32Width * pstCapbXray->xray_energy_num * sizeof(UINT32));
        for (i = 0; i < pstXrawIn->u32Height; i++)
        {
            pu16XrawLine = (UINT16 *)pstXrawIn->pPlaneVir[0] + pstXrawIn->u32Stride[0] * i;
            for (j = 0, k = 0; j < pstXrawIn->u32Width; j++, k++)
            {
                pstBgacParam->pu32PixelSum[k] += pu16XrawLine[j];
            }
        }
        if (XRAY_ENERGY_DUAL == pstCapbXray->xray_energy_num)
        {
            for (i = 0; i < pstXrawIn->u32Height; i++)
            {
                pu16XrawLine = (UINT16 *)pstXrawIn->pPlaneVir[1] + pstXrawIn->u32Stride[1] * i;
                for (j = 0, k = pstXrawIn->u32Width; j < pstXrawIn->u32Width; j++, k++)
                {
                    pstBgacParam->pu32PixelSum[k] += pu16XrawLine[j];
                }
            }
        }

        s32WeightA = -pstSliceCur->s32ProbEva * SAL_MIN(pstBgacParam->u32Sensitivity, 100) / 50;
        s32WeightA = SAL_CLIP(s32WeightA, 0, 100);
        s32WeightB = 100 - s32WeightA;
        k = pstXrawIn->u32Width * pstCapbXray->xray_energy_num;
        for (j = 0; j < k; j++)
        {
            pstBgacParam->pu16FullCorrAuto[j] = (pstBgacParam->pu32PixelSum[j] / pstXrawIn->u32Height * s32WeightA + 
                                                 pstBgacParam->pu16FullCorrAuto[j] * s32WeightB) / 100;
        }
        memcpy(pu16FullCorr, pstBgacParam->pu16FullCorrAuto, pstXrawIn->u32Width * pstCapbXray->xray_energy_num * sizeof(UINT16));
    }

    SAL_mutexTmUnlock(&pstBgacParam->mutexProc, __FUNCTION__, __LINE__);

    return ((pstSliceCur->s32ProbEva < 0) && (pstBgacParam->u32Sensitivity > 0)) ? SAL_SOK : SAL_FAIL;
}


/**
 * @fn      xray_bgac_update_maunal_fullcorr
 * @brief   更新手动满载校正模板
 * 
 * @param   [IN] pHandle 背景自动校正算法句柄，xray_bgac_init()返回
 * @param   [IN] pstXrawIn 手动满载校正数据
 */
VOID xray_bgac_update_maunal_fullcorr(void *pHandle, XIMAGE_DATA_ST *pstXrawIn)
{
    UINT32 i = 0, j = 0, k = 0;
    UINT32 u32XCardNum = 0;
    XRAY_BGAC_PARAM *pstBgacParam = (XRAY_BGAC_PARAM *)pHandle;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    UINT16 *pu16XrawLine = NULL;
    UINT32 u32LeDetectorSum = 0, u32DetectorNum = 0;

    if (NULL == pHandle || NULL == pstXrawIn)
    {
        XSP_LOGE("the pHandle(%p) OR pstXrawIn(%p) is NULL\n", pHandle, pstXrawIn);
        return;
    }

    SAL_mutexTmLock(&pstBgacParam->mutexProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

    // 重置所有条带属性，重新开始
    memset(&pstBgacParam->stBlankSeq, 0, sizeof(SLICE_SEQ));
    memset(pstBgacParam->pu16FullCorrManualDetector, 0, sizeof(UINT16) * pstCapbXray->xraw_width_max / PIXEL_PER_XSENSOR_CARD);
    pstBgacParam->u16FullCorrManualDetectorMax = 0;
    ximg_fill_color(&pstBgacParam->stBinaryDs, 0, pstBgacParam->stBinaryDs.u32Height, 0, pstBgacParam->stBinaryDs.u32Width, 0xFF);
    ximg_fill_color(&pstBgacParam->stBinaryErode, 0, pstBgacParam->stBinaryErode.u32Height, 0, pstBgacParam->stBinaryErode.u32Width, 0xFF);
    ximg_fill_color(&pstBgacParam->stBinaryDilate, 0, pstBgacParam->stBinaryDilate.u32Height, 0, pstBgacParam->stBinaryDilate.u32Width, 0xFF);

    // 计算满载模板
    memset(pstBgacParam->pu32PixelSum, 0, pstXrawIn->u32Width * pstCapbXray->xray_energy_num * sizeof(UINT32));
    u32XCardNum = pstXrawIn->u32Width / PIXEL_PER_XSENSOR_CARD;
    for (i = 0; i < pstXrawIn->u32Height; i++) // 低能
    {
        pu16XrawLine = (UINT16 *)pstXrawIn->pPlaneVir[0] + pstXrawIn->u32Stride[0] * i;
        for (j = 0, k = 0; j < pstXrawIn->u32Width; j++, k++)
        {
            pstBgacParam->pu32PixelSum[k] += pu16XrawLine[j];
        }
    }
    if (XRAY_ENERGY_DUAL == pstCapbXray->xray_energy_num) // 高能
    {
        for (i = 0; i < pstXrawIn->u32Height; i++)
        {
            pu16XrawLine = (UINT16 *)pstXrawIn->pPlaneVir[1] + pstXrawIn->u32Stride[1] * i;
            for (j = 0, k = pstXrawIn->u32Width; j < pstXrawIn->u32Width; j++, k++)
            {
                pstBgacParam->pu32PixelSum[k] += pu16XrawLine[j];
            }
        }
    }

    u32DetectorNum = pstXrawIn->u32Width / PIXEL_PER_XSENSOR_CARD;

    // 计算探测器低能均值
    for (i = 0, k = 0; i < u32DetectorNum; i++)
    {
        for (j = 0; j < PIXEL_PER_XSENSOR_CARD; j++)
        {
            u32LeDetectorSum += pstBgacParam->pu32PixelSum[k];
            k++;
        }
        pstBgacParam->pu16FullCorrManualDetector[i] = u32LeDetectorSum / (PIXEL_PER_XSENSOR_CARD * pstXrawIn->u32Height) ;
        if (pstBgacParam->pu16FullCorrManualDetector[i] < DOSAGE_STAT_RANGE_MAX && pstBgacParam->pu16FullCorrManualDetector[i] > DOSAGE_STAT_RANGE_MIN)
        {
            pstBgacParam->u16FullCorrManualDetectorMax = SAL_MAX(pstBgacParam->u16FullCorrManualDetectorMax, pstBgacParam->pu16FullCorrManualDetector[i]);
        }
        pstBgacParam->u16FullCorrManualDetectorMax = SAL_MAX(pstBgacParam->u16FullCorrManualDetectorMax, pstBgacParam->pu16FullCorrManualDetector[i]);
        u32LeDetectorSum = 0;
    }

    k = pstXrawIn->u32Width * pstCapbXray->xray_energy_num;
    for (j = 0; j < k; j++)
    {
        pstBgacParam->pu16FullCorrManual[j] = pstBgacParam->pu32PixelSum[j] / pstXrawIn->u32Height;
        pstBgacParam->pu16FullCorrAuto[j] = pstBgacParam->pu16FullCorrManual[j];
    }
    #if 0
    CHAR fn[128] = {0};
    snprintf(fn, sizeof(fn), "/mnt/dump/fullcorr_w%u_s%u_t%llu.raw", pstXrawIn->u32Width, pstXrawIn->u32Stride[0], sal_get_tickcnt());
    SAL_WriteToFile(fn, 0, SEEK_SET, pstBgacParam->stFullCorr.pPlaneVir[0], pstXrawIn->u32Width * sizeof(UINT16));
    #endif

    // 统计每块探测板的满载剂量，在合理剂量范围内，则认为正常
    for (i = 0; i < u32XCardNum; i++)
    {
        pstBgacParam->pu32PixelSum[i] = 0;
        pu16XrawLine = pstBgacParam->pu16FullCorrManual + PIXEL_PER_XSENSOR_CARD * i;
        for (j = 0; j < PIXEL_PER_XSENSOR_CARD; j++)
        {
            pstBgacParam->pu32PixelSum[i] += pu16XrawLine[j];
        }
        pstBgacParam->pu32PixelSum[i] /= PIXEL_PER_XSENSOR_CARD;
        if (pstBgacParam->pu32PixelSum[i] > DOSAGE_STAT_RANGE_MIN && pstBgacParam->pu32PixelSum[i] < DOSAGE_STAT_RANGE_MAX)
        {
            pstBgacParam->pBXsensorCardValid[i] = SAL_TRUE;
        }
        else // 该块探测板异常
        {
            pstBgacParam->pBXsensorCardValid[i] = SAL_FALSE;
        }
    }
    j = u32XCardNum / 2; // 分别计算前一半和后一半
    for (i = 1; i <= j; i++)
    {
        if (!pstBgacParam->pBXsensorCardValid[i-1] && pstBgacParam->pBXsensorCardValid[i]) // 前一块无效，后一块有效
        {
            pstBgacParam->u32BlankLeftCal = SAL_MAX(pstBgacParam->u32BlankLeftCal, i * PIXEL_PER_XSENSOR_CARD);
            XSP_LOGW("chan %u, xsensor card %u -> %u turn to good, BlankLeft %u\n", pstBgacParam->u32HandleIdx, i-1, i, pstBgacParam->u32BlankLeftCal);
        }
    }
    for (i = u32XCardNum-1; i >= j; i--)
    {
        if (pstBgacParam->pBXsensorCardValid[i-1] && !pstBgacParam->pBXsensorCardValid[i]) // 前一块有效，后一块无效
        {
            pstBgacParam->u32BlankRightCal = SAL_MAX(pstBgacParam->u32BlankRightCal, (u32XCardNum-i) * PIXEL_PER_XSENSOR_CARD);
            XSP_LOGW("chan %u, xsensor card %u -> %u turn to bad, BlankRight %u\n", pstBgacParam->u32HandleIdx, i-1, i, pstBgacParam->u32BlankRightCal);
        }
    }

    SAL_mutexTmUnlock(&pstBgacParam->mutexProc, __FUNCTION__, __LINE__);

    return;
}


/**
 * @fn      xray_bgac_update_lrblank
 * @brief   更新XRAW数据左右置白区域，置白区域不做统计
 * 
 * @param   [IN] pHandle 背景自动校正算法句柄，xray_bgac_init()返回
 * @param   [IN] u32BlankLeft 左置白区域，基于XRAW数据
 * @param   [IN] u32BlankRight 右置白区域，基于XRAW数据
 */
VOID xray_bgac_update_lrblank(void *pHandle, UINT32 u32BlankLeft, UINT32 u32BlankRight)
{
    INT32 i = 0;
    UINT32 u32XCardNum = 0;
    XRAY_BGAC_PARAM *pstBgacParam = (XRAY_BGAC_PARAM *)pHandle;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    UINT32 u32BlankLeftCal = 0, u32BlankRightCal = 0, u32BlankLeftSet = 0, u32BlankRightSet = 0;

    if (NULL == pHandle)
    {
        XSP_LOGE("the pHandle is NULL\n");
        return;
    }

    SAL_mutexTmLock(&pstBgacParam->mutexProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

    u32BlankLeftSet = u32BlankLeft;
    u32BlankRightSet = u32BlankRight;

    u32XCardNum = pstCapbXray->xraw_width_max / PIXEL_PER_XSENSOR_CARD / 2; // 分别计算前一半和后一半
    for (i = 1; i <= u32XCardNum; i++)
    {
        if (!pstBgacParam->pBXsensorCardValid[i-1] && pstBgacParam->pBXsensorCardValid[i]) // 前一块无效，后一块有效
        {
            u32BlankLeftCal = SAL_MAX(u32BlankLeftCal, i * PIXEL_PER_XSENSOR_CARD);
            XSP_LOGW("chan %u, xsensor card %d -> %d turn to good, BlankLeft %u\n", pstBgacParam->u32HandleIdx, i-1, i, u32BlankLeftCal);
            break;
        }
    }
    for (i = u32XCardNum-1; i >= u32XCardNum; i--)
    {
        if (pstBgacParam->pBXsensorCardValid[i-1] && !pstBgacParam->pBXsensorCardValid[i]) // 前一块有效，后一块无效
        {
            u32BlankRightCal = SAL_MAX(u32BlankRightCal, (u32XCardNum-i) * PIXEL_PER_XSENSOR_CARD);
            XSP_LOGW("chan %u, xsensor card %d -> %d turn to bad, BlankRight %u\n", pstBgacParam->u32HandleIdx, i-1, i, u32BlankRightCal);
            break;
        }
    }

    // 重置空白区域
    if (u32BlankLeftCal != u32BlankLeftSet || u32BlankRightCal != u32BlankRightSet)
    {
        pstBgacParam->u32BlankLeftCal = SAL_MAX(u32BlankLeftSet, u32BlankLeftCal);
        pstBgacParam->u32BlankRightCal = SAL_MAX(u32BlankRightSet, u32BlankRightCal);
        ximg_fill_color(&pstBgacParam->stBinaryDs, 0, pstBgacParam->stBinaryDs.u32Height, 0, pstBgacParam->stBinaryDs.u32Width, 0xFF);
        ximg_fill_color(&pstBgacParam->stBinaryErode, 0, pstBgacParam->stBinaryErode.u32Height, 0, pstBgacParam->stBinaryErode.u32Width, 0xFF);
        ximg_fill_color(&pstBgacParam->stBinaryDilate, 0, pstBgacParam->stBinaryDilate.u32Height, 0, pstBgacParam->stBinaryDilate.u32Width, 0xFF);
    }

    SAL_mutexTmUnlock(&pstBgacParam->mutexProc, __FUNCTION__, __LINE__);

    return;
}


/**
 * @fn      xray_bgac_set_sensitivity
 * @brief   设置背景自动校正灵敏度，算法内部默认值为50
 * 
 * @param   [IN] pHandle 背景自动校正算法句柄，xray_bgac_init()返回
 * @param   [IN] u32Sensitivity 灵敏度，范围[0, 100]
 */
VOID xray_bgac_set_sensitivity(void *pHandle, UINT32 u32Sensitivity)
{
    XRAY_BGAC_PARAM *pstBgacParam = (XRAY_BGAC_PARAM *)pHandle;

    if (NULL == pHandle)
    {
        XSP_LOGE("the pHandle is NULL\n");
        return;
    }

    SAL_mutexTmLock(&pstBgacParam->mutexProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    pstBgacParam->u32Sensitivity = SAL_MIN(u32Sensitivity, 100);
    SAL_mutexTmUnlock(&pstBgacParam->mutexProc, __FUNCTION__, __LINE__);

    return;
}


/**
 * @fn      xray_bgac_get_slice_prob
 * @brief   获取指定条带是空白或包裹的概率，大于0倾向于包裹，小于0倾向于空白
 * 
 * @param   [IN] pHandle 背景自动校正算法句柄，xray_bgac_init()返回
 * @param   [IN] u32SliceNo 条带号
 * 
 * @return  INT32 是空白或包裹的概率，范围[-100, 100]，大于0倾向于包裹，小于0倾向于空白
 */
INT32 xray_bgac_get_slice_prob(void *pHandle, UINT32 u32SliceNo)
{
    INT32 i = 0;
    SLICE_PROFILE *pstProfile = NULL;
    XRAY_BGAC_PARAM *pstBgacParam = (XRAY_BGAC_PARAM *)pHandle;

    for (i = pstBgacParam->stBlankSeq.u32SliceIdx + XSP_PACK_HAS_SLICE_NUM_MAX; i > pstBgacParam->stBlankSeq.u32SliceIdx; i--)
    {
        pstProfile = pstBgacParam->stBlankSeq.astProfile + i % XSP_PACK_HAS_SLICE_NUM_MAX;
        if (pstProfile->u32SliceNo == u32SliceNo)
        {
            return pstProfile->s32ProbEva;
        }
    }

    return 0;
}


/**
 * @fn      xray_bgac_init
 * @brief   背景自动校正算法初始化
 * 
 * @return  VOID* 背景自动校正算法句柄，NULL-初始化失败
 */
VOID *xray_bgac_init(VOID)
{
    UINT32 u32BufSize = 0;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XRAY_BGAC_PARAM *pstBgacParam = NULL;

    pstBgacParam = (XRAY_BGAC_PARAM *)xray_mod_malloc(sizeof(XRAY_BGAC_PARAM));
    if (NULL == pstBgacParam)
    {
        XSP_LOGE("xray_mod_malloc for 'pstBgacParam' failed, buffer size: %zu\n", sizeof(XRAY_BGAC_PARAM));
        return NULL;
    }

    memset(pstBgacParam, 0, sizeof(XRAY_BGAC_PARAM));
    SAL_mutexTmInit(&pstBgacParam->mutexProc, SAL_MUTEX_ERRORCHECK);
    pstBgacParam->u32HandleIdx = gu32BgacHandleCnt++;
    pstBgacParam->u32Sensitivity = 50;

    // 初始化满载模板的Buffer，像素数：xraw_width_max * 2，单像素占2个字节
    u32BufSize = pstCapbXray->xraw_width_max * pstCapbXray->xray_energy_num * sizeof(UINT16);
    pstBgacParam->pu16FullCorrManual = (UINT16 *)xray_mod_malloc(u32BufSize);
    if (NULL == pstBgacParam->pu16FullCorrManual)
    {
        XSP_LOGE("xray_mod_malloc for 'pu16FullCorrManual' failed, buffer size: %u\n", u32BufSize);
        return NULL;
    }

    // 初始化探测器满载低能Buffer
    u32BufSize = pstCapbXray->xraw_width_max / PIXEL_PER_XSENSOR_CARD * sizeof(UINT16);
    pstBgacParam->pu16FullCorrManualDetector = (UINT16 *)xray_mod_malloc(u32BufSize);
    if (NULL == pstBgacParam->pu16FullCorrManualDetector)
    {
        XSP_LOGE("xray_mod_malloc for 'pu16FullCorrManualDetector' failed, buffer size: %u\n", u32BufSize);
        return NULL;
    }

    u32BufSize = pstCapbXray->xraw_width_max * pstCapbXray->xray_energy_num * sizeof(UINT16);
    pstBgacParam->pu16FullCorrAuto = (UINT16 *)xray_mod_malloc(u32BufSize);
    if (NULL == pstBgacParam->pu16FullCorrAuto)
    {
        XSP_LOGE("xray_mod_malloc for 'pu16FullCorrAuto' failed, buffer size: %u\n", u32BufSize);
        return NULL;
    }

    // 初始化计算像素和的Buffer，像素数：xraw_width_max * 2，单像素占4个字节
    u32BufSize = pstCapbXray->xraw_width_max * pstCapbXray->xray_energy_num * sizeof(UINT32);
    pstBgacParam->pu32PixelSum = (UINT32 *)xray_mod_malloc(u32BufSize);
    if (NULL == pstBgacParam->pu32PixelSum)
    {
        XSP_LOGE("xray_mod_malloc for 'pu32PixelSum' failed, buffer size: %u\n", u32BufSize);
        return NULL;
    }

    // 初始化每块探测板是否正常的数组
    u32BufSize = sizeof(BOOL) * pstCapbXray->xraw_width_max / PIXEL_PER_XSENSOR_CARD;
    pstBgacParam->pBXsensorCardValid = xray_mod_malloc(u32BufSize);
    if (NULL == pstBgacParam->pBXsensorCardValid)
    {
        XSP_LOGE("xray_mod_malloc for 'pBXsensorCardValid' failed, buffer size: %u\n", u32BufSize);
        return NULL;
    }

    // 初始化条带归一化图
    pstBgacParam->stSliceNormalize.stMbAttr.bCached = SAL_TRUE;
    if (SAL_SOK != ximg_create(pstCapbXray->xraw_width_max, pstCapbXray->line_per_slice_max, 
                               DSP_IMG_DATFMT_SP16, "xray_normalize", NULL, &pstBgacParam->stSliceNormalize))
    {
        XSP_LOGE("ximg_create for 'xray_normalize' failed, w %u, h %u\n", pstCapbXray->xraw_width_max, pstCapbXray->line_per_slice_max);
        return NULL;
    }

    // 初始化条带二值图
    pstBgacParam->stBinarySlice.stMbAttr.bCached = SAL_TRUE;
    if (SAL_SOK != ximg_create(pstCapbXray->xraw_width_max, pstCapbXray->line_per_slice_max, 
                               DSP_IMG_DATFMT_SP8, "xray_binary", NULL, &pstBgacParam->stBinarySlice))
    {
        XSP_LOGE("ximg_create for 'xray_binary' failed, w %u, h %u\n", pstCapbXray->xraw_width_max, pstCapbXray->line_per_slice_max);
        return NULL;
    }

    // 初始化下采样条带二值图
    pstBgacParam->stBinaryDs.stMbAttr.bCached = SAL_TRUE;
    if (SAL_SOK != ximg_create(pstBgacParam->stBinarySlice.u32Width / XRAW_BINARY_DS_SCALE, 
                               pstBgacParam->stBinarySlice.u32Height / XRAW_BINARY_DS_SCALE + 2 * XRAW_BINARY_PROC_NB, // 上下各扩2个像素作为邻域
                               DSP_IMG_DATFMT_SP8, "xray_binary_ds", NULL, &pstBgacParam->stBinaryDs))
    {
        XSP_LOGE("ximg_create for 'xray_binary' failed, w %u, h %u\n", pstBgacParam->stBinarySlice.u32Width / 2, pstBgacParam->stBinarySlice.u32Height / 2);
        return NULL;
    }
    ximg_fill_color(&pstBgacParam->stBinaryDs, 0, pstBgacParam->stBinaryDs.u32Height, 0, pstBgacParam->stBinaryDs.u32Width, 0xFF);

    // 初始化下腐蚀条带二值图
    pstBgacParam->stBinaryErode.stMbAttr.bCached = SAL_TRUE;
    if (SAL_SOK != ximg_create(pstBgacParam->stBinaryDs.u32Width, pstBgacParam->stBinaryDs.u32Height, // 与DS一致，上下各扩2个像素作为邻域
                               DSP_IMG_DATFMT_SP8, "xray_binary_erode", NULL, &pstBgacParam->stBinaryErode))
    {
        XSP_LOGE("ximg_create for 'xray_binary' failed, w %u, h %u\n", pstBgacParam->stBinaryDs.u32Width, pstBgacParam->stBinaryDs.u32Height);
        return NULL;
    }
    ximg_fill_color(&pstBgacParam->stBinaryErode, 0, pstBgacParam->stBinaryErode.u32Height, 0, pstBgacParam->stBinaryErode.u32Width, 0xFF);

    // 初始化下膨胀条带二值图
    pstBgacParam->stBinaryDilate.stMbAttr.bCached = SAL_TRUE;
    if (SAL_SOK != ximg_create(pstBgacParam->stBinaryDs.u32Width + 2 * XRAW_BINARY_PROC_NB, pstBgacParam->stBinaryDs.u32Height, // 上下左右各扩2个像素作为邻域
                               DSP_IMG_DATFMT_SP8, "xray_binary_dilate", NULL, &pstBgacParam->stBinaryDilate))
    {
        XSP_LOGE("ximg_create for 'xray_binary' failed, w %u, h %u\n", pstBgacParam->stBinaryDs.u32Width, pstBgacParam->stBinarySlice.u32Height >> 2);
        return NULL;
    }
    ximg_fill_color(&pstBgacParam->stBinaryDilate, 0, pstBgacParam->stBinaryDilate.u32Height, 0, pstBgacParam->stBinaryDilate.u32Width, 0xFF);

    // 初始化存储轮廓中每个点坐标的内存
    u32BufSize = sizeof(POS_ATTR) * pstBgacParam->stBinaryDilate.u32Width * pstBgacParam->stBinaryDilate.u32Height;
    pstBgacParam->paContourPoints = xray_mod_malloc(u32BufSize);
    if (NULL == pstBgacParam->paContourPoints)
    {
        XSP_LOGE("xray_mod_malloc for 'paContourPoints' failed, buffer size: %u\n", u32BufSize);
        return NULL;
    }

    // 初始化轮廓包围上下边界线
    u32BufSize = sizeof(UINT32) * pstBgacParam->stBinaryDilate.u32Width;
    pstBgacParam->stBorderCircum.stTop.pu32VerYCoor = xray_mod_malloc(u32BufSize);
    pstBgacParam->stBorderCircum.stBottom.pu32VerYCoor = xray_mod_malloc(u32BufSize);
    if (NULL == pstBgacParam->stBorderCircum.stTop.pu32VerYCoor || NULL == pstBgacParam->stBorderCircum.stBottom.pu32VerYCoor)
    {
        XSP_LOGE("xray_mod_malloc for 'pu32VerYCoor' failed, buffer size: %u\n", u32BufSize);
        return NULL;
    }

    // 初始化轮廓包围左右边界线
    u32BufSize = sizeof(UINT32) * pstBgacParam->stBinaryDilate.u32Height;
    pstBgacParam->stBorderCircum.stLeft.pu32HorXCoor = xray_mod_malloc(u32BufSize);
    pstBgacParam->stBorderCircum.stRight.pu32HorXCoor = xray_mod_malloc(u32BufSize);
    if (NULL == pstBgacParam->stBorderCircum.stLeft.pu32HorXCoor || NULL == pstBgacParam->stBorderCircum.stRight.pu32HorXCoor)
    {
        XSP_LOGE("xray_mod_malloc for 'pu32HorXCoor' failed, buffer size: %u\n", u32BufSize);
        return NULL;
    }

    return (VOID *)pstBgacParam;
}