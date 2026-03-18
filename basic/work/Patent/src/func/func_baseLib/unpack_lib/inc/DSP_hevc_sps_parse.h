#ifndef _HEVC_SPS_PARSE_H_
#define _HEVC_SPS_PARSE_H_

#include "video_dec_commom.h"

enum NalUnitType
{
	NAL_UNIT_CODED_SLICE_TRAIL_N = 0, // 0
	NAL_UNIT_CODED_SLICE_TRAIL_R,     // 1

	NAL_UNIT_CODED_SLICE_TSA_N,       // 2
	NAL_UNIT_CODED_SLICE_TLA_R,       // 3

	NAL_UNIT_CODED_SLICE_STSA_N,      // 4
	NAL_UNIT_CODED_SLICE_STSA_R,      // 5

	NAL_UNIT_CODED_SLICE_RADL_N,      // 6
	NAL_UNIT_CODED_SLICE_RADL_R,      // 7

	NAL_UNIT_CODED_SLICE_RASL_N,      // 8
	NAL_UNIT_CODED_SLICE_RASL_R,      // 9

	NAL_UNIT_RESERVED_VCL_N10,
	NAL_UNIT_RESERVED_VCL_R11,
	NAL_UNIT_RESERVED_VCL_N12,
	NAL_UNIT_RESERVED_VCL_R13,
	NAL_UNIT_RESERVED_VCL_N14,
	NAL_UNIT_RESERVED_VCL_R15,

	NAL_UNIT_CODED_SLICE_BLA_W_LP,    // 16
	NAL_UNIT_CODED_SLICE_BLA_W_RADL,  // 17
	NAL_UNIT_CODED_SLICE_BLA_N_LP,    // 18
	NAL_UNIT_CODED_SLICE_IDR_W_RADL,  // 19
	NAL_UNIT_CODED_SLICE_IDR_N_LP,    // 20
	NAL_UNIT_CODED_SLICE_CRA,         // 21
	NAL_UNIT_RESERVED_IRAP_VCL22,
	NAL_UNIT_RESERVED_IRAP_VCL23,

	NAL_UNIT_RESERVED_VCL24,
	NAL_UNIT_RESERVED_VCL25,
	NAL_UNIT_RESERVED_VCL26,
	NAL_UNIT_RESERVED_VCL27,
	NAL_UNIT_RESERVED_VCL28,
	NAL_UNIT_RESERVED_VCL29,
	NAL_UNIT_RESERVED_VCL30,
	NAL_UNIT_RESERVED_VCL31,

	NAL_UNIT_VPS,                     // 32
	NAL_UNIT_SPS,                     // 33
	NAL_UNIT_PPS,                     // 34
	NAL_UNIT_ACCESS_UNIT_DELIMITER,   // 35
	NAL_UNIT_EOS,                     // 36
	NAL_UNIT_EOB,                     // 37
	NAL_UNIT_FILLER_DATA,             // 38
	NAL_UNIT_PREFIX_SEI,              // 39
	NAL_UNIT_SUFFIX_SEI,              // 40
	NAL_UNIT_RESERVED_NVCL41,
	NAL_UNIT_RESERVED_NVCL42,
	NAL_UNIT_RESERVED_NVCL43,
	NAL_UNIT_RESERVED_NVCL44,
	NAL_UNIT_RESERVED_NVCL45,
	NAL_UNIT_RESERVED_NVCL46,
	NAL_UNIT_RESERVED_NVCL47,
	NAL_UNIT_UNSPECIFIED_48,
	NAL_UNIT_UNSPECIFIED_49,
	NAL_UNIT_UNSPECIFIED_50,
	NAL_UNIT_UNSPECIFIED_51,
	NAL_UNIT_UNSPECIFIED_52,
	NAL_UNIT_UNSPECIFIED_53,
	NAL_UNIT_UNSPECIFIED_54,
	NAL_UNIT_UNSPECIFIED_55,
	NAL_UNIT_UNSPECIFIED_56,
	NAL_UNIT_UNSPECIFIED_57,
	NAL_UNIT_UNSPECIFIED_58,
	NAL_UNIT_UNSPECIFIED_59,
	NAL_UNIT_UNSPECIFIED_60,
	NAL_UNIT_UNSPECIFIED_61,
	NAL_UNIT_UNSPECIFIED_62,
	NAL_UNIT_UNSPECIFIED_63,
	NAL_UNIT_INVALID,
};

/*
nalu_unit_header
forbidden_zero_bit    f(1)
nal_unit_type         u(6)
nuh_res_zero_bit      u(6)
nuh_temporal_id_plus1 u(3)

NalUnitType 0~31 slice 相关
            32~63 slice 不相关
*/
/*********************************************宏定义***********************************************/
#define MAX_UNIT_NUM 200

#define IS_FRM_START_CODE(marker) ( ( (marker & 0x001F) == 1 || (marker & 0x001F) == 5) && (marker & 0x8000) )

#define IS_VPSSPSPPS_START_CODE(nal_type) ( (nal_type == NAL_UNIT_VPS) || (nal_type == NAL_UNIT_SPS) || (nal_type == NAL_UNIT_PPS))

#define IS_BFRAME_MARKER(marker)  ( (marker & 0xFC00) == 0x9C00 || (marker & 0xF000) == 0xA000)

#define CHECK_NALU_TYPE(marker)    ((marker & 0x0000007e) >> 1)
#define CHECK_TEMPORAL_ID(marker) (((marker & 0x00000700) >> 8) - 1)


typedef struct _USER_INPUT_INFO_
{
    FILE *fp_rawfile;
    FILE *fp_outfile;
    FILE *fp_reffile;
    int  max_dec_frames;
    int  thread_num;
    int  parallel_mode;
}USER_INPUT_INFO;

typedef struct _STREAM_INPUT_INFO_
{
    int width;
    int height;
    int ref_frame_num;
    int bit_width_flag;
    int wpp_flag;
    int log2_ctb_size;
}STREAM_INPUT_INFO;

#if 0
typedef struct _UNI_TIMER_
{
    int timer_started_cnt;
#if defined(WIN32) || defined(WIN64)
    __int64 total_time;
    LARGE_INTEGER start_clock;
    LARGE_INTEGER end_clock;
#else
    long long total_time;
    clock_t start_clock;
    clock_t end_clock;
#endif
}UNI_TIMER;

void InitUNITimer(UNI_TIMER *uni_timer);
void StartUNITimer(UNI_TIMER *uni_timer);
void StopUNITimerAndAccu(UNI_TIMER *uni_timer);
double GetTotalTimeInMs(UNI_TIMER *uni_timer);
double GetAverageTimeInMs(UNI_TIMER *uni_timer);
int GetTimerCnt(UNI_TIMER *uni_timer);

int GetUserInfo(USER_INPUT_INFO *user_info, FILE *fp_config_file);
//int GetStreamInfo(STREAM_INPUT_INFO *stream_info, FILE *fp_rawfile);
int SetDecParam(VIDEO_DEC_PARAM *video_dec_param, USER_INPUT_INFO *user_info, STREAM_INPUT_INFO *stream_info);
int CloseUserInfo(USER_INPUT_INFO *user_info);
int ReadOneFrame(unsigned char *streambuf, int *frm_len, FILE *inpf);

//open_funcs.c
int OPENHEVC_GetPicSizeFromSPS_dsp(unsigned char *nalu_buf, int buf_size, VIDEO_INFO *param);
int OPENHEVC_GetPicSizeFromPPS_dsp(unsigned char *nalu_buf, int buf_size, VIDEO_INFO *param);
int OPENHEVC_GetFrameTypeFromSlice_dsp(unsigned char *nalu_buf, int buf_size);
int OPENHEVC_GetNaluType_dsp(unsigned char *nalu_buf, int buf_size);
#endif
int OPENHEVC_GetPicSizeFromSPS_dsp(unsigned char *nalu_buf, int buf_size, VIDEO_INFO *param);
int OPENHEVC_GetPicSizeFromPPS_dsp(unsigned char *nalu_buf, int buf_size, VIDEO_INFO *param);
int OPENHEVC_GetFrameTypeFromSlice_dsp(unsigned char *nalu_buf, int buf_size);
int OPENHEVC_GetNaluType_dsp(unsigned char *nalu_buf, int buf_size);


#endif//_HEVC_SPS_PARSE_H_



