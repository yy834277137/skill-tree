/** @file      ICF_error_code.h
 *  @note      HangZhou Hikvision Digital Technology Co., Ltd.
               All right reserved
 *  @brief     错误码
 *
 *  @author    tianmin
 *  @date      2019/7/12
 */


#ifndef _ICF_ERROR_CODE_H_
#define _ICF_ERROR_CODE_H_


#define ICF_SUCCESS                               (0)

// 内存池模块错误码
#define ICF_MEMPOOL_BASE                          (0x88001000) // 内存池错误码基准
#define ICF_MEMPOOL_NULLPOINTER                   (0x88001000) // 空指针
#define ICF_MEMPOOL_ERR_MALLOC                    (0x88001001) // 分配MALLOC类型内存失败
#define ICF_MEMPOOL_ERR_HISI_SYS_MMZ_ALLOC        (0x88001002) // 海思接口分配MMZ_NO CACHE类型内存失败
#define ICF_MEMPOOL_ERR_HISI_SYS_MMZ_CACHE_ALLOC  (0x88001003) // 海思接口分配MMZ_CACHE类型内存失败
#define ICF_MEMPOOL_ERR_SPACE_TYPE                (0x88001004) // 错误的SPACE类型
#define ICF_MEMPOOL_ERR_TOO_SMALL                 (0x88001005) // 申请的系统内存少于内存池管理所需大小
#define ICF_MEMPOOL_ERR_LACK_MEMORY               (0x88001006) // 内存池中可供使用的内存不足
#define ICF_MEMPOOL_ERR_FREE_OUT_RANGE            (0x88001007) // 传入内存池待释放的地址错误，超出内存池范围
#define ICF_MEMPOOL_ERR_ALIGNMENT_NOT_SURPPORT    (0x88001008) // 待申请的内存对齐方式不支持
#define ICF_MEMPOOL_ERR_SERARCH_MODE_NOT_SURPPORT (0x88001009) // 不支持的内存池检索方式
#define ICF_MEMPOOL_ERR_FREE_MEM_ERROR            (0x8800100A) // 待释放内存地址有误
#define ICF_MEMPOOL_ERR_ALLOC_SIZE                (0x8800100B) // 申请内存大小有误
#define ICF_MEMPOOL_ERR_NOINIT                    (0x8800100C) // 未初始化调用反初始化
#define ICF_MEMPOOL_ERR_REINIT                    (0x8800100D) // 重复初始化
#define ICF_MEMPOOL_ERR_NT_FEATURE                (0x8800100E) // 分配NT_FEATURE类型内存失败
#define ICF_MEMPOOL_ERR_NOTREGESTER               (0x8800100F) // 外部未注册系统底层内存申请和释放接口接口
#define ICF_MEMPOOL_ERR_MEMOVERSTEP               (0x88001010) // 内存越界错误
#define ICF_MEMPOOL_ERR_PARAM                     (0x88001011) // 内存池输入参数错误
#define ICF_MEMPOOL_ERR_TYPEREPETID               (0x88001012) // 输入的内存池配置参数中内存类型重复


// 分级日志模块错误码
#define ICF_LOG_ERR_BASE                          (0x88001500) // 分级日志模块错误码基地址
#define ICF_LOG_ERR_WRONG_CFG_PATH                (0x88001501) // 配置文件路径有误
#define ICF_LOG_ERR_LOG_INFO                      (0x88001502) // 配置文件LOG_INFO错误
#define ICF_LOG_ERR_LOG_CFG_LEVEL                 (0x88001503) // 配置文件LOG_LEVEL错误
#define ICF_LOG_ERR_LOG_SWITCH                    (0x88001504) // 配置文件LOG_SWITCH错误
#define ICF_LOG_ERR_LOG_TYPE                      (0x88001505) // 配置文件LOG_TYPE错误
#define ICF_LOG_ERR_LOG_PATH                      (0x88001506) // 配置文件LOG_PATH错误
#define ICF_LOG_ERR_LOG_MAXLENGTH                 (0x88001507) // 配置文件LOG_MAX_LENGTH错误
#define ICF_LOG_ERR_NULL_PTR                      (0x88001508) // 空指针
#define ICF_LOG_ERR_INPUT                         (0x88001509) // 配置文件有误
#define ICF_LOG_ERR_FILE                          (0x8800150A) // 文件操作有误
#define ICF_LOG_ERR_FILE_OVERFLOW                 (0x8800150B) // 文件大小达到极限
#define ICF_LOG_ERR_LOG_LEVEL                     (0x8800150C) // level传参错误

// 调度层错误码
#define ICF_E_SCHD_BASE                           (0x88002000) // NODE节点管理错误码
#define ICF_E_SCHD_UNINIT                         (0x88002001) // 未初始化
#define ICF_E_SCHD_PARAM                          (0x88002002) // 参数错误
#define ICF_E_SCHD_CREATE                         (0x88002003) // 创建对象失败
#define ICF_E_SCHD_NOTEXIT                        (0x88002004) // 不存在
#define ICF_E_SCHD_NOTFOUND                       (0x88002005) // 未找到
#define ICF_E_SCHD_EMPTY                          (0x88002006) // 结果为空
#define ICF_E_SCHD_NONSUPPORT                     (0x88002007) // 不支持
#define ICF_E_SCHD_REPEAT                         (0x88002008) // 重复
#define ICF_E_SCHD_SYS_MEMORY                     (0x88002009) // new、malloc等内存相关系统函数调用失败
#define ICF_E_SCHD_ALREADY_START                  (0x8800200A) // 调度器已启动
#define ICF_E_SCHD_POSIX                          (0x8800200B) // 
#define ICF_E_SCHD_BINDCORE_NOTINIT               (0x8800200C) // 绑核时线程池尚未初始化
#define ICF_E_SCHD_BINDCORE_PARAM                 (0x8800200D) // 绑核参数错误
#define ICF_E_SCHD_GET_CPU_INFO_ERR				  (0x8800200E) // 获取CPU信息错误
#define ICF_E_SCHD_BINDCORE_ERR				      (0x8800200F) // 绑核错误

// NODE层错误码
#define ICF_NODEMGR_BASE                          (0x88003000) // NODE节点管理错误码
#define ICF_E_NODEMGR_NULL                        (0x88003001) // 指针为空
#define ICF_E_NODEMGR_PARAM                       (0x88003002) // 参数错误
#define ICF_E_NODEMGR_CREATOBJ                    (0x88003003) // 创建对象失败
#define ICF_E_NODEMGR_NOTEXIT                     (0x88003004) // 不存在
#define ICF_E_NODEMGR_SUPPORT                     (0x88003005) // 不支持
#define ICF_E_NODEMGR_REPETID                     (0x88003006) // 重复ID
#define ICF_E_NODEMGR_MALLOC                      (0x88003007) // 申请内存失败
#define ICF_E_NODEMGR_EXCEED                      (0x88003008) // 路数超过
#define ICF_E_NODEMGR_REINIT                      (0x88003009) // 重复初始化
#define ICF_E_NODEMGR_NOINITE                     (0x8800300A) // 未初始化
#define ICF_E_NODEMGR_NOTSUPPORT                  (0x8800300B) // 不支持
#define ICF_E_NODEMGR_LACK_RESOURCE               (0x8800300C) // 资源不足
#define ICF_E_NODEMGR_JSONPARAM                   (0x8800300D) // TASK.json配置文件参数异常
#define ICF_E_NODEMGR_INPUTPORT                   (0x8800300E) // 输入端口数超出最大端口数
#define ICF_E_NODEMGR_NOTSUPPORTSHARE             (0x8800300F) // 合并Node不支持共享
#define ICF_E_NODEMGR_NEEDPORTS                   (0x88003010) // 合并Node需要端口
#define ICF_E_NODEMGR_NODESIDE                    (0x88003011) // NODE端配置出错
#define ICF_E_NODEMGR_JSONREADCONFIG              (0x88003012) // TASK.json文件读取失败
#define ICF_E_NODEMGR_GBUFTYPEREPETID             (0x88003013) // TASK.json配置中全局缓存内存类型重复
#define ICF_E_NODEMGR_JSONGBUFPARAM               (0x88003014) // TASK.json配置中全局缓存内存配置错误
#define ICF_E_NODEMGR_JSONTASKTYPE                (0x88003015) // TASK.json配置中TaskType不支持
#define ICF_E_NODEMGR_JSONNODEPARAM               (0x88003016) // TASK.json配置中节点NODE参数异常
#define ICF_E_NODEMGR_JSONCONNECTPARAM            (0x88003017) // TASK.json配置中流水连接CONNECT参数异常
#define ICF_E_NODEMGR_GRAPH_EVENT                 (0x88004018) // 事件控制不属于同一个通道
#define ICF_E_NODEMGR_DECODE_NUM                  (0x88004019) // 同一通道多个解码节点
#define ICF_E_NODEMGR_NODE_INPUT                  (0x88004020) // 没有找到输入目标节点(如不存在解码节点)
#define ICF_E_NODEMGR_NODE_MEM_TYPE_EXCEED        (0x88004021) // 内存种类数量超出范围



#define ICF_NODEEXEC_BASE                         (0x88004000) // NODE节点执行错误码
#define ICF_E_NODEEXEC_NULL                       (0x88004001) // 指针为空
#define ICF_E_NODEEXEC_PARAM                      (0x88004002) // 参数错误
#define ICF_E_NODEEXEC_CREATOBJ                   (0x88004003) // 创建对象失败
#define ICF_E_NODEEXEC_NOTEXIT                    (0x88004004) // 不存在
#define ICF_E_NODEEXEC_JSON                       (0x88004005) // JSON解析错误
#define ICF_E_NODEEXEC_SUPPORT                    (0x88004006) // 不支持
#define ICF_E_NODEEXEC_REPETID                    (0x88004007) // 重复ID
#define ICF_E_NODEEXEC_OVERFLOW                   (0x88004008) // 溢出
#define ICF_E_NODEEXEC_NOTSUPPORT                 (0x88004009) // 分辨率不满足当前算法
#define ICF_E_NODEEXEC_MALLOC                     (0x8800400A) // 分辨率不满足当前算法
#define ICF_E_NODEEXEC_ALGUINIT                   (0x8800400B) // 算子未初始化
#define ICF_E_NODEEXEC_EXIST                      (0x8800400C) // 存在share对象
#define ICF_E_NODEEXEC_BTACH                      (0x8800400D) // 拼帧数有误
#define ICF_E_NODEEXEC_DATALACK                   (0x8800400E) // 存在share对象
#define ICF_E_NODEEXEC_THREAD                     (0x8800400f) // 线程创建失败
#define ICF_E_NODEEXEC_MEM_LEAK                   (0x88004010) // 同步节点/流控节点 源数据释放函数未注册，释放标记也无效。
#define ICF_E_NODEEXEC_RELEASEPKG                 (0x88004011) // PACKAGE包释放失败
#define ICF_E_NODEEXEC_EXIT                       (0x88004012) // exit类型异常，但程序并不直接退出
#define ICF_E_NODESA_CONFIG_NOT_SUCCESS           (0x88004013) // 序列节点config SET/GET失败
#define ICF_E_NODEEXEC_NOTCOPY                    (0x88004014) // Decode节点拷贝模式下拷贝数据失败
#define ICF_E_NODE_RELATION_TYPE                  (0x88004015) // 节点连接关系不存在,一般是内部创建节点连接关系时出现的问题
#define ICF_W_NODE_NO_ROUTER                      (0x88004016) // 没有动态路由逻辑的警告，内部警告，不需要返回。
#define ICF_W_NODE_NO_IDLE_ALGHANDLE              (0x88004017) // 并行节点找不到空闲的算子句柄
#define ICF_E_NODEEXEC_COVER                      (0x88004018) // 队列发生覆盖

// 数据包错误码
#define ICF_E_PACKAGE_DATA_COUNT                  (0x88004F00) // 数据共享计数器Count<0,原因是释放逻辑进入次数和预期不一致
#define ICF_E_PACKAGE_DATA_NUM                    (0x88004F01) // 数据包中数据个数超出数据包最大限制
#define ICF_E_PACKAGE_DATA_FIND                   (0x88004F02) // 数据包中数据未找到
#define ICF_E_PACKAGE_QUEUE_IDX                   (0x88004F03) // 快照数据包中队列下标无效
#define ICF_E_PACKAGE_SEND_MAX                    (0x88004F04) // 快照数据包中发送个数超过最大数

// 算子层错误码
#define ICF_ALG_BASE                              (0x88005000) // 算法层错误码
#define ICF_ALG_NULL                              (0x88005000) // 算子空指针
#define ICF_ALG_MALLOC                            (0x88005001) // 内存申请失败
#define ICF_ALG_PARAM                             (0x88005002) // 参数错误
#define ICF_ALG_INIT                              (0x88005003) // 初始化错误
#define ICF_ALG_FINIT                             (0x88005004) // 反初始化错误
#define ICF_ALG_XML                               (0x88005005) // XML解析错误
#define ICF_ALG_SUPPORT                           (0x88005006) // 不支持的操作
#define ICF_ALG_CREATE                            (0x88005007) // 创建失败 
#define ICF_ALG_XMLPATH                           (0x88005008) // XML路径错误
#define ICF_ALG_XMLWORKPATTERN                    (0x88005009) // XML工作模式配置错误
#define ICF_ALG_PROCESS                           (0x8800500A) // 算子执行失败
#define ICF_ALG_JSON                              (0x8800500B) // JSON解析失败
#define ICF_ALG_LIBLOAD                           (0x8800500C) // 算法库加载失败
#define ICF_ALG_LIBFUNC                           (0x8800500D) // 算法库符号加载失败
// 算子层错误码-算子池
#define ICF_ALG_USING                             (0x88005010) // 算子都被占用
#define ICF_ALG_RECREATE                          (0x88005011) // 独占节点创建后未销毁再次创建
#define ICF_ALG_NO_EXIST                          (0x88005012) // 算子类型不存在
#define ICF_ALG_HANDLE_NUM                        (0x88005013) // 实际算子句柄数小于设定数目，警告
#define ICF_ALG_CLEAR                             (0x88005014) // 资源未完全清除
// 算子层错误码-管理层
#define ICF_ALG_RESOLUTION                        (0x88005020) // 分辨率不支持
#define ICF_ALG_WORKPATTERN                       (0x88005021) // 工作模式不正确
// 算子层错误码-模型池 
#define ICF_ALG_MODEL_NO_EXIST                    (0x88005030) // 获取或归还模型时对应的模型不存在
#define ICF_ALG_MODEL_USING                       (0x88005031) // 获取或归还模型时对应的模型不存在
#define ICF_ALG_WARN_REF_NNULL                    (0x88005032) // 模型引用计算器非空
// 算子层错误码-事件驱动器
#define ICF_ALG_WARN_HANDLE_NULL                  (0x88005033) // 传给事件驱动器的句柄为空
// 算子层错误码-并行节点
#define ICF_ALG_PARANODESENDSTRA                  (0x88005034) // 拆包错误
#define ICF_ALG_PARANODESYNCSTRA                  (0x88005035) // 同步错误
#define ICF_ALG_ERR_END                           (0x880059FF) // 算法层错误码结束

// 算子封装层错误码
// 0x88005A00 ~ 0x88005FFF提供给算子封装作为错误码
#define ICF_ALGP_NULL                             (0x88005a00) // 空指针
#define ICF_ALGP_PACKAGE_PASS                     (0x88005a01) // Package包不需要处理，直接过
#define ICF_ALGP_MEM_MALLOC                       (0x88005a02) // 内存申请失败
#define ICF_ALGP_MEM_FREE                         (0x88005a03) // 内存释放失败
#define ICF_ALGP_USERDATA                         (0x88005a03) // 用户数据错误
#define ICF_ALGP_MODEL_OPEN                       (0x88005a04) // 模型文件打开失败
#define ICF_ALGP_CONFIG_KEY                       (0x88005a05) // 配置KEY不存在
#define ICF_ALGP_CONFIG_SIZE                      (0x88005a06) // 配置大小不正确
#define ICF_ALGP_MODEL_READ                       (0x88005a07) // 模型文件读取失败

#define ICF_ALGP_DFR_COMPARE                      (0x88005a07) // DFR比对错误
#define ICF_ALGP_DFR_LIVENESS                     (0x88005a08) // DFR活体错误

// POS模块错误码
#define ICF_POSMGR_BASE                           (0x88006000) // POS管理模块错误码基址
#define ICF_POSMGR_POSSIZE_INVALID                (0x88006000) // POS管理模块输入POS大小有误
#define ICF_POSMGR_NULL                           (0x88006001) // POS管理模块输入空指针
#define ICF_POSMGR_INIT_SZ_INVALID                (0x88006002) // POS管理模块初始化大小输入有误
#define ICF_POSMGR_RECREATE                       (0x88006003) // 重复创建
#define ICF_POSMGR_ID_NOT_EXIST                   (0x88006004) // 当前ID不存在
#define ICF_POSMGR_F_DATASIZE_INVALID             (0x88006005) // 框架层datasize输入不一致
#define ICF_POSMGR_P_DATASIZE_INVALID             (0x88006006) // 封装层datasize输入不一致
#define ICF_POSMGR_HAVENOT_READ_TASKJSON          (0x88006007) // 调用init之前应该先在ICF_init中读取task.json
#define ICF_POSMGR_TASKJSON_GRAPGNUM_SIZE         (0x88006008) // graphnum读取超过限制
#define ICF_POSMGR_GRAPGNUM_SIZE                  (0x88006009) // graphnum超过限制
#define ICF_POSMGR_GRAPHID                        (0x8800600A) // graphID为负数
#define ICF_POSMGR_NODENUM                        (0x8800600B) // NodeNum超过限制
#define ICF_POSMGR_LACK_MEMORY                    (0x8800600C) // POS模块内存申请不够
#define ICF_POSMGR_REINIT                         (0x8800600D) // 重复初始化
#define ICF_POSMGR_REFINIT                        (0x8800600E) // 重复反初始化或尚未初始化
#define ICF_POSMGR_DUMP_SIZE_INPUT                (0x8800600F) // POS信息打印输入size错误
#define ICF_POSMGR_USER_NOT_CONFIG                (0x88006010) // 用户未在json文件中配置POS信息

// 工具模块错误码
#define ICF_TOOL_BASE                             (0x88007000) // 工具模块错误码基址
#define ICF_TOOL_NULL                             (0x88007001) // 工具模块输入空指针
#define ICF_TOOL_NOTFOUND                         (0x88007002) // 工具模块未找到序号
#define ICF_TOOL_DYNAMIC_ROUTE_NUM                (0x88007003) // 工具模块动态路由到达节点数为负数
#define ICF_TOOL_DYNAMIC_ROUTE_FAILED             (0x88007004) // 工具模块动态路由失败，一般原因是要到达的节点不可达
#define ICF_TOOL_PACKAFE_INVALID                  (0x88007005) // 数据包无效
#define ICF_TOOL_PACKAFE_NOTFOUND                 (0x88007005) // 数据包未找到

// 加解密工具错误码    
#define ICF_DES_BASE                              (0x88008000) // 加解密工具模块错误码基址
#define ICF_DES_INVALID_PARAM                     (0x88008001) // 参数非法

// 监控模块错误码    
#define ICF_MONITOR_BASE                          (0x88009000) // 状态监控模块错误码基址
#define ICF_MONITOR_NULL                          (0x88009001) // 空指针
#define ICF_MONITOR_INVALID_PARAM                 (0x88009002) // 参数非法
#define ICF_MONITOR_NOTINIT                       (0x88009003) // 未初始化
#define ICF_MONITOR_NODATA                        (0x88009004) // 未采集到数据
#define ICF_MONITOR_JSON                          (0x88009005) // 解析配置文件错误
#define ICF_MONITOR_FILE                          (0x88009006) // 配置文件打开错误
#define ICF_MONITOR_INIT                          (0x88009007) // 创建线程失败
#define ICF_MONITOR_ALARM_CALL                    (0x88009008) // 异常报警调用出错

// 平台层错误码
#define ICF_PLATFORM_BASE						  (0x8800A000) // 平台层基址
#define ICF_PLATFORM_NOT_SUPPORT				  (0x8800A001) // 平台不支持
#define ICF_PLATFORM_PARAM_INVALID				  (0x8800A002) // 平台参数错误

#endif /* _ICF_ERROR_CODE_H_ */

