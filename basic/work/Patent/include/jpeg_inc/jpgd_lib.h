/***************************************************************************************************
*
* 版权信息：版权所有 (c) 2018, 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：jpgd_lib.h
* 摘要：JPEG解码库接口头文件

* 版本：3.1.5
* 作者：王昊威
* 日期： 2022年04月11号
* 备注：1、HKAJPGD_GetImageInfo中的HKAJPGD_IMAGE_INFO结构体可以返回质量参数，目前只支持整数IDCT,与海康编码库的质量参数基本一致,可能会存在一定误差

* 版本：3.1.4
* 作者：徐丽英5
* 日期： 2019年08月12号
* 备注：1、去掉appN中marker length的校验，兼容一些appN长度不对的情况下可以支持解码。
*      2、按照断segment得到的length和读取码流的dht_length中的较小值对DHT marker进行读取。
*      3、基于第2点的修改，删除多处length校验。
*      4、兼容报错退出且完整度不够的情况下，output中宽高取偶也可以兼容。
*      5、增加progressive mode 校验，防止内存越界崩溃（liusheng7 20200320加入，以前日期没有该检验）。
*      6、增加progressive数据缺少，也解码过程（liusheng7 20200422加入，以前日期没有该解码逻辑）。
* 版本：3.1.3
* 作者：徐丽英5， 刘晟7
* 日期：2019年5月14日
* 备注：1.在outut中增加 partial_decoded_ratio（部分解码的阈值,完整度超过一定阈值则可以给出相应的状态码供人脸检测）使用
*      2.修复错误码流导致marker断长度错误，segment length不等于实际marker长度，导致解析错误，码流越界访问，将后面数据不合理修改导致的崩溃问题。
*      3.增加某一些解析出来的数据的大小校验，避免数组越界访问，将后面数据不合理修改导致的崩溃问题。
*      4.sof解析时增加对pixel format的校验，使得一些不支持的格式可以提前退出解码。
*      5.在Create函数中增加JPGD_HKA_CheckMemTab，检查内存分配对齐。
       6.四分量图片（采样率：1111，SSE，Neon版本出问题，C语言版正常）转yuv420，出现画面在转换之后，部分像素点取值异常，以及边界隔行花屏
*      7.根据代码分析之后，数值大于255越界之后，通过clip函数返回值为-1，而非255，修改为返回255.
*      8.边界隔行花屏，未对齐的部分仍然为C语言实现，SSE等版本为2行一做，未对齐部分的C版本少做了一行
*      9.Neon版本格式转换，修复S16类型转为U16，避免数据过大，而为负数，导致转换不正确。
*
* 版本：3.1.2
* 作者：徐丽英5
* 日期：2019年1月19日
* 备注：1.修改输出宽高，在420图片输出的时候保持2对齐。
*       2.修改哈弗曼解析时bits_cnt大于码流最大长度，返回错误码。
*       3. sof 中增加宽高校验。
*       4. 增加多次扫描时完成最后一次扫描标记的初始化。
*       5. 修改熵解码模块函数指针初始化函数，消除海思Hi3516AV200平台bus error的错误。
*
* 版本：3.1.1
* 作者：徐丽英5
* 日期：2018年11月30日
* 备注：删除Fast int 相关代码，简化库内部代码，默认出库为int分支。
*
* 版本：3.1.0
* 作者：贾英，曹小强，林超逸，汤友华
* 日期：2018年9月10日
* 备注：更新解码库版本为V3.1,优化int 分支，修改progressive图片解析，增加格式转换接口。
*
* 版本：3.0.2
* 作者：徐丽英5，李晓波
* 日期：2018.7.02
* 备注：1、修正progressive格式下的解码出错的问题；
*       2、增加appn解析时关于length校验的条件；
*       3、添加默认DHT表和默认DQT表。
*       4.修复baseline的图片，三个分量的数据单独送进来解错的情况。
*       5.针对progressive的图片，但是没有扫描到lastscan的情况，进行保护。例如progressive的图片dc和ac 分量没有单独送进来。
*       6.针对三个分量单独送进来的baseline的数据，如果加了水印现在版本是没有办法解析水印信息的。需要后续进行优化。
*
* 版本：3.0.1
* 作者：陈建华yf
* 日期：2018.1.12
* 备注：1、格式转换函数增加返回值
*
* 版本：3.0.0
* 作者：汪仁魁，刘晟，时辉章，叶淑睿，陈方栋，宋晓丹，刘芸，何芸芸，刘思思
* 日期：2017.11.10
* 备注：1、调整解码库架构，提高模块化设计
*      2、提高解码库对JPEG图像格式的兼容性，支持的格式类型见本头文件中HKAJPGD_COLOR_SPACE枚举类型的定义
*
* 版本：2.5.4
* 作者：戚红命、薛瑞、黄崇基、苏辉、万千、王莉7、周璐璐、陈建华yf、徐丽英5
* 日期：2017-2-9
* 备注：历史版本最新已经升级到V2.5.4
***************************************************************************************************/
#ifndef _HKA_JPGD_LIB_H_
#define _HKA_JPGD_LIB_H_

#include "hka_types.h"
#include "video_dec_commom.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
* 库版本和时间
* 版本信息格式为：版本号＋年（7位）＋月（4位）＋日（5位）；
* 其中版本号为：主版本号（6位）＋子版本号（5位）＋修正版本号（5位）
***************************************************************************************************/
// 当前版本号
#define HKAJPGD_MAJOR_VERSION       (3)
#define HKAJPGD_MINOR_VERSION       (1)
#define HKAJPGD_REVISION_VERSION    (4)
// 版本日期
#define HKAJPGD_VER_YEAR            (20)
#define HKAJPGD_VER_MONTH           (1)
#define HKAJPGD_VER_DAY             (2)

/***************************************************************************************************
* 定义动态库导入导出
***************************************************************************************************/
#if defined WIN32 || defined _WIN32
#  define JPGD_CDECL      __cdecl
#  define JPGD_STDCALL    __stdcall
#else
#  define JPGD_CDECL
#  define JPGD_STDCALL
#endif

#if defined WIN32 || defined _WIN32
#  if defined JPGDAPI_EXPORTS
#    define JPGD_EXPORTS    __declspec(dllexport)
#  elif defined JPGDAPI_DLL
#    define JPGD_EXPORTS    __declspec(dllimport)
#  else
#    define JPGD_EXPORTS
#  endif
#else
#  define JPGD_EXPORTS
#endif

#ifndef JPGDAPI
#  define JPGDAPI(rettype)    JPGD_EXPORTS rettype JPGD_CDECL
#endif

/***************************************************************************************************
* JPEG解码库支持的最大图像宽高和颜色分量数，理论上JPEG标准对最大分辨率没有限制，但本解码库要求宽*高不能超过S32的表示范围
***************************************************************************************************/
#define HKAJPGD_IMG_WIDTH_MAX               (46000)    // 算法支持的最大图像宽度
#define HKAJPGD_IMG_HEIGHT_MAX              (46000)    // 算法支持的最大图像宽度

/***************************************************************************************************
* 算法库结构体以及枚举类型定义，接口函数直接使用
***************************************************************************************************/
// 水印参数索引号，用来设置水印开关
typedef enum _HKAJPGD_WATERMARK_VALUE
{   
    HKAJPGD_WATERMARK_OFF  = 0x0000,     // 水印信息解析关闭
    HKAJPGD_WATERMARK_ON   = 0x0001      // 水印信息解析使能
} HKAJPGD_WATERMARK_VALUE;

// 图像信息参数
typedef struct _HKAJPGD_IMAGE_INFO
{
    HKA_U32                 progressive_mode;     // 是否为progressive mode，0为否，1为是
    HKA_U32                 num_components;       // JPEG中颜色分量数目，本解码库最多支持4分量
    HKA_SIZE_I              img_size;             // JPEG图像宽高
    HKA_U32                 pix_format;           // 图像分量采样率  0x22111100
    HKA_U32                 quality;              //质量参数,即海康JPEG编码库与libjpeg-turbo的quality参数
    HKA_S32                 reserved[4];          // 预留字段
} HKAJPGD_IMAGE_INFO;

// 能力集参数
// 只要该参数级image_info发生变化，就需要重新创建解码句柄
// 建议使用memcmp比较image_info内容是否发生变化
typedef struct _HKAJPGD_ABILITY
{
    HKAJPGD_IMAGE_INFO      image_info;           // 与所需内存大小有关的图像信息参数
    HKAJPGD_WATERMARK_VALUE watermark_enable;     // 水印功能是否打开
    HKA_S32                 reserved[4];          // 预留字段
} HKAJPGD_ABILITY;

// 算法参数KEY-PARAM的索引号，用来设置算法控制参数
typedef enum _HKAJPGD_PARAM_KEY
{   
    HKAJPGD_IDX_WATERMARK  = 0x0001     // 水印信息解析使能，0表示关闭，1表示开启，默认开启 
} HKAJPGD_PARAM_KEY;

// 子处理类型
typedef enum _HKAJPGD_FUNC_TYPE
{
    HKAJPGD_FUNC_WATERMARK_INFO  = 1           // 若水印信息解析使能，可通过子处理函数获取水印信息
}HKAJPGD_FUNC_TYPE;

// 输入解码库的码流信息结构体
typedef struct _HKAJPGD_STREAM
{
    HKA_U08             *stream_buf;           // 码流缓冲区指针
    HKA_S32              stream_len;           // 码流总长度
} HKAJPGD_STREAM;

// 解码库输出信息结构体
typedef struct _HKAJPGD_OUTPUT
{
    HKA_S32              decode_time;          // 解码耗时，单位微秒
#ifdef JPGD_FORMAT_CONVERT_TIME
	HKA_S32              format_convert_time;  // 格式转换时间，单位毫秒
#endif
    HKA_IMAGE            image_out;              // 解码输出图像
    HKA_U32              pix_format;             // 图像分量采样信息
    HKA_S32              partial_decoded_ratio;  // 部分解码的阈值,完整度超过一定阈值则可以给出相应的状态码供人脸检
    HKA_S32              reserved[4];            // 预留字段

} HKAJPGD_OUTPUT;

// 解码库输出水印信息结构体
typedef struct _HKAJPGD_WATERMARK_INFO
{
    HKA_S32              len;                  // 水印长度（包含水印头的6个字节的长度）
    HKA_U08             *data;                 // 水印信息指针地址
} HKAJPGD_WATERMARK_INFO;

/***************************************************************************************************
* 接口函数
***************************************************************************************************/

/***************************************************************************************************
* 功  能：解析JPEG文件头，从中获取图像参数
* 参  数：*
*        stream       -I  输入码流信息
*        image_info   -O  输出JPEG图像信息
* 返回值：状态码
* 备  注：
***************************************************************************************************/
JPGDAPI(HKA_STATUS) HKAJPGD_GetImageInfo(HKAJPGD_STREAM     *stream, 
                                         HKAJPGD_IMAGE_INFO *image_info);

/***************************************************************************************************
* 功  能：获取库所需存储信息
* 参  数：*
*        ability   -I  能力集参数指针
*        mem_tab   -O  存储空间参数结构体
* 返回值：状态码
* 备  注：如果mtab[i].size为0，则不需要分配该块内存
***************************************************************************************************/
JPGDAPI(HKA_STATUS) HKAJPGD_GetMemSize(HKAJPGD_ABILITY *ability,
                                       HKA_MEM_TAB      mem_tab[HKA_MEM_TAB_NUM]);

/***************************************************************************************************
* 功  能：创建库实例
* 参  数：*
*        ability  -I  能力集参数指针
*        mem_tab  -I  存储空间参数结构体
*        handle   -O  库实例句柄
* 返回值：状态码
* 备  注：
***************************************************************************************************/
JPGDAPI(HKA_STATUS) HKAJPGD_Create(HKAJPGD_ABILITY *ability,
                                   HKA_MEM_TAB      mem_tab[HKA_MEM_TAB_NUM],
                                   HKA_VOID       **handle);

/***************************************************************************************************
* 功  能：设置算法库参数
* 参  数：*
*        handle    -O  库实例句柄
*        cfg_type  -I  算法库配置设置类型
*        cfg_buf   -I  配置参数内存地址
*        cfg_size  -I  配置参数内存大小
* 返回值：状态码
* 备  注：①cfg_type取值来自HKA_SET_CFG_TYPE枚举类型
*        ②不支持HKA_SET_CFG_RESTART_LIB、HKA_SET_CFG_CALLBACK和HKA_SET_CFG_CHECK_PARAM
***************************************************************************************************/
JPGDAPI(HKA_STATUS) HKAJPGD_SetConfig(HKA_VOID *handle,
                                      HKA_S32   cfg_type,
                                      HKA_VOID *cfg_buf,
                                      HKA_SZT   cfg_size);

/***************************************************************************************************
 * 功  能：获取算法库参数、版本库信息等
 * 参  数：*
 *        handle     -I  库实例句柄
 *        cfg_type   -I  算法库配置设置类型
 *        cfg_buf    -O  配置参数内存地址
 *        cfg_size   -I  配置参数内存大小
 * 返回值：状态码
 * 备  注：①cfg_type取值来自HKA_GET_CFG_TYPE枚举类型
 *        ②获取版本号时，无需创建算法库，将handle设为NULL
 **************************************************************************************************/
JPGDAPI(HKA_STATUS) HKAJPGD_GetConfig(HKA_VOID *handle,
                                      HKA_S32   cfg_type,
                                      HKA_VOID *cfg_buf,
                                      HKA_SZT   cfg_size);

/***************************************************************************************************
* 功  能：算法库处理函数，解码一帧JPEG图像
* 参  数：*
*        handle    -I  库实例句柄
*        in_buf    -I  处理输入参数地址，解码一帧JPEG图像时应为HKAJPGD_STREAM的地址
*        in_size   -I  处理输入参数大小，解码一帧JPEG图像时应为sizeof(HKAJPGD_STREAM)
*        out_buf   -O  处理输出参数地址，解码一帧JPEG图像时应为HKAJPGD_OUTPUT的地址
*        out_size  -I  处理输出参数大小，解码一帧JPEG图像时应为sizeof(HKAJPGD_OUTPUT)
* 返回值：状态码
* 备  注：
***************************************************************************************************/
JPGDAPI(HKA_STATUS) HKAJPGD_Process(HKA_VOID *handle,
                                    HKA_VOID *in_buf,
                                    HKA_SZT   in_size,
                                    HKA_VOID *out_buf,
                                    HKA_SZT   out_size);

/***************************************************************************************************
* 功  能：算法库子处理函数，获取水印信息
* 参  数：*
*        handle         -I   库实例句柄 
*        func_type      -I   算法处理类型，获取水印信息应设置为HKAJPGD_FUNC_WATERMARK_INFO
*        in_buf         -I   处理输入参数地址，获取水印信息时可设置为HKA_NULL
*        in_size        -I   处理输入参数大小，获取水印信息时可设置为0
*        out_size       -I   处理输出参数大小，获取水印信息时应为sizeof(HKAJPGD_WATERMARK_INFO)
*        out_buf        -O   处理输出参数地址，获取水印信息时应为HKAJPGD_WATERMARK_INFO结构体的地址
* 返回值：状态码
* 备  注：①调用此函数需要水印信息解析使能
*        ②如果返回成功，库外部应该根据水印头判断是否为合法的水印信息，库内部不关注水印的合法性
***************************************************************************************************/
JPGDAPI(HKA_STATUS) HKAJPGD_SubFunction(HKA_VOID *handle, 
                                        HKA_S32   func_type, 
                                        HKA_VOID *in_buf, 
                                        HKA_SZT   in_size, 
                                        HKA_VOID *out_buf, 
                                        HKA_SZT   out_size);

/***************************************************************************************************
* 功  能：对于非420格式的图片进行格式转换
* 参  数：*
*        handle        -I   库实例句柄 
*        out_buf       -I   Process解码输出的图像格式
*        data_dst      -O   输出目标图像缓存
* 返回值：状态码
* 备  注：
***************************************************************************************************/
HKA_STATUS HKAJPGD_FormatConvert(HKA_VOID *handle,
                                 HKA_VOID   *out_buf,
                                 HKA_VOID  **data_dst);

#ifdef __cplusplus
}
#endif

#endif // _HKA_JPGD_LIB_H_
