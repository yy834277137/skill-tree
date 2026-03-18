/**
 * @file   hardware_hal.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  获取硬件相关参数
 * @author cuifeng5
 * @date   2021/1/18
 * @note
 * @note \n History
   1.日    期: 2021/1/18
     作    者: cuifeng5
     修改历史: 创建文件
 */

#include "platform_hal.h"
#include "sal.h"

#define DDM_PCP "/dev/DDM/pcp"

typedef struct
{
    HARDWARE_INPUT_CHIP_E enCaptChip;
    const UINT32 *pu32Board;
} BOARD_CAPT_CHIP_MAP_S;

/* cpld信息 */
static cpld_info g_stCpldInfo = {0};

/*******************************************************************************
* 函数名  : HARDWARE_IsCpldInvalid
* 描  述  : 检查cpld信息是否有效
* 输  入  : cpld_info *pstCpld
* 输  出  :
* 返回值  : SAL_TRUE : 有效
*         SAL_FALSE : 无效
*******************************************************************************/
static BOOL HARDWARE_IsCpldInvalid(cpld_info *pstCpld)
{
    return ((CPLD_MAGIC == g_stCpldInfo.magic) || (DLPC_MAGIC == g_stCpldInfo.magic)) ? SAL_TRUE : SAL_FALSE;
}

/*******************************************************************************
* 函数名  : HARDWARE_GetCpldInfo
* 描  述  : 获取cpld信息
* 输  入  :
* 输  出  :
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 HARDWARE_GetCpldInfo(VOID)
{
    int ret = -1;
    int fd = -1;

    fd = open(DDM_PCP, O_RDWR);
    if (fd < 0)
    {
        HARDWARE_LOGE("open %s fail:%s\n", DDM_PCP, strerror(errno));
        return SAL_FAIL;
    }

    ret = ioctl(fd, DDM_IOC_GET_HARDWRE_INFO, &g_stCpldInfo);
    close(fd);
    if (ret < 0)
    {
        HARDWARE_LOGE("get hardware failed: %s\n", strerror(errno));
        return SAL_FAIL;
    }

    if (SAL_FALSE == HARDWARE_IsCpldInvalid(&g_stCpldInfo))
    {
        HARDWARE_LOGE("check cpld magic[0x%x] failed\n", g_stCpldInfo.magic);
        return SAL_FAIL;
    }

    HARDWARE_LOGI("get cpld info success:board[0x%x] pcb[0x%x]\n", g_stCpldInfo.board, g_stCpldInfo.pcb);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : HARDWARE_GetBoardType
* 描  述  : 获取主板信息
* 输  入  :
* 输  出  :
* 返回值  : 0 : 失败
*         非0 : 成功
*******************************************************************************/
UINT32 HARDWARE_GetBoardType(VOID)
{
    INT32 s32Ret = SAL_SOK;
    static UINT32 u32Board = 0;

    if (0 != u32Board)
    {
        return u32Board;
    }

    if (SAL_FALSE == HARDWARE_IsCpldInvalid(&g_stCpldInfo))
    {
        s32Ret = HARDWARE_GetCpldInfo();
        if (SAL_SOK != s32Ret)
        {
            HARDWARE_LOGE("get cpld info fail\n");
            return SAL_FAIL;
        }
    }

    u32Board = g_stCpldInfo.board;
    return u32Board;
}

/*******************************************************************************
* 函数名  : HARDWARE_GetInputChip
* 描  述  : 获取输入芯片
* 输  入  :
* 输  出  :
* 返回值  : HARDWARE_INPUT_CHIP_E
*******************************************************************************/
inline HARDWARE_INPUT_CHIP_E HARDWARE_GetInputChip(VOID)
{

    UINT32 u32Board = 0;
    UINT32 i = 0;
    const BOARD_CAPT_CHIP_MAP_S *pstMap = NULL;
    const UINT32 *pu32Board = NULL;
    static HARDWARE_INPUT_CHIP_E enChip = HARDWARE_INPUT_CHIP_BUTT;
    static const UINT32 au32MstarType[] = {DB_RS20011_V1_0, DB_INVALID, };

    static const UINT32 au32AdvType[] = {DB_DS50018_V1_0, DB_DS50019_V1_0, DB_DS50018_V2_0, DB_RS20007_V1_0, DB_RS20007_A_V1_0,
                                         DB_RS20001_V1_0, DB_RS20001_V1_1, DB_RS20011_V1_0, DB_INVALID, };

    static const UINT32 au32McuMstarType[] = {DB_RS20016_V1_0, DB_TS3637_V1_0, DB_RS20016_V1_1, DB_RS20016_V1_1_F303, DB_INVALID, };

    static const BOARD_CAPT_CHIP_MAP_S astMap[] = {
        {HARDWARE_INPUT_CHIP_ADV7842, au32AdvType, },
        {HARDWARE_INPUT_CHIP_MSTAR, au32MstarType, },
        {HARDWARE_INPUT_CHIP_MCU_MSTAR, au32McuMstarType, },
    };

    if (HARDWARE_INPUT_CHIP_BUTT != enChip)
    {
        return enChip;
    }

    if (0 == (u32Board = HARDWARE_GetBoardType()))
    {
        DISP_LOGI("get board type fail\n");
        return HARDWARE_INPUT_CHIP_BUTT;
    }

    pstMap = astMap;
    for (i = 0; i < sizeof(astMap) / sizeof(astMap[0]); i++, pstMap++)
    {
        pu32Board = pstMap->pu32Board;
        while (DB_INVALID != *pu32Board)
        {
            if (u32Board == *pu32Board)
            {
                enChip = pstMap->enCaptChip;
                return enChip;
            }

            pu32Board++;
        }
    }

    enChip = HARDWARE_INPUT_CHIP_NONE;
    return enChip;
}

/*******************************************************************************
* 函数名  : HARDWARE_GetInputEdidIIC
* 描  述  : 获取输入edid连接的IIC总线
* 输  入  :
* 输  出  :
* 返回值  : HARDWARE_INPUT_CHIP_E
*******************************************************************************/
INT32 HARDWARE_GetInputEdidIIC(UINT32 u32Chn, IIC_DEV_S *pstIIC)
{
    static const IIC_DEV_S astAdv104IIC[] = {{5, 0xA0}, {5, 0xA0}};
    static const UINT32 au32Adv104Type[] = {DB_RS20001_V1_0, DB_RS20001_V1_1};
    UINT32 u32Board = 0;
    UINT32 i = 0;

    if (u32Chn >= sizeof(astAdv104IIC) / sizeof(astAdv104IIC[0]))
    {
        HARDWARE_LOGE("invalid chn[%u]\n", u32Chn);
        return SAL_FAIL;
    }

    if (0 == (u32Board = HARDWARE_GetBoardType()))
    {
        HARDWARE_LOGE("get board type fail\n");
        return SAL_FAIL;
    }

    for (i = 0; i < sizeof(au32Adv104Type) / sizeof(au32Adv104Type[0]); i++)
    {
        if (u32Board == au32Adv104Type[i])
        {
            pstIIC->u32IIC = astAdv104IIC[i].u32IIC;
            pstIIC->u32DevAddr = astAdv104IIC[i].u32DevAddr;
            return SAL_SOK;
        }
    }

    HARDWARE_LOGE("board[%u] not support edid eeprom\n", u32Board);
    return SAL_FAIL;
}

