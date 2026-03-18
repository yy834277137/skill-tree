/** 
*@file log_client.h
* @note Hangzhou Hikvision digital technology Limited by Share Ltd. All Right Reserved.
* @brief 
*       本文件提供接口函数、相关的定义和使用说明，请仔细查看！
*       
*       目前日志模块每2s会将缓存中的日志写入文件中
*       
*       注意：大小单位均为字节（byte）
*       
* @author  
* @date  2020/11/5
*/

#ifndef LOG_CLIENT_H_
#define LOG_CLIENT_H_
#ifdef __cplusplus
extern "C" {
#endif    
/* log_msg_write中日志模块名称name的长度 */
#define LOG_MODULE_NAME_SIZE    (28)
    
/* 初始化模块时的消息长度 */
#define LOG_MSG_INIT_SIZE       (128)
    
/* log_msg_write中format的长度，单条日志的最大长度，多余的会被丢弃 */
#define LOG_MAX_SIZE            (1024)
    
/* 日志同步大小的最小值，单位byte 
 * 当接收到的日志达到该值时，将缓存中的日志写入到文件中，设置的太小会导致IO操作频繁
 * 实际大小值为LOG_MSG_SYNC_MINI_SIZE - 1280，
 * 该值不能过小，否则日志在某个瞬间特别多时会出现问题
 */
#define LOG_MSG_SYNC_MINI_SIZE  (200*1024)

/* 日志文件的最小值，单位字节 */
#define LOG_MSG_FILE_MINI_SIZE  (5*1024*1024)

/* 日志文件的单个文件最大值，单位字节 */
/* 对于大于100MB的日志文件，将被拆成多个100MB的文件，不是100MB的整数倍则向上取整 */
#define LOG_SINGLE_FILE_MIN_SIZE  (20*1024*1024)

/* 日志初始化字段 */
#define LOG_MSG_MODULE_INIT     "module_init"
#define LOG_MSG_MODULE_EXIT     "module_exit"

/* bsp模块的名称，log server启动时默认注册 */
#define LOG_MODULE_BSP_NAME "bsp"
    
/* 预设APP和DSP预设的模块名称，各资源组可参考使用 */
#define LOG_MODULE_APP_NAME "app"
#define LOG_MODULE_DSP_NAME "dsp"

    
/* 用于注册模块的字符串
 * 格式对应的内容为：
 *  %s：必须为LOG_MSG_MODULE_INIT 
 *  第一个%d：int 储存日志的文件的大小，单位：字节（byte）
 *  第二个%d：int 同步缓存大小，需要保存的日志达到该值时一起写入到文件中，单位：字节（byte）
 *  第三个%d：int 连接类型，暂不使用，设置为0
 */
#define LOG_MODULE_INIT_FORMAT_STRING "%s %d %d %d"
// 第四个%d: int 用于设置多文件储存的单个日志文件大小，最小值为20MB，单位：字节
#define LOG_MODULE_INIT_FORMAT_STRING_MUTIFILES "%s %d %d %d %d"



/* 日志模块的配置文件，用来设置bsp日志模块的大小，请创建在日志文件的保存目录，下次启动生效 */
/* echo bsp=21M > /home/config/log/log.cfg，单位可指定M，否则为字节 */
#define LOG_MODULE_CONFIG_FILE   "log.cfg"

    
/* log_msg_write中level，消息的等级 */
typedef enum tagLogLevel
{
    LOG_LEVEL_PRINT_ONLY = 0,   /* 日志消息的等级 仅打印，不保存到文件 */
    LOG_LEVEL_PRINT_SAVE,       /* 日志消息的等级 打印且保存到文件 */
    LOG_LEVEL_SAVE_ONLY,        /* 日志消息的等级 不打印，只保存到文件 */
    LOG_LEVEL_RES,          /* 该等级保留，无任何操作 */
    LOG_LEVEL_SYNC,         /* 命令等级 主动同步，将缓存中的日志写入flash的操作 */
    LOG_LEVEL_CMD,          /* 命令等级 用于初始化日志模块 */
    LOG_LEVEL_MAX,          /* 枚举的最大值，必须为最后一个 */
}LogLevel;

#define LOG_FLAG_NO_TIMESTAMP 0x1000


/** <日志模块的使用教程>
 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
 * 1.依赖的库
 *      动态库liblog_client.so，放到lib或lib64中
 * 2.启动服务器，第三个参数为日志保存的路径
 *      ./log_server path /home/config/log &
 *
 * 3.首先需要注册模块
 *  3.1 创建注册接口的消息内容，按照如下格式：
 *      snprintf(msg, sizeof(msg), LOG_MODULE_INIT_FORMAT_STRING, LOG_MSG_MODULE_INIT, logsize, sync_size, net_type);
 *      其中：
 *          msg：       char[]长度为LOG_MSG_INIT_SIZE
 *          logsize：   int 储存日志的文件的大小，单位：字节（byte） 
 *          sync_size:  int 缓存日志最大值，达到时将缓存中的日志写入文件，最小值为LOG_MSG_SYNC_MINI_SIZE，单位：字节（byte） 
 *          net_type:   int 默认为0 
 *  3.2 发送注册命令：
 *      log_msg_write(name, LOG_LEVEL_CMD, msg);
 *      其中，name为模块的名称，同时也是日志文件的名称
 *
 * 4.向已注册的模块发送日志消息
 *      log_msg_write(name, LOG_LEVEL_xxxx, Messag_body);
 *      其中：
 *          name：           日志模块的名称，同时也是日志文件的名称，需要符合linux文件的命名规则；
 *          LOG_LEVEL_xxxx： LogLevel枚举的日志等级，用于设置打印、保存或区分命令类型；
 *          Message_body:    日志消息体，字符串类型的日志内容；
 *
 * [可选]5.立即将缓存中的日志写入到文件
 *  消息等级为LOG_LEVEL_SYNC时，format无效，设置为""即可
 *  同步bsp模块的日志：log_msg_write(LOG_MODULE_BSP_NAME, LOG_LEVEL_SYNC, LOG_FLAG_RES, "");
 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
 */
 
/** 日志模块接口
 * @param name char[LOG_MSG_NAME_SIZE]，模块的名称
 * @param level 消息等级，LogLevel枚举类型
 * @param format 日志消息体，长度不超过LOG_MSG_MAX_SIZE
 *              警告：1.禁止使用va_list类型连续传递可变参数
 *                    2.字符串占位符必须有对应的参数，未知内容的字符串建议个资源组先检查一下，
 *                      %s没有对应参数时会导致奔溃，其他占位符不会导致奔溃但会泄漏信息！！！
 * @return 0：成功；其他：失败
 */
int log_msg_write(char *name, LogLevel level, const char *format, ...);

/*
   客户端退出接口，调用日志模块的程序退出时调用，否则内存泄漏
   释放malloc的资源
*/
int log_client_deinit(const int timeout);
#ifdef __cplusplus
}
#endif 
#endif

