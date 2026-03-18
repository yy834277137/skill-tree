/******************************************************************************
  * @project: LT9211
  * @file: lt9211.h
  * @author: zll
  * @company: LONTIUM COPYRIGHT and CONFIDENTIAL
  * @date: 2019.04.10
******************************************************************************/

#ifndef _CAPT_CHIP_LT9211_H_
#define _CAPT_CHIP_LT9211_H_

/*******************************************************************************
* 函数名  : capt_chip_lt9211Config
* 描  述  : 配置Lt9211
* 输  入  : - uiChn       通道
          - u32Width    宽
          - u32height   高
          - fps         帧率
          - bIsCheckPixClk 是否检查时钟
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
void capt_chip_lt9211Config(UINT32 uiChn,UINT32 u32Width,UINT32 u32height,UINT32 fps,BOOL bIsCheckPixClk);

/*******************************************************************************
* 函数名  : capt_chip_lt9211ModuleInit
* 描  述  : 配置Lt9211
* 输  入  : - uiChn       通道
          - u32I2cBusID    IIC总线号
          - u32I2cAddr     IIC地址
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
void capt_chip_lt9211ModuleInit(UINT32 uiChn,UINT32 u32I2cBusID, UINT32 u32I2cAddr);


#endif