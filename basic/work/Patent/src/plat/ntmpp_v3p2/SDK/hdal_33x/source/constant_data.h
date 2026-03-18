#ifndef __CONSTANT_DATA_H__

#define USE_BLACK_BS_640x480    1
#define USE_BLACK_BS_480x288    0
#define USE_BLACK_BS_352x288    0
#define USE_BLACK_BS_352x240    1
#define USE_BLACK_BS_80x96      1


typedef struct {
	UINT32 codec_type;
	HD_DIM dim;
	CHAR   *p_data;
	INT    size;
} const_bitstream_t;


#ifndef __CONSTANT_DATA_C__

#if USE_BLACK_BS_640x480
extern const_bitstream_t const_bs_640x480_h264;
extern const_bitstream_t const_bs_640x480_h265;
extern const_bitstream_t const_bs_640x480_jpeg;
#endif

#if USE_BLACK_BS_480x288
extern const_bitstream_t const_bs_480x288_h264;
extern const_bitstream_t const_bs_480x288_h265;
extern const_bitstream_t const_bs_480x288_jpeg;
#endif

#if USE_BLACK_BS_352x288
extern const_bitstream_t const_bs_352x288_h264;
extern const_bitstream_t const_bs_352x288_h265;
extern const_bitstream_t const_bs_352x288_jpeg;
#endif

#if USE_BLACK_BS_352x240
extern const_bitstream_t const_bs_352x240_h264;
extern const_bitstream_t const_bs_352x240_h265;
extern const_bitstream_t const_bs_352x240_jpeg;
#endif

#if USE_BLACK_BS_80x96
extern const_bitstream_t const_bs_80x96_h264;
extern const_bitstream_t const_bs_80x96_h265;
extern const_bitstream_t const_bs_80x96_jpeg;
#endif

#endif

#endif
