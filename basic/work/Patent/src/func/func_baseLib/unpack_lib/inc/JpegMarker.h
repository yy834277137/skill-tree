
#ifndef _HIK_JPEG_PARAM_H_
#define _HIK_JPEG_PARAM_H_

#define  DCTSIZE2  64

typedef unsigned char    BYTE;
typedef unsigned char *  PBYTE;
typedef unsigned short   WORD;
typedef unsigned short * PWORD;

// Data destination object for compression
typedef struct
{
	PBYTE ptr;			// next byte to write in buffer
} BS_PTR;

typedef struct
{
	BYTE bits[32];	// bits[k] = # of symbols with codes of length k bits; bits[0] is unused
	BYTE huffval[256];		// The symbols, in order of incr code length
} JHUFF_TBL;

// Master record for a compression instance
typedef struct _JPEG_PARAM_TAG_
{
	JHUFF_TBL dc_huff_tbl_ptrs[2];	// dc huffman coding tables
	JHUFF_TBL ac_huff_tbl_ptrs[2];	// ac huffman coding tables

	WORD	luma_dc_derived_tbls[256][2]; // dc derived tables
	WORD	luma_ac_derived_tbls[256][2]; // ac derived tables

	WORD	chroma_dc_derived_tbls[256][2]; // dc derived tables
	WORD	chroma_ac_derived_tbls[256][2]; // ac derived tables

	WORD quantval[64*2];

} JPEG_MARKER_PARAM;

int jpg_set_quality(JPEG_MARKER_PARAM *cinfo, int quality);
int jpg_write_file_hdr(JPEG_MARKER_PARAM *cinfo, BS_PTR *bs, int width, int height);
int jpg_std_huff_tables(JPEG_MARKER_PARAM *cinfo);
#endif /* _HIK_JPEG_PARAM_H_ */