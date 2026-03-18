/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   Description: 不同产品可在该文件中进行配置，对系统中的模块进行绑定，
   从而搭建起产品特有的数据流
   Author: yeyanzhong
   Date: 2020.12.10
   Modification History:
        1、20220815，节点配置信息从link_task.c里拆分到capability目录下的link_cfg.c

******************************************************************************/
#ifndef _LINK_CFG_API_H_
#define _LINK_CFG_API_H_

#include "link_task_api.h"


INT32 link_task_getDupInstCfg(INST_CFG_S **pFrontDupInstCfg, INST_CFG_S **pRearDupInstCfg);

INT32 link_cfg_getBinderInfo(BIND_S **ppstBinderInfo, UINT32 *pu32BinderCnt);


#endif

