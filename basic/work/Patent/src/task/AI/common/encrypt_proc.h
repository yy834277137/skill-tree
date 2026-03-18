#ifndef _ENCRYPT_PROC_H_
#define _ENCRYPT_PROC_H_

//#include <platform_sdk.h>

#include "CRY_lib.h"


#define OTP_OK              (0)     
#define OTP_FAILED          (-1)
#define OTP_E_PTR_NULL      0x6001    //传入空指针
#define OTP_E_GET_KEY_ERR   0x6002    //获取密文失败
#define OTP_E_DICIPHER_ERR  0x6003    //硬件解密失败
#define OTP_E_SET_KEY_ERR   0x6004    //设置明文失败
#define OTP_E_HISI_FUNC_ERR 0x6005    //调用海思SDK接口返回失败


/*打印级别*/
typedef enum
{
	PRT_LVL_NON = 0, 	// 无 不输出任何打印
	PRT_LVL_DBG = 1,     //调试信息，debug时打印，release时不打印
	PRT_LVL_LOG = 2,     //日志信息，debug时打印，release时打印，状态切换时可以使用，每帧处理中不能使用
	PRT_LVL_WRN = 3,
	PRT_LVL_ERR = 4,
	PRT_LVL_DPT = 7,	//直接打印
	PRT_LVL_NUM = 8		//不可修改
}DSP_PRT_LVL;

#define PRT_MNG_MAX_MDL_NUM		(8192)
#define PRT_REDIR_ALL			(0xffffffff)


void dsp_print(DSP_PRT_LVL level, const char *modulename, const char *filename, const int line,const char *format, ...);


#define DSP_OTP_DBG(arg...)	dsp_print(PRT_LVL_DBG,"OTP", __FILE__, __LINE__, ##arg)
#define DSP_OTP_LOG(arg...)	dsp_print(PRT_LVL_LOG,"OTP", __FILE__, __LINE__, ##arg)
#define DSP_OTP_ERR(arg...)	dsp_print(PRT_LVL_ERR,"OTP", __FILE__, __LINE__, ##arg)
#define DSP_OTP_ASSERT_RET(c,r) 	if(c){DSP_OTP_ERR("ASSERT ERROR!\n"); return r;}



#define ALG_KEY_LEN			(32)		//32byte
#define DSP_OTP_CTRL_MAGIC	(0x77712dde)
#define CRY_OK				(1)

typedef struct
{
	int magic;
	unsigned long long	ciper_key_phy_addr;			
	unsigned char 	*ciper_key_vir_addr;		
	unsigned long long	plain_key_phy_addr;			
	unsigned char 	*plain_key_vir_addr;		
	unsigned int 	hi_unf_cipher_handle;		
}DSP_OTP_CTRL;


/**************************************************************************
* 功  能：OTP初始化
* 参  数:
*         handle             -I        句柄
*        
* 返回值：状态码
**************************************************************************/
int encrypt_init(void **handle);


/**************************************************************************
* 功  能：OTP去初始化
* 参  数:
*         handle             -I        句柄
*         
* 返回值：状态码
**************************************************************************/
int encrypt_deinit(void *handle);


/**************************************************************************
* 功  能：OTP处理
* 参  数:
*         handle             -I        句柄
* 返回值：状态码
**************************************************************************/
int encrypt_proc(void *handle);


#endif
