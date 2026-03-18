/**
 *
 * 版权信息: Copyright (c) 2013, 杭州海康威视数字技术股份有限公司
 * 
 * 文件名称: IVS_SYS_Parse.c
 * 文件标识: HIKVISION_IVS_SYS_PARSE_C
 * 摘    要: 海康威视智能信息解析文件
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
 * 当前版本: 4.1.4
 * 作    者: 辛安民
 * 日    期: 2017年06月27号
 * 备    注:
 *            1.错误数据导致规则崩溃进行保护。
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
 * 当前版本: 2.3.0
 * 作    者: 黄崇基
 * 日    期: 2010年7月6号
 * 备    注: 增加对IAS数据的封装和解封装支持，仍然调用与IVS一样的接口
 *
 * 更新版本: 2.2.3
 * 作    者: 黄崇基
 * 日    期: 2009年10月26日
 * 备    注: 增加人脸识别加密数据的解封装接口
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
 * 日    期: 2009年2月12日
 * 备    注:
 */

#include "IVS_SYS_inner.h"
#include "old_keep.h"
#include <string.h>

/** @fn     static void IVS_PARSE_init_buf_manager(IVS_SYS_BUF_MANAGER *bm, IVS_SYS_PROC_PARAM *param)
 *  @brief  初始化内存管理参数
 *  @param  bm      [OUT]   - 内存管理结构体参数
            param   [IN]    - 处理参数结构体
 *  @return 无
 */
static void IVS_PARSE_init_buf_manager(IVS_SYS_BUF_MANAGER *bm, IVS_SYS_PROC_PARAM *param)
{
    if (NULL == bm || NULL == param || param->buf_size < 4)
    {
        return;
    }
	bm->cnt	      = 32; //缓存的bit数为32
	bm->buf_start = param->buf;
	
	bm->byte_buf  = ((UINT32_T)bm->buf_start[0] << 24) + ((UINT32_T)bm->buf_start[1] << 16)
		          + ((UINT32_T)bm->buf_start[2] << 8)  + ((UINT32_T)bm->buf_start[3]);
	
	bm->cur_pos	  = bm->buf_start + 4;  //当前指针后移4字节
	bm->buf_size  = param->buf_size;
    bm->over_flag = 0;
}

/** @fn     UINT32_T IVS_SYS_GetVLCN (IVS_SYS_BUF_MANAGER *bm, UINT32_T n)
 *  @brief  已知长度，求码字
 *  @param  bm      [OUT]   - 内存管理结构体参数
            n       [IN]    - 长度
 *  @return 返回n-bits长度的码字
 *  @note   保证 n <= bm->cnt
 */
UINT32_T IVS_SYS_GetVLCN (IVS_SYS_BUF_MANAGER *bm, UINT32_T n)
{
    UINT32_T rack = bm->byte_buf;   //4字节的缓存内容
    UINT32_T num = bm->cnt;         //当前有效的bit数
    INT32_T code;   //记录返回值
	INT32_T rack_test = 0;
    INT32_T pos       = 0;

    //对输入参数进行有效性判断
    if (NULL == bm )
    {
        return HIK_IVS_SYS_LIB_E_PARA_NULL;
    }
    if (n > bm->cnt)
    {
        bm->over_flag = 1;
        return 0;
    }
	/*assert(n <= num);*/

	code = rack >> (32 - n);    //取rack的高n个bit
    
	rack_test = rack;
	rack <<= n;        //把rack的高n个bit移出

	if (rack_test == rack)
	{
		rack = 0;
	}

	num -= n;   //当前有效bit数减小

    //当解析缓存中的有效位数小于24时，即已完整解析一个字节后，读取一个字节
	while(num <= 24)
	{
        pos = bm->cur_pos - bm->buf_start;
        if (pos + 1 > bm->buf_size)
        {
            bm->cnt = num;
            bm->byte_buf = rack;
            return code;
        }
	   rack |= *bm->cur_pos++ << (24 - num);

	   num += 8;
	}

	bm->cnt = num;      //把解析和读取后，当前缓存中的有效bit数传给bm结构体

	bm->byte_buf = rack;    //把解析和读取后，当前缓存中的实际数据传给bm结构体

	return code;
}

/** @fn     UINT32_T IVS_SYS_GetVLCSymbol (IVS_SYS_BUF_MANAGER *bm, UINT32_T *info)
 *  @brief  获取码字
 *  @param  bm          [IN]    - 内存管理结构体参数
            info        [OUT]   - 码字数据指针
 *  @return 码字长度
 */
UINT32_T IVS_SYS_GetVLCSymbol (IVS_SYS_BUF_MANAGER *bm, UINT32_T *info)
{
    /* rack, 32 bits 的缓存内容     num, 当前有效的bit数    len, rack中高16位连续0的个数
       i, 循环计数      mask, 32bit中只有一位为1，用于与计算求位1个数*/
    UINT32_T rack, num, len, i, mask;
    INT32_T pos       = 0;

    if (NULL == bm || NULL == info)
    {
        return HIK_IVS_SYS_LIB_E_PARA_NULL;
    }

	rack = bm->byte_buf;    //32 bits 的缓存内容
	num = bm->cnt;          //当前有效的bit数

	mask = 1 << 31;         //0x80000000

	len = 1;    //有效bit数为1

	//计算rack中高16位连续0的个数 + 1
	for(i = 0; ((i < 16) && (!(rack & mask))); i ++, mask >>= 1)
	{
		len ++;
	}

	num -= len; //除去高位为0后的有效位数

	rack <<= len;   //从缓存中移出高位为0的bits

    //当解析bit数超过8，即已解析完整一个字节后，读取一个字节
	while(num <= 24)
	{
        pos = bm->cur_pos - bm->buf_start;
        if (pos + 1 > bm->buf_size)    // 防止读取越界
        {
            return 0;
        }
	    rack |= *bm->cur_pos++ << (24 - num);

        num += 8;
	}

    //32bits的缓存中高16位连续0的个数为0，即最高位为1
	if(len <= 1)
	{
	   *info = 0;

	    bm->cnt = num;

	    bm->byte_buf = rack;

	    return 1;
	}

	*info = (rack >> (32 - (len-1)));   //ue编码的解码
	num -= len-1;
	rack <<= len-1;

    //当解析bit数超过8，即已解析完整一个字节后，读取一个字节
	while(num <= 24)
	{
        pos = bm->cur_pos - bm->buf_start;
        if (pos + 1 > bm->buf_size)    // 防止读取越界
        {
            bm->cnt = num;
            bm->byte_buf = rack;
            return 0;
        }
        rack |= *bm->cur_pos++ << (24 - num);

	   num += 8;
	}

    //把解析后内存管理参数的相关字段更新
	bm->cnt = num;
	bm->byte_buf = rack;

	return (len+len-1);
}

/** @fn     UINT32_T IVS_SYS_read_linfo(IVS_SYS_BUF_MANAGER *bm)
 *  @brief  无符号数编码产生的码字解码
 *  @param  bm          [IN]    - 内存管理结构体参数
 *  @return 返回码字
 */
UINT32_T IVS_SYS_read_linfo(IVS_SYS_BUF_MANAGER *bm)
{
    UINT32_T len, inf;  //编码长度，部分解码数据

    if (NULL == bm)
    {
        return HIK_IVS_SYS_LIB_E_PARA_NULL;
    }

    len =  IVS_SYS_GetVLCSymbol (bm, &inf);

    return ((1 << (len >> 1)) + inf - 1);   //返回解码后的数值
}

/** @fn     INT32_T IVS_SYS_read_linfo_signed(IVS_SYS_BUF_MANAGER *bm)
 *  @brief  有符号数编码产生的码字解码
 *  @param  bm          [IN]    - 内存管理结构体参数
 *  @return 返回码字
 */
INT32_T IVS_SYS_read_linfo_signed(IVS_SYS_BUF_MANAGER *bm)
{
    UINT32_T len, inf;      //编码长度，部分解码数据
    INT32_T n;    //记录无符号数解码后的数据
    INT32_T code;   //记录最后的有符号数解码后的数据

    if (NULL == bm)
    {
        return HIK_IVS_SYS_LIB_E_PARA_NULL;
    }

    len =  IVS_SYS_GetVLCSymbol (bm, &inf);

    n = (1 << (len >> 1)) + inf - 1;    //得到的无符号数解码后数据

    /*编码时，若语法元素为正数，则乘2减1，转换成codeNum， 解码时加1除2，得到解码数据*/
    code = (n + 1) >> 1;

    if((n & 0x01) == 0)  // lsb is signed bit
    {
        code = -code;
    }

    return code;
}

/** @fn     static void get_rect_normalize_entropy_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RECT_F *rect)
 *  @brief  解析码流，从中获取归一化后的边界框结构体数据
 *  @param  bm          [IN]    - 内存管理结构体参数
            rect        [OUT]   - 归一化后的边界框结构体指针
 *  @return 无
 */
static void get_rect_normalize_entropy_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RECT_F *rect)
{
	UINT32_T value;     //临时记录坐标值
	
	if (bm == NULL || rect == NULL)
	{
		return ;
	}
	//边界框左上角X轴坐标
	value = IVS_SYS_read_linfo(bm);

    //框的宽高为0没有意义
	if (bm->scale_width == 0 || bm->scale_height == 0)
	{
		return ;
	}

    rect->x = (float)value / bm->scale_width; 

	//边界框左上角Y轴坐标
	value = IVS_SYS_read_linfo(bm);
	rect->y = (float)value / bm->scale_height;

	//边界框的宽度
	value = IVS_SYS_read_linfo(bm);
    rect->width = (float)value / bm->scale_width;

	//边界框的高度
	value = IVS_SYS_read_linfo(bm);
	rect->height = (float)value / bm->scale_height;
}

/** @fn     static void get_vector_info_bits(IVS_SYS_BUF_MANAGER *bm, VCA_SYS_VECTOR_INFO *vector)
 *  @brief  解析码流，从中获取目标速度信息数据
 *  @param  bm          [IN]    - 内存管理结构体参数
            vector      [OUT]   - 目标速度信息结构体指针
 *  @return 无
 */
static void get_vector_info_bits(IVS_SYS_BUF_MANAGER *bm, VCA_SYS_VECTOR_INFO *vector)
{
	INT32_T value;  //临时向量坐标
	
	if (bm == NULL || vector == NULL)
	{
		return ;
	}
    value = IVS_SYS_read_linfo_signed(bm);

    //框的宽高为0没有意义
	if (bm->scale_width == 0 || bm->scale_height == 0)
	{
		return ;
	}
    //X
	vector->vx = (float)value / bm->scale_width;
    //Y
	value = IVS_SYS_read_linfo_signed(bm);
	vector->vy = (float)value / bm->scale_height;
	//width
	value = IVS_SYS_read_linfo_signed(bm);
	vector->ax = (float)value / bm->scale_width;
    //height
	value = IVS_SYS_read_linfo_signed(bm);
	vector->ay = (float)value / bm->scale_height;
}

/** @fn     static void get_path_bits(IVS_SYS_BUF_MANAGER *bm, VCA_SYS_PATH *path)
 *  @brief  解析码流，从中获取目标轨迹
 *  @param  bm          [IN]    - 内存管理结构体参数
            path        [OUT]   - 目标轨迹结构体指针
 *  @return 无
 */
static void get_path_bits(IVS_SYS_BUF_MANAGER *bm, VCA_SYS_PATH *path)
{
	INT32_T i;  //循环计数
	INT32_T mv_x, mv_y, fixed_value;    //x,y绝对坐标
	INT32_T mvd_x, mvd_y;   //x,y相对坐标
	INT32_T mvx_previous = 0, mvy_previous = 0; //记录上一个x,y坐标

	if (bm == NULL || path == NULL)
	{
		return ;
	}
	path->point_num = IVS_SYS_GetVLCN(bm, 5);
    //点数不超过最大值10
    if (path->point_num > VCA_SYS_MAX_PATH_LEN)
    {
		path->point_num = 1;
    }

	// 解析当前路径的新产生的点
	fixed_value = IVS_SYS_GetVLCN(bm, 13);
	path->point[0].x = (float)fixed_value / CONST_13_BITS_OPERATOR;

	fixed_value = IVS_SYS_GetVLCN(bm, 13);
	path->point[0].y = (float)fixed_value / CONST_13_BITS_OPERATOR;

	if (bm->scale_height == 0 || bm->scale_width == 0)
	{
		return ;
	}

    //遍历每个点，解析得到每个点的坐标值
	for (i = 0; i < (INT32_T)path->point_num - 1; i++)
	{
        mvd_x = IVS_SYS_read_linfo_signed(bm);
        mv_x  = mvd_x + mvx_previous;
        path->point[i+1].x = IVS_SYS_MAX(0, path->point[i].x + (float)mv_x / bm->scale_width);

		mvd_y = IVS_SYS_read_linfo_signed(bm);
		mv_y  = mvd_y + mvy_previous;
        path->point[i+1].y = IVS_SYS_MAX(0, path->point[i].y + (float)mv_y / bm->scale_height);

		mvx_previous    = mv_x;
		mvy_previous    = mv_y;	
	} 
} 

/** @fn     static void discard_aligned_bits(IVS_SYS_BUF_MANAGER *bm)
 *  @brief  丢弃用于码字字节对齐bit
 *  @param  bm          [IN]    - 内存管理结构体参数
 *  @return 无
 */
static void discard_aligned_bits(IVS_SYS_BUF_MANAGER *bm)
{
	INT32_T padding_bits_num;   //用于对齐的bit数

    if (NULL == bm)
    {
        return;
    }

    padding_bits_num = bm->cnt % 8;//求得用于对齐的bit数
	
    bm->byte_buf <<= padding_bits_num;  //在内存结构体里丢弃这些bits
	bm->cnt       -= padding_bits_num;
}

/** @fn     static void get_target_bits(IVS_SYS_BUF_MANAGER *bm, 
                                            VCA_TARGET *target, 
                                            UINT32_T extend_num,
                                            UINT16_T uVersion)
 *  @brief  解析码流，从中获取目标数据
 *  @param  bm          [IN]    - 内存管理结构体参数
            target      [OUT]   - 目标结构体指针
            extend_num  [IN]    - 扩展字节个数
 *  @return 无
 */
static void get_target_bits(IVS_SYS_BUF_MANAGER *bm, VCA_TARGET *target, UINT32_T extend_num, UINT16_T uVersion)
{
    UINT32_T i; //循环计数
    int code;   //临时记录解析得到的码字

    if (NULL == bm || NULL == target)
    {
        return;
    }

    //包含扩展字段
	if (extend_num)
	{
		code = IVS_SYS_GetVLCN(bm, 8);
		target->alarm_flg = (code >> 7) & 1; //alarm_flg
		extend_num--;
	}
	for (i = 0; i < extend_num; i++)
    {
		IVS_SYS_GetVLCN(bm, 8); //跳过保留字节
	}

	// 目标类型
	target->type = (BYTE)IVS_SYS_GetVLCN(bm, 4);

	// ID
	if (uVersion >= 0x0400)
	{
		target->ID = IVS_SYS_GetVLCN(bm, 8);
		target->ID = IVS_SYS_GetVLCN(bm, 15) | (target->ID << 24);
	}
	else
	{
		target->ID = IVS_SYS_GetVLCN(bm, 15);
	}

	// 目标框 rect
	get_rect_normalize_entropy_bits(bm, &target->rect);
	
	//老版本才解析
	if (uVersion == 0)
	{
		VCA_SYS_PATH path;
		VCA_SYS_VECTOR_INFO vector_info; 
		//获取当前轨迹位置	
		get_path_bits(bm, &path);     
		
		// vector 速度
		get_vector_info_bits(bm, &vector_info);	
	}

    //支持颜色获取方案reserved[0]= 0xdb，表示是颜色版本，
    //然后reserved[1]，reserved[2]，reserved[3]，表示RGB
    if (uVersion == IVS_EXTEN_TYPE_COLOR)
    {
        target->reserved[0] = IVS_SYS_GetVLCN(bm, 8);
        target->reserved[1] = IVS_SYS_GetVLCN(bm, 8);
        target->reserved[2] = IVS_SYS_GetVLCN(bm, 8);
        target->reserved[3] = IVS_SYS_GetVLCN(bm, 8);
    }
    else if (uVersion == IVS_EXTEN_TYPE_RESVERD_ALL)
    {
        target->reserved[0] = IVS_SYS_GetVLCN(bm, 8);
        target->reserved[1] = IVS_SYS_GetVLCN(bm, 8);
        target->reserved[2] = IVS_SYS_GetVLCN(bm, 8);
        target->reserved[3] = IVS_SYS_GetVLCN(bm, 8);
        target->reserved[4] = IVS_SYS_GetVLCN(bm, 8);
        target->reserved[5] = IVS_SYS_GetVLCN(bm, 8);
    }
    

    // 抛弃用于字节对齐的bits
    discard_aligned_bits(bm);

}

/** @fn     HRESULT IVS_META_DATA_sys_parse(VCA_TARGET_LIST     *p_meta_data,
                                            IVS_SYS_PROC_PARAM  *param)
 *  @brief  解析码流，从中获取帧内目标信息数组数据
 *  @param  p_meta_data     [OUT]   - 目标信息结构体
            param           [IN]    - 处理参数结构体
 *  @return 返回状态码
 */
HRESULT IVS_META_DATA_sys_parse(VCA_TARGET_LIST         *p_meta_data,
						  	    IVS_SYS_PROC_PARAM  *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存管理结构体
    //extend_byte_num_0对应MetaData结构体中的保留字节的个数。extend_byte_num_1对应Target结构体中的保留字节的个数
	INT32_T i, extend_byte_num_0, extend_byte_num_1;    
	UINT32_T value;     //临时记录解析得到的码字
	BYTE *pAnaBuffer;   //输入数据的缓冲
	UINT16_T uTag = 0;      //版本号的高16位
	UINT16_T uVersion = 0;  //版本号的低16位
    IVS_SYS_PROC_PARAM stParam;
	UINT16_T   uExtenMark = 0;

    CHECK_ERROR(p_meta_data == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf  == NULL,                  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len   <  4,                     HIK_IVS_SYS_LIB_S_FAIL);

    stParam.buf = param->buf;
    stParam.buf_size = param->buf_size;
    stParam.len = param->len;
    stParam.scale_height = param->scale_height;
    stParam.scale_width = param->scale_width;

	pAnaBuffer = (BYTE*)param->buf;
    //版本号标识，版本号
	uTag = (pAnaBuffer[0] << 8) + pAnaBuffer[1];	
	if (uTag == 0xffff)
	{
		uVersion= (pAnaBuffer[2] << 8) + pAnaBuffer[3];
		//param->buf = pAnaBuffer + 4;
		//param->len -= 4;
        stParam.buf = pAnaBuffer + 4;
        stParam.len -= 4;

		uExtenMark = uVersion;
	}

	bm = &buf_manager;
	IVS_PARSE_init_buf_manager(bm, &stParam);   //初始化内存管理参数
		
	p_meta_data->target_num = (BYTE)IVS_SYS_GetVLCN(bm, 8); //获得目标数

	if (uExtenMark == IVS_EXTEND_TARGET_NUM_MARK && p_meta_data->target_num > MAX_TARGET_NUM)
	{
		return HIK_IVS_SYS_LIB_E_DATA_OVERFLOW;
	}

    //最大目标数不超过30
    if (p_meta_data->target_num > MAX_TARGET_NUM)
    {
		p_meta_data->target_num = 0;
    }
	
    //解析得到8个bits
    value = IVS_SYS_GetVLCN(bm, 8);

	extend_byte_num_0 = (value >> 4) & 0x7; //MetaData结构体中的保留字节的个数
	extend_byte_num_1 = value & 0xF;        //Target结构体中的保留字节的个数

    for (i = 0; i < extend_byte_num_0; i++)
    {
        IVS_SYS_GetVLCN(bm, 8);//跳过保留字节
    }

	//宽度方向上的缩放比例 
	value = IVS_SYS_GetVLCN(bm, 16);
	bm->scale_width = (UINT16_T)(value & CONST_15_BITS_MASK);

	//高度方向上的缩放比例 
	value = IVS_SYS_GetVLCN(bm, 16);
	bm->scale_height = (UINT16_T)(value & CONST_15_BITS_MASK);

	for (i = 0; i < p_meta_data->target_num; i++)
	{
		get_target_bits(bm, &(p_meta_data->target[i]), extend_byte_num_1, uVersion);    //获取目标数据
	}

	//码流保护
    CHECK_ERROR((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size, 
        HIK_IVS_SYS_LIB_E_MEM_OVER);

    return HIK_IVS_SYS_LIB_S_OK;
}

/** @fn     static void get_counter_bits(IVS_SYS_BUF_MANAGER *bm, VCA_SYS_COUNTER *counter)
 *  @brief  解析码流，从中获取计数器结构体数据
 *  @param  bm          [IN]    - 内存管理结构体参数
            counter     [OUT]   - 计数器结构体指针
 *  @return 无
 */
static void get_counter_bits(IVS_SYS_BUF_MANAGER *bm, VCA_SYS_COUNTER *counter)
{
	UINT32_T counter_num_h, counter_num_l;  //计数值的高低16位。由于码字解析函数IVS_SYS_GetVLCN不接受一次性解析32bit，所以32bits码字的解析分两次送入

    if (NULL == bm || NULL == counter)
    {
        return ;
    }
	
    //获取计数值的高16bits
	counter_num_h  = IVS_SYS_GetVLCN(bm, 16);
	counter_num_h &= CONST_15_BITS_MASK;
	//获取计数值的低16bits
	counter_num_l  = IVS_SYS_GetVLCN(bm, 16);
	counter_num_l &= CONST_15_BITS_MASK;

    //高低各16bits组合成计数器个数32bits
	counter->counter1 = (counter_num_h << 15) | counter_num_l;

    //获取第二个计数值的高低16bits
	counter_num_h  = IVS_SYS_GetVLCN(bm, 16);
	counter_num_h &= CONST_15_BITS_MASK;
	
	counter_num_l  = IVS_SYS_GetVLCN(bm, 16);
	counter_num_l &= CONST_15_BITS_MASK;
	
    //得到第二个计数值
	counter->counter2 = (counter_num_h << 15) | counter_num_l;
}

/** @fn     static void get_rule_param_sets_bits(IVS_SYS_BUF_MANAGER *bm, 
                                                    VCA_RULE_PARAM_SETS *param, 
                                                    UINT32_T            event_type,
                                                    UINT32_T            param_num,
                                                    UINT32_T			 nVersion)
 *  @brief  获取规则参数
 *  @param  bm          [IN]    - 内存管理结构体参数
            param       [OUT]   - 规则参数联合(union)结构体
            event_type  [IN]    - 事件类型
            param_num   [IN]    - 参数个数
 *  @return 无
 */
static void get_rule_param_sets_bits(IVS_SYS_BUF_MANAGER *bm, 
									 VCA_RULE_PARAM_SETS     *param, 
									 UINT32_T             event_type,
									 UINT32_T             param_num,
									 UINT32_T			nVersion)
{
    UINT32_T i; //循环计数
	UINT32_T param_sets[16];//参数设置数组

    if (NULL == bm || NULL == param)
    {
        return ;
    }
	//一律按照16位来解析，不兼容农行定制
	for (i = 0; i < param_num; i++)
	{
		param_sets[i] = IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK;
	}
	
	// 110812 modify param set
	// diff v2.2 and other
    switch(event_type)
	{
	case VCA_TRAVERSE_PLANE://跨越警戒面
        {
            param->traverse_plane.cross_direction = param_sets[0];
            break;
        }
    case VCA_INTRUSION: 	//入侵
        {
            param->intrusion.delay = param_sets[0];
            break;
        }
	case VCA_LOITER:  //徘徊
        {
            param->loiter.delay = param_sets[0];
            break;
        }
	case VCA_LEFT_TAKE: //丢包捡包
	case VCA_LEFT:      //丢包
	case VCA_TAKE:      //捡包
        {
            param->left_take.delay = param_sets[0];
            break;
        }
	case VCA_STICK_UP:	//贴纸条  
        {
            param->stick_up.delay = param_sets[0];
            break;
        }
	case VCA_INSTALL_SCANNER:	//安装读卡器 
        {
            param->insert_scanner.delay = param_sets[0];
            break;
        }
	case VCA_PARKING: //停车
        {
            param->parking.delay = param_sets[0];
            break;
        }
	case VCA_ABNORMAL_FACE: //异常人脸
        {
            param->abnormal_face.delay = param_sets[0];
            if (nVersion == 0x0202)
            {
                param->abnormal_face.mode  = param_sets[1];
            }
            break;
        }
	case VCA_OVER_TIME://操作超时
        {
            param->over_time.delay = param_sets[0];
            break;
        }
	case VCA_RUN:	//奔跑
        {
            param->run.speed = (float)(param_sets[0]) / CONST_15_BITS_OPERATOR;
            if (nVersion == 0x0202)
            {
                param->run.delay = param_sets[1];
                param->run.mode  = param_sets[2];
            }
            else
            {
                param->run.mode  = param_sets[1];
            }

            break;
        }
	case VCA_HIGH_DENSITY:	//人员密度
        {
            param->high_density.alert_density = (float)(param_sets[0]) / CONST_15_BITS_OPERATOR;
            param->high_density.delay = param_sets[1];

            break;
        }
	case VCA_VIOLENT_MOTION://剧烈运动
        {
            param->violent.delay = param_sets[0];
            break;
        }
	// VCA_FLOW_COUNTER and VCA_LEAVE_POSITION add v 2.2
	case VCA_FLOW_COUNTER://客流量
        {
            param->flow_counter.direction.start_point.x = (float)(param_sets[0]) / CONST_15_BITS_OPERATOR;
            param->flow_counter.direction.start_point.y = (float)(param_sets[1]) / CONST_15_BITS_OPERATOR;
            param->flow_counter.direction.end_point.x = (float)(param_sets[2]) / CONST_15_BITS_OPERATOR;
            param->flow_counter.direction.end_point.y = (float)(param_sets[3]) / CONST_15_BITS_OPERATOR;
            break;
        }
	case VCA_LEAVE_POSITION://离岗检测
        {
            param->leave_post.leave_delay  = param_sets[0];
            param->leave_post.static_delay = param_sets[1];
            break;
        }
	case VCA_ENTER_AREA://进入区域
	case VCA_EXIT_AREA://离开区域
	case VCA_HUMAN_ENTER://人员靠近ATM
        {
            break;
        }
	default:
        {
            break;
        }
	}
}

/** @fn     static void get_polygon_bits(IVS_SYS_BUF_MANAGER *bm, VCA_POLYGON_F *polygon)
 *  @brief  获取归一化的多边形信息
 *  @param  bm          [IN]    - 内存管理结构体参数
            polygon     [OUT]   - 归一化的多边形结构体指针
 *  @return 无
 */
static void get_polygon_bits(IVS_SYS_BUF_MANAGER *bm, VCA_POLYGON_F *polygon)
{
	UINT32_T i, temp;

    if (NULL == bm || NULL == polygon)
    {
        return ;
    }

    //多边形顶点数不超过10
	if (polygon->vertex_num > MAX_POLYGON_VERTEX)
	{
		polygon->vertex_num = 0;
		return ;
	}

    for (i = 0; i < polygon->vertex_num; i++)
	{
		temp = IVS_SYS_GetVLCN(bm, 16);
		polygon->point[i].x = (float)(temp & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
		
		temp = IVS_SYS_GetVLCN(bm, 16);
		polygon->point[i].y = (float)(temp & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
	}

}

/** @fn     static void get_rect_normalize_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RECT_F *rect)
 *  @brief  解析码流，从中获取归一化后的边界框结构体数据
 *  @param  bm          [IN]    - 内存管理结构体参数
            rect        [OUT]   - 归一化后的边界框结构体指针
 *  @return 无
 */
static void get_rect_normalize_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RECT_F *rect)
{
	UINT32_T value;//记录解析的码字
	
    if (NULL == bm || NULL == rect)
    {
        return ;
    }

	//边界框左上角X轴坐标
	value = IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK;
	rect->x = (float)value / CONST_15_BITS_OPERATOR;
	
	//边界框左上角Y轴坐标
	value = IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK;
	rect->y = (float)value / CONST_15_BITS_OPERATOR;
	
	//边界框的宽度
	value = IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK;
	rect->width = (float)value / CONST_15_BITS_OPERATOR;
	
	//边界框的高度
	value = IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK;
	rect->height = (float)value / CONST_15_BITS_OPERATOR;
}

/** @fn     static void get_alert_target_bits(IVS_SYS_BUF_MANAGER *bm, VCA_TARGET *target)
 *  @brief  获取简化的目标信息结构体数据(不包含轨迹信息)
 *  @param  bm          [IN]    - 内存管理结构体参数
            target      [OUT]   - 简化的目标信息结构体指针
 *  @return 无
 */
static void get_alert_target_bits(IVS_SYS_BUF_MANAGER *bm, VCA_TARGET *target, unsigned int uVersion)
{
    if (NULL == bm || NULL == target)
    {
        return ;
    }

	// ID
	if (uVersion >= 0x0400)
	{
		target->ID = IVS_SYS_GetVLCN(bm, 8);
		target->ID = (IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) | (target->ID << 24);
	}
	else
	{
		//16bits的目标ID号
		target->ID = IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK;
	}

    //目标类型
	target->type = (BYTE)IVS_SYS_GetVLCN(bm, 8);

    //获取归一化后的边界框结构体数据
	get_rect_normalize_bits(bm, &target->rect);
}

/** @fn     static void get_rule_info_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RULE_INFO *rule_info, UINT16_T uVersion)
 *  @brief  获取简化的规则信息，包含规则的基本信息
 *  @param  bm          [IN]    - 内存管理结构体参数
            rule_info   [OUT]   - 简化规则信息结构体指针
 *  @return 无
 */
static void get_rule_info_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RULE_INFO *rule_info, UINT16_T uVersion)
{
	UINT32_T temp, param_num; //temp临时记录码字的变量；  param_num，参数个数
	INT32_T shift_num;  //EventType

    if (NULL == bm || NULL == rule_info)
    {
        return ;
    }

    //获取8bits的码字
	temp = IVS_SYS_GetVLCN(bm, 8);	
	
	// RuleType
    rule_info->rule_type = (BYTE)(temp >> 6);
	
    // EventType
    shift_num = (temp & 0x3F) - 1;
	
    //警戒类型0-31
	if (shift_num > 31 || shift_num < 0)
	{
		rule_info->event_type = 0;
	}
    else
	{
        rule_info->event_type = 1 << shift_num;
	}
	
    temp = IVS_SYS_GetVLCN(bm, 8);	

	// PointNum
	rule_info->polygon.vertex_num = temp >> 3;
	if (rule_info->polygon.vertex_num > MAX_POLYGON_VERTEX)
	{
        rule_info->polygon.vertex_num = 0;
	}
	
	// ParamNum
    param_num = temp & 7;    
	
	//获取规则参数
    get_rule_param_sets_bits(bm, &(rule_info->param), 
		                     rule_info->event_type, 
							 param_num, uVersion);
	
    // Polygon Point
	get_polygon_bits(bm, &(rule_info->polygon));
	
	// RuleID 触发事件的规则序号
	rule_info->ID = (BYTE)IVS_SYS_GetVLCN(bm, 8);

	if (uVersion >= 0x0401) // 开始支持组合报警
	{
		rule_info->reserved[0] = (BYTE)IVS_SYS_GetVLCN(bm, 8);
		rule_info->reserved[1] = (BYTE)IVS_SYS_GetVLCN(bm, 8);
		rule_info->reserved[2] = (BYTE)IVS_SYS_GetVLCN(bm, 8);
		rule_info->reserved[3] = (BYTE)IVS_SYS_GetVLCN(bm, 8);
		rule_info->reserved[4] = (BYTE)IVS_SYS_GetVLCN(bm, 8);
		rule_info->reserved[5] = (BYTE)IVS_SYS_GetVLCN(bm, 8);
	}
	
	// 老版本解析计数器字段
	if (uVersion == 0)
	{
		VCA_SYS_COUNTER counter;
		get_counter_bits(bm, &counter);
	}
}

/** @fn     HRESULT IVS_EVENT_DATA_sys_parse(VCA_ALERT          *p_event_data,
                                            IVS_SYS_PROC_PARAM *param)
 *  @brief  解析码流，从中获取报警事件结构体数据
 *  @param  p_event_data    [OUT]   - 报警事件结构体指针
            param           [IN]    - 处理参数结构体指针
 *  @return 返回状态码
 */
HRESULT IVS_EVENT_DATA_sys_parse(VCA_ALERT          *p_event_data,
								 IVS_SYS_PROC_PARAM *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存管理结构体
	UINT32_T temp, i, extend_byte_num;  //temp，记录解析的码字；extend_byte_num，扩展字段字节数
	BYTE *pAnaBuffer;       //解析缓存
	UINT16_T uTag = 0;      //版本号高16bits
	UINT16_T uVersion = 0;  //版本号低16bits
    IVS_SYS_PROC_PARAM stParam; //处理参数结构体
    //异常处理
    CHECK_ERROR(p_event_data == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf   == NULL,                  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len   <  4,                     HIK_IVS_SYS_LIB_S_FAIL);

    //对处理参数结构体赋值
    stParam.buf = param->buf;
    stParam.buf_size = param->buf_size;
    stParam.len = param->len;
    stParam.scale_height = param->scale_height;
    stParam.scale_width = param->scale_width;
	
	pAnaBuffer = (BYTE*)param->buf;
	uTag = (pAnaBuffer[0] << 8) + pAnaBuffer[1];	
	if (uTag == 0xffff)
	{
		uVersion= (pAnaBuffer[2] << 8) + pAnaBuffer[3];
		stParam.buf = pAnaBuffer + 4;
		stParam.len -= 4;
	}

	bm = &buf_manager;
    //初始化内存结构体参数
	IVS_PARSE_init_buf_manager(bm, &stParam);

	// 开始解析码流

    temp = IVS_SYS_GetVLCN(bm, 8);
    
	// ViewState
	p_event_data->view_state = (BYTE)(temp>>5);
	
	// ExtendByteNum
	extend_byte_num = temp & 0xF ;
	
    for (i = 0; i < extend_byte_num; i++)
    {
		IVS_SYS_GetVLCN(bm, 8);//跳过保留字节
    }

    get_rule_info_bits(bm, &p_event_data->rule_info, uVersion);

	// 解析目标字段
	get_alert_target_bits(bm, &p_event_data->target, uVersion);
		
	//码流保护
    CHECK_ERROR((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER);
	return HIK_IVS_SYS_LIB_S_OK;
}

/** @fn     static INT32_T get_rule_bits(IVS_SYS_BUF_MANAGER *bm, 
                                            VCA_RULE *rule, 
                                            UINT32_T extend_byte_num,
                                            UINT32_T uVersion)
 *  @brief  解析码流，从中获取警戒规则结构体数据
 *  @param  bm              [IN]    - 内存管理结构体参数
            rule            [OUT]   - 警戒规则数组结构体指针
            extend_byte_num [IN]    - 扩展字节数
 *  @return 返回状态码
 */
static INT32_T get_rule_bits(IVS_SYS_BUF_MANAGER *bm, 
							 VCA_RULE *rule, 
							 UINT32_T extend_byte_num,
							 UINT32_T uVersion)
{
	UINT32_T i, temp, param_num;    //i, 循环计数器；temp，临时记录码字；param_num，参数个数
	INT32_T shift_num = 0;  //事件类型

    CHECK_ERROR(bm == NULL || rule == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);

	temp = IVS_SYS_GetVLCN(bm, 8);

	// RuleType
	rule->rule_type = (BYTE)(temp >> 6);

    // EventType
    shift_num = (temp & 0x3F) - 1;

	if (shift_num > 31 || shift_num < 0)
	{
		rule->event_type = 0;
	}
    else
	{
        rule->event_type = 1 << shift_num;
	}

	temp = IVS_SYS_GetVLCN(bm, 8);	

    // PointNum
	rule->polygon.vertex_num = temp >> 3;
	if (rule->polygon.vertex_num > MAX_POLYGON_VERTEX)
	{
        rule->polygon.vertex_num = 0;
	}
    
	// ParamNum
    param_num = temp & 7;
	
    // Param
    get_rule_param_sets_bits(bm, &(rule->rule_param.param), 
		                     rule->event_type, 
							 param_num,
							 uVersion);

	// RuleID 触发事件的规则序号
	rule->ID = (BYTE)IVS_SYS_GetVLCN(bm, 8);

    // Polygon Point
	get_polygon_bits(bm, &(rule->polygon));
	
    for (i = 0; i < extend_byte_num; i++)
    {
		IVS_SYS_GetVLCN(bm, 8); //跳过保留字节
    }

	return HIK_IVS_SYS_LIB_S_OK;
}

/** @fn     HRESULT IVS_RULE_DATA_sys_parse(VCA_RULE_LIST      *p_rule_data,
                                            IVS_SYS_PROC_PARAM *param)
 *  @brief  输出警戒规则数组结构体数据
 *  @param  p_rule_data     [OUT]   - 警戒规则数组结构体指针
            param           [IN]    - 处理参数结构体指针
 *  @return 返回状态码
 */
HRESULT IVS_RULE_DATA_sys_parse(VCA_RULE_LIST      *p_rule_data,
								IVS_SYS_PROC_PARAM *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存管理结构体
	UINT32_T i, extend_byte_num, temp;      //temp，记录解析的码字；extend_byte_num，扩展字段字节数
	UINT32_T uTag = 0;      //版本号高16bits
	UINT32_T uVersion = 0;  //版本号低16bits
	BYTE *pAnaBuffer;       //解析缓存
    IVS_SYS_PROC_PARAM stParam; //处理参数结构体

    //异常处理
    CHECK_ERROR(p_rule_data == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf  == NULL,                  HIK_IVS_SYS_LIB_E_PARA_NULL);

    //处理参数结构体赋值
    stParam.buf = param->buf;
    stParam.buf_size = param->buf_size;
    stParam.len = param->len;
    stParam.scale_height = param->scale_height;
    stParam.scale_width = param->scale_width;

	if (param->len < 4)
	{
		return HIK_IVS_SYS_LIB_S_FAIL;
	}

	pAnaBuffer = (BYTE*)param->buf;
	uTag = (pAnaBuffer[0]<<8) + pAnaBuffer[1];
//	uVersion= pAnaBuffer[2]<<8 + pAnaBuffer[3];
//	uVersion = 0x0200是新的打包
	if (uTag == 0xffff)
	{
		uVersion = (pAnaBuffer[2] << 8) + pAnaBuffer[3];
		stParam.buf = pAnaBuffer + 4;
		stParam.len -= 4;
	}

	bm = &buf_manager;
    //内存管理结构体参数初始化
	IVS_PARSE_init_buf_manager(bm, &stParam);
 
	// 开始解析码流

	// RuleNum
	p_rule_data->rule_num = IVS_SYS_GetVLCN(bm, 8);
	if (p_rule_data->rule_num > MAX_RULE_NUM)
	{
		p_rule_data->rule_num = 0;
	}

	// PaddingFlag  ExtendByteNum
	temp             = IVS_SYS_GetVLCN(bm, 8);
	extend_byte_num  = temp & 0x7F;

    for (i = 0; i < p_rule_data->rule_num; i++)
    {
        //获取警戒规则结构体数据
        get_rule_bits(bm, &p_rule_data->rule[i], extend_byte_num, uVersion);
    }
     
	//码流保护
    CHECK_ERROR((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size, 
        HIK_IVS_SYS_LIB_E_MEM_OVER);

	return HIK_IVS_SYS_LIB_S_OK;
}

/** @fn     HRESULT IVS_FACE_IDENTIFICATION_sys_parse(HIK_FACE_IDENTIFICATION *face_idt,
                                                        IVS_SYS_PROC_PARAM      *param)
 *  @brief  解析输出人脸识别数据
 *  @param  face_idt    [OUT]   - 人脸识别数据结构体指针
            param       [IN]    - 处理参数结构体指针
 *  @return 返回状态码
 *  @note   没有标识版本的4个字节
 */
HRESULT IVS_FACE_IDENTIFICATION_sys_parse(HIK_FACE_IDENTIFICATION *face_idt,
								          IVS_SYS_PROC_PARAM      *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存结构体参数
	UINT32_T i, extend_byte_num, temp;      //temp，记录解析的码字；extend_byte_num，扩展字段字节数
    UINT32_T data_len = 0;  //人脸检测描述子字节数
	BYTE padding_flag;  //对齐位标记

    //异常处理
	CHECK_ERROR(param == NULL || face_idt  == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(face_idt->encrypt_data_buf == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf                 == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
	
	bm = &buf_manager;
    //内存结构体参数初始化
	IVS_PARSE_init_buf_manager(bm, param);
 
	// 开始解析码流...

#ifdef  HIK_WORDS_BIGENDIAN        //大端模式
	data_len |= IVS_SYS_GetVLCN(bm, 8);
	data_len |= (IVS_SYS_GetVLCN(bm, 8)) << 8;
	data_len |= (IVS_SYS_GetVLCN(bm, 8)) << 16;
	data_len |= (IVS_SYS_GetVLCN(bm, 8)) << 24;
#else
	data_len |= (IVS_SYS_GetVLCN(bm, 8)) << 24;
	data_len |= (IVS_SYS_GetVLCN(bm, 8)) << 16;
	data_len |= (IVS_SYS_GetVLCN(bm, 8)) << 8;
	data_len |= IVS_SYS_GetVLCN(bm, 8);
#endif

	// PaddingFlag  ExtendByteNum
	temp             = IVS_SYS_GetVLCN(bm, 8);
    padding_flag     = (BYTE)(temp >> 7);
	extend_byte_num  = temp & 0x7F;

	if (padding_flag) 
	{
        data_len -= bm->buf_start[data_len - 1]; //去掉填充字节		
	}

    for (i = 0; i < extend_byte_num; i++)
	{
		IVS_SYS_GetVLCN(bm, 8);  //跳过保留字段
	}

	data_len -= 5;  //DataLen(32bits), PaddingFlag(1bit), ExtendByteNum(7bits)
	data_len -= extend_byte_num;

	for (i = 0; i < HIK_CARD_SERIAL_NUM_LEN; i++)  //CardSerialNum(12*8bits)
	{
	    face_idt->card_serial_num[i] = (BYTE)IVS_SYS_GetVLCN(bm, 8);	
	}

	for (i = 0; i < data_len; i++)  //获取人脸识别加密数据
	{
       face_idt->encrypt_data_buf[i] = (BYTE)IVS_SYS_GetVLCN(bm, 8);
	}
     
	//码流保护
    CHECK_ERROR((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size, 
        HIK_IVS_SYS_LIB_E_MEM_OVER);

    return HIK_IVS_SYS_LIB_S_OK;
}

/** @fn     static void get_event_bits(IVS_SYS_BUF_MANAGER *bm, VCA_EVT_INFO *p_evt_info, UINT32_T extend_byte_num, UINT32_T uVersion)
 *  @brief  输出异常事件信息
 *  @param  bm              [IN]    - 内存管理结构体参数
            p_evt_info      [OUT]   - 事件信息结构体参数
            extend_byte_num [IN]    - 扩展字节数
            uVersion        [IN]    - 版本号
 *  @return 无
 */
static void get_event_bits(IVS_SYS_BUF_MANAGER *bm, VCA_EVT_INFO *p_evt_info, UINT32_T extend_byte_num, UINT32_T uVersion)
{
	UINT32_T i; //循环计数器
	UINT32_T event_type;    //事件类型

    if (NULL == bm || NULL == p_evt_info)
    {
        return ;
    }

    //某个时间触发的规则ID
    p_evt_info->rule_id = (BYTE)IVS_SYS_GetVLCN(bm, 8);
    //事件类型
	event_type = (IVS_SYS_GetVLCN(bm, 16) << 16) | IVS_SYS_GetVLCN(bm, 16);
    p_evt_info->event_type = event_type;

	//跳过extend_byte_num2
	for (i = 0; i < extend_byte_num; i++)
	{
		IVS_SYS_GetVLCN(bm, 8);
	}
	
	switch(event_type)
	{
	case VCA_HUMAN_ENTER: //人员靠近ATM
        {
            p_evt_info->event_value.human_in = IVS_SYS_GetVLCN(bm, 8);
            break;
        }
	case VCA_HIGH_DENSITY:  //人员密度
        {
            p_evt_info->event_value.crowd_density = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
            break;
        }
	case VCA_VIOLENT_MOTION://剧烈运动
        {
            //0x0201之前打的是motion_entropy
            if (uVersion == 0)
            {
                p_evt_info->event_value.motion_entropy = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
            }
            else	//0x0201
            {
                p_evt_info->event_value.abrun_info[0]  = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
                p_evt_info->event_value.abrun_info[1]  = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
                p_evt_info->event_value.abrun_info[2]  = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
                p_evt_info->event_value.abrun_info[3]  = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
            }
            break;
        }
	case VCA_RUN:  //奔跑
        {
            //0x0201开始加
            if (uVersion != 0)
            {
                p_evt_info->event_value.abrun_info[0]  = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
                p_evt_info->event_value.abrun_info[1]  = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
                p_evt_info->event_value.abrun_info[2]  = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
                p_evt_info->event_value.abrun_info[3]  = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
            }		
            break;

        }
	case VCA_FLOW_COUNTER:  //客流量
        {
            p_evt_info->event_value.pdc_counter.enter_num = (IVS_SYS_GetVLCN(bm, 16) << 16) | IVS_SYS_GetVLCN(bm, 16);
            p_evt_info->event_value.pdc_counter.leave_num = (IVS_SYS_GetVLCN(bm, 16) << 16) | IVS_SYS_GetVLCN(bm, 16);
            break;
        }
	case VCA_AUDIO_ABNORMAL:  //声音异常检测
        {
            p_evt_info->event_value.audio_entropy = (float)(IVS_SYS_GetVLCN(bm, 16) & CONST_15_BITS_MASK);
            break;
        }
	default:
        {
            break;
        }
	}

}

/** @fn     HRESULT IVS_EVENT_LIST_sys_parse(VCA_EVT_INFO_LIST  *p_evt_list, IVS_SYS_PROC_PARAM *param)
 *  @brief  输出事件列表结构体数据
 *  @param  p_evt_list  [OUT]   - 事件列表结构体指针
            param       [IN]    - 处理参数结构体指针
 *  @return 返回状态码
 */
HRESULT IVS_EVENT_LIST_sys_parse(VCA_EVT_INFO_LIST  *p_evt_list,
							    IVS_SYS_PROC_PARAM  *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存结构体参数
    //extend_byte_num_1对应Evt_Info_List结构体中的保留字节的个数。extend_byte_num_2对应Event结构体中的保留字节的个数
	UINT32_T i, extend_byte_num1, extend_byte_num2, temp;
	UINT32_T enable = 0;    //规则的启用标识
	UINT32_T uTag = 0;      //版本号高16bits
	UINT32_T uVersion = 0;  //版本号低16bits
	BYTE *pAnaBuffer;       //解析缓存
    IVS_SYS_PROC_PARAM stParam; //处理参数结构体

    //异常处理
    CHECK_ERROR(p_evt_list == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf  == NULL,                 HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len   <  4,                   HIK_IVS_SYS_LIB_S_FAIL);

    //处理参数结构体初始化
    stParam.buf = param->buf;
    stParam.buf_size = param->buf_size;
    stParam.len = param->len;
    stParam.scale_height = param->scale_height;
    stParam.scale_width = param->scale_width;

	
	pAnaBuffer = (BYTE*)param->buf;
	uTag = (pAnaBuffer[0]<<8) + pAnaBuffer[1];
	//	uVersion= pAnaBuffer[2]<<8 + pAnaBuffer[3];
	//	uVersion = 0x0200是新的打包
	if (uTag == 0xffff)
	{
		uVersion = (pAnaBuffer[2] << 8) + pAnaBuffer[3];
		stParam.buf = pAnaBuffer + 4;
		stParam.len -= 4;
	}

	bm = &buf_manager;
    //内存结构体初始化
	IVS_PARSE_init_buf_manager(bm, &stParam);
	
	// 开始解析码流
	
	// EventNum
    p_evt_list->event_num = (BYTE)IVS_SYS_GetVLCN(bm, 8);
	if (p_evt_list->event_num > MAX_RULE_NUM)
	{
		p_evt_list->event_num = 0;
	}
	
	// PaddingFlag  ExtendByteNum
	temp             = IVS_SYS_GetVLCN(bm, 8);
	extend_byte_num1 = (temp & 0x78) >> 3;
	extend_byte_num2 = temp & 0x7;

    //获得是否启用规则的标识
	enable = IVS_SYS_GetVLCN(bm, 8);

	//跳过extend_byte_num1
	for (i = 0; i < extend_byte_num1; i++)
	{
		IVS_SYS_GetVLCN(bm, 8);
	}
	
    //遍历 输出异常事件信息
    for (i = 0; i < MAX_RULE_NUM; i++)
    {
        if (enable & (1 << i))
		{
			p_evt_list->event_info[i].enable = TRUE;
			get_event_bits(bm, &p_evt_list->event_info[i], extend_byte_num2, uVersion);	
		}
		else
		{
			p_evt_list->event_info[i].enable = FALSE;
		}
    }
	
	//码流保护
    CHECK_ERROR((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER);
	return HIK_IVS_SYS_LIB_S_OK;
}

/** @fn     HRESULT IVS_FACE_DETECT_RULE_sys_parse(FACE_DETECT_RULE   *p_face_rule,
                                                    IVS_SYS_PROC_PARAM *param)
 *  @brief  解析异常人脸检测规则参数
 *  @param  p_face_rule     [IN]    - 人脸检测规则结构体指针
            param           [OUT]   - 处理参数结构体指针
 *  @return 返回状态码
 *  @note   该接口不再使用
 */
HRESULT IVS_FACE_DETECT_RULE_sys_parse(FACE_DETECT_RULE   *p_face_rule,
							           IVS_SYS_PROC_PARAM *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存结构体参数
    //extend_byte_num_1，extend_byte_num_2 两段保留字节的个数； i，循环计数器； temp,记录解析的码字
	UINT32_T i, extend_byte_num1, extend_byte_num2, temp;
	UINT32_T enable = 0;    //规则是否启用标识
	
    //异常处理
	CHECK_ERROR(p_face_rule == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf  == NULL,                  HIK_IVS_SYS_LIB_E_PARA_NULL);
	
	bm = &buf_manager;
    //内存结构体参数初始化
	IVS_PARSE_init_buf_manager(bm, param);
	
	// 开始解析码流
	if (param->len < 4)
	{
		p_face_rule->enable = FALSE;
		return HIK_IVS_SYS_LIB_S_OK;
	}
	
    //启用人脸检测规则
	p_face_rule->enable = TRUE;

	// PaddingFlag  ExtendByteNum
	temp             = IVS_SYS_GetVLCN(bm, 8);
	extend_byte_num1 = (temp & 0x78) >> 3;
	extend_byte_num2 = temp & 0x7;
	//跳过extend_byte_num1
	for (i = 0; i < extend_byte_num1; i++)
	{
		IVS_SYS_GetVLCN(bm, 8);
	}
	//跳过extend_byte_num2
	for (i = 0; i < extend_byte_num2; i++)
	{
		IVS_SYS_GetVLCN(bm, 8);
	}

	p_face_rule->rule_param.sensitivity = IVS_SYS_GetVLCN(bm, 8);//规则灵敏度参数
	p_face_rule->rule_param.param.abnormal_face.delay = (IVS_SYS_GetVLCN(bm, 16) << 16) | IVS_SYS_GetVLCN(bm, 16);
	
    //尺寸过滤器启用
	enable = IVS_SYS_GetVLCN(bm, 8);
	p_face_rule->rule_param.size_filter.enable = enable;
	if (enable)
	{
		p_face_rule->rule_param.size_filter.mode = IVS_SYS_GetVLCN(bm, 8);
        //获取归一化后的边界框结构体数据
        get_rect_normalize_bits(bm, &(p_face_rule->rule_param.size_filter.min_rect));
        get_rect_normalize_bits(bm, &(p_face_rule->rule_param.size_filter.max_rect));
	}
    //顶点数
	p_face_rule->polygon.vertex_num = IVS_SYS_GetVLCN(bm, 8);
	get_polygon_bits(bm, &(p_face_rule->polygon));//获取归一化的多边形信息
	
	//码流保护
    CHECK_ERROR((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER);
	
	return HIK_IVS_SYS_LIB_S_OK;
}


HRESULT IVS_RULE_DATA_sys_parseEx(HIK_IVS_DATA_INOUT_PACKET* pstdata, IVS_SYS_PROC_PARAM *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存管理结构体
	UINT32_T i, extend_byte_num, temp;      //temp，记录解析的码字；extend_byte_num，扩展字段字节数
	UINT32_T uTag = 0;      //版本号高16bits
	UINT32_T uVersion = 0;  //版本号低16bits
	BYTE *pAnaBuffer;       //解析缓存
	IVS_SYS_PROC_PARAM stParam; //处理参数结构体
	VCA_RULE_LIST* p_rule_data_1 = NULL;
	VCA_RULE_LIST_EX* p_rule_data_2 = NULL;
	VCA_RULE_LIST_V3* p_rule_data_3 = NULL;

	//异常处理
	CHECK_ERROR(pstdata == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf  == NULL||pstdata->pdata == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);

	//处理参数结构体赋值
	stParam.buf = param->buf;
	stParam.buf_size = param->buf_size;
	stParam.len = param->len;
	stParam.scale_height = param->scale_height;
	stParam.scale_width = param->scale_width;

	if (param->len < 4)
	{
		return HIK_IVS_SYS_LIB_S_FAIL;
	}

	pAnaBuffer = (BYTE*)param->buf;
	uTag = (pAnaBuffer[0]<<8) + pAnaBuffer[1];
	//	uVersion= pAnaBuffer[2]<<8 + pAnaBuffer[3];
	//	uVersion = 0x0200是新的打包
	if (uTag == 0xffff)
	{
		uVersion = (pAnaBuffer[2] << 8) + pAnaBuffer[3];
		stParam.buf = pAnaBuffer + 4;
		stParam.len -= 4;
	}

	bm = &buf_manager;
	//内存管理结构体参数初始化
	IVS_PARSE_init_buf_manager(bm, &stParam);

	switch(pstdata->num)
	{
	case 8:
		{
			p_rule_data_1 = (VCA_RULE_LIST*)pstdata->pdata;
			// RuleNum
			p_rule_data_1->rule_num = IVS_SYS_GetVLCN(bm, 8);
			if (uVersion >= 0x0400)
			{
				pstdata->algo_type = p_rule_data_1->rule_num;
				p_rule_data_1->rule_num = IVS_SYS_GetVLCN(bm, 8) | (p_rule_data_1->rule_num << 24);
			}

			if ((p_rule_data_1->rule_num & 0xffffff) > MAX_RULE_NUM)
			{
				p_rule_data_1->rule_num = 0;
			}
			// PaddingFlag  ExtendByteNum
			temp             = IVS_SYS_GetVLCN(bm, 8);
			extend_byte_num  = temp & 0x7F;

			for (i = 0; i < (p_rule_data_1->rule_num & 0xffffff); i++)
			{
				//获取警戒规则结构体数据
				get_rule_bits(bm, &p_rule_data_1->rule[i], extend_byte_num, uVersion);
			}

			break;
		}
	case 16:
		{
			p_rule_data_2 = (VCA_RULE_LIST_EX*)pstdata->pdata;
			// RuleNum
			p_rule_data_2->rule_num = IVS_SYS_GetVLCN(bm, 8);
			if (uVersion >= 0x0400)
			{
				pstdata->algo_type = p_rule_data_2->rule_num;
				p_rule_data_2->rule_num = IVS_SYS_GetVLCN(bm, 8) | (p_rule_data_2->rule_num << 24);
			}

			if ((p_rule_data_2->rule_num & 0xffffff) > MAX_RULE_NUM_EX)
			{
				p_rule_data_2->rule_num = 0;
			}
			
			// PaddingFlag  ExtendByteNum
			temp             = IVS_SYS_GetVLCN(bm, 8);
			extend_byte_num  = temp & 0x7F;

			for (i = 0; i < (p_rule_data_2->rule_num & 0xffffff); i++)
			{
				//获取警戒规则结构体数据
				get_rule_bits(bm, &p_rule_data_2->rule[i], extend_byte_num, uVersion);
			}

			break;
		}
	case 64:
		{
			p_rule_data_3 = (VCA_RULE_LIST_V3*)pstdata->pdata;
			// RuleNum
			p_rule_data_3->rule_num = IVS_SYS_GetVLCN(bm, 8);
			if (uVersion >= 0x0400)
			{
				pstdata->algo_type = p_rule_data_3->rule_num;
				p_rule_data_3->rule_num = IVS_SYS_GetVLCN(bm, 8) | (p_rule_data_3->rule_num << 24);
			}

			if ((p_rule_data_3->rule_num & 0xffffff) > MAX_RULE_NUM_V3)
			{
				p_rule_data_3->rule_num = 0;
			}

			// PaddingFlag  ExtendByteNum
			temp             = IVS_SYS_GetVLCN(bm, 8);
			extend_byte_num  = temp & 0x7F;

			for (i = 0; i < (p_rule_data_3->rule_num & 0xffffff); i++)
			{
				//获取警戒规则结构体数据
				get_rule_bits(bm, &p_rule_data_3->rule[i], extend_byte_num, uVersion);
			}

			break;
		}
	}

	//码流保护
    CHECK_ERROR((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size, 
        HIK_IVS_SYS_LIB_E_MEM_OVER);

	return HIK_IVS_SYS_LIB_S_OK;
}

HRESULT IVS_EVENT_LIST_sys_parseEx(HIK_IVS_DATA_INOUT_PACKET* pstdata, IVS_SYS_PROC_PARAM *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存结构体参数
	//extend_byte_num_1对应Evt_Info_List结构体中的保留字节的个数。extend_byte_num_2对应Event结构体中的保留字节的个数
	UINT32_T i, extend_byte_num1, extend_byte_num2, temp, j;
	UINT32_T enable = 0;    //规则的启用标识
	UINT32_T enable_1[8];    //规则的启用标识
	UINT32_T uTag = 0;      //版本号高16bits
	UINT32_T uVersion = 0;  //版本号低16bits
	BYTE *pAnaBuffer;       //解析缓存
	IVS_SYS_PROC_PARAM stParam; //处理参数结构体
	VCA_EVT_INFO_LIST* p_evt_list_1 = NULL;
	VCA_EVT_INFO_LIST_EX* p_evt_list_2 = NULL;
	VCA_EVT_INFO_LIST_V3* p_evt_list_3 = NULL;

	//异常处理
	CHECK_ERROR(pstdata == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf  == NULL,              HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len   <  4,                HIK_IVS_SYS_LIB_S_FAIL);

	//处理参数结构体初始化
	stParam.buf = param->buf;
	stParam.buf_size = param->buf_size;
	stParam.len = param->len;
	stParam.scale_height = param->scale_height;
	stParam.scale_width = param->scale_width;

	pAnaBuffer = (BYTE*)param->buf;
	uTag = (pAnaBuffer[0]<<8) + pAnaBuffer[1];
	//	uVersion= pAnaBuffer[2]<<8 + pAnaBuffer[3];
	//	uVersion = 0x0200是新的打包
	if (uTag == 0xffff)
	{
		uVersion = (pAnaBuffer[2] << 8) + pAnaBuffer[3];
		stParam.buf = pAnaBuffer + 4;
		stParam.len -= 4;
	}

	bm = &buf_manager;
	//内存结构体初始化
	IVS_PARSE_init_buf_manager(bm, &stParam);

	// 开始解析码流
	switch(pstdata->num)
	{
	case 8:
		{
			p_evt_list_1 = (VCA_EVT_INFO_LIST*)pstdata->pdata;
			CHECK_ERROR(p_evt_list_1  == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
			// EventNum
			p_evt_list_1->event_num = (BYTE)IVS_SYS_GetVLCN(bm, 8);
			if (p_evt_list_1->event_num > MAX_RULE_NUM)
			{
				p_evt_list_1->event_num = 0;
			}
			break;
		}
	case 16:
		{
			p_evt_list_2 = (VCA_EVT_INFO_LIST_EX*)pstdata->pdata;
			CHECK_ERROR(p_evt_list_2  == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
			// EventNum
			p_evt_list_2->event_num = (BYTE)IVS_SYS_GetVLCN(bm, 8);
			if (p_evt_list_2->event_num > MAX_RULE_NUM_EX)
			{
				p_evt_list_2->event_num = 0;
			}
			break;
		}
	case 64:
		{
			p_evt_list_3 = (VCA_EVT_INFO_LIST_V3*)pstdata->pdata;
			CHECK_ERROR(p_evt_list_3  == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
			// EventNum
			p_evt_list_3->event_num = (BYTE)IVS_SYS_GetVLCN(bm, 8);
			if (p_evt_list_3->event_num > MAX_RULE_NUM_V3)
			{
				p_evt_list_3->event_num = 0;
			}
			break;
		}
	default:
		{
			return HIK_IVS_SYS_LIB_S_FAIL;
		}
	}

	// PaddingFlag  ExtendByteNum
	temp             = IVS_SYS_GetVLCN(bm, 8);
	extend_byte_num1 = (temp & 0x78) >> 3;
	extend_byte_num2 = temp & 0x7;

	//获得是否启用规则的标识
	for (i = 0; i < 8; i++)
	{
		enable_1[i] = IVS_SYS_GetVLCN(bm, 8);
	}

	//跳过extend_byte_num1
	for (i = 0; i < extend_byte_num1; i++)
	{
		IVS_SYS_GetVLCN(bm, 8);
	}

	switch(pstdata->num)
	{
	case 8:
		{
			//遍历 输出异常事件信息
            enable = enable_1[0];       
			for (i = 0; i < MAX_RULE_NUM; i++)
			{
				if (enable & (1 << i))
				{
					p_evt_list_1->event_info[i].enable = TRUE;
					get_event_bits(bm, &p_evt_list_1->event_info[i], extend_byte_num2, uVersion);	
				}
				else
				{
					p_evt_list_1->event_info[i].enable = FALSE;
				}
			}
			break;
		}
	case 16:
		{
			//遍历 输出异常事件信息
			for (i = 0; i < MAX_RULE_NUM_EX; i++)
			{
				enable = enable_1[(i / 8)];
				if (enable & (1 << i))
				{
					p_evt_list_2->event_info[i].enable = TRUE;
					get_event_bits(bm, &p_evt_list_2->event_info[i], extend_byte_num2, uVersion);	
				}
				else
				{
					p_evt_list_2->event_info[i].enable = FALSE;
				}
			}
			break;
		}
	case 64:
		{
			//遍历 输出异常事件信息
			for (i = 0; i < MAX_RULE_NUM_V3; i++)
			{
				j = (i / 8);
				enable = enable_1[j];
				if (enable & (1 << (i - j*8)))
				{
					p_evt_list_3->event_info[i].enable = TRUE;
					get_event_bits(bm, &p_evt_list_3->event_info[i], extend_byte_num2, uVersion);	
				}
				else
				{
					p_evt_list_3->event_info[i].enable = FALSE;
				}
			}
			break;
		}
	}

	//码流保护
	CHECK_ERROR((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size, HIK_IVS_SYS_LIB_E_MEM_OVER);

	return HIK_IVS_SYS_LIB_S_OK;
}

HRESULT IVS_DATA_sys_parse_old(unsigned char *pdata,IVS_SYS_PROC_PARAM *param, unsigned int nType, unsigned char* alog_type)
{
	int ret = 0;

	switch(nType)
	{
	case IVS_TYPE_META_DATA:
		{
			VCA_TARGET_LIST* out_data = (VCA_TARGET_LIST*)pdata;

			ret = IVS_META_DATA_sys_parse(out_data, param);
			if (HIK_IVS_SYS_LIB_S_OK != ret)
			{
				return ret;
			}

			if (out_data->target_num)
			{
				*alog_type = (out_data->target[0].ID >> 24);
			}

			return HIK_IVS_SYS_LIB_S_OK;
		}
	case IVS_TYPE_EVENT_DATA:
		{
			VCA_ALERT* out_data = (VCA_ALERT*)pdata;

			ret = IVS_EVENT_DATA_sys_parse(out_data, param);
			if (HIK_IVS_SYS_LIB_S_OK != ret)
			{
				return ret;
			}

			*alog_type = (out_data->target.ID >> 24);

			return HIK_IVS_SYS_LIB_S_OK;
		}
	case IVS_TYPE_RULE_DATA:
		{
			return IVS_RULE_DATA_sys_parse((VCA_RULE_LIST*)pdata, param);
		}
	case IVS_TYPE_EVENT_LIST:
		{
			return IVS_EVENT_LIST_sys_parse((VCA_EVT_INFO_LIST*)pdata, param);
		}
	case IVS_TYPE_FACE_IDENTIFICATION:
		{
			return IVS_FACE_IDENTIFICATION_sys_parse((HIK_FACE_IDENTIFICATION*)pdata, param);
		}
	case IVS_FACE_DETECT_RULE:
		{
			return IVS_FACE_DETECT_RULE_sys_parse((FACE_DETECT_RULE*)pdata, param);
		}
	default:
		{
			break;
		}
	}

	return HIK_IVS_SYS_LIB_S_FAIL;
}

//解析新接口
HRESULT IVS_DATA_sys_parse(unsigned char *pstdata,IVS_SYS_PROC_PARAM *param, unsigned int nType)
{
	HIK_IVS_DATA_INOUT_PACKET* stParam = (HIK_IVS_DATA_INOUT_PACKET*)pstdata;
	unsigned char* pData = NULL;
	int ret = 0;

	CHECK_ERROR(param == NULL || pstdata == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf == NULL,                HIK_IVS_SYS_LIB_E_PARA_NULL);

	pData = param->buf;
	if (param->len < 8)
	{
		return HIK_IVS_SYS_LIB_S_FAIL;
	}

	stParam->version   = (pData[0] << 8) + pData[1];
	stParam->num       = (pData[2] << 8) + pData[3];
	stParam->type      = (pData[4] << 8) + pData[5];
	stParam->datalen   = (pData[6] << 8) + pData[7];
	stParam->algo_type = 0;

	if (0x0101 != stParam->version
	  &&0x0102 != stParam->version)
	{
		VCA_TARGET_LIST* out_data = (VCA_TARGET_LIST*)stParam->pdata;

		stParam->version   = INOUT_PACKET_VERSION;
		stParam->num       = 8;
		stParam->type      = nType;
		stParam->algo_type = 0;

		return IVS_DATA_sys_parse_old((unsigned char *)out_data, param, nType, &stParam->algo_type);
	}

	param->buf = (unsigned char*)param->buf + 8;
	param->len -= 8;

	switch(stParam->type)
	{
	case IVS_TYPE_META_DATA:
		{
			VCA_TARGET_LIST* out_data = (VCA_TARGET_LIST*)stParam->pdata;
			ret = IVS_META_DATA_sys_parse(out_data, param);
			if (HIK_IVS_SYS_LIB_S_OK != ret)
			{
				return ret;
			}

			if (out_data->target_num)
			{
				stParam->algo_type = (out_data->target[0].ID >> 24);
			}

			return HIK_IVS_SYS_LIB_S_OK;
		}
	case IVS_TYPE_EVENT_DATA:
		{
			VCA_ALERT* out_data = (VCA_ALERT*)stParam->pdata;
			ret = IVS_EVENT_DATA_sys_parse(out_data, param);
			if (HIK_IVS_SYS_LIB_S_OK != ret)
			{
				return ret;
			}

			stParam->algo_type = (out_data->target.ID >> 24);

			return HIK_IVS_SYS_LIB_S_OK;
		}
	case IVS_TYPE_RULE_DATA:
		{
			return IVS_RULE_DATA_sys_parseEx(stParam, param);
		}
	case IVS_TYPE_EVENT_LIST:
		{
			return IVS_EVENT_LIST_sys_parseEx(stParam, param);
		}
	case IVS_TYPE_FACE_IDENTIFICATION:
		{
			return HIK_IVS_SYS_LIB_S_FAIL;
			//return IVS_FACE_IDENTIFICATION_sys_parse((HIK_FACE_IDENTIFICATION*)stParam.pdata, param);
		}
	case IVS_FACE_DETECT_RULE:
		{
			return HIK_IVS_SYS_LIB_S_FAIL;
			//return IVS_FACE_DETECT_RULE_sys_parse((FACE_DETECT_RULE*)stParam.pdata, param);
		}
	default:
		{
			break;
		}
	}

	return HIK_IVS_SYS_LIB_S_FAIL;
}


/** @fn     HRESULT IVS_META_DATA_sys_parse_v2(VCA_TARGET_LIST_V2     *p_meta_data,
                                            IVS_SYS_PROC_PARAM  *param)
 *  @brief  解析码流，从中获取帧内目标信息数组数据
 *  @param  p_meta_data     [OUT]   - 目标信息结构体
            param           [IN]    - 处理参数结构体
 *  @return 返回状态码
 */
HRESULT IVS_META_DATA_sys_parse_v2(VCA_TARGET_LIST_V2         *p_meta_data,
						  	    IVS_SYS_PROC_PARAM  *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存管理结构体
    //extend_byte_num_0对应MetaData结构体中的保留字节的个数。extend_byte_num_1对应Target结构体中的保留字节的个数
	INT32_T i, extend_byte_num_0, extend_byte_num_1;    
	UINT32_T value;     //临时记录解析得到的码字
	BYTE *pAnaBuffer;   //输入数据的缓冲
	UINT16_T uTag = 0;      //版本号的高16位
	UINT16_T uVersion = 0;  //版本号的低16位
    IVS_SYS_PROC_PARAM stParam;
	UINT16_T   uExtenMark = 0;

    CHECK_ERROR(p_meta_data == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf  == NULL,                  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len   <  4,                    HIK_IVS_SYS_LIB_S_FAIL);

    stParam.buf = param->buf;
    stParam.buf_size = param->buf_size;
    stParam.len = param->len;
    stParam.scale_height = param->scale_height;
    stParam.scale_width = param->scale_width;

	pAnaBuffer = (BYTE*)param->buf;
    //版本号标识，版本号
	uTag = (pAnaBuffer[0] << 8) + pAnaBuffer[1];	
	if (uTag == 0xffff)
	{
		uVersion= (pAnaBuffer[2] << 8) + pAnaBuffer[3];
		//param->buf = pAnaBuffer + 4;
		//param->len -= 4;
        stParam.buf = pAnaBuffer + 4;
        stParam.len -= 4;
		uExtenMark = uVersion;
		if  (uExtenMark != IVS_EXTEND_TARGET_NUM_MARK)
		{
			return HIK_IVS_SYS_LIB_S_FAIL;
		}
	}

	bm = &buf_manager;
	IVS_PARSE_init_buf_manager(bm, &stParam);   //初始化内存管理参数
		
	p_meta_data->target_num = (BYTE)IVS_SYS_GetVLCN(bm, 8); //获得目标数

    //最大目标数不超过30
    if (p_meta_data->target_num > MAX_TARGET_NUM_V2)
    {
		p_meta_data->target_num = 0;
    }
	
    //解析得到8个bits
    value = IVS_SYS_GetVLCN(bm, 8);

	extend_byte_num_0 = (value >> 4) & 0x7; //MetaData结构体中的保留字节的个数
	extend_byte_num_1 = value & 0xF;        //Target结构体中的保留字节的个数

    for (i = 0; i < extend_byte_num_0; i++)
    {
        IVS_SYS_GetVLCN(bm, 8);//跳过保留字节
    }

	//宽度方向上的缩放比例 
	value = IVS_SYS_GetVLCN(bm, 16);
	bm->scale_width = (UINT16_T)(value & CONST_15_BITS_MASK);

	//高度方向上的缩放比例 
	value = IVS_SYS_GetVLCN(bm, 16);
	bm->scale_height = (UINT16_T)(value & CONST_15_BITS_MASK);

	for (i = 0; i < p_meta_data->target_num; i++)
	{
		get_target_bits(bm, &(p_meta_data->target[i]), extend_byte_num_1, uVersion);    //获取目标数据
	}

	//码流保护
    CHECK_ERROR((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size, 
        HIK_IVS_SYS_LIB_E_MEM_OVER);

    return HIK_IVS_SYS_LIB_S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/** @fn     HRESULT IVS_META_DATA_sys_parse_com(HIK_VCA_TARGET_LIST_COM         *p_meta_data,
						  	    IVS_SYS_PROC_PARAM  *param)
 *  @brief  解析码流，从中获取帧内目标信息数组数据
 *  @param  p_meta_data     [OUT]   - 目标信息结构体
            param           [IN]    - 处理参数结构体
 *  @return 返回状态码
 */
HRESULT IVS_META_DATA_sys_parse_com(HIK_VCA_TARGET_LIST_COM         *p_meta_data,
						  	    IVS_SYS_PROC_PARAM  *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存管理结构体
    //extend_byte_num_0对应MetaData结构体中的保留字节的个数。extend_byte_num_1对应Target结构体中的保留字节的个数
	INT32_T i, extend_byte_num_0, extend_byte_num_1;    
	UINT32_T value;     //临时记录解析得到的码字
	BYTE *pAnaBuffer;   //输入数据的缓冲
	UINT16_T uTag = 0;      //版本号的高16位
	UINT16_T uVersion = 0;  //版本号的低16位
    IVS_SYS_PROC_PARAM stParam;
	UINT16_T   uExtenMark = 0;
	HRESULT hr = 0;

    CHECK_ERROR(p_meta_data == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf  == NULL,                  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(p_meta_data->p_target  == NULL,       HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len   <  4,                    HIK_IVS_SYS_LIB_S_FAIL);


    stParam.buf = param->buf;
    stParam.buf_size = param->buf_size;
    stParam.len = param->len;
    stParam.scale_height = param->scale_height;
    stParam.scale_width = param->scale_width;

	pAnaBuffer = (BYTE*)param->buf;
    //版本号标识，版本号
	uTag = (pAnaBuffer[0] << 8) + pAnaBuffer[1];	
	if (uTag == 0xffff)
	{
		uVersion = (pAnaBuffer[2] << 8) + pAnaBuffer[3];
		if((uVersion != IVS_EXTEND_TYPE_MARK) 
			&& (uVersion != IVS_EXTEND_TARGET_NUM_MARK)
			&& (uVersion >  IVS_PACK_VERSION)
            && (uVersion != IVS_EXTEN_TYPE_COLOR)
            && (uVersion != IVS_EXTEN_TYPE_RESVERD_ALL))
		{
			return HIK_IVS_SYS_LIB_S_FAIL;
		}

		if (uVersion <= IVS_PACK_VERSION
			&& (uVersion != IVS_EXTEND_TYPE_MARK)
			&& (uVersion != IVS_EXTEND_TARGET_NUM_MARK)
            && (uVersion != IVS_EXTEN_TYPE_COLOR)
            && (uVersion != IVS_EXTEN_TYPE_RESVERD_ALL))
		{
			VCA_TARGET_LIST stList = { 0 };
			hr = IVS_META_DATA_sys_parse(&stList, param);
			if (HIK_IVS_SYS_LIB_S_OK!= hr)
			{
				return hr;
			}

			p_meta_data->target_num = stList.target_num;
			p_meta_data->type = SHOW_TYPE_NORMAL | MATCH_TYPE_NORMAL;


			//最大目标数不超过30
			if (p_meta_data->target_num > MAX_TARGET_NUM)
			{
				p_meta_data->target_num = 0;
			}

			for (i = 0; i < p_meta_data->target_num; i++ )
			{
				memcpy(&p_meta_data->p_target[i], &stList.target[i], sizeof(VCA_TARGET));
			}

			return HIK_IVS_SYS_LIB_S_OK;
		}
		else if (uVersion == IVS_EXTEND_TARGET_NUM_MARK)
		{
			VCA_TARGET_LIST_V2 stList = { 0 };
			hr = IVS_META_DATA_sys_parse_v2(&stList, param);
			if (HIK_IVS_SYS_LIB_S_OK!= hr)
			{
				return hr;
			}

			p_meta_data->target_num = stList.target_num;
			p_meta_data->type = SHOW_TYPE_NORMAL | MATCH_TYPE_NORMAL;


			//最大目标数不超过160
			if (p_meta_data->target_num > MAX_TARGET_NUM_V2)
			{
				p_meta_data->target_num = 0;
			}

			for (i = 0; i < p_meta_data->target_num; i++ )
			{
				memcpy(&p_meta_data->p_target[i], &stList.target[i], sizeof(VCA_TARGET));
			}
			return HIK_IVS_SYS_LIB_S_OK;

		}

		stParam.buf = pAnaBuffer + 4;
		stParam.len -= 4;
		uExtenMark = uVersion;
	}

	bm = &buf_manager;
	IVS_PARSE_init_buf_manager(bm, &stParam);   //初始化内存管理参数
	
	if((uTag == 0xffff) && 
       (uExtenMark == IVS_EXTEND_TYPE_MARK || 
        uExtenMark == IVS_EXTEN_TYPE_COLOR || 
        uExtenMark == IVS_EXTEN_TYPE_RESVERD_ALL))
	{
		p_meta_data->type = IVS_SYS_GetVLCN(bm, 32);
		p_meta_data->target_num = (BYTE)IVS_SYS_GetVLCN(bm, 24); //获得目标数
		if (p_meta_data->target_num > MAX_TARGET_NUM_V2)
		{
			p_meta_data->target_num = 0;
		}
	}
	else
	{
		p_meta_data->target_num = (BYTE)IVS_SYS_GetVLCN(bm, 8); //获得目标数
		//最大目标数不超过30
		if (p_meta_data->target_num > MAX_TARGET_NUM)
		{
			p_meta_data->target_num = 0;
		}
	}

    //解析得到8个bits
    value = IVS_SYS_GetVLCN(bm, 8);

	extend_byte_num_0 = (value >> 4) & 0x7; //MetaData结构体中的保留字节的个数
	extend_byte_num_1 = value & 0xF;        //Target结构体中的保留字节的个数

    for (i = 0; i < extend_byte_num_0; i++)
    {
        IVS_SYS_GetVLCN(bm, 8);//跳过保留字节
    }

	//宽度方向上的缩放比例 
	value = IVS_SYS_GetVLCN(bm, 16);
	bm->scale_width = (UINT16_T)(value & CONST_15_BITS_MASK);

	//高度方向上的缩放比例 
	value = IVS_SYS_GetVLCN(bm, 16);
	bm->scale_height = (UINT16_T)(value & CONST_15_BITS_MASK);

	for (i = 0; i < p_meta_data->target_num; i++)
	{
		get_target_bits(bm, &(p_meta_data->p_target[i]), extend_byte_num_1, uVersion);    //获取目标数据
	}

	//码流保护
    CHECK_ERROR((((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size) || bm->over_flag), HIK_IVS_SYS_LIB_E_MEM_OVER);

    return HIK_IVS_SYS_LIB_S_OK;
}


/** @fn     HRESULT IVS_RULE_DATA_sys_parse_com(HIK_VCA_RULE_LIST_COM      *p_rule_data, IVS_SYS_PROC_PARAM *param)
 *  @brief  解析码流，从中获取帧内规则信息数组数据
 *  @param  p_rule_data     [OUT]   - 规则信息结构体
            param           [IN]    - 处理参数结构体
 *  @return 返回状态码
 */
HRESULT IVS_RULE_DATA_sys_parse_com(HIK_VCA_RULE_LIST_COM      *p_rule_data,
								IVS_SYS_PROC_PARAM *param)
{
	IVS_SYS_BUF_MANAGER buf_manager, *bm;   //内存管理结构体
	UINT32_T i, extend_byte_num, temp = 0;      //temp，记录解析的码字；extend_byte_num，扩展字段字节数
	UINT32_T uTag = 0;      //版本号高16bits
	UINT32_T uVersion = 0;  //版本号低16bits
	BYTE *pAnaBuffer;       //解析缓存
	IVS_SYS_PROC_PARAM stParam; //处理参数结构体
	UINT16_T   uExtenMark = 0;
	HRESULT hr = 0;
	HIK_IVS_DATA_INOUT_PACKET stInOutBuf = { 0 }; 
	VCA_RULE_LIST_V3 rule_list_v3 = {0};
	//异常处理
	CHECK_ERROR(p_rule_data == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(param->buf  == NULL,                  HIK_IVS_SYS_LIB_E_PARA_NULL);
	CHECK_ERROR(p_rule_data->p_rule  == NULL,         HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len   <  4,                     HIK_IVS_SYS_LIB_S_FAIL);

	//处理参数结构体赋值
	stParam.buf = param->buf;
	stParam.buf_size = param->buf_size;
	stParam.len = param->len;
	stParam.scale_height = param->scale_height;
	stParam.scale_width = param->scale_width;

	pAnaBuffer = (BYTE*)param->buf;
	uTag = (pAnaBuffer[0]<<8) + pAnaBuffer[1];
	
	if (uTag ==  0x0101 ||uTag == 0x0102)
	{
        if (param->len < 8)
        {
            return HIK_IVS_SYS_LIB_S_FAIL;
        }
		stInOutBuf.version   = (pAnaBuffer[0] << 8) + pAnaBuffer[1];
		stInOutBuf.num       = (pAnaBuffer[2] << 8) + pAnaBuffer[3];
		stInOutBuf.type      = (pAnaBuffer[4] << 8) + pAnaBuffer[5];
		stInOutBuf.datalen   = (pAnaBuffer[6] << 8) + pAnaBuffer[7];
		stInOutBuf.algo_type = 0;
		stInOutBuf.pdata = (unsigned char *)&rule_list_v3;
	
		param->buf = pAnaBuffer + 8;
		param->len -= 8;
		
		hr = IVS_RULE_DATA_sys_parseEx(&stInOutBuf, param);
		if (hr != HIK_IVS_SYS_LIB_S_OK)
		{
			return hr;
		}
        
        if (rule_list_v3.rule_num > 64)
        {
            rule_list_v3.rule_num = 0;
        }

		p_rule_data->rule_num = rule_list_v3.rule_num;
		p_rule_data->type = SHOW_TYPE_NORMAL | MATCH_TYPE_NORMAL;

		for (i = 0; i < rule_list_v3.rule_num; i++)
		{
			memcpy(&p_rule_data->p_rule[i], &rule_list_v3.rule[i], sizeof(VCA_RULE));
		}

		return hr;
	}

	//	uVersion= pAnaBuffer[2]<<8 + pAnaBuffer[3];
	//	uVersion = 0x0200是新的打包
	if (uTag == 0xffff)
	{
		uVersion = (pAnaBuffer[2] << 8) + pAnaBuffer[3];
		uExtenMark = uVersion;

		if((uExtenMark != IVS_EXTEND_TYPE_MARK)
			&& (uExtenMark != IVS_EXTEN_TYPE_COLOR)
            && (uExtenMark != IVS_EXTEN_TYPE_RESVERD_ALL)
			&& (uExtenMark > IVS_PACK_VERSION))
		{
			return HIK_IVS_SYS_LIB_S_FAIL;
		}

		if ((uExtenMark <= IVS_PACK_VERSION)
			&& (uExtenMark != IVS_EXTEND_TYPE_MARK)
            && (uExtenMark != IVS_EXTEN_TYPE_RESVERD_ALL)
			&& (uExtenMark != IVS_EXTEN_TYPE_COLOR))
		{
			VCA_RULE_LIST rule_list = { 0 };
			hr = IVS_RULE_DATA_sys_parse(&rule_list, param);
			if (hr != HIK_IVS_SYS_LIB_S_OK)
			{
				return hr;
			}

			p_rule_data->rule_num = rule_list.rule_num;
			if (p_rule_data->rule_num > MAX_RULE_NUM)
			{
				p_rule_data->rule_num = 0;
			}

			p_rule_data->type = SHOW_TYPE_NORMAL | MATCH_TYPE_NORMAL;
			for (i = 0; i < p_rule_data->rule_num; i++ )
			{
				memcpy(&p_rule_data->p_rule[i], &rule_list.rule[i], sizeof(VCA_RULE));
			}
			return hr;
		}

		stParam.buf = pAnaBuffer + 4;
		stParam.len -= 4;
	}
	

	bm = &buf_manager;
	//内存管理结构体参数初始化
	IVS_PARSE_init_buf_manager(bm, &stParam);

	// 开始解析码流
	if(uExtenMark  == IVS_EXTEND_TYPE_MARK ||
        uExtenMark == IVS_EXTEN_TYPE_COLOR ||
        uExtenMark == IVS_EXTEN_TYPE_RESVERD_ALL)
	{
		p_rule_data->type = IVS_SYS_GetVLCN(bm, 32);
 		p_rule_data->rule_num = IVS_SYS_GetVLCN(bm, 24);
		if (p_rule_data->rule_num > MAX_RULE_NUM_V3)
		{
			p_rule_data->rule_num = 0;
		}
	}
	else 
	{
		// RuleNum
		p_rule_data->rule_num = IVS_SYS_GetVLCN(bm, 8);
		if (p_rule_data->rule_num > MAX_RULE_NUM)
		{
			p_rule_data->rule_num = 0;
		}
	}

	// PaddingFlag  ExtendByteNum
	temp             = IVS_SYS_GetVLCN(bm, 8);
	extend_byte_num  = temp & 0x7F;

	for (i = 0; i < p_rule_data->rule_num; i++)
	{
		//获取警戒规则结构体数据
		get_rule_bits(bm, &p_rule_data->p_rule[i], extend_byte_num, uVersion);
		
		//支持颜色获取方案reserved[0]= 0xdb，表示是颜色版本，
		//然后reserved[1]，reserved[2]，reserved[3]，表示RGB
		if (uExtenMark == IVS_EXTEN_TYPE_COLOR)
		{   
			p_rule_data->p_rule[i].reserved[0] = IVS_SYS_GetVLCN(bm, 8);
		    p_rule_data->p_rule[i].reserved[1] = IVS_SYS_GetVLCN(bm, 8);
			p_rule_data->p_rule[i].reserved[2] = IVS_SYS_GetVLCN(bm, 8);
            p_rule_data->p_rule[i].reserved[3] = IVS_SYS_GetVLCN(bm, 8);
		}
        else if (uExtenMark == IVS_EXTEN_TYPE_RESVERD_ALL)
        {
            p_rule_data->p_rule[i].reserved[0] = IVS_SYS_GetVLCN(bm, 8);
            p_rule_data->p_rule[i].reserved[1] = IVS_SYS_GetVLCN(bm, 8);
            p_rule_data->p_rule[i].reserved[2] = IVS_SYS_GetVLCN(bm, 8);
            p_rule_data->p_rule[i].reserved[3] = IVS_SYS_GetVLCN(bm, 8);
            p_rule_data->p_rule[i].reserved[4] = IVS_SYS_GetVLCN(bm, 8);
        }
	}

	//码流保护
    CHECK_ERROR((((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size) || bm->over_flag), HIK_IVS_SYS_LIB_E_MEM_OVER);

	return HIK_IVS_SYS_LIB_S_OK;
}


/***************************************************
多算法库支持函数
****************************************************/

/******************************************************************************
* 功  能: 颜色信息解压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         color    - 颜色信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_color_bits(IVS_SYS_BUF_MANAGER *bm,IS_PRIVT_INFO_COLOR *color)
{
    if (NULL == bm || NULL == color)
    {
        return;
    }
    memset(color, 0, sizeof(IS_PRIVT_INFO_COLOR));
    color->red   = IVS_SYS_GetVLCN(bm, 8);
    color->green = IVS_SYS_GetVLCN(bm, 8);
    color->blue  = IVS_SYS_GetVLCN(bm, 8);
    color->alpha = IVS_SYS_GetVLCN(bm, 8);
}


/******************************************************************************
* 功  能: 速度信息解压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         speed    - 速度信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_speed_coord_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_SPECOORD *speed)
{
    if (NULL == bm || NULL == speed)
    {
        return;
    }
    memset(speed, 0, sizeof(IS_PRIVT_INFO_SPECOORD));
    speed->x       = IVS_SYS_GetVLCN(bm, 8);
    speed->y       = IVS_SYS_GetVLCN(bm, 8);
    speed->speed   = IVS_SYS_GetVLCN(bm, 8);
    speed->id      = IVS_SYS_GetVLCN(bm, 8);
    speed->flag    = IVS_SYS_GetVLCN(bm, 4);
}


/******************************************************************************
* 功  能: 轨迹信息解压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         line     - 轨迹信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_color_line_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_COLORL *line)
{
    if (NULL == bm || NULL == line)
    {
        return;
    }
    memset(line, 0, sizeof(IS_PRIVT_INFO_COLORL));

    line->color.red   = IVS_SYS_GetVLCN(bm, 8);
    line->color.green = IVS_SYS_GetVLCN(bm, 8);
    line->color.blue  = IVS_SYS_GetVLCN(bm, 8);
    line->color.alpha = IVS_SYS_GetVLCN(bm, 8);
    line->line_time   = IVS_SYS_GetVLCN(bm, 8);
    line->target_type = IVS_SYS_GetVLCN(bm, 8);
}

/******************************************************************************
* 功  能: 双字节坐标信息解压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         coord    - 坐标信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_coord_short_bits(IVS_SYS_BUF_MANAGER *bm,IS_PRIVT_INFO_COORDS *coord)
{
    if (NULL == bm || NULL == coord)
    {
        return;
    }
    memset(coord, 0, sizeof(IS_PRIVT_INFO_COORDS));
    coord->x   = IVS_SYS_GetVLCN(bm, 16);
    coord->y   = IVS_SYS_GetVLCN(bm, 16);
}


/******************************************************************************
* 功  能: 颜色车速信息信息压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         coord    - 坐标信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_color_speed_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_COLSPEED *colspeed)
{
    if (NULL == bm || NULL == colspeed)
    {
        return;
    }
    memset(colspeed, 0, sizeof(IS_PRIVT_INFO_COLSPEED));
    get_color_bits(bm, &colspeed->color);
    colspeed->speed = IVS_SYS_GetVLCN(bm, 8);
}


/******************************************************************************
* 功  能: 颜色测温信息信息压缩函数
* 参  数: bm         - 内存管理结构体参数(out)
*         coltemp    - 颜色测温信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_color_temp_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_TEMP *coltemp)
{
    INT32_T  fixed_value = 0;
    INT32_T  flag        = 0;
    if (NULL == bm || NULL == coltemp)
    {
        return ;
    }

    memset(coltemp, 0, sizeof(IS_PRIVT_INFO_TEMP));

    get_color_bits(bm, &coltemp->color);
 
    coltemp->temp_unit = IVS_SYS_GetVLCN(bm, 4);
    coltemp->font_size = IVS_SYS_GetVLCN(bm, 4);
    coltemp->enable    = IVS_SYS_GetVLCN(bm, 1);
    coltemp->x         = IVS_SYS_GetVLCN(bm, 10);
    coltemp->y         = IVS_SYS_GetVLCN(bm, 10);

    // 读取正负号
    flag = IVS_SYS_GetVLCN(bm, 1);

    //读取温度值
    fixed_value = IVS_SYS_read_linfo(bm);

    // 计算温度信息
    coltemp->temp = (float)fixed_value / 10;
    coltemp->temp = (flag == 0) ? coltemp->temp : (0 - coltemp->temp);
}

/******************************************************************************
* 功  能: 颜色违禁物信息压缩函数
* 参  数: bm         - 内存管理结构体参数(out)
*         contraband - 颜色违禁物信息结构体(in)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_color_contraband_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO_CONTRABAND *contraband)
{
    INT32_T  temp        = 0;
    INT32_T  i           = 0;
    if (NULL == bm || NULL == contraband)
    {
        return ;
    }

    memset(contraband, 0, sizeof(IS_PRIVT_INFO_CONTRABAND));

    get_color_bits(bm, &contraband->color);

    contraband->confidence = IVS_SYS_GetVLCN(bm, 8);
    contraband->pos_len    = IVS_SYS_GetVLCN(bm, 8);

    for (i = 0; i < 4; i++)
    {
        temp = IVS_SYS_GetVLCN(bm, 8);
        contraband->type += (temp << (i * 8));
    }

    for (i = 0 ; i < contraband->pos_len; i++)
    {
        contraband->pos_data[i] = IVS_SYS_GetVLCN(bm, 8);
    }
}




/******************************************************************************
* 功  能: 用户扩展信息解压缩函数
* 参  数: bm            - 内存管理结构体参数(out)
*         privt_type    - 扩展数据类型(in)
*         privt_data    - 扩展数据（in）
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
int get_privt_info_bits(IVS_SYS_BUF_MANAGER *bm, unsigned int privt_type,unsigned char* privt_data)
{
    unsigned int privt_len = 0;
    CHECK_ERROR(bm == NULL || privt_data == NULL,  HIK_IVS_SYS_LIB_E_PARA_NULL);

    switch(privt_type)
    {
    case IS_PRIVT_COLOR:        // 存储颜色信息
    case IS_PRIVT_COLOR_RANG:   // 色块填充
        {
            get_color_bits(bm, (IS_PRIVT_INFO_COLOR*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_COLOR);
            break;
        }
    case IS_PRIVT_COLOR_LINE:// 存储轨迹信息
        {
            get_color_line_bits(bm, (IS_PRIVT_INFO_COLORL*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_COLORL);
            break;
        }
    case IS_PRIVY_SPEED_COORD:// 存储速度信息
        {
            get_speed_coord_bits(bm, (IS_PRIVT_INFO_SPECOORD*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_SPECOORD);
            break;
        }
    case IS_PRIVT_COORD_S:// 存储双字节坐标信息
        {
            get_coord_short_bits(bm, (IS_PRIVT_INFO_COORDS*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_COORDS);
            break;
        }
    case IS_PRIVT_EAGLE_R:// 鹰眼跟踪红色四角框
        {
            // 无参数，直接退出
            break;
        }
    case IS_PRIVT_COLOR_SPEED:// 车速颜色信息
        {
            get_color_speed_bits(bm, (IS_PRIVT_INFO_COLSPEED*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_COLSPEED);
            break;
        }
    case IS_PRIVT_COLOR_TEMP:
        {
            get_color_temp_bits(bm, (IS_PRIVT_INFO_TEMP*)privt_data);
            privt_len = sizeof(IS_PRIVT_INFO_TEMP);
            break;
        }
    case IS_PRIVT_COLOR_CONTRABAND:
        {
            get_color_contraband_bits(bm, (IS_PRIVT_INFO_CONTRABAND*)privt_data);
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

/******************************************************************************
* 功  能: 扩展信息转换函数
* 参  数: p_res         - 保留字段指针(IN)
*         privt_info    - 扩展数据指针(OUT)
*         len           - 保留字段长度(IN)
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
HRESULT chance_privt_data(IS_PRIVT_INFO* privt_info, unsigned char* p_res, unsigned int len, unsigned int ivs_type)
{
    CHECK_ERROR(NULL == p_res || NULL == privt_info, HIK_IVS_SYS_LIB_E_PARA_NULL);

    privt_info->privt_type = 0;
    privt_info->privt_len  = 0;

    switch(p_res[0])
    {
    case 0xDC:   // 自定义颜色+轨迹持续时间
        {
            if (IVS_TYPE_TARGE == ivs_type)     // 目标扩展
            {
                if (len > 4)
                {
                    privt_info->privt_type = IS_PRIVT_COLOR_LINE;
                    privt_info->privt_len  = sizeof(IS_PRIVT_INFO_COLORL);
                    memset(privt_info->privt_data, 0, privt_info->privt_len);
                    privt_info->privt_data[0] = p_res[1];
                    privt_info->privt_data[1] = p_res[2];
                    privt_info->privt_data[2] = p_res[3];
                    privt_info->privt_data[3] = 0x00;        // alpha
                    privt_info->privt_data[4] = p_res[4];
                    privt_info->privt_data[5] = p_res[5];
                }
            }
            else if (IVS_TYPE_RULE == ivs_type)     // 规则扩展
            {
                if (len > 3)
                {
                    privt_info->privt_type = IS_PRIVT_COLOR_RANG;
                    privt_info->privt_len  = sizeof(IS_PRIVT_INFO_COLOR);
                    memset(privt_info->privt_data, 0, privt_info->privt_len);
                    privt_info->privt_data[0] = p_res[1];
                    privt_info->privt_data[1] = p_res[2];
                    privt_info->privt_data[2] = p_res[3];
                    privt_info->privt_data[3] = p_res[4];        // alpha
                }
            }

            break;
        }
    case 0xE7:  // 经纬信息
        {
            if (len > 4)
            {
                privt_info->privt_type = IS_PRIVT_COORD_S;
                privt_info->privt_len  = sizeof(IS_PRIVT_INFO_COORDS);
                memset(privt_info->privt_data, 0, privt_info->privt_len);
                privt_info->privt_data[0] = p_res[2];
                privt_info->privt_data[1] = p_res[1];
                privt_info->privt_data[2] = p_res[4];
                privt_info->privt_data[3] = p_res[3];
            }
            break;
        }
    case 0xE8:   // id +速度 + 坐标
        {
            if (len > 5)
            {
                privt_info->privt_type = IS_PRIVY_SPEED_COORD;
                privt_info->privt_len  = sizeof(IS_PRIVT_INFO_SPECOORD);
                memset(privt_info->privt_data, 0, privt_info->privt_len);
                privt_info->privt_data[0] = p_res[1];
                privt_info->privt_data[1] = p_res[2];
                privt_info->privt_data[2] = p_res[3];
                privt_info->privt_data[3] = p_res[4];
                privt_info->privt_data[4] = p_res[5];
            }
            break;
        }
    case 0xEB: // IS_PRIVT_EAGLE_R
        {
            privt_info->privt_type = IS_PRIVT_EAGLE_R;
            privt_info->privt_len  = 0;
            break;
        }
    case 0xDF:   //颜色车辆信息
        {
            if (len > 5)
            {
                privt_info->privt_type = IS_PRIVT_COLOR_SPEED;
                privt_info->privt_len  = sizeof(IS_PRIVT_INFO_COLSPEED);
                memset(privt_info->privt_data, 0, privt_info->privt_len);
                privt_info->privt_data[0] = p_res[1];
                privt_info->privt_data[1] = p_res[2];
                privt_info->privt_data[2] = p_res[3];
                privt_info->privt_data[3] = 0;
                privt_info->privt_data[4] = p_res[4];
            }
            break;
        }
    case 0xDB:
    case 0xDD:
    case 0xDE:// 自定义颜色
        {
            if (len > 3)
            {
                privt_info->privt_type = IS_PRIVT_COLOR;
                privt_info->privt_len  = sizeof(IS_PRIVT_INFO_COLOR);
                memset(privt_info->privt_data, 0, privt_info->privt_len);
                privt_info->privt_data[0] = p_res[1];
                privt_info->privt_data[1] = p_res[2];
                privt_info->privt_data[2] = p_res[3];
                privt_info->privt_data[3] = 0x00;
            }
            break;
        }
    default:
        {
            break;
        }
    }
    return HIK_IVS_SYS_LIB_S_OK;
}


HRESULT  get_multi_privt_bits(IVS_SYS_BUF_MANAGER *bm, IS_PRIVT_INFO* privt)
{
    unsigned char* out_buf      = NULL;
    unsigned int   out_buf_len  = 0;
    int            ret          = 0;
    unsigned int   privt_type   = 0;

    CHECK_ERROR(NULL == bm || NULL == privt, HIK_IVS_SYS_LIB_E_PARA_NULL);
    out_buf = privt->privt_data;

    // 扩展类型
    privt->privt_type = IVS_SYS_GetVLCN(bm, 8);
    privt_type        = privt->privt_type;

    // 扩展信息长度
    privt->privt_len = IVS_SYS_GetVLCN(bm, 8);

    CHECK_ERROR(privt->privt_len > IS_MAX_PRIVT_LEN, HIK_IVS_SYS_LIB_E_DATA_OVERFLOW);

    ret = get_privt_info_bits(bm, privt_type, out_buf);
    if (ret < 0)
    {
        return ret;
    }

    if (privt->privt_len < ret + out_buf_len)
    {
        return HIK_IVS_SYS_LIB_E_DATA_OVERFLOW;
    }

    // 抛弃用于字节对齐的bits
    discard_aligned_bits(bm);

    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 新版目标信息解压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         target   - 目标信息结构体(in)
*         uVersion - 接口版本（in）
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/
HRESULT get_multi_target_bits(IVS_SYS_BUF_MANAGER *bm, HIK_MULT_VCA_TARGET *target, UINT32_T extend_num, UINT16_T uVersion)
{
    int ret = 0;
    CHECK_ERROR(NULL == bm || NULL == target, HIK_IVS_SYS_LIB_E_PARA_NULL);

    // 获取目标框信息
    get_target_bits(bm, &target->target, extend_num, uVersion);

    ret = get_multi_privt_bits(bm, &target->privt_info);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }
    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 新版规则信息解压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         rule     - 规则信息结构体(in)
*         uVersion - 接口版本（in）
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/
HRESULT get_multi_rule_bits(IVS_SYS_BUF_MANAGER *bm, HIK_MULT_VCA_RULE *rule,UINT32_T extend_num, UINT16_T uVersion)
{
    int ret = 0;
    CHECK_ERROR(bm == NULL || rule == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);

    // 设置激活
    rule->rule.enable = 1;

    // 读取规则信息
    ret = get_rule_bits(bm, &rule->rule, extend_num, uVersion);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }

    ret = get_multi_privt_bits(bm, &rule->privt_info);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }

    return HIK_IVS_SYS_LIB_S_OK;
} 



/******************************************************************************
* 功  能: 新版报警信息解压缩函数
* 参  数: bm       - 内存管理结构体参数(out)
*         alert    - 报警信息结构体(in)
*         uVersion - 接口版本（in）
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/
HRESULT get_multi_alert_bits(IVS_SYS_BUF_MANAGER *bm, HIK_MULT_VCA_ALERT *alert, UINT16_T uVersion)
{
    int ret = 0;
    CHECK_ERROR(bm == NULL || alert == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);

    // 读取报警信息
    alert->alert.alert      = 1;
    alert->alert.view_state = IVS_SYS_GetVLCN(bm, 3);

    get_rule_info_bits(bm, &alert->alert.rule_info, uVersion);

    // 解析目标字段
    get_alert_target_bits(bm, &alert->alert.target, uVersion);

    ret = get_multi_privt_bits(bm, &alert->privt_info);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }

    return HIK_IVS_SYS_LIB_S_OK;
} 


/******************************************************************************
* 功  能: 新版目标智能信息解压缩接口
* 参  数: p_meta_data - 新版目标智能信结构体指针(in)
*         param       - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_MULT_META_DATA_sys_parse(HIK_MULT_VCA_TARGET_LIST* p_meta_data, IVS_SYS_PROC_PARAM* param)
{
    IVS_SYS_BUF_MANAGER     buf_manager;                // 内存管理结构体指针
    IVS_SYS_BUF_MANAGER*    bm = NULL;                  // 内存管理结构体
    IVS_SYS_PROC_PARAM      param_com;                    // 调用旧版接口参数
    HIK_VCA_TARGET_LIST_COM meta_data;                  // 调用旧版接口参数
    VCA_TARGET              target[MAX_TARGET_NUM_V2];  // 调用旧版接口参数
    UINT16_T                ver_high          = 0;      // 版本号的高16位
    UINT16_T                ver_low           = 0;      // 版本号的低16位
    INT32_T                 i                 = 0;
    INT32_T                 extend_byte_num_0 = 0;      // extend_byte_num_0对应MetaData结构体中的保留字节的个数。
    INT32_T                 extend_byte_num_1 = 0;      // extend_byte_num_1对应Target结构体中的保留字节的个数
    UINT32_T                temp_value        = 0;      // 临时记录解析得到的码字
    BYTE*                   ana_buffer        = NULL;   // 输入数据的缓冲
    HRESULT                 hr                = 0;      // 函数返回值

    CHECK_ERROR(p_meta_data == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf  == NULL,                  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(p_meta_data->p_target  == NULL,       HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len < 4,                       HIK_IVS_SYS_LIB_S_FAIL);

    param_com.buf          = param->buf;
    param_com.buf_size     = param->buf_size;
    param_com.len          = param->len;
    param_com.scale_height = param->scale_height;
    param_com.scale_width  = param->scale_width;

    ana_buffer = (BYTE*)param->buf;
    //读取版本标识和版本号
    ver_high = (ana_buffer[0] << 8) + ana_buffer[1];
    ver_low  = (ana_buffer[2] << 8) + ana_buffer[3];

    if (ver_high == 0xFFFF && ver_low >= IVS_MULTI_VERSION)
    {
        if (ver_low != IVS_MULTI_VERSION)
        {
            return HIK_IVS_SYS_LIB_S_FAIL;
        }
        param_com.buf  = ana_buffer + 4;
        param_com.len -= 4;
    }
    else
    {
        // 调用旧版接口解压缩
        memset(&meta_data, 0, sizeof(HIK_VCA_TARGET_LIST_COM));
        memset(target,     0, sizeof(VCA_TARGET) * MAX_TARGET_NUM_V2);
        meta_data.p_target = target;

        hr = IVS_META_DATA_sys_parse_com(&meta_data, param);
        if (hr == HIK_IVS_SYS_LIB_S_OK)
        {
            // 将解压缩出来的目标信息映射到数据结构体中
            p_meta_data->type       = meta_data.type;
            p_meta_data->target_num = meta_data.target_num;
            p_meta_data->arith_type = 0;

            for (i = 0; i < meta_data.target_num; i++)
            {
                memcpy(&p_meta_data->p_target[i].target, &meta_data.p_target[i], sizeof(VCA_TARGET));
                hr = chance_privt_data(&p_meta_data->p_target[i].privt_info, meta_data.p_target[i].reserved, 6, IVS_TYPE_TARGE);
                if (hr != HIK_IVS_SYS_LIB_S_OK)
                {
                    return hr;
                }
            }
            return HIK_IVS_SYS_LIB_S_OK;
        }
        else
        {
            return hr;
        }
    }

    bm = &buf_manager;
    IVS_PARSE_init_buf_manager(bm, &param_com);                   // 初始化内存管理参数

    p_meta_data->type       = IVS_SYS_GetVLCN(bm, 32);
    p_meta_data->target_num = (BYTE)IVS_SYS_GetVLCN(bm, 24);    // 获得目标数
    p_meta_data->arith_type = IVS_SYS_GetVLCN(bm, 24);          // 获取算法类型

    if (p_meta_data->target_num > MAX_TARGET_NUM_V2)            // 超过最大目标个数，目标个数归零，防止缓存读取溢出
    {
        p_meta_data->target_num = 0;
    }

    //解析得到8个bits
    temp_value        = IVS_SYS_GetVLCN(bm, 8);
    extend_byte_num_0 = ((temp_value >> 4) & 0x7);              // MetaData结构体中的保留字节的个数
    extend_byte_num_1 = (temp_value & 0xF);                     // Target结构体中的保留字节的个数

    for (i = 0; i < extend_byte_num_0; i++)
    {
        IVS_SYS_GetVLCN(bm, 8);//跳过保留字节
    }

    //宽度方向上的缩放比例 
    temp_value      = IVS_SYS_GetVLCN(bm, 16);
    bm->scale_width = (UINT16_T)(temp_value & CONST_15_BITS_MASK);

    //高度方向上的缩放比例 
    temp_value       = IVS_SYS_GetVLCN(bm, 16);
    bm->scale_height = (UINT16_T)(temp_value & CONST_15_BITS_MASK);

    // 读取目标信息
    for (i = 0; i < p_meta_data->target_num; i++)
    {
        hr = get_multi_target_bits(bm, &(p_meta_data->p_target[i]),extend_byte_num_1, ver_low);
        if (hr != HIK_IVS_SYS_LIB_S_OK)
        {
            return hr;
        }
    }

    //码流保护
    CHECK_ERROR((((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size) || bm->over_flag), HIK_IVS_SYS_LIB_E_MEM_OVER);

    return HIK_IVS_SYS_LIB_S_OK;
}



/******************************************************************************
* 功  能: 新版规则智能信息解压缩接口
* 参  数: p_rule_data - 新版规则智能信结构体指针(in)
*         param       - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_MULT_RULE_DATA_sys_parse(HIK_MULT_VCA_RULE_LIST* p_rule_data, IVS_SYS_PROC_PARAM* param)
{
    IVS_SYS_BUF_MANAGER     buf_manager;                // 内存管理结构体指针
    IVS_SYS_BUF_MANAGER*    bm = NULL;                  // 内存管理结构体
    IVS_SYS_PROC_PARAM      param_com;                  // 调用旧版接口参数
    HIK_VCA_RULE_LIST_COM   rule_data;                  // 调用旧版接口参数
    VCA_RULE                rule[MAX_RULE_NUM_V3];      // 调用旧版接口参数
    UINT16_T                ver_high          = 0;      // 版本号的高16位
    UINT16_T                ver_low           = 0;      // 版本号的低16位
    INT32_T                 i                 = 0;
    INT32_T                 extend_byte_num_0 = 0;      // extend_byte_num_0对应MetaData结构体中的保留字节的个数。
    INT32_T                 extend_byte_num_1 = 0;      // extend_byte_num_1对应Target结构体中的保留字节的个数
    UINT32_T                temp_value        = 0;      // 临时记录解析得到的码字
    UINT32_T                enable_num        = 0;      // 有效规则个数
    BYTE*                   ana_buffer        = NULL;   // 输入数据的缓冲
    HRESULT                 hr                = 0;      // 函数返回值

    CHECK_ERROR(p_rule_data == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf  == NULL,                  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(p_rule_data->p_rule == NULL,          HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len < 4,                       HIK_IVS_SYS_LIB_S_FAIL);

    ana_buffer = (BYTE*)param->buf;
    //读取版本标识和版本号
    ver_high = (ana_buffer[0] << 8) + ana_buffer[1];
    ver_low  = (ana_buffer[2] << 8) + ana_buffer[3];

    param_com.buf          = param->buf;
    param_com.buf_size     = param->buf_size;
    param_com.len          = param->len;
    param_com.scale_height = param->scale_height;
    param_com.scale_width  = param->scale_width;

    if (ver_high == 0xFFFF && ver_low >= IVS_MULTI_VERSION)
    {
        if (ver_low != IVS_MULTI_VERSION)
        {
            return HIK_IVS_SYS_LIB_S_FAIL;
        }
        param_com.buf = ana_buffer    + 4;
        param_com.len = param_com.len - 4;
    }
    else
    {
        memset(&rule_data, 0, sizeof(HIK_VCA_RULE_LIST_COM));
        memset(rule,       0, sizeof(VCA_RULE) * MAX_RULE_NUM_V3);
        rule_data.p_rule = rule;

        hr = IVS_RULE_DATA_sys_parse_com(&rule_data, &param_com);
        if (hr == HIK_IVS_SYS_LIB_S_OK)
        {
            // 将解压缩出来的规则信息映射到数据结构体中
            p_rule_data->type       = rule_data.type;
            p_rule_data->rule_num   = rule_data.rule_num;
            p_rule_data->arith_type = 0;
            for (i = 0; i < rule_data.rule_num; i++)
            {
                memcpy(&p_rule_data->p_rule[i].rule, &rule_data.p_rule[i], sizeof(VCA_RULE));
                hr = chance_privt_data(&p_rule_data->p_rule[i].privt_info, rule_data.p_rule[i].reserved, 5, IVS_TYPE_RULE);
                if (hr != HIK_IVS_SYS_LIB_S_OK)
                {
                    return hr;
                }
            }
            return HIK_IVS_SYS_LIB_S_OK;
        }
        else
        {
            return hr;
        }
    }

    bm = &buf_manager;
    IVS_PARSE_init_buf_manager(bm, &param_com);                 // 初始化内存管理参数

    p_rule_data->type       = IVS_SYS_GetVLCN(bm, 32);
    p_rule_data->rule_num   = (BYTE)IVS_SYS_GetVLCN(bm, 24);    // 获得目标数
    p_rule_data->arith_type = IVS_SYS_GetVLCN(bm, 24);          // 获取算法类型

    if (p_rule_data->rule_num > MAX_RULE_NUM_V3)                // 超过最大目标个数，目标个数归零，防止缓存读取溢出
    {
        p_rule_data->rule_num = 0;
    }

    //解析得到8个bits
    temp_value        = IVS_SYS_GetVLCN(bm, 8);
    extend_byte_num_0 = ((temp_value >> 4) & 0x7);              // MetaData结构体中的保留字节的个数
    extend_byte_num_1 = (temp_value & 0xF);                     // Target结构体中的保留字节的个数

    // 读取有效规则个数
    enable_num = IVS_SYS_GetVLCN(bm, 24);
    if (enable_num != p_rule_data->rule_num)
    {
        p_rule_data->rule_num = enable_num;
    }

    for (i = 0; i < extend_byte_num_0; i++)
    {
        IVS_SYS_GetVLCN(bm, 8);//跳过保留字节
    }

    //宽度方向上的缩放比例 
    temp_value      = IVS_SYS_GetVLCN(bm, 16);
    bm->scale_width = (UINT16_T)(temp_value & CONST_15_BITS_MASK);

    //高度方向上的缩放比例 
    temp_value       = IVS_SYS_GetVLCN(bm, 16);
    bm->scale_height = (UINT16_T)(temp_value & CONST_15_BITS_MASK);

    for (i = 0; i < enable_num; i++)
    {
        hr = get_multi_rule_bits(bm, &p_rule_data->p_rule[i], extend_byte_num_1, ver_low);
    }

    //码流保护
    CHECK_ERROR((((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size) || bm->over_flag), HIK_IVS_SYS_LIB_E_MEM_OVER);

    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 新版报警智能信息解压缩接口
* 参  数: p_alert_data - 新版报警智能信结构体指针(in)
*         param       - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_MULT_ALERT_DATA_sys_parse(HIK_MULT_VCA_ALERT_LIST* p_alert_data, IVS_SYS_PROC_PARAM* param)
{
    IVS_SYS_BUF_MANAGER     buf_manager;                // 内存管理结构体指针
    IVS_SYS_BUF_MANAGER*    bm = NULL;                  // 内存管理结构体
    IVS_SYS_PROC_PARAM      param_com;                  // 调用旧版接口参数
    VCA_ALERT               alert;                      // 调用旧版接口参数
    UINT16_T                ver_high          = 0;      // 版本号的高16位
    UINT16_T                ver_low           = 0;      // 版本号的低16位
    INT32_T                 i                 = 0;
    INT32_T                 extend_byte_num_0 = 0;      // extend_byte_num_0对应MetaData结构体中的保留字节的个数。
    UINT32_T                temp_value        = 0;      // 临时记录解析得到的码字
    UINT32_T                enable_num        = 0;      // 有效规则个数
    BYTE*                   ana_buffer        = NULL;   // 输入数据的缓冲
    HRESULT                 hr                = 0;      // 函数返回值

    CHECK_ERROR(p_alert_data == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf  == NULL,                   HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(p_alert_data->p_alert == NULL,         HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len < 4,                        HIK_IVS_SYS_LIB_S_FAIL);

    ana_buffer = (BYTE*)param->buf;
    //读取版本标识和版本号
    ver_high = (ana_buffer[0] << 8) + ana_buffer[1];
    ver_low  = (ana_buffer[2] << 8) + ana_buffer[3];

    param_com.buf          = param->buf;
    param_com.buf_size     = param->buf_size;
    param_com.len          = param->len;
    param_com.scale_height = param->scale_height;

    if (ver_high == 0xFFFF && ver_low >= IVS_MULTI_VERSION)
    {
        if (ver_low != IVS_MULTI_VERSION)
        {
            return HIK_IVS_SYS_LIB_S_FAIL;
        }
        param_com.buf = ana_buffer    + 4;
        param_com.len = param_com.len - 4;
    }
    else
    {
        memset(&alert, 0, sizeof(VCA_ALERT));
        hr = IVS_EVENT_DATA_sys_parse(&alert, &param_com);
        if (hr == HIK_IVS_SYS_LIB_S_OK)
        {
            // 将解压缩出来的规则信息映射到数据结构体中
            p_alert_data->type       = SHOW_TYPE_NORMAL | MATCH_TYPE_NORMAL;
            p_alert_data->alert_num  = 1;
            p_alert_data->arith_type = 0;

            memcpy(&p_alert_data->p_alert[0].alert, &alert, sizeof(VCA_ALERT));
            p_alert_data->p_alert[0].privt_info.privt_len  = 0;
            p_alert_data->p_alert[0].privt_info.privt_type = 0;

            return HIK_IVS_SYS_LIB_S_OK;
        }
        else
        {
            return hr;
        }
    }

    bm = &buf_manager;
    IVS_PARSE_init_buf_manager(bm, &param_com);                 // 初始化内存管理参数

    p_alert_data->type       = IVS_SYS_GetVLCN(bm, 32);
    p_alert_data->alert_num   = (BYTE)IVS_SYS_GetVLCN(bm, 24);    // 获得目标数
    p_alert_data->arith_type = IVS_SYS_GetVLCN(bm, 24);          // 获取算法类型

    if (p_alert_data->alert_num > MAX_RULE_NUM_V3)                // 超过最大目标个数，目标个数归零，防止缓存读取溢出
    {
        p_alert_data->alert_num = 0;
    }

    //解析得到8个bits
    temp_value        = IVS_SYS_GetVLCN(bm, 8);
    extend_byte_num_0 = ((temp_value >> 4) & 0x7);              // MetaData结构体中的保留字节的个数

    // 读取有效规则个数
    enable_num = IVS_SYS_GetVLCN(bm, 24);
    if (enable_num != p_alert_data->alert_num)
    {
        p_alert_data->alert_num = enable_num;
    }

    for (i = 0; i < extend_byte_num_0; i++)
    {
        IVS_SYS_GetVLCN(bm, 8);//跳过保留字节
    }

    //宽度方向上的缩放比例 
    temp_value      = IVS_SYS_GetVLCN(bm, 16);
    bm->scale_width = (UINT16_T)(temp_value & CONST_15_BITS_MASK);

    //高度方向上的缩放比例 
    temp_value       = IVS_SYS_GetVLCN(bm, 16);
    bm->scale_height = (UINT16_T)(temp_value & CONST_15_BITS_MASK);

    for(i = 0; i < enable_num; i++)
    {
        hr = get_multi_alert_bits(bm, &p_alert_data->p_alert[i], ver_low);
        if (hr != HIK_IVS_SYS_LIB_S_OK)
        {
             return hr;
        }
    }

    //码流保护
    CHECK_ERROR((((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size) || bm->over_flag), HIK_IVS_SYS_LIB_E_MEM_OVER);

    return HIK_IVS_SYS_LIB_S_OK;
}


/*********************************
区域定制
**********************************/

/******************************************************************************
* 功  能: 解析旋转框结构体数据(支持正负)
* 参  数: bm      - 内存管理结构体指针        (out)
*         rect    - 归一化的旋转框结构体指针  (in)  
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_rotate_rect_bits(IVS_SYS_BUF_MANAGER *bm, VCA_ROTATE_RECT_F *rect)
{
    UINT32_T value    = 0;     //临时记录坐标值
    DWORD    flag     = 0;
    float    abs_temp = 0;

    if (bm == NULL || rect == NULL)
    {
        return ;
    }

    //框的宽高为0没有意义
    if (bm->scale_width == 0 || bm->scale_height == 0)
    {
        return ;
    }

    //边界框左上角X轴坐标
    flag     = IVS_SYS_GetVLCN(bm, 1);
    value    = IVS_SYS_read_linfo(bm);
    abs_temp = (float)value / bm->scale_width;
    rect->cx = (flag) ? (0 - abs_temp) : abs_temp;

    //边界框左上角Y轴坐标
    flag     = IVS_SYS_GetVLCN(bm, 1);
    value    = IVS_SYS_read_linfo(bm);
    abs_temp = (float)value / bm->scale_width;
    rect->cy = (flag) ? (0 - abs_temp) : abs_temp;

    //边界框的宽度
    flag        = IVS_SYS_GetVLCN(bm, 1);
    value       = IVS_SYS_read_linfo(bm);
    abs_temp    = (float)value / bm->scale_width;
    rect->width = (flag) ? (0 - abs_temp) : abs_temp;

    //边界框的高度
    flag         = IVS_SYS_GetVLCN(bm, 1);
    value        = IVS_SYS_read_linfo(bm);
    abs_temp     = (float)value / bm->scale_width;
    rect->height = (flag) ? (0 - abs_temp) : abs_temp;

    // 旋转角度
    flag        = IVS_SYS_GetVLCN(bm, 1);
    value       = IVS_SYS_read_linfo(bm);
    abs_temp    = (float)value / bm->scale_width;
    rect->theta = (flag) ? (0 - abs_temp) : abs_temp;
}


/******************************************************************************
* 功  能: 解析多边形结构体数据(支持正负)
* 参  数: bm      - 内存管理结构体指针        (out)
*         polygon - 归一化的多边形结构体指针  (in)  
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_polygon_ext_bits(IVS_SYS_BUF_MANAGER *bm, VCA_POLYGON_F *polygon)
{
    UINT32_T i        = 0;
    UINT32_T temp     = 0;
    DWORD    flag     = 0;
    float    abs_temp = 0;

    if (NULL == bm || NULL == polygon)
    {
        return ;
    }

    //多边形顶点数不超过10
    if (polygon->vertex_num > MAX_POLYGON_VERTEX)
    {
        polygon->vertex_num = 0;
        return ;
    }

    for (i = 0; i < polygon->vertex_num; i++)
    {
        flag                = IVS_SYS_GetVLCN(bm, 1);
        temp                = IVS_SYS_GetVLCN(bm, 16);
        abs_temp            = (float)(temp & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
        polygon->point[i].x = (flag) ? (0 - abs_temp) : abs_temp;

        flag                = IVS_SYS_GetVLCN(bm, 1);
        temp                = IVS_SYS_GetVLCN(bm, 16);
        abs_temp            = (float)(temp & CONST_15_BITS_MASK) / CONST_15_BITS_OPERATOR;
        polygon->point[i].y = (flag) ? (0 - abs_temp) : abs_temp;
    }
}


/******************************************************************************
* 功  能: 解析矩形框结构体数据(支持正负)
* 参  数: bm      - 内存管理结构体指针        (out)
*         rect    - 归一化的矩形框结构体指针  (in)  
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_rect_normalize_entropy_ext_bits(IVS_SYS_BUF_MANAGER *bm, VCA_RECT_F *rect)
{
    UINT32_T value    = 0;     //临时记录坐标值
    DWORD    flag     = 0;
    float    abs_temp = 0;

    if (bm == NULL || rect == NULL)
    {
        return ;
    }

    //框的宽高为0没有意义
    if (bm->scale_width == 0 || bm->scale_height == 0)
    {
        return ;
    }

    //边界框左上角X轴坐标
    flag     = IVS_SYS_GetVLCN(bm, 1);
    value    = IVS_SYS_read_linfo(bm);
    abs_temp = (float)value / bm->scale_width;
    rect->x  = (flag) ? (0 - abs_temp) : abs_temp;

    //边界框左上角Y轴坐标
    flag     = IVS_SYS_GetVLCN(bm, 1);
    value    = IVS_SYS_read_linfo(bm);
    abs_temp = (float)value / bm->scale_width;
    rect->y  = (flag) ? (0 - abs_temp) : abs_temp;

    //边界框的宽度
    flag        = IVS_SYS_GetVLCN(bm, 1);
    value       = IVS_SYS_read_linfo(bm);
    abs_temp    = (float)value / bm->scale_width;
    rect->width = (flag) ? (0 - abs_temp) : abs_temp;

    //边界框的高度
    flag         = IVS_SYS_GetVLCN(bm, 1);
    value        = IVS_SYS_read_linfo(bm);
    abs_temp     = (float)value / bm->scale_width;
    rect->height = (flag) ? (0 - abs_temp) : abs_temp;
}


/******************************************************************************
* 功  能: 解析区域结构体数据(支持正负)
* 参  数: bm       - 内存管理结构体指针        (out)
*         p_region - 区域结构体指针  (in)  
*         
* 返回值: 无
* 备  注:
*******************************************************************************/
static void get_region_bits(IVS_SYS_BUF_MANAGER *bm, VCA_REGION* p_region)
{
    unsigned int region_type = 0;

    if (NULL == bm || NULL == p_region)
    {
        return;
    }
    region_type = IVS_SYS_GetVLCN(bm, 4);

    switch(region_type)
    {
    case VCA_REGION_POLYLINE:       // 多边形线
    case VCA_REGION_POLYGON:        // 多边形
        {
            p_region->polygon.vertex_num = IVS_SYS_GetVLCN(bm, 6);
            get_polygon_ext_bits(bm, &p_region->polygon);
            break;
        }
    case VCA_REGION_RECT:           // 四边形框
        {
            get_rect_normalize_entropy_ext_bits(bm, &p_region->rect);
            break;
        }
    case VCA_REGION_ROTATE_RECT:    // 旋转框
        {
            get_rotate_rect_bits(bm, &p_region->rotate_rect);
            break;
        }
    default:
        {
            return;
        }
    }

    p_region->region_type = region_type;
}


/******************************************************************************
* 功  能: 目标区域信息解压缩函数
* 参  数: bm        - 内存管理结构体参数(out)
*         target    - 目标区域信息结构体(in)
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/
static void get_target_blob_bits(IVS_SYS_BUF_MANAGER *bm, VCA_BLOB_BASIC_INFO *target)
{
    UINT32_T i    = 0;      //循环计数
    int      code = 0;      //临时记录解析得到的码字

    if (NULL == bm || NULL == target)
    {
        return;
    }

    //目标的生成序号
    for (i = 0, code = 0; i < 4; i++)
    {
        code += (IVS_SYS_GetVLCN(bm, 8) << (i * 8)); //跳过保留字节
    }
    target->id = code;

    // 保存当前目标的区域类型
    for (i = 0, code = 0; i < 4; i++)
    {
        code += (IVS_SYS_GetVLCN(bm, 8) << (i * 8)); //跳过保留字节
    }
    target->blob_type = code;

    // 置信度
    target->confidence = (short)IVS_SYS_GetVLCN(bm, 16);

    // 区域框 rect
    get_region_bits(bm, &target->region);

    // 抛弃用于字节对齐的bits
    discard_aligned_bits(bm);

}


/******************************************************************************
* 功  能: 扩展目标区域信息解压缩函数
* 参  数: bm        - 内存管理结构体参数(out)
*         target    - 扩展目标信息结构体(in)
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/
HRESULT get_multi_target_blob_bits(IVS_SYS_BUF_MANAGER *bm, HIK_TARGET_BLOB *target)
{
    int ret = 0;
    CHECK_ERROR(NULL == bm || NULL == target, HIK_IVS_SYS_LIB_E_PARA_NULL);

    // 获取目标框信息
    get_target_blob_bits(bm, &target->target);

    ret = get_multi_privt_bits(bm, &target->privt_info);
    if (ret != HIK_IVS_SYS_LIB_S_OK)
    {
        return ret;
    }
    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 源目标参数结构体转为区域目标结构体
* 参  数: target    - 目标参数结构体(in)
*         blob      - 区域目标结构体(out)
*         
* 返回值: 状态码
* 备  注:
*******************************************************************************/

HRESULT target_change_blob(HIK_MULT_VCA_TARGET* target, HIK_TARGET_BLOB* blob)
{
    if (NULL == target || NULL == blob)
    {
        return HIK_IVS_SYS_LIB_E_PARA_NULL;
    }

    blob->target.id         = target->target.ID;
    blob->target.blob_type  = target->target.type;
    blob->target.confidence = target->target.alarm_flg;

    // 原目标固定为矩形框
    blob->target.region.region_type = VCA_REGION_RECT;
    memcpy(&blob->target.region.rect, &target->target.rect, sizeof(VCA_RECT_F));

    // 复制扩展信息
    memcpy(&blob->privt_info, &target->privt_info, sizeof(IS_PRIVT_INFO));

    return HIK_IVS_SYS_LIB_S_OK;
}


/******************************************************************************
* 功  能: 扩展目标区域信息解压缩接口
* 参  数: p_target_data - 扩展目标区域信息结构体指针(in)
*         param         - 处理参数结构体指针    (out)                 
*
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_META_BLOB_DATA_sys_parse(HIK_TARGET_BLOB_LIST* p_target_data, IVS_SYS_PROC_PARAM* param)
{
    IVS_SYS_BUF_MANAGER     buf_manager;                // 内存管理结构体指针
    IVS_SYS_BUF_MANAGER*    bm                = NULL;   // 内存管理结构体
    IVS_SYS_PROC_PARAM      param_com;                  // 调用旧版接口参数
    UINT16_T                ver_high          = 0;      // 版本号的高16位
    UINT16_T                ver_low           = 0;      // 版本号的低16位
    INT32_T                 i                 = 0;
    INT32_T                 extend_byte_num_0 = 0;      // extend_byte_num_0对应MetaData结构体中的保留字节的个数。
    UINT32_T                temp_value        = 0;      // 临时记录解析得到的码字
    BYTE*                   ana_buffer        = NULL;   // 输入数据的缓冲
    HRESULT                 hr                = 0;      // 函数返回值
    HIK_MULT_VCA_TARGET_LIST target_list;               // 旧版接口结构体
    HIK_MULT_VCA_TARGET      target[MAX_TARGET_NUM_V2]; // 旧版接口目标结构体

    CHECK_ERROR(p_target_data == NULL || param == NULL, HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->buf    == NULL,                  HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(p_target_data->p_target  == NULL,       HIK_IVS_SYS_LIB_E_PARA_NULL);
    CHECK_ERROR(param->len < 4,                         HIK_IVS_SYS_LIB_S_FAIL);

    param_com.buf          = param->buf;
    param_com.buf_size     = param->buf_size;
    param_com.len          = param->len;
    param_com.scale_height = param->scale_height;
    param_com.scale_width  = param->scale_width;

    ana_buffer = (BYTE*)param->buf;
    //读取版本标识和版本号
    ver_high = (ana_buffer[0] << 8) + ana_buffer[1];
    ver_low  = (ana_buffer[2] << 8) + ana_buffer[3];

    if (ver_high == 0xFFFF && ver_low >= IVS_MULTI_BLOB)
    {
        if (ver_low != IVS_MULTI_BLOB)
        {
            return HIK_IVS_SYS_LIB_S_FAIL;
        }
        param_com.buf  = ana_buffer + 4;
        param_com.len -= 4;
    }
    else
    {
        memset(&target_list, 0, sizeof(HIK_MULT_VCA_TARGET_LIST));
        memset(target,       0, sizeof(HIK_MULT_VCA_TARGET) * MAX_TARGET_NUM_V2);
        target_list.p_target = target;

        hr = IVS_MULT_META_DATA_sys_parse(&target_list, &param_com);
        if (hr != HIK_IVS_SYS_LIB_S_OK)
        {
            return hr;
        }

        p_target_data->type       = target_list.type;
        p_target_data->arith_type = target_list.arith_type;
        p_target_data->target_num = target_list.target_num;

        for (i = 0; i < target_list.target_num; i++)
        {
            target_change_blob(&target_list.p_target[i], &p_target_data->p_target[i]);
        }

        return HIK_IVS_SYS_LIB_S_OK;
    }

    bm = &buf_manager;
    IVS_PARSE_init_buf_manager(bm, &param_com);                   // 初始化内存管理参数

    p_target_data->type       = IVS_SYS_GetVLCN(bm, 32);
    p_target_data->target_num = (BYTE)IVS_SYS_GetVLCN(bm, 24);    // 获得目标数
    p_target_data->arith_type = IVS_SYS_GetVLCN(bm, 24);          // 获取算法类型

    if (p_target_data->target_num > MAX_TARGET_NUM_V2)            // 超过最大目标个数，目标个数归零，防止缓存读取溢出
    {
        p_target_data->target_num = 0;
    }

    //解析得到8个bits
    temp_value        = IVS_SYS_GetVLCN(bm, 8);
    extend_byte_num_0 = ((temp_value >> 4) & 0x7);              // MetaData结构体中的保留字节的个数

    for (i = 0; i < extend_byte_num_0; i++)
    {
        IVS_SYS_GetVLCN(bm, 8);//跳过保留字节
    }

    //宽度方向上的缩放比例 
    temp_value      = IVS_SYS_GetVLCN(bm, 16);
    bm->scale_width = (UINT16_T)(temp_value & CONST_15_BITS_MASK);

    //高度方向上的缩放比例 
    temp_value       = IVS_SYS_GetVLCN(bm, 16);
    bm->scale_height = (UINT16_T)(temp_value & CONST_15_BITS_MASK);

    // 读取目标信息
    for (i = 0; i < p_target_data->target_num; i++)
    {
        hr = get_multi_target_blob_bits(bm, &(p_target_data->p_target[i]));
        if (hr != HIK_IVS_SYS_LIB_S_OK)
        {
            return hr;
        }
    }

    //码流保护
    CHECK_ERROR((UINT32_T)(bm->cur_pos - bm->buf_start) > (UINT32_T)bm->buf_size, 
                HIK_IVS_SYS_LIB_E_MEM_OVER);

    return HIK_IVS_SYS_LIB_S_OK;
}