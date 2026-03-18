/** @file      alg_error_code.h
 *  @note      HangZhou Hikvision Digital Technology Co., Ltd.
               All right reserved
 *  @brief     错误码
 *
 *  @author    张俊力
 *  @date      2022/08/29
 */


#ifndef _ALG_ERROR_CODE_H_
#define _ALG_ERROR_CODE_H_

#define    ALG_SUCCESS                               (0)             // 正确

#define    APP_COM_E_MEM_SPACE                       (0x8A150101)    // 内存类型不合理
#define    APP_COM_E_MEM_ALLOC                       (0x8A150102)    // 内存分配错误
#define    APP_COM_E_MEM_FREE                        (0x8A150103)    // 内存释放错误
#define    APP_COM_E_INVALIDE_KEY                    (0x8A150104)    // 配置key无效


/*************** RK3588端算子 ******************/
#define    MTME_ERROR_CODE_BASE                      (0x8a100000)

//内部公共模块
#define    MTME_APP_PARAM_CFG_PATH_NULL              (0x8a100000)    // 输入配置路径为空
#define    MTME_APP_PARAM_CFG_PARSE_ERR              (0x8a100001)    // 输入JSON配置解析错误

#define    MTME_INTERFACE_NULL_PTR                   (0x8a100002)    // 获取APP版本空指针
#define    MTME_INTERFACE_SIZE_ERR                   (0x8a100003)    // 获取APP版本结构体大小错误

//DECODE 节点
#define    MTME_DECODE_E_BLOB_NUM                    (0x8a100009)
#define    MTME_DECODE_E_SHAPE                       (0x8a10000a)
#define    MTME_DECODE_E_DATA_FORMAT                 (0x8a10006b)
#define    MTME_DECODE_E_WORK_MODE                   (0x8a10006c)
#define    MTME_DECODE_E_PARAM_ERR                   (0x8a10006d)   // 输入数据信息存在问题

//FD-TRK
#define    MTME_FD_TRK_MODEL_PATH_NULL               (0x8a100016)
#define    MTME_FD_TRK_PARAM_ERR                     (0x8a100016)

//DFR-LandMark-Score节点
#define    MTME_DFR_SCORE_PARAM_SIZE                 (0x8a100017)
#define    MTME_DFR_SCORE_MODEL_PATH_NULL            (0x8a100018)

//MATCH_PACK节点
#define    MTME_MATCH_PACK_PARAM_SIZE                (0x8a100019)
#define    MTME_MATCH_PACK_PARAM_ERR                 (0x8a10001A)
#define    MTME_MATCH_PACK_OBJ_NUM_OVERFLOW          (0x8a10001B)    // 目标缓存队列错误


//MATCH_FACE节点
#define    MTME_MATCH_FACE_PARAM_ERR                 (0x8a10001c)
#define    MTME_MATCH_FACE_PARAM_SIZE                (0x8a10001d)
#define    MTME_MATCH_FACE_OUT_RANGE                 (0x8a10001E)


//标定
#define    MTME_CABLIC_PARAM_ERR                     (0x8a100021)
#define    MTME_CABLIC_NULL_PTR                      (0x8a100022)
#define    MTME_CAPTURE_PARAM_SIZE                   (0x8a100023)
 
//单目相机标定
#define    MTME_SVCABLIC_PARAM_ERR                   (0x8a100024)
#define    MTME_SVCABLIC_NULL_PTR                    (0x8a100025)
#define    MTME_SVCABLIC_PARAM_SIZE                  (0x8a100026)


#endif

