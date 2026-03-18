
#ifndef __UART_H_
#define __UART_H_

/* UART ID，个数由硬件决定 */
typedef enum
{
    UART_ID_0,
    UART_ID_1,
    UART_ID_2,
    UART_ID_3,
    UART_ID_4,
    UART_ID_5,
    UART_ID_6,
    UART_ID_7,
    UART_ID_8,
    UART_ID_9,
    UART_ID_BUTT,
} UART_ID_E;

/* 停止位 */
typedef enum
{
    UART_STOP_BITS_1,
    UART_STOP_BITS_1_5,
    UART_STOP_BITS_2,
    UART_STOP_BITS_BUTT,
} UART_STOP_BITS_E;

/* 校验位 */
typedef enum
{
    UART_POLARITY_ODD,          /* 奇校验 */
    UART_POLARITY_EVEN,         /* 偶校验 */
    UART_POLARITY_NONE,         /* 无校验 */
    UART_POLARITY_BUTT,
} UART_POLARITY_E;

/* 流控 */
typedef enum
{
    UART_FLOWCONTROL_NONE,
    UART_FLOWCONTROL_XON_XOFF,
    UART_FLOWCONTROL_RTS_CTS,
    UART_FLOWCONTROL_BUTT,
} UART_FLOWCONTROL_E;

/* 串口属性 */
typedef struct
{
    UINT32 u32Baud;
    UINT32 u32DataBits;
    UART_STOP_BITS_E enStopBits;
    UART_POLARITY_E enPolarity;
    UART_FLOWCONTROL_E enFlowControl;
    UINT32 u32MinBytes;         // 对应c_cc[MIN]
    UINT32 u32TimeOut;          // 对应c_cc[VTIME]
} UART_ATTR_S;

INT32 UART_SetBaud(UART_ID_E enId, UINT32 u32Baud);
INT32 UART_SetDataBits(UART_ID_E enId, UINT32 u32DataBits);
INT32 UART_SetStopBits(UART_ID_E enId, UART_STOP_BITS_E enStopBits);
INT32 UART_SetPolarity(UART_ID_E enId, UART_POLARITY_E enPolarity);
INT32 UART_SetFlowControl(UART_ID_E enId, UART_FLOWCONTROL_E enFlowControl);
INT32 UART_SetRawMode(UART_ID_E enId);
INT32 UART_SetOverTime(UART_ID_E enId, UINT32 u32MinBytes, UINT32 u32OverTime);
INT32 UART_Init(UART_ID_E enId, UART_ATTR_S *pstAttr);
INT32 UART_Flush(UART_ID_E enId);
INT32 UART_Read(UART_ID_E enId, UINT8 *pu8Buff, UINT32 u32BufLen, UINT32 *pu32ReadBytes, UINT32 u32TimeOut);
INT32 UART_Write(UART_ID_E enId, const UINT8 *pu8Buff, UINT32 u32BufLen, UINT32 *pu32WriteBytes);

#endif


