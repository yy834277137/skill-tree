/*** 
 * @file   sal_errno.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  错误码定义
 * @author cuifeng5
 * @date   2022-02-24
 * @note   
 * @note History:
 */

#ifndef __SAL_ERRNO_H_
#define __SAL_ERRNO_H_

/*
 * 32bits错误码定义如下：最高位默认为1，BU_ID，MOD_ID，ERR_LEVEL，ERR_ID
 * 
 * BU_ID     : 对应资源组表示
 * MOD_ID    : 模块ID
 * ERR_LEVEL : 错误码等级
 * ERR_ID    : 错误码
 *
 */

/******************************************************************************
        |------------------------------------------------------|
        | 1 |   BU_ID   |    MOD_ID   | ERR_LEVEL |   ERR_ID   |
        |------------------------------------------------------|
        |<-> <--7bits--> <---8bits---> <--4bits--> <--12bits-->|
******************************************************************************/

#define BU_DSP_ID       (3u)        /* DSP资源组ID标识 */

/*
 * 根据错误模块(mod)、等级(level)、错误码(err)，生成错误码.
 */
#define DSP_DEF_ERR(mod, level, err) \
    ((INT32)((1 << 31u) | ((BU_DSP_ID & 0x7F) << 24u) | (((mod) & 0xFF) << 16u) | (((level) & 0xF) << 12u) | ((err) & 0xFFF)))

/*
 * @brief 根据错误模块名称(mod)、模块类型名称(type)、错误码名称(err)，生成错误码
 */
#define DSP_MOD_DEF_ERR_CODE(mod, type, err)  CAT3(mod, _, err) = DSP_DEF_ERR(CAT4(MOD_, type, _, mod), ERR_DSP_LEVEL_ERROR, CAT(ERR_DSP_, err))

/*
 * @brief 根据错误模块名称(mod)、模块类型名称(type)，生成所有错误码
 */
#define DSP_MOD_DEF_ALL_ERR(mod, type)                      \
enum {                                                      \
    DSP_MOD_DEF_ERR_CODE(mod, type, ILLEGAL_PARAM),         \
    DSP_MOD_DEF_ERR_CODE(mod, type, NULL_PTR),              \
    DSP_MOD_DEF_ERR_CODE(mod, type, NOT_SUPPORT),           \
    DSP_MOD_DEF_ERR_CODE(mod, type, TIMEOUT),               \
    DSP_MOD_DEF_ERR_CODE(mod, type, UNREADY),               \
    DSP_MOD_DEF_ERR_CODE(mod, type, EXIST),                 \
    DSP_MOD_DEF_ERR_CODE(mod, type, UNEXIST),               \
    DSP_MOD_DEF_ERR_CODE(mod, type, FAILED_INIT),           \
    DSP_MOD_DEF_ERR_CODE(mod, type, FAILED_RESET),          \
    DSP_MOD_DEF_ERR_CODE(mod, type, FAILED_CONFIG),         \
    DSP_MOD_DEF_ERR_CODE(mod, type, FAILED_PROCESS),        \
    DSP_MOD_DEF_ERR_CODE(mod, type, FAILED_ENABLE),         \
    DSP_MOD_DEF_ERR_CODE(mod, type, FAILED_DISABLE),        \
    DSP_MOD_DEF_ERR_CODE(mod, type, FAILED_MEM_ALLOC),      \
    DSP_MOD_DEF_ERR_CODE(mod, type, FAILED_MEM_FREE),       \
    DSP_MOD_DEF_ERR_CODE(mod, type, INVALID_CHAN),          \
    DSP_MOD_DEF_ERR_CODE(mod, type, INVALID_PARAM),         \
    DSP_MOD_DEF_ERR_CODE(mod, type, INVALID_ADDR),          \
    DSP_MOD_DEF_ERR_CODE(mod, type, INVALID_DATA),          \
    DSP_MOD_DEF_ERR_CODE(mod, type, INVALID_POINTER),       \
    DSP_MOD_DEF_ERR_CODE(mod, type, INVALID_HOST_CMD),      \
    DSP_MOD_DEF_ERR_CODE(mod, type, INVALID_DSP_CMD),       \
    DSP_MOD_DEF_ERR_CODE(mod, type, INVALID_STATUS),        \
    DSP_MOD_DEF_ERR_CODE(mod, type, INVALID_PLATFORM),      \
    DSP_MOD_DEF_ERR_CODE(mod, type, INVALID_HANDLE),        \
    DSP_MOD_DEF_ERR_CODE(mod, type, UNSUPPORTED_CHAN),      \
    DSP_MOD_DEF_ERR_CODE(mod, type, UNSUPPORTED_PARAM),     \
    DSP_MOD_DEF_ERR_CODE(mod, type, UNSUPPORTED_BOARD),     \
    DSP_MOD_DEF_ERR_CODE(mod, type, OVERFLOW_FRAME),        \
    DSP_MOD_DEF_ERR_CODE(mod, type, OVERFLOW_BUF),          \
    DSP_MOD_DEF_ERR_CODE(mod, type, OVERFLOW_STREAM),       \
    DSP_MOD_DEF_ERR_CODE(mod, type, OVERFLOW_GROUP),        \
    DSP_MOD_DEF_ERR_CODE(mod, type, NEED_MORE_DATA),        \
    DSP_MOD_DEF_ERR_CODE(mod, type, WAIT_BUF),              \
}

/**
 * @brief 错误码，严重等级定义
 */
enum ERR_DSP_LEVEL_E
{
    ERR_DSP_LEVEL_INFO = 0,    /* informational                                */
    ERR_DSP_LEVEL_WARNING,     /* warning conditions                           */
    ERR_DSP_LEVEL_ERROR,       /* error conditions                             */
    ERR_DSP_LEVEL_ALERT,       /* action must be taken immediately             */

    ERR_DSP_LEVEL_BUTT
};

/**
 * @brief 通用错误码定义，取值范围为0x00--0x7FF。0x800--0xFFF为模块之中定义的私有错误码
 */
enum
{
    ERR_DSP_ILLEGAL_PARAM      = 0x00,           /* 参数非法                                       */
    ERR_DSP_NULL_PTR           = 0x01,           /* 空指针                                         */
    ERR_DSP_NOT_SUPPORT        = 0x02,           /* 当前操作/功能，不支持                          */
    ERR_DSP_TIMEOUT            = 0x03,           /* 处理超时                                       */
    ERR_DSP_UNREADY            = 0x04,           /* 模块未准备好                                   */
    ERR_DSP_EXIST              = 0x05,           /* 试图申请或者创建已经存在的设备、通道或者资源   */
    ERR_DSP_UNEXIST            = 0x06,           /* 试图使用或者销毁不存在的设备、通道或者资源     */

    ERR_DSP_FAILED_INIT        = 0x07,           /* 初始化失败                                     */
    ERR_DSP_FAILED_RESET       = 0x08,           /* 复位失败                                       */
    ERR_DSP_FAILED_CONFIG      = 0x09,           /* 配置失败                                       */
    ERR_DSP_FAILED_PROCESS     = 0x0a,           /* 处理失败                                       */
    ERR_DSP_FAILED_ENABLE      = 0x0b,           /* 功能使能失败                                   */
    ERR_DSP_FAILED_DISABLE     = 0x0c,           /* 功能禁止失败                                   */
    ERR_DSP_FAILED_MEM_ALLOC   = 0x0d,           /* 内存申请失败                                   */
    ERR_DSP_FAILED_MEM_FREE    = 0x0e,           /* 内存释放失败                                   */

    ERR_DSP_INVALID_CHAN       = 0x0f,           /* 无效通道                                       */
    ERR_DSP_INVALID_PARAM      = 0x10,           /* 无效参数                                       */
    ERR_DSP_INVALID_ADDR       = 0x11,           /* 无效地址                                       */
    ERR_DSP_INVALID_DATA       = 0x12,           /* 无效数据                                       */
    ERR_DSP_INVALID_POINTER    = 0x13,           /* 无效数据                                       */
    ERR_DSP_INVALID_HOST_CMD   = 0x14,           /* 无效主机命令                                   */
    ERR_DSP_INVALID_DSP_CMD    = 0x15,           /* 无效DSP命令                                    */
    ERR_DSP_INVALID_STATUS     = 0x16,           /* 无效状态                                       */
    ERR_DSP_INVALID_PLATFORM   = 0x17,           /* 无效平台                                       */
    ERR_DSP_INVALID_HANDLE     = 0x18,           /* 无效操作                                       */

    ERR_DSP_UNSUPPORTED_CHAN   = 0x19,           /* 不支持的通道                                   */
    ERR_DSP_UNSUPPORTED_PARAM  = 0x1a,           /* 不支持的参数                                   */
    ERR_DSP_UNSUPPORTED_BOARD  = 0x1b,           /* 不支持的板子                                   */

    ERR_DSP_OVERFLOW_FRAME     = 0x1c,           /* 帧溢出                                         */
    ERR_DSP_OVERFLOW_BUF       = 0x1d,           /* 缓冲溢出                                       */
    ERR_DSP_OVERFLOW_STREAM    = 0x1e,           /* 流溢出                                         */
    ERR_DSP_OVERFLOW_GROUP     = 0x1f,           /* 信息组溢出                                     */
    ERR_DSP_NEED_MORE_DATA     = 0x20,           /* 需要更多的数据                                 */
    ERR_DSP_WAIT_BUF           = 0x21,           /* 等待缓冲                                       */

    ERR_DSP_CODE_MAX = 0x7FF,

    ERR_DSP_CODE_BUTT
};

#endif /* __SAL_ERRNO_H_ */

