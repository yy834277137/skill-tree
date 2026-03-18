/*******************************************************************************
 * drawChar.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Version: V1.0.0  2008年11月25日 Wangjun          Create
 * Version: V1.0.1  2009年02月06日 Yuezhenxiao      增加DrawCharH4V4
 *
 * Description : 对点阵字库进行X和Y方向的放大缩小
 * Modification:
 *******************************************************************************/

#include <sal.h>
#include "drawChar.h"


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
                  UINT32 color,UINT32 bgcolor)
{
    UINT32 i, j, k;
    UINT32 fontColumnCnt = w / 8;
    unsigned short *pTemp = (unsigned short *)pDst;

    /* OSD_LOGI("draw char h1v1\n"); */
    for(i = 0; i < h; i++)
    {
        for(j = 0; j < fontColumnCnt; j++)
        {
            for(k = 0; k < 8; k++)
            {
                if((pFont[i * fontColumnCnt + j] >> (7 - k)) & 0x1)
                {
                    pTemp[i * pitch + j * 8 + k] = (short)color;
                }
                else
                {
                    pTemp[i * pitch + j * 8 + k] = bgcolor;
                }
            }

            /* pTemp[i*pitch+j]=pFont[i*fontColumnCnt+j]; */
        }
    }

    /* for(i=0;i<(h);i++) */
    /* { */
    /*  for(j=0;j<(w);j++) */
    /*  { */
    /*      if(pTemp[i * pitch + j] == 0x8000) */
    /*      { */
    /*          if(((j!=(w-1)) && (pTemp[i * pitch + j + 1] == OSD_COLOR)) */
    /*              || ((j!=0) && (pTemp[i*pitch + j - 1] == OSD_COLOR)) */
    /*              || ((i!=0) && (pTemp[(i-1)*pitch +j] == OSD_COLOR)) */
    /*              || ((i!=(h-1)) && (pTemp[(i+1) *pitch +j] == OSD_COLOR))) */
    /*          { */
    /*              pTemp[i*pitch + j] = 0x0000; */
    /*          } */
    /*      } */
    /*  } */
    /* } */
}


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
                       UINT32 color)
{
    UINT32 i, j, k;
    unsigned short *pTemp = (unsigned short *)pDst;
    unsigned short *pSTemp = (unsigned short *)pSrc;

    for(i = 0; i < (h / 2); i++)
    {
        for(j = 0; j < w / 2; j++)
        {
            k = 0;

            if(pSTemp[i * 2 * pitch + j * 2] == (short)color)
            {
                k++;
            }

            if(pSTemp[i * 2 * pitch + j * 2 + 1] == (short)color)
            {
                k++;
            }

            if(pSTemp[(i * 2 + 1) * pitch + j * 2] == (short)color)
            {
                k++;
            }

            if(pSTemp[(i * 2 + 1) * pitch + j * 2 + 1] == (short)color)
            {
                k++;
            }

            /* OSD_LOGI("nColor=%x\n",nColor); */
            if(k >= 1)
            {
                pTemp[i * pitch / 2 + j] = (short)color;
            }
            else
            {
                pTemp[i * pitch / 2 + j] = OSD_BACK_COLOR;
            }
        }
    }
}


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
                  UINT32 color,UINT32 bgcolor)
{
    UINT32 i, j, k;
    unsigned short *pTemp = (unsigned short *)pDst;
    unsigned short *pSTemp = (unsigned short *)pSrc;

    /* short r,g,b,nColor; */
    for(i = 0; i < (h / 2); i++)
    {
        for(j = 0; j < w / 2; j++)
        {
            k = 0;

            if(pSTemp[i * 2 * pitch + j * 2] == (short)color)
            {
                k++;
            }

            if(pSTemp[i * 2 * pitch + j * 2 + 1] == (short)color)
            {
                k++;
            }

            if(pSTemp[(i * 2 + 1) * pitch + j * 2] == (short)color)
            {
                k++;
            }

            if(pSTemp[(i * 2 + 1) * pitch + j * 2 + 1] == (short)color)
            {
                k++;
            }

            /* r=((pSTemp[i*2*pitch + j*2]>>10) &0x1f);
               g=((pSTemp[i*2*pitch + j*2]>>5) &0x1f);
               b=((pSTemp[i*2*pitch + j*2]) &0x1f);

               r+=((pSTemp[i*2*pitch + j*2 + 1]>>10) &0x1f);
               g+=((pSTemp[i*2*pitch + j*2 + 1]>>5) &0x1f);
               b+=((pSTemp[i*2*pitch + j*2 + 1]) &0x1f);

               r+=((pSTemp[(i*2+1)*pitch + j*2]>>10) &0x1f);
               g+=((pSTemp[(i*2+1)*pitch + j*2]>>5) &0x1f);
               b+=((pSTemp[(i*2+1)*pitch + j*2]) &0x1f);

               r+=((pSTemp[(i*2+1)*pitch + j*2+1]>>10) &0x1f);
               g+=((pSTemp[(i*2+1)*pitch + j*2+1]>>5) &0x1f);
               b+=((pSTemp[(i*2+1)*pitch + j*2+1]) &0x1f);

               nColor = (((r>>2)&0x1f)<<10) | (((g>>2)&0x1f)<<5) | (((b>>2)&0x1f));

               if(nColor==0)
               {
                nColor = 0x8000;
               }
               pTemp[i * pitch / 2  + j] = nColor;*/
            /* OSD_LOGI("nColor=%x\n",nColor); */
            if(k >= 2)
            {
                pTemp[i * pitch / 2 + j] = (short)color;
            }
            else
            {
                pTemp[i * pitch / 2 + j] = OSD_BACK_COLOR;
            }
        }
    }

    /* for(i=0;i<(h);i++) */
    /* { */
    /*  for(j=0;j<(w);j++) */
    /*  { */
    /*      if(pTemp[i * pitch + j] == 0x8000) */
    /*      { */
    /*          if(((j!=(w-1)) && (pTemp[i * pitch + j + 1] == OSD_COLOR)) */
    /*              || ((j!=0) && (pTemp[i*pitch + j - 1] == OSD_COLOR)) */
    /*              || ((i!=0) && (pTemp[(i-1)*pitch +j] == OSD_COLOR)) */
    /*              || ((i!=(h-1)) && (pTemp[(i+1) *pitch +j] == OSD_COLOR))) */
    /*          { */
    /*              pTemp[i*pitch + j] = 0x0000; */
    /*          } */
    /*      } */
    /*  } */
    /* } */
}


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
                  UINT32 color,UINT32 bgcolor)
{
    UINT32 i, j;
    UINT32 fontColumnCnt = w / 8;

    h /= 2;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < fontColumnCnt; j++)
        {
            pDst[2 * i * pitch + j] = pFont[i * fontColumnCnt + j];
            pDst[(2 * i + 1) * pitch + j] = pFont[i * fontColumnCnt + j];
        }
    }
}


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
                  UINT32 color,UINT32 bgcolor)
{
    UINT32 i, j, k;
    UINT32 fontColumnCnt;
    unsigned short *pTemp = (unsigned short *)pDst;

    w /= 2;
    h /= 2;
    fontColumnCnt = w / 8;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < fontColumnCnt; j++)
        {
            for(k = 0; k < 8; k++)
            {
                if((pFont[i * fontColumnCnt + j] >> (7 - k)) & 0x1)
                {
                    pTemp[2 * i * pitch + (j * 8 + k) * 2] = (short)color;
                    pTemp[2 * i * pitch + (j * 8 + k) * 2 + 1] = (short)color;
                }
                else
                {
                    pTemp[2 * i * pitch + (j * 8 + k) * 2] = bgcolor;
                    pTemp[2 * i * pitch + (j * 8 + k) * 2 + 1] = bgcolor;
                }
            }
        }
    }

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w * 2; j++)
        {
            pTemp[(2 * i + 1) * pitch + j] = pTemp[2 * i * pitch + j];
        }
    }
}

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
void DrawCharH3V3(PUINT8 pFont, PUINT8 pDst, UINT32 w, UINT32 h, UINT32 pitch, UINT32 color,UINT32 bgcolor)
{
    UINT32 i, j, k;
    UINT32 fontColumnCnt;
    unsigned short *pTemp = (unsigned short *)pDst;

    w /= 3;
    h /= 3;
    fontColumnCnt = w / 8;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < fontColumnCnt; j++)
        {
            for(k = 0; k < 8; k++)
            {
                if((pFont[i * fontColumnCnt + j] >> (7 - k)) & 0x1)
                {
                    pTemp[3 * i * pitch + (j * 8 + k) * 3]     = (short)color;
                    pTemp[3 * i * pitch + (j * 8 + k) * 3 + 1] = (short)color;
					pTemp[3 * i * pitch + (j * 8 + k) * 3 + 2] = (short)color;
                }
                else
                {
                    pTemp[3 * i * pitch + (j * 8 + k) * 3]     = bgcolor;
                    pTemp[3 * i * pitch + (j * 8 + k) * 3 + 1] = bgcolor;
					pTemp[3 * i * pitch + (j * 8 + k) * 3 + 2] = bgcolor;
                }
            }
        }
    }

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w * 3; j++)
        {
            pTemp[(3 * i + 1) * pitch + j] = pTemp[3 * i * pitch + j];
			pTemp[(3 * i + 2) * pitch + j] = pTemp[3 * i * pitch + j];
        }
    }
}


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
                  UINT32 color,UINT32 bgcolor)
{
    UINT32 i, j, k;
    UINT32 fontColumnCnt;

    unsigned short *pTemp = (unsigned short *)pDst;

    w /= 4;
    h /= 4;
    fontColumnCnt = w / 8;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < fontColumnCnt; j++)
        {

            for(k = 0; k < 8; k++)
            {
                if ((i * fontColumnCnt + j) > 4096)
                {
                    return ;
                }
                if((pFont[i * fontColumnCnt + j] >> (7 - k)) & 0x1)
                {
                    pTemp[4 * i * pitch + (j * 8 + k) * 4] = (short)color;
                    pTemp[4 * i * pitch + (j * 8 + k) * 4 + 1] = (short)color;
                    pTemp[4 * i * pitch + (j * 8 + k) * 4 + 2] = (short)color;
                    pTemp[4 * i * pitch + (j * 8 + k) * 4 + 3] = (short)color;
                }
                else
                {
                    pTemp[4 * i * pitch + (j * 8 + k) * 4] = bgcolor;
                    pTemp[4 * i * pitch + (j * 8 + k) * 4 + 1] = bgcolor;
                    pTemp[4 * i * pitch + (j * 8 + k) * 4 + 2] = bgcolor;
                    pTemp[4 * i * pitch + (j * 8 + k) * 4 + 3] = bgcolor;
                }
            }
        }
    }

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w * 4; j++)
        {
            pTemp[(4 * i + 1) * pitch + j] = pTemp[4 * i * pitch + j];
            pTemp[(4 * i + 2) * pitch + j] = pTemp[4 * i * pitch + j];
            pTemp[(4 * i + 3) * pitch + j] = pTemp[4 * i * pitch + j];
        }
    }
}

