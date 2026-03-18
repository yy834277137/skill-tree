
#include "system_common_api.h"
#include "dspcommon.h"
#include "xpack_drv.h"

/*****************************************************************************
                                全局结构体
*****************************************************************************/

/*****************************************************************************
                                函数
*****************************************************************************/

/*****************************************************************************
   函 数 名  : CmdProc_vdaCmdProc
   功能描述  : DSP 模块vda的处理命令
   输入参数  : 无
   输出参数  : 无
   返 回 值      :成功SAL_SOK
                            失败SAL_FALSE
*****************************************************************************/
INT32 CmdProc_XpackCmdProc(HOST_CMD cmd, UINT32 chan, UINT32 *pParam, VOID *pBuf)
{
    int iRet = SAL_SOK;

    switch (cmd)
    {
    case HOST_CMD_XPACK_SAVE:
        if (SAL_SOK != (iRet = Xpack_DrvSaveScreen(chan, (XPACK_SAVE_PRM *)pBuf)))
        {
            SAL_ERROR("xpack chan %d, Xpack_DrvSaveScreen Failed !!!\n", chan);
        }
        break;

    case HOST_CMD_XPACK_SET_JPG:
        if (SAL_SOK != (iRet = Xpack_DrvSetPackJpgCbPrm(chan, (XPACK_JPG_SET_ST *)pBuf)))
        {
            SAL_ERROR("Chn %d Xpack_tskSetJpg Failed !!!\n", chan);
        }
        break;

    case HOST_CMD_XPACK_SET_YUVSHOW_OFFSET:
        if (SAL_SOK != (iRet = Xpack_DrvSetDispVertOffset(chan, (XPACK_YUVSHOW_OFFSET *)pBuf)))
        {
            SAL_ERROR("Chn %d Xpack_DrvSetDispVertOffset Failed !!!\n", chan);
        }
        break;

    case HOST_CMD_XPACK_SET_SEGMENT:
        if (SAL_SOK != (iRet = Xpack_DrvSetSegCbPrm(chan, (XPACK_DSP_SEGMENT_SET *)pBuf)))
        {
            SAL_ERROR("Chn %d Xpack_DrvSetSegCbPrm Failed !!!\n", chan);
        }
        break;

    case HOST_CMD_SET_DISP_SYNC_TIME:
        if (SAL_SOK != (iRet = Xpack_DrvSetDispSync(*(UINT32 *)pParam)))
        {
            SAL_ERROR("Xpack_DrvSetDispSync Failed !!!\n");
        }
        break;

    default:
        SAL_ERROR("MODULE_VENC: <0x%x> is nonsupport !!!\n", cmd);
        break;
    }

    return iRet;
}

