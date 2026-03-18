

#ifndef __FREETYPE_FUNC_H_
#define __FREETYPE_FUNC_H_

#include "sal.h"
/* 加载字体方式 */
typedef enum tagFreetypeFaceLoadType
{
    FREETYPE_FACE_LOAD_TYPE_FILE = 0,           /* 从字体文件中加载 */
    FREETYPE_FACE_LOAD_TYPE_MEMORY,             /* 从内存中加载 */
    FREETYPE_FACE_LOAD_TYPE_BUTT,
} FREETYPE_FACE_LOAD_TYPE_E;

/* 内存参数 */
typedef struct tagFreeTypeMemory
{
    VOID *pvAddr;
    UINT32 u32Size;
} FREETYPE_MEMORY_S;

/* freetype初始化结构体 */
typedef struct tagFreetypeFacePrm
{
    FREETYPE_FACE_LOAD_TYPE_E enLoadType;
    union
    {
        char *szFontName;
        FREETYPE_MEMORY_S stMemory;
    };
} FREETYPE_FACE_PRM_S;

/* 上层调用填充字库结构体 */
typedef struct tagFreetypeBlock
{
    UINT8 *pu8Buffer;       /* 经过freetype处理后的点阵存放位置 */
    UINT32 u32Width;        /* 有效数据的宽度，单位：pixel */
    UINT32 u32Height;       /* 有效数据的高度，单位：pixel */
    UINT32 u32Stride;       /* 跨距，单位：字节 */
    UINT32 u32Size;         /* 内存大小 */
} FREETYPE_BLOCK_S;

/* 当前字符起笔的位置 */
typedef struct tagFreetypePen
{
    UINT32 u32XOffset;      /* 起笔位置在x方向第u32XOffset个字节 */
    UINT32 u32YOffset;      /* 起笔位置在y方向第u32YOffset个字节，此处没有作用，做预留 */
    UINT8 u8XShift;         /* 起笔位置在x方向第u32XOffset个字节的第u8XShift位 */
    UINT8 u8YShift;         /* 起笔位置在y方向第u32YOffset个字节的第u8YShift位，此处没有作用，做预留 */
} FREETYPE_PEN_S;

INT32 Freetype_SetFace(FREETYPE_FACE_PRM_S *pstFace);
INT32 Freetype_DeInit(VOID);
INT32 Freetype_DrawByString(FREETYPE_BLOCK_S *pstBlock, char *szString, UINT32 u32Size, ENCODING_FORMAT_E enEncFormat);


#endif

