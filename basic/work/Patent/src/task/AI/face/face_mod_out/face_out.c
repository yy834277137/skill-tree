/**
 * @file   face_out.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  人脸模块外部源文件
 * @author sunzelin
 * @date   2022/8/22
 * @note
 * @note \n History
   1.日    期: 2022/8/22
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "face_out.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/**
 * @function   Face_tskInit
 * @brief      人脸模块tsk层初始化
 * @param[in]  void
 * @param[out] None
 * @return     ATTRIBUTE_WEAK INT32
 */
ATTRIBUTE_WEAK INT32 Face_tskInit(void)
{
    return SAL_SOK;
}

/**
 * @function   CmdProc_faceCmdProc
 * @brief      应用层命令字交互接口
 * @param[in]  HOST_CMD cmd
 * @param[in]  UINT32 chan
 * @param[in]  void *prm
 * @param[out] None
 * @return     ATTRIBUTE_WEAK INT32
 */
ATTRIBUTE_WEAK INT32 CmdProc_faceCmdProc(HOST_CMD cmd, UINT32 chan, void *prm)
{
    return SAL_SOK;
}

