/***************************************************************************************************
* 
* 版权信息：Copyright (c) 2012, 杭州海康威视数字技术股份有限公司
* 
* 文件名称：video_dec_common.h
* 文件标识：_HIKVISION_VIDEO_DECODER_COMMON_H_
* 摘    要：海康威视视频解码器通用结构体参数头文件
*
* 当前版本：3.7
* 作    者：陈建华yf、方树清、徐丽英5
* 日    期：2018年1月17号
* 备    注：根据H.264、H.265的解码差错隐藏需求增加接口
*
* 历史版本：3.6
* 作    者：陈建华yf
* 日    期：2017年10月27号
* 备    注：增加H.264、H.265解码参考帧外部管理控制参数，增加参考帧外部管理时输出不再参考的参考帧信息
*
* 历史版本：3.5
* 作    者：万千
* 日    期：2015年04月23号
* 备    注：增加解码输出模式参数，增加显示序号输出
*
* 历史版本：3.4
* 作    者：万千
* 日    期：2015年01月14号
* 备    注：增加一个线程错误码
*
* 历史版本：3.3
* 作    者：赵俊 万千
* 日    期：2014年11月24号
* 备    注：HEVC增加log2_ctb_size解码参数
*
* 历史版本：3.2
* 作    者：万千 赵俊
* 日    期：2014年11月10号
* 备    注：新增分块获取内存的方式
*
* 历史版本：3.1
* 作    者：苏辉
* 日    期：2014年08月14号
* 备    注：为HEVC解码进行的扩充
*
* 历史版本：3.0
* 作    者：苏辉
* 日    期：2014年03月14号
* 备    注：为多线程进行的扩充
*
* 历史版本：2.0.1
* 作    者：赵俊 苏辉
* 日    期：2014年04月21号
* 备    注：新增SVAC解码参数
*
* 历史版本：2.0
* 作    者：苏辉
* 日    期：2014年02月24号
* 备    注：新增各种库的更新扩充
*
* 历史版本：1.1.1
* 作    者：黄崇基
* 日    期：2012年07月06号
* 备    注：修正操作失败的状态码为1的问题
*
* 历史版本：1.1
* 作    者：黄崇基
* 日    期：2012年05月04号
* 备    注：在AVC的输出信息中增加标记YUV源范围的flag信息，同时增加了操作失败的状态码
*
* 历史版本：1.0
* 作    者：黄崇基
* 日    期：2011年10月11号
* 备    注：
****************************************************************************************************
*/

#ifndef _VIDEO_DECODER_COMMON_H_
#define _VIDEO_DECODER_COMMON_H_


// 水印类型标记
#define HIK_WATERMARK_INFO                 0x494d5748  // "HWMI": 海康格式
#define USR_WATERMARK_INFO                 0x494d5755  // "UWMI": 用户自定义格式


// 状态码定义
#define HIK_VIDEO_DEC_LIB_S_FAIL           0x00000000  // 操作失败
#define HIK_VIDEO_DEC_LIB_S_OK             0x00000001  // 操作成功
#define HIK_VIDEO_DEC_LIB_S_WAIT           0x00000002  // 等待数据送入，未使用

// 错误码定义
#define HIK_VIDEO_DEC_LIB_E_NULL           0x80000001  // 参数指针为空
#define HIK_VIDEO_DEC_LIB_E_MEM            0x80000002  // 内存为空或内存操作出错
#define HIK_VIDEO_DEC_LIB_E_PARAM          0x80000003  // 输入参数不合法
#define HIK_VIDEO_DEC_LIB_E_BSM_RST        0x80000004  // 码流出错，需重新create的错误
#define HIK_VIDEO_DEC_LIB_E_BSM_NOM        0x80000005  // 码流出错，无需重新create，但可能导致花屏
                                                       // 包括差错隐藏开启后的错误码流也会输出该错误码
#define HIK_VIDEO_DEC_LIB_E_UNSUPPORT_BSM  0x80000006  // 不支持的码流
#define HIK_VIDEO_DEC_LIB_RESETTING        0x80000007  // 接收到强制退出的返回，未使用
#define HIK_VIDEO_DEC_LIB_E_THREAD         0x80000008  // 线程错误，未使用

// 最大支持线程数
#define MAX_THREAD_NUM_SUPPORT             8

// 最大支持内存分块数
#define MAX_BUFFER_BLOCK_NUM              50

// 最大输出帧数
#define MAX_OUTPUT_FRAME_NUM               3

// 解码库所需的最大参考帧数
#define MAX_DPB_NUM                       25

// AVC解码参数集信息结构体
typedef struct _HIK_VIDEO_AVC_INFO_STRU
{
    int          interlace;           // 场编码标记
    unsigned int profile_idc;         // 框架
    unsigned int level_idc;           // 级别
    unsigned int log2_max_frame_num;  // frame_num码字的bit数
    unsigned int reffrm_num;          // 最大参考帧数目
    unsigned int crop_left;           // 左边的裁剪像素值
    unsigned int crop_right;          // 右边的裁剪像素值
    unsigned int crop_top;            // 顶部的裁剪像素值
    unsigned int crop_bottom;         // 底部的裁剪像素值

    int          video_full_rng_flg;  // 标记视频源（YUV）的取值范围，0表示小范围[16，235]，1表示大范围[0,255]
                                      // 这个标记取不同值对应的YUV转RGB采用的矩阵也不同
}VIDEO_AVC_INFO;


// MPEG4解码参数结构体
typedef struct _HIK_VIDEO_MPEG4_INFO_STRU
{
    int  interlace;      // 场编码标记

}VIDEO_MPEG4_INFO;


// JPEG解码参数结构体
typedef struct _HIK_VIDEO_JPEG_INFO_STRU
{
    unsigned int pix_fmt;         // jpeg像素格式
    unsigned int nb_components;   // jpeg中颜色分量数目
    unsigned int progressive_dct; // 编码方式是否为Progressive mode，0为否，非0为是

}VIDEO_JPEG_INFO;


// AVS解码参数结构体
typedef struct _HIK_VIDEO_AVS_INFO_STRU
{
    int  interlace;      // 场编码标记

}VIDEO_AVS_INFO;

// SVAC解码参数结构体
typedef struct _HIK_VIDEO_SVAC_INFO_STRU
{
    int  interlace;      // 场编码标记. 1为帧，0为帧或者场都有可能，当前帧的输出为帧或者场以每帧输出标志为准
    int  vui_frame_rate; // vui中的帧率信息，0为默认25，否则为vui_frame_rate
    int  svc_flag;       // svc标志 1为svc码流，0为非svc码流。变更需要重新GetMemSize和Create处理

}VIDEO_SVAC_INFO;

// HEVC解码参数集信息结构体
typedef struct _HIK_VIDEO_HEVC_INFO_STRU
{
    int          interlace;      // 场编码标记
    int          wpp_sync_flag;  // wpp码流是否支持
    unsigned int reffrm_num;     // 最大参考帧数目
    unsigned int bit_width_flag; // 0: 8bit 1:10bit
    int          log2_ctb_size;  // 亮度CTB大小

    unsigned int crop_left;           // 左边的裁剪像素值
    unsigned int crop_right;          // 右边的裁剪像素值
    unsigned int crop_top;            // 顶部的裁剪像素值
    unsigned int crop_bottom;         // 底部的裁剪像素值

}VIDEO_HEVC_INFO;

// 图像参数集信息，create前需根据解析sps，vol，jpeg文件头等信息来获取，
// 除了codec_specific，其它的成员为公用参数
typedef struct _HIK_VIDEO_INFORMATION_STRU
{
    int img_width;  // 图像宽度
    int img_height; // 图像高度

    union     
    {
        VIDEO_AVC_INFO   *avc_info;    // AVC解码参数集信息
        VIDEO_MPEG4_INFO *mpeg4_info;  // MPEG4解码参数集信息
        VIDEO_JPEG_INFO  *jpeg_info;   // JPEG解码参数集信息
        VIDEO_AVS_INFO   *avs_info;    // AVS解码参数集信息
        VIDEO_SVAC_INFO  *svac_info;   // SVAC解码参数集信息
        VIDEO_HEVC_INFO  *hevc_info;   // HEVC解码参数集信息

    }codec_specific;

    void *ptr_reserved[4];  // 指针保留
    int   int_reserved2[4];  // 数组保留

}VIDEO_INFO;


// AVC解码器能力集设置参数
// 与解码器所需内存大小有关，只要该参数集发生变化，就需要重新创建解码句柄
typedef struct _HIK_VIDEO_AVC_DECODER_PARAM_STRU
{
    int  reffrm_num;         // 参考帧数目，根据sps中的信息获取
    int  ref_frame_out;      // 0：参考帧由解码库内部管理，1：参考帧由解码库外部管理
}AVC_DEC_PARAM;

// HEVC解码器能力集设置参数
// 与解码器所需内存大小有关，只要该参数集发生变化，就需要重新创建解码句柄
typedef struct _HIK_VIDEO_HEVC_DECODER_PARAM_STRU
{
    int  reffrm_num;         // 参考帧数目，根据sps中的信息获取
    int  bit_width_flag;     // 0: 8bit 1:10bit
    int  log2_ctb_size;      // 亮度CTB大小的log2对数
    int  ref_frame_out;      // 0：参考帧由解码库内部管理，1：参考帧由解码库外部管理

}HEVC_DEC_PARAM;


// 海康264解码器设置参数
typedef struct _HIK_VIDEO_HK264_DECODER_PARAM_STRU
{
    int  interlace; // 是否为场编码，0为否，1为是

}HK264_DEC_PARAM;


// JPEG解码器设置参数
typedef struct _HIK_VIDEO_JPEG_DECODER_PARAM_STRU
{
    unsigned int pix_fmt;       // 像素格式
    unsigned int nb_components; // 颜色分量数目
    unsigned int progressive;   // 编码方式是否为Progressive mode，0为否，非0为是

}JPEG_DEC_PARAM;


// SVAC解码器设置参数
typedef struct _HIK_VIDEO_SVAC_DECODER_PARAM_STRU
{
    int  svc_flag;

}SVAC_DEC_PARAM;


// 与申请解码器内存有关的参数集，除了codec_specific，其它的成员为公用参数
typedef struct _HIK_VIDEO_DECODER_PARAMETERS_STRU
{

    unsigned char *dec_buffer;      // 解码器缓存区地址，目前解码库并未使用
    int            dec_buffer_size; // 解码器所需缓存的大小，目前解码库并未使用

    int          img_width;  // 图像宽度，AVC要求16对齐，HEVC要求8对齐
    int          img_height; // 图像高度，AVC要求16对齐，HEVC要求8对齐

    union{
        AVC_DEC_PARAM   *avc_param;    // AVC解码器能力集设置参数
        HK264_DEC_PARAM *hk264_param;  // 海康264解码器能力集设置参数
        JPEG_DEC_PARAM  *jpeg_param;   // JPEG解码器能力集设置参数
        SVAC_DEC_PARAM  *svac_param;   // SVAC解码器能力集设置参数
        HEVC_DEC_PARAM  *hevc_param;   // HEVC解码设能力集置参数

    }codec_specific;

    void           *ptr_reserved[2];   // 指针保留
    unsigned char **buffer_block;      // 缓存块地址数组，长度为MAX_BUFFER_BLOCK_NUM
    unsigned int   *buffer_block_size; // 缓存块大小数组，长度为MAX_BUFFER_BLOCK_NUM，曾发生由于封装层获取分辨率有误导致32bit无符号整型不够表示所需内存的bug，后来播放库对最大分辨率做了限制规避该bug

    int             output_mode;     //0: 不做差错隐藏，出错立即停止输出，播放库等到下一个I帧再送入解码
                                     //1: 错误宏块的YUV全部填充为128，或从参考帧相同位置拷贝数据（可能有花屏）
                                     //2: 利用周围宏块信息，做帧内DC预测，或帧间运动补偿（可能有花屏）
                                     //3: 在2的基础上进行滤波，改善主观质量（可能有花屏）
                                     //4: 包括I帧在内的所有宏块进行帧间运动补偿及滤波（可能有花屏）
    int             int_reserved;   // 保留字段
    int             thread_num;        // 线程个数，最大支持8线程
    int             parallel_mode;     // 0: 帧级别多线程  1：宏块级别多线程，目前解码库并未使用

}VIDEO_DEC_PARAM;


// YUV帧结构
#ifndef _YUV_FRAME_
#define _YUV_FRAME_
typedef struct
{
    unsigned char *y; // 输出图像缓存地址，首地址16字节对齐，内存由库外部申请
                      // 参考帧外部管理时，H.264和H.265解码的Y分量大小为：
                      // HKA_SIZE_ALIGN_16(height) * HKA_SIZE_ALIGN_16(width) + 16
    unsigned char *u; // 输出图像缓存地址，首地址16字节对齐，内存由库外部申请
                      // 参考帧外部管理时，H.264和H.265解码的U分量大小为：
                      // (HKA_SIZE_ALIGN_16(height >> 1) * HKA_SIZE_ALIGN_16(width >> 1) + 16
    unsigned char *v; // 输出图像缓存地址，首地址16字节对齐，内存由库外部申请
                      // 分配大小与U分量相同
}YUV_FRAME;
#endif


// AVC解码处理参数
typedef struct _HIK_VIDEO_AVC_DECODER_PROCESS_PARAM_STRU
{

    int   interlace;      // 是否为两场输出，1为是，0为否 [out]
    int   fld_lost;       // 当intelace为1时，0表示未丢场，1表示顶场丢失，2表示底场丢失 [out]
    int   call_back_type; // 这个变量只有H.264解码有，H.265解码没有。回调函数处理的任务类型：
                          // 0：解码后回调，无论是参考帧内部管理的模式，还是参考帧外部管理的模式，用法相同。以下类型仅针对参考帧外部管理的模式：
                          // 1：frame_num不连续时，填充中间空缺的帧，参考帧外部管理时需要重新进行参考帧释放和分配，分配的YUV地址通过p_frame[1]传进来，p_frame[1].y为空表示外部分配不出YUV节点

}AVCDEC_PROC_PARAM;


// 海康264解码处理参数
typedef struct _HIK_VIDEO_HK264_DECODER_PROCESS_PARAM_STRU
{

    int   mode;          // 输入码流的海康264组模式，参考hik_system.h中关于组类型的宏定义 [in]
    int   frame_amount;  // p_frame中总共输出的帧数，不会超过3 [out]
    int   interlace;     // 是否为两场输出，1为是，0为否 [out]

}HK264DEC_PROC_PARAM;


// AVS解码处理参数
typedef struct _HIK_VIDEO_AVS_DECODER_PROCESS_PARAM_STRU
{
    int   interlace;     // 是否为两场输出，1为是，0为否 [out]

}AVSDEC_PROC_PARAM;

// SVAC解码处理参数
typedef struct _HIK_VIDEO_SVAC_DECODER_PROCESS_PARAM_STRU
{
    int   interlace;     // 是否为两场输出，1为是，0为否 [out]

}SVACDEC_PROC_PARAM;

// HEVC解码处理参数
typedef struct _HIK_VIDEO_HEVC_DECODER_PROCESS_PARAM_STRU
{
    int   interlace;      // 是否为两场输出，1为是，0为否 [out]

}HEVCDEC_PROC_PARAM;

// 允许释放的参考帧数量及地址
typedef struct _HIK_VIDEO_DECODER_UNREF_FRAME
{
    unsigned int   unref_frame_num;              // 允许释放的参考帧个数
    unsigned char *unref_frame_buf[MAX_DPB_NUM]; // 允许释放的参考帧地址

} HIK_VIDEO_DECODER_UNREF_FRAME;

// 解码输入输出参数结构体
// 说明：如果输出是两场，AVC和HEVC两场数据已经做好交织进行存放了，色度输出统一使用I420格式
// 除AVC和HEVC之外的其他解码器两场数据在内存中是分开存放的，内部没有进行交织，色度输出统一使用I420输出
// 除了codec_specific，其它的成员为公用参数
typedef struct _HIK_VIDEO_DECODER_PROCESS_PARAM_STRU
{

    YUV_FRAME p_frame[MAX_OUTPUT_FRAME_NUM]; // 输出帧地址 [in:out]
                                             // HIK264解码时送一帧解三帧，数组中三个元素都会用到
                                             // H.264和H.265时，p_frame[0]用来保存当前帧的YUV地址，p_frame[0].y为空表示外部分配不出YUV节点
                                             // （接上一行）p_frame[1]用来保存fill_frame_gap时帧的YUV地址，p_frame[1].y为空表示外部分配不出YUV节点

    int   width;                             // 输出帧的宽，未裁剪[out]
    int   height;                            // 输出帧的高，未裁剪[out]

    unsigned char *p_bsm;   // 码流缓冲区指针，需要比码流真实长度多申请8KB [in]
                            // 解析后的码流可能被库内部修改，不可重复使用
    int            bsm_len; // 码流的真实长度 [in]

    union{
        AVCDEC_PROC_PARAM   *avc_param;    // AVC解码参数
        HK264DEC_PROC_PARAM *hk264_param;  // 海康264解码参数
        AVSDEC_PROC_PARAM   *avs_param;    // AVS解码参数
        SVACDEC_PROC_PARAM  *svac_param;   // SVAC解码参数
        HEVCDEC_PROC_PARAM  *hevc_param;   // HEVC解码参数

    }codec_specific;

    void *unref_frame;      // 允许释放的参考帧数量及地址，内存由解码库外部分配，参考帧外部管理时不能为空 [in:out]
    void *block_status_map; // 解码库输出的H.264 MB或H.265 CTB状态表，每个MB或CTB占一个字节 [in:out]
                            // H.264解码对应表的大小计算方法如下：
                            // mb_height = HKA_SIZE_ALIGN_16(height) >> 4;
                            // mb_width  = HKA_SIZE_ALIGN_16(width)  >> 4;
                            // size      = mb_height * mb_width;
                            // H.265解码对应表的大小计算方法如下：（log2_ctb_size是SPS中解析出来的，见VIDEO_HEVC_INFO结构体的定义）
                            // ctu_height = (height + ((1 << log2_ctb_size) - 1)) >> log2_ctb_size;
                            // ctu_width  = (width  + ((1 << log2_ctb_size) - 1)) >> log2_ctb_size;
                            // size       = ctu_height * ctu_width;
                            // 该内存由解码库外部分配，当output_mode > 0时，该内存必须有效。
                            // 目前含义如下：
                            // ----------------------------------------------------------------------------------------------
                            // |   bit7   |   bit6   |   bit5   |   bit4   |   bit3   |    bit2   |     bit1    |    bit0   |
                            // ----------------------------------------------------------------------------------------------
                            // | Reserved | Reserved | Reserved | Reserved | Reserved | Reserved  | SLICE_START | BLOCK_ERR |
                            // ----------------------------------------------------------------------------------------------
    void *ptr_reserved[2];  // 指针保留
    int   decode_time;      // 解码耗时（微秒），在H.264、H.265解码库支持输出解码耗时[out]
    int   err_block_num;    // 解码库检测输出的错误宏块或者CTB块数量[out]

    int   display_order;     // 显示顺序序号[out]
    int   yuv_output_format; // 0:yv12 format 1:nv12 format，目前解码库并未使用

}DEC_PROC_PARAM;


#endif

