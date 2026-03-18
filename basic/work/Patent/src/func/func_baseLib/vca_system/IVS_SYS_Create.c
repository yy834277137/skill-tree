/******************************************************************************
* 
* 版权信息: Copyright (c) 2009, 杭州海康威视数字技术股份有限公司
* 
* 文件名称: IVS_SYS_Create.c
* 文件标识: HIKVISION_IVS_SYS_CREATE_C
* 摘    要: 海康威视智能信息系统层创建文件
*
*
* 当前版本: 4.1.5
* 作    者: 辛安民
* 日    期: 2017年12月20号
* 备    注:
*            1.支持规则多种颜色，使用VCA_RULE中reserved的前4个字节
				reserved[0] = 0xdb (R+G+B十六进制和)，表示是颜色版本，reserved[1]为R，reserved[2]为G，reserved[3]为B，表示RGB
			 2. IVS_RULE_DATA_to_system_com/IVS_RULE_DATA_sys_parse_com 打包及解析接口向前兼容
*
*
* 当前版本: 4.1.3
* 作    者: 辛安民
* 日    期: 2017年03月07号
* 备    注:
*            1.增加目标及规则框的打包解析新接口，不再依赖于算法库。
*
* 当前版本: 4.1.2
* 作    者: 辛安民
* 日    期: 2016年11月22号
* 备    注:
*            1.更新vca_common.h到1.2.9版本
*            2.增加对 VCA_TARGET_LIST_V2的打包及解析支持
*            3.注意IVS_META_DATA_to_system/IVS_META_DATA_sys_parse及IVS_META_DATA_to_system_v2/IVS_META_DATA_sys_parse_v2配对使用最优
*			  考虑兼容性IVS_META_DATA_to_system_v2封装的数据可以使用IVS_META_DATA_sys_parse解析，返回HIK_IVS_SYS_LIB_E_DATA_OVERFLOW
*			  此时可以进一步调用IVS_META_DATA_sys_parse_v2；IVS_META_DATA_to_system封装的数据不支持IVS_META_DATA_sys_parse_v2解析。
*			 
*
* 当前版本: 4.1.1
* 作    者: 许江浩
* 日    期: 2014年10月17号
* 备    注: 支持组合报警
*
* 当前版本: 4.1.0
* 作    者: 许江浩
* 日    期: 2014年08月05号
* 备    注: 支持多算法库得到库类型
*
* 当前版本: 4.0.0
* 作    者: 许江浩
* 日    期: 2014年07月23号
* 备    注: 增加打包新接口
*
* 当前版本: 3.1.0
* 作    者: 黄崇基
* 日    期: 2010年10月23号
* 备    注: 增加打包和解包事件列表和异常人脸规则数据的接口
*
* 当前版本: 3.0.0
* 作    者: 黄崇基
* 日    期: 2010年10月12号
* 备    注: 根据智能组更新后的vca_common.h将整个库重新修改，IVS和IAS统一成一个库
*
* 更新版本: 2.3.0
* 作    者: 黄崇基
* 日    期: 2010年7月6号
* 备    注: 增加对IAS数据的封装和解封装支持，仍然调用与IVS一样的接口
*
* 更新版本: 2.2.3
* 作    者: 黄崇基
* 日    期: 2009年10月26日
* 备    注: 增加人脸识别加密数据的封装接口
*
* 更新版本: 2.2.2
* 作    者: 陈军
* 日    期: 2009年7月13日
* 备    注: MetaData中ID由熵编码改为固定长度编码
*
* 更新版本: 2.2.1
* 作    者: 陈军
* 日    期: 2009年5月13日
* 备    注: (1)在IVS_SYS_PARSE.c中，对涉及到个数的参数(如target_num等)进行
*              大小判断，对超过最大值的数，赋值为0或1.
*           (2)在IVS_SYS_Create.c中，IVS_EVENT_DATA_to_system()函数对
*              是否有报警信息进行判断：	if (p_event_data->alert)
*
* 更新版本: 2.2
* 作    者: 陈军
* 日    期: 2009年5月4日
* 备    注: (1)IVS_lib.h，HIK_RULE_INFO结构体中增加两个填充字节用于对齐
*           (2)IVS_SYS_Create.c，IVS_EVENT_DATA_to_system()函数的
*              重新设置PaddingFlag修改为：
*               bm->buf_start[0] |= padding_flag << 4 ;
*
* 更新版本: 2.1
* 作    者: 陈军
* 日    期: 2009年4月17日
* 备    注: IVS2.0接口更新：
*           (1) 去除目前IVS库不支持功能(人数统计，逆行)相关的参数。
*           (2) 去除目前没有使用的预留参数。
*           (3) 将HIK_RULE,和HIK_RULE_INFO两个结构体中的参数联合体，
*               用typedef进行类型定义，减少重复代码。
*           (4) 开放IVS2.0库中新加入的几个参数。
*
* 更新版本: 2.0
* 作    者: 陈军
* 日    期: 2009年4月15日
* 备    注: 根据新接口声明文件(IVS_lib.h 版本2.0)做了修改
*
* 更新版本: 1.1
* 作    者: 陈军
* 日    期: 2009年3月23日
* 备    注: 将RULE_DATA中的保留字节放入循环内部
*
* 更新版本: 1.0
* 作    者: 陈军
* 日    期: 2009年2月27日
* 备    注:
*           
*******************************************************************************
*/

#include <math.h>
#include "IVS_SYS_inner.h"
#include "endian.h"
#include "IVS_SYS_lib.h"

/**********************************************************************************
* Add the new bits to the appropriate buffer, 这个函数最好一次不要写超过16比特
**********************************************************************************/
#define EMIT_BITS(CODE, LEN)                                               \
{                                                                          \
	/* Add the new bits to the bitstream. */                               \
	bm->cnt      += (LEN);                                                 \
    bm->byte_buf |= ((CODE) << (32 - bm->cnt));                            \
                                                                           \
	/* Write out a byte if it is full */                                   \
	while(bm->cnt >= 8)                                                    \
	{                                                                      \
	   *bm->cur_pos++  = (BYTE)(bm->byte_buf >> 24);                       \
		bm->byte_buf <<= 8;                                                \
		bm->cnt       -= 8;                                                \
	}                                                                      \
}


#define BitScanReverse(CODE, BITS)                            \
    {                                                         \
        INT32_T nn = (CODE) >> 1;                             \
        for ((BITS) = 0; ((BITS) < 16) && (nn != 0); (BITS)++)\
        {                                                     \
		    nn >>= 1;                                         \
        }                                                     \
    }

	
#define BitScanReverseLen(CODE, BITS)   \
	    BitScanReverse(CODE, BITS);     \
        (BITS) = (BITS) * 2 + 1;

/******************************************************************************
* 功  能: 熵编码
* 参  数: se   - 待编码数据(in)
*         len  - 熵编码后长度(out)
*         code - 熵编码后数据(out) 
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
// static void se_linfo(INT32_T se, INT32_T *len,INT32_T *code)
// {
//          INT32_T i, n, n1;
// 
//          n = abs(se) << 1;
// 
//          n1 = n + 1;
// 
//          BitScanReverseLen(n1, i);
// 
//          *len = i;
//          *code = n + (se <= 0);
// }

/******************************************************************************
* 功  能: Unsigned integer Exp-Golomb-coded entropy coding
* 参  数: bm  - 内存管理结构体参数(out)
*         n   - 待编码数(int)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void write_n_linfo_UVLC(IVS_SYS_BUF_MANAGER *bm, INT32_T n)
{	
    INT32_T len;
		
    n ++;
	
    BitScanReverseLen(n, len);
	   	
    EMIT_BITS(n, len);	
}

/******************************************************************************
* 功  能: Signed integer Exp-Golomb-coded entropy coding
* 参  数: bm  - 内存管理结构体参数(out)
*         n   - 待编码数(int)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
// static void write_se_linfo_UVLC(IVS_SYS_BUF_MANAGER *bm, INT32_T n)
// {	
//     INT32_T len, code;
// 
//     se_linfo(n, &len, &code);
// 
//     EMIT_BITS(code, len);
// }

/******************************************************************************
* 功  能: 初始化内存管理参数
* 参  数: param - 处理参数结构体 (in)
*         bm    - 内存管理结构体参数(out)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void IVS_CREATE_init_buf_manager(IVS_SYS_PROC_PARAM *param, IVS_SYS_BUF_MANAGER *bm)
{
	bm->scale_width  = param->scale_width;
	bm->scale_height = param->scale_height; 

	bm->cnt       = 0;
    bm->byte_buf  = 0;
    bm->over_flag = 0;
	
	bm->buf_start = param->buf;
	bm->cur_pos   = bm->buf_start;
	bm->buf_size  = param->buf_size;
}

/******************************************************************************
* 功  能: 使输出码流字节对齐
* 参  数: bm   - 内存管理结构体参数(out)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void byte_aligned(IVS_SYS_BUF_MANAGER *bm)
{	
	/* Flush out the last data */
    EMIT_BITS(0, 7);
	
	bm->byte_buf = 0;
	bm->cnt      = 0;	
}

/******************************************************************************
* 功  能: 输出归一化后的边界框结构体数据
* 参  数: bm    - 内存管理结构体参数(out)
*         rect  - 归一化后的边界框结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_rect_normalize_entropy_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RECT_F *rect)
{
	INT32_T fixed_value;

	fixed_value = (INT32_T)(rect->x * bm->scale_width);
    write_n_linfo_UVLC(bm, fixed_value);
	
	/* 表示目标的外接矩形框的左上角的Y轴位置与视频图像宽度的比例(Y*1.0)/bm->scale_height */
	fixed_value = (INT32_T)(rect->y * bm->scale_height);
    write_n_linfo_UVLC(bm, fixed_value);
	
	/*表示目标外接矩形宽的宽度与视频图像宽度的比例(Width*1.0)/bm->scale_width */
    fixed_value = (INT32_T)(rect->width * bm->scale_width);
	write_n_linfo_UVLC(bm, fixed_value);
	
	/*表示目标外接矩形宽的高度与视频图像高度的比例(Height*1.0)/bm->scale_height */
    fixed_value = (INT32_T)(rect->height * bm->scale_height);
	write_n_linfo_UVLC(bm, fixed_value);
}

/******************************************************************************
* 功  能: 输出目标速度信息数据
* 参  数: bm     - 内存管理结构体参数(out)
*         vector - 目标速度信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
// static void emit_vector_info_bits(IVS_SYS_BUF_MANAGER *bm, VCA_SYS_VECTOR_INFO *vector)
// {
// 	INT32_T fixed_value;
// 
// 	/*表示目标沿X轴方向速度相对于视频图像宽度的比例(Vx *1.0) / bm->scale_width */
//     fixed_value = (INT32_T)(vector->vx * bm->scale_width);
//     write_se_linfo_UVLC(bm, fixed_value);
// 	
// 	/*表示目标沿Y轴方向速度相对于视频图像高度的比例(Vy*1.0)/ bm->scale_height */
// 	fixed_value = (INT32_T)(vector->vy * bm->scale_height);
// 	write_se_linfo_UVLC(bm, fixed_value);
// 	
// 	/*表示目标沿X轴方向加速度相对于视频图像宽度的比例(Ax*1.0)/ bm->scale_width */
//     fixed_value = (INT32_T)(vector->ax * bm->scale_width);
// 	write_se_linfo_UVLC(bm, fixed_value);
// 		
//     /*表示目标沿Y轴方向加速度相对于视频图像高度的比例(Ay*1.0)/ bm->scale_height */
//     fixed_value = (INT32_T)(vector->ay * bm->scale_height);
// 	write_se_linfo_UVLC(bm, fixed_value);
// }

/******************************************************************************
* 功  能: 输出经过运动向量差分的路径信息
* 参  数: bm    - 内存管理结构体参数(out)
*         path  - 输出目标轨迹结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
// static void emit_path_bits(IVS_SYS_BUF_MANAGER *bm, VCA_SYS_PATH *path)
// {
// 	INT32_T i;
// 	INT32_T mv_x, mv_y, mvd, fixed_value;
// 	INT32_T mvx_previous = 0, mvy_previous = 0;
// 
// 	// 有效点个数
// 	EMIT_BITS(path->point_num, 5);
// 
// 	// 当前路径的新产生的点
//     fixed_value = (INT32_T)((path->point[0].x) * CONST_13_BITS_OPERATOR);
//     EMIT_BITS(fixed_value, 13);
// 
// 	fixed_value = (INT32_T)((path->point[0].y) * CONST_13_BITS_OPERATOR);
// 	EMIT_BITS(fixed_value, 13);
// 
// 	// 点坐标差分
// 	for (i = 0; i < (INT32_T)path->point_num - 1; i++)
// 	{
// 		mv_x = (INT32_T)((path->point[i+1].x - path->point[i].x) * bm->scale_width);  
// 		mv_y = (INT32_T)((path->point[i+1].y - path->point[i].y) * bm->scale_height); 
// 
//         mvd = mv_x - mvx_previous;
// 		write_se_linfo_UVLC(bm, mvd);
// 
//         mvd = mv_y - mvy_previous;
//         write_se_linfo_UVLC(bm, mvd);
// 
// 		mvx_previous    = mv_x;
// 		mvy_previous    = mv_y;			
// 	}
// }

/******************************************************************************
* 功  能: 输出目标信息结构体数据
* 参  数: bm     - 内存管理结构体参数(out)
*         target - 目标信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_target_bits(IVS_SYS_BUF_MANAGER *bm, VCA_TARGET *target, UINT16_T uVersion)
{
	UINT32_T code = 0;
	//alarm_flg
	EMIT_BITS((target->alarm_flg & 1) << 7, 8);    
	
	//表示目标的类型
	EMIT_BITS(target->type, 4);

	//表示当前目标的生成序号
	//write_n_linfo_UVLC(bm, target->ID);
	EMIT_BITS((target->ID >> 24), 8);
	code = (target->ID & 0x7fff);
	EMIT_BITS(code, 15);
	
    emit_rect_normalize_entropy_bits(bm, &target->rect);

    // reserved[0] = 0xdb (R+G+B十六进制和)，表示是颜色版本，
    //reserved[1]为R，reserved[2]为G，reserved[3]为B，表示RGB
    //每个结构体都填写颜色，实在是浪费，后面可以优化
    if (uVersion == IVS_EXTEN_TYPE_COLOR)
    {
        EMIT_BITS(target->reserved[0], 8);
        EMIT_BITS(target->reserved[1], 8);
        EMIT_BITS(target->reserved[2], 8);
        EMIT_BITS(target->reserved[3], 8);	
    }
    else if (uVersion == IVS_EXTEN_TYPE_RESVERD_ALL)  // 保留所有字段信息
    {
        EMIT_BITS(target->reserved[0], 8);
        EMIT_BITS(target->reserved[1], 8);
        EMIT_BITS(target->reserved[2], 8);
        EMIT_BITS(target->reserved[3], 8);	
        EMIT_BITS(target->reserved[4], 8);	
        EMIT_BITS(target->reserved[5], 8);	
    }

	byte_aligned(bm);
}

/******************************************************************************
* 功  能: 输出填充字节
* 参  数: bm  - 内存管理结构体参数(out)
*         
* 返回值: 返回'1'表示有填充字节，否则就没有
* 备  注:
*******************************************************************************/
static BYTE emit_padding_byte(IVS_SYS_BUF_MANAGER  *bm)
{
	UINT32_T total_size;
	BYTE   padding_num, padding_flag;

    //计算是否需要padding
	total_size = (UINT32_T)(bm->cur_pos - bm->buf_start);
	
	padding_flag = (BYTE)((total_size % 4) != 0);
		
	//padding byte
    if (padding_flag)
	{
		padding_num = (BYTE)((~(total_size) + 1) & 3); 
		
		if (padding_num > 1)
		{
			EMIT_BITS(0xFFFFFF, (padding_num-1) * 8);
		}
		
		// padding num
		EMIT_BITS(padding_num, 8);
    }

	return padding_flag;
		
}

/******************************************************************************
* 功  能: 输出帧内目标信息数组(Meta Data)数据
* 参  数: p_meta_data - 目标信息结构体(in)
*         param       - 处理参数结构体 (out)
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_META_DATA_to_system(VCA_TARGET_LIST    *p_meta_data, 
								IVS_SYS_PROC_PARAM *param)
{   
	IVS_SYS_BUF_MANAGER   buf_manager;
    IVS_SYS_BUF_MANAGER  *bm;	
	INT32_T i;
	BYTE padding_flag;
	UINT32_T code;

	CHECK_ERROR(param == NULL || p_meta_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->scale_width > CONST_13_BITS_OPERATOR 
		     || param->scale_height > CONST_13_BITS_OPERATOR, HIK_IVS_SYS_LIB_E_SCALE_WH);
 
    bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);

	//版本号标示
	EMIT_BITS(0xffff, 16);
	EMIT_BITS(IVS_PACK_VERSION, 16);

    //当前帧视频图像智能算法检测到的目标数量
    EMIT_BITS(p_meta_data->target_num, 8);
	
    //PaddingFlag先设成'0', 后面重新设置
	//PaddingFlag = 0 ExtendByteNum_0 =0  ExtendByteNum_1 = 1  
	EMIT_BITS(1, 8);
    
	//宽度方向上的缩放比例 
	code = (MARKER_BIT << 15) | bm->scale_width;
    EMIT_BITS(code, 16);

	//高度方向上的缩放比例 
	code = (MARKER_BIT << 15) | bm->scale_height;
    EMIT_BITS(code, 16);
             
	for (i = 0; i < p_meta_data->target_num; i++)
	{
        emit_target_bits(bm, &(p_meta_data->target[i]),IVS_PACK_VERSION);
	}

	padding_flag = emit_padding_byte(bm);

	//重新设置PaddingFlag
	bm->buf_start[5] |= padding_flag << 7 ;

    //计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);

	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER); //FIXME:
	
    return HIK_IVS_SYS_LIB_S_OK;
}

/******************************************************************************
* 功  能: 输出计数器结构体数据
* 参  数: bm      -  内存管理结构体指针 (out)
*         counter -  计数器结构体指针   (in)       
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
// static INT32_T emit_counter_bits(IVS_SYS_BUF_MANAGER *bm, VCA_SYS_COUNTER *counter)
// {
// 	UINT32_T counter_h, counter_l;
//     DWORD code;
// 	
// 	if (   counter->counter1 > CONST_30_BITS_MASK
// 		|| counter->counter2 > CONST_30_BITS_MASK)
// 	{
// 		return HIK_IVS_SYS_LIB_E_DATA_OVERFLOW;
// 	}
// 
//     /* counter 1 计数器1代表该事件的触发次数。
// 	 * 当事件为跨线事件时，计数器1统计目标从左到右穿越直线的跨线事件。
// 	 */
// 	counter_h = (counter->counter1 >> 15) & CONST_15_BITS_MASK;
// 	counter_l = counter->counter1 & CONST_15_BITS_MASK;
// 
// 	code = (MARKER_BIT << 15) | counter_h;
// 	EMIT_BITS(code, 16);
// 
//     code = (MARKER_BIT << 15) | counter_l;
//     EMIT_BITS(code, 16);	
// 
//     /* counter 2 当事件为跨线事件时，该计数器才生效，
// 	 * 统计目标从右到左穿越直线的跨线事件。
// 	 */
// 	counter_h = (counter->counter2 >> 15) & CONST_15_BITS_MASK;
// 	counter_l = counter->counter2 & CONST_15_BITS_MASK;
// 	
// 	code = (MARKER_BIT << 15) | counter_h;
// 	EMIT_BITS(code, 16);
// 
//     code = (MARKER_BIT << 15) | counter_l;
//     EMIT_BITS(code, 16);
// 
// 	return HIK_IVS_SYS_LIB_S_OK;
// }

/******************************************************************************
* 功  能: 输出多边型结构体数据
* 参  数: bm      - 内存管理结构体指针        (out)
*         polygon - 归一化的多边形结构体指针  (in)  
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_polygon_bits(IVS_SYS_BUF_MANAGER *bm, VCA_POLYGON_F *polygon)
{
	UINT32_T i;
	WORD fixed_value;
	UINT32_T code;
	
	// 输出多边形边界点
	for (i = 0; i < polygon->vertex_num; i++)
	{	
		// x
		fixed_value = (WORD)(polygon->point[i].x * CONST_15_BITS_OPERATOR);
		code = (MARKER_BIT << 15) | fixed_value;
		EMIT_BITS(code, 16);
		
		// y
		fixed_value = (WORD)(polygon->point[i].y * CONST_15_BITS_OPERATOR);
		code = (MARKER_BIT << 15) | fixed_value;
		EMIT_BITS(code, 16);
	}

}

/******************************************************************************
* 功  能: 输出归一化后的边界框结构体数据
* 参  数: bm    - 内存管理结构体参数(out)
*         rect  - 归一化后的边界框结构体(in)
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
static void emit_rect_normalize_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RECT_F *rect)
{
	WORD fixed_value;
	DWORD code;
	
	/* 表示目标的外接矩形框的左上角的X轴位置与视频图像宽度的比例(X*1.0)/0x7fff，
	* 值域为0x0000-0x7fff。*/
    fixed_value = (WORD)(rect->x * CONST_15_BITS_OPERATOR);
	code = (MARKER_BIT << 15) | fixed_value;
    EMIT_BITS(code, 16);
	
	/* 表示目标的外接矩形框的左上角的Y轴位置与视频图像宽度的比例(Y*1.0)/0x7fff，
	* 值域为0x0000-0x7fff。*/
	fixed_value = (WORD)(rect->y * CONST_15_BITS_OPERATOR);
	code = (MARKER_BIT << 15) | fixed_value;
    EMIT_BITS(code, 16);
	
	/*表示目标外接矩形宽的宽度与视频图像宽度的比例(Width*1.0)/0x7fff，
	值域为0x0000-0x7fff。*/
    fixed_value = (WORD)(rect->width * CONST_15_BITS_OPERATOR);
	code = (MARKER_BIT << 15) | fixed_value;
    EMIT_BITS(code, 16);
	
	/*表示目标外接矩形宽的高度与视频图像高度的比例(Height*1.0)/0x7fff，
	值域为0x0000-0x7fff。*/
    fixed_value = (WORD)(rect->height * CONST_15_BITS_OPERATOR);
	code = (MARKER_BIT << 15) | fixed_value;
    EMIT_BITS(code, 16);
 
}

/******************************************************************************
* 功  能: 发送规则参数
* 参  数: bm         -  内存管理结构体指针 (out)
*         rule_sets  -  规则参数联合(union)结构体(in)
*         event_type -  事件类型(in)        
*
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_rule_param_sets_bits(IVS_SYS_BUF_MANAGER *bm, 
								      VCA_RULE_PARAM_SETS     *rule_sets, 
								      UINT32_T             event_type)
{
	UINT32_T code;
	WORD param_num, speed, density, delay;

    switch(event_type)
	{
	case VCA_TRAVERSE_PLANE:
		param_num = 1;
		EMIT_BITS(param_num, 3);

		code = (MARKER_BIT << 15) | rule_sets->traverse_plane.cross_direction; //FIXME:

		EMIT_BITS(code, 16);
		
		break;
		
    case VCA_INTRUSION: 
		param_num = 1;
		EMIT_BITS(param_num, 3);

        code = (MARKER_BIT << 15) | (rule_sets->intrusion.delay & CONST_15_BITS_MASK);
		
		EMIT_BITS(code, 16);
		
		break;
		
    case VCA_LOITER:  
		param_num = 1;
		EMIT_BITS(param_num, 3);
		
        code = (MARKER_BIT << 15) | (rule_sets->loiter.delay & CONST_15_BITS_MASK);
		
		EMIT_BITS(code, 16);
		
		break;
				
	case VCA_LEFT_TAKE:
	case VCA_LEFT:
	case VCA_TAKE:
        param_num = 1;
		EMIT_BITS(param_num, 3);
		
        code = (MARKER_BIT << 15) | (rule_sets->left_take.delay & CONST_15_BITS_MASK);
		
		EMIT_BITS(code, 16);
		
		break;
		
	case VCA_PARKING:
		param_num = 1;
		EMIT_BITS(param_num, 3);
		
        code = (MARKER_BIT << 15) | (rule_sets->parking.delay & CONST_15_BITS_MASK);
		
		EMIT_BITS(code, 16);
		break;

	case VCA_HIGH_DENSITY:
		param_num = 2;
		EMIT_BITS(param_num, 3);
		
		density = (WORD)(rule_sets->high_density.alert_density * CONST_15_BITS_OPERATOR) ;		
        code = (MARKER_BIT << 15) | density;
		
		EMIT_BITS(code, 16);

		delay = (WORD)(rule_sets->high_density.delay);
		code = (MARKER_BIT << 15) | delay;
		EMIT_BITS(code, 16);
		
		break;

    case VCA_STICK_UP:
		param_num = 1;
		EMIT_BITS(param_num, 3);
		
		delay = (WORD)(rule_sets->stick_up.delay);		
        code = (MARKER_BIT << 15) | delay;		
		EMIT_BITS(code, 16);

		break;

	case VCA_INSTALL_SCANNER:
		param_num = 1;
		EMIT_BITS(param_num, 3);
		
		delay = (WORD)(rule_sets->insert_scanner.delay);		
        code = (MARKER_BIT << 15) | delay;		
		EMIT_BITS(code, 16);

		break;
	case VCA_OVER_TIME:
		param_num = 1;
		EMIT_BITS(param_num, 3);
		
		delay = (WORD)(rule_sets->over_time.delay);	
        code = (MARKER_BIT << 15) | delay;		
		EMIT_BITS(code, 16);
		
		break;		
	// version 0x0202 add
	case VCA_ABNORMAL_FACE:
		param_num = 2;
		EMIT_BITS(param_num, 3);

		delay = (WORD)(rule_sets->abnormal_face.delay);	
        code = (MARKER_BIT << 15) | delay;
		EMIT_BITS(code, 16);

		code = (MARKER_BIT << 15) | (WORD)(rule_sets->abnormal_face.mode);	
		EMIT_BITS(code, 16);
		break;
	// version 0x0202 modify
	case VCA_RUN:
		param_num = 3;
		EMIT_BITS(param_num, 3);
		
		speed = (WORD)(rule_sets->run.speed * CONST_15_BITS_OPERATOR);	
        code = (MARKER_BIT << 15) | speed;		
		EMIT_BITS(code, 16);

		delay = (WORD)(rule_sets->run.delay);	
        code = (MARKER_BIT << 15) | delay;	
		EMIT_BITS(code, 16);

        code = (MARKER_BIT << 15) | rule_sets->run.mode;
		EMIT_BITS(code, 16);
		
		break;	
	// 0x0202 add
	case VCA_VIOLENT_MOTION:
		param_num = 1;
		EMIT_BITS(param_num, 3);
		
        code = (MARKER_BIT << 15) | (WORD)(rule_sets->violent.delay) ;
		EMIT_BITS(code, 16);
		break;
	// 0x0202 add
	case VCA_FLOW_COUNTER:
		param_num = 4;
		EMIT_BITS(param_num, 3);

		speed = (WORD)(rule_sets->flow_counter.direction.start_point.x * CONST_15_BITS_OPERATOR);	
        code = (MARKER_BIT << 15) | speed;		
		EMIT_BITS(code, 16);

		speed = (WORD)(rule_sets->flow_counter.direction.start_point.y * CONST_15_BITS_OPERATOR);	
        code = (MARKER_BIT << 15) | speed;		
		EMIT_BITS(code, 16);

		speed = (WORD)(rule_sets->flow_counter.direction.end_point.x * CONST_15_BITS_OPERATOR);	
        code = (MARKER_BIT << 15) | speed;		
		EMIT_BITS(code, 16);

		speed = (WORD)(rule_sets->flow_counter.direction.end_point.y * CONST_15_BITS_OPERATOR);	
        code = (MARKER_BIT << 15) | speed;		
		EMIT_BITS(code, 16);
		break;
	// 0x0202 add
	case VCA_LEAVE_POSITION:
		param_num = 2;
		EMIT_BITS(param_num, 3);

        code = (MARKER_BIT << 15) | (WORD)(rule_sets->leave_post.leave_delay);	
		EMIT_BITS(code, 16);

		code = (MARKER_BIT << 15) | (WORD)(rule_sets->leave_post.static_delay);	
		EMIT_BITS(code, 16);
		break;
	default:
		param_num = 0;
        EMIT_BITS(param_num, 3);
	}
}

/******************************************************************************
* 功  能: 发送简化的规则信息，包含规则的基本信息
* 参  数: bm        - 内存管理结构体指针        (out)
*         rule_info - 简化规则信息结构体指针    (in) 
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
static INT32_T emit_rule_info_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RULE_INFO *rule)
{
	UINT32_T temp, shift_bits_num;

	// RuleType
	EMIT_BITS(rule->rule_type & 0x3, 2);
	
	// EventType事件信息类型
	temp = rule->event_type;
	
	shift_bits_num = 0;
	
	while (temp)
    {
		shift_bits_num++;  /* Find the left shift number of bits of '1' */
		
		temp >>= 1;
	}
	
	EMIT_BITS(shift_bits_num, 6);
	
	//PointNum
	EMIT_BITS(rule->polygon.vertex_num, 5);
	
	emit_rule_param_sets_bits(bm, &(rule->param), rule->event_type);
	
	// Polygon Point
	emit_polygon_bits(bm, &(rule->polygon));	
	
	// 触发事件的规则序号 RuleID
	EMIT_BITS(rule->ID, 8);

	EMIT_BITS(rule->reserved[0], 8);
	EMIT_BITS(rule->reserved[1], 8);
	EMIT_BITS(rule->reserved[2], 8);
	EMIT_BITS(rule->reserved[3], 8);
	EMIT_BITS(rule->reserved[4], 8);
	EMIT_BITS(rule->reserved[5], 8);
	
	//Counter
//	ret = emit_counter_bits(bm, &(rule->counter));
//  CHECK_ERROR(ret != HIK_IVS_SYS_LIB_S_OK, ret);

	return HIK_IVS_SYS_LIB_S_OK;
}

/******************************************************************************
* 功  能: 发送简化目标结构体信息
* 参  数: bm     - 内存管理结构体指针        (out)
*         target - 简化的目标信息结构体指针  (in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_alert_target_bits(IVS_SYS_BUF_MANAGER *bm, VCA_TARGET *target)
{
	UINT32_T code;

	EMIT_BITS((target->ID >> 24), 8);
    // ID 当前目标的生成序号
	code = (MARKER_BIT << 15) | (target->ID & 0xffffff);
    EMIT_BITS(code, 16);
	
	// Type 目标的类型
	EMIT_BITS(target->type, 8);
	
	//RectNormalize
    emit_rect_normalize_bits(bm, &target->rect);

}

/******************************************************************************
* 功  能: 输出报警事件结构体数据
* 参  数: p_event_data - 报警事件结构体指针 (in)
*         param        - 处理参数结构体指针 (out)         
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_EVENT_DATA_to_system(VCA_ALERT          *p_event_data, 
								 IVS_SYS_PROC_PARAM *param)
{
    IVS_SYS_BUF_MANAGER   buf_manager;
    IVS_SYS_BUF_MANAGER  *bm;
	INT32_T  ret;
	BYTE padding_flag;

    CHECK_ERROR(param == NULL || p_event_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL,                     HIK_IVS_SYS_LIB_E_PARA_NULL);

    bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);

	// 没有报警信息，则无需往码流中加入智能信息
	if (p_event_data->alert == 1)
	{
		EMIT_BITS(0xffff, 16);
	    EMIT_BITS(IVS_PACK_VERSION, 16);
 
		// 表示当前输入视频的信号状态
		EMIT_BITS(p_event_data->view_state, 3);

		//PaddingFlag先设成'0', 后面重新设置
		//PaddingFlag = 0 ExtendByteNum =0 
		EMIT_BITS(0, 5); 

		// RuleInfo
		ret = emit_rule_info_bits(bm, &(p_event_data->rule_info));
		CHECK_ERROR(ret != HIK_IVS_SYS_LIB_S_OK, ret);

		// Alert Target
		emit_alert_target_bits(bm, &(p_event_data->target));
		
		padding_flag = emit_padding_byte(bm);
		
		// 重新设置PaddingFlag
		bm->buf_start[4] |= padding_flag << 4 ;
	}

	//计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);
	
	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER); //FIXME:
	
    return HIK_IVS_SYS_LIB_S_OK;

}

/******************************************************************************
* 功  能: 输出警戒规则结构体数据
* 参  数: bm   - 内存管理结构体指针    (out)
*         rule - 警戒规则数组结构体指针(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_rule_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RULE *rule)
{
	UINT32_T temp, shift_bits_num;
		 
    // RuleType
	EMIT_BITS(rule->rule_type & 0x3, 2);

	// EventType事件信息类型
	temp = rule->event_type;
	
	shift_bits_num = 0;	
	while (temp)
    {
		shift_bits_num++;  /* Find the left shift number of bits of '1' */
		temp >>= 1;
	}
	
	EMIT_BITS(shift_bits_num, 6);

    //PointNum
	EMIT_BITS(rule->polygon.vertex_num, 5);
	
	emit_rule_param_sets_bits(bm, &(rule->rule_param.param), rule->event_type);
	
	// 触发事件的规则序号 RuleID
	EMIT_BITS(rule->ID, 8);
	
	// Polygon Point
	emit_polygon_bits(bm, &(rule->polygon));		
}

/******************************************************************************
* 功  能: 输出警戒规则数组结构体数据
* 参  数: p_rule_data - 警戒规则数组结构体指针(in)
*         param       - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_RULE_DATA_to_system(VCA_RULE_LIST       *p_rule_data,
								IVS_SYS_PROC_PARAM  *param)
{
	UINT32_T i, rule_num=0; 
	BYTE padding_flag;
    IVS_SYS_BUF_MANAGER   buf_manager;
    IVS_SYS_BUF_MANAGER  *bm;

	CHECK_ERROR(param == NULL || p_rule_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
	
    bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);

    EMIT_BITS(0xffff, 16);
	EMIT_BITS(IVS_PACK_VERSION, 16);

    /* 表示HIK_RULE_LIST中包含的规则数量，规则数量代表智能分析算法设置成功的规则数量，
	 * 每条规则将触发特定的事件信息。*/ 
	EMIT_BITS(p_rule_data->rule_num, 8);

	//PaddingFlag先设成'0', 后面重新设置
	//PaddingFlag = 0 ExtendByteNum =0 
	EMIT_BITS(0, 8); 

	for (i = 0; i < MAX_RULE_NUM; i++)
	{
		if (p_rule_data->rule[i].enable == TRUE)
		{
			emit_rule_bits(bm, &p_rule_data->rule[i]);
			rule_num ++;
		}

		if (rule_num == p_rule_data->rule_num)
		{
			break;
		}
	}
	
	padding_flag = emit_padding_byte(bm);
	
	//重新设置PaddingFlag
	bm->buf_start[5] |= padding_flag << 7;
	
	//计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);
	
	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER);
	
    return HIK_IVS_SYS_LIB_S_OK;
}

/******************************************************************************
* 功  能: 封装人脸识别数据
* 参  数: face_ident  - 人脸识别数据结构体指针(in)
*         param       - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注: 该接口已不再使用
*******************************************************************************/
HRESULT IVS_FACE_IDENTIFICATION_to_system(HIK_FACE_IDENTIFICATION *face_idt, 
										  IVS_SYS_PROC_PARAM      *param)
{
	UINT32_T i;
	BYTE padding_flag;
	PBYTE p_data;
    IVS_SYS_BUF_MANAGER   buf_manager;
    IVS_SYS_BUF_MANAGER  *bm;

	CHECK_ERROR(param == NULL || face_idt  == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(face_idt->encrypt_data_buf == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf                 == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
	
    bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);

	//DataLen，32bits，先设成'0', 后面重新设置 // 插1bit解决0x000001冲突
	EMIT_BITS(0, 32);

	//PaddingFlag先设成'0', 后面重新设置
	//PaddingFlag = 0 ExtendByteNum = 0 
	EMIT_BITS(0, 8); 

	//CardSerialNumber，(HIK_CARD_SERIAL_NUM_LEN * 8) bits
	for (i = 0; i < HIK_CARD_SERIAL_NUM_LEN; i++)
	{
	    EMIT_BITS(face_idt->card_serial_num[i], 8);	
	}	
	
	p_data = (PBYTE)face_idt->encrypt_data_buf;

	for (i = 0; i < face_idt->data_len; i++)  //打包人脸识别加密数据
	{
	    EMIT_BITS(p_data[i], 8);	
	}

	padding_flag = emit_padding_byte(bm);

	//重新设置PaddingFlag
	bm->buf_start[4] |= padding_flag << 7;

	//计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);

#ifdef  HIK_WORDS_BIGENDIAN        //大端模式
	bm->buf_start[0] = (BYTE)(param->len);
	bm->buf_start[1] = (BYTE)(param->len >> 8);
	bm->buf_start[2] = (BYTE)(param->len >> 16);
	bm->buf_start[3] = (BYTE)(param->len >> 24);
#else
	bm->buf_start[0] = (BYTE)(param->len >> 24);
	bm->buf_start[1] = (BYTE)(param->len >> 16);
	bm->buf_start[2] = (BYTE)(param->len >> 8);
	bm->buf_start[3] = (BYTE)(param->len);
#endif

	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER);
	
    return HIK_IVS_SYS_LIB_S_OK;
}

/******************************************************************************
* 功  能: 输出事件信息结构体数据
* 参  数: bm         - 内存管理结构体指针 (out)
*         p_evt_info - 事件信息结构体指针 (in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_event_bits(IVS_SYS_BUF_MANAGER  *bm, VCA_EVT_INFO *p_evt_info)
{
	UINT32_T event_type, code;
//	UINT32_T param_num;
	WORD density;

	EMIT_BITS(p_evt_info->rule_id , 8);
	EMIT_BITS((p_evt_info->event_type >> 16) & 0xffff, 16);
	EMIT_BITS(p_evt_info->event_type & 0xffff, 16);

	event_type = p_evt_info->event_type;

    switch(event_type)
	{
	case VCA_HUMAN_ENTER:
		
// 		param_num = 0;
//      EMIT_BITS(param_num, 8);
		EMIT_BITS(p_evt_info->event_value.human_in, 8);
		break;
	case VCA_HIGH_DENSITY:

// 		param_num = 1;
// 		EMIT_BITS(param_num, 8);
		
		density = (WORD)(p_evt_info->event_value.crowd_density * CONST_15_BITS_OPERATOR) ;
		
        code = (MARKER_BIT << 15) | density;
		
		EMIT_BITS(code, 16);
		break;
	case VCA_VIOLENT_MOTION:       //TODO: check which one is used, motion_entropy or abrun_info

// 		param_num = 1;
// 		EMIT_BITS(param_num, 8);
		
		//density = (WORD)(p_evt_info->event_value.motion_entropy * CONST_15_BITS_OPERATOR) ;
		//
  //      code = (MARKER_BIT << 15) | density;
		//
		//EMIT_BITS(code, 16);

        EMIT_BITS((MARKER_BIT << 15) | ((WORD)(p_evt_info->event_value.abrun_info[0] * CONST_15_BITS_OPERATOR)), 16);
 		
        EMIT_BITS((MARKER_BIT << 15) | ((WORD)(p_evt_info->event_value.abrun_info[1] * CONST_15_BITS_OPERATOR)), 16);
 		
 		EMIT_BITS((MARKER_BIT << 15) | ((WORD)(p_evt_info->event_value.abrun_info[2] * CONST_15_BITS_OPERATOR)), 16);
 		
 		EMIT_BITS((MARKER_BIT << 15) | ((WORD)(p_evt_info->event_value.abrun_info[3] * CONST_15_BITS_OPERATOR)), 16);
		break;

	case VCA_FLOW_COUNTER:

//  	param_num = 2;
//  	EMIT_BITS(param_num, 8);

 	    EMIT_BITS((p_evt_info->event_value.pdc_counter.enter_num >> 16) & 0xffff, 16);
 		EMIT_BITS(p_evt_info->event_value.pdc_counter.enter_num & 0xffff, 16);
 
 	    EMIT_BITS((p_evt_info->event_value.pdc_counter.leave_num >> 16) & 0xffff, 16);
 		EMIT_BITS(p_evt_info->event_value.pdc_counter.leave_num & 0xffff, 16);
		break;

	case VCA_RUN:
		
 		EMIT_BITS((MARKER_BIT << 15) | ((WORD)(p_evt_info->event_value.abrun_info[0] * CONST_15_BITS_OPERATOR)), 16);
 		
        EMIT_BITS((MARKER_BIT << 15) | ((WORD)(p_evt_info->event_value.abrun_info[1] * CONST_15_BITS_OPERATOR)), 16);
 
 		EMIT_BITS((MARKER_BIT << 15) | ((WORD)(p_evt_info->event_value.abrun_info[2] * CONST_15_BITS_OPERATOR)), 16);
 
 		EMIT_BITS((MARKER_BIT << 15) | ((WORD)(p_evt_info->event_value.abrun_info[3] * CONST_15_BITS_OPERATOR)), 16);
		break;
		// V2.3 add by chenjie 121211
	case VCA_AUDIO_ABNORMAL:
		EMIT_BITS((MARKER_BIT << 15) | (WORD)p_evt_info->event_value.audio_entropy, 16);
//		EMIT_BITS(p_evt_info->event_value.audio_entropy, 16);
		break;
	default:
// 		param_num = 0;
// 		EMIT_BITS(param_num, 8);
		break;
	}
}

/******************************************************************************
* 功  能: 封装事件信息列表
* 参  数: p_evt_list  - 事件信息列表结构体指针(in)
*         param       - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_EVENT_LIST_to_system(VCA_EVT_INFO_LIST      *p_evt_list,
							    IVS_SYS_PROC_PARAM *param)
{
	UINT32_T i, event_num=0; 
	BYTE padding_flag;
    IVS_SYS_BUF_MANAGER   buf_manager;
    IVS_SYS_BUF_MANAGER  *bm;
	UINT32_T enable = 0;

	CHECK_ERROR(param == NULL || p_evt_list == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
	
    bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);

	EMIT_BITS(0xffff, 16);
	EMIT_BITS(IVS_PACK_VERSION, 16);

	if (p_evt_list->event_num == 0)
	{
		param->len = 0;
		return HIK_IVS_SYS_LIB_S_OK;
	}

    /* 表示VCA_EVT_INFO_LIST中包含的事件数量 */ 
    EMIT_BITS(p_evt_list->event_num, 8);

    //PaddingFlag先设成'0', 后面重新设置
    //PaddingFlag = 0 ExtendByteNum1 = 0, ExtendByteNum2 =0, 
    //ExtendByteNum1(4bits),ExtendByteNum2(3bits)
    EMIT_BITS(0, 8);  

	EMIT_BITS(0, 8); //enable,对应8个事件的enable，每个1bit，暂时填0后面修改

	for (i = 0; i < MAX_RULE_NUM; i++)
	{
		if (p_evt_list->event_info[i].enable == TRUE)
		{
			emit_event_bits(bm, &p_evt_list->event_info[i]);
			event_num++;
			enable |= 1 << i;
		} 
	}
    
	//码流出错
	CHECK_ERROR(event_num != p_evt_list->event_num, HIK_IVS_SYS_LIB_S_FAIL);
	
	padding_flag = emit_padding_byte(bm);
	
	//重新设置PaddingFlag
	bm->buf_start[5] |= padding_flag << 7;

	//重新设置enable
	bm->buf_start[6] = enable & 0xff;
	
	//计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);
	
	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER); //FIXME: 
	
    return HIK_IVS_SYS_LIB_S_OK;
}

/******************************************************************************
* 功  能: 输出目标尺寸过滤器结构体数据
* 参  数: bm         - 内存管理结构体指针 (out)
*         p_filter   - 输出目标尺寸过滤器结构体指针 (in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_size_filter(IVS_SYS_BUF_MANAGER  *bm, VCA_SIZE_FILTER *p_filter)
{
	EMIT_BITS((BYTE)(p_filter->mode), 8);
    emit_rect_normalize_bits(bm, &(p_filter->min_rect));
    emit_rect_normalize_bits(bm, &(p_filter->max_rect));

}

/******************************************************************************
* 功  能: 封装异常人脸检测规则参数
* 参  数: p_face_rule   - 人脸检测规则结构体指针(in)
*         param         - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注: 该接口不再使用
*******************************************************************************/
HRESULT IVS_FACE_DETECT_RULE_to_system(FACE_DETECT_RULE   *p_face_rule,
							           IVS_SYS_PROC_PARAM *param)
{
	BYTE padding_flag;
    IVS_SYS_BUF_MANAGER   buf_manager;
    IVS_SYS_BUF_MANAGER  *bm;
	UINT32_T code;
	VCA_RULE_PARAM *p_param;
	
	CHECK_ERROR(param == NULL || p_face_rule == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
	
    if (p_face_rule->enable == FALSE)
	{
		param->len = 0;
		return HIK_IVS_SYS_LIB_S_OK;
	}
	
	bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);
	
	//PaddingFlag先设成'0', 后面重新设置
	//PaddingFlag = 0 ExtendByteNum1 = 0, ExtendByteNum2 =0, 
	//ExtendByteNum1(4bits),ExtendByteNum2(3bits)
	EMIT_BITS(0, 8);
	
    p_param = &p_face_rule->rule_param;
	EMIT_BITS(p_param->sensitivity & 0xff, 8);

	code = p_param->param.abnormal_face.delay;
	EMIT_BITS((code >> 16) & 0xffff, 16);
	EMIT_BITS(code & 0xffff, 16);

	EMIT_BITS((BYTE)(p_param->size_filter.enable), 8);
	if (p_param->size_filter.enable)
	{
		emit_size_filter(bm, &(p_param->size_filter));
	}
	EMIT_BITS(p_face_rule->polygon.vertex_num, 8);
    emit_polygon_bits(bm, &(p_face_rule->polygon));
	
	padding_flag = emit_padding_byte(bm);
	
	//重新设置PaddingFlag
	bm->buf_start[0] |= padding_flag << 7;
	
	//计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);
	
	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER);
	
    return HIK_IVS_SYS_LIB_S_OK;
}

// 
HRESULT IVS_RULE_DATA_to_systemEx(HIK_IVS_DATA_INOUT_PACKET* pstdata, IVS_SYS_PROC_PARAM *param)
{
	UINT32_T i, rule_num=0; 
	BYTE padding_flag;
	IVS_SYS_BUF_MANAGER   buf_manager;
	IVS_SYS_BUF_MANAGER  *bm;
	VCA_RULE_LIST* p_rule_data_1 = NULL;
	VCA_RULE_LIST_EX* p_rule_data_2 = NULL;
	VCA_RULE_LIST_V3* p_rule_data_3 = NULL;
	INT16_T version = 0;
	INT16_T stnum   = 0;
	INT16_T count   = 0;
	INT16_T type    = 0;
	INT16_T datalen = 0;

	CHECK_ERROR(param == NULL || pstdata == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL ||pstdata->pdata == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);

	stnum = pstdata->num;
	switch(stnum)
	{
	case 8:
		{
			p_rule_data_1 = (VCA_RULE_LIST*)pstdata->pdata;
			count = 8;
			break;
		}
	case 16:
		{
			p_rule_data_2 = (VCA_RULE_LIST_EX*)pstdata->pdata;
			count = 16;
			break;
		}
	case 64:
		{
			p_rule_data_3 = (VCA_RULE_LIST_V3*)pstdata->pdata;
			count = 64;
			break;
		}
	}

	version = pstdata->version;
	stnum   = pstdata->num;
	type    = pstdata->type;
	datalen = pstdata->datalen;

	bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);

	EMIT_BITS(version, 16);
	EMIT_BITS(stnum, 16);
	EMIT_BITS(type, 16);
	EMIT_BITS(datalen, 16);

	EMIT_BITS(0xffff, 16);
	EMIT_BITS(IVS_PACK_VERSION, 16);

	/* 表示HIK_RULE_LIST中包含的规则数量，规则数量代表智能分析算法设置成功的规则数量，
	* 每条规则将触发特定的事件信息。*/ 
	switch(stnum)
	{
	case 8:
		{
			// 高8位表示算法库类型，低24表示原数量
			EMIT_BITS((p_rule_data_1->rule_num >> 24), 8);
			EMIT_BITS((p_rule_data_1->rule_num & 0xffffff), 8);
			break;
		}
	case 16:
		{
			EMIT_BITS((p_rule_data_2->rule_num >> 24), 8);
			EMIT_BITS((p_rule_data_2->rule_num & 0xffffff), 8);
			break;
		}
	case 64:
		{
			EMIT_BITS((p_rule_data_3->rule_num >> 24), 8);
			EMIT_BITS((p_rule_data_3->rule_num & 0xffffff), 8);
			break;
		}
	}

	//PaddingFlag先设成'0', 后面重新设置
	//PaddingFlag = 0 ExtendByteNum =0 
	EMIT_BITS(0, 8); 

	switch(count)
	{
	case 8:
		{
			for (i = 0; i < count; i++)
			{
				if (p_rule_data_1->rule[i].enable == TRUE)
				{
					emit_rule_bits(bm, &p_rule_data_1->rule[i]);
					rule_num ++;
				}

				if (rule_num == (p_rule_data_1->rule_num & 0xffffff)) // 高8位表示算法库类型
				{
					break;
				}
			}
			break;
		}
	case 16:
		{
			for (i = 0; i < count; i++)
			{
				if (p_rule_data_2->rule[i].enable == TRUE)
				{
					emit_rule_bits(bm, &p_rule_data_2->rule[i]);
					rule_num ++;
				}     

				if (rule_num == (p_rule_data_2->rule_num & 0xffffff)) // 同上
				{
					break;
				}
			}
			break;
		}
	case 64:
		{
			for (i = 0; i < count; i++)
			{
				if (p_rule_data_3->rule[i].enable == TRUE)
				{
					emit_rule_bits(bm, &p_rule_data_3->rule[i]);
					rule_num ++;
				}     

				if (rule_num == (p_rule_data_3->rule_num & 0xffffff)) // 同上
				{
					break;
				}
			}
			break;
		}
	default:
		{
			return HIK_IVS_SYS_LIB_S_FAIL;
		}
	}

	padding_flag = emit_padding_byte(bm);

	//重新设置PaddingFlag
	bm->buf_start[9] |= padding_flag << 7;

	//计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);

	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER);

	return HIK_IVS_SYS_LIB_S_OK;
}

HRESULT IVS_EVENT_LIST_to_systemEx(HIK_IVS_DATA_INOUT_PACKET* pstdata, IVS_SYS_PROC_PARAM *param)
{
	UINT32_T i, event_num=0; 
	UINT32_T j = 0;
	BYTE padding_flag;
	IVS_SYS_BUF_MANAGER   buf_manager;
	IVS_SYS_BUF_MANAGER  *bm;
	UINT32_T enable   = 0;
	VCA_EVT_INFO_LIST* p_evt_list_1 = NULL;
	VCA_EVT_INFO_LIST_EX* p_evt_list_2 = NULL;
	VCA_EVT_INFO_LIST_V3* p_evt_list_3 = NULL;
	INT16_T version = 0;
	INT16_T stnum   = 0;
	INT16_T count   = 0;
	INT16_T type    = 0;
	INT16_T datalen = 0;


	CHECK_ERROR(param == NULL  || pstdata == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL || pstdata->pdata == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);

	stnum = pstdata->num;
	switch(stnum)
	{
	case 8:
		{
			p_evt_list_1 = (VCA_EVT_INFO_LIST*)pstdata->pdata;
			count = 8;
			break;
		}
	case 16:
		{
			p_evt_list_2 = (VCA_EVT_INFO_LIST_EX*)pstdata->pdata;
			count = 16;
			break;
		}
	case 64:
		{
			p_evt_list_3 = (VCA_EVT_INFO_LIST_V3*)pstdata->pdata;
			count = 64;
			break;
		}
	}

	version = pstdata->version;
	stnum   = pstdata->num;
	type    = pstdata->type;
	datalen = pstdata->datalen;

	bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);

	EMIT_BITS(version, 16);
	EMIT_BITS(stnum, 16);
	EMIT_BITS(type, 16);
	EMIT_BITS(datalen, 16);

	EMIT_BITS(0xffff, 16);
	EMIT_BITS(IVS_PACK_VERSION, 16);

	/* 表示VCA_EVT_INFO_LIST中包含的事件数量 */ 
	switch(stnum)
	{
	case 8:
		{
			EMIT_BITS(p_evt_list_1->event_num, 8);
			break;
		}
	case 16:
		{
			EMIT_BITS(p_evt_list_2->event_num, 8);
			break;
		}
	case 64:
		{
			EMIT_BITS(p_evt_list_3->event_num, 8);
			break;
		}
	}

	//PaddingFlag先设成'0', 后面重新设置
	//PaddingFlag = 0 ExtendByteNum1 = 0, ExtendByteNum2 =0, 
	//ExtendByteNum1(4bits),ExtendByteNum2(3bits)
	EMIT_BITS(0, 8);  

	EMIT_BITS(0, 16); //enable,对应64个事件的enable，每个1bit，暂时填0后面修改
	EMIT_BITS(0, 16); //enable,对应64个事件的enable，每个1bit，暂时填0后面修改
	EMIT_BITS(0, 16); //enable,对应64个事件的enable，每个1bit，暂时填0后面修改
	EMIT_BITS(0, 16); //enable,对应64个事件的enable，每个1bit，暂时填0后面修改

	switch(count)
	{
	case 8:
		{
			for (i = 0; i < count; i++)
			{
				if (p_evt_list_1->event_info[i].enable == TRUE)
				{
					emit_event_bits(bm, &p_evt_list_1->event_info[i]);
					event_num++;
					enable |= 1 << i;
				} 
			}

			bm->buf_start[14] = enable & 0xff;
			//码流出错
			CHECK_ERROR(event_num != (p_evt_list_1->event_num & 0xffffff), HIK_IVS_SYS_LIB_S_FAIL);

			break;
		}
	case 16:
		{
			for (i = 0; i < count; i++)
			{
				if (p_evt_list_2->event_info[i].enable == TRUE)
				{
					emit_event_bits(bm, &p_evt_list_2->event_info[i]);
					event_num++;
					if (i == 8)
					{
						bm->buf_start[14] = enable & 0xff;
						enable = 0;
					}
					enable |= 1 << i;
				} 
			}

			bm->buf_start[15] = enable & 0xff;
			//码流出错
			CHECK_ERROR(event_num != (p_evt_list_2->event_num & 0xffffff), HIK_IVS_SYS_LIB_S_FAIL);

			break;
		}
	case 64:
		{
			for (i = 0; i < count; i++)
			{
				if (p_evt_list_3->event_info[i].enable == TRUE)
				{
					emit_event_bits(bm, &p_evt_list_3->event_info[i]);
					event_num++;

					if ((i % 8 == 0) && (i >= 8))
					{
						j = (i / 8);
						bm->buf_start[14 + j - 1] = enable & 0xff;
						enable = 0;
					}

					enable |= 1 << (i - (j * 8));
				}
			}

			bm->buf_start[21] = enable & 0xff;
			//码流出错
			CHECK_ERROR(event_num != (p_evt_list_3->event_num & 0xffffff), HIK_IVS_SYS_LIB_S_FAIL);

			break;
		}
	default:
		{
			return HIK_IVS_SYS_LIB_S_FAIL;
		}
	}

	padding_flag = emit_padding_byte(bm);

	//重新设置PaddingFlag
	bm->buf_start[13] |= padding_flag << 7;

	//重新设置enable
	//bm->buf_start[14] = enable & 0xff;

	//计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);

	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER); //FIXME: 

	return HIK_IVS_SYS_LIB_S_OK;
}

//打包新接口
HRESULT IVS_DATA_to_system(unsigned char *pstdata, IVS_SYS_PROC_PARAM *param)
{
	HIK_IVS_DATA_INOUT_PACKET* stParam = NULL;

	CHECK_ERROR(param == NULL || pstdata == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL,                HIK_IVS_SYS_LIB_E_PARA_NULL);

	stParam = (HIK_IVS_DATA_INOUT_PACKET*)pstdata;

	switch(stParam->type)
	{
	case IVS_TYPE_META_DATA:
		{
			return IVS_META_DATA_to_system((VCA_TARGET_LIST*)stParam->pdata, param);
		}
	case IVS_TYPE_EVENT_DATA:
		{
			return IVS_EVENT_DATA_to_system((VCA_ALERT*)stParam->pdata, param);
		}
	case IVS_TYPE_RULE_DATA:
		{
			return IVS_RULE_DATA_to_systemEx(stParam, param);
		}
	case IVS_TYPE_EVENT_LIST:
		{
			return IVS_EVENT_LIST_to_systemEx(stParam, param);
		}
	case IVS_TYPE_FACE_IDENTIFICATION:
		{
			return HIK_IVS_SYS_LIB_S_FAIL;
			//return IVS_FACE_IDENTIFICATION_to_system((HIK_FACE_IDENTIFICATION*)stParam.pdata, param);
		}
	case IVS_FACE_DETECT_RULE:
		{
			return HIK_IVS_SYS_LIB_S_FAIL;
			//return IVS_FACE_DETECT_RULE_to_system((FACE_DETECT_RULE*)stParam.pdata, param);
		}
	default:
		{
			break; 
		}
	}

	return HIK_IVS_SYS_LIB_S_FAIL;
}  

/******************************************************************************
* 功  能: 输出帧内目标信息数组(Meta Data)数据
* 参  数: p_meta_data - 目标信息结构体(in)
*         param       - 处理参数结构体 (out)
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_META_DATA_to_system_v2(VCA_TARGET_LIST_V2    *p_meta_data, 
								IVS_SYS_PROC_PARAM *param)
{   
	IVS_SYS_BUF_MANAGER   buf_manager;
	IVS_SYS_BUF_MANAGER  *bm;	
	INT32_T i;
	BYTE padding_flag;
	UINT32_T code;

	CHECK_ERROR(param == NULL || p_meta_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->scale_width > CONST_13_BITS_OPERATOR 
		|| param->scale_height > CONST_13_BITS_OPERATOR, HIK_IVS_SYS_LIB_E_SCALE_WH);

	bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);

	//版本号标示
	EMIT_BITS(0xffff, 16);
	EMIT_BITS(IVS_EXTEND_TARGET_NUM_MARK, 16);

	//当前帧视频图像智能算法检测到的目标数量
	EMIT_BITS(p_meta_data->target_num, 8);

	//PaddingFlag先设成'0', 后面重新设置
	//PaddingFlag = 0 ExtendByteNum_0 =0  ExtendByteNum_1 = 1  
	EMIT_BITS(1, 8);

	//宽度方向上的缩放比例 
	code = (MARKER_BIT << 15) | bm->scale_width;
	EMIT_BITS(code, 16);

	//高度方向上的缩放比例 
	code = (MARKER_BIT << 15) | bm->scale_height;
	EMIT_BITS(code, 16);

	for (i = 0; i < p_meta_data->target_num; i++)
	{
		emit_target_bits(bm, &(p_meta_data->target[i]), IVS_EXTEND_TARGET_NUM_MARK);
	}

	padding_flag = emit_padding_byte(bm);

	//重新设置PaddingFlag
	bm->buf_start[5] |= padding_flag << 7 ;

	//计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);

	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER); //FIXME:

	return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 打包任意个目标的长度数据
* 参  数: p_meta_data - 目标信息结构体(in)
*         param       - 处理参数结构体 (out)
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_META_DATA_to_system_com(HIK_VCA_TARGET_LIST_COM     *p_meta_data,
									IVS_SYS_PROC_PARAM  *param)
{
	IVS_SYS_BUF_MANAGER   buf_manager;
	IVS_SYS_BUF_MANAGER  *bm;	
	INT32_T i;
	BYTE padding_flag;
	UINT32_T code;
    unsigned short extrn_type = IVS_EXTEN_TYPE_RESVERD_ALL;

	CHECK_ERROR(param == NULL || p_meta_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->scale_width > CONST_13_BITS_OPERATOR 
		|| param->scale_height > CONST_13_BITS_OPERATOR, HIK_IVS_SYS_LIB_E_SCALE_WH);

	bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);

	//扩展标示
	EMIT_BITS(0xffff, 16);
	EMIT_BITS(extrn_type, 16); //内部标志

	EMIT_BITS(p_meta_data->type, 32);

	//当前帧视频图像智能算法检测到的目标数量
	EMIT_BITS(p_meta_data->target_num, 24);

	//PaddingFlag先设成'0', 后面重新设置
	//PaddingFlag = 0 ExtendByteNum_0 =0  ExtendByteNum_1 = 1  
	EMIT_BITS(1, 8);

	//宽度方向上的缩放比例 
	code = (MARKER_BIT << 15) | bm->scale_width;
	EMIT_BITS(code, 16);

	//高度方向上的缩放比例 
	code = (MARKER_BIT << 15) | bm->scale_height;
	EMIT_BITS(code, 16);

	for (i = 0; i < p_meta_data->target_num; i++)
	{
		emit_target_bits(bm, &(p_meta_data->p_target[i]), extrn_type);
	}

	padding_flag = emit_padding_byte(bm);

	//重新设置PaddingFlag
	bm->buf_start[11] |= padding_flag << 7 ;

	//计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);

	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER); //FIXME:

	return HIK_IVS_SYS_LIB_S_OK;
}

/******************************************************************************
* 功  能: 输出警戒规则数组结构体数据
* 参  数: p_rule_data - 警戒规则数组结构体指针(in)
*         param       - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_RULE_DATA_to_system_com(HIK_VCA_RULE_LIST_COM       *p_rule_data,
								IVS_SYS_PROC_PARAM  *param)
{
	UINT32_T i = 0; 
	BYTE padding_flag;
    IVS_SYS_BUF_MANAGER   buf_manager;
    IVS_SYS_BUF_MANAGER  *bm;
	unsigned short extrn_type = IVS_EXTEN_TYPE_RESVERD_ALL;

	CHECK_ERROR(param == NULL || p_rule_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
	
    bm = &buf_manager;
	IVS_CREATE_init_buf_manager(param, bm);

    EMIT_BITS(0xffff, 16);
	EMIT_BITS(extrn_type, 16);

	EMIT_BITS(p_rule_data->type, 32);
    /* 表示HIK_RULE_LIST中包含的规则数量，规则数量代表智能分析算法设置成功的规则数量，
	 * 每条规则将触发特定的事件信息。*/ 
	EMIT_BITS(p_rule_data->rule_num, 24);

	//PaddingFlag先设成'0', 后面重新设置
	//PaddingFlag = 0 ExtendByteNum =0 
	EMIT_BITS(0, 8); 

	for (i = 0; i < p_rule_data->rule_num; i++)
	{
		if (p_rule_data->p_rule[i].enable == TRUE)
		{
			emit_rule_bits(bm, &p_rule_data->p_rule[i]);

			if (extrn_type == IVS_EXTEN_TYPE_COLOR)
			{
				// reserved[0] = 0xdb (R+G+B十六进制和)，表示是颜色版本，
				//reserved[1]为R，reserved[2]为G，reserved[3]为B，表示RGB
				//每个结构体都填写颜色，实在是浪费，后面可以优化
				EMIT_BITS(p_rule_data->p_rule[i].reserved[0], 8);
				EMIT_BITS(p_rule_data->p_rule[i].reserved[1], 8);
				EMIT_BITS(p_rule_data->p_rule[i].reserved[2], 8);
				EMIT_BITS(p_rule_data->p_rule[i].reserved[3], 8);	
			}
            else if (extrn_type == IVS_EXTEN_TYPE_RESVERD_ALL)
            {
                EMIT_BITS(p_rule_data->p_rule[i].reserved[0], 8);
                EMIT_BITS(p_rule_data->p_rule[i].reserved[1], 8);
                EMIT_BITS(p_rule_data->p_rule[i].reserved[2], 8);
                EMIT_BITS(p_rule_data->p_rule[i].reserved[3], 8);	
                EMIT_BITS(p_rule_data->p_rule[i].reserved[4], 8);	
            }
		}
	}

	padding_flag = emit_padding_byte(bm);
	
	//重新设置PaddingFlag
	bm->buf_start[11] |= padding_flag << 7;
	
	//计算输出数据长度
	param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);
	
	//码流保护
	CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER);

    return HIK_IVS_SYS_LIB_S_OK;
}



/***************************************************
多算法库接口
****************************************************/

/******************************************************************************
* 功  能: 颜色信息压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         color    - 颜色信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_color_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_COLOR *color)
{
    if (bm == NULL || color == NULL)
    {
        return;
    }
    EMIT_BITS(color->red,   8);
    EMIT_BITS(color->green, 8);
    EMIT_BITS(color->blue,  8);
    EMIT_BITS(color->alpha, 8);
    return;
}

/******************************************************************************
* 功  能: 速度信息压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         speed    - 速度信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_speed_coord_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_SPECOORD *speed)
{
    if (bm == NULL || speed == NULL)
    {
        return;
    }
    EMIT_BITS(speed->x,     8);
    EMIT_BITS(speed->y,     8);
    EMIT_BITS(speed->speed, 8);
    EMIT_BITS(speed->id,    8);
    EMIT_BITS(speed->flag,  4);
    return;
}

/******************************************************************************
* 功  能: 轨迹信息压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         line     - 轨迹信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_color_line_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_COLORL *line)
{
    if (bm == NULL || line == NULL)
    {
        return;
    }
    EMIT_BITS(line->color.red,   8);
    EMIT_BITS(line->color.green, 8);
    EMIT_BITS(line->color.blue,  8);
    EMIT_BITS(line->color.alpha, 8);
    EMIT_BITS(line->line_time,   8);
    EMIT_BITS(line->target_type, 8);
    return;
}


/******************************************************************************
* 功  能: 双字节坐标信息压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         coord    - 坐标信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_coord_short_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_COORDS *coord)
{
    if (bm == NULL || coord == NULL)
    {
        return;
    }
    EMIT_BITS(coord->x,   16);
    EMIT_BITS(coord->y,   16);
    return;
}


/******************************************************************************
* 功  能: 颜色车速信息信息压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         colspeed - 颜色车速信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_color_speed_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_COLSPEED *colspeed)
{
    if (bm == NULL || colspeed == NULL)
    {
        return;
    }
    emit_color_bits(bm, &colspeed->color);
    EMIT_BITS(colspeed->speed, 8);
    return;
}


/******************************************************************************
* 功  能: 颜色测温信息信息压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         coltemp  - 颜色测温信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_color_temp_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_TEMP *coltemp)
{
    INT32_T  fixed_value = 0;
    float    temp        = 0.0;
    INT32_T  flag        = 0;
    if (NULL == bm ||NULL == coltemp)
    {
        return;
    }
     emit_color_bits(bm, &coltemp->color);
     EMIT_BITS(coltemp->temp_unit, 4);
     EMIT_BITS(coltemp->font_size, 4);
     EMIT_BITS(coltemp->enable,    1);
     EMIT_BITS(coltemp->x, 10);
     EMIT_BITS(coltemp->y, 10);

     // 记录正负
     flag = (coltemp->temp > 0) ? 0 : 1;
     EMIT_BITS(flag, 1);

    // 温度信息精度为小数点后1位
    temp = (flag == 0) ? (coltemp->temp * 10) : (0 - coltemp->temp * 10);
    fixed_value = (INT32_T)temp;
    write_n_linfo_UVLC(bm, fixed_value);
}


/******************************************************************************
* 功  能: 颜色违禁物信息压缩函数
* 参  数: bm         - 内存管理结构体参数(out)
*         contraband - 颜色违禁物信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_color_contraband_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_CONTRABAND *contraband)
{
    INT32_T  temp        = 0;
    INT32_T  i           = 0;
    if (NULL == bm ||NULL == contraband)
    {
        return;
    }
    emit_color_bits(bm, &contraband->color);
    EMIT_BITS(contraband->confidence, 8);
    EMIT_BITS(contraband->pos_len, 8);
    temp = contraband->type;
    for (i = 0; i < 4; i++)
    {
        EMIT_BITS((temp & 0xFF), 8);
        temp = (temp >> 8);
    }
    for (i = 0; i < contraband->pos_len; i++)
    {
        EMIT_BITS(contraband->pos_data[i], 8);
    }
}


/******************************************************************************
* 功  能: 用户扩展信息压缩函数
* 参  数: bm            - 内存管理结构体参数(out)
*         privt_type    - 扩展数据类型(in)
*         privt_data    - 扩展数据（in）
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
int emit_privt_data_bits(IVS_SYS_BUF_MANAGER *bm, unsigned int privt_type,unsigned char* privt_data)
{
    unsigned int privt_len = 0;
    CHECK_ERROR(bm == NULL || privt_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);

    switch(privt_type)
    {
    case IS_PRIVT_COLOR:        // 存储颜色信息
    case IS_PRIVT_COLOR_RANG:   // 色块信息
        {
            emit_color_bits(bm, (IS_PRIVT_INFO_COLOR*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_COLOR);
            break;
        }
    case IS_PRIVT_COLOR_LINE:// 存储轨迹信息
        {
            emit_color_line_bits(bm, (IS_PRIVT_INFO_COLORL*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_COLORL);
            break;
        }
    case IS_PRIVY_SPEED_COORD:// 存储速度信息
        {
            emit_speed_coord_bits(bm, (IS_PRIVT_INFO_SPECOORD*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_SPECOORD);
            break;
        }
    case IS_PRIVT_COORD_S:// 存储双字节坐标信息
        {
            emit_coord_short_bits(bm, (IS_PRIVT_INFO_COORDS*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_COORDS);
            break;
        }
    case IS_PRIVT_EAGLE_R:// 鹰眼跟踪红色四角框
        {
            // 无参数，直接退出
            break;
        }
    case IS_PRIVT_COLOR_SPEED:  // 颜色车速信息
        {
            emit_color_speed_bits(bm, (IS_PRIVT_INFO_COLSPEED*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_COLSPEED);
            break;
        }
    case IS_PRIVT_COLOR_TEMP:
        {
            emit_color_temp_bits(bm, (IS_PRIVT_INFO_TEMP*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_TEMP);
            break;
        }
    case IS_PRIVT_COLOR_CONTRABAND:
        {
            emit_color_contraband_bits(bm, (IS_PRIVT_INFO_CONTRABAND*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_CONTRABAND);
            break;
        }
    default:
        {
            return HIK_IVS_SYS_LIB_S_FAIL;
        }
    }

    return privt_len;
}

HRESULT emit_multi_privt_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO* privt)
{
    unsigned int   privt_type     = 0;
    unsigned char *privt_data     = NULL;
    int            ret            = 0;

    CHECK_ERROR(bm == NULL || privt == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);

    CHECK_ERROR(privt->privt_len > IS_MAX_PRIVT_LEN, HIK_IVS_SYS_LIB_E_DATA_OVERFLOW);
    
    // 存储扩展类型
    EMIT_BITS(privt->privt_type, 8);

    // 存储扩展信息长度
    EMIT_BITS(privt->privt_len, 8);

    privt_type     = privt->privt_type;
    privt_data     = privt->privt_data;

    ret = emit_privt_data_bits(bm, privt_type, privt_data);
    if (ret < 0)
    {
        return ret;
    }

    // 输出剩余数据并字节对齐
    byte_aligned(bm);

    return HIK_IVS_SYS_LIB_S_OK;
}




/******************************************************************************
* 功  能: 新版目标信息压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         target   - 目标信息结构体(in)
*         uVersion - 接口版本（in）
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/
static HRESULT emit_multi_target_bits(IVS_SYS_BUF_MANAGER *bm, HIK_MULT_VCA_TARGET *target, UINT16_T uVersion)
{
    int ret = 0;
    CHECK_ERROR(bm == NULL || target == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);

    // 存储目标信息
    emit_target_bits(bm, &target->target, uVersion);

    ret = emit_multi_privt_bits(bm, &target->privt_info);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }

    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 新版规则信息压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         rule     - 规则信息结构体(in)
*         uVersion - 接口版本（in）
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/
static HRESULT emit_multi_rule_bits(IVS_SYS_BUF_MANAGER *bm, HIK_MULT_VCA_RULE *rule)
{
    int ret = 0;

    CHECK_ERROR(bm == NULL || rule == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);

    // 存储规则信息
    emit_rule_bits(bm, &rule->rule);

    ret = emit_multi_privt_bits(bm, &rule->privt_info);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }

    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 新版目标信息压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         target   - 目标信息结构体(in)
*         uVersion - 接口版本（in）
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/
static HRESULT emit_multi_alert_bits(IVS_SYS_BUF_MANAGER *bm, HIK_MULT_VCA_ALERT* alert)
{
    int ret = 0;

    CHECK_ERROR(bm == NULL || alert == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);

    // 存储报警信息
    EMIT_BITS(alert->alert.view_state, 3);

    ret = emit_rule_info_bits(bm, &alert->alert.rule_info);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }

    emit_alert_target_bits(bm, &alert->alert.target);

    // 存储扩展信息
    ret = emit_multi_privt_bits(bm, &alert->privt_info);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }

    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 多算法目标智能信息压缩接口
* 参  数: p_meta_data - 多算法目标智能信结构体指针(in)
*         param       - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_MULT_META_DATA_to_system(HIK_MULT_VCA_TARGET_LIST* p_meta_data, IVS_SYS_PROC_PARAM* param)
{
    IVS_SYS_BUF_MANAGER   buf_manager;                      // 内存管理结构体
    IVS_SYS_BUF_MANAGER  *bm           = NULL;              // 内存管理结构体指针
    unsigned short        extrn_type   = IVS_MULTI_VERSION; // 接口版本信息
    BYTE                  padding_flag = 0x00;              // 填充字段
    UINT32_T              code         = 0;                 // 临时记录解析得到的码字
    INT32_T               i            = 0;
    HRESULT               ret          = 0;

    CHECK_ERROR(param == NULL || p_meta_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR((param->scale_width  > CONST_13_BITS_OPERATOR || param->scale_width  <= 0) || 
                (param->scale_height > CONST_13_BITS_OPERATOR || param->scale_height <= 0), 
                HIK_IVS_SYS_LIB_E_SCALE_WH);
    CHECK_ERROR(p_meta_data->target_num > MAX_TARGET_NUM_V2,  HIK_IVS_SYS_LIB_E_DATA_OVERFLOW);
    CHECK_ERROR(p_meta_data->p_target == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);

    bm = &buf_manager;
    IVS_CREATE_init_buf_manager(param, bm);

    //扩展标示
    EMIT_BITS(0xffff, 16);
    EMIT_BITS(extrn_type, 16); //内部标志

    EMIT_BITS(p_meta_data->type, 32);

    //当前帧视频图像智能算法检测到的目标数量
    EMIT_BITS(p_meta_data->target_num, 24);

    // 压缩算法信息
    EMIT_BITS(p_meta_data->arith_type, 24);

    //PaddingFlag先设成'0', 后面重新设置
    //PaddingFlag = 0 ExtendByteNum_0 =0  ExtendByteNum_1 = 1  
    EMIT_BITS(1, 8);

    //宽度方向上的缩放比例 
    code = (MARKER_BIT << 15) | bm->scale_width;
    EMIT_BITS(code, 16);

    //高度方向上的缩放比例 
    code = (MARKER_BIT << 15) | bm->scale_height;
    EMIT_BITS(code, 16);

    // 循环打包目标框信息
    for(i = 0;i < p_meta_data->target_num; i++)
    {
        ret = emit_multi_target_bits(bm, &p_meta_data->p_target[i], extrn_type);
        if (ret != HIK_IVS_SYS_LIB_S_OK)
        {
            return ret;
        }
    }

    padding_flag = emit_padding_byte(bm);

    //重新设置PaddingFlag
    bm->buf_start[14] |= padding_flag << 7 ;
    // 
    //计算输出数据长度
    param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);

    //码流保护
    CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER); //FIXME:

    return HIK_IVS_SYS_LIB_S_OK;
}




/******************************************************************************
* 功  能: 多算法规则智能信息压缩接口
* 参  数: p_rule_data - 警戒规则数组结构体指针(in)
*         param       - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_MULT_RULE_DATA_to_system(HIK_MULT_VCA_RULE_LIST* p_rule_data, IVS_SYS_PROC_PARAM* param)
{
    BYTE padding_flag = 0x00;
    IVS_SYS_BUF_MANAGER   buf_manager;
    IVS_SYS_BUF_MANAGER  *bm    = NULL;
    unsigned short extrn_type   = IVS_MULTI_VERSION;
    unsigned int   i            = 0;
    unsigned int   enable_num   = 0;      //有效使能规则个数
    int            ret          = 0;
    UINT32_T       code         = 0;                 // 临时记录解析得到的码字

    CHECK_ERROR(param == NULL || p_rule_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR((param->scale_width  > CONST_13_BITS_OPERATOR || param->scale_width  <= 0) || 
                (param->scale_height > CONST_13_BITS_OPERATOR || param->scale_height <= 0), 
                HIK_IVS_SYS_LIB_E_SCALE_WH);
    CHECK_ERROR(p_rule_data->rule_num > MAX_RULE_NUM_V3,  HIK_IVS_SYS_LIB_E_DATA_OVERFLOW);
    CHECK_ERROR(p_rule_data->p_rule == NULL,              HIK_IVS_SYS_LIB_E_PARA_NULL);

    bm = &buf_manager;
    IVS_CREATE_init_buf_manager(param, bm);

    EMIT_BITS(0xffff, 16);
    EMIT_BITS(extrn_type, 16);

    EMIT_BITS(p_rule_data->type, 32);
    /* 表示HIK_RULE_LIST中包含的规则数量，规则数量代表智能分析算法设置成功的规则数量，
    * 每条规则将触发特定的事件信息。*/ 
    EMIT_BITS(p_rule_data->rule_num, 24);

    EMIT_BITS(p_rule_data->arith_type, 24);

    //PaddingFlag先设成'0', 后面重新设置
    //PaddingFlag = 0 ExtendByteNum =0 
    EMIT_BITS(0, 8); 

    // 用于保存有效规则个数
    EMIT_BITS(0, 24);

    //宽度方向上的缩放比例 
    code = (MARKER_BIT << 15) | bm->scale_width;
    EMIT_BITS(code, 16);

    //高度方向上的缩放比例 
    code = (MARKER_BIT << 15) | bm->scale_height;
    EMIT_BITS(code, 16);

    for (i = 0; i < p_rule_data->rule_num; i++)
    {
        if (p_rule_data->p_rule[i].rule.enable == TRUE)
        {
            ret = emit_multi_rule_bits(bm, &p_rule_data->p_rule[i]);
            if (ret != HIK_IVS_SYS_LIB_S_OK)
            {
                return ret;
            }
            enable_num++;
        }
    }

    padding_flag = emit_padding_byte(bm);

    //重新设置PaddingFlag
    bm->buf_start[14] |= padding_flag << 7;

    // 重新设置有效规则个数
    bm->buf_start[15] |= (enable_num & 0xFF0000) >> 16;
    bm->buf_start[16] |= (enable_num & 0x00FF00) >> 8;
    bm->buf_start[17] |= (enable_num & 0x0000FF);

    //计算输出数据长度
    param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);

    //码流保护
    CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER);

    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 多算法规则智能信息压缩接口
* 参  数: p_rule_data - 警戒规则数组结构体指针(in)
*         param       - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_MULT_ALERT_DATA_to_system(HIK_MULT_VCA_ALERT_LIST* p_alert_data, IVS_SYS_PROC_PARAM* param)
{
    BYTE                  padding_flag = 0x00;
    IVS_SYS_BUF_MANAGER   buf_manager;
    IVS_SYS_BUF_MANAGER  *bm  = NULL;
    unsigned short extrn_type = IVS_MULTI_VERSION;
    unsigned int i            = 0;
    unsigned int enable_num   = 0;
    int          ret          = 0;
    UINT32_T     code         = 0;                 // 临时记录解析得到的码字

    CHECK_ERROR(param == NULL || p_alert_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR((param->scale_width  > CONST_13_BITS_OPERATOR || param->scale_width  <= 0) || 
                (param->scale_height > CONST_13_BITS_OPERATOR || param->scale_height <= 0), 
                HIK_IVS_SYS_LIB_E_SCALE_WH);
    CHECK_ERROR(p_alert_data->alert_num > MAX_ALERT_NUM,  HIK_IVS_SYS_LIB_E_DATA_OVERFLOW);
    CHECK_ERROR(p_alert_data->p_alert == NULL,              HIK_IVS_SYS_LIB_E_PARA_NULL);

    bm = &buf_manager;
    IVS_CREATE_init_buf_manager(param, bm);

    EMIT_BITS(0xffff, 16);
    EMIT_BITS(extrn_type, 16);

    EMIT_BITS(p_alert_data->type, 32);

    /* 表示数组中包含的报警数量 */ 
    EMIT_BITS(p_alert_data->alert_num, 24);

    EMIT_BITS(p_alert_data->arith_type, 24);

    //PaddingFlag先设成'0', 后面重新设置
    //PaddingFlag = 0 ExtendByteNum =0 
    EMIT_BITS(0, 8); 

    // 用于保存有效报警个数
    EMIT_BITS(0, 24);

    //宽度方向上的缩放比例 
    code = (MARKER_BIT << 15) | bm->scale_width;
    EMIT_BITS(code, 16);

    //高度方向上的缩放比例 
    code = (MARKER_BIT << 15) | bm->scale_height;
    EMIT_BITS(code, 16);


    // 循环打包报警信息
    for(i = 0;i < p_alert_data->alert_num; i++)
    {
        if (p_alert_data->p_alert[i].alert.alert == 1)
        {
            ret = emit_multi_alert_bits(bm, &p_alert_data->p_alert[i]);
            if (ret != HIK_IVS_SYS_LIB_S_OK)
            {
                return ret;
            }
            enable_num++;
        }
    }

    padding_flag = emit_padding_byte(bm);

    //重新设置PaddingFlag
    bm->buf_start[14] |= padding_flag << 7 ;

    // 重新设置有效报警个数
    bm->buf_start[15] |= (enable_num & 0xFF0000) >> 16;
    bm->buf_start[16] |= (enable_num & 0x00FF00) >> 8;
    bm->buf_start[17] |= (enable_num & 0x0000FF);
    // 
    //计算输出数据长度
    param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);

    //码流保护
    CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER); //FIXME:

    return HIK_IVS_SYS_LIB_S_OK;
}



/*********************************
    区域接口
**********************************/

/******************************************************************************
* 功  能: 输出旋转框结构体数据(支持正负)
* 参  数: bm      - 内存管理结构体指针        (out)
*         rect    - 归一化的旋转框结构体指针  (in)  
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_rotate_rect_bits(IVS_SYS_BUF_MANAGER *bm, VCA_ROTATE_RECT_F *rect)
{
    INT32_T fixed_value;
    float    abs_value;

    if (NULL == bm || NULL == rect)
    {
        return;
    }

    fixed_value = (rect->cx < 0) ? 1 : 0;
    abs_value   = (rect->cx < 0) ? (0 - rect->cx) : rect->cx;
    EMIT_BITS(fixed_value, 1);

    fixed_value = (INT32_T)(abs_value * bm->scale_width);
    write_n_linfo_UVLC(bm, fixed_value);

    /* 表示目标的外接矩形框的左上角的Y轴位置与视频图像宽度的比例(Y*1.0)/bm->scale_height */
    fixed_value = (rect->cy < 0) ? 1 : 0;
    abs_value   = (rect->cy < 0) ? (0 - rect->cy) : rect->cy;
    EMIT_BITS(fixed_value, 1);

    fixed_value = (INT32_T)(abs_value * bm->scale_width);
    write_n_linfo_UVLC(bm, fixed_value);

    /*表示目标外接矩形宽的宽度与视频图像宽度的比例(Width*1.0)/bm->scale_width */
    fixed_value = (rect->width < 0) ? 1 : 0;
    abs_value   = (rect->width < 0) ? (0 - rect->width) : rect->width;
    EMIT_BITS(fixed_value, 1);

    fixed_value = (INT32_T)(abs_value * bm->scale_width);
    write_n_linfo_UVLC(bm, fixed_value);

    /*表示目标外接矩形宽的高度与视频图像高度的比例(Height*1.0)/bm->scale_height */
    fixed_value = (rect->height < 0) ? 1 : 0;
    abs_value   = (rect->height < 0) ? (0 - rect->height) : rect->height;
    EMIT_BITS(fixed_value, 1);

    fixed_value = (INT32_T)(abs_value * bm->scale_width);
    write_n_linfo_UVLC(bm, fixed_value);

    /*表示目标外接矩形旋转角度与视频图像高度的比例(Height*1.0)/bm->scale_height */
    fixed_value = (rect->theta < 0) ? 1 : 0;
    abs_value   = (rect->theta < 0) ? (0 - rect->theta) : rect->theta;
    EMIT_BITS(fixed_value, 1);

    fixed_value = (INT32_T)(abs_value * bm->scale_width);
    write_n_linfo_UVLC(bm, fixed_value);

}

/******************************************************************************
* 功  能: 输出多边形结构体数据(支持正负)
* 参  数: bm      - 内存管理结构体指针        (out)
*         polygon - 归一化的多边形结构体指针  (in)  
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_polygon_ext_bits(IVS_SYS_BUF_MANAGER *bm, VCA_POLYGON_F *polygon)
{
    UINT32_T i           = 0;
    WORD     fixed_value = 0;
    UINT32_T code        = 0;
    float    abs_value   = 0;
    if (NULL == bm || NULL == polygon)
    {
        return;
    }

    // 输出多边形边界点
    for (i = 0; i < polygon->vertex_num; i++)
    {
        // x
        fixed_value = (polygon->point[i].x < 0) ? 1 : 0;
        abs_value   = (polygon->point[i].x < 0) ? (0 - polygon->point[i].x) : polygon->point[i].x;
        EMIT_BITS(fixed_value, 1);

        fixed_value = (WORD)(abs_value * CONST_15_BITS_OPERATOR);
        code        = (MARKER_BIT << 15) | fixed_value;
        EMIT_BITS(code, 16);

        // y
        fixed_value = (polygon->point[i].y < 0) ? 1 : 0;
        abs_value   = (polygon->point[i].y < 0) ? (0 - polygon->point[i].y) : polygon->point[i].y;
        EMIT_BITS(fixed_value, 1);

        fixed_value = (WORD)(abs_value * CONST_15_BITS_OPERATOR);
        code        = (MARKER_BIT << 15) | fixed_value;
        EMIT_BITS(code, 16);
    }
}


/******************************************************************************
* 功  能: 输出矩形框结构体数据(支持正负)
* 参  数: bm      - 内存管理结构体指针        (out)
*         rect    - 归一化的矩形框结构体指针  (in)  
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void emit_rect_normalize_entropy_ext_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RECT_F *rect)
{
    INT32_T fixed_value = 0;
    float   abs_value   = 0;

    if (bm == NULL || rect == NULL)
    {
        return;
    }

    /* 表示目标的外接矩形框的左上角的X轴位置与视频图像宽度的比例(X*1.0)/bm->scale_height */
    fixed_value = (rect->x < 0) ? 1 : 0;
    abs_value   = (rect->x < 0) ? (0 - rect->x) : rect->x;
    EMIT_BITS(fixed_value, 1);

    fixed_value = (INT32_T)(abs_value * bm->scale_width);
    write_n_linfo_UVLC(bm, fixed_value);

    /* 表示目标的外接矩形框的左上角的Y轴位置与视频图像宽度的比例(Y*1.0)/bm->scale_height */
    fixed_value = (rect->y < 0) ? 1 : 0;
    abs_value   = (rect->y < 0) ? (0 - rect->y) : rect->y;
    EMIT_BITS(fixed_value, 1);

    fixed_value = (INT32_T)(abs_value * bm->scale_width);
    write_n_linfo_UVLC(bm, fixed_value);

    /*表示目标外接矩形宽的宽度与视频图像宽度的比例(Width*1.0)/bm->scale_width */
    fixed_value = (rect->width < 0) ? 1 : 0;
    abs_value   = (rect->width < 0) ? (0 - rect->width) : rect->width;
    EMIT_BITS(fixed_value, 1);

    fixed_value = (INT32_T)(abs_value * bm->scale_width);
    write_n_linfo_UVLC(bm, fixed_value);

    /*表示目标外接矩形宽的高度与视频图像高度的比例(Height*1.0)/bm->scale_height */
    fixed_value = (rect->height < 0) ? 1 : 0;
    abs_value   = (rect->height < 0) ? (0 - rect->height) : rect->height;
    EMIT_BITS(fixed_value, 1);

    fixed_value = (INT32_T)(abs_value * bm->scale_width);
    write_n_linfo_UVLC(bm, fixed_value);
}


/******************************************************************************
* 功  能: 输出区域结构体数据(支持正负)
* 参  数: bm       - 内存管理结构体指针        (out)
*         p_region - 区域结构体指针  (in)  
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static HRESULT emit_region_bits(IVS_SYS_BUF_MANAGER *bm, VCA_REGION* p_region)
{
    CHECK_ERROR(bm == NULL || p_region == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);

    EMIT_BITS(p_region->region_type, 4);

    switch(p_region->region_type)
    {
    case VCA_REGION_POLYLINE:       // 多边形线
    case VCA_REGION_POLYGON:        // 多边形
        {
            //PointNum
            EMIT_BITS(p_region->polygon.vertex_num, 6);
            emit_polygon_ext_bits(bm, &p_region->polygon);
            break;
        }
    case VCA_REGION_RECT:           // 四边形框
        {
            emit_rect_normalize_entropy_ext_bits(bm, &p_region->rect);
            break;
        }
    case VCA_REGION_ROTATE_RECT:    // 旋转框
        {
            emit_rotate_rect_bits(bm, &p_region->rotate_rect);
            break;
        }
    default:
        {
            return HIK_IVS_SYS_LIB_E_PARA_NULL;
        }
    }

    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 目标区域信息压缩函数
* 参  数: bm        - 内存管理结构体参数(out)
*         target    - 目标区域信息结构体(in)
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/
static HRESULT emit_target_blob_bits(IVS_SYS_BUF_MANAGER *bm, VCA_BLOB_BASIC_INFO *target)
{
    UINT32_T code = 0;
    int      i    = 0;
    int      ret  = 0;
    CHECK_ERROR(bm == NULL || target == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);

    // 保存当前目标的生成序号
    for (i = 0, code = target->id; i < 4;i++, code = code >> 8)
    {
        EMIT_BITS(code, 8);
    }

    // 保存当前目标的区域类型
    for (i = 0, code = target->blob_type; i < 4; i++, code = code >> 8)
    {
        EMIT_BITS(code, 8);
    }

    // 置信度
    EMIT_BITS(target->confidence, 16);

    ret = emit_region_bits(bm, &target->region);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }

    // 字节对齐
    byte_aligned(bm);

    return HIK_IVS_SYS_LIB_S_OK;
}

/******************************************************************************
* 功  能: 扩展目标区域信息压缩函数
* 参  数: bm        - 内存管理结构体参数(out)
*         target    - 扩展目标信息结构体(in)
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/
static HRESULT emit_multi_target_blob_bits(IVS_SYS_BUF_MANAGER *bm, HIK_TARGET_BLOB *target)
{
    int ret = 0;
    CHECK_ERROR(bm == NULL || target == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);

    // 存储目标信息
    emit_target_blob_bits(bm, &target->target);

    ret = emit_multi_privt_bits(bm, &target->privt_info);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }

    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 扩展目标区域信息压缩接口
* 参  数: p_target_data - 扩展目标区域信息结构体指针(in)
*         param         - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_META_BLOB_DATA_to_system(HIK_TARGET_BLOB_LIST* p_target_data, IVS_SYS_PROC_PARAM* param)
{
    BYTE                  padding_flag = 0x00;
    IVS_SYS_BUF_MANAGER   buf_manager;
    IVS_SYS_BUF_MANAGER  *bm         = NULL;
    unsigned short        extrn_type = IVS_MULTI_BLOB;
    unsigned int          i          = 0;
    int                   ret        = 0;
    UINT32_T              code       = 0;                 // 临时记录解析得到的码字

    CHECK_ERROR(param == NULL || p_target_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf == NULL,                    HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR((param->scale_width  > CONST_13_BITS_OPERATOR || param->scale_width  <= 0) || 
                (param->scale_height > CONST_13_BITS_OPERATOR || param->scale_height <= 0), 
                HIK_IVS_SYS_LIB_E_SCALE_WH);
    CHECK_ERROR(p_target_data->target_num > MAX_ALERT_NUM,  HIK_IVS_SYS_LIB_E_DATA_OVERFLOW);
    CHECK_ERROR(p_target_data->p_target == NULL,              HIK_IVS_SYS_LIB_E_PARA_NULL);


    bm = &buf_manager;
    IVS_CREATE_init_buf_manager(param, bm);

    EMIT_BITS(0xffff,     16);
    EMIT_BITS(extrn_type, 16);

    EMIT_BITS(p_target_data->type, 32);

    //当前帧视频图像智能算法检测到的目标数量
    EMIT_BITS(p_target_data->target_num, 24);

    // 压缩算法信息
    EMIT_BITS(p_target_data->arith_type, 24);

    //PaddingFlag先设成'0', 后面重新设置
    //PaddingFlag = 0 ExtendByteNum_0 =0  ExtendByteNum_1 = 1  
    EMIT_BITS(1, 8);

    //宽度方向上的缩放比例 
    code = (MARKER_BIT << 15) | bm->scale_width;
    EMIT_BITS(code, 16);

    //高度方向上的缩放比例 
    code = (MARKER_BIT << 15) | bm->scale_height;
    EMIT_BITS(code, 16);

    // 循环打包目标框信息
    for(i = 0;i < p_target_data->target_num; i++)
    {
        ret = emit_multi_target_blob_bits(bm, &p_target_data->p_target[i]);
        if (ret != HIK_IVS_SYS_LIB_S_OK)
        {
            return ret;
        }
    }

    padding_flag = emit_padding_byte(bm);

    //重新设置PaddingFlag
    bm->buf_start[14] |= padding_flag << 7 ;
    // 
    //计算输出数据长度
    param->len = (UINT32_T)(bm->cur_pos - bm->buf_start);

    //码流保护
    CHECK_ERROR(param->len > bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER); //FIXME:

    return HIK_IVS_SYS_LIB_S_OK;
}
