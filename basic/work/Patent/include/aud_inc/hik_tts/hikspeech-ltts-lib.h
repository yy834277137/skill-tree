/***********************************************************************************************************************
*
* 版权信息：版权所有(c) 2017-2020, 杭州海康威视研究院, 保留所有权利
*
* 文件名称：hikspeech-ltts-lib.h
* 文件标识：_HIKSPEECH_LTTS_LIB_H_
* 摘    要：语音合成LTTS V1.0
*
* 当前版本：1.0.1
* 作    者：梁文杰
* 日    期：2021-08-20
*
* 当前版本：1.0.0
* 作    者：王兮楼
* 日    期：2021-03-06
* 备    注：初始版本
***********************************************************************************************************************/

#ifndef _HIKSPEECH_LTTS_LIB_H_
#define _HIKSPEECH_LTTS_LIB_H_


#ifdef __cplusplus
extern "C" {
#endif


#include "hikspeech-common.h"
#include "hiknlu_common.h"

#define HIKSPEECH_LTTS_LIB_E_PTR_NULL 				0x81F10621
#define HIKSPEECH_LTTS_LIB_TEXT_OUT_OF_LENGTH 		0x81F10622
#define HIKSPEECH_LTTS_LIB_TEXT_UNCORRECT			0x81F10623      //非法输入文本(长文本需要标点)
#define HIKSPEECH_LTTS_LIB_KEY_PARAM_ERR			0x81F10624		//参数设置错误
#define HIKSPEECH_LTTS_LIB_E_MEM_OUT				0x81F10625		// 内存不足
#define HIKSPEECH_LTTS_LIB_INPUT_TOO_LONG			0x81F10626		// 輸入字符過長


#define LTTS_MAX_SENT_TOKENS  					   3000
// #define PP_MAX_SENT_TOKENS    					   3000
#define PY_BYTE_EVERY_TIME    					   236
// #define LTTS_MAX_SAMPLE_POINT 					   80000
// #define LTTS_MAX_OUTPUT_SAMPLE_NUM                 960000        //限制最大输出长度，最大输出对应60s的输出音频



typedef struct _HIKSPEECH_LTTS_MODEL_PARAM_
{
	char fs2_vocab_path[HIKSPEECH_MAX_STR_LEN];
	char fs2_py2ph_path[HIKSPEECH_MAX_STR_LEN];
	char fs2_model_path[HIKSPEECH_MAX_STR_LEN];
	
	char melgan_model_path[HIKSPEECH_MAX_STR_LEN];
	// char tn_model_path[HIKSPEECH_MAX_STR_LEN];
	// char tn_vocab_path[HIKSPEECH_MAX_STR_LEN];
	// char tn_vocab_path_dec[HIKSPEECH_MAX_STR_LEN];
	// char pp_model_path[HIKSPEECH_MAX_STR_LEN];
	// char pp_vocab_path[HIKSPEECH_MAX_STR_LEN];
	char py_model_path[HIKSPEECH_MAX_STR_LEN];

	int speaker;

	//std::map<std::string, std::vector<std::string>> vocab_map_;

	void* vocab_map;
}HIKSPEECH_LTTS_MODEL_PARAM;


/***********************************************************************************************************************
*  TTS算法库版本号及编译日期定义
***********************************************************************************************************************/


typedef struct _LTTS_MEM_TAB_
{
	size_t                     size;                     // ��BYTEΪ��λ���ڴ���С
    HIKSPEECH_MEM_ALIGNMENT    alignment;                // �ڴ���������, ����Ϊ128
    HIKSPEECH_MEM_SPACE        space;                    // �ڴ������ռ� 
    HIKSPEECH_MEM_ATTRS        attrs;                    // �ڴ����� 
    void                       *base;                    // ���������ڴ�ָ�� 
    HIKSPEECH_MEM_PLAT         plat;                     // ƽ̨
}LTTS_MEM_TAB;



/***********************************************************************************************************************
*  LTTS 
***********************************************************************************************************************/
typedef struct _HIKSPEECH_LTTS_MEM_TAB_
{
	LTTS_MEM_TAB ltts_mem_tab;
	LTTS_MEM_TAB melgan_mem_tab;
	NLU_MEM_TAB pinyin_mem_tab;
	// HIKNLU_MEM_TAB pp_mem_tab;
	// HIKNLU_MEM_TAB tn_mem_tab;
	HIKNLU_MEM_TAB fs2_mem_tab;
}HIKSPEECH_LTTS_MEM_TAB;

//包含五个Model_handle
typedef struct 
{
	// void *tn_model_handle;
	// void *pp_model_handle;
	void *pinyin_model_hdle;
	void *fs2_model_handle;
	void *melgan_model_handle;

	void* vocab_map;
}HIKSPEECH_LTTS_MODEL_HANDLE;


typedef struct _HIKSPEECH_LTTS_CONFIG_
{
	int speaker;										// 说话人 0 female 1 male
	float speak_ratio;									// 语速	
	float speak_tone;									// 语调
}HIKSPEECH_LTTS_CONFIG;



//
typedef struct _HIKSPEECH_LTTS_HANDLE_
{
	void *fs2_handle;
	void *melgan_handle;
	void *pinyin_handle;
	// void *pp_handle;
	// void *tn_handle;

	int num_mel_bins;
	int sentence_cur_len;
	int *seq_data;
	int *sentence_seq_data;
	int outputs_per_step;
	int sentence_split_eos_flag;

	//taco map 
	void* vocab_map;
	
	//melgan
	void *in_data;
	void *tmp_in_data;
	float *contex_feature;
	short *s16_out;

	float *fs2_output;
	float *mel_fs2_output;

	int is_start;
	int is_end;
	int chunck_num;
	int cur_chunck;
	char chunck_slice[100][300];
}HIKSPEECH_LTTS_HANDLE;

//INPUT
typedef struct _HIKSPEECH_LTTS_INPUT_
{
	char *input_file;
	char ltts_data[LTTS_MAX_SENT_TOKENS];                  	// 输入数据 

	float pace;	// 语速倍率
}HIKSPEECH_LTTS_INPUT;

//OUTPUT
typedef struct _HIKSPEECH_LTTS_OUTPUT_
{
	char *output_file;
	short *output_wav;
	int   output_wav_length;
	int   eos_flag;

	//流式
	int   s16_count;
	int   eos_sentence;
}HIKSPEECH_LTTS_OUTPUT;


HRESULT HIKSPEECH_LTTS_LIB_GetModelMemSize(HIKSPEECH_LTTS_MODEL_PARAM *param, HIKSPEECH_LTTS_MEM_TAB *mem_tab);

HRESULT HIKSPEECH_LTTS_LIB_CreateModel(void *model_hdle, HIKSPEECH_LTTS_MODEL_PARAM *model_param, HIKSPEECH_LTTS_MEM_TAB *model_mem_tab);

HRESULT HIKSPEECH_LTTS_LIB_GetMemSize(void *model_hdle, HIKSPEECH_LTTS_MEM_TAB *mem_tab);

HRESULT HIKSPEECH_LTTS_LIB_Create(void *model_hdle, HIKSPEECH_LTTS_MEM_TAB *mem_tab, void **ltts_handle);

HRESULT HIKSPEECH_LTTS_LIB_Process(void *ltts_handle, HIKSPEECH_LTTS_INPUT* ltts_input, HIKSPEECH_LTTS_OUTPUT* ltts_output);

HRESULT HIKSPEECH_LTTS_LIB_Reset(void *ltts_handle,  HIKSPEECH_LTTS_OUTPUT *ltts_output);

/***********************************************************************************************************************
* 功  能: 释放函数
* 参  数: model_handle                   - I/O   模型句柄
*         ltts_handle                   - I/O   运行句柄
* 返回值: 状态码
***********************************************************************************************************************/
HRESULT HIKSPEECH_LTTS_LIB_Release(void *model_handle, void *ltts_handle);



#ifdef __cplusplus
}
#endif


#endif /* _HIKSPEECH_LTTS_LIB_H_ */
