/***********************************************************************************************************************
* 版权信息：版权所有 (c) 2010-2027, 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：hcnn_error_code.h
* 摘    要：海思3559平台错误码
*
* 当前版本：0.8.0
* 作    者：刘锦胜
* 日    期：2018/03/13
* 备    注：
***********************************************************************************************************************/
#ifndef _HCNN_ERROR_CODE_H_
#define _HCNN_ERROR_CODE_H_


#define HCNN_ERR_OK                     (0)

// #define  CRY_PROC_ERROR           0x86ffffff  // 加密错误码

//HCNN错误码段0X86100000～0X86100FFF
#define HCNN_ERR_BASE                        (0x86100000)
#define HCNN_ERR_MEM_TAB_INVALID             (HCNN_ERR_BASE + 1)
#define HCNN_ERR_MEM_SPACE_DEFINE_ERROR      (HCNN_ERR_BASE + 2)
#define HCNN_ERR_MEM_ATTR_DEFINE_ERROR       (HCNN_ERR_BASE + 3)
#define HCNN_ERR_INPUT_NULL_POINTER          (HCNN_ERR_BASE + 4)
#define HCNN_ERR_INPUT_INVALID_VALUE         (HCNN_ERR_BASE + 5)
#define HCNN_ERR_VERSION_LIMITED             (HCNN_ERR_BASE + 6)
#define HCNN_ERR_VERSION_MISMATCH            (HCNN_ERR_BASE + 7)        // hcnn lib 和dsp bin版本不匹配
#define HCNN_ERR_PLATFORM_MISMATCH           (HCNN_ERR_BASE + 8)        // hcnn lib 和dsp bin平台不匹配
#define HCNN_ERR_SHVERSION_MISMATCH          (HCNN_ERR_BASE + 9)        // 调度器版本不正确
#define HCNN_ERR_SET_CONFIG_ERR              (HCNN_ERR_BASE + 10)       // setconfig不正确

//NNIE错误码段0X86100200～0X861003FF
#define HCNN_NNIE_ERR_BASE                   (0X86100200)
#define HCNN_NNIE_ERR_NULL_PTR               (HCNN_NNIE_ERR_BASE + 1)    //空指针
#define HCNN_NNIE_ERR_PARAM_VALUE            (HCNN_NNIE_ERR_BASE + 2)    //参数错误
#define HCNN_NNIE_ERR_MEM_LACK               (HCNN_NNIE_ERR_BASE + 3)    //内存不足
#define HCNN_NNIE_ERR_PRC_SIZE               (HCNN_NNIE_ERR_BASE + 4)    //process函数传入数据的大小错误
#define HCNN_NNIE_MODEL_PARAM_ERROR          (HCNN_NNIE_ERR_BASE + 5)    //模型参数错误
#define HCNN_NNIE_MODEL_FIRST_SEG_NOT_NNIE   (HCNN_NNIE_ERR_BASE + 6)    //第一段不是NNIE
#define HCNN_NNIE_MODEL_SEG_NUM_ERROR        (HCNN_NNIE_ERR_BASE + 7)    //分段段数错误
#define HCNN_NNIE_MODEL_SEG_BLOB_NUM_ERROR   (HCNN_NNIE_ERR_BASE + 8)    //段的输入输出BLOB个数出错
#define HCNN_NNIE_MODEL_NAME_TOO_LONG        (HCNN_NNIE_ERR_BASE + 9)    //模型type或name超过128个字节
#define HCNN_NNIE_NOT_SUPPORT_LAYER          (HCNN_NNIE_ERR_BASE + 10)   //库不支持的当前层
#define HCNN_NNIE_MODEL_NOT_FIND             (HCNN_NNIE_ERR_BASE + 11)   //传入模型中不包含海思量化wk模型
#define HCNN_NNIE_MODEL_SIZE_ERROR           (HCNN_NNIE_ERR_BASE + 12)   //传入模型大小错误
#define HCNN_NNIE_HISI_FUNC_ERROR            (HCNN_NNIE_ERR_BASE + 13)   //调用海思API函数出错
#define HCNN_NNIE_IN_BLOB_NUM_ERROR          (HCNN_NNIE_ERR_BASE + 14)   //外界配置的输入blob数错误
#define HCNN_NNIE_NET_INPUT_BLOB_N_ERROR     (HCNN_NNIE_ERR_BASE + 15)   //外界输入的blob的n错误
#define HCNN_NNIE_BLOB_FORMAT_NOT_SUPPORT    (HCNN_NNIE_ERR_BASE + 16)   //外界输入blob格式不支持
#define HCNN_NNIE_BLOB_DATA_CONVERT_ERROR    (HCNN_NNIE_ERR_BASE + 17)   //内部blob数据转换出错
#define HCNN_NNIE_PROC_IN_BLOB_ERROR         (HCNN_NNIE_ERR_BASE + 18)   //process函数输入blob数错误
#define HCNN_NNIE_PROC_OUT_BLOB_ERROR        (HCNN_NNIE_ERR_BASE + 19)   //process函数输出blob数错误
#define HCNN_NNIE_PROC_NNIE_ID_ERROR         (HCNN_NNIE_ERR_BASE + 20)   //nnie执行的id号错误
#define HCNN_NNIE_PROC_SEM_ERROR             (HCNN_NNIE_ERR_BASE + 21)   //process函数信号量操作失败
#define HCNN_NNIE_INIT_SEM_ERROR             (HCNN_NNIE_ERR_BASE + 22)   //信号量初始化错误
#define HCNN_NNIE_DESTROY_SEM_ERROR          (HCNN_NNIE_ERR_BASE + 23)   //信号量销毁错误
#define HCNN_NNIE_BLOB_NAME_TOO_LONG         (HCNN_NNIE_ERR_BASE + 24)   //blob名字超出规定范围，海思规定blob名字长度是32字节
#define HCNN_NNIE_BLOB_NAME_MAP_ERROR        (HCNN_NNIE_ERR_BASE + 25)   //blob通过名字比配错误
#define HCNN_NNIE_PROC_DSP_ID_ERROR          (HCNN_NNIE_ERR_BASE + 26)   //nnie执行私有层时dsp的id错误
#define HCNN_NNIE_EER_NOT_SUPPORT            (HCNN_NNIE_ERR_BASE + 27)   //未支持
#define HCNN_NNIE_LSTM_SEQ_BATCH_ERROR       (HCNN_NNIE_ERR_BASE + 28)   //LSTM网络的输入batch数超出范围
#define HCNN_NNIE_LSTM_DST_BLOB_NUM_ERROR    (HCNN_NNIE_ERR_BASE + 29)   //LSTM网络的输出blob个数需为1
#define HCNN_NNIE_MODEL_MEAN_ERROR           (HCNN_NNIE_ERR_BASE + 30)   //模型均值/Scale错误
#define HCNN_NNIE_GET_BBOX_BLOB_NUM          (HCNN_NNIE_ERR_BASE + 31)   // 获取Bbox blob num 错误
#define HCNN_NNIE_ERR_DEBUG                  (HCNN_NNIE_ERR_BASE + 32)   // nnie debug模式，运行错误

// NNIE FEAT RESHAPE
#define HCNN_NNIE_FEAT_RE_GET_MOD_SIZE  (HCNN_NNIE_ERR_BASE + 100)  // getmodelsize出错
#define HCNN_NNIE_FEAT_RE_CREATE_MOD    (HCNN_NNIE_ERR_BASE + 101)  // createmod 出错
#define HCNN_NNIE_FEAT_RE_GET_MEM_SIZE  (HCNN_NNIE_ERR_BASE + 102)  // getmemsize 出错
#define HCNN_NNIE_FEAT_RE_CREATE        (HCNN_NNIE_ERR_BASE + 103)  // creatre出错
#define HCNN_NNIE_FEAT_RE_FORWARD_PARAM (HCNN_NNIE_ERR_BASE + 104)  // forward中参数校验错误
#define HCNN_NNIE_FEAT_RE_FORWARD_NULL  (HCNN_NNIE_ERR_BASE + 105)  // forward中数据空指针

// NNIE FEAT RESHAPE
#define HCNN_NNIE_RESHAPE_GET_MOD_SIZE  (HCNN_NNIE_ERR_BASE + 120)  // getmodelsize出错
#define HCNN_NNIE_RESHAPE_CREATE_MOD    (HCNN_NNIE_ERR_BASE + 121)  // createmod 出错
#define HCNN_NNIE_RESHAPE_GET_MEM_SIZE  (HCNN_NNIE_ERR_BASE + 122)  // getmemsize 出错
#define HCNN_NNIE_RESHAPE_CREATE        (HCNN_NNIE_ERR_BASE + 123)  // creatre出错
#define HCNN_NNIE_RESHAPE_FORWARD_PARAM (HCNN_NNIE_ERR_BASE + 124)  // forward中参数校验错误
#define HCNN_NNIE_RESHAPE_FORWARD_NULL  (HCNN_NNIE_ERR_BASE + 125)  // forward中数据空指针

// NNIE FROUT
#define HCNN_NNIE_FROUT_GET_MOD_SIZE    (HCNN_NNIE_ERR_BASE + 106)  // getmodelsize出错
#define HCNN_NNIE_FROUT_CREATE_MOD      (HCNN_NNIE_ERR_BASE + 107)  // createmod 出错
#define HCNN_NNIE_FROUT_GET_MEM_SIZE    (HCNN_NNIE_ERR_BASE + 108)  // getmemsize 出错
#define HCNN_NNIE_FROUT_CREATE          (HCNN_NNIE_ERR_BASE + 109)  // creatre出错
#define HCNN_NNIE_FROUT_FORWARD_PARAM   (HCNN_NNIE_ERR_BASE + 110)  // forward中参数校验错误
#define HCNN_NNIE_FROUT_FORWARD_NULL    (HCNN_NNIE_ERR_BASE + 111)  // forward中数据空指针


//私有层YLOUT反馈码
#define HCNN_NNIE_ERR_YLOUT_GET_MODEL_SIZE_NULL_PTR          (HCNN_NNIE_ERR_BASE + 150)     //YLOUT_Get_Model_Size空指针
#define HCNN_NNIE_ERR_YLOUT_CREAT_MODEL_NULL_PTR             (HCNN_NNIE_ERR_BASE + 151)     //YLOUT_CREAT_Model空指针
#define HCNN_NNIE_ERR_YLOUT_MEN_BUFFER_OUT_RANGE             (HCNN_NNIE_ERR_BASE + 152)     //YLOUT_CREAT_Model内存超过范围
#define HCNN_NNIE_ERR_YLOUT_CREAT_INIT_MODEL_ERR             (HCNN_NNIE_ERR_BASE + 153)     //YLOUT_CREAT_Model参数错误
#define HCNN_NNIE_ERR_YLOUT_GET_MEN_SIZE_NULL_PTR            (HCNN_NNIE_ERR_BASE + 154)     //YLOUT_Get_Men_Size空指针
#define HCNN_NNIE_ERR_YLOUT_GET_MEN_SIZE_PARM_ERR            (HCNN_NNIE_ERR_BASE + 155)     //YLOUT_Get_Men_Size输入参数错误
#define HCNN_NNIE_ERR_YLOUT_GET_MEN_SIZE_RESHAPE_ERR         (HCNN_NNIE_ERR_BASE + 156)     //YLOUT_Get_Men_Size reshape错误
#define HCNN_NNIE_ERR_YLOUT_CREAT_NULL_PTR                   (HCNN_NNIE_ERR_BASE + 157)    //YLOUT_Great空指针
#define HCNN_NNIE_ERR_YLOUT_CREAT_PARM_ERR                   (HCNN_NNIE_ERR_BASE + 158)    //YLOUT_Great参数错误
#define HCNN_NNIE_ERR_YLOUT_CREAT_RESHAPE_PARM_ERR           (HCNN_NNIE_ERR_BASE + 159)    //YLOUT_Great reshape错误
#define HCNN_NNIE_ERR_YLOUT_FORWARD_PARA_ERR                 (HCNN_NNIE_ERR_BASE + 160)    //YLOUT_Forward参数错误 
#define HCNN_NNIE_ERR_YLOUT_FORWARD_PROCESS_ERR              (HCNN_NNIE_ERR_BASE + 161)    //YLOUT_Forward处理错误
#define HCNN_NNIE_ERR_YLOUT_MULTITASK_WK_SIZE                (HCNN_NNIE_ERR_BASE + 162)    //YLOUT 多任务输入blob尺寸错误

//私有层PROPOSAL层反馈码
#define HCNN_NNIE_ERR_PROPOSAL_GET_MODEL_SIZE_NULL_PTR          (HCNN_NNIE_ERR_BASE + 162)     //PROPOSAL_Get_Model_Size空指针
#define HCNN_NNIE_ERR_PROPOSAL_CREAT_MODEL_NULL_PTR             (HCNN_NNIE_ERR_BASE + 163)     //PROPOSAL_CREAT_Model空指针
#define HCNN_NNIE_ERR_PROPOSAL_MEN_BUFFER_OUT_RANGE             (HCNN_NNIE_ERR_BASE + 164)     //PROPOSAL_CREAT_Model内存超过范围
#define HCNN_NNIE_ERR_PROPOSAL_CREAT_INIT_MODEL_ERR             (HCNN_NNIE_ERR_BASE + 165)     //PROPOSAL_CREAT_Model参数错误
#define HCNN_NNIE_ERR_PROPOSAL_GET_MEN_SIZE_NULL_PTR            (HCNN_NNIE_ERR_BASE + 166)     //PROPOSAL_Get_Men_Size空指针
#define HCNN_NNIE_ERR_PROPOSAL_GET_MEN_SIZE_PARM_ERR            (HCNN_NNIE_ERR_BASE + 167)     //PROPOSAL_Get_Men_Size输入参数错误
#define HCNN_NNIE_ERR_PROPOSAL_GET_MEN_SIZE_RESHAPE_ERR         (HCNN_NNIE_ERR_BASE + 168)     //PROPOSAL_Get_Men_Size reshape错误
#define HCNN_NNIE_ERR_PROPOSAL_CREAT_NULL_PTR                   (HCNN_NNIE_ERR_BASE + 169)    //PROPOSAL_Great空指针
#define HCNN_NNIE_ERR_PROPOSAL_CREAT_PARM_ERR                   (HCNN_NNIE_ERR_BASE + 170)    //PROPOSAL_Great参数错误
#define HCNN_NNIE_ERR_PROPOSAL_CREAT_RESHAPE_PARM_ERR           (HCNN_NNIE_ERR_BASE + 171)    //PROPOSAL_Great reshape错误
#define HCNN_NNIE_ERR_PROPOSAL_GENE_BASE_ANCHORS_ERR            (HCNN_NNIE_ERR_BASE + 172)    //PROPOSAL_Great 产生基准框错误
#define HCNN_NNIE_ERR_PROPOSAL_FORWARD_PARA_ERR                 (HCNN_NNIE_ERR_BASE + 173)    //PROPOSAL_Forward参数错误 
#define HCNN_NNIE_ERR_PROPOSAL_FORWARD_PROCESS_ERR              (HCNN_NNIE_ERR_BASE + 174)  


// 私有层CROP层错误码
#define HCNN_NNIE_ERR_CROP_GET_MODEL_SIZE_NULL_PTR              (HCNN_NNIE_ERR_BASE + 175)     //CROP_Get_Model_Size空指针
#define HCNN_NNIE_ERR_CROP_CREAT_MODEL_NULL_PTR                 (HCNN_NNIE_ERR_BASE + 176)     //CROP_CREAT_Model空指针
#define HCNN_NNIE_ERR_CROP_MEN_BUFFER_OUT_RANGE                 (HCNN_NNIE_ERR_BASE + 177)     //CROP_CREAT_Model内存超过范围
#define HCNN_NNIE_ERR_CROP_CREAT_INIT_MODEL_ERR                 (HCNN_NNIE_ERR_BASE + 178)     //CROP_CREAT_Model参数错误
#define HCNN_NNIE_ERR_CROP_GET_MEN_SIZE_NULL_PTR                (HCNN_NNIE_ERR_BASE + 179)     //CROP_Get_Men_Size空指针
#define HCNN_NNIE_ERR_CROP_GET_MEN_SIZE_PARM_ERR                (HCNN_NNIE_ERR_BASE + 180)     //CROP_Get_Men_Size输入参数错误
#define HCNN_NNIE_ERR_CROP_GET_MEN_SIZE_RESHAPE_ERR             (HCNN_NNIE_ERR_BASE + 181)     //CROP_Get_Men_Size reshape错误
#define HCNN_NNIE_ERR_CROP_CREAT_NULL_PTR                       (HCNN_NNIE_ERR_BASE + 182)     //CROP_Great空指针
#define HCNN_NNIE_ERR_CROP_CREAT_PARM_ERR                       (HCNN_NNIE_ERR_BASE + 183)     //CROP_Great参数错误
#define HCNN_NNIE_ERR_CROP_CREAT_RESHAPE_PARM_ERR               (HCNN_NNIE_ERR_BASE + 184)     //CROP_Great reshape错误
#define HCNN_NNIE_ERR_CROP_FORWARD_PARA_ERR                     (HCNN_NNIE_ERR_BASE + 185)     //CROP_Forward参数错误 
#define HCNN_NNIE_ERR_CROP_FORWARD_PROCESS_ERR                  (HCNN_NNIE_ERR_BASE + 186)     //CROP_Forward处理错误

// 私有层YLPROPOSAL层错误码
#define HCNN_NNIE_ERR_YLPROPOSAL_GET_MODEL_SIZE_NULL_PTR              (HCNN_NNIE_ERR_BASE + 187)     //YLPROPOSAL_Get_Model_Size空指针
#define HCNN_NNIE_ERR_YLPROPOSAL_CREAT_MODEL_NULL_PTR                 (HCNN_NNIE_ERR_BASE + 188)     //YLPROPOSAL_CREAT_Model空指针
#define HCNN_NNIE_ERR_YLPROPOSAL_MEN_BUFFER_OUT_RANGE                 (HCNN_NNIE_ERR_BASE + 189)     //YLPROPOSAL_CREAT_Model内存超过范围
#define HCNN_NNIE_ERR_YLPROPOSAL_CREAT_INIT_MODEL_ERR                 (HCNN_NNIE_ERR_BASE + 190)     //YLPROPOSAL_CREAT_Model参数错误
#define HCNN_NNIE_ERR_YLPROPOSAL_GET_MEN_SIZE_NULL_PTR                (HCNN_NNIE_ERR_BASE + 191)     //YLPROPOSAL_Get_Men_Size空指针
#define HCNN_NNIE_ERR_YLPROPOSAL_GET_MEN_SIZE_PARM_ERR                (HCNN_NNIE_ERR_BASE + 192)     //YLPROPOSAL_Get_Men_Size输入参数错误
#define HCNN_NNIE_ERR_YLPROPOSAL_GET_MEN_SIZE_RESHAPE_ERR             (HCNN_NNIE_ERR_BASE + 193)     //YLPROPOSAL_Get_Men_Size reshape错误
#define HCNN_NNIE_ERR_YLPROPOSAL_CREAT_NULL_PTR                       (HCNN_NNIE_ERR_BASE + 194)     //YLPROPOSAL_Great空指针
#define HCNN_NNIE_ERR_YLPROPOSAL_CREAT_PARM_ERR                       (HCNN_NNIE_ERR_BASE + 195)     //YLPROPOSAL_Great参数错误
#define HCNN_NNIE_ERR_YLPROPOSAL_CREAT_RESHAPE_PARM_ERR               (HCNN_NNIE_ERR_BASE + 196)     //YLPROPOSAL_Great reshape错误
#define HCNN_NNIE_ERR_YLPROPOSAL_FORWARD_PARA_ERR                     (HCNN_NNIE_ERR_BASE + 197)     //YLPROPOSAL_Forward参数错误 
#define HCNN_NNIE_ERR_YLPROPOSAL_FORWARD_PROCESS_ERR                  (HCNN_NNIE_ERR_BASE + 198)     //YLPROPOSAL_Forward处理错误

// 私有层NMS层错误码
#define HCNN_NNIE_ERR_NMS_GET_MODEL_SIZE_NULL_PTR              (HCNN_NNIE_ERR_BASE + 199)     //NMS_Get_Model_Size空指针
#define HCNN_NNIE_ERR_NMS_CREAT_MODEL_NULL_PTR                 (HCNN_NNIE_ERR_BASE + 200)     //NMS_CREAT_Model空指针
#define HCNN_NNIE_ERR_NMS_MEN_BUFFER_OUT_RANGE                 (HCNN_NNIE_ERR_BASE + 201)     //NMS_CREAT_Model内存超过范围
#define HCNN_NNIE_ERR_NMS_CREAT_INIT_MODEL_ERR                 (HCNN_NNIE_ERR_BASE + 202)     //NMS_CREAT_Model参数错误
#define HCNN_NNIE_ERR_NMS_GET_MEN_SIZE_NULL_PTR                (HCNN_NNIE_ERR_BASE + 203)     //NMS_Get_Men_Size空指针
#define HCNN_NNIE_ERR_NMS_GET_MEN_SIZE_PARM_ERR                (HCNN_NNIE_ERR_BASE + 204)     //NMS_Get_Men_Size输入参数错误
#define HCNN_NNIE_ERR_NMS_GET_MEN_SIZE_RESHAPE_ERR             (HCNN_NNIE_ERR_BASE + 205)     //NMS_Get_Men_Size reshape错误
#define HCNN_NNIE_ERR_NMS_CREAT_NULL_PTR                       (HCNN_NNIE_ERR_BASE + 206)     //NMS_Great空指针
#define HCNN_NNIE_ERR_NMS_CREAT_PARM_ERR                       (HCNN_NNIE_ERR_BASE + 207)     //NMS_Great参数错误
#define HCNN_NNIE_ERR_NMS_CREAT_RESHAPE_PARM_ERR               (HCNN_NNIE_ERR_BASE + 208)     //NMS_Great reshape错误
#define HCNN_NNIE_ERR_NMS_FORWARD_PARA_ERR                     (HCNN_NNIE_ERR_BASE + 209)     //NMS_Forward参数错误 
#define HCNN_NNIE_ERR_NMS_FORWARD_PROCESS_ERR                  (HCNN_NNIE_ERR_BASE + 210)     //NMS_Forward处理错误

// 私有层YLOUT_PARK层错误码
#define HCNN_NNIE_ERR_YLOUT_PARK_GET_MODEL_SIZE_NULL_PTR          (HCNN_NNIE_ERR_BASE + 211)     //YLOUT_PARK_Get_Model_Size空指针
#define HCNN_NNIE_ERR_YLOUT_PARK_CREAT_MODEL_NULL_PTR             (HCNN_NNIE_ERR_BASE + 212)     //YLOUT_PARK_CREAT_Model空指针
#define HCNN_NNIE_ERR_YLOUT_PARK_MEM_BUFFER_OUT_RANGE             (HCNN_NNIE_ERR_BASE + 213)     //YLOUT_PARK_CREAT_Model内存超过范围
#define HCNN_NNIE_ERR_YLOUT_PARK_CREAT_INIT_MODEL_ERR             (HCNN_NNIE_ERR_BASE + 214)     //YLOUT_PARK_CREAT_Model参数错误
#define HCNN_NNIE_ERR_YLOUT_PARK_GET_MEM_SIZE_NULL_PTR            (HCNN_NNIE_ERR_BASE + 215)     //YLOUT_PARK_Get_Men_Size空指针
#define HCNN_NNIE_ERR_YLOUT_PARK_GET_MEM_SIZE_PARM_ERR            (HCNN_NNIE_ERR_BASE + 216)     //NMS_Get_Men_Size输入参数错误
#define HCNN_NNIE_ERR_YLOUT_PARK_GET_MEM_SIZE_RESHAPE_ERR         (HCNN_NNIE_ERR_BASE + 217)     //YLOUT_PARK_Get_Men_Size reshape错误
#define HCNN_NNIE_ERR_YLOUT_PARK_CREAT_NULL_PTR                   (HCNN_NNIE_ERR_BASE + 218)     //YLOUT_PARK_Great空指针
#define HCNN_NNIE_ERR_YLOUT_PARK_CREAT_PARM_ERR                   (HCNN_NNIE_ERR_BASE + 219)     //NMS_Great参数错误
#define HCNN_NNIE_ERR_YLOUT_PARK_CREAT_RESHAPE_PARM_ERR           (HCNN_NNIE_ERR_BASE + 220)     //YLOUT_PARK_Great reshape错误
#define HCNN_NNIE_ERR_YLOUT_PARK_FORWARD_PARA_ERR                 (HCNN_NNIE_ERR_BASE + 221)     //YLOUT_PARK_Forward参数错误 
#define HCNN_NNIE_ERR_YLOUT_PARK_FORWARD_PROCESS_ERR              (HCNN_NNIE_ERR_BASE + 222)     //YLOUT_PARK_Forward处理错误

// 私有层YLRBOX_PROPOSAL层错误码(旋转矩阵)
#define HCNN_NNIE_ERR_YLRBOX_PROPOSAL_GET_MODEL_SIZE_NULL_PTR     (HCNN_NNIE_ERR_BASE + 230)     // yolo旋转矩阵PROPASAL获取模型空指针
#define HCNN_NNIE_ERR_YLRBOX_PROPOSAL_CREAT_MODEL_NULL_PTR        (HCNN_NNIE_ERR_BASE + 231)     // yolo旋转矩阵PROPASAL创建模型空指针
#define HCNN_NNIE_ERR_YLRBOX_PROPOSAL_MEM_BUFFER_OUT_RANGE        (HCNN_NNIE_ERR_BASE + 232)     // yolo旋转矩阵PROPASAL分配内存超标
#define HCNN_NNIE_ERR_YLRBOX_PROPOSAL_CREAT_INIT_MODEL_ERR        (HCNN_NNIE_ERR_BASE + 233)     // yolo旋转矩阵PROPASAL初始化模型错误
#define HCNN_NNIE_ERR_YLRBOX_PROPOSAL_GET_MEN_SIZE_NULL_PTR       (HCNN_NNIE_ERR_BASE + 234)     // yolo旋转矩阵PROPASAL获取层内存空指针
#define HCNN_NNIE_ERR_YLRBOX_PROPOSAL_CREAT_NULL_PTR              (HCNN_NNIE_ERR_BASE + 235)     // yolo旋转矩阵PROPASAL创建层空指针
#define HCNN_NNIE_ERR_YLRBOX_PROPOSAL_FORWARD_PARA_ERR            (HCNN_NNIE_ERR_BASE + 236)     // yolo旋转矩阵PROPASALforward参数错误
#define HCNN_NNIE_ERR_YLRBOX_PROPOSAL_FORWARD_PROCESS_ERR         (HCNN_NNIE_ERR_BASE + 237)     // yolo旋转矩阵PROPASALforward失败
#define HCNN_NNIE_ERR_YLRBOX_PROPOSAL_HYPER_PARAM_GT_MAX          (HCNN_NNIE_ERR_BASE + 238)     // yolo旋转矩阵PROPASAL超参数>最大值

// 私有层YLRBOX_NMS层错误码(旋转矩阵)
#define HCNN_NNIE_ERR_YLRBOX_NMS_GET_MODEL_SIZE_NULL_PTR          (HCNN_NNIE_ERR_BASE + 239)     // yolo旋转矩阵NMS获取模型空指针
#define HCNN_NNIE_ERR_YLRBOX_NMS_CREAT_MODEL_NULL_PTR             (HCNN_NNIE_ERR_BASE + 240)     // yolo旋转矩阵NMS创建模型空指针
#define HCNN_NNIE_ERR_YLRBOX_NMS_MEM_BUFFER_OUT_RANGE             (HCNN_NNIE_ERR_BASE + 241)     // yolo旋转矩阵NMS分配内存超标
#define HCNN_NNIE_ERR_YLRBOX_NMS_CREAT_INIT_MODEL_ERR             (HCNN_NNIE_ERR_BASE + 242)     // yolo旋转矩阵NMS初始化模型错误
#define HCNN_NNIE_ERR_YLRBOX_NMS_GET_MEN_SIZE_NULL_PTR            (HCNN_NNIE_ERR_BASE + 243)     // yolo旋转矩阵NMS获取层内存空指针
#define HCNN_NNIE_ERR_YLRBOX_NMS_CREAT_NULL_PTR                   (HCNN_NNIE_ERR_BASE + 244)     // yolo旋转矩阵NMS创建层空指针
#define HCNN_NNIE_ERR_YLRBOX_NMS_FORWARD_PARA_ERR                 (HCNN_NNIE_ERR_BASE + 245)     // yolo旋转矩阵NMSforward参数错误
#define HCNN_NNIE_ERR_YLRBOX_NMS_FORWARD_PROCESS_ERR              (HCNN_NNIE_ERR_BASE + 246)     // yolo旋转矩阵NMSforward失败
#define HCNN_NNIE_ERR_YLRBOX_NMS_HYPER_PARAM_GT_MAX               (HCNN_NNIE_ERR_BASE + 247)     // yolo旋转矩阵NMS超参数>最大值

// 私有层east_out 错误码
#define HCNN_NNIE_EAST_OUT_BLOB_NUM                            (HCNN_NNIE_ERR_BASE + 248)  // getmodelsize出错
#define HCNN_NNIE_ERR_EAST_OUT_CREAT_MODEL_NULL_PTR            (HCNN_NNIE_ERR_BASE + 249)  // getmodelsize出错
#define HCNN_NNIE_ERR_EAST_OUT_GET_MODEL_SIZE_NULL_PTR         (HCNN_NNIE_ERR_BASE + 250)  // getmodelsize出错
#define HCNN_NNIE_EAST_OUT_CREATE_MOD                          (HCNN_NNIE_ERR_BASE + 251)  // createmod 出错
#define HCNN_NNIE_ERR_EAST_OUT_INIT_MODEL_ERR                  (HCNN_NNIE_ERR_BASE + 252)  // createmod 出错
#define HCNN_NNIE_EAST_OUT_GET_MEM_SIZE                        (HCNN_NNIE_ERR_BASE + 253)  // getmemsize 出错
#define HCNN_NNIE_ERR_EAST_OUT_GET_MEM_SIZE_NULL_PTR           (HCNN_NNIE_ERR_BASE + 254)     //YLOUT_PARK_Get_Men_Size空指针
#define HCNN_NNIE_EAST_OUT_CREATE                              (HCNN_NNIE_ERR_BASE + 255)  // creatre出错
#define HCNN_NNIE_ERR_EAST_OUT_CREAT_NULL_PTR                  (HCNN_NNIE_ERR_BASE + 256)     //YLOUT_PARK_Great空指针
#define HCNN_NNIE_EAST_OUT_FORWARD_PARAM                       (HCNN_NNIE_ERR_BASE + 257)  // forward中参数校验错误
#define HCNN_NNIE_EAST_OUT_FORWARD_NULL                        (HCNN_NNIE_ERR_BASE + 258)  // forward中数据空指针

// 私有层 resize 错误码
#define HCNN_NNIE_RESIZE_GET_MOD_SIZE                          (HCNN_NNIE_ERR_BASE + 259)  // getmodelsize出错
#define HCNN_NNIE_RESIZE_CREATE_MOD                            (HCNN_NNIE_ERR_BASE + 260)  // createmod 出错
#define HCNN_NNIE_RESIZE_GET_MEM_SIZE                          (HCNN_NNIE_ERR_BASE + 261)  // getmemsize 出错
#define HCNN_NNIE_RESIZE_CREATE                                (HCNN_NNIE_ERR_BASE + 262)  // creatre出错
#define HCNN_NNIE_RESIZE_FORWARD_PARAM                         (HCNN_NNIE_ERR_BASE + 263)  // forward中参数校验错误
#define HCNN_NNIE_RESIZE_FORWARD_NULL                          (HCNN_NNIE_ERR_BASE + 264)  // forward中数据空指针

// 私有层 TRANSPARENT 错误码
#define HCNN_NNIE_TRANSPARENT_GET_MOD_SIZE  (HCNN_NNIE_ERR_BASE + 265)  // getmodelsize出错
#define HCNN_NNIE_TRANSPARENT_CREATE_MOD    (HCNN_NNIE_ERR_BASE + 266)  // createmod 出错
#define HCNN_NNIE_TRANSPARENT_GET_MEM_SIZE  (HCNN_NNIE_ERR_BASE + 267)  // getmemsize 出错
#define HCNN_NNIE_TRANSPARENT_CREATE        (HCNN_NNIE_ERR_BASE + 268)  // creatre出错
#define HCNN_NNIE_TRANSPARENT_FORWARD_PARAM (HCNN_NNIE_ERR_BASE + 269)  // forward中参数校验错误
#define HCNN_NNIE_TRANSPARENT_FORWARD_NULL  (HCNN_NNIE_ERR_BASE + 270)  // forward中数据空指针
#define HCNN_NNIE_TRANSPARENT_IN_ROI_NUM    (HCNN_NNIE_ERR_BASE + 271)  // 输入ROI个数超过阈值

// 私有层 kpnporposal 错误码
#define HCNN_NNIE_KPNPROPOSAL_GET_MOD_SIZE  (HCNN_NNIE_ERR_BASE + 272)  // getmodelsize出错
#define HCNN_NNIE_KPNPROPOSAL_CREATE_MOD    (HCNN_NNIE_ERR_BASE + 273)  // createmod 出错
#define HCNN_NNIE_KPNPROPOSAL_GET_MEM_SIZE  (HCNN_NNIE_ERR_BASE + 274)  // getmemsize 出错
#define HCNN_NNIE_KPNPROPOSAL_CREATE        (HCNN_NNIE_ERR_BASE + 275)  // creatre出错
#define HCNN_NNIE_KPNPROPOSAL_FORWARD_PARAM (HCNN_NNIE_ERR_BASE + 276)  // forward中参数校验错误
#define HCNN_NNIE_KPNPROPOSAL_FORWARD_NULL  (HCNN_NNIE_ERR_BASE + 277)  // forward中数据空指针

// 私有层 kpnout 错误码
#define HCNN_NNIE_KPNOUT_GET_MOD_SIZE  (HCNN_NNIE_ERR_BASE + 278)  // getmodelsize出错
#define HCNN_NNIE_KPNOUT_CREATE_MOD    (HCNN_NNIE_ERR_BASE + 279)  // createmod 出错
#define HCNN_NNIE_KPNOUT_GET_MEM_SIZE  (HCNN_NNIE_ERR_BASE + 280)  // getmemsize 出错
#define HCNN_NNIE_KPNOUT_CREATE        (HCNN_NNIE_ERR_BASE + 281)  // creatre出错
#define HCNN_NNIE_KPNOUT_FORWARD_PARAM (HCNN_NNIE_ERR_BASE + 282)  // forward中参数校验错误
#define HCNN_NNIE_KPNOUT_FORWARD_NULL  (HCNN_NNIE_ERR_BASE + 283)  // forward中数据空指针

// 私有层 roi align 错误码
#define HCNN_NNIE_ROI_ALIGN_GET_MODEL_SIZE_NULL_PTR     (HCNN_NNIE_ERR_BASE + 284)     // roi_align层获取模型空指针
#define HCNN_NNIE_ROI_ALIGN_CREAT_MODEL_NULL_PTR        (HCNN_NNIE_ERR_BASE + 285)     // roi_align层创建模型空指针
#define HCNN_NNIE_ROI_ALIGN_MEM_BUFFER_OUT_RANGE        (HCNN_NNIE_ERR_BASE + 286)     // roi_align层分配内存超标
#define HCNN_NNIE_ROI_ALIGN_CREAT_INIT_MODEL_ERR        (HCNN_NNIE_ERR_BASE + 287)     // roi_align层初始化模型错误
#define HCNN_NNIE_ROI_ALIGN_GET_MEM_SIZE_NULL_PTR       (HCNN_NNIE_ERR_BASE + 288)     // roi_align层获取层内存空指针
#define HCNN_NNIE_ROI_ALIGN_CREAT_NULL_PTR              (HCNN_NNIE_ERR_BASE + 289)     // roi_align层创建层空指针
#define HCNN_NNIE_ROI_ALIGN_FORWARD_PARA_ERR            (HCNN_NNIE_ERR_BASE + 290)     // roi_align层forward参数错误
#define HCNN_NNIE_ROI_ALIGN_FORWARD_PROCESS_ERR         (HCNN_NNIE_ERR_BASE + 291)     // roi_align层forward失败

// 私有层 yl_keypoint_proposal 错误码
#define HCNN_NNIE_YL_KP_PROPOSAL_GET_MODEL_SIZE_NULL_PTR     (HCNN_NNIE_ERR_BASE + 292)     // yl_kp_proposal层获取模型空指针
#define HCNN_NNIE_YL_KP_PROPOSAL_CREAT_MODEL_NULL_PTR        (HCNN_NNIE_ERR_BASE + 293)     // yl_kp_proposal层创建模型空指针
#define HCNN_NNIE_YL_KP_PROPOSAL_MEM_BUFFER_OUT_RANGE        (HCNN_NNIE_ERR_BASE + 294)     // yl_kp_proposal层分配内存超标
#define HCNN_NNIE_YL_KP_PROPOSAL_CREAT_INIT_MODEL_ERR        (HCNN_NNIE_ERR_BASE + 295)     // yl_kp_proposal层初始化模型错误
#define HCNN_NNIE_YL_KP_PROPOSAL_GET_MEM_SIZE_NULL_PTR       (HCNN_NNIE_ERR_BASE + 296)     // yl_kp_proposal层获取层内存空指针
#define HCNN_NNIE_YL_KP_PROPOSAL_CREAT_NULL_PTR              (HCNN_NNIE_ERR_BASE + 297)     // yl_kp_proposal层创建层空指针
#define HCNN_NNIE_YL_KP_PROPOSAL_FORWARD_PARA_ERR            (HCNN_NNIE_ERR_BASE + 298)     // yl_kp_proposal层forward参数错误
#define HCNN_NNIE_YL_KP_PROPOSAL_FORWARD_PROCESS_ERR         (HCNN_NNIE_ERR_BASE + 299)     // yl_kp_proposal层forward失败

// 私有层 Eltwise max 错误码
#define HCNN_NNIE_NEIGHBOR_ELTWISE_GET_MODEL_SIZE_NULL_PTR       (HCNN_NNIE_ERR_BASE + 300)     // NEIGHBOR ELTWISE层获取模型空指针
#define HCNN_NNIE_NEIGHBOR_ELTWISE_CREAT_MODEL_NULL_PTR          (HCNN_NNIE_ERR_BASE + 301)     // NEIGHBOR ELTWISE层创建模型空指针
#define HCNN_NNIE_NEIGHBOR_ELTWISE_GET_MEM_SIZE_NULL_PTR         (HCNN_NNIE_ERR_BASE + 302)     // NEIGHBOR ELTWISE层获取层内存空指针
#define HCNN_NNIE_NEIGHBOR_ELTWISE_GET_MEM_SIZE_RESHAPE_PARM_ERR (HCNN_NNIE_ERR_BASE + 303)     // NEIGHBOR ELTWISE层getmemsize函数reshape错误
#define HCNN_NNIE_NEIGHBOR_ELTWISE_CREAT_NULL_PTR                (HCNN_NNIE_ERR_BASE + 304)     // NEIGHBOR ELTWISE层create空指针
#define HCNN_NNIE_NEIGHBOR_ELTWISE_CREAT_RESHAPE_PARM_ERR        (HCNN_NNIE_ERR_BASE + 305)     // NEIGHBOR ELTWISE层create函数reshape错误
#define HCNN_NNIE_NEIGHBOR_ELTWISE_FORWARD_NULL_PTR              (HCNN_NNIE_ERR_BASE + 306)     // NEIGHBOR ELTWISE层forward空指针

// 私有层 kyp_pos 错误码
#define HCNN_NNIE_ERR_KYP_POS_GET_MODEL_SIZE_NULL_PTR     (HCNN_NNIE_ERR_BASE + 307)     // kyp_pos层获取模型空指针
#define HCNN_NNIE_ERR_KYP_POS_CREAT_MODEL_NULL_PTR        (HCNN_NNIE_ERR_BASE + 308)     // kyp_pos层创建模型空指针
#define HCNN_NNIE_ERR_KYP_POS_MEM_BUFFER_OUT_RANGE        (HCNN_NNIE_ERR_BASE + 309)     // kyp_pos层分配内存超标
#define HCNN_NNIE_ERR_KYP_POS_CREAT_INIT_MODEL_ERR        (HCNN_NNIE_ERR_BASE + 311)     // kyp_pos层初始化模型错误
#define HCNN_NNIE_ERR_KYP_POS_GET_MEM_SIZE_NULL_PTR       (HCNN_NNIE_ERR_BASE + 312)     // kyp_pos层获取层内存空指针
#define HCNN_NNIE_ERR_KYP_POS_CREAT_NULL_PTR              (HCNN_NNIE_ERR_BASE + 313)     // kyp_pos层创建层空指针
#define HCNN_NNIE_ERR_KYP_POS_FORWARD_PARA_ERR            (HCNN_NNIE_ERR_BASE + 314)     // kyp_pos层forward参数错误
#define HCNN_NNIE_ERR_KYP_POS_FORWARD_PROCESS_ERR         (HCNN_NNIE_ERR_BASE + 315)     // kyp_pos层forward失败

// 私有层 COLLECT_DISTRIBUTE_FPN_RPN_PROPOSAL 错误码
#define HCNN_NNIE_ERR_COLLECT_DISTRIBUTE_GET_MODEL_SIZE_NULL_PTR     (HCNN_NNIE_ERR_BASE + 316)     // COLLECT_DISTRIBUTE层获取模型空指针
#define HCNN_NNIE_ERR_COLLECT_DISTRIBUTE_CREAT_MODEL_NULL_PTR        (HCNN_NNIE_ERR_BASE + 317)     // COLLECT_DISTRIBUTE层创建模型空指针
#define HCNN_NNIE_ERR_COLLECT_DISTRIBUTE_MEM_BUFFER_OUT_RANGE        (HCNN_NNIE_ERR_BASE + 318)     // COLLECT_DISTRIBUTE层分配内存超标
#define HCNN_NNIE_ERR_COLLECT_DISTRIBUTE_CREAT_INIT_MODEL_ERR        (HCNN_NNIE_ERR_BASE + 319)     // COLLECT_DISTRIBUTE层初始化模型错误
#define HCNN_NNIE_ERR_COLLECT_DISTRIBUTE_GET_MEM_SIZE_NULL_PTR       (HCNN_NNIE_ERR_BASE + 320)     // COLLECT_DISTRIBUTE层获取层内存空指针
#define HCNN_NNIE_ERR_COLLECT_DISTRIBUTE_CREAT_NULL_PTR              (HCNN_NNIE_ERR_BASE + 321)     // COLLECT_DISTRIBUTE层创建层空指针
#define HCNN_NNIE_ERR_COLLECT_DISTRIBUTE_FORWARD_PARA_ERR            (HCNN_NNIE_ERR_BASE + 322)     // COLLECT_DISTRIBUTE层forward参数错误
#define HCNN_NNIE_ERR_COLLECT_DISTRIBUTE_FORWARD_PROCESS_ERR         (HCNN_NNIE_ERR_BASE + 323)     // COLLECT_DISTRIBUTE层forward失败

// MASKRCNN_POSTPROCESS 错误码
#define HCNN_NNIE_ERR_MR_PP_GET_MODEL_SIZE_NULL_PTR             (HCNN_NNIE_ERR_BASE + 324)      // MASKRCNN_POSTPROCESS获取模型大小错误
#define HCNN_NNIE_ERR_MR_PP_CREATE_MODEL_NULL_PTR               (HCNN_NNIE_ERR_BASE + 325)      // MASKRCNN_POSTPROCESS创建模型空指针
#define HCNN_NNIE_ERR_MR_PP_CREATE_MODEL                        (HCNN_NNIE_ERR_BASE + 326)      // MASKRCNN_POSTPROCESS创建模型错误
#define HCNN_NNIE_ERR_MR_PP_INIT_MODEL_ERR                      (HCNN_NNIE_ERR_BASE + 327)      // MASKRCNN_POSTPROCESS初始化模型错误
#define HCNN_NNIE_ERR_MR_PP_GET_MEM_SIZE_NULL_PTR               (HCNN_NNIE_ERR_BASE + 328)      // MASKRCNN_POSTPROCESS获取网络内存空指针
#define HCNN_NNIE_ERR_MR_PP_COMPUTE_SHAPE                       (HCNN_NNIE_ERR_BASE + 329)      // MASKRCNN_POSTPROCESS计算shape错误
#define HCNN_NNIE_ERR_MR_PP_CREATE_NULL_PTR                     (HCNN_NNIE_ERR_BASE + 330)      // MASKRCNN_POSTPROCESS创建网络空指针
#define HCNN_NNIE_ERR_MR_PP_CREATE                              (HCNN_NNIE_ERR_BASE + 331)      // MASKRCNN_POSTPROCESS创建错误
#define HCNN_NNIE_ERR_MR_PP_FORWARD                             (HCNN_NNIE_ERR_BASE + 332)      // MASKRCNN_POSTPROCESS forward错误
#define HCNN_NNIE_ERR_MR_PP_FORWARD_NULL_PTR                    (HCNN_NNIE_ERR_BASE + 333)      // MASKRCNN_POSTPROCESS forward空指针
#define HCNN_NNIE_ERR_MR_PP_DATA_CONVERT                        (HCNN_NNIE_ERR_BASE + 334)      // MASKRCNN_POSTPROCESS数据转换错误
#define HCNN_NNIE_ERR_MR_PP_OBJ_NUM_NOTMATCH                    (HCNN_NNIE_ERR_BASE + 335)      // MASKRCNN_POSTPROCESS结果数量不匹配

#define HCNN_NNIE_ERR_RPN_AP_GET_MODEL_SIZE_NULL_PTR    (HCNN_NNIE_ERR_BASE + 336)     // 获取模型内存空指针
#define HCNN_NNIE_ERR_RPN_AP_CREATE_MODEL_NULL_PTR      (HCNN_NNIE_ERR_BASE + 337)     // 创建模型空指针
#define HCNN_NNIE_ERR_RPN_AP_CREATE_MODEL               (HCNN_NNIE_ERR_BASE + 338)     // 创建模型错误
#define HCNN_NNIE_ERR_RPN_AP_INIT_MODEL_ERR             (HCNN_NNIE_ERR_BASE + 339)     // 初始化模型错误
#define HCNN_NNIE_ERR_RPN_AP_INIT_MODEL_PARAM_ERR       (HCNN_NNIE_ERR_BASE + 340)     // 初始化模型错误
#define HCNN_NNIE_ERR_RPN_AP_GET_MEM_SIZE_NULL_PTR      (HCNN_NNIE_ERR_BASE + 341)     // 获取层内存空指针
#define HCNN_NNIE_ERR_RPN_AP_COMPUTE_IN_OUT_SHAPE       (HCNN_NNIE_ERR_BASE + 342)     // 计算输入输出大小错误
#define HCNN_NNIE_ERR_RPN_AP_CREAT_NULL_PTR             (HCNN_NNIE_ERR_BASE + 343)     // 创建层空指针
#define HCNN_NNIE_ERR_RPN_AP_CREAT_GENERATE_PTR         (HCNN_NNIE_ERR_BASE + 344)     // 创建anchor错误
#define HCNN_NNIE_ERR_RPN_AP_FORWARD_NULL_PTR           (HCNN_NNIE_ERR_BASE + 345)     // forward空指针
#define HCNN_NNIE_ERR_RPN_AP_FORWARD                    (HCNN_NNIE_ERR_BASE + 346)     // forward参数错误
#define HCNN_NNIE_ERR_NMS_ATTR_GET_MODEL_SIZE_NULL_PTR  (HCNN_NNIE_ERR_BASE + 347)     // 获取模型内存空指针
#define HCNN_NNIE_ERR_NMS_ATTR_CREATE_MODEL_NULL_PTR    (HCNN_NNIE_ERR_BASE + 348)     // 创建模型空指针
#define HCNN_NNIE_ERR_NMS_ATTR_CREATE_MODEL             (HCNN_NNIE_ERR_BASE + 349)     // 创建模型错误
#define HCNN_NNIE_ERR_NMS_ATTR_INIT_MODEL_ERR           (HCNN_NNIE_ERR_BASE + 350)     // 初始化模型错误
#define HCNN_NNIE_ERR_NMS_ATTR_GET_MEM_SIZE_NULL_PTR    (HCNN_NNIE_ERR_BASE + 351)     // 获取层内存空指针
#define HCNN_NNIE_ERR_NMS_ATTR_CHECK_INPUT_NULL_PTR     (HCNN_NNIE_ERR_BASE + 352)     // 获取层内存空指针
#define HCNN_NNIE_ERR_NMS_ATTR_PARAM_VALUE              (HCNN_NNIE_ERR_BASE + 353)     // 获取层内存空指针
#define HCNN_NNIE_ERR_NMS_ATTR_CREAT_NULL_PTR           (HCNN_NNIE_ERR_BASE + 354)     // 创建层空指针
#define HCNN_NNIE_ERR_NMS_ATTR_CREAT_GENERATE_PTR       (HCNN_NNIE_ERR_BASE + 355)     // 创建anchor错误
#define HCNN_NNIE_ERR_NMS_ATTR_FORWARD_PARA_PTR         (HCNN_NNIE_ERR_BASE + 356)     // forward空指针
#define HCNN_NNIE_ERR_NMS_ATTR_FORWARD_PROCESS_PTR      (HCNN_NNIE_ERR_BASE + 357)     // forward空指针
#define HCNN_NNIE_ERR_NMS_ATTR_NULL_PTR                 (HCNN_NNIE_ERR_BASE + 358)     // forward空指针

//私有层YOLO_LAND
#define HCNN_NNIE_EER_YLOUT_LAND_FORWARD_PARA           (HCNN_NNIE_ERR_BASE + 359)       // YLOUT_Forward参数错误 
#define HCNN_NNIE_EER_YLOUT_LAND_FORWARD_PROCESS        (HCNN_NNIE_ERR_BASE + 360)       // YLOUT_Forward处理错误
#define HCNN_NNIE_YLOUT_LAND_MODEL_ERROR                (HCNN_NNIE_ERR_BASE + 361)       // YLOUT普通参数校验失败
#define HCNN_NNIE_YLOUT_LAND_HYPER_ERROR                (HCNN_NNIE_ERR_BASE + 362)       // YLOUT超参解析出错
#define HCNN_NNIE_YLOUT_LAND_NULL_ERROR                 (HCNN_NNIE_ERR_BASE + 363)       // YLOUT输入参数为空
#define HCNN_NNIE_YLOUT_LAND_MEM_ERROR                  (HCNN_NNIE_ERR_BASE + 364)       // YLOUT内存分配出错
#define HCNN_NNIE_YLOUT_MEM_ERROR                       (HCNN_NNIE_ERR_BASE + 365)       // YLOUT内存分配出错
#define HCNN_NNIE_YLOUT_EER_NO_ENOUGH_MEM               (HCNN_NNIE_ERR_BASE + 366)       // YLOUT内存分配出错
#define HCNN_NNIE_YLOUT_EER_NULL_PTR                    (HCNN_NNIE_ERR_BASE + 367)       // YLOUT内存分配出错

// RPN_PROPOSAL_WITH_THRESH 错误码
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_GET_MODEL_SIZE_NULL_PTR          (HCNN_NNIE_ERR_BASE + 368)     //RPN_PROPOSAL_WITH_THRESH_Get_Model_Size空指针
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_CREAT_MODEL_NULL_PTR             (HCNN_NNIE_ERR_BASE + 369)     //RPN_PROPOSAL_WITH_THRESH_CREAT_Model空指针
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_MEN_BUFFER_OUT_RANGE             (HCNN_NNIE_ERR_BASE + 370)     //RPN_PROPOSAL_WITH_THRESH_CREAT_Model内存超过范围
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_CREAT_INIT_MODEL_ERR             (HCNN_NNIE_ERR_BASE + 371)     //RPN_PROPOSAL_WITH_THRESH_CREAT_Model参数错误
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_GET_MEN_SIZE_NULL_PTR            (HCNN_NNIE_ERR_BASE + 372)     //RPN_PROPOSAL_WITH_THRESH_Get_Men_Size空指针
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_GET_MEN_SIZE_PARM_ERR            (HCNN_NNIE_ERR_BASE + 373)     //RPN_PROPOSAL_WITH_THRESH_Get_Men_Size输入参数错误
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_GET_MEN_SIZE_RESHAPE_ERR         (HCNN_NNIE_ERR_BASE + 374)     //RPN_PROPOSAL_WITH_THRESH_Get_Men_Size reshape错误
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_CREAT_NULL_PTR                   (HCNN_NNIE_ERR_BASE + 375)     //RPN_PROPOSAL_WITH_THRESH_Great空指针
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_CREAT_PARM_ERR                   (HCNN_NNIE_ERR_BASE + 376)     //RPN_PROPOSAL_WITH_THRESH_Great参数错误
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_CREAT_RESHAPE_PARM_ERR           (HCNN_NNIE_ERR_BASE + 377)     //RPN_PROPOSAL_WITH_THRESH_Great reshape错误
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_GENE_BASE_ANCHORS_ERR            (HCNN_NNIE_ERR_BASE + 378)     //RPN_PROPOSAL_WITH_THRESH_Great 产生基准框错误
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_FORWARD_PARA_ERR                 (HCNN_NNIE_ERR_BASE + 379)     //RPN_PROPOSAL_WITH_THRESH_Forward参数错误 
#define HCNN_NNIE_ERR_RPN_PROPOSAL_WITH_THRESH_FORWARD_PROCESS_ERR              (HCNN_NNIE_ERR_BASE + 380)  

// 私有层 ROTATE_ROI_ALIGN 错误码
#define HCNN_NNIE_ERR_ROTATE_ROI_ALIGN_GET_MODEL_SIZE_NULL_PTR      (HCNN_NNIE_ERR_BASE + 381)     // 获取模型内存空指针
#define HCNN_NNIE_ERR_ROTATE_ROI_ALIGN_CREAT_MODEL_NULL_PTR         (HCNN_NNIE_ERR_BASE + 382)     // 创建模型空指针
#define HCNN_NNIE_ERR_ROTATE_ROI_ALIGN_CREAT_MODEL                  (HCNN_NNIE_ERR_BASE + 383)     // 创建模型错误
#define HCNN_NNIE_ERR_ROTATE_ROI_ALIGN_INIT_MODEL_ERR               (HCNN_NNIE_ERR_BASE + 384)     // 初始化模型错误
#define HCNN_NNIE_ERR_ROTATE_ROI_ALIGN_GET_MEM_SIZE_NULL_PTR        (HCNN_NNIE_ERR_BASE + 385)     // 获取层内存空指针
#define HCNN_NNIE_ERR_ROTATE_ROI_ALIGN_COMPUTE_IN_OUT_SHAPE         (HCNN_NNIE_ERR_BASE + 386)     // 计算输入输出大小错误
#define HCNN_NNIE_ERR_ROTATE_ROI_ALIGN_CREAT_NULL_PTR               (HCNN_NNIE_ERR_BASE + 387)     // 创建层空指针
#define HCNN_NNIE_ERR_ROTATE_ROI_ALIGN_FORWARD_NULL_PTR             (HCNN_NNIE_ERR_BASE + 388)     // forward空指针
#define HCNN_NNIE_ERR_ROTATE_ROI_ALIGN_FORWARD_PARAM                (HCNN_NNIE_ERR_BASE + 389)     // forward参数错误
#define HCNN_NNIE_ERR_ROTATE_ROI_ALIGN_MEM_BUFFER_OUT_RANGE         (HCNN_NNIE_ERR_BASE + 390)     // 辅助内存空间不足

// 私有层 FRCN_ROTATE_OUTPUT 错误码
#define HCNN_NNIE_ERR_FRCN_ROTATE_OUTPUT_GET_MODEL_SIZE_NULL_PTR      (HCNN_NNIE_ERR_BASE + 391)     // 获取模型内存空指针
#define HCNN_NNIE_ERR_FRCN_ROTATE_OUTPUT_CREAT_MODEL_NULL_PTR         (HCNN_NNIE_ERR_BASE + 392)     // 创建模型空指针
#define HCNN_NNIE_ERR_FRCN_ROTATE_OUTPUT_CREAT_MODEL                  (HCNN_NNIE_ERR_BASE + 393)     // 创建模型错误
#define HCNN_NNIE_ERR_FRCN_ROTATE_OUTPUT_INIT_MODEL_ERR               (HCNN_NNIE_ERR_BASE + 394)     // 初始化模型错误
#define HCNN_NNIE_ERR_FRCN_ROTATE_OUTPUT_GET_MEM_SIZE_NULL_PTR        (HCNN_NNIE_ERR_BASE + 395)     // 获取层内存空指针
#define HCNN_NNIE_ERR_FRCN_ROTATE_OUTPUT_COMPUTE_IN_OUT_SHAPE         (HCNN_NNIE_ERR_BASE + 396)     // 计算输入输出大小错误
#define HCNN_NNIE_ERR_FRCN_ROTATE_OUTPUT_CREAT_NULL_PTR               (HCNN_NNIE_ERR_BASE + 397)     // 创建层空指针
#define HCNN_NNIE_ERR_FRCN_ROTATE_OUTPUT_FORWARD_NULL_PTR             (HCNN_NNIE_ERR_BASE + 398)     // forward空指针
#define HCNN_NNIE_ERR_FRCN_ROTATE_OUTPUT_FORWARD_PARAM                (HCNN_NNIE_ERR_BASE + 399)     // forward参数错误
#define HCNN_NNIE_ERR_FRCN_ROTATE_OUTPUT_MEM_BUFFER_OUT_RANGE         (HCNN_NNIE_ERR_BASE + 400)     // 辅助内存空间不足

// 私有层 YLOUT_POLY 错误码
#define HCNN_NNIE_ERR_YLOUT_POLY_CREAT_NULL_PTR               (HCNN_NNIE_ERR_BASE + 401)     // 创建层空指针
#define HCNN_NNIE_ERR_YLOUT_POLY_GET_MEM_SIZE_NULL_PTR        (HCNN_NNIE_ERR_BASE + 402)     // 获取层内存空指针
#define HCNN_NNIE_ERR_YLOUT_POLY_CREAT_MODEL_NULL_PTR         (HCNN_NNIE_ERR_BASE + 403)     // 创建模型空指针
#define HCNN_NNIE_ERR_YLOUT_POLY_CREAT_MODEL                  (HCNN_NNIE_ERR_BASE + 404)     // 创建模型错误
#define HCNN_NNIE_ERR_YLOUT_POLY_INIT_MODEL_ERR               (HCNN_NNIE_ERR_BASE + 405)     // 初始化模型错误
#define HCNN_NNIE_ERR_YLOUT_POLY_GET_MODEL_SIZE_NULL_PTR      (HCNN_NNIE_ERR_BASE + 406)     // 获取模型内存空指针
#define HCNN_NNIE_ERR_YLOUT_POLY_FORWARD_NULL_PTR             (HCNN_NNIE_ERR_BASE + 407)     // forward参数空指针
#define HCNN_NNIE_ERR_YLOUT_POLY_FORWARD_INPUT_BLOB_ERR       (HCNN_NNIE_ERR_BASE + 408)     // forward输入数据非法

// zengyu7
// 私有层 GFL_PROPOSAL 错误码
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_GET_MODEL_SIZE_NULL_PTR     (HCNN_NNIE_ERR_BASE + 409)     // 获取模型内存空指针
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_CREAT_MODEL_NULL_PTR        (HCNN_NNIE_ERR_BASE + 410)     // 创建模型空指针
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_CREAT_MODEL                 (HCNN_NNIE_ERR_BASE + 411)     // 创建模型错误
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_INIT_MODEL_ERR              (HCNN_NNIE_ERR_BASE + 412)     // 初始化模型错误
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_GET_MEM_SIZE_NULL_PTR       (HCNN_NNIE_ERR_BASE + 413)     // 获取层内存空指针
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_COMPUTE_IN_OUT_SHAPE        (HCNN_NNIE_ERR_BASE + 414)     // 计算输入输出大小错误
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_CREAT_NULL_PTR              (HCNN_NNIE_ERR_BASE + 415)     // 创建层空指针
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_FORWARD_NULL_PTR            (HCNN_NNIE_ERR_BASE + 416)     // forward空指针
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_FORWARD_PARAM               (HCNN_NNIE_ERR_BASE + 417)     // forward参数错误
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_FORWARD_PROCESS             (HCNN_NNIE_ERR_BASE + 418)     // forward处理错误
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_MEM_BUFFER_OUT_RANGE        (HCNN_NNIE_ERR_BASE + 419)     // 辅助内存空间不足
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_CREAT_VALUE                 (HCNN_NNIE_ERR_BASE + 420)     // create阶段参数值错误
#define HCNN_NNIE_ERR_NANO_DET_PROPOSAL_GENE_BASE_ANCHORS           (HCNN_NNIE_ERR_BASE + 421)     // 生成anchor错误

// 私有层 BATCH_NMS 错误码
#define HCNN_NNIE_ERR_BATCH_NMS_GET_MODEL_NULL_PTR                  (HCNN_NNIE_ERR_BASE + 422)     // 获取模型内存空指针
#define HCNN_NNIE_ERR_BATCH_NMS_CREAT_MODEL_NULL_PTR                (HCNN_NNIE_ERR_BASE + 423)     // 创建模型空指针
#define HCNN_NNIE_ERR_BATCH_NMS_INIT_MODEL_ERR                      (HCNN_NNIE_ERR_BASE + 424)     // 初始化模型错误
#define HCNN_NNIE_ERR_BATCH_NMS_GET_MEM_SIZE_NULL_PTR               (HCNN_NNIE_ERR_BASE + 425)     // 获取层内存空指针
#define HCNN_NNIE_ERR_BATCH_NMS_COMPUTE_IN_OUT_SHAPE                (HCNN_NNIE_ERR_BASE + 426)     // 计算输入输出大小错误
#define HCNN_NNIE_ERR_BATCH_NMS_FORWARD_NULL_PTR                    (HCNN_NNIE_ERR_BASE + 427)     // forward空指针
#define HCNN_NNIE_ERR_BATCH_NMS_FORWARD_PARAM                       (HCNN_NNIE_ERR_BASE + 428)     // forward参数错误
#define HCNN_NNIE_ERR_BATCH_NMS_FORWARD_PROCESS                     (HCNN_NNIE_ERR_BASE + 429)     // forward处理错误
#define HCNN_NNIE_ERR_BATCH_NMS_MEM_BUFFER_OUT_RANGE                (HCNN_NNIE_ERR_BASE + 430)     // 辅助内存空间不足
#define HCNN_NNIE_ERR_BATCH_NMS_CREAT_NULL_PTR                      (HCNN_NNIE_ERR_BASE + 431)     // create阶段空指针
#define HCNN_NNIE_ERR_BATCH_NMS_CREAT_PARAM                         (HCNN_NNIE_ERR_BASE + 432)     // create阶段参数错误

// 私有层 FRCN_PROPOSAL_IBOX 错误码
#define HCNN_NNIE_ERR_FRCN_PROPOSAL_IBOX_GET_MODEL_MEM_NULL_PTR         (HCNN_NNIE_ERR_BASE + 432)     // 得到模型空指针
#define HCNN_NNIE_ERR_FRCN_PROPOSAL_IBOX_CREAT_MODEL_NULL_PTR           (HCNN_NNIE_ERR_BASE + 433)     // 创建模型空指针
#define HCNN_NNIE_ERR_FRCN_PROPOSAL_IBOX_CREAT_MODEL                    (HCNN_NNIE_ERR_BASE + 434)     // 创建模型错误
#define HCNN_NNIE_ERR_FRCN_PROPOSAL_IBOX_INIT_MODEL_ERR                 (HCNN_NNIE_ERR_BASE + 435)     // 初始化模型错误
#define HCNN_NNIE_ERR_FRCN_PROPOSAL_IBOX_GET_MEM_NULL_PTR               (HCNN_NNIE_ERR_BASE + 436)     // 得到模型空指针
#define HCNN_NNIE_ERR_FRCN_PROPOSAL_IBOX_CREAT_NULL_PTR                 (HCNN_NNIE_ERR_BASE + 437)     // 创建层空指针
#define HCNN_NNIE_ERR_FRCN_PROPOSAL_IBOX_CREAT_PARAM                    (HCNN_NNIE_ERR_BASE + 438)     // 创建模型时参数错误
#define HCNN_NNIE_ERR_FRCN_PROPOSAL_IBOX_FORWARD_PARAM                  (HCNN_NNIE_ERR_BASE + 439)     // forward参数错误
#define HCNN_NNIE_ERR_FRCN_PROPOSAL_IBOX_FORWARD_NULL_PTR               (HCNN_NNIE_ERR_BASE + 440)     // forward空指针错误
#define HCNN_NNIE_ERR_FRCN_PROPOSAL_IBOX_FORWARD_PROCESS                (HCNN_NNIE_ERR_BASE + 441)     // forward处理错误

// 私有层 BBOXES_SELECT 错误码
#define HCNN_NNIE_ERR_BBOXES_SELECT_GET_MODEL_MEM_NULL_PTR              (HCNN_NNIE_ERR_BASE + 442)     // 得到模型空指针
#define HCNN_NNIE_ERR_BBOXES_SELECT_CREAT_MODEL_NULL_PTR                (HCNN_NNIE_ERR_BASE + 443)     // 创建模型空指针
#define HCNN_NNIE_ERR_BBOXES_SELECT_INIT_MODEL_NULL_PTR                 (HCNN_NNIE_ERR_BASE + 444)     // 初始化模型过程中空指针
#define HCNN_NNIE_ERR_BBOXES_SELECT_INIT_MODEL_ERR                      (HCNN_NNIE_ERR_BASE + 445)     // 初始化模型过程中错误
#define HCNN_NNIE_ERR_BBOXES_SELECT_GET_MEM_NULL_PTR                    (HCNN_NNIE_ERR_BASE + 446)     // GetMemSize过程中空指针
#define HCNN_NNIE_ERR_BBOXES_SELECT_CREAT_NULL_PTR                      (HCNN_NNIE_ERR_BASE + 447)     // Create过程中空指针
#define HCNN_NNIE_ERR_BBOXES_SELECT_CREAT_PARAM                         (HCNN_NNIE_ERR_BASE + 448)     // Create过程中检查参数时错误
#define HCNN_NNIE_ERR_BBOXES_SELECT_FORWARD_PROCESS                     (HCNN_NNIE_ERR_BASE + 449)     // 前向过程中出现错误
#define HCNN_NNIE_ERR_BBOXES_SELECT_FORWARD_PARAM                       (HCNN_NNIE_ERR_BASE + 450)     // 前向过程中参数出现错误
#define HCNN_NNIE_ERR_BBOXES_SELECT_FORWARD_NULL_PTR                    (HCNN_NNIE_ERR_BASE + 451)     // 前向过程中参数出现空指针

// 私有层 AUTOASSIGN_POSTPROC 错误码
#define HCNN_NNIE_ERR_AUTOASSIGN_POSTPROC_CREAT_NULL_PTR               (HCNN_NNIE_ERR_BASE + 351)     // 创建层空指针
#define HCNN_NNIE_ERR_AUTOASSIGN_POSTPROC_GET_MEM_SIZE_NULL_PTR        (HCNN_NNIE_ERR_BASE + 352)     // 获取层内存空指针
#define HCNN_NNIE_ERR_AUTOASSIGN_POSTPROC_CREAT_MODEL_NULL_PTR         (HCNN_NNIE_ERR_BASE + 353)     // 创建模型空指针
#define HCNN_NNIE_ERR_AUTOASSIGN_POSTPROC_CREAT_MODEL                  (HCNN_NNIE_ERR_BASE + 354)     // 创建模型错误
#define HCNN_NNIE_ERR_AUTOASSIGN_POSTPROC_INIT_MODEL_ERR               (HCNN_NNIE_ERR_BASE + 355)     // 初始化模型错误
#define HCNN_NNIE_ERR_AUTOASSIGN_POSTPROC_GET_MODEL_SIZE_NULL_PTR      (HCNN_NNIE_ERR_BASE + 356)     // 获取模型内存空指针
#define HCNN_NNIE_ERR_AUTOASSIGN_POSTPROC_FORWARD_NULL_PTR             (HCNN_NNIE_ERR_BASE + 357)     // forward参数空指针
#define HCNN_NNIE_ERR_AUTOASSIGN_POSTPROC_FORWARD_INPUT_BLOB_ERR       (HCNN_NNIE_ERR_BASE + 358)     // forward输入数据非法

//调度错误码段0X86100400～0X861005FF
#define HCNN_SCHEDULER_ERR_BASE            (0X86100400)


//DSP错误码段 0X86100600～0X861007FF
#define HCNN_DSP_ERR_BASE                  (0X86100600)
#define HCNN_VP6_EER_NULL_PTR              (HCNN_DSP_ERR_BASE + 1)        // 空指针
#define HCNN_VP6_EER_PARAM_VALUE           (HCNN_DSP_ERR_BASE + 2)        // 明显参数错误
#define HCNN_VP6_EER_PRC_SIZE              (HCNN_DSP_ERR_BASE + 3)        // 输入大小错误
#define HCNN_VP6_EER_NO_ENOUGH_MEM         (HCNN_DSP_ERR_BASE + 4)        // 没有足够的内存用于分配
#define HCNN_VP6_EER_FLUSH_CACHE           (HCNN_DSP_ERR_BASE + 5)        // mmz刷cache失败
#define HCNN_VP6_ERR_IN_PRIORITY           (HCNN_DSP_ERR_BASE + 6)        // 输入优先级错误
#define HCNN_VP6_ERR_PIC_FORMAT            (HCNN_DSP_ERR_BASE + 7)        // 输入图片的格式错误
#define HCNN_VP6_EER_MEM_TYPE              (HCNN_DSP_ERR_BASE + 8)        // 输入图片内存类型错误
#define HCNN_VP6_EER_GETVIRMEMINFO         (HCNN_DSP_ERR_BASE + 9)        // 获取物理地址失败
#define HCNN_VP6_EER_DSP_RPC               (HCNN_DSP_ERR_BASE + 10)       // HI_MPI_SVP_DSP_RPC接口调用失败
#define HCNN_VP6_EER_DSP_QUERY_TO          (HCNN_DSP_ERR_BASE + 11)       // HI_MPI_SVP_DSP_Query接口调用失败（超时）
#define HCNN_VP6_EER_HWQM_SD               (HCNN_DSP_ERR_BASE + 12)       // VP6调用hwqm_send_data失败
#define HCNN_VP6_EER_DSP_FORWARD           (HCNN_DSP_ERR_BASE + 13)       // VP6层实现中返回错误（具体DSP返回值看打印）
#define HCNN_VP6_EER_NOT_SUPPORT           (HCNN_DSP_ERR_BASE + 14)       // 未支持
#define HCNN_VP6_EER_PLATFORM              (HCNN_DSP_ERR_BASE + 15)       // 非VP6或ARM HCNN_VP6无法支持
#define HCNN_VP6_EER_RELEASE_SEM           (HCNN_DSP_ERR_BASE + 16)       // 释放信号量错误
#define HCNN_VP6_EER_MODEL_FL              (HCNN_DSP_ERR_BASE + 17)       // 模型中的量化信息错误
#define HCNN_VP6_EER_OUT_BLOB_FORMAT       (HCNN_DSP_ERR_BASE + 18)       // 输出前，blob排布方式错误
#define HCNN_VP6_MODEL_ERROR               (HCNN_DSP_ERR_BASE + 19)       // 模型错误，后续要细分

// DSP_YLOUT
#define HCNN_VP6_EER_YLOUT_FORWARD_PARA    (HCNN_DSP_ERR_BASE + 20)       // YLOUT_Forward参数错误 
#define HCNN_VP6_EER_YLOUT_FORWARD_PROCESS (HCNN_DSP_ERR_BASE + 21)       // YLOUT_Forward处理错误
#define HCNN_VP6_YLOUT_MODEL_ERROR         (HCNN_DSP_ERR_BASE + 22)       // YLOUT普通参数校验失败
#define HCNN_VP6_YLOUT_HYPER_ERROR         (HCNN_DSP_ERR_BASE + 23)       // YLOUT超参解析出错
#define HCNN_VP6_YLOUT_NULL_ERROR          (HCNN_DSP_ERR_BASE + 24)       // YLOUT输入参数为空
#define HCNN_VP6_YLOUT_MEM_ERROR           (HCNN_DSP_ERR_BASE + 25)       // YLOUT内存分配出错

// DSP_BN
#define HCNN_VP6_BN_MODEL_ERROR            (HCNN_DSP_ERR_BASE + 26)       // BN普通参数校验失败
#define HCNN_VP6_BN_HYPER_ERROR            (HCNN_DSP_ERR_BASE + 27)       // BN超参解析出错
#define HCNN_VP6_BN_NULL_ERROR             (HCNN_DSP_ERR_BASE + 28)       // BN输入参数为空
#define HCNN_VP6_BN_MEM_ERROR              (HCNN_DSP_ERR_BASE + 29)       // BN内存分配出错

// DSP_ELTWISE
#define HCNN_VP6_ETW_MODEL_ERROR           (HCNN_DSP_ERR_BASE + 30)       // eltwise普通参数校验失败
#define HCNN_VP6_ETW_HYPER_ERROR           (HCNN_DSP_ERR_BASE + 31)       // eltwise超参解析出错
#define HCNN_VP6_ETW_NULL_ERROR            (HCNN_DSP_ERR_BASE + 32)       // eltwise输入参数为空
#define HCNN_VP6_ETW_MEM_ERROR             (HCNN_DSP_ERR_BASE + 33)       // eltwise内存分配出错
#define HCNN_VP6_ETW_NSP_ERROR             (HCNN_DSP_ERR_BASE + 34)       // eltwise不支持

// DSP_CONCAT
#define HCNN_VP6_CAT_MODEL_ERROR           (HCNN_DSP_ERR_BASE + 35)       // concat普通参数校验失败
#define HCNN_VP6_CAT_HYPER_ERROR           (HCNN_DSP_ERR_BASE + 36)       // concat超参解析出错
#define HCNN_VP6_CAT_NULL_ERROR            (HCNN_DSP_ERR_BASE + 37)       // concat输入参数为空
#define HCNN_VP6_CAT_MEM_ERROR             (HCNN_DSP_ERR_BASE + 38)       // concat内存分配出错

// DSP_CONV
#define HCNN_VP6_CONV_MODEL_ERROR          (HCNN_DSP_ERR_BASE + 36)       // conv普通参数校验失败
#define HCNN_VP6_CONV_HYPER_ERROR          (HCNN_DSP_ERR_BASE + 37)       // conv超参解析出错
#define HCNN_VP6_CONV_NULL_ERROR           (HCNN_DSP_ERR_BASE + 38)       // conv输入参数为空
#define HCNN_VP6_CONV_MEM_ERROR            (HCNN_DSP_ERR_BASE + 39)       // conv内存分配出错

// DSP_FC
#define HCNN_VP6_FC_MODEL_ERROR            (HCNN_DSP_ERR_BASE + 40)       // fc普通参数校验失败
#define HCNN_VP6_FC_HYPER_ERROR            (HCNN_DSP_ERR_BASE + 41)       // fc超参解析出错
#define HCNN_VP6_FC_NULL_ERROR             (HCNN_DSP_ERR_BASE + 42)       // fc输入参数为空
#define HCNN_VP6_FC_MEM_ERROR              (HCNN_DSP_ERR_BASE + 43)       // fc内存分配出错

// DSP_OUT
#define HCNN_VP6_OUT_PARAM_ERROR           (HCNN_DSP_ERR_BASE + 44)       // out普通参数校验失败
#define HCNN_VP6_OUT_NULL_ERROR            (HCNN_DSP_ERR_BASE + 46)       // out输入参数为空
#define HCNN_VP6_OUT_NTS_LAYER             (HCNN_DSP_ERR_BASE + 47)       // out不支持该层输出

// DSP_IPCM_TASK
#define HCNN_VP6_IPCM_MODEL_ERROR          (HCNN_DSP_ERR_BASE + 48)       // ipcm模型参数错误
#define HCNN_VP6_IPCM_NULL_ERROR           (HCNN_DSP_ERR_BASE + 49)       // ipcm输入参数为空
#define HCNN_VP6_IPCM_NTS_LAYER            (HCNN_DSP_ERR_BASE + 50)       // ipcm不支持的层
#define HCNN_VP6_IPCM_NTS_OPT              (HCNN_DSP_ERR_BASE + 51)       // ipcm不支持的操作
#define HCNN_VP6_IPCM_MEM_ERROR            (HCNN_DSP_ERR_BASE + 52)       // ipcm内存分配不足

// DSP_POOL
#define HCNN_VP6_POOL_MODEL_ERROR          (HCNN_DSP_ERR_BASE + 53)       // pool普通参数校验失败
#define HCNN_VP6_POOL_HYPER_ERROR          (HCNN_DSP_ERR_BASE + 54)       // pool超参解析出错
#define HCNN_VP6_POOL_NULL_ERROR           (HCNN_DSP_ERR_BASE + 55)       // pool输入参数为空
#define HCNN_VP6_POOL_MEM_ERROR            (HCNN_DSP_ERR_BASE + 56)       // pool内存分配出错

// DSP_PRELU
#define HCNN_VP6_PRELU_MODEL_ERROR         (HCNN_DSP_ERR_BASE + 57)       // prelu普通参数校验失败
#define HCNN_VP6_PRELU_HYPER_ERROR         (HCNN_DSP_ERR_BASE + 58)       // prelu超参解析出错
#define HCNN_VP6_PRELU_NULL_ERROR          (HCNN_DSP_ERR_BASE + 59)       // prelu输入参数为空
#define HCNN_VP6_PRELU_MEM_ERROR           (HCNN_DSP_ERR_BASE + 60)       // prelu内存分配出错

// MOW 调度
#define HCNN_VP6_MOW_OUT_NOT_DWH           (HCNN_DSP_ERR_BASE + 61)       // mow调度的输出默认是DWH
#define HCNN_VP6_MOW_BLOB_NUM_ERR          (HCNN_DSP_ERR_BASE + 62)       // mow处理的层输出blob大于1

#define HCNN_VP6_OVERFLOW                  (HCNN_DSP_ERR_BASE + 63)       // 溢出风险

// DSP_MEANOFFSET
#define HCNN_VP6_MEANOFFSET_MODEL_ERROR    (HCNN_DSP_ERR_BASE + 64)       // MEANOFFSET普通参数校验失败
#define HCNN_VP6_MEANOFFSET_HYPER_ERROR    (HCNN_DSP_ERR_BASE + 65)       // MEANOFFSET超参解析出错
#define HCNN_VP6_MEANOFFSET_NULL_ERROR     (HCNN_DSP_ERR_BASE + 66)       // MEANOFFSET输入参数为空
#define HCNN_VP6_MEANOFFSET_MEM_ERROR      (HCNN_DSP_ERR_BASE + 67)       // MEANOFFSET内存分配出错

// DSP_ELTBN
#define HCNN_VP6_ETWBN_MODEL_ERROR         (HCNN_DSP_ERR_BASE + 68)       // eltbn普通参数校验失败
#define HCNN_VP6_ETWBN_HYPER_ERROR         (HCNN_DSP_ERR_BASE + 69)       // eltbn超参解析出错
#define HCNN_VP6_ETWBN_NULL_ERROR          (HCNN_DSP_ERR_BASE + 70)       // eltbn输入参数为空
#define HCNN_VP6_ETWBN_MEM_ERROR           (HCNN_DSP_ERR_BASE + 71)       // eltbn内存分配出错
#define HCNN_VP6_ETWBN_NSP_ERROR           (HCNN_DSP_ERR_BASE + 72)       // eltbn不支持

// DSP_UNPOOL
#define HCNN_VP6_UNPOOL_MODEL_ERROR        (HCNN_DSP_ERR_BASE + 73)       // unpool普通参数校验失败
#define HCNN_VP6_UNPOOL_HYPER_ERROR        (HCNN_DSP_ERR_BASE + 74)       // unpool超参解析出错
#define HCNN_VP6_UNPOOL_NULL_ERROR         (HCNN_DSP_ERR_BASE + 75)       // unpool输入参数为空
#define HCNN_VP6_UNPOOL_MEM_ERROR          (HCNN_DSP_ERR_BASE + 76)       // unpool内存分配出错

// DSP_SCALE
#define HCNN_VP6_SCALE_MODEL_ERROR         (HCNN_DSP_ERR_BASE + 77)       // scale普通参数校验失败
#define HCNN_VP6_SCALE_HYPER_ERROR         (HCNN_DSP_ERR_BASE + 78)       // scale超参解析出错
#define HCNN_VP6_SCALE_NULL_ERROR          (HCNN_DSP_ERR_BASE + 79)       // scale输入参数为空
#define HCNN_VP6_SCALE_MEM_ERROR           (HCNN_DSP_ERR_BASE + 80)       // scale内存分配出错

// DSP_SIGMOID
#define HCNN_VP6_SIGMOID_MODEL_ERROR       (HCNN_DSP_ERR_BASE + 81)       // sigmoid普通参数校验失败
#define HCNN_VP6_SIGMOID_HYPER_ERROR       (HCNN_DSP_ERR_BASE + 82)       // sigmoid超参解析出错
#define HCNN_VP6_SIGMOID_NULL_ERROR        (HCNN_DSP_ERR_BASE + 83)       // sigmoid输入参数为空
#define HCNN_VP6_SIGMOID_MEM_ERROR         (HCNN_DSP_ERR_BASE + 84)       // sigmoid内存分配出错

// DSP_UPSAMPLE
#define HCNN_VP6_UPSAMPLE_MODEL_ERROR          (HCNN_DSP_ERR_BASE + 85)       // UPSAMPLE普通参数校验失败
#define HCNN_VP6_UPSAMPLE_HYPER_ERROR          (HCNN_DSP_ERR_BASE + 86)       // UPSAMPLE超参解析出错
#define HCNN_VP6_UPSAMPLE_NULL_ERROR           (HCNN_DSP_ERR_BASE + 87)       // UPSAMPLE输入参数为空
#define HCNN_VP6_UPSAMPLE_MEM_ERROR            (HCNN_DSP_ERR_BASE + 88)       // UPSAMPLE内存分配出错

// DSP_Y3COMBINE    Y3COMBINE_Forward错误待定
#define HCNN_VP6_EER_Y3COMBINE_FORWARD_PARA    (HCNN_DSP_ERR_BASE + 89)       // Y3COMBINE_Forward参数错误 
#define HCNN_VP6_EER_Y3COMBINE_FORWARD_PROCESS (HCNN_DSP_ERR_BASE + 90)       // Y3COMBINE_Forward处理错误
#define HCNN_VP6_Y3COMBINE_MODEL_ERROR         (HCNN_DSP_ERR_BASE + 91)       // Y3COMBINE普通参数校验失败
#define HCNN_VP6_Y3COMBINE_HYPER_ERROR         (HCNN_DSP_ERR_BASE + 92)       // Y3COMBINE超参解析出错
#define HCNN_VP6_Y3COMBINE_NULL_ERROR          (HCNN_DSP_ERR_BASE + 93)       // Y3COMBINE输入参数为空
#define HCNN_VP6_Y3COMBINE_MEM_ERROR           (HCNN_DSP_ERR_BASE + 94)       // Y3COMBINET内存分配出错

// DSP_SOFTMAX
#define HCNN_VP6_SOFTMAX_MODEL_ERROR         (HCNN_DSP_ERR_BASE + 95)      // SOFTMAX普通参数校验失败
#define HCNN_VP6_SOFTMAX_HYPER_ERROR         (HCNN_DSP_ERR_BASE + 96)      // SOFTMAX超参解析出错
#define HCNN_VP6_SOFTMAX_NULL_ERROR          (HCNN_DSP_ERR_BASE + 97)      // SOFTMAX输入参数为空
#define HCNN_VP6_SOFTMAX_MEM_ERROR           (HCNN_DSP_ERR_BASE + 98)      // SOFTMAX内存分配出错

// DSP_ELTBIAS
#define HCNN_VP6_ELTBIAS_MODEL_ERROR       (HCNN_DSP_ERR_BASE + 99)        // eltbias普通参数校验失败
#define HCNN_VP6_ELTBIAS_HYPER_ERROR       (HCNN_DSP_ERR_BASE + 100)       // eltbias超参解析出错
#define HCNN_VP6_ELTBIAS_NULL_ERROR        (HCNN_DSP_ERR_BASE + 101)       // eltbias输入参数为空
#define HCNN_VP6_ELTBIAS_MEM_ERROR         (HCNN_DSP_ERR_BASE + 102)       // eltbias内存分配出错
#define HCNN_VP6_ELTBIAS_NSP_ERROR         (HCNN_DSP_ERR_BASE + 103)       // eltbias不支持

// DSP_FILSAMPLE
#define HCNN_VP6_FILTER_SAMPLE_MODEL_ERROR (HCNN_DSP_ERR_BASE + 104)       // filsample普通参数校验失败
#define HCNN_VP6_FILTER_SAMPLE_HYPER_ERROR (HCNN_DSP_ERR_BASE + 105)       // filsample超参解析出错
#define HCNN_VP6_FILTER_SAMPLE_NULL_ERROR  (HCNN_DSP_ERR_BASE + 106)       // filsample输入参数为空
#define HCNN_VP6_FILTER_SAMPLE_MEM_ERROR   (HCNN_DSP_ERR_BASE + 107)       // filsample内存分配出错

// DSP_DECONV
#define HCNN_VP6_DECONV_MODEL_ERROR        (HCNN_DSP_ERR_BASE + 108)       // deconv普通参数校验失败
#define HCNN_VP6_DECONV_HYPER_ERROR        (HCNN_DSP_ERR_BASE + 109)       // deconv超参解析出错
#define HCNN_VP6_DECONV_NULL_ERROR         (HCNN_DSP_ERR_BASE + 110)       // deconv输入参数为空
#define HCNN_VP6_DECONV_MEM_ERROR          (HCNN_DSP_ERR_BASE + 111)       // deconv内存分配出错

// DSP_YLPROPOSAL
#define HCNN_VP6_YLPROPOSAL_FORWARD_PARA   (HCNN_DSP_ERR_BASE + 112)       // YLPROPOSAL_Forward参数错误 
#define HCNN_VP6_YLPROPOSAL_FORWARD_PRO    (HCNN_DSP_ERR_BASE + 113)       // YLPROPOSAL_Forward处理错误
#define HCNN_VP6_YLPROPOSAL_MODEL_ERROR    (HCNN_DSP_ERR_BASE + 114)       // YLPROPOSAL普通参数校验失败
#define HCNN_VP6_YLPROPOSAL_HYPER_ERROR    (HCNN_DSP_ERR_BASE + 115)       // YLPROPOSAL超参解析出错
#define HCNN_VP6_YLPROPOSAL_NULL_ERROR     (HCNN_DSP_ERR_BASE + 116)       // YLPROPOSAL输入参数为空
#define HCNN_VP6_YLPROPOSAL_MEM_ERROR      (HCNN_DSP_ERR_BASE + 117)       // YLPROPOSAL内存分配出错

// DSP_NMS
#define HCNN_VP6_NMS_FORWARD_PARA          (HCNN_DSP_ERR_BASE + 118)       // NMS_Forward参数错误 
#define HCNN_VP6_NMS_FORWARD_PRO           (HCNN_DSP_ERR_BASE + 119)       // NMS_Forward处理错误
#define HCNN_VP6_NMS_MODEL_ERROR           (HCNN_DSP_ERR_BASE + 120)       // NMS普通参数校验失败
#define HCNN_VP6_NMS_HYPER_ERROR           (HCNN_DSP_ERR_BASE + 121)       // NMS超参解析出错
#define HCNN_VP6_NMS_NULL_ERROR            (HCNN_DSP_ERR_BASE + 122)       // NMS输入参数为空
#define HCNN_VP6_NMS_MEM_ERROR             (HCNN_DSP_ERR_BASE + 123)       // NMS内存分配出错

// DSP_NORMALIZE
#define HCNN_VP6_NORMALIZE_MODEL_ERROR     (HCNN_DSP_ERR_BASE + 124)       // normalize普通参数校验失败
#define HCNN_VP6_NORMALIZE_HYPER_ERROR     (HCNN_DSP_ERR_BASE + 125)       // normalize超参解析出错
#define HCNN_VP6_NORMALIZE_NULL_ERROR      (HCNN_DSP_ERR_BASE + 126)       // normalize输入参数为空
#define HCNN_VP6_NORMALIZE_MEM_ERROR       (HCNN_DSP_ERR_BASE + 127)       // normalize内存分配出错


// DSP_YPARK
#define HCNN_VP6_EER_YLPARK_FORWARD_PARA    (HCNN_DSP_ERR_BASE + 128)       // YPARK_Forward参数错误
#define HCNN_VP6_EER_YLPARK_FORWARD_PROCESS (HCNN_DSP_ERR_BASE + 129)       // YPARK_Forward处理错误
#define HCNN_VP6_YLPARK_MODEL_ERROR         (HCNN_DSP_ERR_BASE + 130)       // YPARK普通参数校验失败
#define HCNN_VP6_YLPARK_HYPER_ERROR         (HCNN_DSP_ERR_BASE + 131)       // YPARK超参解析出错
#define HCNN_VP6_YLPARK_NULL_ERROR          (HCNN_DSP_ERR_BASE + 132)       // YPARK输入参数为空
#define HCNN_VP6_YLPARK_MEM_ERROR           (HCNN_DSP_ERR_BASE + 133)       // YPARk内存分配出错

// DSP roi_align
#define HCNN_VP6_ROI_ALIGN_GET_MODEL_SIZE_NULL_PTR     (HCNN_DSP_ERR_BASE + 134)     // roi_align层获取模型空指针
#define HCNN_VP6_ROI_ALIGN_CREAT_MODEL_NULL_PTR        (HCNN_DSP_ERR_BASE + 135)     // roi_align层创建模型空指针
#define HCNN_VP6_ROI_ALIGN_MEM_BUFFER_OUT_RANGE        (HCNN_DSP_ERR_BASE + 136)     // roi_align层分配内存超标
#define HCNN_VP6_ROI_ALIGN_CREAT_INIT_MODEL_ERR        (HCNN_DSP_ERR_BASE + 137)     // roi_align层初始化模型错误
#define HCNN_VP6_ROI_ALIGN_GET_MEM_SIZE_NULL_PTR       (HCNN_DSP_ERR_BASE + 138)     // roi_align层获取层内存空指针
#define HCNN_VP6_ROI_ALIGN_CREAT_NULL_PTR              (HCNN_DSP_ERR_BASE + 139)     // roi_align层创建层空指针
#define HCNN_VP6_ROI_ALIGN_FORWARD_PARA_ERR            (HCNN_DSP_ERR_BASE + 130)     // roi_align层forward参数错误
#define HCNN_VP6_ROI_ALIGN_FORWARD_PROCESS_ERR         (HCNN_DSP_ERR_BASE + 131)     // roi_align层forward失败

// DSP yoloquadra
#define HCNN_VP6_YOLO_QUADRA_MODEL_ERROR               (HCNN_DSP_ERR_BASE + 132)     // yolo_quadra层模型参数错误
#define HCNN_VP6_YOLO_QUADRA_NULL_ERROR                (HCNN_DSP_ERR_BASE + 133)     // yolo_quadra层空指针

// DSP permute
#define HCNN_VP6_PERMUTE_MODEL_PARAM_ERROR             (HCNN_DSP_ERR_BASE + 150)     // permute层模型参数错误
#define HCNN_VP6_PERMUTE_MEM_LACK_ERROR                (HCNN_DSP_ERR_BASE + 151)     // permute层内存不足
#define HCNN_VP6_PERMUTE_NULL_ERROR                    (HCNN_DSP_ERR_BASE + 152)     // permute层空指针

#define HCNN_VP6_ERR_ARGMAX_NULL_PTR        (HCNN_DSP_ERR_BASE + 153)    // 空指针错误
#define HCNN_VP6_ERR_ARGMAX_PARAM           (HCNN_DSP_ERR_BASE + 154)    // 输入参数错误
#define HCNN_VP6_ERR_ARGMAX_VIR_MEM_INFO    (HCNN_DSP_ERR_BASE + 155)    // 获得虚拟地址信息错误
#define HCNN_VP6_ERR_ARGMAX_ARGMAX          (HCNN_DSP_ERR_BASE + 156)    // argmax 函数错误

// dsp rpn proposal
#define HCNN_VP6_ERR_PROPOSAL_NULL_PTR                  (HCNN_DSP_ERR_BASE + 157)  // 空指针
#define HCNN_VP6_ERR_PROPOSAL_INVALID_PARAM             (HCNN_DSP_ERR_BASE + 158)  // 参数错误
#define HCNN_VP6_ERR_PROPOSAL_GET_MODEL_SIZE_NULL_PTR   (HCNN_DSP_ERR_BASE + 159)  // getmodelmemsize接口空指针
#define HCNN_VP6_ERR_PROPOSAL_CREAT_INIT_MODEL_ERR      (HCNN_DSP_ERR_BASE + 160)  // 模型初始化失败
#define HCNN_VP6_ERR_PROPOSAL_CREAT_RESHAPE_PARM_ERR    (HCNN_DSP_ERR_BASE + 161)  // reshape参数错误
#define HCNN_VP6_ERR_PROPOSAL_MEN_BUFFER_OUT_RANGE      (HCNN_DSP_ERR_BASE + 162)  // 内存不足
#define HCNN_VP6_ERR_PROPOSAL_GET_MEN_SIZE_NULL_PTR     (HCNN_DSP_ERR_BASE + 163)  // getmemsize接口空指针
#define HCNN_VP6_ERR_PROPOSAL_GET_MEN_SIZE_RESHAPE_ERR  (HCNN_DSP_ERR_BASE + 164)  // 参数错误
#define HCNN_VP6_ERR_PROPOSAL_GET_MEN_SIZE_PARM_ERR     (HCNN_DSP_ERR_BASE + 165)  // 参数错误
#define HCNN_VP6_ERR_PROPOSAL_CREAT_PARM_ERR            (HCNN_DSP_ERR_BASE + 166)  // 参数错误
#define HCNN_VP6_ERR_PROPOSAL_CREAT_NULL_PTR            (HCNN_DSP_ERR_BASE + 167)  // 空指针
#define HCNN_VP6_ERR_PROPOSAL_GENE_BASE_ANCHORS_ERR     (HCNN_DSP_ERR_BASE + 168)  // 基准狂生成失败
#define HCNN_VP6_ERR_PROPOSAL_INPUT_NUM_MISSMATCH       (HCNN_DSP_ERR_BASE + 169)  // 输入blob个数不正确
#define HCNN_VP6_ERR_PROPOSAL_CREAT_MODEL_NULL_PTR      (HCNN_DSP_ERR_BASE + 170)  // 空指针

// DSP yolooutland
#define HCNN_VP6_EER_YLOUT_LAND_FORWARD_PARA                       (HCNN_DSP_ERR_BASE + 171)       // YLOUT_Forward参数错误 
#define HCNN_VP6_EER_YLOUT_LAND_FORWARD_PROCESS                    (HCNN_DSP_ERR_BASE + 172)       // YLOUT_Forward处理错误
#define HCNN_VP6_YLOUT_LAND_MODEL_ERROR                            (HCNN_DSP_ERR_BASE + 173)       // YLOUT普通参数校验失败
#define HCNN_VP6_YLOUT_LAND_HYPER_ERROR                            (HCNN_DSP_ERR_BASE + 174)       // YLOUT超参解析出错
#define HCNN_VP6_YLOUT_LAND_NULL_ERROR                             (HCNN_DSP_ERR_BASE + 175)       // YLOUT输入参数为空
#define HCNN_VP6_YLOUT_LAND_MEM_ERROR                              (HCNN_DSP_ERR_BASE + 176)       // YLOUT内存分配出错


// 私有层 RPN_AP 错误码
#define HCNN_VP6_ERR_RPN_AP_GET_MODEL_SIZE_NULL_PTR    (HCNN_DSP_ERR_BASE + 172)     // 获取模型内存空指针
#define HCNN_VP6_ERR_RPN_AP_CREAT_MODEL_NULL_PTR       (HCNN_DSP_ERR_BASE + 173)     // 创建模型空指针
#define HCNN_VP6_ERR_RPN_AP_CREAT_MODEL                (HCNN_DSP_ERR_BASE + 174)     // 创建模型错误
#define HCNN_VP6_ERR_RPN_AP_INIT_MODEL_ERR             (HCNN_DSP_ERR_BASE + 175)     // 初始化模型错误
#define HCNN_VP6_ERR_RPN_AP_INIT_MODEL_PARAM_ERR       (HCNN_DSP_ERR_BASE + 176)     // 初始化模型错误
#define HCNN_VP6_ERR_RPN_AP_GET_MEM_SIZE_NULL_PTR      (HCNN_DSP_ERR_BASE + 177)     // 获取层内存空指针
#define HCNN_VP6_ERR_RPN_AP_COMPUTE_IN_OUT_SHAPE       (HCNN_DSP_ERR_BASE + 178)     // 计算输入输出大小错误
#define HCNN_VP6_ERR_RPN_AP_CREAT_NULL_PTR             (HCNN_DSP_ERR_BASE + 179)     // 创建层空指针
#define HCNN_VP6_ERR_RPN_AP_CREAT_GENERATE_PTR         (HCNN_DSP_ERR_BASE + 180)     // 创建anchor错误
#define HCNN_VP6_ERR_RPN_AP_FORWARD_NULL_PTR           (HCNN_DSP_ERR_BASE + 181)     // forward空指针
#define HCNN_VP6_ERR_RPN_AP_FORWARD                    (HCNN_DSP_ERR_BASE + 182)     // forward参数错误

// 私有层 NMS_ATTR 错误码
#define HCNN_VP6_ERR_NMS_ATTR_GET_MODEL_SIZE_NULL_PTR  (HCNN_DSP_ERR_BASE + 183)     // 获取模型内存空指针
#define HCNN_VP6_ERR_NMS_ATTR_CREAT_MODEL_NULL_PTR     (HCNN_DSP_ERR_BASE + 184)     // 创建模型空指针
#define HCNN_VP6_ERR_NMS_ATTR_CREAT_MODEL              (HCNN_DSP_ERR_BASE + 185)     // 创建模型错误
#define HCNN_VP6_ERR_NMS_ATTR_INIT_MODEL_ERR           (HCNN_DSP_ERR_BASE + 186)     // 初始化模型错误
#define HCNN_VP6_ERR_NMS_ATTR_GET_MEM_SIZE_NULL_PTR    (HCNN_DSP_ERR_BASE + 187)     // 获取层内存空指针
#define HCNN_VP6_ERR_NMS_ATTR_CHECK_INPUT_NULL_PTR     (HCNN_DSP_ERR_BASE + 188)     // 获取层内存空指针
#define HCNN_VP6_ERR_NMS_ATTR_PARAM_VALUE              (HCNN_DSP_ERR_BASE + 189)     // 获取层内存空指针
#define HCNN_VP6_ERR_NMS_ATTR_CREAT_NULL_PTR           (HCNN_DSP_ERR_BASE + 190)     // 创建层空指针
#define HCNN_VP6_ERR_NMS_ATTR_CREAT_GENERATE_PTR       (HCNN_DSP_ERR_BASE + 191)     // 创建anchor错误
#define HCNN_VP6_ERR_NMS_ATTR_FORWARD_PARA_PTR         (HCNN_DSP_ERR_BASE + 192)     // forward空指针
#define HCNN_VP6_ERR_NMS_ATTR_FORWARD_PROCESS_PTR      (HCNN_DSP_ERR_BASE + 193)     // forward空指针
#define HCNN_VP6_ERR_NMS_ATTR_NULL_PTR                 (HCNN_DSP_ERR_BASE + 194)     // forward空指针

// DSP_ROI_POOLING
#define HCNN_VP6_ROI_POOLING_MODEL_ERROR                            (HCNN_DSP_ERR_BASE + 195)       // ROI_POOLING模型错误 
#define HCNN_VP6_ROI_POOLING_FORWARD_PARA_ERROR                     (HCNN_DSP_ERR_BASE + 196)       // ROI_POOLING参数错误 
#define HCNN_VP6_ROI_POOLING_SHAPE_ERROR                            (HCNN_DSP_ERR_BASE + 197)       // ROI_POOLING shape大小错误 
#define HCNN_VP6_ROI_POOLING_NULL_ERROR                             (HCNN_DSP_ERR_BASE + 198)       // ROI_POOLING输入地址为空
#define HCNN_VP6_ROI_POOLING_HYPER_ERROR                            (HCNN_DSP_ERR_BASE + 199)      // ROI_POOLING超参错误


// 比对算子错误码 0X86100E00～0X86100FFF  (新比对库使用，HCNN中不使用)
#define HCNN_CMP_ERR_BASE                  (0X86100E00)

#endif 