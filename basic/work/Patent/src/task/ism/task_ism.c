/**
 * @file   disp_tsk.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  安检机模块
 * @author liuxianying
 * @date   2022/4/6
 * @note
 * @note \n History
   1.日    期: 2022/4/6
     作    者: liuxianying
     修改历史: 创建文件

**/

/* 安检机外部依赖函数，安检机使用正常功能，非安检机使用空函数*/

#include"task_ism.h"


#ifdef DSP_ISM
/* 安检机产品，使用xsp, xpack目录内功能*/

#else
/* 非安检机产品，使用本空函数，方便编译*/

/*******************非安检机产品外部调用XPACK函数实现*************************/

SAL_STATUS Xpack_DrvSetAiXrOsdShow(UINT32 chan, DISP_SVA_SWITCH *pstDispOsd, BOOL bShowTip)
{
    return SAL_SOK;
}

SAL_STATUS Xpack_DrvSetAidgrLinePrm(UINT32 chan, DISPLINE_PRM *pstAiDgrLine)
{
    return SAL_SOK;
}


#endif
