/******************************************************************************
* 
* 版权信息: Copyright (c) 2009, 杭州海康威视数字技术股份有限公司
* 
* 文件名称: IVS_HIK_lib.h
* 文件标识: HIKVISION_IVS_HIK_LIB_H
* 摘    要: 海康威视创建HIK文件库头文件
*
* 当前版本: 1.0
* 作    者: 陈军
* 日    期: 2009年2月13日
* 备    注:
*           
*******************************************************************************
*/

#ifndef _IVS_HIK_LIB_H_
#define _IVS_HIK_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>


#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
	typedef int HRESULT;
#endif // !_HRESULT_DEFINED


//错误码定义
#define HIK_IVS2HIK_LIB_S_OK                    1               // 成功	    
#define HIK_IVS2HIK_LIB_S_FAIL                  0               // 失败	        
#define HIK_IVS2HIK_LIB_E_PARA_NULL	            0x80000000	    // 参数指针为空	    
#define HIK_IVS2HIK_LIB_E_MEM_NULL	            0x80000001	    // 传入的内存为空   
#define HIK_IVS2HIK_LIB_E_MEM_OVER	            0x80000002	    // 内存溢出	
#define HIK_IVS2HIK_LIB_E_SIZE                  0x80000003 
#define HIK_IVS2HIK_LIB_E_FILE_OPERATION        0x80000004


typedef enum _HIK_PIC_MODE_
{
    I_FRAME_MODE  = 0x00001001,
    III_FRAME_MODE,              
    P_FRAME_MODE,                
    BP_FRAME_MODE,              
    BBP_FRAME_MODE,
	AUDIO_I_FRAME_MODE,
	AUDIO_P_FRAME_MODE,
	VCA_IVS_MODE = 0x00003001,  //智能视频监控信息
	FI_MODE = 0x00004001,  //人脸检测信息
	VCA_ITS_MODE = 0x00005001,  //智能交通信息
	VCA_IAS_MODE = 0x00006001  //智能ATM信息
}HIK_PIC_MODE;

// 块类型定义
typedef enum _HIK_BLOCK_TYPE_
{
	AUDIO_I_BLOCK = 0x00001001,
    AUDIO_P_BLOCK,
    VIDEO_I_BLOCK,
    VIDEO_P_BLOCK,
    VIDEO_B_BLOCK,
	IVS_M_BLOCK   = 0x00003001, //智能Metadata信息
	IVS_E_BLOCK,                //智能Event信息
	IVS_R_BLOCK,                 //智能Rule信息
 	IVS_S_BLOCK,                 //智能事件状态Event_list信息
	FI_BLOCK      = 0x00004001, //人脸识别
	ITS_M_BLOCK   = 0x00005001, //智能交通Metadata信息
	ITS_AID_BLOCK,              //智能交通AIDdata信息
	ITS_TPS_BLOCK,              //智能交通TPSdata信息
	IAS_M_BLOCK   = 0x00006001, //智能ATM Metadata信息
	IAS_E_BLOCK,                //智能ATM Event信息
	IAS_R_BLOCK                 //智能ATM Rule信息

}HIK_BLOCK_TYPE; 

/* 库与外部进行数据交换的参数结构体*/
typedef struct _IVS_HIK_PROC_PARAM_
{
	struct GROUP_PARAM_STRU
    {
		
		unsigned int   frame_rate;   //输入视频帧率
		unsigned short width;        //图像宽度
        unsigned short height;       //图像高度  

        unsigned int   frame_num;    //当前group中起始帧序号
		HIK_PIC_MODE   pic_mode;     //组模式
        unsigned int   block_num;    //当前组中的包含块的数量，包括音频块和视频块		
	}gp_param;

    struct BLOCK_PARAM_STRU
    {
        
		HIK_BLOCK_TYPE blk_type;        //块类型
        unsigned int   blk_size;        //块大小
		unsigned short blk_version;     //输入块的版本
		unsigned short reserved;		
	}blk_param;
    
	void          *buf_out;       /* 输出数据缓存地址 (IN) */
    unsigned int   buf_out_size;  /* 输出数据缓存大小 (IN) */
	unsigned int   len_out;       /* 输出数据长度     (OUT)*/
}IVS_HIK_PROC_PARAM;


/********************** 封装部分 ******************************/
HRESULT IVS_HIK_FILE_HDR_Create(IVS_HIK_PROC_PARAM *param);

HRESULT IVS_HIK_GROUP_HDR_Create(IVS_HIK_PROC_PARAM *param);

HRESULT IVS_HIK_BLOCK_HDR_Create(IVS_HIK_PROC_PARAM *param);

/********************** 解析部分 ******************************/
HRESULT HIK_IVS_SKIP_FILE_Header(FILE *fp);

HRESULT HIK_IVS_GROUP_HDR_Parse(IVS_HIK_PROC_PARAM *param, FILE *fp);

HRESULT HIK_IVS_GET_BLOCK_Data(IVS_HIK_PROC_PARAM *param, FILE *fp);


#ifdef __cplusplus
}
#endif 

#endif