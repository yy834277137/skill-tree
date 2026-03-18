/**
*      @file xsp_dump.hpp
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*
*      @brief XSP算法库dump数据工具函数集合
*
*      @author wangtianshu
*      @date 2023/7/12
*
*      @note
*/

#ifndef _XSP_DUMP_HPP_
#define _XSP_DUMP_HPP_

#include "xmat.hpp"
#include "xsp_def.hpp"
#include "xsp_check.hpp"

#define DUMP_PATH "./Dump"  // 算法库单元测试demo默认Dump路径

enum DUMP_RET
{
	XSP_OK                  =   0,   // 成功
	XSP_NULLPTR             =  -1,   // 指针为空
	XSP_MAT_INVALID         =  -2,   // 无效矩阵
	XSP_OPENFILE_FAIL       =  -3,   // 打开文件失败
	XSP_READFILESIZE_ERROR  =  -4,   // 读取文件大小错误
	XSP_MAT_SIZE_ERR        =  -5,   // 矩阵尺寸错误
	XSP_IDX_OVERFLOW        =  -6,   // 存储通道溢出
};

/**@fn      WriteRaw
* @brief    raw数据存储
* @param1   filepath       [I]     - 存储路径
* @param2   name           [I]     - 存储名称
* @param3   idx            [I]     - 存储通道 0-9
* @param4   low            [I]     - 低能数据
* @param5   high           [I]     - 高能数据
* @param6   ZValue         [I]     - 原子序数
* @return   错误码
* @note     idx用于区分dump不同数据时的条带号,
*           本头文件内所有dump函数存储通道共用，不能重复
*
* example：
* @code 
*           WriteRaw("./Dump", "dump", 0, low, high, z);
*/
int32_t WriteRaw(const char* filepath, const char* name, int32_t idx, 
	         XMat& low, XMat& high, XMat& ZValue);
int32_t WriteRaw(const char* filepath, const char* name, int32_t idx, XMat& raw);

/**@fn      WriteDat
* @brief    raw数据存储
* @param1   filepath       [I]     - 存储路径
* @param2   name           [I]     - 存储名称
* @param3   idx            [I]     - 存储通道 0-9
* @param4   low            [I]     - 低能数据
* @param5   high           [I]     - 高能数据
* @return   错误码
* @note     idx用于区分dump不同数据时的条带号,
*           本头文件内所有dump函数存储通道共用，不能重复
*
* example：
* @code
*           WriteDat("./Dump", "dump", 1, low, high);
*/
int32_t WriteDat(const char* filepath, const char* name, int32_t idx,
	         XMat& low, XMat& high);




#endif // _XSP_DUMP_HPP_
