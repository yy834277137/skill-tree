/** @file      alg_error_code.h
 *  @note      HangZhou Hikvision Digital Technology Co., Ltd.
               All right reserved
 *  @brief     错误码
 *
 *  @author    张俊力
 *  @date      2020/7/2
 */

#ifndef _ALG_ERROR_CODE_H_
#define _ALG_ERROR_CODE_H_

// 安检仪应用错误码
#define XSIE_ALG_SUCCESS                          (0)
#define XSIE_ALG_BASE                             (0x8A180000)              // 开放平台错误码基准
#define XSIE_ALG_NULL_PTR                         (XSIE_ALG_BASE + 0)       // 空指针
#define XSIE_ALG_MEM_TRAN_TYPE_ERR                (XSIE_ALG_BASE + 1)       // 内存类型转换错误
#define XSIE_ALG_PRAM_ERR                         (XSIE_ALG_BASE + 2)       // 输入参数错误
#define XSIE_ALG_ENINPUT_TRAN_ALGINPUT_ERR        (XSIE_ALG_BASE + 3)       // 算子输出到引擎转换错误
#define XSIE_ALG_OBD_MODEL_NUM_ZERO               (XSIE_ALG_BASE + 4)       // OBD输入模型数量为0
#define XSIE_ALG_OBD_MODEL_PARAM_ERR              (XSIE_ALG_BASE + 5)       // 模型参数错误
#define XSIE_ALG_OBD_MEM_ERR                      (XSIE_ALG_BASE + 6)       // 内存分配错误
#define XSIE_ALG_OBD_MODEL_HEADER_ERR             (XSIE_ALG_BASE + 7)       // OBD 模型头错误
#define XSIE_ALG_OBD_MODEL_MUN_UNMATCH            (XSIE_ALG_BASE + 8)       // 模型数量与输入模型不匹配
#define XSIE_ALG_OBD_MODEL_ANALYSIS_ERR           (XSIE_ALG_BASE + 9)       // OBD模型解析错误
#define XSIE_ALG_OBD_GETMENSIZE_ERR               (XSIE_ALG_BASE + 10)      // OBD GetMemsize错误
#define XSIE_ALG_OBD_CREAT_ERR                    (XSIE_ALG_BASE + 11)      // OBD Create错误
#define XSIE_ALG_OBD_RESIZE_ERR                   (XSIE_ALG_BASE + 12)      // OBD Resize错误
#define XSIE_ALG_OBD_PROCESS_ERR                  (XSIE_ALG_BASE + 13)      // OBD Process错误
#define XSIE_ALG_XSI_GETMENSIZE_ERR               (XSIE_ALG_BASE + 14)      // XSI GetMemsize错误
#define XSIE_ALG_XSI_CREAT_ERR                    (XSIE_ALG_BASE + 15)      // XSI Create错误
#define XSIE_ALG_XSI_PROCESS_ERR                  (XSIE_ALG_BASE + 16)      // XSI Process错误
#define XSIE_ALG_GET_VERSION_ERR                  (XSIE_ALG_BASE + 17)      // 获取版本信息错误
#define XSIE_ALG_READ_FILE_ERROR                  (XSIE_ALG_BASE + 18)      // 读取文件错误
#define XSIE_ALG_DEC_SET_CFG_ERROR                (XSIE_ALG_BASE + 19)      // XSI_DEC_SET_CFG错误
#define XSIE_ALG_DEC_GET_CFG_ERROR                (XSIE_ALG_BASE + 20)      // XSI_DEC_GET_CFG错误
#define XSIE_ALG_TYPE_SET_ERROR                   (XSIE_ALG_BASE + 21)      // AlgType设置错误
#define XSIE_NODE_ID_SET_ERROR                    (XSIE_ALG_BASE + 22)      // NODE_ID设置错误
#define XSIE_NODE_DATA_SEND_ERROR                 (XSIE_ALG_BASE + 23)      // 同步数据包发送错误
#define XSIE_ALG_ROUTE_SET_CFG_ERROR              (XSIE_ALG_BASE + 24)      // XSI_DEC_SET_CFG错误
#define XSIE_ALG_ROUTE_GET_CFG_ERROR              (XSIE_ALG_BASE + 25)      // XSI_DEC_GET_CFG错误
#define XSIE_ALG_LCD_MODEL_PARAM_ERR              (XSIE_ALG_BASE + 26)      // 模型参数错误
#define XSIE_ALG_LCD_MODEL_ANALYSIS_ERR           (XSIE_ALG_BASE + 27)      // LCD模型解析错误
#define XSIE_ALG_SET_CONFIG                       (XSIE_ALG_BASE + 28)      // LCD设置参数错误
#define XSIE_ALG_ROUTE_CACHE_ERR                  (XSIE_ALG_BASE + 29)      // 路由节点缓存错误

#endif /* _ALG_ERROR_CODE_H_ */

