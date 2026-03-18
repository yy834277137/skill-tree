

#ifndef _DRAWCHAR_H_
#define _DRAWCHAR_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


#if defined(HI3559A_SPC030)
#define OSD_COLOR_KEY       0x7F7F
#define OSD_COLOR           0x5EF7

#define OSD_COLOR_WHITE     0x7FFF
#define OSD_COLOR_BLACK     0x0000
#define OSD_BACK_COLOR      0xC210
#define OSD_COLOR_RED       0x7C00

#define OSD_COLOR24_WHITE   0xFFFFFF
#define OSD_COLOR24_RED     0xFF0000
#define OSD_COLOR24_BACK    0x808080

#elif defined(RK3588)
#define OSD_COLOR_KEY       0x7F7F
#define OSD_COLOR           0x5EF7

/*RK底层只支持BGRA5551，大端模式。所以上层还是用ARGB1555小端模式。与NT还是海思一致还需再看一下效果，目前与NT一致，难穿透能叠上 */
#define OSD_COLOR_WHITE     0xFFFF
#define OSD_COLOR_BLACK     0x8000
#define OSD_BACK_COLOR      0x4210   //0x4210 透明,即a为0；0xC210非透明，a为1


#define OSD_COLOR_RED       0xFC00


#define OSD_COLOR24_WHITE   0xFFFFFF
#define OSD_COLOR24_RED     0xFF0000 /* 这里还需确认要不要调整大小端，目前这个TIP的颜色是红色是对的 */
#define OSD_COLOR24_BACK    0x808080


#elif defined(NT98336)
#define OSD_COLOR_KEY       0x7F7F
#define OSD_COLOR           0x5EF7

#define OSD_COLOR_WHITE     0xFFFF
#define OSD_COLOR_BLACK     0x8000
#define OSD_BACK_COLOR      0x4210      // 之前为0xC210,nt开发修改，hisi未测试
#define OSD_COLOR_RED       0xFC00


#define OSD_COLOR24_WHITE   0xFFFFFF
#define OSD_COLOR24_RED     0xFF0000
#define OSD_COLOR24_BACK    0x808080


#endif



/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */
/*******************************************************************************
* 函数名  : DrawCharH1V1
* 描  述  : 复制单倍大小OSD 字符点阵
* 输  入  : - pFont: 字符原指针
*         : - pDst : 目的指针
*         : - w    : 字符宽度
*         : - h    : 字符高度
*         : - pitch: OSD字节数的图像宽度
*         : - color: 点阵颜色
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
void DrawCharH1V1(PUINT8 pFont, PUINT8 pDst, UINT32 w, UINT32 h, UINT32 pitch,
                  UINT32 color,UINT32 bgcolor);


/*******************************************************************************
* Function      : DrawCharH0V0Colon
* Description   : 1/2大小OSD 字符点阵，专门画冒号
* Input         : - pSrc : 字符原指针
*               : - pDst : 目的指针
*               : - w    : 字符宽度
*               : - h    : 字符高度
*               : - pitch: OSD总字节数的图像字节数宽度
*               : - color: OSD颜色
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
void DrawCharH0V0Colon(PUINT8 pSrc, PUINT8 pDst, UINT32 w, UINT32 h, UINT32 pitch,
                       UINT32 color);


/*******************************************************************************
* Function      : DrawCharH1V2
* Description   : 复制单倍大小OSD 字符点阵
* Input         : - pFont: 字符原指针
*               : - pDst : 目的指针
*               : - w    : 字符宽度
*               : - h    : 字符高度
*               : - pitch: OSD块字节数图像宽度
*               : - color: OSD颜色
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
void DrawCharH0V0(PUINT8 pSrc, PUINT8 pDst, UINT32 w, UINT32 h, UINT32 pitch,
                  UINT32 color,UINT32 bgcolor);


/*******************************************************************************
* Function      : DrawCharH1V2
* Description   : 画水平方向单倍大小，垂直方向2倍放大的OSD 字符点阵
* Input         : - pFont: 字符原指针
*               : - pDst : 目的指针
*               : - w    : 字符宽度
*               : - h    : 字符高度
*               : - pitch: OSD块字节数图像宽度
*               : - color: OSD颜色
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
void DrawCharH1V2(PUINT8 pFont, PUINT8 pDst, UINT32 w, UINT32 h, UINT32 pitch,
                  UINT32 color,UINT32 bgcolor);


/*******************************************************************************
* Function      : DrawCharH2V2
* Description   : 画水平方向2 倍放大，垂直方向2倍放大的OSD 字符点阵
* Input         : - pFont: 字符原指针
*               : - pDst : 目的指针
*               : - w    : 字符宽度
*               : - h    : 字符高度
*               : - pitch: OSD块字节数图像宽度
*               : - color: OSD颜色
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
void DrawCharH2V2(PUINT8 pFont, PUINT8 pDst, UINT32 w, UINT32 h, UINT32 pitch,
                  UINT32 color,UINT32 bgcolor);

/*******************************************************************************
* Function      : DrawCharH3V3
* Description   : 画水平方向3 倍放大，垂直方向3倍放大的OSD 字符点阵
* Input         : - pFont: 字符原指针
*               : - pDst : 目的指针
*               : - w    : 字符宽度
*               : - h    : 字符高度
*               : - pitch: OSD块字节数图像宽度
*               : - color: OSD颜色
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
void DrawCharH3V3(PUINT8 pFont, PUINT8 pDst, UINT32 w, UINT32 h, UINT32 pitch, UINT32 color,UINT32 bgcolor);

/*******************************************************************************
* Function      : DrawCharH4V4
* Description   : 画水平方向和垂直方向均4 倍放大的OSD 字符点阵
* Input         : - pFont: 字符原指针
*               : - pDst : 目的指针
*               : - w    : 字符宽度
*               : - h    : 字符高度
*               : - pitch: OSD字符总字节数的图像宽度
*               : - color: OSD颜色
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
void DrawCharH4V4(PUINT8 pFont, PUINT8 pDst, UINT32 w, UINT32 h, UINT32 pitch,
                  UINT32 color,UINT32 bgcolor);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

