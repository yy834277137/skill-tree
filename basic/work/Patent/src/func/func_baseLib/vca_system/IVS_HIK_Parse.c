/******************************************************************************
* 
* 版权信息: Copyright (c) 2009, 杭州海康威视数字技术股份有限公司
* 
* 文件名称: IVS_HIK_Parse.c
* 文件标识: HIKVISION_IVS_HIK_PARSE_C
* 摘    要: 海康威视HIK文件解析文件
*
* 当前版本: 1.0
* 作    者: 陈军
* 日    期: 2009年2月18日
* 备    注:
*           
*******************************************************************************
*/

#include "IVS_HIK_lib.h"
#include "IVS_HIK_inner.h"
#include "type_def.h"
#include "hik_endian.h"


/******************************************************************************
* 功  能: 跳过HIK文件头
* 参  数: fp - 文件指针(IN)
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT HIK_IVS_SKIP_FILE_Header(FILE *fp)
{
	int ret;
	ret = fseek(fp, sizeof(HIKVISION_MEDIA_FILE_HEADER), SEEK_SET);
    CHECK_ERROR(ret, HIK_IVS2HIK_LIB_E_FILE_OPERATION);

	return HIK_IVS2HIK_LIB_S_OK;
}

/******************************************************************************
* 功  能: 解析HIK文件Group头
* 参  数: param - 处理参数结构体指针 (OUT)
*         fp    - 文件指针           (IN)
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT HIK_IVS_GROUP_HDR_Parse(IVS_HIK_PROC_PARAM *param, FILE *fp)
{
	GROUP_HEADER group_hdr;
	UINT32_T count;

	/* Read the group header */
	count = fread(&group_hdr, sizeof(GROUP_HEADER), 1, fp);
	CHECK_ERROR(count != 1, HIK_IVS2HIK_LIB_E_FILE_OPERATION);
	
	param->gp_param.block_num = u32lh(group_hdr.block_number) - CONST_NUMBER_BASE;
	param->gp_param.frame_num = u32lh(group_hdr.frame_num)    - CONST_NUMBER_BASE;
	param->gp_param.pic_mode  = u32lh(group_hdr.picture_mode);
	
	if (u32lh(group_hdr.start_code) != START_CODE || !param->gp_param.block_num)
	{
		return HIK_IVS2HIK_LIB_S_FAIL;
	}

	return HIK_IVS2HIK_LIB_S_OK;
}

/******************************************************************************
* 功  能: 获取HIK文件Block块数据
* 参  数: param - 处理参数结构体指针 (OUT)
*         fp    - 文件指针           (IN)
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT HIK_IVS_GET_BLOCK_Data(IVS_HIK_PROC_PARAM *param, FILE *fp)
{
	INT32_T ret;
    BLOCK_HEADER block_hdr;
	
	if(param->gp_param.block_num > 0)
	{ 
		/* Read the block header */
		ret = fread(&block_hdr, sizeof(BLOCK_HEADER), 1, fp);
		CHECK_ERROR(ret != 1, HIK_IVS2HIK_LIB_E_FILE_OPERATION);
		
		param->blk_param.blk_type = u16lh(block_hdr.type);
		param->blk_param.blk_version = u16lh(block_hdr.version);
		
		/* block size */
		param->blk_param.blk_size = u32lh(block_hdr.block_size);
		CHECK_ERROR(param->blk_param.blk_size > param->buf_out_size, HIK_IVS2HIK_LIB_E_MEM_OVER);
		
		param->len_out = param->blk_param.blk_size;

		/* read the block content, i.e. an h264 NAL Unit */
		ret = fread(param->buf_out, param->len_out, 1, fp);
		CHECK_ERROR(ret != 1, HIK_IVS2HIK_LIB_E_FILE_OPERATION);
	}
	else
	{
        return HIK_IVS2HIK_LIB_S_FAIL;
	}

   	return HIK_IVS2HIK_LIB_S_OK;
}
