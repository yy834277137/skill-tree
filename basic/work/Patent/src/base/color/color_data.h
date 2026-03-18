/*** 
 * @file   color_data.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  颜色转换查找表头文件
 * @author liwenbin
 * @date   2022-03-02
 * @note   
 * @note History:
 */

#ifndef _COLOR_DATA_H_
#define _COLOR_DATA_H_


/* ========================================================================== */
/*                             数据结构                                       */
/* ========================================================================== */

typedef struct
{
    UINT8 u8Y;
    UINT8 u8U;
    UINT8 u8V;
} COLOR_YUV_S;


/* YUV 2 RGB tab */
extern const short R_Y[256];
extern const short R_V[256];
extern const short G_U[256];
extern const short G_V[256];
extern const short B_U[256];
/* RGB 2 YUV tab */
extern const short Y_R[256];
extern const short Y_G[256];
extern const short Y_B[256];
extern const short U_R[256];
extern const short U_G[256];
extern const short U_B[256];
extern const short V_R[256];
extern const short V_G[256];
extern const short V_B[256];

/* RGB 2 HSV 倒数查找表 */
extern const int div_table[];
/* RGB 2 HSV h倒数查找表 */
extern const int hdiv_table180[];


#endif

