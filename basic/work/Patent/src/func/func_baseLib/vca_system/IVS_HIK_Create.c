/******************************************************************************
* 
* 版权信息: Copyright (c) 2009, 杭州海康威视数字技术股份有限公司
* 
* 文件名称: IVS_HIK_Create.c
* 文件标识: HIKVISION_IVS_HIK_CREATE_C 
* 摘    要: 海康威视HIK文件创建
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

/******************************************************************************
* 功  能: 创建HIK文件头
* 参  数: param - 处理参数结构体指针 (in:out)
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_HIK_FILE_HDR_Create(IVS_HIK_PROC_PARAM *param)
{
	HIKVISION_MEDIA_FILE_HEADER *p_file_hdr;

	CHECK_ERROR(param == NULL, HIK_IVS2HIK_LIB_E_PARA_NULL);

	CHECK_ERROR(param->buf_out_size < sizeof(HIKVISION_MEDIA_FILE_HEADER), 
		        HIK_IVS2HIK_LIB_E_SIZE);

    p_file_hdr = (HIKVISION_MEDIA_FILE_HEADER *) param->buf_out;
	
	p_file_hdr->start_code   = HKH4_FILE_CODE;
	p_file_hdr->magic_number = MAGIC_NUMBER;
	p_file_hdr->version      = CURRENT_VERSION;
	p_file_hdr->check_sum    = 0;
	p_file_hdr->stream_type  = 1 + CONST_NUMBER_BASE; //STREAM_TYPE_VIDEO
	p_file_hdr->video_format = FORMAT_PAL;

	if(param->gp_param.height == 480 || param->gp_param.height == 240)
    {
		p_file_hdr->video_format = FORMAT_NTSC;
    }
	
	p_file_hdr->channels        = AUDIO_MONO;
	p_file_hdr->bits_per_sample = BITSPERSAMP16;
	p_file_hdr->samples_per_sec = 16000;
		
    p_file_hdr->image_size.size.image_width  = param->gp_param.width;
    p_file_hdr->image_size.size.image_height = param->gp_param.height;
	
	p_file_hdr->audio_type = 0;
	
	p_file_hdr->system_param = 0x80000000;

	param->len_out = sizeof(HIKVISION_MEDIA_FILE_HEADER);

	return HIK_IVS2HIK_LIB_S_OK;
}

/******************************************************************************
* 功  能: 创建HIK文件Group头
* 参  数: param - 处理参数结构体指针 (in:out)
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_HIK_GROUP_HDR_Create(IVS_HIK_PROC_PARAM *param)
{
	GROUP_HEADER *p_gp_hdr;
	INT32_T block_num = 0;

	CHECK_ERROR(param == NULL, HIK_IVS2HIK_LIB_E_PARA_NULL);
	
	CHECK_ERROR(param->buf_out_size < sizeof(GROUP_HEADER), HIK_IVS2HIK_LIB_E_SIZE);

	p_gp_hdr = (GROUP_HEADER *)param->buf_out;

    //memset(p_gp_hdr, 0, sizeof(GROUP_HEADER));

	switch(param->gp_param.pic_mode)
	{
	case P_FRAME_MODE:
	
	case I_FRAME_MODE:
		block_num = 1;
		break;
	
	case BP_FRAME_MODE:
		block_num = 2;
		break;
	
	case BBP_FRAME_MODE:
	
	case III_FRAME_MODE:
		block_num = 3;
	    break;

    case VCA_IVS_MODE:
	case VCA_ITS_MODE:
	case VCA_IAS_MODE:
		block_num = param->gp_param.block_num;
        break;

	default:
		break;
	}
	
	p_gp_hdr->start_code  = START_CODE; 
	p_gp_hdr->frame_num   = param->gp_param.frame_num + CONST_NUMBER_BASE;

    /* 1/64 second */
	p_gp_hdr->time_stamp  = param->gp_param.frame_num * 64 / param->gp_param.frame_rate; 
    p_gp_hdr->is_audio    = 0 + CONST_NUMBER_BASE;
	
    p_gp_hdr->block_number = block_num + CONST_NUMBER_BASE;

	p_gp_hdr->image_size.size.image_width  = param->gp_param.width;
    p_gp_hdr->image_size.size.image_height = param->gp_param.height;
    
	p_gp_hdr->picture_mode   = param->gp_param.pic_mode;

	p_gp_hdr->frame_rate   = param->gp_param.frame_rate + CONST_NUMBER_BASE;	

	param->len_out = sizeof(GROUP_HEADER);

	return HIK_IVS2HIK_LIB_S_OK;
}

/******************************************************************************
* 功  能: 创建HIK文件Block头
* 参  数: param - 处理参数结构体指针 (in:out)
*         
* 返回值: 返回状态码
* 备  注:
*******************************************************************************/
HRESULT IVS_HIK_BLOCK_HDR_Create(IVS_HIK_PROC_PARAM *param)
{
	BLOCK_HEADER *p_blk_hdr;

	CHECK_ERROR(param == NULL, HIK_IVS2HIK_LIB_E_PARA_NULL);
	
	CHECK_ERROR(param->buf_out_size < sizeof(BLOCK_HEADER), HIK_IVS2HIK_LIB_E_SIZE);
	
	p_blk_hdr = (BLOCK_HEADER *)param->buf_out;
	
    p_blk_hdr->type = (UINT16_T)param->blk_param.blk_type;
	
    p_blk_hdr->version = param->blk_param.blk_version;

	p_blk_hdr->block_size = param->blk_param.blk_size;

	param->len_out = sizeof(BLOCK_HEADER);

	return HIK_IVS2HIK_LIB_S_OK;
}