/** 
 * @file      alg_error_code.h
 * @note      HangZhou Hikvision Digital Technology Co., Ltd.
              All right reserved
 * @brief     错误码，各应用统一错误码，分三大类，另外提取公共模块和通用算子；
 *            由专人维护改动
 *
 * @version   v1.0.0 
 * @author    pignyinan
 * @date      2021/11/16
 * @note      添加idr相关错误码
 * 
 * @version   v1.0.0 
 * @author    chenpeng43
 * @date      2021/9/29
 * @note      合并错误码
 *
 * @version   v0.8.0 
 * @author    yanghang5
 * @date      2020/4/9
 * @note      初始版本
 */


#ifndef _ALG_ERROR_CODE_H_
#define _ALG_ERROR_CODE_H_

#define ALG_SUCCESS                               (0)          // 正确


/***************************************************************************************************
 * @name     公共模块
 * @brief    包括文件处理，计时，图像软核处理，硬件加速等公共模块；
 * @note     0x89000000~0x89000FFF
***************************************************************************************************/
// 文件处理+计时等系统操作，0x89000000~0x890000FF
#define ALG_COMMON_E_MEM_SPACE_TYPE               (0x89000000) // 内存类型错误


// 图像软核处理，0x89000100~0x890001FF

// 硬核加速模块，0x89000200~0x890002FF

/***************************************************************************************************
 * @name     通用算子
 * @brief    提取出的通用算子；
 * @note     0x89010000~0x890100FF
***************************************************************************************************/
// 图像预处理算子



/***************************************************************************************************
 * @name     人脸相关应用
 * @note     0x89100000~0x893FFFFF
***************************************************************************************************/
// 人脸，0x89110000~0x893FFFFF

// 引擎自身相关 0x8A120000~0x8A1200FF
// 能力集相关错误码
#define ICF_ALPG_ABILITY_CONFIG_NULL                (0x8A120000) // 能力集文件或结构体为空
#define ICF_ALPG_ABILITY_CONFIG_SIZE_ERR            (0x8A120001) // 能力集结构体大小不正确
#define ICF_ALPG_ABILITY_CONFIG_PATH_LENGTH_ERR     (0x8A120002) // 能力集字符串路径长度大于256字节
#define ICF_ALPG_ABILITY_MODEL_BUFF_SIZE_SMALL      (0x8A120003) // 能力集结构体外部模型缓存过小
#define ICF_ALPG_ABILITY_MODEL_BUFF_ERR             (0x8A120004) // 能力集结构体外部模型缓存指针与数据信息不匹配（如指针为空，数据长度不为0，或者数据长度为0，指针不为空）
#define ICF_ALPG_ABILITY_MODULE_ENABLE_ERR          (0x8A120005) // 能力集模块使能配置错误，既不为0也不为1，或者模块的使能受到限制，必须为0或者1
#define ICF_ALPG_ABILITY_MEM_EXHAUSTED              (0x8A120006)
#define ICF_ALPG_ABILITY_MODEL_BUFF_EMPTY_ERR       (0x8A120007) // 能力集结构体外部模型缓存为空

#define ICF_ALPG_DOUBLE_MODULE_NOT_ENABLE           (0x8A120010) // 双路建模模块未启用，请创建对应业务线
#define ICF_ALGP_JSON_ANAYLSE_FILE_ERR              (0x8A120020) // JSON的文件存在问题（路径）
#define ICF_ALGP_JSON_ANAYLSE_ROOT_ERR              (0x8A120021) // JSON的root不存在（未使用ICF_SAE_JsonAnaylse_Init进行初始化）
#define ICF_ALGP_JSON_ANAYLSE_NODE_ERR              (0x8A120022) // JSON的node不存在
#define ICF_ALGP_JSON_ANAYLSE_PARAM_ERR             (0x8A120023) // JSON的param不存在
#define ICF_ALGP_JSON_ANAYLSE_PATH_LENGTH_ERR       (0x8A120024) // JSON的字符串路径长度过长


#define ICF_ALGP_GENERAL_NULL                       (0x8A120030) // 算子空指针
#define ICF_ALGP_GENERAL_PARAM                      (0x8A120031) // 参数错误
#define ICF_ALGP_ALG_HANDLE_NULL                    (0x8A120032) // 算子空指针
#define ICF_ALGP_INIT_HANDLE_NULL                   (0x8A120033) // 算子空指针

// 第三方函数相关错误码
#define ICF_ALGP_R_FUNCTION_IMG_FORMAT_ERR          (0x8A120040) // 外部注册函数的图像格式错误
#define ICF_ALGP_R_FUNCTION_NO_CROP_ERR             (0x8A120041) // 外部CROP注册函数未实现crop功能

// 平台相关错误码
#define ICF_ALGP_CORE_BIND_PLATFORM_ERR             (0x8A120050) // 核心绑定平台错误
#define ICF_ALGP_CORE_BIND_ID_ERR                   (0x8A120051) // 核心绑定ID错误
#define ICF_ALGP_IA_RESIZE_NOT_SUPPORT_ERR          (0x8A120052) // IA不支持错误

// 人脸输入数据检查相关错误码 0x8A120100~0x8A1201FF
#define ICF_ALGP_INDATA_PROC_TYPE_ERR                   (0x8A120100) // 输入数据中proc_type配置错误
#define ICF_ALGP_INDATA_FACE_PROC_TYPE_ERR              (0x8A120101) // 输入数据中face_proc_type配置错误
#define ICF_ALGP_INDATA_DET_PRIORITY_ERR                (0x8A120102) // 输入数据中det_priority配置错误
#define ICF_ALGP_INDATA_FEAT_PRIORITY_ERR               (0x8A120103) // 输入数据中feat_priority配置错误
#define ICF_ALGP_INDATA_ROI_PARAM_ERR                   (0x8A120104) // 输入数据中roi_rect配置错误
#define ICF_ALGP_INDATA_YUV_PTR_NULL                    (0x8A120105) // 输入数据中yuv_data数据指针为空错误
#define ICF_ALGP_INDATA_DET_TRACK_COMPARE_ERR           (0x8A120106) // 输入数据中DET_TRACK(_V2)业务线不能使能COMPARE功能
#define ICF_ALGP_INDATA_FEAT_LIVENESS_TRACK_ERR         (0x8A120107) // 输入数据中FEAT_LIVENESS业务线不能使能TRACK功能
#define ICF_ALGP_INDATA_FEAT_LIVENESS_FACE_ERR          (0x8A120108) // 输入数据中FEAT_LIVENESS业务线只能处理单人脸
#define ICF_ALGP_INDATA_DET_FEATURE_TRACK_ERR           (0x8A120109) // 输入数据中DET_FEATURE业务线不能使能TRACK功能
#define ICF_ALGP_INDATA_DET_FEATURE_COMPARE_ERR         (0x8A12010A) // 输入数据中DET_FEATURE业务线不能使能COMPARE功能
#define ICF_ALGP_INDATA_DET_FEATURE_LIVENESS_ERR        (0x8A12010B) // 输入数据中DET_FEATURE业务线不能使能LIVENESS功能
#define ICF_ALGP_INDATA_DET_FEATURE_FACE_ERR            (0x8A12010C) // 输入数据中DET_FEATURE业务线只能处理单人脸
#define ICF_ALGP_INDATA_DET_FEATCMP_TRACK_ERR           (0x8A12010D) // 输入数据中DET_FEATCMP(_V2)业务线不能使能TRACK功能
#define ICF_ALGP_INDATA_DET_FEATCMP_FACE_ERR            (0x8A12010E) // 输入数据中DET_FEATCMP(_V2)业务线只能处理单人脸
#define ICF_ALGP_BLOB_EBLOBFORMAT_ERR                   (0x8A12010F) // stBlobData[0]中eBlobFormat不是NV21或NV12
#define ICF_ALPG_INDATA_PROC_HELMET_ERR                 (0x8A120110) // 输入数据中 proc_type 需要设置为 ICF_FACE_PROC_TYPE_HELMET 业务线
#define ICF_ALPG_INDATA_PROC_SELECT_FRAMET_ERR          (0x8A120111) // 输入数据中 proc_type 需要设置为 ICF_FACE_PROC_TYPE_TRACK_SELECT 业务线
#define ICF_ALGP_INDATA_SELECT_FRAMET_TRACK_ERR         (0x8A120112) // 输入数据中ICF_FACE_PROC_TYPE_TRACK_SELECT业务线必须使能TRACK功能
#define ICF_ALGP_INDATA_ERROR_PROC_TYPE_ERR             (0x8A120113) // 输入数据中error_proc_type配置错误
#define ICF_ALGP_INDATA_ERROR_LIVENESS_TYPE_ERR         (0x8A120114) // 输入数据中liveness_type配置错误
#define ICF_ALGP_INDATA_YUV_FORMAT_ERR                  (0x8A120115) // 输入数据中yuv_data数据类型错误
#define ICF_ALGP_INDATA_INPUT_IMG_ALIGNED_ERR           (0x8A120116) // 输入数据中的图像宽高不是4对齐的
#define ICF_ALGP_INDATA_INPUT_IMG_SIZE_ERR              (0x8A120117) // 输入数据中的图像宽高超过配置的能力集
#define ICF_ALGP_INDATA_MODULE_ENABLE_TYPE_ERR          (0x8A120118) // 输入数据中的模块使能类型错误，即不是0或1
#define ICF_ALGP_INDATA_INPUT_IMG_TYPE_ERR              (0x8A120119) // 输入数据中的图像类型错误，不是RGB或者IR，检查 img_type_info 信息
#define ICF_ALGP_INDATA_INPUT_IMG_VALID_TYPE_ERR        (0x8A12011A) // 输入数据中的图像有效性错误，不是0或者1，检查 img_type_info 信息
#define ICF_ALGP_INDATA_INPUT_LIVE_IMG_VALID_TYPE_ERR   (0x8A12011B) // 输入数据中的活体需要的图像有效性错误，检查 img_type_info 信息或者活体使能类型
#define ICF_ALGP_INDATA_INPUT_IMG_MAIN_INVALID_ERR      (0x8A12011C) // 输入数据中的主类型图像标志位为无效，检查 img_type_info 信息
#define ICF_ALGP_INDATA_INPUT_IMG_TYPE_DUPLICATED_ERR   (0x8A12011D) // 输入数据中的有效图像类型重复，如2个都是IR图像，检查 img_type_info 信息
#define ICF_ALGP_INDATA_INPUT_FACE_NUM_ERR              (0x8A12011E) // 输入数据中的人脸数量 face_num 不在有效范围内 [0 SAE_FACE_MAX_FACE_NUM]
#define ICF_ALGP_INDATA_INPUT_FACE_RECT_ERR             (0x8A12011F) // 输入数据中的人脸框错误，框不合法或超出图像
#define ICF_ALGP_INDATA_INPUT_FACE_CONF_ERR             (0x8A120120) // 输入数据中的人脸置信度错误，不属于[0 1]

// 各个算子模块内部错误码
// FD_DETECT 0x8A120200~0x8A1202FF
#define ICF_ALGP_DETECT_STS_INIT_STATUS             (0x8A120200) // 结果状态位初始化状态
#define ICF_ALGP_DETECT_STS_IMG_PROC_ERR            (0x8A120201) // 结果状态位图像级错误
#define ICF_ALGP_DETECT_STS_MODULE_PROC_ERR         (0x8A120202) // 结果状态位模块级错误
#define ICF_ALGP_DETECT_RESOLUTION_NOT_SUPPORT      (0x8A120210) // 检测所用图像分辨率不支持（小图分辨率超过了960*960）
#define ICF_ALGP_DETECT_NO_FACE                     (0x8A120211) // 检测人脸数目为零
#define ICF_ALGP_DETECT_ABILITY_MAX_FACE_NUM_ERR    (0x8A120212) // AlgParam.json或能力集结构体中能检测的最大人脸数量配置错误
#define ICF_ALGP_DETECT_RESOLUTION_MAX_ERR          (0x8A120213) // 大图或小图的分辨率超过了能力集

// FD_TRACK 0x8A120300~0x8A1203FF
#define ICF_ALGP_TRACK_STS_INIT_STATUS              (0x8A120300) // 结果状态位初始化状态
#define ICF_ALGP_TRACK_STS_IMG_PROC_ERR             (0x8A120301) // 结果状态位图像级错误
#define ICF_ALGP_TRACK_STS_MODULE_PROC_ERR          (0x8A120302) // 结果状态位模块级错误
#define ICF_ALGP_TRACK_NO_FACE                      (0x8A120310) // 跟踪人脸数目为零
#define ICF_ALGP_TRACK_FACE_NUM_ERR                 (0x8A120311) // 追踪的人脸数目大于最大值
#define ICF_ALGP_TRACK_VALID_FACE_NUM_ERR           (0x8A120312) // 跟踪有效的最大人脸数目错误
#define ICF_ALGP_TRACK_CONFIG_SET_ERR               (0x8A120313) // 设置Track配置错误
#define ICF_ALGP_TRACK_FRIST_SET_ERR                (0x8A120314) // 默认设置Track配置错误
#define ICF_ALGP_TRACK_CONFIG_GET_ERR               (0x8A120315) // 获取Track配置错误
#define ICF_ALGP_TRACK_NO_VL                        (0x8A120316) // 检测可见光失败，无法进行跟踪
#define ICF_ALGP_TRACK_CONFIG_GENERATE_RATE_ERR     (0x8A120317) // 跟踪配置中 生成速率 参数错误
#define ICF_ALGP_TRACK_CONFIG_SENSITIVITY_ERR       (0x8A120318) // 跟踪配置中 灵敏度 参数错误
#define ICF_ALGP_TRACK_CONFIG_RULE_LIST_ERR         (0x8A120319) // 跟踪配置中 规则链表 参数错误
#define ICF_ALGP_TRACK_RESOLUTION_ERR               (0x8A12031A) // 跟踪输入的图像分辨率错误（输入的宽高必须各自小于create时的宽高）
#define ICF_ALGP_TRACK_DET_IMG_FAIL                 (0x8A12031B) // 因为对应图像检测失败，导致跟踪无法进行

// FD_QUALITY 0x8A120400~0x8A1204FF 
#define ICF_ALGP_FD_QLTY_STS_INIT_STATUS            (0x8A120400) // 结果状态位初始化状态
#define ICF_ALGP_FD_QLTY_STS_IMG_PROC_ERR           (0x8A120401) // 结果状态位图像级错误
#define ICF_ALGP_FD_QLTY_STS_MODULE_PROC_ERR        (0x8A120402) // 结果状态位模块级错误
#define ICF_ALGP_FD_QLTY_STRUCT_SIZE_ERR            (0x8A120410) // 引擎与算法评分结构体大小不一致
#define ICF_ALGP_FD_QLTY_RESOLUTION_ERR             (0x8A120411) // fd评分输入的图像分辨率错误（输入的宽高必须各自小于create时的宽高）

// DFR_LANDMARK 0x8A120500~0x8A1205FF   
#define ICF_ALGP_LANDMARK_STS_INIT_STATUS           (0x8A120500) // 结果状态位初始化状态
#define ICF_ALGP_LANDMARK_STS_IMG_PROC_ERR          (0x8A120501) // 结果状态位图像级错误
#define ICF_ALGP_LANDMARK_STS_MODULE_PROC_ERR       (0x8A120502) // 结果状态位模块级错误
#define ICF_ALGP_LANDMARK_SELECT_FACE_NUM_ERR       (0x8A120510) // 用以筛选的人脸数设置错误（必须<=3）
#define ICF_ALGP_LANDMARK_SELECT_FACE_RECT_ERR      (0x8A120511) // 用以筛选的人脸框设置错误
#define ICF_ALGP_LANDMARK_NO_TRACK_VALID_FACE       (0x8A120512) // 跟踪有效人脸数目为零

// DFR_CALC_BRIGHT 0x8A120600~0x8A1206FF    
#define ICF_ALGP_CALC_BRIGHT_STS_INIT_STATUS        (0x8A120600) // 结果状态位初始化状态
#define ICF_ALGP_CALC_BRIGHT_STS_IMG_PROC_ERR       (0x8A120601) // 结果状态位图像级错误
#define ICF_ALGP_CALC_BRIGHT_STS_MODULE_PROC_ERR    (0x8A120602) // 结果状态位模块级错误
#define ICF_ALGP_GREY_VALUE_EXT                     (0x8A120610) // 图像灰度错误（外部算法）

// DFR_CALIB 0x8A120700~0x8A1207FF  
#define ICF_ALGP_CALIB_STS_INIT_STATUS              (0x8A120700) // 结果状态位初始化状态
#define ICF_ALGP_CALIB_STS_IMG_PROC_ERR             (0x8A120701) // 结果状态位图像级错误
#define ICF_ALGP_CALIB_STS_MODULE_PROC_ERR          (0x8A120702) // 结果状态位模块级错误
#define ICF_ALGP_FACE_MATCH_FAIL                    (0x8A120710) // 人脸匹配失败

// DFR_QUALITY 0x8A120800~0x8A1208FF
#define ICF_ALGP_DFR_QLTY_STS_INIT_STATUS           (0x8A120800) // 结果状态位初始化状态
#define ICF_ALGP_DFR_QLTY_STS_IMG_PROC_ERR          (0x8A120801) // 结果状态位图像级错误
#define ICF_ALGP_DFR_QLTY_STS_MODULE_PROC_ERR       (0x8A120802) // 结果状态位模块级错误
#define ICF_ALGP_DFR_QLTY_STS_STRUCT_SIZE_ERR       (0x8A120810) // 引擎与算法评分结构体大小不一致
#define ICF_ALGP_DFR_QLTY_STS_FACE_FILTER_ERR       (0x8A120811) // 人脸评分过滤失败 人脸被过滤
#define ICF_ALGP_DFR_QLTY_CONFIG_GET_ERR            (0x8A120812) // 获取dfr quality配置错误
#define ICF_ALGP_DFR_QLTY_CONFIG_CLASS_ERR          (0x8A120813) // 获取dfr quality配置的评分类别错误
#define ICF_ALGP_DFR_QLTY_LANDMARK_NUM_ERR          (0x8A120814) // 评分输入关键点数量错误
#define ICF_ALGP_DFR_QLTY_NO_VALID_FACE             (0x8A120815) // 评分输入无有效的人脸

// DFR_LIVENESS 0x8A120900~0x8A1209FF
#define ICF_ALGP_LIVENESS_STS_INIT_STATUS           (0x8A120900) // 结果状态位初始化状态
#define ICF_ALGP_LIVENESS_STS_IMG_PROC_ERR          (0x8A120901) // 结果状态位图像级错误
#define ICF_ALGP_LIVENESS_STS_MODULE_PROC_ERR       (0x8A120902) // 结果状态位模块级错误
#define ICF_ALGP_LIVENESS_KEY_LANDMARK_NUM_ERR      (0x8A120910) // 活体输入输出错误
#define ICF_ALGP_LIVENESS_LANDMARK_NUM_ERR          (0x8A120911) // 活体输入关键点数量错误
#define ICF_ALGP_LIVENESS_RESOLUTION_NOT_SUPPORT    (0x8A120912) // 活体图像分辨率不支持
#define ICF_ALGP_LIVENESS_FUNC_YUV2RGB_NULL         (0x8A120913) // 活体中yuv2rgb的注册函数为空
#define ICF_ALGP_LIVENESS_NO_VALID_FACE             (0x8A120914) // 活体输入无有效的人脸
#define ICF_ALGP_LIVENESS_TYPE_INVALID              (0x8A120915) // 活体类型无效，输入的活体类型没有初始化使能

// DFR_ATTRIBUTE 0x8A120A00~0x8A120AFF
#define ICF_ALGP_ATTRIBUTE_STS_INIT_STATUS          (0x8A120A00) // 结果状态位初始化状态
#define ICF_ALGP_ATTRIBUTE_STS_IMG_PROC_ERR         (0x8A120A01) // 结果状态位图像级错误
#define ICF_ALGP_ATTRIBUTE_STS_MODULE_PROC_ERR      (0x8A120A02) // 结果状态位模块级错误
#define ICF_ALGP_ATTRIBUTE_NO_VALID_FACE            (0x8A120A03) // 属性输入无有效的人脸
#define ICF_ALGP_ATTRIBUTE_GLASS_THRESHOLD_ERR      (0x8A120A04) // 配置眼镜置信度阈值错误      // todo pengbo ADD
#define ICF_ALGP_ATTRIBUTE_MASK_THRESHOLD_ERR       (0x8A120A05) // 配置口罩置信度阈值错误      // todo pengbo ADD
#define ICF_ALGP_ATTRIBUTE_HAT_THRESHOLD_ERR        (0x8A120A06) // 配置帽子置信度阈值错误      // todo pengbo ADD
#define ICF_ALGP_ATTRIBUTE_MANNER_THRESHOLD_ERR     (0x8A120A07) // 配置口罩规范置信度阈值错误  // todo pengbo ADD
#define ICF_ALGP_ATTRIBUTE_MASK_TYPE_THRESHOLD_ERR  (0x8A120A08) // 配置口罩类型置信度阈值错误  // todo pengbo ADD
#define ICF_ALGP_ATTRIBUTE_GENDER_THRESHOLD_ERR     (0x8A120A09) // 配置性别置信度阈值错误      // todo pengbo ADD
#define ICF_ALGP_ATTRIBUTE_EXPRESS_THRESHOLD_ERR    (0x8A120A10) // 配置表情置信度阈值错误      // todo pengbo ADD

// DFR_FEATURE 0x8A120B00~0x8A120BFF
#define ICF_ALGP_FEATURE_STS_INIT_STATUS            (0x8A120B00) // 结果状态位初始化状态
#define ICF_ALGP_FEATURE_STS_IMG_PROC_ERR           (0x8A120B01) // 结果状态位图像级错误
#define ICF_ALGP_FEATURE_STS_MODULE_PROC_ERR        (0x8A120B02) // 结果状态位模块级错误
#define ICF_ALGP_FEATURE_LANDMARK_NUM_ERR           (0x8A120B03) // 建模输入关键点数量错误
#define ICF_ALGP_FEATURE_NO_VALID_FACE              (0x8A120B04) // 建模输入无有效的人脸

// DFR_COMPARE 0x8A120C00~0x8A120CFF
#define ICF_ALGP_COMPARE_STS_INIT_STATUS            (0x8A120C00) // 结果状态位初始化状态
#define ICF_ALGP_COMPARE_STS_IMG_PROC_ERR           (0x8A120C01) // 结果状态位图像级错误
#define ICF_ALGP_COMPARE_STS_MODULE_PROC_ERR        (0x8A120C02) // 结果状态位模块级错误
#define ICF_ALGP_COMPARE_NODE_NOT_CREATE            (0x8A120C08) // 带有compare节点的业务线未创建
#define ICF_ALGP_COMPARE_VERSION_ERR                (0x8A120C10) // 模型版本错误
#define ICF_ALGP_COMPARE_PARAM_PATCH_NUM_ERR        (0x8A120C11) // AlgParam.json或能力集结构体中模型底库patch个数设置错误
#define ICF_ALGP_COMPARE_PARAM_FEAT_DIM_ERR         (0x8A120C12) // AlgParam.json或能力集结构体中模型底库feat_dim设置错误
#define ICF_ALGP_COMPARE_PARAM_HEAD_LEN_ERR         (0x8A120C13) // AlgParam.json或能力集结构体中模型底库head_length设置错误（必须为16字节）
#define ICF_ALGP_COMPARE_PARAM_FEAT_STRIDE_ERR      (0x8A120C14) // AlgParam.json或能力集结构体中模型底库stride设置错误（至少大于单patch的大小）
#define ICF_ALGP_COMPARE_PARAM_MAX_FEAT_NUM_ERR     (0x8A120C15) // AlgParam.json或能力集结构体中模型底库最大值错误（小于1000000）
#define ICF_ALGP_COMPARE_TOP_N_ERR                  (0x8A120C16) // AlgParam.json或能力集结构体中topN设置超过最大值或小于等于0
#define ICF_ALGP_COMPARE_CONFIG_REPO_NUM_ERR        (0x8A120C20) // Setconfig中的模型底库个数设置错误
#define ICF_ALGP_COMPARE_CONFIG_REPO_ADDR_NULL      (0x8A120C21) // Setconfig/1vN中的模型底库地址为空
#define ICF_ALGP_COMPARE_CONFIG_REPO_ITEM_NUM_ERR   (0x8A120C22) // Setconfig中的模型底库的元素个数大于AlgParam.json中设置的最大值
#define ICF_ALGP_COMPARE_CONFIG_REPO_STRIDE_ERR     (0x8A120C23) // Setconfig/1vN中的模型底库的stride设置错误，小于单patch大小（272字节）
#define ICF_ALGP_COMPARE_CONFIG_REPO_THRESH_ERR     (0x8A120C24) // Setconfig中的模型底库的相似度阈值sim_thresh设置错误
#define ICF_ALGP_COMPARE_CONFIG_MODEL_ADDR_NULL     (0x8A120C25) // Setconfig/1vN中的模型地址为空
#define ICF_ALGP_COMPARE_CONFIG_REPO_MASK_ID_ERR    (0x8A120C26) // Setconfig中的模型底库的mask_id设置错误
#define ICF_ALGP_COMPARE_REPO_NOT_SET               (0x8A120C30) // 模型底库未设置
#define ICF_ALGP_COMPARE_REPO_ADDR_NULL             (0x8A120C31) // 模型底库地址为空
#define ICF_ALGP_COMPARE_REPO_LOCK_NULL             (0x8A120C32) // 模型底库锁地址为空
#define ICF_ALGP_COMPARE_REPO_NUM_MAX_ERR           (0x8A120C33) // 模型底库个数存在错误
#define ICF_ALGP_COMPARE_REPO_MASK_ID_ERR           (0x8A120C34) // 模型底库mask_id配置存在错误
#define ICF_ALGP_COMPARE_REPO_NOT_MACTHING          (0x8A120C35) // 模型底库无匹配元素
#define ICF_ALGP_COMPARE_NO_VALID_FACE              (0x8A120C36) // 比对输入无有效的人脸特征
#define ICF_ALGP_COMPARE_LACK_MEMORY                (0x8A120C37) // 外部比对接口内存不足

// CLS_HELMET 0x8A120D00~0x8A120DFF
#define ICF_ALPG_HELMET_NOT_ENABLE                  (0x8A120D10) // 安全帽模块未启用，请创建对应业务线
#define ICF_ALGP_HELMET_RESOLUTION_NOT_SUPPORT      (0x8A120D11) // 检测所用图像分辨率不支持
#define ICF_ALGP_HELMET_ATTR_ERR                    (0x8A120D12) // 安全帽模块输出属性错误

// SELECT_FRAME 0x8A120E00~0x8A120EFF
#define ICF_ALPG_SELECT_FRAME_NOT_ENABLE            (0x8A120E10) // 选帧模块未启用，请创建对应业务线
#define ICF_ALPG_SELECT_FRAME_TRACK_NOT_ENABLE      (0x8A120E11) // 选帧模块未启用跟踪功能，请在输入数据时，开启跟踪功能
#define ICF_ALPG_SELECT_FRAME_ALGPARAM_ERR          (0x8A120E12) // 选帧模块AlgParam.json配置非法
#define ICF_ALPG_SELECT_FRAME_CONFIG_INVALID_KEY    (0x8A120E13) // 选帧模块配置非法的key值
#define ICF_ALPG_SELECT_FRAME_CONFIG_VALUE_ERR      (0x8A120E14) // 选帧模块setconfig配置参数错误
#define ICF_ALPG_SELECT_FRAME_FACE_PROC_TYPE_ERR    (0x8A120E15) // 选帧模式下只能配置为SAE_FACE_FACE_PROC_TYPE_MULTI_FACE

// IDR 0x8A120F00~0x8A120FFF
#define ICF_ALGP_IDR_INDATA_ROI_PARAM_ERR           (0x8A120F00) // 输入数据中roi_rect配置错误
#define ICF_ALGP_IDR_INDATA_PROC_MODU_ERR           (0x8A120F01) // 输入数据中proc_modu配置错误
#define ICF_ALGP_IDR_INDATA_RESOLUTION_MAX_ERR      (0x8A120F02) // 输入数据中分辨率大于能力集配置值
#define ICF_ALGP_IDR_INDATA_DATA_FORMAT_ERR         (0x8A120F03) // 输入数据中数据格式错误
#define ICF_ALGP_IDR_INDATA_DATA_PTR_NULL           (0x8A120F04) // 输入数据中数据指针为空
#define ICF_ALGP_IDR_INDATA_RESOLUTION_ERR          (0x8A120F05) // 输入图像宽高需32位对齐/step与w不相等

#define ICF_ALPG_IDR_MODU_ENABLE_ERR                (0x8A120F10) // 二维码检测模块 modu能力集 错误
#define ICF_ALPG_IDR_RESOLUTION_NOT_SUPPORT         (0x8A120F11) // 二维码检测模块 分辨率不支持
#define ICF_ALGP_IDR_NO_VALID_CODE                  (0x8A120F12) // 合法二维码数目为零
#define ICF_ALGP_IDR_ROI_INFO_SIZE_ERR              (0x8A120F13) // 二维码roi外置结构体与内置结构体大小不相等

#define ICF_ALGP_IDR_CODE_MAX_NUM_DIFFER            (0x8A120F14) // 引擎的最大条码处理数与算法限制不同
#define ICF_ALGP_IDR_CODE_MAX_NUM_NOT_SUPPORT       (0x8A120F15) // 能力集中最大处理条码数不支持
#define ICF_ALGP_IDR_INDATA_PROC_TYPE_ERR           (0x8A120F16) // 输入数据中处理模式不支持
#define ICF_ALGP_IDR_INDATA_RECT_NUM_ERR            (0x8A120F17) // 输入数据中检测框数量错误
#define ICF_ALGP_IDR_INDATA_PROC_NUM_ERR            (0x8A120F18) // 输入数据中读码条数错误（目前只支持单码识别）
#define ICF_ALGP_IDR_INDATA_ROI_NUM_ERR             (0x8A120F19) // 一致性测试时，外界输入检测框数据错误

#define ICF_ALGP_IDR_STS_NOT_RUN                    (0x8A120F20) // 模块未执行
#define ICF_ALGP_IDR_STS_INIT_STATUS                (0x8A120F21) // 结果状态位初始化状态
#define ICF_ALGP_IDR_CODE_ROI_INFO_DIFFER           (0x8A120F22) // 算法与引擎的ROI_INFO不同

// SAE 公共错误码 0x8A121800~0x8A1218FF
#define SAE_E_NODE_ID_DOES_NOT_EXIST                (0x8A121800) // 节点ID不存在
#define SAE_E_FUCN_MODULE_DOES_NOT_EXIST            (0x8A121801) // 功能模块不存在
#define SAE_E_PROC_TYPE_NOT_SUPPORT                 (0x8A121802) // 处理类型不支持
#define SAE_E_GRAPH_ID_DOES_NOT_EXIST               (0x8A121803) // 不存在的计算图
#define SAE_E_MODULE_NOT_CREATE                     (0x8A121804) // 模块未创建错误，检查能力集是否使能

/***************************************************************************************************
 * @name     结构化&车辆
 * @note     0x89400000~0x896FFFFF
***************************************************************************************************/
// 结构化，0x89400000~0x8940FFFF

// 无人车，0x89410000~0x8941FFFF


/***************************************************************************************************
 * @name     行业
 * @note     0x89700000~0x899FFFFF
***************************************************************************************************/
// 周界，0x89700000~0x8970FFFF

// 精准动线，0x89710000~0x8971FFFF
// IPP算子（示例）
#define BKTE_IPP_E_NULL_PTR                       (0x89710000) // 空指针
#define BKTE_IPP_E_MEM_ALLOC                      (0x89710010) // 内存分配错误
#define BKTE_IPP_E_MEM_SPACE                      (0x89710011) // 内存类型错误
#define BKTE_IPP_E_MEM_FREE                       (0x89710012) // 内存释放错误
#define BKTE_IPP_E_NO_YUV_IN                      (0x89710020) // package中找不到YUV送入
#define BKTE_IPP_E_NO_IPP_OUT                     (0x89710021) // package中找不到IPP输出内存
#define BKTE_IPP_E_PDATA_NULL_PTR                 (0x89710022) // 输入blob pdata指针为空
#define BKTE_IPP_E_MODEL_PATH_NULL                (0x89710030) // 模型文件路径为空
#define BKTE_IPP_E_OPEN_MODEL_FILE                (0x89710031) // 模型文件打开错误
#define BKTE_IPP_E_READ_MODEL_FILE                (0x89710032) // 模型文件读取错误

// 开放平台，0x89720000~0x8972FFFF

/***************************************************************************************************
 * @name     语音识别
 * @note     0x8A1A1000~0x8A1A1FFF
***************************************************************************************************/
// 通用
#define SVRE_MODU_ENABLE_ERR                   (0x8A1A1000)  // 能力集模块创建参数配置错误
#define SVRE_MODEL_PATH_ERR                    (0x8A1A1001)  // 模型路径错误
#define SVRE_CREATE_PARAM_ERR                  (0x8A1A1002)  // 能力集配置参数错误
#define SVRE_CREATE_ERR                        (0x8A1A1003)  // 创建失败或使用了未创建的模块
#define SVRE_PROC_TYPE_ERR                     (0x8A1A1004)  // 输入处理模式错误
#define SVRE_PROC_DATA_NULL                    (0x8A1A1005)  // 输入处理参数空指针
#define SVRE_PROC_DATA_ERR                     (0x8A1A1006)  // 输入处理参数错误

// 声纹识别
#define SVRE_FEAT_NUM_ERR                      (0x8A1A1101)  // 能力集底库最大值配置错误


#endif

