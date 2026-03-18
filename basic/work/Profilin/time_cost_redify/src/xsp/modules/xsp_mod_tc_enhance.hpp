/*
 * @Copyright: HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
 * @file name: xsp_mod_tc_enhance.hpp
 * @Data: 2024-05-14
 * @LastEditor: sunguojian5
 * @LastData: 2024-11-12
 * @Describe: 图像方案-测试体增强模块头文件
 */

 #ifndef _XSP_MOD_TC_ENHANCE_HPP_
 #define _XSP_MOD_TC_ENHANCE_HPP_
 #include "xsp_interface.hpp"
 #include "xray_shared_para.hpp"
 #include <queue>
 #include <vector>
 #include <iostream>
 #include <cmath>
 #include <chrono>
 #include <string>
 
 /*-------------------Isl通用图像处理宏定义-----------------------*/ 
 /* 空指针检查宏定义 */ 
 #define CHECK_PTR_IS_NULL(ptr, errno)              \
     do                                             \
     {                                              \
         if (!ptr)                                  \
         {                                          \
             log_error("the '%s' is NULL\n", #ptr); \
             return (errno);                        \
         }                                          \
     } while (0)
 
 #define ISL_OK (0)                          // 成功
 #define ISL_FAIL (-1)                       // 失败
 #define ISL_XIMAGE_PLANE_MAX (4)            // 图像平面数，最多支持4个平面
 #define XIMG_CONTOURS_GRP_IDX_MAX 254       // 图像中能标定的最多轮廓数，轮廓值范围[1, 254]，排除0和255
 #define XIMG_CONTOURS_NUM_MAX 64            // 图像中能检测到的最多轮廓数
 #define M_PI 3.14159265358979323846         // pi
 #define XIMG_32BIT_Y_VAL(x) ((x) >> 16 & 0xFF)
 #define XIMG_32BIT_U_VAL(x) ((x) >> 8 & 0xFF)
 #define XIMG_32BIT_V_VAL(x) ((x) >> 0 & 0xFF)
 #define XIMG_32BIT_A_VAL(x) ((x) >> 24 & 0xFF)
 #define XIMG_32BIT_B_VAL(x) ((x) >> 16 & 0xFF)
 #define XIMG_32BIT_G_VAL(x) ((x) >> 8 & 0xFF)
 #define XIMG_32BIT_R_VAL(x) ((x) >> 0 & 0xFF)
 
 /* 获取数组成员数量 */
 #define ISL_arraySize(array) (sizeof(array) / sizeof((array)[0]))
 #define ISL_SWAP(x, y) \
     do                 \
     {                  \
         x ^= y;        \
         y = x ^ y;     \
         x = x ^ y;     \
     } while (0)
 
 /* 数值比较 */
 #define ISL_MAX(a, b) ((int32_t)(a) > (int32_t)(b) ? (a) : (b))
 #define ISL_MIN(a, b) ((int32_t)(a) < (int32_t)(b) ? (a) : (b))
 #define ISL_CLIP(a, min, max) (ISL_MIN(ISL_MAX((int32_t)a, (int32_t)min), (int32_t)(max)))
 #define ISL_ABS(x) ((int32_t)(x) >= 0 ? (x) : (-(x)))
 #ifndef ISL_SUB_SAFE // 被减数小于减数则强制置0
 #define ISL_SUB_SAFE(a, b) (((int32_t)(a) > (int32_t)(b)) ? ((a) - (b)) : 0)
 #endif
 #ifndef ISL_SUB_ABS // 两数相减的绝对值
 #define ISL_SUB_ABS(a, b) (((int32_t)(a) > (int32_t)(b)) ? ((a) - (b)) : ((b) - (a)))
 #endif
 #ifndef ISL_DIV_SAFE // 除数为0不做操作
 #define ISL_DIV_SAFE(a, b) (0 == (b) ? (a) : (a) / (b))
 #endif
 /* 对齐操作*/
 #define ISL_align(value, align) (((value) + (align - 1)) & ~(align - 1))
 #define ISL_alignDown(value, align) ((value) & ~(align - 1))
 #define ISL_isAligned(value, align) (((value) & (align - 1)) == 0)
 #define ISL_CEIL(value, align) ((0 == (value) % (align)) ? ((value) / (align)) : ((value) / (align) + 1))
 #define ISL_floor(value, align) (((value) / (align)) * (align))
 
 /* Isl测试体增强宏定义 */ 
 #define PIE_CHART_NUM_IN_TEST4  5           // 单个Test4中的铅饼数量
 #define XIMG_FUSE_ENHANCE_NUM_MAX  5        // 多帧图像的最大融合次数
 
 /* 测试体B处理宏定义 */
 #define XING_TESTB_FB_HEIGHT    150     //测试体B FastBrief点高度
 #define XING_TESTB_FB_WIDTH     400     //测试体B FastBrief点宽度
 
 /* Isl图像数据格式 */
 typedef enum
 {
     /* 无效的数据格式，可用于初始化数据格式变量 */
     ISL_IMG_DATFMT_UNKNOWN,
     /*==================== YUV420格式 ====================*/
     ISL_IMG_DATFMT_YUV420_MASK = 0x100,
     /* NV12，YUV420半平面格式，Y在一个平面，UV在另一平面（交错存储，U在前，V在后），Y、U、V各8bit */
     ISL_IMG_DATFMT_YUV420SP_UV,
     /* NV21，YUV420半平面格式，Y在一个平面，VU在另一平面（交错存储，V在前，U在后），Y、U、V各8bit */
     ISL_IMG_DATFMT_YUV420SP_VU,
     /* iYUV，YUV420平面格式，Y、U、V分别在一个平面，Y在前，U在中，V在后，Y、U、V各8bit */
     ISL_IMG_DATFMT_YUV420P,
     /*==================== YUV422格式 ====================*/
     ISL_IMG_DATFMT_YUV422_MASK = 0x200,
     /*====================   RGB格式  ====================*/
     ISL_IMG_DATFMT_RGB_MASK = 0x400,
     /* ARGB交叉格式，单平面，B在低地址，GR在中，A在高地址，即BBBBBGGGGGRRRRRA，A为1bit，RGB各5bit */
     ISL_IMG_DATFMT_BGRA16_1555,
     /* RGB平面格式，BGR分别在一个平面，B在前，G在中，R在后，R、G、B各8bit */
     ISL_IMG_DATFMT_BGRP,
     /* RGB平面格式，BGRA分别在一个平面，R、G、B、A各8bit */
     ISL_IMG_DATFMT_BGRAP,
     /* RGB交叉格式，单平面，B在低地址，G在中，R在高地址，即BGRBGRBGR...，1个像素24bit，R、G、B各8bit */
     ISL_IMG_DATFMT_BGR24,
     /* ARGB交叉格式，单平面，B在低地址，GR在中，A在高地址，即BGRABGRABGRA...，1个像素32bit，R、G、B、A各8bit */
     ISL_IMG_DATFMT_BGRA32,
     /*====================  XRay格式  ====================*/
     ISL_IMG_DATFMT_XRAY_MASK = 0x800,
     /* XRay单能格式，单平面，1个像素16bit */
     ISL_IMG_DATFMT_SP16,
     /* XRay单能格式，单平面，1个像素8bit */
     ISL_IMG_DATFMT_SP8,
     /* XRay多能平面格式，单平面，低能L在前，高能H在后，L、H各16bit */
     ISL_IMG_DATFMT_LHP,
     /* XRay多能平面格式，单平面，低能L在前，高能H在中，原子序数Z在后，L、H各16bit，Z为8bit */
     ISL_IMG_DATFMT_LHZP,
     /* XRay多能平面格式，单平面，低能L在前，高能H在中，原子序数Z在后，L、H各16bit，Z为16bit */
     ISL_IMG_DATFMT_LHZ16P,
     /* 三通道语义分割预处理数据 为32bit */
     ISL_IMG_DATFMT_MASK,
 } ISL_IMG_DATFMT;
 
 /* 图像统一格式，支持表示BGR，YUV，融合图像，高低能原子序数图像，高低能输入图像 */
 typedef struct
 {
     ISL_IMG_DATFMT enImgFmt;               /* 图像格式 */
     uint32_t u32Width;                         /* 图像宽 */
     uint32_t u32Height;                        /* 图像高 */
     uint32_t u32Stride[ISL_XIMAGE_PLANE_MAX];  /* 图像跨距 */
     VOID* pPlaneVir[ISL_XIMAGE_PLANE_MAX]; /* 每个图像平面的数据起始地址 */
 } XIMAGE_DATA_ST;
 
 /* 视频帧、单幅图像、开窗大小、抠图的矩形区域表示 */
 typedef struct
 {
     /* 视频区域的左偏移 */
     uint32_t left;
     /* 视频区域的上偏移 */
     uint32_t top;
     /* 视频区域的宽度 */
     uint32_t width;
     /* 视频区域的高度 */
     uint32_t height;
 } ISL_VideoCrop;
 
 typedef enum _stXImageFlipMode_
 {
     XIMG_FLIP_NONE = 0,      // 无镜像
     XIMG_FLIP_VERT = 1,      // 竖直方向镜像，高度方向
     XIMG_FLIP_HORZ = 2,      // 水平方向镜像，宽度方向
     XIMG_FLIP_HORZ_VERT = 3, // 先水平镜像再垂直镜像，宽度方向
 } XIMAGE_FLIP_MODE;
 
 /* Isl 通用图像处理函数 */
 int32_t Isl_ximg_fill_color(XIMAGE_DATA_ST * pstImageData, uint32_t u32StartRow, uint32_t u32FillHeight, uint32_t u32StartCol, uint32_t u32FillWidth, uint32_t u32BgValue);
 int32_t Isl_ximg_create_sub(XIMAGE_DATA_ST * pstSrcImg, XIMAGE_DATA_ST * pstDstImg, ISL_VideoCrop * pstCropPrm);
 int32_t Isl_ximg_get_size(XIMAGE_DATA_ST * pstImg, uint32_t * pu32DataSize);
 int32_t Isl_ximg_copy_nb(XIMAGE_DATA_ST * pstImageSrc, XIMAGE_DATA_ST * pstImageDst, ISL_VideoCrop * pstSrcCropPrm, ISL_VideoCrop * pstDstCropPrm, XIMAGE_FLIP_MODE enFlipMode);
 int32_t Isl_ximg_dump(const char* filepath, const char* name, int32_t idx, XIMAGE_DATA_ST* pstImageData);
 
 /* 测试体几何属性相关结构体定义 */
 typedef enum
 {
     PIE_ORIENT_TOPLEFT,     // 左上
     PIE_ORIENT_TOPRIGHT,    // 右上
     PIE_ORIENT_BOTTOMLEFT,  // 左下
     PIE_ORIENT_BOTTOMRIGHT, // 右下
     PIE_ORIENTATIONS_NUM,   // 朝向数
     PIE_ORIENT_UNDEF        // 未定义，非法值
 } PIE_CAHRT_ORIENT;         // 圆饼朝向，3/4圆的开口方向
 
 typedef struct
 {
     uint32_t u32IdxStart; // 索引起点
     uint32_t u32IdxEnd;   // 索引终点
 } GROUP_RANGE;
 
 // 矩形坐标
 typedef struct tagRext
 {
     uint32_t uiX;      /*起始X坐标*/
     uint32_t uiY;      /*起始Y坐标*/
     uint32_t uiWidth;  /*宽度*/
     uint32_t uiHeight; /*高度*/
 } RECT;
 
 // 纵向边界线，横坐标从PointStart开始，长度为BorderLen，横坐标不同
 typedef struct
 {
     uint32_t u32VerStart;
     uint32_t u32BorderLen;
     uint32_t *pu32HorXCoor; // 数组个数为xraw_height_max
 } BORDER_VER;
 
 // 横向边界线，横坐标从PointStart开始，长度为BorderLen，纵坐标不同
 typedef struct
 {
     uint32_t u32HorStart;
     uint32_t u32BorderLen;
     uint32_t *pu32VerYCoor; // 数组个数为xraw_width_max
 } BORDER_HOR;
 
 // 位置坐标
 typedef struct tagPosAttrSt
 {
     uint32_t x;
     uint32_t y;
 } POS_ATTR;
 
 typedef struct
 {
     uint32_t pixelCnt;
     GROUP_RANGE endpoints;
     uint32_t lineLen;
     uint32_t lineError;
 
     uint32_t horTh;
     uint32_t *horHist;
     uint32_t horMin;
     uint32_t horMax;
 
     uint32_t verTh; // 直线误差像素数
     uint32_t *verHist;
     uint32_t verMin;
     uint32_t verMax;
 
 } STRAIGHT_LINE;
 
 typedef struct
 {
     POS_ATTR pointPos;         // 角点坐标
     bool bConcave;             // 是否为凹角
     PIE_CAHRT_ORIENT enOrient; // 缺口朝向
     GROUP_RANGE horEp;         // 水平线端点
     GROUP_RANGE verEp;         // 垂直线端点
     uint32_t horLen;               // 角横向长度
     uint32_t verLen;               // 角纵向长度
 } CORNER_ATTR;
 
 typedef struct
 {
     STRAIGHT_LINE lineHor;
     STRAIGHT_LINE lineVer;
     uint32_t cornersNum; // 直角数量
     CORNER_ATTR connersAttr[32];
 } RIGHT_ANGLE;
 
 typedef struct
 {
     uint32_t u32ContourNo;     // 轮廓编号，从1开始计
     uint32_t u32Area;          // 轮廓面积
     uint32_t u32Avg;           // 轮廓灰度均值
     RECT stBoundingBox;    // 轮廓包围盒
     POS_ATTR stStartPoint; // 轮廓起始点
 } CONTOUR_ATTR;
 
 // 轮廓清除方式
 typedef enum
 {
     CONTOUR_EDGE_2_BLACK,       // 轮廓边缘置为黑色（0x0）
     CONTOUR_INSIDE_2_WHITE_HOR, // 轮廓内部（包括边缘）均置为白色（0xFF），横向遍历
     CONTOUR_INSIDE_2_WHITE_VER  // 轮廓内部（包括边缘）均置为白色（0xFF），纵向遍历
 } CONTOUR_CLEAR_MODE;
 
 // 各方向外接边界线
 typedef struct
 {
     BORDER_HOR stTop;
     BORDER_HOR stBottom;
     BORDER_VER stLeft;
     BORDER_VER stRight;
 } BORDER_CIRCUM;
 
 typedef struct // 边界定义，围成的矩形宽为（right-left+1），高为（bottom-top+1）
 {
     uint32_t u32Top;    // 上边界，与图像上沿的距离
     uint32_t u32Bottom; // 下边界，与图像上沿的距离
     uint32_t u32Left;   // 左边界，与图像左沿的距离
     uint32_t u32Right;  // 右边界，与图像左沿的距离
 } XIMG_BORDER;
 
 typedef struct // 点定义
 {
     uint32_t u32X; // 横坐标
     uint32_t u32Y; // 纵坐标
 } XIMG_POINT;
 
 typedef struct // 直线定义
 {
     XIMG_POINT u32PointStart; // 起始点
     XIMG_POINT u32PointEnd;   // 结束点
 } XIMG_LINE;
 
 typedef enum _stXImageRotateMode_
 {
     XIMG_ROTATE_NONE = 0, // 无旋转
     XIMG_ROTATE_90 = 1,   // 旋转90度
     XIMG_ROTATE_180 = 2,  // 旋转180度
     XIMG_ROTATE_270 = 3,  // 旋转270度
 } XIMAGE_ROTATE_MODE;
 
 typedef struct
 {
     int32_t top;    // 上边界
     int32_t bottom; // 下边界
     int32_t left;   // 左边界
     int32_t right;  // 右边界
 } XIMG_BOX;
 
 //   O +-------------------------------------->
 //     |                                      X
 //     | Point(i)
 //     |      +---------- Bottom ---------+
 //     |      |                           |
 //     |      |                           |
 //     |    Left                        Right
 //     |      |                           |
 //     |      |                           |
 //     |      +----------- Top -----------+
 //     |
 //     |
 //     V Y
 typedef enum
 {
     RECT_CORNER_BOTTOMLEFT,  // 坐标点定义为【左下】，对应图像的【左上】
     RECT_CORNER_BOTTOMRIGHT, // 右下
     RECT_CORNER_TOPRIGHT,    // 右上
     RECT_CORNER_TOPLEFT,     // 左上
     RECT_CORNER_NUM
 } RECT_CORNER_POINTS;
 
 typedef struct
 {
     int32_t x;
     int32_t y;
 } POINT_INT;
 
 typedef struct
 {
     double angle;                      // 旋转角度，范围(-45, 45]，大于0为顺时针旋转，小于0为逆时针旋转
     int32_t width;                         // 以angle旋转后重合的边定义为Width
     int32_t height;                        // 与Width垂直的定义为Height
     int32_t area;                          // 旋转矩形的面积
     POINT_INT center;                  // 中心点
     POINT_INT corner[RECT_CORNER_NUM]; // Width边的两个顶点定义为Bottom，对边的两个顶点定义为Top，同一边的Left在左侧，Right在右侧
 } ROTATED_RECT;
 
 /* 测试体缓存条带属性 */
 typedef struct
 {
     unsigned long long  SliceIndex;             // 当前条带序号
     int32_t                 testAEnhanceGrade;      // 增强等级(0-100 int32_t) 信息展示为101-200
     bool                bShowEnhanceInfo;       // 是否在图像上面进行展示增强信息
     bool                bOpenTestBeDet;         // 是否开启测试体B检测
 } XRAY_TEST_ENHANCE_STATE;
 
 /* 测试体缓存条带处理状态 */
 typedef enum
 {
     TEST_SLICE_UNKNOW,                  // 分片状态未知
     TESTB_SLICE_BEGIN,                  // 分片为TESTB区域开始
     TESTB_SLICE_WAITING_END,            // 分片为TESTB区域结束
     TESTA_SLICE_START_MATCH,            // 开始匹配TEST4区域条带
     TESTA_SLICE_END_MATCH_FIRST,        // 结束第一个TEST4区域条带匹配
     TESTA_SLICE_START_MATCH_LINE_PAIR,  // 开始匹配空间分辨条带
     TESTA_SLICE_END_MATCH_LINE_PAIR,    // 结束匹配空间分辨中间区域条带，TEST4条带检测函数需要根据这一标志进行空间分辨增强处理
     TESTA_SLICE_START_MATCH_SECOND,     // 开始匹配第二个TEST4区域条带
     TESTA_SLICE_END_MATCH_SECOND,       // 结束第二个TEST4区域条带匹配
     TESTA_SLICE_WAITING_END,            // 等待第二个TEST4区域条带消失，重新进行匹配
     TESTA_SLICE_STATE_NUM
 } TEST_SLICE_PROCESS_STATE;
 
 // 注意这里的镜像都是针对的处理时的TEST4区域，主要用来代表当时TEST4区域的状态
 typedef struct
 {
     bool bTEST44onTop;     // TEST4区域“TEST4”字符中“4”是否位于上部
     bool bTEST4onLeft;     // TEST4区域“TEST4”字符是否位于左侧
     bool bTEST4VertMirror; // TEST4区域“TEST4”是否进行垂直镜像
     int32_t u32BlendCnt;       // TEST4区域融合次数
     double angle;          // 旋转角度，范围(-45, 45]，大于0为顺时针旋转，小于0为逆时针旋转
 } ISL_TEST4_FUSE_STATE;
 
 // TEST4数据缓存队列结构体
 typedef struct
 {
     XIMAGE_DATA_ST stNrawTEST4ABlend; // TEST4-A区域缓存融合数据
     XIMAGE_DATA_ST stNrawTEST4BBlend; // TEST4-B区域缓存融合数据
     RECT test4BlendRectA;             // 记录融合图像中TEST4的位置信息
     RECT test4BlendRectB;             // 记录融合图像中TEST4的位置信息
     double test4AngleA;               // 旋转角度，范围(-45, 45]，大于0为顺时针旋转，小于0为逆时针旋转
     double test4AngleB;               // 旋转角度，范围(-45, 45]，大于0为顺时针旋转，小于0为逆时针旋转
 } ISL_TEST4_FUSE_DEQUE;
 
 typedef struct
 {
     XIMAGE_DATA_ST stPreNrawSliceData;                       // 前一组包裹的原始NRAW数据
     XIMAGE_DATA_ST stNrawMatchSlice;                         // 包裹的原始NRAW数据
     XIMAGE_DATA_ST stNrawBinarySlice;                        // 基于stNrawMatchSlice的二值化图像 uint8_t
     XIMAGE_DATA_ST stTest4Area;                              // TEST4区域复制用来做增强的数据 第一平面用来存储高能数据
     XIMAGE_DATA_ST stTest4Lhe;                               // TEST4均衡直方图数据
     XIMAGE_DATA_ST stMaskImg;                                // TEST4 mask蒙版图层添加
     uint32_t u32Test4BinTh;                                      // TEST4，穿透分辨测试块，二值化阈值
     uint32_t u32PackageAreaTh;                                   // 包裹面积阈值，不同机型有差异
     uint32_t u32PackageBinTh;                                    // 包裹二值化阈值
     uint32_t u32Neighbor;                                        // 扩边邻域，默认为4Pixelx
     TEST_SLICE_PROCESS_STATE eSliceState;                   // 当前处理的条带状态
     CONTOUR_ATTR stPackageCont;                              // 包裹轮廓
     uint32_t Nrawsliceindex;                                     // NRAW条带index
     ROTATED_RECT stRectTest4;                                // TEST4几何状态
     RECT stRectTest3PackDir;                                 // TEST3区域空间位置（大致）
     uint32_t uTest3LineNum;                                      // TEST3区域条带数量
     uint32_t u32ContourNum;                                      // 轮廓数量
     uint32_t u32ContourAreaMin;                                  // 最小轮廓面积阈值
     CONTOUR_ATTR astContAttr[XIMG_CONTOURS_NUM_MAX];         // 每个条带最多记录XIMG_CONTOURS_NUM_MAX个可能为包裹的轮廓
     GROUP_RANGE aContourGrpRange[XIMG_CONTOURS_GRP_IDX_MAX]; // 各轮廓在数组paContourPoints中，从哪个索引点开始，到哪个索引点结束
     POS_ATTR* paContourPoints;                               // 轮廓点集，计算过程中的临时变量
     BORDER_CIRCUM stBorderCircum;                            // 轮廓包围边界线，计算过程中的临时变量
 
     /* ---秘奥义---测试体究极融合大法，非紧急情况不可使用！！！*/
     std::deque<ISL_TEST4_FUSE_DEQUE> stTest4Deque;           // 使用原始指针管理结构体队列
     uint32_t uBlendImgNum;                                       // 用来进行图像融合的数量
     TEST_SLICE_PROCESS_STATE enPreTest4State;               // 记录上一次送入TEST4的状态
     std::vector<ISL_TEST4_FUSE_DEQUE> stTEST4CacheVector;    // TEST4区域缓存融合数据
     XIMAGE_DATA_ST stTEST4BlendTemp;                         // TEST4-A区域缓存融合数据,用来中间数据融合
     ISL_TEST4_FUSE_STATE stTest4Fused;                       // TEST4区域前后融合结构体
 } XSP_TESTASLICE_ENHANCE;
 
 /* 用于TESTB的简易版本ORB匹配，不支持旋转匹配 */
 typedef struct
 {
     int32_t total;//角点数量限定为400*150
     int32_t* height_index;  
     int32_t* width_index;
     unsigned char* brief;
 } FastBriefPix;
 
 /* 这里保存的bin文件，为如下格式前4个字节保存角点总数，后面400*150*2*4个字节保存对应角点的信息，最后400*150*8个字节保存对应角点的描述符信息 */
 typedef struct
 {
     FastBriefPix TESTBFastPix;          // TESTB特征描述符
     FastBriefPix FastPixDefault_0;      // TESTB特征描述符0
     FastBriefPix FastPixDefault_1;      // TESTB特征描述符1
 } XSP_TESTBSLICE_ENHANCE;
 
 /* 从文件中读取FastBriefPix */
 int32_t LORB_read_FastBriefPix(const char* filename, FastBriefPix* fastpix);
 
 /* 测试体增强类 */
 class TcProcModule : public BaseModule
 {
 public:
     TcProcModule();
 
     /**@fn      Release
      * @brief    释放接口
      * @return   错误码
      * @note
      */
     virtual XRAY_LIB_HRESULT Release();
 
     /**@fn      GetMemSize
      * @brief    获取内存表所需内存大小(字节单位)
      * @param1   MemTab             [O]     - 内存表
      * @param2   nDetectorWidth     [I]     - 探测器宽度
      * @return   错误码
      * @note     算法库所需内存由外部申请, 需提前计算所需内存大小
      */
     virtual XRAY_LIB_HRESULT GetMemSize(XRAY_LIB_MEM_TAB &MemTab, XRAY_LIB_ABILITY &ability);
 
     /**@fn      Init
      * @brief    测试体处理模块初始化
      * @param1   pPara                   [I]     - 公共参数
      * @return   错误码
      * @note
      */
     XRAY_LIB_HRESULT Init(SharedPara *pPara, int32_t nStripeNum, int32_t nUseTc);
 
 public:
     // 测试体增强接口
     TEST_SLICE_PROCESS_STATE IslTcEnhance(XMat& MatLow, XMat& MatHigh, int32_t nWidth, int32_t nHeight, int32_t* pWindowCenter);
 
     // 默认增强参数
     XRAY_TEST_ENHANCE_STATE m_sTcProcParam;
 
     // 条带序号记录
     std::vector<uint32_t> m_vecSliceSeq;
 
     int32_t m_nStripeBufferNum;
 
     // 测试体B条带序号记录
     int32_t m_nTestBStartIdx;
 
 protected:
     // 测试体A条带状态检测函数
     TEST_SLICE_PROCESS_STATE lslTestASliceDetect(XIMAGE_DATA_ST *pstSourceSliceNraw, int32_t rtSliceSeqsize);
 
     // TEST4区域增强前扩边以及内存扩充等
     void IslTest4Expand(XIMAGE_DATA_ST* pSrcPackData,XIMAGE_DATA_ST* pDstPackData,CONTOUR_ATTR* pstTest4Cont);
 
     // TEST4解析增强模块
     int32_t lslTest4AnalyEnhance(XIMAGE_DATA_ST *pstTest4Img, XIMAGE_DATA_ST *pstFuseSrcData, int32_t* pWindowCenter, CONTOUR_ATTR *pstTest4Cont);
 
     // TEST4融合增强模块
     int32_t lslTest4FuseEnhance(XIMAGE_DATA_ST *pstFuseSrcData, CONTOUR_ATTR *pstTest4Cont, TEST_SLICE_PROCESS_STATE enTestCase);
 
     // TEST3线对高低能重融增强模块
     int32_t lslTest3AnalyEnhance(XIMAGE_DATA_ST *pstImageData, int32_t nHProcStart, int32_t nHProcEnd, int32_t nWProcStart, int32_t nWProcEnd);
 
 private:
     // 共享参数
     SharedPara *m_pSharedPara;
 
     // 测试体A条带处理结构体
     XSP_TESTASLICE_ENHANCE m_stTestASlice;
 
     // 测试体B条带处理结构体
     XSP_TESTBSLICE_ENHANCE m_stTestBSlice;
 
 }; // TcProcModule
 
 #endif // _XSP_MOD_TC_ENHANCE_HPP_
 