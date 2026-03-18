/**
* @file  hal_comm_neon.h
* @note  2001-2012, ~{:<V]:#?5M~JSJ}WV<<Ju9I7]SPO^9+K>~}
* @brief  fpga~{EdVCM7ND<~~}

* @author  wutao19
* @date    2019/01/09

* @note :
* @note \n History:
  1.~{HU~}    ~{FZ~}: 2019/01/09
    ~{Ww~}    ~{U_~}: wutao19
    ~{P^8D@zJ7~}: ~{44=(ND<~~}
*/
#ifndef _HAL_COMM_NEON_H_
#define _HAL_COMM_NEON_H_

/*----------------------------------------------*/
/*                 ~{0|:,M7ND<~~}                   */
/*----------------------------------------------*/
#include "sal_type.h"
#include "sal.h"

/*----------------------------------------------*/
/*                 ~{:j@`PM6(Re~}                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 ~{H+>V1dA?~}                     */
/*----------------------------------------------*/


/*----------------------------------------------*/
/*                ~{=a99Le6(Re~}                    */
/*----------------------------------------------*/

//~{=+~}nv21~{8qJ=~}yuv~{W*3I~}rgb888~{#,R;4N4&@m~}width~{8vJ}>]~}width~{PhR*~}8~{6TFk~}
void Hal_CommNv21ToRgb24RowAlign8(const char* src_y, const char* src_vu, char* dst_rgb24,int width);

void Hal_commDrawCharBgBlackH1V1(UINT8 *pFont, UINT8 * pDst, UINT32 w, UINT32 h, UINT32 pitch, UINT16 color,UINT32 bGcolor);

void Hal_commDrawCharBgBlackH2V2(UINT8 *pFont, UINT8 * pDst, UINT32 w, UINT32 h, UINT32 pitch, UINT16 color,UINT32 bGcolor);

void Hal_commDrawCharBgBlackH3V3(UINT8 *pFont, UINT8 * pDst, UINT32 w, UINT32 h, UINT32 pitch, UINT16 color,UINT32 bGcolor);

void Hal_commDrawCharBgBlackH4V4(UINT8 *pFont, UINT8 * pDst, UINT32 w, UINT32 h, UINT32 pitch, UINT16 color,UINT32 bGcolor);

void Hal_commDrawNv21Line2V(UINT8* pAddr_y,UINT8* pAddr_vu,UINT32 s,UINT32 hight,UINT8 colorY[8],UINT16 colorVu[4]);

void Hal_commDrawNv21LineAlign8H(UINT8* pAddr_y,UINT8* pAddr_vu,UINT32 width,UINT8 colorY[8],UINT16 colorVu[4]);

void Hal_commDrawNv21LineAlign16H(UINT8* pAddr_y,UINT8* pAddr_vu,UINT32 width,UINT8 colorY[8],UINT16 colorVu[4]);

void Hal_commDrawYLineAlign16H(UINT8* pAddr_y,UINT16 width,UINT8 colorY[8]);

void Hal_commDrawYLineAlign8H(UINT8* pAddr_y,UINT32 width,UINT8 colorY[8]);

void Hal_commNv21ToRgbAlign8(const char* src_y,const char* src_vu, char* rgb_buf,int width,int height,int stried);

void Hal_commDrawNv21Line2VDouble(UINT8* pAddr_y,UINT8* pAddr_vu,UINT32 s,UINT32 w,UINT32 hight,UINT8 colorY[8],UINT16 colorVu[4]);

#endif

