/** @file      ICF_error_code_v2.h
 *  @note      HangZhou Hikvision Digital Technology Co., Ltd.
               All right reserved
 *  @reason:   ICF错误码
 *
 *  @version   v0.9.2
 *  @author    liuzhaozh
 *  @date      2022/1/12
 *  @note      Overwrite error code.
 * 
 *  @author    tianmin
 *  @date      2019/7/12
 */


#ifndef _ICF_ERROR_CODE_H_
#define _ICF_ERROR_CODE_H_


#define ICF_SUCCESS                               (0)


// ----------------------------------------------------内存池错误码----------------------------------------------------

/* @reason: 内存池相关的空指针
// @brief:  计算图对应的内存池句柄为空/内存池指针为空/内存池状态为空/内存表为空/释放内存池内存时输入的内存地址为空/内存池配置参数为空/
// @brief:  VB内存池无效/
*/
#define ICF_MEMPOOL_NULLPOINTER                   (0x88001000)

/* @reason: MALLOC分配内存引起的错误
// @brief:  分配'ICF_MEM_MALLOC'类型内存失败/分配'ICF_K91_MEM_FEATURE'类型内存失败/分配'ICF_P4_MEM_FEATURE'类型内存失败/系统内存申请失败/
*/
#define ICF_MEMPOOL_ERR_MALLOC                    (0x88001001)

/* @reason: 海思接口分配MMZ_NO_CACHE内存引起的错误
// @brief:  分配'ICF_HISI_MEM_MMZ_NO_CACHE'类型内存失败/分配'ICF_HISI_MEM_MMZ_NO_CACHE_PRIORITY'类型内存失败/
*/
#define ICF_MEMPOOL_ERR_HISI_SYS_MMZ_ALLOC        (0x88001002)

/* @reason: 海思接口分配MMZ_CACHE内存引起的错误
// @brief:  分配'ICF_HISI_MEM_MMZ_WITH_CACHE'类型内存失败/分配'ICF_HISI_MEM_MMZ_WITH_CACHE_PRIORITY'类型内存失败/
*/
#define ICF_MEMPOOL_ERR_HISI_SYS_MMZ_CACHE_ALLOC  (0x88001003)

/* @reason: SPACE类型相关的错误
// @brief:  SPACE类型不相等/内存类型超出范围/内存类型不支持/内存类型的数量超出了最大值/
*/
#define ICF_MEMPOOL_ERR_SPACE_TYPE                (0x88001004)

/* @reason: 内存太小引起的错误
// @brief:  内存单元总共包含的内存block单元数<=0/申请的系统内存少于内存池管理所需大小/内存大小<=0/
*/
#define ICF_MEMPOOL_ERR_TOO_SMALL                 (0x88001005)

/* @reason: 缺少内存引起的错误
// @brief:  内存池中可供使用的内存不足/
*/
#define ICF_MEMPOOL_ERR_LACK_MEMORY               (0x88001006)

/* @reason: 超出范围引起的错误
// @brief:  传入内存池待释放的地址错误，超出内存池范围/内存block超出范围/
*/
#define ICF_MEMPOOL_ERR_OUT_RANGE            (0x88001007)

/* @reason: 内存对齐不支持引起的错误
// @brief:  待申请的内存对齐方式不支持/
*/
#define ICF_MEMPOOL_ERR_ALIGNMENT_NOT_SURPPORT    (0x88001008)

/* @reason: 内存池检索方式不支持引起的错误
// @brief:  当内存池检索最先适合的free chunk时，检索方式不支持/当内存池检索最适合的free chunk时，检索方式不支持/
*/
#define ICF_MEMPOOL_ERR_SERARCH_MODE_NOT_SURPPORT (0x88001009)

/* @reason: 释放内存相关的错误
// @brief:  内存重复释放/待释放的内存地址有误/待释放的内存地址未对齐/待释放的内存地址空闲占用情况错误/未找到待释放的指针/
// @brief:  系统内存释放(释放输入内存或中间结果内存)失败/
*/
#define ICF_MEMPOOL_ERR_FREE_MEM_ERROR            (0x8800100A)

/* @reason: 申请的内存大小相关的错误
// @brief:  内存表的内存大小<=0/内存表的block大小<=0/
*/
#define ICF_MEMPOOL_ERR_ALLOC_SIZE                (0x8800100B)

/* @reason: 未初始化调用反初始化
// @brief:  本错误码暂未使用/
*/
#define ICF_MEMPOOL_ERR_NOINIT                    (0x8800100C)

/* @reason: 重复初始化引起的错误
// @brief:  内存池重复初始化/
*/
#define ICF_MEMPOOL_ERR_REINIT                    (0x8800100D)

/* @reason: 内存池中未注册引起的错误
// @brief:  外部未注册系统底层内存申请和释放接口/
*/
#define ICF_MEMPOOL_ERR_NOTREGISTER               (0x8800100F)

/* @reason: 内存越界错误
// @brief:  检查出了内存越界/
*/
#define ICF_MEMPOOL_ERR_MEMOVERSTEP               (0x88001010)

/* @reason: 内存池参数相关的错误
// @brief:  内存类型不支持/内存类型超出范围/内存类型数量超出范围/内存池配置参数的预留字段内容错误/未找到内存类型/外部未注册系统内存申请接口，VB无效/
// @brief:  申请、释放VB内存时，平台不正确/
*/
#define ICF_MEMPOOL_ERR_PARAM                     (0x88001011)

/* @reason: 内存类型重复
// @brief:  内存池配置参数中的内存类型重复/
*/
#define ICF_MEMPOOL_ERR_TYPEREPETID               (0x88001012)

/* @reason: 海思接口分配VB内存引起的错误
// @brief:  分配'ICF_HISI_MEM_VB_REMAP_NONE'类型内存失败/分配'ICF_HISI_MEM_VB_REMAP_NOCACHE'类型内存失败/
// @brief:  分配'ICF_HISI_MEM_VB_REMAP_CACHED'类型内存失败/
*/
#define ICF_MEMPOOL_ERR_HISI_SYS_VB               (0x88001013)

/* @reason: 内存池中某些内容不存在引起的错误
// @brief:  内存池不存在/
*/
#define ICF_MEMPOOL_ERR_NOTEXIST                  (0x88001015)

/* @reason: 内存池内存类型不一致，一般要求HVA序列化、反序列化算子结果内存类型相同
// @brief:  创建HVA算子时内存类型不一致/
*/
#define ICF_MEMPOOL_ERR_TYPE_ERROR                (0x88001016)


// ----------------------------------------------------分级日志模块错误码----------------------------------------------------

/* @reason: 配置文件路径有误
// @brief:  配置文件路径无法打开/
*/
#define ICF_LOG_ERR_WRONG_CFG_PATH                (0x88001501)

/* @reason: 日志信息相关的错误
// @brief:  配置文件中的'LogInfo'关键字不存在/获取'LogInfo'关键字的内容失败/
*/
#define ICF_LOG_ERR_LOG_INFO                      (0x88001502)

/* @reason: 日志等级相关的错误
// @brief:  配置文件中的'logLevel'关键字不存在/'logLevel'关键字的值错误/
*/
#define ICF_LOG_ERR_LOG_CFG_LEVEL                 (0x88001503)

/* @reason: 日志开关相关的错误
// @brief:  配置文件中的'logSwitch'关键字不存在/'logSwitch'关键字的值错误/
*/
#define ICF_LOG_ERR_LOG_SWITCH                    (0x88001504)

/* @reason: 日志类型相关的错误
// @brief:  配置文件中的'logType'关键字不存在/'logType'关键字的值错误/
*/
#define ICF_LOG_ERR_LOG_TYPE                      (0x88001505)

/* @reason: 日志路径相关的错误
// @brief:  配置文件中的'logPath'关键字不存在/
*/
#define ICF_LOG_ERR_LOG_PATH                      (0x88001506)

/* @reason: 日志最大长度相关的错误
// @brief:  配置文件中的'logMaxLength'关键字不存在/'logMaxLength'关键字的值错误/
*/
#define ICF_LOG_ERR_LOG_MAXLENGTH                 (0x88001507)

/* @reason: 日志相关的空指针
// @brief:  类对象指针为空/申请的内存地址为空/解析配置文件得到的结果为空/日志系统配置参数结构体为空/配置参数结构体中的日志等级为空/
// @brief:  配置参数结构体中的日志开关为空/配置参数结构体中的日志类型为空/配置参数结构体中的日志回滚方式为空/配置参数结构体中的日志路径为空/
*/
#define ICF_LOG_ERR_NULL_PTR                      (0x88001508)

/* @reason: 输入相关的错误
// @brief:  日志系统配置参数结构体大小错误/日志系统配置关键字错误/配置参数结构体中的日志等级字符串错误/打开日志路径错误/
// @brief:  配置参数结构体中的日志开关错误/配置参数结构体中的日志最大长度错误/配置参数结构体中的日志是否刷新的值错误/
// @brief:  配置参数结构体中的日志回滚方式的值错误/配置参数结构体中的日志类型错误/配置参数结构体中的日志路径的值为空/
*/
#define ICF_LOG_ERR_INPUT                         (0x88001509)

/* @reason: 文件相关的错误
// @brief:  文件长度<=0/读文件出错/打开文件出错/移动文件流的读写位置出错/将数据写入到文本文件中出错(fprintf)/
*/
#define ICF_LOG_ERR_FILE                          (0x8800150A)

/* @reason: 日志等级相关的错误
// @brief:  日志等级索引错误/
*/
#define ICF_LOG_ERR_LOG_LEVEL                     (0x8800150C)

/* @reason: 日志初始化相关的错误
// @brief:  日志开关初始化失败/
*/
#define ICF_LOG_ERR_LOG_INIT                      (0x8800150D)

/* @reason: 日志Tag相关的错误
// @brief:  配置文件中'logTag'关键字的值类型错误/
*/
#define ICF_LOG_ERR_LOG_TAG                       (0x8800150E)

/* @reason: 日志回滚相关的错误
// @brief:  配置文件中'logRollBack'关键字的值错误/
*/
#define ICF_LOG_ERR_LOG_ROLL_BACK                 (0x8800150F)

/* @reason: 日志刷新相关的错误
// @brief:  配置文件中'logNotFlush'关键字的值类型错误/'logNotFlush'关键字的值内容错误/
*/
#define ICF_LOG_ERR_LOG_NOT_FLUSH                 (0x88001510)


// ----------------------------------------------------调度器模块错误码----------------------------------------------------

/* @reason: 未初始化引起的错误
// @brief:  未开启线程池就已添加任务/
*/
#define ICF_E_SCHD_UNINIT                         (0x88002001)

/* @reason: 调度器参数相关的错误
// @brief:  线程数量超过允许的最大值/任务优先级小于0/核心绑定数量、绑核ID小于0/线程数量错误/线程名称字符串为空/
*/
#define ICF_E_SCHD_PARAM                          (0x88002002)

/* @reason: 不存在引起的错误
// @brief:  协程任务不存在/
*/
#define ICF_E_SCHD_NOTEXIT                        (0x88002004)

/* @reason: 未找到引起的错误
// @brief:  未找到协程/未找到线程/
*/
#define ICF_E_SCHD_NOTFOUND                       (0x88002005)

/* @reason: 结果为空引起的错误
// @brief:  任务队列为空/
*/
#define ICF_E_SCHD_EMPTY                          (0x88002006)

/* @reason: 调度器不支持的操作
// @brief:  调度模式不支持/线程调度策略不支持/线程优先级不支持/线程堆栈大小不支持/
*/
#define ICF_E_SCHD_NONSUPPORT                     (0x88002007)

/* @reason: 调度器系统内存相关的错误
// @brief:  任务管理器、内存池new内存失败/线程new内存失败/
*/
#define ICF_E_SCHD_SYS_MEMORY                     (0x88002009) 

/* @reason: POSIX相关的错误
// @brief:  设置CPU亲和性出错/
*/
#define ICF_E_SCHD_POSIX                          (0x8800200B)

/* @reason: 调度器模块相关的空指针
// @brief:  调度器指针为空/调度参数为空/任务单元为空/调度处理接口为空/负载查询接口为空/接口参数为空/
*/
#define ICF_E_SCHD_NULL                           (0x88002010)


// ----------------------------------------------------计算图层错误码----------------------------------------------------

/* @reason: 计算图层相关的空指针
// @brief:  App参数信息为空/节点配置参数结构体为空/节点配置参数结构体中的配置参数为空/节点为空/回调函数指针为空/类指针为空/工具集配置路径为空/
// @brief:  异步输入数据时，输入数据的指针为空/计算图的顶点为空/数据包为空/计算图指针为空/设备地址配置信息为空/获取到的计算图层对象指针为空/
// @brief:  ICF创建参数或创建句柄为空/ICF初始化参数或初始化句柄为空/模型句柄为空/查找到拆分后的计算图为空/通道句柄为空/ICF分析句柄为空/ICF输出数据为空/
// @brief:  CallBack参数结构体为空/ICF配置参数结构体为空/ICF配置参数数据为空/引擎版本结构体为空/内存大小结构体为空/加载、卸载模型的参数为空/
// @brief:  缩放目标数据为空/获得的日志对象为空/
*/
#define ICF_E_GRAPH_NULL                          (0x88003001)

/* @reason: 销毁计算图引起的错误
// @brief:  销毁远程计算图池中的计算图失败/
*/
#define ICF_E_GRAPH_DESTROY                       (0x88003002)

/* @reason: ICF启动相关的错误
// @brief:  ICF以本地模式启动但是没有计算图/
*/
#define ICF_E_GRAPH_START_MODEL                   (0x88003004)

/* @reason: 远程配置相关的错误
// @brief:  非主GRAPH配置了远程节点/
*/
#define ICF_E_GRAPH_SET_REMOTE                    (0x88003006)

/* @reason: 获取远程配置相关的错误
// @brief:  非主GRAPH获取了远程节点的配置/
*/
#define ICF_E_GRAPH_GET_REMOTE                    (0x88003007)

/* @reason: 图管理器初始化失败
// @brief:  初始化图管理器时，某计算图已经存在/当连接节点时没有找到节点、计算图类型、计算图ID/端口是否有效出现冲突/
*/
#define ICF_E_GRAPH_MGR_INIT                      (0x88003008)

/* @reason: 创建原始图失败
// @brief:  未找到对应节点的算子/未找到节点/
*/
#define ICF_E_GRAPH_ORG_CREATE                    (0x88003009)

/* @reason: 创建执行图失败
// @brief:  在远程计算图池中创建或更新计算图失败/
*/
#define ICF_E_GRAPH_EXE_CREATE                    (0x8800300A)

/* @reason: 计算图层相关的错误
// @brief:  节点ID<=0/回调类型不支持/输入数据大小不正确/同步处理时同时存在解码节点和源节点/解码节点的数量不正确/同步处理时节点有多个输入输出/
// @brief:  ICF初始化参数结构体、创建参数结构体的大小错误/ICF初始化句柄、创建句柄的大小错误/配置文件加解密的标志值错误/ICF初始化时，引擎配置的缓存指针不为空或缓存大小不为0/
// @brief:  ICF初始化句柄、创建句柄、模型句柄不存在或已经被销毁/GraphType、GraphID的数值不在范围内/外部保证最大送入ICF的帧数小于0/ICF创建参数中的缓存大小小于0/
// @brief:  模型参数的字节比较不相等/ICF的输入数据大小不正确/CallBack的参数大小不正确/ICF配置参数大小不正确/ICF配置关键字不正确/版本信息结构体大小不正确/
// @brief:  内存大小参数结构体的大小不正确/模型参数结构体的大小不正确/模型句柄的大小不正确/日志配置参数结构体大小不正确/输入数据的blob格式(类型)不支持/
// @brief:  输入数据的blob数量超出范围/
*/
#define ICF_E_GRAPH_PARAM                         (0x8800300B)

/* @reason: 节点是否存在引起的错误
// @brief:  未根据NodeID找到节点/解码节点不存在/
*/
#define ICF_E_GRAPH_NODE_EXIST                    (0x8800300C)

/* @reason: 创建节点失败
// @brief:  根据节点类型创建节点失败/
*/
#define ICF_E_GRAPH_VERTEX_GET_NODE               (0x8800300D)

/* @reason: 计算图中创建对象相关的错误
// @brief:  创建的调度器对象为空/获得的计算图层对象指针为空/
*/
#define ICF_E_GRAPH_CREATOBJ                      (0x88003010)

/* @reason: 算子不存在引起的错误
// @brief:  未能根据AlgID找到对应的算子/
*/
#define ICF_E_GRAPH_ALG_NOT_EXIST                 (0x88003011)

/* @reason: 计算图中不存在某些内容引起的错误
// @brief:  数据流不存在/本地节点的数据流不存在/未找到本地执行计算图/算子ID不存在/
*/
#define ICF_E_GRAPH_NOTEXIST                      (0x88003012) 

/* @reason: 申请内存失败
// @brief:  内存表中分配出的内存指针为空/
*/
#define ICF_E_GRAPH_MALLOC                        (0x88003015)

/* @reason: 计算图中不支持的操作
// @brief:  算子是允许共享的，但其模型不允许共享/ICF_Create之前计算图就已经存在/
*/
#define ICF_E_GRAPH_NOTSUPPORT                    (0x88003019)

/* @reason: ICF句柄空间相关的错误
// @brief:  ICF句柄空间内的指针申请内存时错误/
*/
#define ICF_E_GRAPH_HANDLE_SPACE                  (0x8800301A)

/* @reason: TASK.json配置文件参数相关的错误
// @brief:  json关键字对应的数据条目类型不正确/数据类型转换错误/json格式错误/分布式模式下'appID'关键字未配置/设备地址为空/
// @brief:  关键字(或关键字数组)'GraphInfo'未配置/计算图ID的数量超出允许范围/计算图ID重复/配置的调度模式不存在/绑核的ID数量超出允许范围/节点ID重复/节点名称重复/
// @brief:  关键字'scheduleMode'的内容类型不正确/绑核模式下，用户不允许配置'threadSchedulePolicy'、'threadPriority'/解码节点数量错误/存在重复类型的Task/
// @brief:  关键字'graphType'、'graphIDs'、'bindCoreIDs'、'graphID'、'cacheSize'、'cacheType'的内容不存在或类型错误/全局缓存配置中存在重复的计算图类型/
// @brief:  关键字'threadNum'、'threadStackSize'、'GlobalCaches'的内容类型或数值大小不正确/配置文件中的内存类型字符串转化为内存类型时出错/
*/
#define ICF_E_GRAPH_JSONPARAM                     (0x8800301B)

/* @reason: alg.json配置文件参数异常
// @brief:  配置文件路径指针为空/
*/
#define ICF_E_GRAPH_JSON_ALGPARAM                 (0x8800301C)

/* @reason: 读取配置文件相关的错误
// @brief:  TASK.json文件读取失败/
*/
#define ICF_E_GRAPH_JSONREADCONFIG                (0x88003022)

/* @reason: 缓存类型重复引起的错误
// @brief:  TASK.json配置中全局缓存内存类型重复/
*/
#define ICF_E_GRAPH_GBUFTYPEREPETID               (0x88003023)

/* @reason: TASK.json配置中节点NODE参数异常
// @brief:  关键字'NodeInfo'、'nodeSides'不存在或其对应的内容不是队列/关键字'nodeType'、'algID'、'portID'、'queueMax'不存在或其内容类型不正确/
// @brief:  节点类型的数值超出允许范围/配置的节点数量不正确/反馈节点行为对应的数值超出允许范围/节点类型数量不正确/线程调度策略不合法/
// @brief:  关键字'nAlgType'、'bModelShare'、'splitPackageMax'、'workMode'、'nBatchNum'、'feedBehave'、'queueBehave'的内容类型不正确/
// @brief:  解析关键字'nodeName'、'pluginDllPath'、'pluginFuncPreProcess'、'pluginFuncPostProcess'、'pluginFuncEventProcess'出错\
// @brief:  解析关键字'bModelShare'、'bAlgShare'、'judgebehave'、'strategyFunc'、'syncFunc'、'nodeIDs'、出错/
// @brief:  绑核ID的数量不正确/端口的数量不正确/解析节点中的资源参数出错/端口ID的数值超出允许范围/队列的最大长度不正确/
// @brief:  同一个节点的NodeID出现重复/事件处理错误使用共享算子/事件处理错误使用多算子节点/算子配置出错/
*/
#define ICF_E_GRAPH_JSONNODEPARAM                 (0x88003026)

/* @reason: TASK.json配置中流水连接CONNECT参数异常
// @brief:  'NodeConnects'关键字不存在/节点连接信息中的'graphID'、'srcNodeID'、'desNodeID'不存在或内容类型不正确/
// @brief:  'desPortID'关键字不存在或其数值不正确/连接关系配置的计算图ID未包含在Task配置中计算图ID中/连接关系中的srcNodeID或desNodeID未包含在Nodes设置的节点ID中/
*/
#define ICF_E_GRAPH_JSONCONNECTPARAM              (0x88003027)

/* @reason: 配置文件路径引起的错误
// @brief:  配置路径文件为空/配置文件路径错误，无法打开/
*/
#define ICF_E_GRAPH_WRONG_JSON_PATH               (0x88003032)

/* @reason: 数据包不足引起的错误
// @brief:  无法根据创建的句柄获得数据包/无法创建新的数据包/
*/
#define ICF_E_GRAPH_SHORT_OF_PKG                  (0x88003033)

/* @reason: 计算图中某些内容已存在引起的错误
// @brief:  计算图已存在/
*/
#define ICF_E_GRAPH_EXISTED                       (0x88003034)

/* @reason: 设备连接相关的错误
// @brief:  分布式下计算图层设备连接失败，未获取到通信通道/
*/
#define ICF_E_GRAPH_DEV_CONNECT                   (0x88003036)

/* @reason: 设备信息相关的错误
// @brief:  未能成功将本地系统服务地址字符串解析设置为参数/网络服务信息无效/
*/
#define ICF_E_GRAPH_DEV_INFO                      (0x88003037)

/* @reason: APP信息注册相关的错误
// @brief:  分布式模式下，注册、获取App信息失败/
*/
#define ICF_E_GRAPH_APP_REGISTER                  (0x88003038)

/* @reason: 和解析相关的空指针
// @brief:  json字符串为空/解析得到的结果为空/解析得到的计算图指针为空/
*/
#define ICF_E_GRAPH_PARSE_NULL                    (0x88003039)


// ----------------------------------------------------NODE层错误码----------------------------------------------------

/* @reason: 空指针
// @brief:  快照数据指针为空/数据包句柄为空/节点SetConfig、GetConfig参数为空/节点输入数据为空/获取到的算子配置信息为空且算子ID超出范围/
// @brief:  算子内存池为空/多输入队列节点无法获得内存/异步数据输入为空/package包信息为空/外部配置的外部缓存释放标记位pUseFlag为空/
// @brief:  前处理和后处理函数为空/获得的UID句柄对象指针为空/设置参数的内存为空/解码数据为空/同步的数据包为空/
*/
#define ICF_E_NODEEXEC_NULL                       (0x88004001)

/* @reason: 参数相关的错误
// @brief:  快照数据包中队列端口下标超出范围/SetConfig、GetConfig参数大小错误或参数为空/节点端口号错误/队列输入端口数超过最大值限制/
// @brief:  同步节点的算子不支持malloc内存/同步节点、解码节点的算子只支持封装类型的算子/外部配置的外部缓存释放标记位pUseFlag的值错误/
// @brief:  SetConfig、GetConfig关键字错误/未找到算子序号/串行拼装算子节点不允许共享/
*/
#define ICF_E_NODEEXEC_PARAM                      (0x88004002)

/* @reason: 不存在引起的错误
// @brief:  数据包不存在/未找到快照数据/共享源节点管理器对象指针对应的count创建失败/
*/
#define ICF_E_NODEEXEC_NOTEXIST                   (0x88004004)

/* @reason: 溢出引起的错误
// @brief:  要发送的数据包个数超出节点队列最大发送长度/用户新创建或复制的数据包数量超出节点队列最大长度/
// @brief:  数据压入多Batch list时超出队列最大值/
*/
#define ICF_E_NODEEXEC_OVERFLOW                   (0x88004008)

/* @reason: 不支持引起的错误
// @brief:  队列行为不支持/分辨率不满足当前算法/算子类型不支持/对每个数据包进行的处理状态类型不支持/并行节点不支持前后处理/
// @brief:  解码节点、多类型输入合并节点(同步节点)算子不允许共享/多输入节点算子不支持HVA模式/多输入节点的判断行为不支持/源节点不支持输入数据/
*/
#define ICF_E_NODEEXEC_NOTSUPPORT                 (0x88004009)

/* @reason: 算子未初始化引起的错误
// @brief:  算子句柄为空/
*/
#define ICF_E_NODEEXEC_ALGUINIT                   (0x8800400B)

/* @reason: PACKAGE包释放失败
// @brief:  释放新建但未下发的数据包时出错/'ICF_PROC_REMOVE'处理状态下释放数据包出错/相同指针类型的多路并行数据包释放出错/
// @brief:  释放数据包内存失败/待释放的数据包为空/
*/
#define ICF_E_NODEEXEC_RELEASEPKG                 (0x88004011)

/* @reason: exit类型异常相关的错误，但程序并不直接退出
// @brief:  非共享数据包释放时数量不为1/
*/
#define ICF_E_NODEEXEC_EXIT                       (0x88004012)

/* @reason: 序列节点config SET/GET失败
// @brief:  并行节点的子节点SetConfig、GetConfig失败/串行拼装算子节点SetConfig、GetConfig失败/
*/
#define ICF_E_NODESA_CONFIG_NOT_SUCCESS           (0x88004013)

/* @reason: 拷贝相关的错误
// @brief:  Decode节点拷贝模式下拷贝数据失败/
*/
#define ICF_E_NODEEXEC_NOTCOPY                    (0x88004014)

/* @reason: 没有动态路由逻辑
// @brief:  路由无效下配置文件进行路由/输出结果动态路由回调返回失败/未找到动态路由Info/
*/
#define ICF_W_NODE_NO_ROUTER                      (0x88004016) 

/* @reason: 没有空闲的算子句柄引起的错误
// @brief:  并行节点找不到空闲的算子句柄，获取算子句柄失败/
*/
#define ICF_W_NODE_NO_IDLE_ALGHANDLE              (0x88004017)

/* @reason: 队列发生覆盖引起的错误
// @brief:  本错误码在通知监控模块记录异常报警时使用/
*/
#define ICF_E_NODEEXEC_COVER                      (0x88004018)

/* @reason: meta data相关的错误
// @brief:  meta data数据类型转换出错/meta data组装出错/
*/
#define ICF_E_NODEEXEC_METADATA                   (0x88004019)

/* @reason: 发送数据错误
// @brief:  源节点发送数据失败/
*/
#define ICF_E_NODEEXEC_SEND_DATA                  (0x8800401A)


// ----------------------------------------------------数据包错误码----------------------------------------------------

/* @reason: 数据共享计数器<0
// @brief:  释放逻辑进入次数和预期不一致/
*/
#define ICF_E_PACKAGE_DATA_COUNT                  (0x88004F00)

/* @reason: 数据个数相关的错误
// @brief:  Blob数量超过限制/数据包中数据的个数/需要移动的源ICF_SOURCE_BLOB的下标超出范围/
*/
#define ICF_E_PACKAGE_DATA_NUM                    (0x88004F01)

/* @reason: 数据包中数据未找到引起的错误
// @brief:  未按照数据指针找到对应的algID/
*/
#define ICF_E_PACKAGE_DATA_FIND                   (0x88004F02)

/* @reason: 快照数据包中队列下标相关的错误
// @brief:  快照数据包中队列下标超出范围/
*/
#define ICF_E_PACKAGE_QUEUE_IDX                   (0x88004F03)

/* @reason: InputData()输入数据参数异常
// @brief:  将ICF_INPUT_DATA转换为PACKAGE时得到的package为空/
*/
#define ICF_E_PACKAGE_INPUTDAT_PARAM              (0x88004F05)

/* @reason: 数据包不存在引起的错误
// @brief:  归还数据包时数据包不存在/
*/
#define ICF_E_PACKAGE_NOT_EXIST                   (0x88004F06)

/* @reason: 数据包重复归还
// @brief:  归还数据包时数据包已经处于归还状态/
*/
#define ICF_E_PACKAGE_REBACK                      (0x88004F06)

/* @reason: 数据包相关的空指针
// @brief:  按照数据指针查找对应的algID的数据为空/待插入的数据包、输入数据指针为空/数据指针(源数据或算子输出内存)为空/同步后输出的数据包指针为空/
// @brief:  数据包句柄、数据指针、数据指针个数为空/数据包句柄中的计算图指针为空/节点数据内存表、源数据内存表为空/待同步的数据包组为空/待合并的数据包为空/
// @brief:  数据缓存为空/
*/
#define ICF_E_PACKAGE_NULLPTR                     (0x88004F06)


// ----------------------------------------------------算子层错误码----------------------------------------------------

/* @reason: 算子相关的空指针
// @brief:  算子插件参数为空指针/模型句柄信息为空/算子句柄信息为空/算子创建参数为空/子图输入输出数据为空/接口指针为空/
// @brief:  事件处理的输入和输出为空/运行时参数为空/数据包为空/类对象指针为空/算子池为空/算子标签信息为空/HVA DAT接口动态调用相关参数/
// @brief:  序列化、反序列化数据为空/HVA序列化参数为空/获取到的meta data为空/
*/
#define ICF_ALG_NULL                              (0x88005000)

/* @reason: 申请内存失败
// @brief:  创建模型时没有申请到内存/产生算子时没有申请到内存/创建节点数据、源数据时分配内存失败/
*/
#define ICF_ALG_MALLOC                            (0x88005001)

/* @reason: 算子参数错误
// @brief:  模型数量超过最大值限制/HVA算子输入输出数据大小不一致/元数据大小错误/模型内存表大小错误/配置参数错误/算子分辨率错误/
// @brief:  序列化、反序列化数据的大小错误/
*/
#define ICF_ALG_PARAM                             (0x88005002)

/* @reason: 初始化相关的错误
// @brief:  算子管理器重复初始化/
*/
#define ICF_ALG_INIT                              (0x88005003)

/* @reason: 反初始化相关的错误
// @brief:  未初始化就进行了反初始化/
*/
#define ICF_ALG_FINIT                             (0x88005004)

/* @reason: XML解析相关的错误
// @brief:  json格式不正确/关键字'AlgInfo'未配置或没有内容或内容类型不是队列/关键字'AlgAttribute'、'AlgParam'未配置/HVA数据类型转换出错/插件动态库地址长度错误/
// @brief:  关键字'InputParam'、'OutputParam'、'dataName'、'dataType'、'metaType'、'modelPath'、'algMemType'、'registerMemType'、'userParam'未配置或其内容的格式不正确/
// @brief:  关键字'protoPath'、'protoMemType'未配置或其内容的格式不正确/
// @brief:  关键字'pluginDllPath'、'modelMemType'、'dlProcType'、'handleNum'、'resolutionFix'、'maxWidth'、'maxHeight'、'minWidth'、'minHeight'配置出错/
// @brief:  关键字'OutputNeedDeserialize'、'registerPath'的类型不正确/函数名长度错误/配置文件中的内存类型字符串转化为内存类型时出错/
*/
#define ICF_ALG_XML                               (0x88005005)

/* @reason: 不支持的操作
// @brief:  平台不支持使用堆栈工具/
*/
#define ICF_ALG_SUPPORT                           (0x88005006)

/* @reason: 创建算子相关的错误
// @brief:  未成功创建HVA算子/
*/
#define ICF_ALG_CREATE                            (0x88005007)

/* @reason: 算子json相关的错误
// @brief:  关键字'algID'、'algType'、'algName'未配置/
*/
#define ICF_ALG_JSON                              (0x8800500B)

/* @reason: 加载库引发的错误
// @brief:  添加插件时加载库出现错误/获取库函数时出错/
*/
#define ICF_ALG_LIBLOAD                           (0x8800500C)

/* @reason: 算法库函数相关的错误
// @brief:  获取算法库句柄函数失败,算法库句柄函数包括:创建模型/销毁模型/创建算法/销毁算法/处理算法/处理事件消息/
// @brief:  配置参数/获取参数配置/获取模型内存大小/获取算法内存大小/获取创建数据流所需内存大小/创建数据流/获取运行时内存大小等/
*/
#define ICF_ALG_LIBFUNC                           (0x8800500D)

/* @reason: 算子类型相关的错误
// @brief:  选择了不支持的算子类型/
*/
#define ICF_ALG_ALGTYPE                           (0x8800500E)

/* @reason: 算子使用引起的错误
// @brief:  获取共享、非共享算子时发现算子未被使用/无法根据算子ID获取算子插件/
*/
#define ICF_ALG_USING                             (0x88005010)

/* @reason: 算子相关的内容不存在
// @brief:  算子销毁模型时没有找到对应的算子类型/算子池不存在/获取共享、非共享算子时池中没有找到对应的算子类型/
// @brief:  返还、删除共享、非共享算子时没有找到对应的算子类型/
*/
#define ICF_ALG_NO_EXIST                          (0x88005012)

/* @reason: 清空算子时引起的错误
// @brief:  共享、非共享算子没有被全部销毁/
*/
#define ICF_ALG_CLEAR                             (0x88005014)

/* @reason: 模型是否存在引发的错误
// @brief:  添加模型时，模型已经存在/删除模型时，模型不存在/归还模型时，模型不存在/
*/
#define ICF_ALG_MODEL_EXIST                       (0x88005030)

/* @reason: 模型使用引起的错误
// @brief:  当需要引用模型时,模型不被节点共享/引用模型时,模型在不同通道存在多份/
*/
#define ICF_ALG_MODEL_USING                       (0x88005031)

/* @reason: 传给事件驱动器的句柄为空
// @brief:  控制对象句柄为空/
*/
#define ICF_ALG_WARN_HANDLE_NULL                  (0x88005033)

/* @reason: 算法层错误码结束
// @brief:  数据包有错误，所有数据包均被释放/
*/
#define ICF_ALG_ERR_END                           (0x880059FF)


// ------------------------------------------------------算子封装层错误码------------------------------------------------------

/* @reason: 算子封装层空指针引起的错误/
// @brief:  算子句柄为空/引擎内部数据(如事件控制器，队列状态等)为空/算子输入输出数据包数组为空/算子输入输出数据包为空/配置数据为空/
*/
#define ICF_ALGP_NULL                             (0x88005a00)

/* @reason: 内存申请失败/
// @brief:  申请内存池内存失败/创建的package为空/
*/
#define ICF_ALGP_MEM_MALLOC                       (0x88005a02)

/* @reason: 内存释放失败/
// @brief:  释放内存池内存失败/
*/
#define ICF_ALGP_MEM_FREE                         (0x88005a03)

/* @reason: 打开模型引发的错误
// @brief:  无法打开模型路径/
*/
#define ICF_ALGP_MODEL_OPEN                       (0x88005a04)

/* @reason: 配置KEY不存在
// @brief:  SetConfig时配置了不支持的key/
*/
#define ICF_ALGP_CONFIG_KEY                       (0x88005a05)

/* @reason: 配置大小不正确
// @brief:  SetConfig时配置的大小和预期不同/
*/
#define ICF_ALGP_CONFIG_SIZE                      (0x88005a06)

/* @reason: 读取模型文件引发的错误
// @brief:  无法读取模型文件/
*/
#define ICF_ALGP_MODEL_READ                       (0x88005a07)

/* @reason: 并非是错误，而是不再继续处理
// @brief:  HVA模式在前后处理中使用/
*/
#define ICF_ALGP_PROCESS_RETURN                   (0x88005a10)

// ------------------------------------------------------POS模块错误码------------------------------------------------------

/* @reason: POS数据大小相关的错误
// @brief:  写入POS的数据大小<=0/
*/
#define ICF_POSMGR_POS_DATA_SIZE                  (0x88006000)

/* @reason: POS模块空指针
// @brief:  写入POS的数据为空/数据包为空/ICF初始化句柄为空/POS缓存输出指针为空/输入内存指针为空/
*/
#define ICF_POSMGR_NULL                           (0x88006001)

/* @reason: 无效初始化引起的错误
// @brief:  POS管理模块初始化时申请的POS缓存大小错误/
*/
#define ICF_POSMGR_INIT_INVALID                   (0x88006002)

/* @reason: 框架层数据大小无效引起的错误
// @brief: 将要写入POS的数据大小和之前的输入大小不一致/
*/
#define ICF_POSMGR_F_DATASIZE_INVALID             (0x88006005)

/* @reason: 封装层数据大小无效引起的错误
// @brief: 每个slot大小和之前的输入大小不一致/
*/
#define ICF_POSMGR_P_DATASIZE_INVALID             (0x88006006)

/* @reason: 计算图数量相关的错误
// @brief:  POS管理模块初始化时Graph ID的总数错误/封装层POS信息中的通道(计算图)数量超过限制/
*/
#define ICF_POSMGR_GRAPGNUM_SIZE                  (0x88006009)

/* @reason: graphID相关的错误
// @brief:  graphID为负数/
*/
#define ICF_POSMGR_GRAPHID                        (0x8800600A)

/* @reason: 节点数量相关的错误
// @brief:  节点数量超过限制/
*/
#define ICF_POSMGR_NODENUM                        (0x8800600B)

/* @reason: 缺少内存引起的错误
// @brief:  POS模块内存申请不够/
*/
#define ICF_POSMGR_LACK_MEMORY                    (0x8800600C)

/* @reason: 重复初始化引起的错误
// @brief:  POS管理器重复初始化/
*/
#define ICF_POSMGR_REINIT                         (0x8800600D)

/* @reason: 反初始化引起的错误
// @brief:  POS管理器反初始化时尚未初始化/
*/
#define ICF_POSMGR_FINIT                          (0x8800600E)

/* @reason: POS输出信息大小相关的错误
// @brief:  POS输出信息的大小不等于输入的大小/
*/
#define ICF_POSMGR_BUF_OUT_SIZE                   (0x8800600F)

/* @reason: 用户未配置引起的错误
// @brief:  用户未在json文件中配置POS信息/
*/
#define ICF_POSMGR_USER_NOT_CONFIG                (0x88006010)


// ----------------------------------------------------工具模块错误码----------------------------------------------------

/* @reason: 工具模块相关的空指针
// @brief:  节点个数指针为空/节点句柄为空/数据包句柄为空/节点标签指针为空/目标节点名称指针为空/目标节点ID指针为空/目标节点下的算子指针为空/
// @brief:  节点端口ID指针为空/Layout格式转换时参数为空/Image格式转换时参数为空/Memory类型转换时参数为空/解析模块结构体指针为空/
// @brief:  UID句柄指针为空/通道句柄为空/数据缓存为空/需要创建的计数器的内存指针为空/快照数据指针为空/
*/
#define ICF_TOOL_NULL                             (0x88007001)

/* @reason: 是否找到引起的错误
// @brief:  找到(已经存在)计数器的内存指针/未找到计数器的内存指针/
*/
#define ICF_TOOL_FOUND                         (0x88007002)

/* @reason: 动态路由失败，一般原因是要到达的节点不可达
// @brief:  节点ID错误/端口ID错误/
*/
#define ICF_TOOL_DYNAMIC_ROUTE_FAILED             (0x88007004)

/* @reason: 数据包无效
// @brief:  数据包标识错误/源数据包句柄和目标数据包句柄相等/
*/
#define ICF_TOOL_PACKAGE_INVALID                  (0x88007005)

/* @reason: 未找到引起的错误
// @brief:  源数据包句柄中的源数据未找到/
*/
#define ICF_TOOL_PACKAGE_NOTFOUND                 (0x88007006)

/* @reason: 类型转换引起的错误
// @brief:  待转换的Blob格式不在范围内/待转换的Image格式不在范围内/待转换的Memory类型不在范围内/转换CNN库GPU内存类型时平台不支持/
// @brief:  内存类型转换时，对应的内存类型名称不支持/HVA数据(组)类型转换时，对应的元数据类型名称不支持/
*/
#define ICF_TOOL_TRAN_TYPE_ERR                    (0x88007007)

/* @reason: 参数相关的错误
// @brief:  解析参数时未得到结果，但该参数却是必须的/解析得到的参数类型不正确/内存表大小超出限制/数据缓存UUID错误/快照数据包中队列数据的下标超出范围/
*/
#define ICF_TOOL_PARAM                            (0x88007008) 

/* @reason: 加解密相关的错误
// @brief:  配置文件加密、解密时的参数不正确/
*/
#define ICF_TOOL_ENCRYPT_DECRYPT                     (0x88008001)


// ----------------------------------------------------监控模块错误码----------------------------------------------------

/* @reason: 监控模块相关的空指针
// @brief:  获取Monitor对象指针为空/配置文件路径为空/
*/
#define ICF_MONITOR_NULL                          (0x88009001)

/* @reason: 参数无效引起的错误
// @brief:  计算图类型无效/
*/
#define ICF_MONITOR_INVALID_PARAM                 (0x88009002)

/* @reason: 未初始化引起的错误
// @brief:  未初始化/
*/
#define ICF_MONITOR_NOTINIT                       (0x88009003)

/* @reason: 未采集到数据引起的错误
// @brief:  未采集到数据/
*/
#define ICF_MONITOR_NODATA                        (0x88009004)

/* @reason: 配置文件相关的错误
// @brief:  监控配置文件无法打开/关键字'switch'不存在/关键字'switch'的值不正确/关键字'infoControl'不存在/关键字'infoControl'的值格式错误/
*/
#define ICF_MONITOR_JSON                          (0x88009005)

/* @reason: 配置文件打开时引起的错误
// @brief:  配置文件打开错误/
*/
#define ICF_MONITOR_FILE                          (0x88009006)

/* @reason: 初始化引起的错误
// @brief:  创建线程失败/
*/
#define ICF_MONITOR_INIT                          (0x88009007)

/* @reason: 异常报警调用引起的错误
// @brief:  异常报警调用出错/
*/
#define ICF_MONITOR_ALARM_CALL                    (0x88009008)


// ----------------------------------------------------平台层错误码----------------------------------------------------

/* @reason: 平台不支持引起的错误
// @brief:  平台不支持/
*/
#define ICF_PLATFORM_NOT_SUPPORT				  (0x8800A001)

/* @reason: 平台参数引起的错误
// @brief:  平台参数错误/
*/
#define ICF_PLATFORM_PARAM_INVALID				  (0x8800A002)


// ----------------------------------------------------RPC模块错误码----------------------------------------------------

/* @reason: 空指针引起的错误
// @brief:  空指针/
*/
#define ICF_DIST_RPC_NULL                         (0x8800B001)

/* @reason: 通信标准引起的错误
// @brief:  通信标准错误/
*/
#define ICF_DIST_RPC_COMBETYPE                    (0x8800B002)

/* @reason: 参数引起的错误
// @brief:  参数错误/
*/
#define ICF_DIST_RPC_PARAM                        (0x8800B003)

/* @reason: 初始化引起的错误
// @brief:  尚未初始化/
*/
#define ICF_DIST_RPC_NOT_INITED                   (0x8800B004)

/* @reason: socket创建引起的错误
// @brief:  socket创建失败/
*/
#define ICF_DIST_RPC_SOCKET_CREATE                (0x8800B005)

/* @reason: socket事件创建引起的错误
// @brief:  socket事件创建失败/
*/
#define ICF_DIST_RPC_SOCKET_EVENT                 (0x8800B006)

/* @reason: socket通信引起的错误
// @brief:  socket通信失败/
*/
#define ICF_DIST_RPC_SOCKET_COMMU                 (0x8800B007)

/* @reason: socket销毁引起的错误
// @brief:  socket销毁失败/
*/
#define ICF_DIST_RPC_SOCKET_DESTORY               (0x8800B008)

/* @reason: server初始化引起的错误
// @brief:  server初始化失败/
*/
#define ICF_DIST_RPC_SERVER_INIT                  (0x8800B009)

/* @reason: 重复引起的错误
// @brief:  重复/
*/
#define ICF_DIST_RPC_REPRATED                     (0x8800B00A)

/* @reason: RPC请求发送引起的错误
// @brief:  RPC请求发送失败/
*/
#define ICF_DIST_RPC_SEND                         (0x8800B00B)

/* @reason: 未完成引起的错误
// @brief:  未完成/
*/
#define ICF_DIST_RPC_NOTFINISH                    (0x8800B00C)

/* @reason: IPC创建引起的错误
// @brief:  IPC创建失败/
*/
#define ICF_DIST_RPC_IPC_CREATE                   (0x8800B00D)

/* @reason: IPC销毁引起的错误
// @brief:  IPC销毁失败/
*/
#define ICF_DIST_RPC_IPC_DESTORY                  (0x8800B00E)

/* @reason: IPC通信引起的错误
// @brief:  IPC通信失败/
*/
#define ICF_DIST_RPC_IPC_COMMU                    (0x8800B00F)

/* @reason: Socket消息引起的错误
// @brief:  Socket消息不符合要求/
*/
#define ICF_DIST_RPC_SOCKET_MSG                   (0x8800B010)


// ----------------------------------------------------代理服务层错误码----------------------------------------------------

/* @reason: 空指针引起的错误
// @brief:  空指针/
*/
#define ICF_DIST_SERVICE_NULL                     (0x8800C001)

/* @reason: 参数引起的错误
// @brief:  参数错误/
*/
#define ICF_DIST_SERVICE_REQUEST_PARAM            (0x8800C002)

/* @reason: 没有通道引起的错误
// @brief:  无法获取连接通道/
*/
#define ICF_DIST_SERVICE_NO_CHANNEL               (0x8800C003)

/* @reason: 没有内存引起的错误
// @brief:  没有内存供申请/
*/
#define ICF_DIST_SERVICE_NO_MEMORY                (0x8800C004)

/* @reason: 服务是否存在引起的错误
// @brief:  该服务不存在/
*/
#define ICF_DIST_SERVICE_SERVER_EXIST             (0x8800C005)

/* @reason: 服务器未启动引起的错误
// @brief:  服务器未启动/
*/
#define ICF_DIST_SERVICE_SERVER_NOT_START         (0x8800C006)

/* @reason: 服务存在引起的错误
// @brief:  服务已经存在/
*/
#define ICF_DIST_SERVICE_SERVICE_EXIST            (0x8800C007)

/* @reason: 服务不存在引起的错误
// @brief:  服务不存在/
*/
#define ICF_DIST_SERVICE_SERVICE_NOT_EXIST        (0x8800C008)

/* @reason: 服务不支持引起的错误
// @brief:  服务不支持/
*/
#define ICF_DIST_SERVICE_NOT_SUPPORT              (0x8800C009)

/* @reason: 文件大小引起的错误
// @brief:  文件写的大小不正确/
*/
#define ICF_DIST_APP_LAUNCH_WRITE_FILE_SIZE       (0x8800C00A)

/* @reason: 创建目录引起的错误
// @brief:  创建目录失败/
*/
#define ICF_DIST_APP_LAUNCH_CREATE_DIR            (0x8800C00B)

/* @reason: 打开文件引起的错误
// @brief:  文件打开失败/
*/
#define ICF_DIST_FILE_OPEN                        (0x8800C00C)

/* @reason: rpc状态引起的错误
// @brief:  rpc状态错误/
*/
#define ICF_DIST_RPC_STATUE                       (0x8800C00D)

/* @reason: 分配资源引起的错误
// @brief:  分配资源失败/
*/
#define ICF_DIST_SERVICE_ALLOC_RESOURCE           (0x8800C00E)

// ----------------------------------------------------全局缓存错误码----------------------------------------------------

/* @reason: 全局缓存配置信息相关的错误
// @brief:  全局缓存配置信息的容器为空/
*/
#define ICF_GLOBAL_CACHE_CONFIG_INFO              (0x8800D001)

/* @reason: 内存限制相关的错误
// @brief:  输入的内存大小超出了配置的最大内存限制/输入的缓存大小大于通道剩余的缓存大小/
*/
#define ICF_GLOBAL_CACHE_ERR_MEM_LIMIT            (0x8800D002)

/* @reason: 命令不支持引起的错误
// @brief:  分布式缓存服务响应不支持的命令/
*/
#define ICF_GLOBAL_CACHE_CMD_NOT_SUPPORT          (0x8800D003)

/* @reason: 全局缓存关键字相关的错误
// @brief:  全局缓存关键字不存在/
*/
#define ICF_GLOBAL_CACHE_KEY_NOT_EXIST            (0x8800D004)

/* @reason: 缓存锁定失败
// @brief:  未能尝试加锁/
*/
#define ICF_GLOBAL_CACHE_LOCK_FAILED              (0x8800D005)

/* @reason: 全局缓存类型相关的错误
// @brief:  配置的缓存类型和输入的缓存类型不一致/
*/
#define ICF_GLOBAL_CACHE_ERR_MEM_TYPE             (0x8800D008)

/* @reason: 全局缓存Master地址相关的空指针
// @brief:  获取到的主设备计算图的地址为空/
*/
#define ICF_GLOBAL_CACHE_MASTER_ADDRESS_NULLPTR   (0x8800D009)

/* @reason: 创建不一致引起的错误
// @brief:  相同的参数在多次创建缓存块时不一致/
*/
#define ICF_GLOBAL_CACHE_CREATE_NOT_CONSIST       (0x8800D00A)

/* @reason: 本地地址相关的错误
// @brief:  创建、释放缓存块时，通信地址为本地地址/获取、归还元数据锁时，通信地址为本地地址/获取、同步元数据时，通信地址为本地地址/
// @brief:  获取数据时，通信地址为本地地址/
*/
#define ICF_GLOBAL_CACHE_LOCAL_ADDRESS            (0x8800D00B)

/* @reason: 全局缓存大小相关的错误
// @brief:  获取到的分布式缓存服务响应的缓存大小和本地不一致/本地的全局缓存块大小小于写入大小/
*/
#define ICF_GLOBAL_CACHE_SIZE                     (0x8800D00D)

/* @reason: 接口不支持引起的错误
// @brief:  强弱一致性接口混用/
*/
#define ICF_GLOBAL_CACHE_INTERFACE_NOT_SUPPORT    (0x8800D00E)

/* @reason: 全局缓存相关的空指针
// @brief:  获取到的分布式缓存服务响应的数据指针为空/本地的全局缓存管理器为空/创建缓存块时输入的缓存关键字、缓存关键字对应的计算图句柄、缓存创建参数为空/
// @brief:  全局缓存信息为空/
*/
#define ICF_GLOBAL_CACHE_NULL                     (0x8800D00F)


#endif /* _ICF_ERROR_CODE_H_ */

