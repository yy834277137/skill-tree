/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: link_task.c
   Description: 不同产品在能力集模块下link_cfg.c里进行配置，本文件接口根据配置信息进行绑定，
   从而搭建起产品特有的数据流
   Author: yeyanzhong
   Date: 2020.12.10
   Modification History:
******************************************************************************/
#ifndef _LINK_DRV_H_
#define _LINK_DRV_H_

#include "link_drv_api.h"

typedef struct _BIND_S
{
    CHAR szSrcInst[NAME_LEN];
    CHAR szSrcNode[NAME_LEN];
    CHAR szDstInst[NAME_LEN];
    CHAR szDstNode[NAME_LEN];
} BIND_S;

INT32 link_taskBindMods();

#endif

