/******************************************************************
 * @file   yuv2gif.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  yuv转换成RGB; 基于giflib开源库，完成rgb转换成gif。rgb转gif会将高分辨率的
           图像转换为低分辨率，并通过lzw压缩， yuv转rgb无压缩。
 * @author
 * @date   2020/10/29
 * @note \n History:
   1.Date        : 2020/10/29
     Author      :
     Modification: 创建文档
   2.模块化编译，去掉与vdec_task的耦合；yeyanzhong, 2021/8/10

 *******************************************************************/


#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#include "gif_lib.h"

#include "sal.h"
#include "platform_hal.h"
#include "yuv2gif.h"



#define PROGRAM_NAME "rgb2gif"
#define GIF_MESSAGE(Msg)	fprintf(stderr, "\n%s: %s\n", PROGRAM_NAME, Msg)
#define GIF_EXIT(Msg)		{ GIF_MESSAGE(Msg); exit(-3); }
#define SAL_SOK		(0)
#define SAL_FAIL	(-1)
#define YUV64VirAddr
#define	RGB64VirAddr

static int
    ExpNumOfColors = 8,
    ColorMapSize = 256;

unsigned int gifTest_getCurns(void);

#if 0
int SAL_SetThreadCoreBind(unsigned int uiCoreNum);
int yuv_to_gif(char *yuvbuf, int w, int h, char *outfile);
int yuv420sp_to_rgbplaner(char *FileName,
                          GifByteType **RedBuffer,
                          GifByteType **GreenBuffer,
                          GifByteType **BlueBuffer,
                          int Width, int Height);

static void SaveGif(GifByteType *OutputBuffer,
                    ColorMapObject *OutputColorMap,
                    int ExpColorMapSize, int Width, int Height, char *outfile);
static void QuitGifError(GifFileType *GifFile);
#endif

int Gif_DrvBufWrite(GifFileType *GifFile, const GifByteType *pByteTypeBuf, int uiLen);
int Gif_DrvSave(GIF_ATTR_S *pstGifAttr, RGB_ATTR_S *pstRgbAttr, ColorMapObject *OutputColorMap, int ExpColorMapSize);
int Gif_DrvYuv2Rgb24(YUV_ATTR_S *pstYuvStr,RGB_ATTR_S *pstRgbAttr);

#define COLOR_ARRAY_SIZE	32768
#define BITS_PER_PRIM_COLOR 5
#define MAX_PRIM_COLOR		0x1f

static int SortRGBAxis;

typedef struct QuantizedColorType
{
    GifByteType RGB[3];
    GifByteType NewColorIndex;
    long Count;
    struct QuantizedColorType *Pnext;
} QuantizedColorType;

typedef struct NewColorMapType
{
    GifByteType RGBMin[3], RGBWidth[3];
    unsigned int NumEntries; /* # of QuantizedColorType in linked list below */
    unsigned long Count; /* Total number of pixels in all the entries */
    QuantizedColorType *QuantizedColors;
} NewColorMapType;

static int SubdivColorMap(NewColorMapType *NewColorSubdiv,
                          unsigned int ColorMapSize,
                          unsigned int *NewColorMapSize);
static int SortCmpRtn(const void *Entry1, const void *Entry2);

/******************************************************************
   Function:   gifTest_getCurns
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
unsigned int gifTest_getCurns(void)
{
    unsigned int uiResult = 0;

    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    clock_gettime(1, &ts);
    uiResult = ((ts.tv_sec * 1000) + ts.tv_nsec / 1000000);
    return uiResult;
}

/******************************************************************************
* Quantize high resolution image into lower one. Input image consists of a
* 2D array for each of the RGB colors with size Width by Height. There is no
* Color map for the input. Output is a quantized image with 2D array of
* indexes into the output color map.
*   Note input image can be 24 bits at the most (8 for red/green/blue) and
* the output has 256 colors at the most (256 entries in the color map.).
* ColorMapSize specifies size of color map up to 256 and will be updated to
* real size before returning.
*   Also non of the parameter are allocated by this routine.
*   This function returns GIF_OK if succesfull, GIF_ERROR otherwise.
******************************************************************************/
static int
QuantizeBuffer(unsigned int Width,
               unsigned int Height,
               int *ColorMapSize,
               GifByteType *RedInput,
               GifByteType *GreenInput,
               GifByteType *BlueInput,
               GifByteType *OutputBuffer,
               GifColorType *OutputColorMap) {

    /* 高分辨率图像量化为低分辨率图像,输入图像由每个RGB颜色的二维数组, 输出是一个二维数组的量化图像 */
    unsigned int Index, NumOfEntries;
    int i, j, MaxRGBError[3];
    unsigned int NewColorMapSize;
    long Red, Green, Blue;
    NewColorMapType NewColorSubdiv[256];
    QuantizedColorType *ColorArrayEntries, *QuantizedColor;

    ColorArrayEntries = (QuantizedColorType *)malloc(
        sizeof(QuantizedColorType) * COLOR_ARRAY_SIZE);
    if (ColorArrayEntries == NULL)
    {
        return GIF_ERROR;
    }

    for (i = 0; i < COLOR_ARRAY_SIZE; i++)
    {
        ColorArrayEntries[i].RGB[0] = i >> (2 * BITS_PER_PRIM_COLOR);
        ColorArrayEntries[i].RGB[1] = (i >> BITS_PER_PRIM_COLOR)
                                      & MAX_PRIM_COLOR;
        ColorArrayEntries[i].RGB[2] = i & MAX_PRIM_COLOR;
        ColorArrayEntries[i].Count = 0;
    }

    /* 采样颜色及其分布: */
    for (i = 0; i < (int)(Width * Height); i++)
    {
        Index = ((RedInput[i] >> (8 - BITS_PER_PRIM_COLOR))
                 << (2 * BITS_PER_PRIM_COLOR))
                + ((GreenInput[i] >> (8 - BITS_PER_PRIM_COLOR))
                   << BITS_PER_PRIM_COLOR)
                + (BlueInput[i] >> (8 - BITS_PER_PRIM_COLOR));
        ColorArrayEntries[Index].Count++;
    }

    /* Put all the colors in the first entry of the color map, and call the
     * recursive subdivision process.  */
    for (i = 0; i < 256; i++)
    {
        NewColorSubdiv[i].QuantizedColors = NULL;
        NewColorSubdiv[i].Count = NewColorSubdiv[i].NumEntries = 0;
        for (j = 0; j < 3; j++)
        {
            NewColorSubdiv[i].RGBMin[j] = 0;
            NewColorSubdiv[i].RGBWidth[j] = 255;
        }
    }

    /* Find the non empty entries in the color table and chain them: */
    for (i = 0; i < COLOR_ARRAY_SIZE; i++)
    {
        if (ColorArrayEntries[i].Count > 0)
            break;
    }

    QuantizedColor = NewColorSubdiv[0].QuantizedColors = &ColorArrayEntries[i];
    NumOfEntries = 1;
    while (++i < COLOR_ARRAY_SIZE)
    {
        if (ColorArrayEntries[i].Count > 0)
        {
            QuantizedColor->Pnext = &ColorArrayEntries[i];
            QuantizedColor = &ColorArrayEntries[i];
            NumOfEntries++;
        }
    }

    QuantizedColor->Pnext = NULL;

    NewColorSubdiv[0].NumEntries = NumOfEntries; /* Different sampled colors */
    NewColorSubdiv[0].Count = ((long)Width) * Height; /* Pixels */
    NewColorMapSize = 1;
    if (SubdivColorMap(NewColorSubdiv, *ColorMapSize, &NewColorMapSize)
        != GIF_OK)
    {
        free((char *)ColorArrayEntries);
        return GIF_ERROR;
    }

    if (NewColorMapSize < *ColorMapSize)
    {
        /* And clear rest of color map: */
        for (i = NewColorMapSize; i < *ColorMapSize; i++)
            OutputColorMap[i].Red = OutputColorMap[i].Green
                                        = OutputColorMap[i].Blue = 0;
    }

    /* 将每个条目中的颜色平均为要在输出颜色映射中使用的颜色，并将其插入到输出颜色映射本身中  */
    for (i = 0; i < NewColorMapSize; i++)
    {
        if ((j = NewColorSubdiv[i].NumEntries) > 0)
        {
            QuantizedColor = NewColorSubdiv[i].QuantizedColors;
            Red = Green = Blue = 0;
            while (QuantizedColor)
            {
                QuantizedColor->NewColorIndex = i;
                Red += QuantizedColor->RGB[0];
                Green += QuantizedColor->RGB[1];
                Blue += QuantizedColor->RGB[2];
                QuantizedColor = QuantizedColor->Pnext;
            }

            OutputColorMap[i].Red = (Red << (8 - BITS_PER_PRIM_COLOR)) / j;
            OutputColorMap[i].Green = (Green << (8 - BITS_PER_PRIM_COLOR)) / j;
            OutputColorMap[i].Blue = (Blue << (8 - BITS_PER_PRIM_COLOR)) / j;
        }
        else
            fprintf(stderr,
                    "\n%s: Null entry in quantized color map - that's weird.\n",
                    PROGRAM_NAME);
    }

    /* 最后再次扫描输入缓冲区并将映射索引放入              输出缓冲??  */
    MaxRGBError[0] = MaxRGBError[1] = MaxRGBError[2] = 0;
    for (i = 0; i < (int)(Width * Height); i++)
    {
        Index = ((RedInput[i] >> (8 - BITS_PER_PRIM_COLOR))
                 << (2 * BITS_PER_PRIM_COLOR))
                + ((GreenInput[i] >> (8 - BITS_PER_PRIM_COLOR))
                   << BITS_PER_PRIM_COLOR)
                + (BlueInput[i] >> (8 - BITS_PER_PRIM_COLOR));
        Index = ColorArrayEntries[Index].NewColorIndex;
        OutputBuffer[i] = Index;
        if (MaxRGBError[0] < SAL_SUB_ABS(OutputColorMap[Index].Red , RedInput[i]))
            MaxRGBError[0] = SAL_SUB_ABS(OutputColorMap[Index].Red , RedInput[i]);

        if (MaxRGBError[1] < SAL_SUB_ABS(OutputColorMap[Index].Green , GreenInput[i]))
            MaxRGBError[1] = SAL_SUB_ABS(OutputColorMap[Index].Green , GreenInput[i]);

        if (MaxRGBError[2] < SAL_SUB_ABS(OutputColorMap[Index].Blue , BlueInput[i]))
            MaxRGBError[2] = SAL_SUB_ABS(OutputColorMap[Index].Blue , BlueInput[i]);
    }

#ifdef DEBUG
    fprintf(stderr,
            "Quantization L(0) errors: Red = %d, Green = %d, Blue = %d.\n",
            MaxRGBError[0], MaxRGBError[1], MaxRGBError[2]);
#endif /* DEBUG */

    free((char *)ColorArrayEntries);

    *ColorMapSize = NewColorMapSize;

    return GIF_OK;
}

/******************************************************************************
* Routine to subdivide the RGB space recursively using median cut in each
* axes alternatingly until ColorMapSize different cubes exists.
* The biggest cube in one dimension is subdivide unless it has only one entry.
* Returns GIF_ERROR if failed, otherwise GIF_OK.
******************************************************************************/
static int
SubdivColorMap(NewColorMapType *NewColorSubdiv,
               unsigned int ColorMapSize,
               unsigned int *NewColorMapSize) {                                     /* 在每个区域使用中值切割递归细分RGB空间的例??             *交替轴，直到ColorMapSize不同的立方体存在?? */

    int MaxSize;
    unsigned int i, j, Index = 0, NumEntries, MinColor, MaxColor;
    long Sum, Count;
    QuantizedColorType *QuantizedColor, **SortArray;

    while (ColorMapSize > *NewColorMapSize)
    {
        /* Find candidate for subdivision: */
        MaxSize = -1;
        for (i = 0; i < *NewColorMapSize; i++)
        {
            for (j = 0; j < 3; j++)
            {
                if ((((int)NewColorSubdiv[i].RGBWidth[j]) > MaxSize)
                    && (NewColorSubdiv[i].NumEntries > 1))
                {
                    MaxSize = NewColorSubdiv[i].RGBWidth[j];
                    Index = i;
                    SortRGBAxis = j;
                }
            }
        }

        if (MaxSize == -1)
            return GIF_OK;

        /* Split the entry Index into two along the axis SortRGBAxis: */

        /* Sort all elements in that entry along the given axis and split at the median. */
        SortArray = (QuantizedColorType **)malloc(
            sizeof(QuantizedColorType *)
            * NewColorSubdiv[Index].NumEntries);
        if (SortArray == NULL)
            return GIF_ERROR;

        for (j = 0, QuantizedColor = NewColorSubdiv[Index].QuantizedColors;
             j < NewColorSubdiv[Index].NumEntries && QuantizedColor != NULL;
             j++, QuantizedColor = QuantizedColor->Pnext)
            SortArray[j] = QuantizedColor;

        qsort(SortArray, NewColorSubdiv[Index].NumEntries,
              sizeof(QuantizedColorType *), SortCmpRtn);

        /* Relink the sorted list into one: */
        for (j = 0; j < NewColorSubdiv[Index].NumEntries - 1; j++)
            SortArray[j]->Pnext = SortArray[j + 1];

        SortArray[NewColorSubdiv[Index].NumEntries - 1]->Pnext = NULL;
        NewColorSubdiv[Index].QuantizedColors = QuantizedColor = SortArray[0];
        free((char *)SortArray);

        /* Now simply add the Counts until we have half of the Count: */
        Sum = NewColorSubdiv[Index].Count / 2 - QuantizedColor->Count;
        NumEntries = 1;
        Count = QuantizedColor->Count;
        while (QuantizedColor->Pnext != NULL
               && (Sum -= QuantizedColor->Pnext->Count) >= 0
               && QuantizedColor->Pnext->Pnext != NULL)
        {
            QuantizedColor = QuantizedColor->Pnext;
            NumEntries++;
            Count += QuantizedColor->Count;
        }

        /* Save the values of the last color of the first half, and first */
        /* of the second half so we can update the Bounding Boxes later. */
        /* Also as the colors are quantized and the BBoxes are full 0..255, */
        /* they need to be rescaled. */

        MaxColor = QuantizedColor->RGB[SortRGBAxis]; /* Max. of first half */
        /* coverity[var_deref_op] */
        MinColor = QuantizedColor->Pnext->RGB[SortRGBAxis]; /* of second */
        MaxColor <<= (8 - BITS_PER_PRIM_COLOR);
        MinColor <<= (8 - BITS_PER_PRIM_COLOR);

        /* Partition right here: */
        NewColorSubdiv[*NewColorMapSize].QuantizedColors
            = QuantizedColor->Pnext;
        QuantizedColor->Pnext = NULL;
        NewColorSubdiv[*NewColorMapSize].Count = Count;
        NewColorSubdiv[Index].Count -= Count;
        NewColorSubdiv[*NewColorMapSize].NumEntries
            = NewColorSubdiv[Index].NumEntries - NumEntries;
        NewColorSubdiv[Index].NumEntries = NumEntries;
        for (j = 0; j < 3; j++)
        {
            NewColorSubdiv[*NewColorMapSize].RGBMin[j]
                = NewColorSubdiv[Index].RGBMin[j];
            NewColorSubdiv[*NewColorMapSize].RGBWidth[j]
                = NewColorSubdiv[Index].RGBWidth[j];
        }

        NewColorSubdiv[*NewColorMapSize].RGBWidth[SortRGBAxis]
            = NewColorSubdiv[*NewColorMapSize].RGBMin[SortRGBAxis]
              + NewColorSubdiv[*NewColorMapSize].RGBWidth[SortRGBAxis] - MinColor;
        NewColorSubdiv[*NewColorMapSize].RGBMin[SortRGBAxis] = MinColor;

        NewColorSubdiv[Index].RGBWidth[SortRGBAxis]
            = MaxColor - NewColorSubdiv[Index].RGBMin[SortRGBAxis];

        (*NewColorMapSize)++;
    }

    return GIF_OK;
}

/****************************************************************************
* Routine called by qsort to compare two entries.
****************************************************************************/
static int
SortCmpRtn(const void *Entry1,
           const void *Entry2) {

    return (*((QuantizedColorType **) Entry1))->RGB[SortRGBAxis]
           - (*((QuantizedColorType **) Entry2))->RGB[SortRGBAxis];
}

#if 0
/******************************************************************************
* 功  能：将rgb888数据转为GIF格式图像
* 参  数：rgbbuf——rgb数据文件名，  outfile——GIF输出文件名，width——图像宽，height——图像高
*
* 返回值：返回0
* 备  注：
* 修  改:
******************************************************************************/
int yuv_to_gif(char *yuvbuf, int w, int h, char *outfile)
{
    int Width = w, Height = h, ret = SAL_SOK;
    GifByteType *RedBuffer = NULL, *GreenBuffer = NULL, *BlueBuffer = NULL,
    *OutputBuffer = NULL;
    ColorMapObject *OutputColorMap = NULL;

    ColorMapSize = 1 << ExpNumOfColors;


    if (yuvbuf == NULL)
    {
        ret = SAL_FAIL;
        printf("no yuv file");
        return ret;
    }

    printf("全局颜色数=%d\n", ColorMapSize);
    unsigned int time3, time4, time5, time6, time1, time2;

    time1 = gifTest_getCurns();

    ret = yuv420sp_to_rgbplaner(yuvbuf, &RedBuffer, &GreenBuffer, &BlueBuffer, Width, Height);

    time2 = gifTest_getCurns();
    printf("IVE运行时间：%u.ms\n", time2 - time1);
    if ( NULL == (OutputColorMap = MakeMapObject(ColorMapSize, NULL)) 
        || NULL == (OutputBuffer = (GifByteType *) malloc(Width * Height * sizeof(GifByteType))))
        GIF_EXIT("Failed to allocate memory required, aborted.");

    time3 = gifTest_getCurns();
    if (QuantizeBuffer(Width, Height, &ColorMapSize,
                       RedBuffer, GreenBuffer, BlueBuffer,
                       OutputBuffer, OutputColorMap->Colors) == GIF_ERROR)
        QuitGifError(NULL);

    time4 = gifTest_getCurns();
    printf("rgb量化时间：%u.ms\n", time4 - time3);
    time5 = gifTest_getCurns();


    SaveGif(OutputBuffer, OutputColorMap, ExpNumOfColors, Width, Height, outfile);
    time6 = gifTest_getCurns();
    printf("lzw压缩运行时间：%u.ms\n", time6 - time5);
#ifdef MALLOC
    free((char *) RedBuffer);
    free((char *) GreenBuffer);
    free((char *) BlueBuffer);
#endif
    return ret;
}

/******************************************************************************
* Save the GIF resulting image.
******************************************************************************/
static void SaveGif(GifByteType *OutputBuffer,
                    ColorMapObject *OutputColorMap,
                    int ExpColorMapSize, int Width, int Height, char *outfile)
{
    int i;
    GifFileType *GifFile;
    GifByteType *Ptr = OutputBuffer;
    int fp12;

    fp12 = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    /* Open stdout for the output file: */
    if ((GifFile = EGifOpenFileHandle(fp12)) == NULL)
        QuitGifError(GifFile);

    if (EGifPutScreenDesc(GifFile, Width, Height, ExpColorMapSize, 0, OutputColorMap) == GIF_ERROR
        || EGifPutImageDesc(GifFile, 0, 0, Width, Height, false, NULL) == GIF_ERROR)
        QuitGifError(GifFile);

    for (i = 0; i < Height; i++)
    {
        if (EGifPutLine(GifFile, Ptr, Width) == GIF_ERROR)
            QuitGifError(GifFile);

        /* GifQprintf("\b\b\b\b%-4d", Height - i - 1); */

        Ptr += Width;
    }

    if (EGifCloseFile(GifFile) == GIF_ERROR)
        QuitGifError(GifFile);
}

/******************************************************************************
* Close output file (if open), and exit.
******************************************************************************/
static void QuitGifError(GifFileType *GifFile)
{
    PrintGifError();
    if (GifFile != NULL)
        EGifCloseFile(GifFile);

    exit(EXIT_FAILURE);
}

/******************************************************************************
* 功  能：将yuv420数据转为rgb888数据
* 参  数：url_in——yuv420数据文件名，  url_out——rgb输出文件名，width——图像宽，height——图像高
*
* 返回值：返回0
* 备  注：
* 修  改:
******************************************************************************/


int yuv420sp_to_rgbplaner(char *FileName,
                          GifByteType **RedBuffer,
                          GifByteType **GreenBuffer,
                          GifByteType **BlueBuffer,
                          int Width, int Height)
{
    int i;
    IVE_HANDLE hiyuvHandle;
    GifByteType *pData;

    FILE *fp;
    int ret = SAL_SOK;
    UINT32 uiWidth = 0;
    UINT32 uiHeight = 0;
    UINT32 u64yuvSize = Width * Height * 3 / 2;
    UINT32 u64bgrSize = Width * Height * 3;
    UINT64 u64PhyAddr = 0, bgr64PhyAddr = 0;
    UINT64 *p64VirAddr = NULL, *bgr64VirAddr = NULL;
    IVE_IMAGE_S pstYuv420sp, pstRgbPlaner;
    IVE_CSC_CTRL_S stCscCtrl;

    memset(&pstYuv420sp, 0, sizeof(IVE_IMAGE_S));
    memset(&pstRgbPlaner, 0, sizeof(IVE_IMAGE_S));

    /* 申请的内存池宽必须16对齐，高必须2对齐 */
    uiWidth	= SAL_align(Width, 16);
    uiHeight = SAL_align(Height, 2);
    u64yuvSize = uiWidth * uiHeight * 3 / 2;
    /* 开辟vb内存，移植到安检机时可以去掉 */
    p64VirAddr = HAL_memAllocFd(u64yuvSize);
    if (SAL_isNull(p64VirAddr))
    {
        printf("mallocFd fail pVirAddr %p!!!\n", p64VirAddr);
        return SAL_FAIL;
    }

    u64PhyAddr = HAL_memGetPhyAddr(p64VirAddr);
    if (SAL_FAIL == u64PhyAddr)
    {
        printf("get PhysAddr failed size %llu!\n", u64PhyAddr);
        return SAL_FAIL;
    }

    pstYuv420sp.enType = IVE_IMAGE_TYPE_YUV420SP;
    pstYuv420sp.au64VirAddr[0] = (HI_U64)p64VirAddr;
    pstYuv420sp.au64VirAddr[1] = pstYuv420sp.au64VirAddr[0] + uiWidth * uiHeight;
    pstYuv420sp.au64PhyAddr[0] = u64PhyAddr;
    pstYuv420sp.au64PhyAddr[1] = pstYuv420sp.au64PhyAddr[0] + uiWidth * uiHeight;
    pstYuv420sp.au32Stride[0] = Width;
    pstYuv420sp.au32Stride[1] = Width;
    pstYuv420sp.u32Width = Width;
    pstYuv420sp.u32Height = Height;

    u64bgrSize = uiWidth * uiHeight * 3;
    bgr64VirAddr = HAL_memAllocFd(u64bgrSize);
    if (SAL_isNull(bgr64VirAddr))
    {
        printf("mallocFd fail pVirAddr %p!!!\n", bgr64VirAddr);
        return SAL_FAIL;
    }

    bgr64PhyAddr = HAL_memGetPhyAddr(bgr64VirAddr);
    if (SAL_FAIL == bgr64PhyAddr)
    {
        printf("get PhysAddr failed size %llu!\n", bgr64PhyAddr);
        return SAL_FAIL;
    }

    pstRgbPlaner.enType = IVE_IMAGE_TYPE_U8C3_PLANAR;       /* 选择输出为b、g、r分开输出 */
    pstRgbPlaner.au64VirAddr[0] = (HI_U64)bgr64VirAddr;
    pstRgbPlaner.au64VirAddr[1] = pstRgbPlaner.au64VirAddr[0] + uiWidth * uiHeight;
    pstRgbPlaner.au64VirAddr[2] = pstRgbPlaner.au64VirAddr[1] + uiWidth * uiHeight;
    pstRgbPlaner.au64PhyAddr[0] = bgr64PhyAddr;
    pstRgbPlaner.au64PhyAddr[1] = pstRgbPlaner.au64PhyAddr[0] + uiWidth * uiHeight;
    pstRgbPlaner.au64PhyAddr[2] = pstRgbPlaner.au64PhyAddr[1] + uiWidth * uiHeight;
    pstRgbPlaner.au32Stride[0] = Width;
    pstRgbPlaner.au32Stride[1] = Width;
    pstRgbPlaner.au32Stride[2] = Width;
    pstRgbPlaner.u32Width = Width;
    pstRgbPlaner.u32Height = Height;

    fp = fopen(FileName, "rb");

    pData = (HI_U8 *)pstYuv420sp.au64VirAddr[0];
    for (i = 0; i < pstYuv420sp.u32Height; i++, pData += pstYuv420sp.au32Stride[0])
    {
        if (1 != fread(pData, pstYuv420sp.u32Width, 1, fp))
        {
            ret = SAL_FAIL;
            printf("read Y fail\n");
            break;
        }
    }

    pData = (HI_U8 *)pstYuv420sp.au64VirAddr[1];
    for (i = 0; i < pstYuv420sp.u32Height / 2; i++, pData += pstYuv420sp.au32Stride[1])
    {
        if (1 != fread(pData, pstYuv420sp.u32Width, 1, fp))
        {
            ret = SAL_FAIL;
            printf("read VU fail\n");
            break;
        }
    }

    /* 选择ive三通道输出B G        R模式*/
    stCscCtrl.enMode = IVE_CSC_MODE_PIC_BT601_YUV2RGB,

    ret = HI_MPI_IVE_CSC(&hiyuvHandle, &pstYuv420sp, &pstRgbPlaner, &stCscCtrl, HI_TRUE);

    if (ret != 0)
    {
        printf("rgb转换失败");
        return SAL_FAIL;
    }
#define MALLOC
#ifdef MALLOC
    unsigned long Size;
    unsigned char *RedP, *GreenP, *BlueP;

    Size = ((long) Width) * Height * sizeof(GifByteType);
    if ((*RedBuffer = (GifByteType *) malloc((unsigned int) Size)) == NULL
        || (*GreenBuffer = (GifByteType *) malloc((unsigned int) Size)) == NULL
        || (*BlueBuffer = (GifByteType *) malloc((unsigned int) Size)) == NULL)
        GIF_EXIT("Failed to allocate memory required, aborted.");

    RedP = *RedBuffer;
    GreenP = *GreenBuffer;
    BlueP = *BlueBuffer;

    memcpy(BlueP, (GifByteType *)pstRgbPlaner.au64VirAddr[0], Width * Height);
    memcpy(GreenP, (GifByteType *)pstRgbPlaner.au64VirAddr[1], Width * Height);
    memcpy(RedP, (GifByteType *)pstRgbPlaner.au64VirAddr[2], Width * Height);

#else

    *BlueBuffer = (GifByteType *)pstRgbPlaner.au64VirAddr[0];
    *GreenBuffer = (GifByteType *)pstRgbPlaner.au64VirAddr[1];
    *RedBuffer = (GifByteType *)pstRgbPlaner.au64VirAddr[2];
#endif

    /* ret = SAL_memFree(p64VirAddr); */
    /* ret = SAL_memFree(bgr64VirAddr); */
    fclose(fp);

    return ret;

}

/******************************************************************
   Function:   yuv2gif_text
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
void yuv2gif_text()
{
    if (SAL_SetThreadCoreBind(3) < 0)
        printf("绑定内核失败");

    unsigned int time1 = 0, time2 = 0;
    int width = 0, height = 0;
    /* char *YUVfile = NULL,*GIFfile = NULL; */
    printf("input width height \n");
    scanf("%d%d", &width, &height);
    /* printf("input width height yuv file name\n"); */
    /* printf("input yuv file name\n"); */
    /* scanf("%s", GIFfile); */

    time1 = gifTest_getCurns();

    yuv_to_gif("shuru.yuv", width, height, "shuchu.gif");

    time2 = gifTest_getCurns();
    printf("运行总时间2.03：%u.ms\n", time2 - time1);

}
#endif
/*重构几个函数，yuv->rgb->gif, 从内存读取, 最后转换gif*/

static INT32 gif_Nv21ToRgb24P(U08 *pSrc,U08* pRBuf, U08*pGBuf, U08* pBBuf, UINT32 u32Width, UINT32 u32Height, UINT32 u32Stride)
{
    INT32 i = 0, j = 0;          /*有符号整型*/
    UINT8 y = 0, u = 0, v = 0;
    INT16 r = 0, g = 0, b = 0;
    U08* pRBufTemp = NULL;
    U08* pGBufTemp = NULL;
    U08* pBBufTemp = NULL;
    if ((NULL == pSrc) || (NULL == pRBuf) || (NULL == pGBuf) || (NULL == pBBuf))
    {
        SAL_ERROR("pSrc[%p], pRBuf[%p], pGBuf[%p], pBBuf[%p] is error!\n", pSrc, pRBuf, pGBuf, pBBuf);
        return SAL_FAIL;
    }

    if ((0== u32Width) || (0 == u32Height) || (0 == u32Stride))
    {
        SAL_ERROR("u32Width[%u], u32Height[%u], u32Stride[%u] is error!\n", u32Width, u32Height, u32Stride);
        return SAL_FAIL;
    }

    pBBufTemp = pBBuf;
    pRBufTemp = pRBuf;
    pGBufTemp = pGBuf;

    for (i = 0; i < u32Height; i++)
    {
        for (j = 0; j < u32Width; j++)
        {
            y = *(pSrc + i * u32Stride + j);
            v = *(pSrc + u32Stride * u32Height + (i >> 1) * u32Stride + j - j % 2);
            u = *(pSrc + u32Stride * u32Height + (i >> 1) * u32Stride + 1 + j - j % 2);

            CVT_YUV2RGB(y,u,v,r,g,b);

            *(pRBufTemp++) = (UINT8)r;
            *(pGBufTemp++) = (UINT8)g;
            *(pBBufTemp++) = (UINT8)b;
        }
    }

    return SAL_SOK;
}

/**
 * @function:   Gif_DrvYuv2Gif
 * @brief:      yuv转换gif流程，yuv->rgb24->rgb8->gif
 * @param[in]:  YUV_ATTR_S *pstYuvStr   YUV属性
 * @param[in]:  RGB_ATTR_S *pstRgbAttr  RGB属性
 * @param[in]:  GIF_ATTR_S *pstGifAttr  GIF属性
 * @param[out]: None   无
 * @return:     INT32 GIF_ERROR 失败
                      GIF_OK    成功
 */
INT32 Gif_DrvYuv2Gif(YUV_ATTR_S *pstYuvStr,RGB_ATTR_S *pstRgbAttr, GIF_ATTR_S *pstGifAttr)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 time1 = 0, time2 = 0;

    if(NULL == pstYuvStr || NULL == pstRgbAttr || NULL == pstGifAttr)
    {
        SAL_ERROR("pstYuvStr [%p] pstRgbAttr [%p] or pstGifAttr [%p] is error!\n", pstYuvStr, pstRgbAttr, pstGifAttr);
        return GIF_ERROR;
    }

    if(NULL == pstYuvStr->pVirAddr)
    {
        SAL_ERROR("pstYuvStr->pVirAddr [%p] is error!\n", pstYuvStr->pVirAddr);
        return GIF_ERROR;
    }

    if(NULL == pstGifAttr->pGifBuf)
    {
        SAL_ERROR("pstGifAttr->pGifBuf [%p] is error!\n", pstGifAttr->pGifBuf);
        return GIF_ERROR;
    }

    if((NULL == pstRgbAttr->pRgb8Buf) 
       || (NULL == pstRgbAttr->pRgb24Buf[0]) 
       || (NULL == pstRgbAttr->pRgb24Buf[1]) 
       || (NULL == pstRgbAttr->pRgb24Buf[2]))
    {
        SAL_ERROR("pRgb8Buf [%p] pRgb24Buf0 [%p] pRgb24Buf1 [%p], pRgb24Buf2 [%p] is error!\n", 
                  pstRgbAttr->pRgb8Buf, 
                  pstRgbAttr->pRgb24Buf[0], 
                  pstRgbAttr->pRgb24Buf[1],
                  pstRgbAttr->pRgb24Buf[2]);
        return GIF_ERROR;
    }

    /***************YUV转换成RGB24****************/

    time1 = gifTest_getCurns();
    s32Ret = Gif_DrvYuv2Rgb24(pstYuvStr, pstRgbAttr);
    time2 = gifTest_getCurns();
    SAL_INFO("yuv->rgb24:%u ms\n", time2 - time1);
    if(GIF_OK != s32Ret)
    {
        SAL_ERROR("Gif_DrvYuv2Rgb24 is error!\n");
        return GIF_ERROR;
    }

    if (GIF_OK != Gif_DrvRgb2Gif(pstRgbAttr, pstGifAttr))
    {
        XSP_LOGE("Gif_DrvRgb2Gif failed\n");
        return GIF_ERROR;
    }

    return GIF_OK;
}

/**
 * @function:   Gif_DrvRgb2Gif
 * @brief:      rgb转换gif流程，rgb24->rgb8->gif
 * @param[in]:  RGB_ATTR_S *pstRgbAttr  RGB属性
 * @param[in]:  GIF_ATTR_S *pstGifAttr  GIF属性
 * @param[out]: None   无
 * @return:     INT32 GIF_ERROR 失败
                      GIF_OK    成功
 */
INT32 Gif_DrvRgb2Gif(RGB_ATTR_S *pstRgbAttr, GIF_ATTR_S *pstGifAttr)
{
    static ColorMapObject *OutputColorMap = NULL;

    if(NULL == pstRgbAttr || NULL == pstGifAttr)
    {
        SAL_ERROR("pstRgbAttr [%p] or pstGifAttr [%p] is error!\n", pstRgbAttr, pstGifAttr);
        return GIF_ERROR;
    }

    if(NULL == pstGifAttr->pGifBuf)
    {
        SAL_ERROR("pstGifAttr->pGifBuf [%p] is error!\n", pstGifAttr->pGifBuf);
        return GIF_ERROR;
    }

    if((NULL == pstRgbAttr->pRgb8Buf) 
       || (NULL == pstRgbAttr->pRgb24Buf[0]) 
       || (NULL == pstRgbAttr->pRgb24Buf[1]) 
       || (NULL == pstRgbAttr->pRgb24Buf[2]))
    {
        SAL_ERROR("pRgb8Buf [%p] pRgb24Buf0 [%p] pRgb24Buf1 [%p], pRgb24Buf2 [%p] is error!\n", 
                  pstRgbAttr->pRgb8Buf, 
                  pstRgbAttr->pRgb24Buf[0], 
                  pstRgbAttr->pRgb24Buf[1],
                  pstRgbAttr->pRgb24Buf[2]);
        return GIF_ERROR;
    }
    /***************RGB24转换成RGB8***************/
    /*调色板,256个，创建映射对象*/
    ColorMapSize = 1 << ExpNumOfColors;
    if (NULL == OutputColorMap)
    {
        if (NULL == (OutputColorMap = MakeMapObject(ColorMapSize, NULL)))
        {
            SAL_ERROR("OutputColorMap [%p] is error!\n", OutputColorMap);
            return GIF_ERROR;
        }
    }

    if (GIF_ERROR == QuantizeBuffer(pstRgbAttr->uiWidth, pstRgbAttr->uiHeight, &ColorMapSize,
                                    (GifByteType *)pstRgbAttr->pRgb24Buf[0], 
                                    (GifByteType *)pstRgbAttr->pRgb24Buf[1], 
                                    (GifByteType *)pstRgbAttr->pRgb24Buf[2],
                                    (GifByteType *)pstRgbAttr->pRgb8Buf, 
                                    OutputColorMap->Colors))
    {
        SAL_ERROR("QuantizeBuffer is error!\n");
        return GIF_ERROR;
    }

    /****************RGB8转换成GIF****************/
    if (GIF_OK != Gif_DrvSave(pstGifAttr, pstRgbAttr, OutputColorMap, ExpNumOfColors))
    {
        SAL_ERROR("Gif_DrvSave is error!\n");
        return GIF_ERROR;
    }

    return GIF_OK;
}

/**
 * @function:   Gif_DrvYuv2Rgb24
 * @brief:      yuv转换rgb24流程，yuv->rgb24，目前宽度必须16对齐
 * @param[in]:  YUV_ATTR_S *pstYuvStr   YUV属性
 * @param[in]:  RGB_ATTR_S *pstRgbAttr  RGB属性
 * @param[out]: None   无
 * @return:     INT32 GIF_ERROR 失败
                      GIF_OK    成功
 */
INT32 Gif_DrvYuv2Rgb24(YUV_ATTR_S *pstYuvStr,RGB_ATTR_S *pstRgbAttr)
{
    int s32Ret = SAL_SOK;
    UINT32 uiWidth = 0;
    UINT32 uiHeight = 0;
    UINT32 uiStride = 0;
    IVE_HAL_IMAGE stIveYuv = {0};
    IVE_HAL_IMAGE stIveRgb = {0};
    IVE_HAL_MODE_CTRL stCscCtrl = {0};

    memset(&stIveYuv, 0, sizeof(IVE_HAL_IMAGE));
    memset(&stIveRgb, 0, sizeof(IVE_HAL_IMAGE));

    if(NULL == pstYuvStr || NULL == pstRgbAttr)
    {
        SAL_ERROR("pstYuvStr [%p] or pstRgbAttr [%p] is error!\n", pstYuvStr, pstRgbAttr);
        return GIF_ERROR;
    }

    if(NULL == pstYuvStr->pVirAddr)
    {
        SAL_ERROR("pstYuvStr->pVirAddr [%p] is error!\n", pstYuvStr->pVirAddr);
        return GIF_ERROR;
    }

    if((NULL == pstRgbAttr->pRgb8Buf) 
       || (NULL == pstRgbAttr->pRgb24Buf[0]) 
       || (NULL == pstRgbAttr->pRgb24Buf[1]) 
       || (NULL == pstRgbAttr->pRgb24Buf[2]))
    {
        SAL_ERROR("pRgb8Buf [%p] pRgb24Buf0 [%p] pRgb24Buf1 [%p], pRgb24Buf2 [%p] is error!\n", 
                  pstRgbAttr->pRgb8Buf, 
                  pstRgbAttr->pRgb24Buf[0], 
                  pstRgbAttr->pRgb24Buf[1],
                  pstRgbAttr->pRgb24Buf[2]);
        return GIF_ERROR;
    }

    SAL_INFO("pstYuvStr w %d, S %d, H %d!\n", pstYuvStr->uiWidth, pstYuvStr->u32Stride, pstYuvStr->uiHeight);

    /*ive功能支持的分辨率是[64*64]~[1920*1080]，但是实际支持[64*64]~[1920*1920]，所以采用后者，地址必须16对齐*/
    if ((0 == pstYuvStr->uiWidth % 16) 
        && (0 == pstYuvStr->uiHeight % 16)
        && ((pstYuvStr->uiWidth >= 64) && (pstYuvStr->uiWidth <= 1920))
        && ((pstYuvStr->uiHeight >= 64) && (pstYuvStr->uiHeight <= 1920)))
    {
        /* 申请的内存池宽必须16对齐，高必须2对齐 */
        uiWidth = pstYuvStr->uiWidth;
        uiStride = pstYuvStr->u32Stride;
        uiHeight = SAL_align(pstYuvStr->uiHeight, 2);
        stIveYuv.enColorType = SAL_VIDEO_DATFMT_YUV420SP_UV;      /*YUV420 SemiPlanar*/
        stIveYuv.u64VirAddr[0] = (UINT64)(pstYuvStr->pVirAddr);
        stIveYuv.u64VirAddr[1] = stIveYuv.u64VirAddr[0] + uiStride * uiHeight;
        stIveYuv.u64PhyAddr[0] = pstYuvStr->u64PhyAddr;
        stIveYuv.u64PhyAddr[1] = stIveYuv.u64PhyAddr[0] + uiStride * uiHeight;
        stIveYuv.u32Stride[0] = uiStride; /*需要16对齐*/
        stIveYuv.u32Stride[1] = uiStride;
        stIveYuv.u32Width = uiWidth;
        stIveYuv.u32Height = uiHeight;

        stIveRgb.enColorType = SAL_VIDEO_DATFMT_BGR;       /* 选择输出为b、g、r分开输出 */
        stIveRgb.u64VirAddr[0] = (UINT64)(pstRgbAttr->pRgb24Buf[0]);
        stIveRgb.u64VirAddr[1] = (UINT64)(pstRgbAttr->pRgb24Buf[1]);
        stIveRgb.u64VirAddr[2] = (UINT64)(pstRgbAttr->pRgb24Buf[2]);
        stIveRgb.u64PhyAddr[0] = pstRgbAttr->u64PhyAddr;
        stIveRgb.u64PhyAddr[1] = stIveRgb.u64PhyAddr[0] + uiStride * uiHeight;
        stIveRgb.u64PhyAddr[2] = stIveRgb.u64PhyAddr[1] + uiStride * uiHeight;
        stIveRgb.u32Stride[0] = uiStride;
        stIveRgb.u32Stride[1] = uiStride;
        stIveRgb.u32Stride[2] = uiStride;
        stIveRgb.u32Width = uiWidth;
        stIveRgb.u32Height = uiHeight;

        /* 海思ive模块：输出B G R模式，地址16对齐*/
        stCscCtrl.u32enMode = IVE_CSC_PIC_BT601_YUV2RGB,
        //s32Ret = HI_MPI_IVE_CSC(&iveHandle, &stIveYuv, &stIveRgb, &stCscCtrl, HI_TRUE);
		s32Ret = ive_hal_CSC(&stIveYuv, &stIveRgb, &stCscCtrl);
        if (SAL_SOK != s32Ret)
        {
            SAL_ERROR("ive_hal_CSC is error s32Ret with %x!\n ", s32Ret);
            return GIF_ERROR;
        }
    }
    else
    {
        uiStride = pstYuvStr->u32Stride;
        SAL_WARN("gif: width %d  stride %d or height %d, yuv->gif by software!\n", pstYuvStr->uiWidth, uiStride, pstYuvStr->uiHeight);
        gif_Nv21ToRgb24P(pstYuvStr->pVirAddr, pstRgbAttr->pRgb24Buf[0], pstRgbAttr->pRgb24Buf[1], pstRgbAttr->pRgb24Buf[2], pstYuvStr->uiWidth, pstYuvStr->uiHeight, uiStride);
    }

    return GIF_OK;
}

/**
 * @function:   Gif_DrvSave
 * @brief:      rgb8转换gif流程，rgb8->gif
 * @param[in]:  GIF_ATTR_S *pstGifAttr  GIF属性
 * @param[in]:  RGB_ATTR_S *pstRgbAttr  RGB属性
 * @param[in]:  ColorMapObject *OutputColorMap 颜色板
 * @param[in]:  int ExpColorMapSize 调色板大小
 * @param[out]: None   无
 * @return:     INT32 GIF_ERROR 失败
                      GIF_OK    成功
 */
INT32 Gif_DrvSave(GIF_ATTR_S *pstGifAttr, RGB_ATTR_S *pstRgbAttr, ColorMapObject *OutputColorMap, int ExpColorMapSize)
{
    int i = 0 ;
    GifFileType *GifFile = NULL;
    GifByteType *Ptr = (GifByteType *)pstRgbAttr->pRgb8Buf;
    UINT32 Width = pstRgbAttr->uiWidth;
    UINT32 Height = pstRgbAttr->uiHeight;

    if(NULL == pstGifAttr || NULL == pstRgbAttr || NULL == OutputColorMap)
    {
        SAL_ERROR("pstGifAttr [%p] or pstRgbAttr [%p] , pstGifAttr [%p] is error! and return.\n", pstGifAttr, pstRgbAttr, OutputColorMap);
        return GIF_ERROR;
    }

    if (NULL == (GifFile = EGifOpen((void*)pstGifAttr, Gif_DrvBufWrite)))
    {
        SAL_ERROR("GifFile [%p] error! and return.\n", GifFile);
        return GIF_ERROR;
    }

    if(GIF_ERROR == (EGifPutScreenDesc(GifFile, Width, Height, ExpColorMapSize, 0, OutputColorMap)))
    {
        SAL_ERROR("EGifPutScreenDesc is error\n");
        return GIF_ERROR;
    }

    if(GIF_ERROR == (EGifPutImageDesc(GifFile, 0, 0, Width, Height, false, NULL)))
    {
        SAL_ERROR("EGifPutImageDesc is error\n");
        return GIF_ERROR;
    }

    for (i = 0; i < Height; i++)
    {
        if (EGifPutLine(GifFile, Ptr, Width) == GIF_ERROR)
        {
            SAL_ERROR("EGifPutLine is error\n");
            return GIF_ERROR;
        }

        Ptr += Width;
    }

    if (EGifCloseFile(GifFile) == GIF_ERROR)
    {
        SAL_ERROR("EGifCloseFile is error\n");
        return GIF_ERROR;
    }

    return GIF_OK;
}

/**
 * @function:   Gif_DrvBufWrite
 * @brief:      gif缓冲区写操作
 * @param[in]:  GifFileType *GifFile  GIF对象
 * @param[in]:  const GifByteType *pByteTypeBuf 写入的源数据
 * @param[in]:  int uiLen 写入长度
 * @param[out]: None   无
 * @return:     INT32 GIF_ERROR 失败
                INT32 uiLen     写入长度
 */
INT32 Gif_DrvBufWrite(GifFileType *GifFile, const GifByteType *pByteTypeBuf, INT32 uiLen)
{
    if ((NULL == GifFile) || (NULL == pByteTypeBuf))
    {
        SAL_ERROR("GifFile [%p] or pSrcBuf [%p] is error!\n", GifFile, pByteTypeBuf);
        return GIF_ERROR;
    }

    GIF_ATTR_S *pstGifAttr = (GIF_ATTR_S *)GifFile->UserData;
    if (NULL == pstGifAttr)
    {
        SAL_ERROR("GifFile->UserData [%p] is error!\n", GifFile->UserData);
        return GIF_ERROR;
    }
    if (NULL == pstGifAttr->pGifBuf)
    {
        SAL_ERROR("pstGifAttr->pGifBuf [%p] is error!\n", pstGifAttr->pGifBuf);
        return GIF_ERROR;
    }

    if (pstGifAttr->uiWriteId + uiLen > pstGifAttr->uiSizeMax)
    {
        SAL_ERROR("uiWriteId [%d] is error, uiLen %d, uiSizeMax %d !\n", 
                  pstGifAttr->uiWriteId, 
                  uiLen, 
                  pstGifAttr->uiSizeMax);
        return GIF_ERROR;
    }

    sal_memcpy_s(pstGifAttr->pGifBuf + pstGifAttr->uiWriteId, uiLen, pByteTypeBuf, uiLen);
    pstGifAttr->uiWriteId += uiLen;

    /*返回长度*/
    return uiLen;
}
