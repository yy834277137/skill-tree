/**
 * @file   icf_version_compat.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  icf符号版本兼容V2
 * @author sunzelin
 * @date   2022/7/4
 * @note
 * @note \n History
   1.日    期: 2022/7/4
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
/* ICF数据结构与接口符号根据不同的版本增加后缀，目前有两种情况: 无后缀和_V2。
   当前仅NT98336和RK3588使用V2.1.8，即ICF数据结构加后缀_V2的版本 */
#if defined ICF_V2
#define ICF_VER_SUFFIX(x) x##_V2
#else
#define ICF_VER_SUFFIX(x) x
#endif
/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
#define ICF_MEM_INFO ICF_VER_SUFFIX(ICF_MEM_INFO)
#define ICF_MEM_BUFFER ICF_VER_SUFFIX(ICF_MEM_BUFFER)
#define ICF_MEM_INTERFACE ICF_VER_SUFFIX(ICF_MEM_INTERFACE)
#define ICF_BUFF ICF_VER_SUFFIX(ICF_BUFF)
#define ICF_CONFIG_INFO ICF_VER_SUFFIX(ICF_CONFIG_INFO)
#define ICF_INIT_PARAM ICF_VER_SUFFIX(ICF_INIT_PARAM)
#define ICF_INIT_HANDLE ICF_VER_SUFFIX(ICF_INIT_HANDLE)
#define ICF_HVA_PARAM_INFO ICF_VER_SUFFIX(ICF_HVA_PARAM_INFO)
#define ICF_APP_PARAM_INFO ICF_VER_SUFFIX(ICF_APP_PARAM_INFO)
#define ICF_MEMSIZE_PARAM ICF_VER_SUFFIX(ICF_MEMSIZE_PARAM)
#define ICF_MODEL_PARAM ICF_VER_SUFFIX(ICF_MODEL_PARAM)
#define ICF_MODEL_HANDLE ICF_VER_SUFFIX(ICF_MODEL_HANDLE)
#define ICF_CREATE_PARAM ICF_VER_SUFFIX(ICF_CREATE_PARAM)
#define ICF_CREATE_HANDLE ICF_VER_SUFFIX(ICF_CREATE_HANDLE)
#define ICF_CALLBACK_PARAM ICF_VER_SUFFIX(ICF_CALLBACK_PARAM)
#define ICF_CONFIG_PARAM ICF_VER_SUFFIX(ICF_CONFIG_PARAM)
#define ICF_SYSTEMTIME ICF_VER_SUFFIX(ICF_SYSTEMTIME)
#define ICF_MEDIA_RAW_DATA ICF_VER_SUFFIX(ICF_MEDIA_RAW_DATA)
#define ICF_MEDIA_DATA ICF_VER_SUFFIX(ICF_MEDIA_DATA)
#define ICF_DECODE_DATA ICF_VER_SUFFIX(ICF_DECODE_DATA)
#define ICF_SOURCE_BLOB ICF_VER_SUFFIX(ICF_SOURCE_BLOB)
#define ICF_INPUT_DATA ICF_VER_SUFFIX(ICF_INPUT_DATA)
#define ICF_MEDIA_INFO ICF_VER_SUFFIX(ICF_MEDIA_INFO)
#define ICF_DATA_PACK ICF_VER_SUFFIX(ICF_DATA_PACK)
#define ICF_NODE_LABEL ICF_VER_SUFFIX(ICF_NODE_LABEL)
#define ICF_MEMPOOL_STATUS ICF_VER_SUFFIX(ICF_MEMPOOL_STATUS)
#define ICF_VERSION ICF_VER_SUFFIX(ICF_VERSION)
#define ICF_LOG_CFG ICF_VER_SUFFIX(ICF_LOG_CFG)

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/
#define ICF_Init ICF_VER_SUFFIX(ICF_Init)
#define ICF_Finit ICF_VER_SUFFIX(ICF_Finit)
#define ICF_LoadModel ICF_VER_SUFFIX(ICF_LoadModel)
#define ICF_UnloadModel ICF_VER_SUFFIX(ICF_UnloadModel)
#define ICF_Create ICF_VER_SUFFIX(ICF_Create)
#define ICF_Destroy ICF_VER_SUFFIX(ICF_Destroy)
#define ICF_InputData ICF_VER_SUFFIX(ICF_InputData)
#define ICF_SetConfig ICF_VER_SUFFIX(ICF_SetConfig)
#define ICF_GetConfig ICF_VER_SUFFIX(ICF_GetConfig)
#define ICF_SetCallback ICF_VER_SUFFIX(ICF_SetCallback)
#define ICF_SubFunction ICF_VER_SUFFIX(ICF_SubFunction)
#define ICF_System_SetConfig ICF_VER_SUFFIX(ICF_System_SetConfig)
#define ICF_System_GetConfig ICF_VER_SUFFIX(ICF_System_GetConfig)
#define ICF_GetVersion ICF_VER_SUFFIX(ICF_GetVersion)
#define ICF_APP_GetVersion ICF_VER_SUFFIX(ICF_APP_GetVersion)

