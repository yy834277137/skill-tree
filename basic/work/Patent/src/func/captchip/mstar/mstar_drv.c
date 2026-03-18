
/*******************************************************************************
 * mstar_drv.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : cuifeng5
 * Version: V1.0.0  2021年01月11日 Create
 *
 * Description : mstar操作函数
 * Modification: 
 *******************************************************************************/

#include "sal.h"
#include "platform_hal.h"
#include "../inc/mstar_drv.h"


#define MSTAR_NUM_MAX       (2)

static CAPT_CHIP_FUNC_S g_mstarChipFunc = {0};
static const UART_ID_E g_aenUartMap[MSTAR_NUM_MAX] = {UART_ID_3, UART_ID_4};

/* 枚举对应的字符串名称 */
static const char *g_aszCableName[CAPT_CABLE_BUTT + 1] = {
    [CAPT_CABLE_VGA]  = "VGA",
    [CAPT_CABLE_HDMI] = "HDMI",
    [CAPT_CABLE_DVI]  = "DVI",
    [CAPT_CABLE_BUTT] = "NO INPUT"
};

/* 分辨率过大时需要缩放的比例 */
static const UINT32 g_au32Ratio[][2] = {
    {4, 3},
    {5, 4},
    {16, 9},
};

#define MSTAR_UART_BAUD_DEFAULT         (115200)

#define MSTAR_RESULT_FAIL               (0x00)      /* 返回结果失败 */
#define MSTAR_RESULT_SUCCESS            (0x01)      /* 返回结果成功 */

#define LEAD_CODE_MASTER                (0x5A)      /* 主控设备发送的包引导码 */
#define LEAD_CODE_SLAVE                 (0x5B)      /* 被控设备发送的包引导码 */

#define MSTAR_CMD_RESULT                (0x2F)              /* 返回的结果 */

#define MSTAR_CMD_UART_TEST             (0x00)              /* 串口测试 */
#define MSTAR_CMD_SET_BAUD              (0x30)              /* 设置波特率 */
#define MSTAR_CMD_GET_INPUT_STATUS      (0x40)              /* 获取输入 */
#define MSTAR_CMD_SET_OUPUT_SEL         (0x41)              /* 配置哪个口输出 */
#define MSTAR_CMD_SET_OUPUT_RES         (0x42)              /* 配置输出分辨率 */
#define MSTAR_CMD_GET_OUPUT_RES         (0x43)              /* 获取mstar输出分辨率 */
#define MSTAR_CMD_POS_AUTO_ADJUST       (0x44)              /* 自动调整位置 */
#define MSTAR_CMD_POS_LEFT              (0x45)              /* 图像左移 */
#define MSTAR_CMD_POS_RIGHT             (0x46)              /* 图像右移 */
#define MSTAR_CMD_POS_UP                (0x47)              /* 图像上移 */
#define MSTAR_CMD_POS_DOWN              (0x48)              /* 图像下移 */

#define MSTAR_CMD_SET_CSC               (0x50)              /* 设置图像CSC参数 */
#define MSTAR_CMD_GPIO_READ             (0x60)              /* 读GPIO电平 */
#define MSTAR_CMD_GPIO_WRITE            (0x61)              /* 写GPIO电平 */

#define MSTAR_CMD_RESET                 (0xE0)              /* 软件复位mstar */
#define MSTAR_CMD_HOT_PLUG              (0xE1)              /* 输入热插拔 */
#define MSTAR_CMD_GET_VERSION           (0xE2)              /* 获取版本信息 */
#define MSTAR_CMD_GET_BUILD_TIME        (0xE3)              /* 获取编译时间 */
#define MSTAR_CMD_GET_EDID              (0xE4)              /* 获取EDID */
#define MSTAR_CMD_SET_EDID              (0xE5)              /* 设置EDID */

#define MSTAR_CMD_FIREWARE_UPDATA       (0xF0)              /* 固件升级 */
#define MSTAR_CMD_EDID_UPDATA           (0xF1)
#define MSTAR_CMD_GET_EDID_BLOCK_NUM    (0xF2)
#define MSTAR_CMD_GET_EDID_BLOCK        (0xF3)
#define MSTAR_CMD_SEND_DATA_START       (0xFD)              /* 开始发送数据 */
#define MSTAR_CMD_SEND_DATA_END         (0xFE)              /* 结束发送数据 */
#define MSTAR_CMD_SEND_DATA             (0xFF)              /* 发送数据 */

#define EDID_TOTAL_MAX                  (256)       // EDID最大值
#define EDID_BLOCK_LEN                  (128)

/*******************************************************************************
* 函数名  : mstar_CalCrc
* 描  述  : 计算CRC参数
* 输  入  : UINT8 u8Crc : 已有数据的CRC参数
          UINT8 *pu8Data : 计算CRC的数据
          UINT32 u32Len : 数据长度
* 输  出  : 
* 返回值  : 计算后的CRC参数
*******************************************************************************/
static UINT8 mstar_CalCrc(UINT8 u8Crc, const UINT8 *pu8Data, UINT32 u32Len)
{
    UINT8 u8TmpCrc = u8Crc;

    while (u32Len--)
    {
        u8TmpCrc += *pu8Data++;
    }

    return u8TmpCrc;
}


/*******************************************************************************
* 函数名  : mstar_SendCmd
* 描  述  : 发送命令
* 输  入  : UINT8 u8Cmd : 命令字
          UINT8 *pu8Data : 数据缓存
          UINT32 u32Len : 数据长度
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mstar_SendCmd(UART_ID_E enId, UINT8 u8Cmd, const UINT8 *pu8Data, UINT32 u32Len)
{
    UINT8 au8Buff[65];
    UINT8 *pu8Buff = au8Buff;
    UINT32 u32WriteBytes = 0;
    UINT32 u32LenTmp = u32Len + MSATR_MESSAGE_MIN_LEN;
    
    if (u32Len + 5 > sizeof(au8Buff))
    {
        MSTAR_LOGE("send data len[%u] is bigger than buff[%u]\n", u32Len + 5, (UINT32)sizeof(au8Buff));
        return SAL_FAIL;
    }

    *pu8Buff++ = LEAD_CODE_MASTER;
    *pu8Buff++ = (u32LenTmp >> 8) & 0xff;
    *pu8Buff++ = u32LenTmp & 0xff;
    *pu8Buff++ = u8Cmd;
    
    u32LenTmp  = u32Len;
    while (u32LenTmp--)
    {
        *pu8Buff++ = *pu8Data++;
    }
    
    *pu8Buff = mstar_CalCrc(0, au8Buff, u32Len + MSTAR_MESSAGE_DATA_START);

    /* 引导码，数据长度，命令字，校验码至少4个字节 */
    if ((SAL_SOK != UART_Write(enId, au8Buff, u32Len + MSATR_MESSAGE_MIN_LEN, &u32WriteBytes)) || ((u32Len + MSATR_MESSAGE_MIN_LEN) != u32WriteBytes))
    {
        MSTAR_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, u32Len + 4, u32WriteBytes);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*
* 函数名  : mstar_drvSendData
* 描  述  : 发送数据
* 输  入  : UART_ID_E enId
          UINT32 u32Offset
          UINT8 *pu8Data
          UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*/
static INT32 mstar_drvSendData(UART_ID_E enId, UINT32 u32Offset, const UINT8 *pu8Data, UINT32 u32Len)
{
    UINT8 *pu8Buff = NULL;
    UINT8 *ptmpBuff = NULL;
    UINT32 u32WriteBytes = 0;
    UINT32 u32LenTmp = u32Len + MSATR_MESSAGE_MIN_LEN;


    pu8Buff = (UINT8 *)SAL_memMalloc(u32LenTmp, "mstar_drv", "mstar");
    if(NULL == pu8Buff)
    {
        MSTAR_LOGE("uart[%d] SAL_memMalloc fail, \n", enId);
        return SAL_FAIL;
    }
    ptmpBuff = pu8Buff;

    /*协议头*/
    *ptmpBuff++ = LEAD_CODE_MASTER;
    /*包长度*/
    *ptmpBuff++ = (u32LenTmp >> 8) & 0xff;
    *ptmpBuff++ = u32LenTmp & 0xff;
    /*指令*/
    *ptmpBuff++ = MSTAR_CMD_SET_EDID;
    
    /*数据拷贝*/
    memcpy(ptmpBuff, pu8Data, u32Len);

    /*校验码*/
    ptmpBuff = pu8Buff+ u32LenTmp - MSTAR_MESSAGE_CRC_LEN;
    *ptmpBuff = mstar_CalCrc(0, pu8Buff, u32LenTmp - MSTAR_MESSAGE_CRC_LEN);

    /*发送一包数据*/
    if ((SAL_SOK != UART_Write(enId, pu8Buff, u32LenTmp, &u32WriteBytes)) || (u32LenTmp != u32WriteBytes))
    {
        MSTAR_LOGE("uart[%d] send bytes fail, %u bytes send success\n", enId, u32WriteBytes);
        SAL_memfree(pu8Buff, "mstar_drv", "mstar");
        return SAL_FAIL;
    }

    SAL_memfree(pu8Buff, "mstar_drv", "mstar");

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_RecResult
* 描  述  : 接收串口数据
* 输  入  : UART_ID_E enId : 串口ID
          UINT8 *pu8Data : 接收数据缓存
          UINT32 u32BuffLen : 接收数据缓存大小
          UINT32 *pu32Len : 接收到的有效数据位，将引导码，命令字，数据长度，校验码剥离
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mstar_RecResult(UART_ID_E enId, UINT8 *pu8Data, UINT32 u32BuffLen, UINT32 *pu32Len, UINT32 u32TimeOut)
{
    UINT8 au8Buff[300];
    UINT8 *pu8Buff = au8Buff + MSTAR_MESSAGE_DATA_START;
    UINT32 u32RecLen = 0;
    UINT32 u32ReadByte = 0;
    UINT32 u32ValidLen = 0;
    UINT32 u32DataLen = 5;      // 最少接收到5个字节
    UINT8 u8Cmd = 0;
    UINT8 u8Crc = 0;
    UINT32 i = 0;

    *pu32Len = 0;
    while (u32RecLen < u32DataLen)
    {
        for (i = 0; i < 3; i++)
        {
            u32ReadByte = 0;
            if (SAL_SOK != UART_Read(enId, au8Buff + u32RecLen, sizeof(au8Buff) - u32RecLen, &u32ReadByte, u32TimeOut))
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if (u32ReadByte > 0)
        {
            u32RecLen += u32ReadByte;
        }
        else
        {
            MSTAR_LOGE("uart[%d] read %u times, %u bytes, but need %u bytes\n", enId, i, u32RecLen, u32DataLen);
            return SAL_FAIL;
        }

        if (u32RecLen >= 2)
        {
            if (LEAD_CODE_SLAVE != au8Buff[0])
            {
                MSTAR_LOGE("uart[%d] receive lead code[0x%x] error\n", enId, au8Buff[0]);
                return SAL_FAIL;
            }

            u32DataLen = (((UINT16)au8Buff[1]) << 8) | au8Buff[2];
        }
    }
    
    if ((u32DataLen <= MSATR_MESSAGE_MIN_LEN) || (u32DataLen > u32RecLen))
    {
        MSTAR_LOGE("uart[%d] receive %u bytes, buff len[%u] error\n", enId, u32RecLen, u32DataLen);
        return SAL_FAIL;
    }

    u32ValidLen = u32DataLen - MSATR_MESSAGE_MIN_LEN;
    if (u32ValidLen > u32BuffLen)
    {
        MSTAR_LOGE("uart[%d] receive %u bytes, buff len[%u] is too short\n", enId, u32ValidLen, u32BuffLen);
        return SAL_FAIL;
    }

    *pu32Len = u32ValidLen;

    u8Cmd = au8Buff[MSTAR_MESSAGE_CMD_START];
    if (MSTAR_CMD_RESULT != u8Cmd)
    {
        MSTAR_LOGE("uart[%d] receive cmd code[0x%x] error\n", enId, u8Cmd);
        return SAL_FAIL;
    }

    /* 校验CRC参数 */
    u8Crc = mstar_CalCrc(0, au8Buff, u32DataLen - MSTAR_MESSAGE_CRC_LEN);
    if (u8Crc != au8Buff[u32DataLen - MSTAR_MESSAGE_CRC_LEN])
    {
        MSTAR_LOGE("uart[%d] check crc[0x%x 0x%x] fail\n", enId, u8Crc, au8Buff[u32DataLen - 1]);
        return SAL_FAIL;
    }

    while (u32ValidLen--)
    {
        *pu8Data++ = *pu8Buff++;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_Reset
* 描  述  : mstar复位
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_Reset(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;
    UART_ID_E enId = g_aenUartMap[u32Chn];

    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_RESET, NULL, 0);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send reset cmd fail\n", u32Chn);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_HotPlug
* 描  述  : 对接入口进行热插拔
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_HotPlug(UINT32 u32Chn, CAPT_CABLE_E enCable)
{
    INT32 s32Ret = SAL_SOK;
    UINT8 au8Buff[8];
    UINT32 u32DataLen = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];

    /* 默认拉对应输入的hot plug */
    SAL_UNUSED(enCable);

    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_HOT_PLUG, NULL, 0);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send hot plug cmd fail\n", u32Chn);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (1 != u32DataLen) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
    {
        MSTAR_LOGE("mstar chn[%u] rec set hot hlug cmd fail:rec len[%u]\n", u32Chn, u32DataLen);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_GetVersion
* 描  述  : mstar初始化
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mstar_GetVersion(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[8];
    UINT32 u32DataLen = 0;

    (VOID)UART_Flush(enId);

    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_GET_VERSION, NULL, 0);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send get version cmd fail\n", u32Chn);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret))
    {
        MSTAR_LOGE("mstar chn[%u] rec get version cmd fail:rec len[%u]\n", u32Chn, u32DataLen);
        return SAL_FAIL;
    }

    MSTAR_LOGW("mstar chn[%u] firmware version:V%s\n", u32Chn, au8Buff);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_GetBuildTime
* 描  述  : 获取mstar固件编译时间
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mstar_GetBuildTime(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[64];
    UINT32 u32DataLen = 0;

    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_GET_BUILD_TIME, NULL, 0);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send get build time cmd fail\n", u32Chn);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (0 == u32DataLen))
    {
        MSTAR_LOGE("mstar chn[%u] rec get build cmd cmd fail:rec len[%u]\n", u32Chn, u32DataLen);
        return SAL_FAIL;
    }

    MSTAR_LOGW("mstar chn[%u] firmware build time:%s\n", u32Chn, au8Buff);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_SetBaudrate
* 描  述  : 获取波特率
* 输  入  : UINT32 u32Chn
          UINT32 u32Baud
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_SetBaudrate(UINT32 u32Chn, UINT32 u32Baud)
{
    INT32 s32Ret = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[8];
    UINT32 u32DataLen = 0;

    au8Buff[0] = (u32Baud >> 24) & 0xFF;
    au8Buff[1] = (u32Baud >> 16) & 0xFF;
    au8Buff[2] = (u32Baud >> 8) & 0xFF;
    au8Buff[3] = u32Baud & 0xFF;
    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_SET_BAUD, au8Buff, 4);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("chn[%u] send set baudrate[%u] cmd fail\n", u32Chn, u32Baud);
        return SAL_FAIL;
    }
    
    s32Ret = UART_SetBaud(enId, u32Baud);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("uart[%d] set baudrate[%u] fail\n", enId, u32Baud);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (1 != u32DataLen) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
    {
        MSTAR_LOGE("chn[%u] rec set baudrate[%u] fail cmd fail:rec len[%u]\n", u32Chn, u32Baud, u32DataLen);
        return SAL_FAIL;
    }

    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : mstar_GetInputStatus
* 描  述  : 获取输入口的状态
* 输  入  : UINT32 u32Chn
          CAPT_CABLE_E enCable
          CAPT_STATUS_S *pstStatus
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_GetInputStatus(UINT32 u32Chn, CAPT_CABLE_E enCable, CAPT_STATUS_S *pstStatus)
{
    INT32 s32Ret = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[8];
    UINT32 u32DataLen = 0;

    au8Buff[0] = enCable;
    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_GET_INPUT_STATUS, au8Buff, 1);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("chn[%u] send get %s status cmd fail\n", u32Chn, g_aszCableName[enCable]);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (6 != u32DataLen))
    {
        MSTAR_LOGE("chn[%u] rec get %s status cmd fail:rec len[%u]\n", u32Chn, g_aszCableName[enCable], u32DataLen);
        return SAL_FAIL;
    }

    pstStatus->enCable         = au8Buff[0];
    pstStatus->stRes.u32Width  = (((UINT32)au8Buff[1]) << 8) | au8Buff[2];
    pstStatus->stRes.u32Height = (((UINT32)au8Buff[3]) << 8) | au8Buff[4];
    pstStatus->stRes.u32Fps    = au8Buff[5];

    if (pstStatus->enCable >= CAPT_CABLE_BUTT)
    {
        MSTAR_LOGE("chn[%u] received invalid cable[%d]\n", u32Chn, pstStatus->enCable);
        return SAL_FAIL;
    }

    if (enCable != pstStatus->enCable)
    {
        MSTAR_LOGE("chn[%u] send cable[%s] not match received[%s]\n", u32Chn, g_aszCableName[enCable], g_aszCableName[pstStatus->enCable]);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_SetOutputSel
* 描  述  : 选择对应的输入口绑定到输出
* 输  入  : UINT32 u32Chn
          CAPT_CABLE_E enCable
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_SetOutputSel(UINT32 u32Chn, CAPT_CABLE_E enCable)
{
    INT32 s32Ret = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[8];
    UINT32 u32DataLen = 0;

    if (enCable > CAPT_CABLE_BUTT)
    {
        enCable = CAPT_CABLE_BUTT;          // auto模式
    }

    au8Buff[0] = enCable;
    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_SET_OUPUT_SEL, au8Buff, 1);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send set output sel[%s] cmd fail\n", u32Chn, g_aszCableName[enCable]);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (1 != u32DataLen) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
    {
        MSTAR_LOGE("mstar chn[%u] rec set output sel[%s] cmd fail:rec len[%u]\n", u32Chn, g_aszCableName[enCable], u32DataLen);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_SetOutputRes
* 描  述  : 设置输出分辨率
* 输  入  : UINT32 u32Chn
          CAPT_RESOLUTION_S *pstRes
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_SetOutputRes(UINT32 u32Chn, CAPT_RESOLUTION_S *pstRes)
{
    INT32 s32Ret = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[8];
    UINT32 u32DataLen = 0;

    au8Buff[0] = (pstRes->u32Width >> 8) & 0xFF;
    au8Buff[1] = pstRes->u32Width & 0xFF;
    au8Buff[2] = (pstRes->u32Height >> 8) & 0xFF;
    au8Buff[3] = pstRes->u32Height & 0xFF;
    au8Buff[4] = pstRes->u32Fps;
    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_SET_OUPUT_RES, au8Buff, 5);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send set output res[%uX%uP%u] cmd fail\n",
                   u32Chn, pstRes->u32Width, pstRes->u32Height, pstRes->u32Fps);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (1 != u32DataLen) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
    {
        MSTAR_LOGE("mstar chn[%u] rec set output res[%uX%uP%u] cmd fail:rec len[%u]\n",
                   u32Chn, pstRes->u32Width, pstRes->u32Height, pstRes->u32Fps, u32DataLen);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_GetOutputRes
* 描  述  : 获取输出分辨率
* 输  入  : UINT32 u32Chn
          CAPT_RESOLUTION_S *pstRes
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mstar_GetOutputRes(UINT32 u32Chn, CAPT_CABLE_E *penCable, CAPT_RESOLUTION_S *pstRes)
{
    INT32 s32Ret = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[8];
    UINT32 u32DataLen = 0;
    
    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_GET_OUPUT_RES, NULL, 0);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send get output res cmd fail\n", u32Chn);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (6 != u32DataLen))    /* 接收到的数据量参考串口协议 */
    {
        MSTAR_LOGE("mstar chn[%u] rec get output res cmd fail:rec len[%u]\n", u32Chn, u32DataLen);
        return SAL_FAIL;
    }

    *penCable = au8Buff[0];
    pstRes->u32Width  = (((UINT32)au8Buff[1]) << 8) | au8Buff[2];
    pstRes->u32Height = (((UINT32)au8Buff[3]) << 8) | au8Buff[4];
    pstRes->u32Fps    = au8Buff[5];

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_AutoAdjustPos
* 描  述  : 自动调整位置
* 输  入  : UINT32 u32Chn
          CAPT_CABLE_E enCable
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_AutoAdjustPos(UINT32 u32Chn, CAPT_CABLE_E enCable)
{
    INT32 s32Ret = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[8];
    UINT32 u32DataLen = 0;

    au8Buff[0] = enCable;
    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_POS_AUTO_ADJUST, au8Buff, 1);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send set %s pos auto adjust cmd fail\n", u32Chn, g_aszCableName[enCable]);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 100 * 1000);
    if ((SAL_SOK != s32Ret) || (1 != u32DataLen) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
    {
        MSTAR_LOGE("mstar chn[%u] rec set %s pos auto adjust cmd fail:rec len[%u]\n", u32Chn, g_aszCableName[enCable], u32DataLen);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_AdjustPos
* 描  述  : 调整位置
* 输  入  : UINT32 u32Chn
          CAPT_CABLE_E enCable
          UINT32 u32Pixel
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_AdjustPos(UINT32 u32Chn, CAPT_CABLE_E enCable, CAPT_POS_E enPos, UINT32 u32Pixel)
{
    INT32 s32Ret = SAL_SOK;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[8];
    UINT32 u32DataLen = 0;
    static const UINT8 au8Cmd[CAPT_POS_BUTT] = {
        [CAPT_POS_UP]    = MSTAR_CMD_POS_UP,
        [CAPT_POS_DOWN]  = MSTAR_CMD_POS_DOWN,
        [CAPT_POS_LEFT]  = MSTAR_CMD_POS_LEFT,
        [CAPT_POS_RIGHT] = MSTAR_CMD_POS_RIGHT,
    };
    static const char *aszName[CAPT_POS_BUTT] = {
        [CAPT_POS_UP]    = "up",
        [CAPT_POS_DOWN]  = "down",
        [CAPT_POS_LEFT]  = "left",
        [CAPT_POS_RIGHT] = "right",
    };

    au8Buff[0] = enCable;
    au8Buff[1] = (u32Pixel >> 8) & 0xFF;
    au8Buff[2] = u32Pixel & 0xFF;
    s32Ret = mstar_SendCmd(enId, au8Cmd[enPos], au8Buff, 3);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send set pos[%s] %u pixels cmd fail\n", u32Chn, aszName[enPos], u32Pixel);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (1 != u32DataLen) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
    {
        MSTAR_LOGE("mstar chn[%u] rec set pos[%s] %u pixels cmd fail:rec len[%u]\n", u32Chn, aszName[enPos], u32Pixel, u32DataLen);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_SetCsc
* 描  述  : 设置CSC参数
* 输  入  : UINT32 u32Chn
          VIDEO_CSC_S *pstCsc
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_SetCsc(UINT32 u32Chn, VIDEO_CSC_S *pstCsc)
{
    INT32 s32Ret = 0;
    UINT32 u32Len = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 *pu8Data = NULL;
    UINT8 au8Buff[8];

    VIDEO_CSC_PARAM_S astCscParam[] = {
          {VIDEO_CSC_LUMA,      pstCsc->uiLuma},
          {VIDEO_CSC_CONTRAST,  pstCsc->uiContrast},
          {VIDEO_CSC_HUE,       pstCsc->uiHue},
          {VIDEO_CSC_SATUATURE, pstCsc->uiSatuature},
      };

    pu8Data = (UINT8 *)astCscParam;
    u32Len = sizeof(astCscParam)/sizeof(astCscParam[0]);
    
    s32Ret = mstar_drvSendData(enId, 0, pu8Data,  u32Len);
    if (SAL_SOK != s32Ret)
    {
         MSTAR_LOGE("mstar chn[%u] \n", u32Chn);
         return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32Len, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (1 != u32Len) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
    {
        MSTAR_LOGE("mstar chn[%u] rec  cmd fail:rec len[%u]\n", u32Chn, u32Len);
        return SAL_FAIL;
    }
       

    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : mstar_SetEdid
* 描  述  : 设置EDID
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mstar_SetEdid(UINT32 u32Chn, VIDEO_EDID_BUFF_S *pstEdidBuff, CAPT_CABLE_E enCable)
{
    INT32 s32Ret = SAL_SOK;
    INT32 s32Result = SAL_SOK;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[16];
    UINT32 u32DataLen = 0;
    UINT32 u32Len = pstEdidBuff->u32Len;
    UINT8 *pu8Buff = pstEdidBuff->au8Buff;
    UINT32 i = 0;
    UINT8 au8TmpBuff[300];

    /* 当前最多256个字节，一个block为128字节 */
    if ((u32Len > 0x100) || (0 == u32Len) || (0 != u32Len % 128))
    {
        MSTAR_LOGE("mstar chn[%u] invalid edid len[%u]\n", u32Chn, u32Len);
        return SAL_FAIL;
    }

    (void)UART_Flush(enId);

    /* 数据第一位表示VGA/HDMI/DVI类型 edid最大256*/
    au8TmpBuff[0] = enCable;
    sal_memcpy_s(au8TmpBuff+1, EDID_TOTAL_MAX, pu8Buff, u32Len);
    for (i = 0; i < 3; i++)
    {
        s32Ret = mstar_drvSendData(enId, 0, au8TmpBuff, u32Len+1);
        if (SAL_SOK != s32Ret)
        {
            MSTAR_LOGW("mstar chn[%u] send edid data[%u] fail\n", u32Chn, u32Len);
            continue;
        }

        s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
        if ((SAL_SOK != s32Ret) || u32DataLen != 1)
        {
            MSTAR_LOGW("mstar chn[%u] rec send edid data cmd fail:rec len[%u] au8Buff = 0x%x\n", u32Chn, u32DataLen, au8Buff[0]);
            continue;
        }

        /* 发送成功 */
        MSTAR_LOGI("mstar chn[%u] send edid[%u] metadata success\n", u32Chn, u32Len);
        break;
    }

    /* 3次均失败 */
    if (i >= 3)
    {
        MSTAR_LOGW("mstar chn[%u] send edid data cmd fail\n", u32Chn);
        s32Result = SAL_FAIL;
    }



    return s32Result;
}


/*******************************************************************************
* 函数名  : mstar_GetEdidFromRom
* 描  述  : 获取EDID
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
UINT32 mstar_GetEdidFromRom(UINT32 u32Chn, UINT8 *pu8Buff, UINT32 u32BuffLen, CAPT_CABLE_E enCable)
{
    INT32 s32Ret = SAL_SOK;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[300];
    UINT32 u32DataLen = 0;
    UINT32 u32EdidLen = 0;

    (void)UART_Flush(enId);

    au8Buff[0] = enCable;
    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_GET_EDID, au8Buff, 1);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send get edid block num cmd fail\n", u32Chn);
        return 0;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32EdidLen, 5 * 1000);
    if ((SAL_SOK != s32Ret))
    {
        MSTAR_LOGE("mstar chn[%u] rec get edid block num cmd fail:rec len[%u], block num[%u]\n", u32Chn, u32DataLen, au8Buff[0]);
        return 0;
    }
    
    /*去掉数据第一位接口类型*/
    sal_memcpy_s(pu8Buff, 300, au8Buff+1, u32EdidLen-1);

    MSTAR_LOGI("mstar chn[%u] get edid len:%u success\n", u32Chn, u32EdidLen);
    return u32EdidLen;
}


/*******************************************************************************
* 函数名  : mstar_CheckEdid
* 描  述  : mstar升级的时候可能会把edid清掉，所以，初始化检查EDID
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static UINT32 mstar_CheckEdid(UINT32 u32Chn, CAPT_CABLE_E enCable)
{
    UINT8 au8Buff[512];
    UINT32 u32Len = 0;
    VIDEO_EDID_BUFF_S *pstEdidBuff = NULL;
    UINT32 i = 0;

    if (0 == (u32Len = mstar_GetEdidFromRom(u32Chn, au8Buff, sizeof(au8Buff)/sizeof(au8Buff[0]), enCable)))
    {
        MSTAR_LOGE("mstar chn[%u] get cable[%d] edid from rom fail\n", u32Chn, enCable);
        return SAL_FAIL;
    }

    if (NULL == (pstEdidBuff = EDID_GetEdid(u32Chn, enCable)))
    {
        MSTAR_LOGE("mstar chn[%u] get cable[%d] edid from file fail\n", u32Chn, enCable);
        return SAL_FAIL;
    }

    /* 只校验一个block就可以 */
    for (i = 0; i < EDID_BLOCK_LEN; i++)
    {
        if (au8Buff[i] != pstEdidBuff->au8Buff[i])
        {
            MSTAR_LOGW("mstar chn[%u] will update cable[%d] edid from file\n", u32Chn, enCable);
            if (SAL_SOK != mstar_SetEdid(u32Chn, pstEdidBuff, enCable))
            {
                MSTAR_LOGE("mstar chn[%u] set cable[%d] edid fail\n", u32Chn, enCable);
                return SAL_FAIL;
            }
            return SAL_SOK;
        }
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mstar_FirmwareUpdate
* 描  述  : 固件升级
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_FirmwareUpdate(UINT32 u32Chn, const UINT8 *pu8Buff, UINT32 u32Len)
{
    INT32 s32Ret = SAL_SOK;
    INT32 s32Result = SAL_SOK;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[16];
    UINT32 u32DataLen = 0;
    UINT32 i = 0;
    UINT32 u32SendBytes = 0;
    UINT32 u32Bytes = 0;

    au8Buff[0] = (u32Len >> 24) & 0xFF;
    au8Buff[1] = (u32Len >> 16) & 0xFF;
    au8Buff[2] = (u32Len >> 8) & 0xFF;
    au8Buff[3] = u32Len & 0xFF;
    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_SEND_DATA_START, au8Buff, 4);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send start update firmware cmd fail\n", u32Chn);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (1 != u32DataLen) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
    {
        MSTAR_LOGE("mstar chn[%u] rec start update firmware cmd fail:rec len[%u]\n", u32Chn, u32DataLen);
        s32Result = SAL_FAIL;
        goto end;
    }

    while (u32SendBytes < u32Len)
    {
        u32Bytes = (u32Len - u32SendBytes) > 1024 ? 1024 : (u32Len - u32SendBytes);

        /* 最多发送3次，如果3次都失败，结束本次edid升级 */
        for (i = 0; i < 3; i++)
        {
            s32Ret = mstar_drvSendData(enId, u32SendBytes, pu8Buff + u32SendBytes, u32Bytes);
            if (SAL_SOK != s32Ret)
            {
                MSTAR_LOGW("mstar chn[%u] send firmware data[%u] fail\n", u32Chn, u32Bytes);
                continue;
            }

            s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
            if ((SAL_SOK != s32Ret) || (1 != u32DataLen) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
            {
                MSTAR_LOGW("mstar chn[%u] rec send firmware data cmd fail:rec len[%u]\n", u32Chn, u32DataLen);
                continue;
            }
            
            break;
        }

        /* 3次均失败 */
        if (i >= 3)
        {
            MSTAR_LOGW("mstar chn[%u] send firmware data cmd fail\n", u32Chn);
            s32Result = SAL_FAIL;
            goto end;
        }
        else
        {
            u32SendBytes += u32Bytes;
            MSTAR_LOGW("mstar chn[%u] tranfer firmware %u bytes\n", u32Chn, u32SendBytes);
        }
    }

    MSTAR_LOGW("mstar chn[%u] tranfer firmware %u bytes complete!!!!!\n", u32Chn, u32SendBytes);

end:
    s32Ret = mstar_SendCmd(enId, MSTAR_CMD_SEND_DATA_END, NULL, 0);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] send end update firmware cmd fail\n", u32Chn);
        return SAL_FAIL;
    }

    s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 5 * 1000);
    if ((SAL_SOK != s32Ret) || (1 != u32DataLen) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
    {
        MSTAR_LOGE("mstar chn[%u] rec end update firmware cmd fail:rec len[%u]\n", u32Chn, u32DataLen);
        return SAL_FAIL;
    }


    if (SAL_SOK == s32Result)
    {
        s32Ret = mstar_SendCmd(enId, MSTAR_CMD_FIREWARE_UPDATA, NULL, 0);
        if (SAL_SOK != s32Ret)
        {
            MSTAR_LOGE("mstar chn[%u] send end update firmware cmd fail\n", u32Chn);
            return SAL_FAIL;
        }

        s32Ret = mstar_RecResult(enId, au8Buff, sizeof(au8Buff), &u32DataLen, 50 * 1000);
        if ((SAL_SOK != s32Ret) || (1 != u32DataLen) || (MSTAR_RESULT_SUCCESS != au8Buff[0]))
        {
            MSTAR_LOGE("mstar chn[%u] rec end update firmware cmd fail:rec len[%u]\n", u32Chn, u32DataLen);
            return SAL_FAIL;
        }

        /* 前后各获取一次固件编译时间，用于对比是否升级成功 */
        (VOID)mstar_GetBuildTime(u32Chn);

        MSTAR_LOGW("mstar chn[%u] update success, will reboot...\n", u32Chn);
        
        s32Ret = mstar_Reset(u32Chn);
        if (SAL_SOK != s32Ret)
        {
            MSTAR_LOGE("mstar chn[%u] reset fail\n", u32Chn);
            return SAL_FAIL;
        }

        /* mstar重启时间 */
        SAL_msleep(10 * 1000);
        
        (VOID)mstar_GetVersion(u32Chn);
        (VOID)mstar_GetBuildTime(u32Chn);
    }
    
    
    return s32Result;
}


/*******************************************************************************
* 函数名  : mstar_Detect
* 描  述  : 检测输入并配置
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_Detect(UINT32 u32Chn, CAPT_STATUS_S *pstStatus)
{
    INT32 s32Ret = SAL_SOK;
    CAPT_STATUS_S stStatus;
    CAPT_CABLE_E enCable = CAPT_CABLE_HDMI;

    if ((u32Chn >= MSTAR_NUM_MAX) || (NULL == pstStatus))
    {
        MSTAR_LOGE("mstar chn[%u] invalid param:%p\n", u32Chn, pstStatus);
        return SAL_FAIL;
    }

    memset(pstStatus, 0, sizeof(*pstStatus));
    pstStatus->enCable = CAPT_CABLE_BUTT;

    s32Ret = mstar_GetOutputRes(u32Chn, &enCable, &stStatus.stRes);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] get output resolution fail\n", u32Chn);
        return SAL_FAIL;
    }

    MSTAR_LOGD("mstar chn[%u] get output status:cable[%d] res[%uX%uP%u], cable[%d] video[%d]\n",
                u32Chn, stStatus.enCable, stStatus.stRes.u32Width, stStatus.stRes.u32Height,
                stStatus.stRes.u32Fps, stStatus.bCableConnected, stStatus.bVideoDetected);
    
#if 0
    stStatus.stRes.u32Width  = 1920; 
    stStatus.stRes.u32Height = 1080;
#endif

    stStatus.enCable = enCable;
    if (stStatus.enCable >= CAPT_CABLE_BUTT)
    {
        MSTAR_LOGW("mstar chn[%u] no input\n", u32Chn);
        return SAL_SOK;
    }

    stStatus.bVideoDetected  = SAL_TRUE;
    sal_memcpy_s(pstStatus, sizeof(*pstStatus), &stStatus, sizeof(stStatus));

    /* 四对齐方便FPGA抠图 */
    pstStatus->stRes.u32Width  &= 0xFFFFFFFC;
    pstStatus->stRes.u32Height &= 0xFFFFFFFC;
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mstar_Scale
* 描  述  : mstar输入分辨率大于面板分辨率时，会对输入分辨率进行缩放，计算缩放后的分辨率
* 输  入  : CAPT_RESOLUTION_S *pstSrc
* 输  出  : CAPT_RESOLUTION_S *pstDst
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_Scale(CAPT_RESOLUTION_S *pstDst, CAPT_RESOLUTION_S *pstSrc)
{
    UINT32 u32Num = sizeof(g_au32Ratio)/sizeof(g_au32Ratio[0]);
    UINT32 i = 0;

    if ((NULL == pstDst) || (NULL == pstSrc))
    {
        MSTAR_LOGE("invalid pointer:src[%p], dst[%p]\n", pstSrc, pstDst);
        return SAL_FAIL;
    }

    pstDst->u32Fps = pstSrc->u32Fps;

    if (pstSrc->u32Width > MSTAR_WIDTH_MAX || pstSrc->u32Height > MSTAR_HEIGHT_MAX)
    {
        for (i = 0; i < u32Num; i++)
        {
            /* 对比是否有支持的比例 */
            if (pstSrc->u32Width * g_au32Ratio[i][1] == pstSrc->u32Height * g_au32Ratio[i][0])
            {
                /* mstar输出的宽高比大于相应比例 */
                if (MSTAR_WIDTH_MAX * g_au32Ratio[i][1] > MSTAR_HEIGHT_MAX * g_au32Ratio[i][0])
                {
                    pstDst->u32Height = MSTAR_HEIGHT_MAX;
                    pstDst->u32Width  = pstDst->u32Height * g_au32Ratio[i][0] / g_au32Ratio[i][1];
                }
                else
                {
                    pstDst->u32Width  = MSTAR_WIDTH_MAX;
                    pstDst->u32Height = pstDst->u32Width * g_au32Ratio[i][1] / g_au32Ratio[i][0];
                }

                /* 四对齐，方便FPGA抠图 */
                pstDst->u32Width  &= 0xFFFFFFFC;
                pstDst->u32Height &= 0xFFFFFFFC;

                return SAL_SOK;
            }
        }

        pstDst->u32Width  = MSTAR_WIDTH_MAX;
        pstDst->u32Height = MSTAR_HEIGHT_MAX;

        return SAL_SOK;
    }

    pstDst->u32Width  = pstSrc->u32Width;
    pstDst->u32Height = pstSrc->u32Height;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mstar_Init
* 描  述  : mstar初始化
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mstar_Init(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UART_ATTR_S stUartAttr;
    UINT32 u32Times = 3;

    stUartAttr.u32Baud       = MSTAR_UART_BAUD_DEFAULT;
    stUartAttr.u32DataBits   = 8;
    stUartAttr.enStopBits    = UART_STOP_BITS_1;
    stUartAttr.enPolarity    = UART_POLARITY_NONE;
    stUartAttr.enFlowControl = UART_FLOWCONTROL_NONE;
    stUartAttr.u32MinBytes   = 256;
    stUartAttr.u32TimeOut    = 500;

    s32Ret = UART_Init(enId, &stUartAttr);
    if (SAL_SOK != s32Ret)
    {
        MSTAR_LOGE("mstar chn[%u] uart[%d] init fail\n", u32Chn, enId);
        return SAL_FAIL;
    }

    while (u32Times--)
    {
        if (SAL_SOK == mstar_GetVersion(u32Chn))
        {
            break;
        }
    }


    (VOID)mstar_GetBuildTime(u32Chn);
    (VOID)mstar_CheckEdid(u32Chn, CAPT_CABLE_VGA);
    (VOID)mstar_CheckEdid(u32Chn, CAPT_CABLE_HDMI);
    (VOID)mstar_CheckEdid(u32Chn, CAPT_CABLE_DVI);
    

    MSTAR_LOGI("mstar chn[%u] init success\n", u32Chn);

    return SAL_SOK;
}


CAPT_CHIP_FUNC_S *mstar_chipRegister(void)
{

    memset(&g_mstarChipFunc, 0, sizeof(CAPT_CHIP_FUNC_S));
    g_mstarChipFunc.pfuncInit       = mstar_Init;
    g_mstarChipFunc.pfuncHotPlug    = mstar_HotPlug;
    g_mstarChipFunc.pfuncReset      = NULL;
    g_mstarChipFunc.pfuncDetect     = mstar_Detect;
    g_mstarChipFunc.pfuncSetRes     = mstar_SetOutputRes;
    g_mstarChipFunc.pfuncSetCsc     = mstar_SetCsc;
    g_mstarChipFunc.pfuncAutoAdjust = mstar_AutoAdjustPos;
    g_mstarChipFunc.pfuncGetEdid    = mstar_GetEdidFromRom;
    g_mstarChipFunc.pfuncSetEdid    = mstar_SetEdid;
    g_mstarChipFunc.pfuncScale      = mstar_Scale;
    g_mstarChipFunc.pfuncUpdate     = mstar_FirmwareUpdate;

    return &g_mstarChipFunc;
}



