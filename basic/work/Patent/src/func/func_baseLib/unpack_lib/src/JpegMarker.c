#include <assert.h>
#include <memory.h>
#include "JpegMarker.h"

/* These marker codes are exported since applications and data source modules
* are likely to want to use them.
*/
typedef enum 
{
	M_SOF0  = 0xc0,
	M_SOF1  = 0xc1,
	M_SOF2  = 0xc2,
	M_SOF3  = 0xc3,
	
	M_SOF5  = 0xc5,
	M_SOF6  = 0xc6,
	M_SOF7  = 0xc7,
	
	M_JPG   = 0xc8,
	M_SOF9  = 0xc9,
	M_SOF10 = 0xca,
	M_SOF11 = 0xcb,
	
	M_SOF13 = 0xcd,
	M_SOF14 = 0xce,
	M_SOF15 = 0xcf,
	
	M_DHT   = 0xc4,
	
	M_DAC   = 0xcc,
	
	M_RST0  = 0xd0,
	M_RST1  = 0xd1,
	M_RST2  = 0xd2,
	M_RST3  = 0xd3,
	M_RST4  = 0xd4,
	M_RST5  = 0xd5,
	M_RST6  = 0xd6,
	M_RST7  = 0xd7,
	
	M_SOI   = 0xd8,
	M_EOI   = 0xd9,
	M_SOS   = 0xda,
	M_DQT   = 0xdb,
	M_DNL   = 0xdc,
	M_DRI   = 0xdd,
	M_DHP   = 0xde,
	M_EXP   = 0xdf,
	
	M_APP0  = 0xe0,
	M_APP1  = 0xe1,
	M_APP2  = 0xe2,
	M_APP3  = 0xe3,
	M_APP4  = 0xe4,
	M_APP5  = 0xe5,
	M_APP6  = 0xe6,
	M_APP7  = 0xe7,
	M_APP8  = 0xe8,
	M_APP9  = 0xe9,
	M_APP10 = 0xea,
	M_APP11 = 0xeb,
	M_APP12 = 0xec,
	M_APP13 = 0xed,
	M_APP14 = 0xee,
	M_APP15 = 0xef,
	
	M_JPG0  = 0xf0,
	M_JPG13 = 0xfd,
	M_COM   = 0xfe,
	
	M_TEM   = 0x01,
	
	M_ERROR = 0x100
} JPEG_MARKER;


#if 1				/* This table is not actually needed in v6a */

const int jpeg_zigzag_order[DCTSIZE2] = {
	0,  1,  5,  6, 14, 15, 27, 28,
	2,  4,  7, 13, 16, 26, 29, 42,
	3,  8, 12, 17, 25, 30, 41, 43,
	9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
};

#endif

const int jpgenc_natural_order_mux[DCTSIZE2+16] = {
	0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63,
	63, 63, 63, 63, 63, 63, 63, 63, /* extra entries for safety in decoder */
	63, 63, 63, 63, 63, 63, 63, 63
};


/*
* Compute the derived values for a Huffman table.
* This routine also performs some validation checks on the table.
*
* Note this is also used by jcphuff.c.
*/
void jpeg_make_c_derived_tbl (JHUFF_TBL *htbl, unsigned short * dtbl, int isDC)
{
	int p, i, l, lastp, si, maxsymbol;
	char huffsize[257];
	unsigned int huffcode[257];
	unsigned int code;

	// Figure C.1: make table of Huffman code length for each symbol
	p = 0;
	for (l = 1; l <= 16; l++) {
		i = (int) htbl->bits[l];
		assert(!(i < 0 || p + i > 256)); // protect against table overrun
		while (i--)
			huffsize[p++] = (char) l;
	}
	huffsize[p] = 0;
	lastp = p;

	// Figure C.2: generate the codes themselves
	// We also validate that the counts represent a legal Huffman code tree.

	code = 0;
	si = huffsize[0];
	p = 0;
	while (huffsize[p]) 
	{
		while (((int) huffsize[p]) == si) 
		{
			huffcode[p++] = code;
			code++;
		}
		// code is now 1 more than the last code used for codelength si; but
		// it must still fit in si bits, since no code is allowed to be all ones.
		assert(!((int)code >= (1 << si)));
		code <<= 1;
		si++;
	}

	/* Figure C.3: generate encoding tables */
	/* These are code and size indexed by symbol value */

	/* Set all codeless symbols to have code length 0;
	* this lets us detect duplicate VAL entries here, and later
	* allows emit_bits to detect any attempt to emit such symbols.
	*/
	memset(dtbl, 0, 2*256*sizeof(short));

	/* This is also a convenient place to check for out-of-range
	* and duplicated VAL entries.  We allow 0..255 for AC symbols
	* but only 0..15 for DC.  (We could constrain them further
	* based on data depth and mode, but this seems enough.)
	*/
	maxsymbol = isDC ? 15 : 255;

	for (p = 0; p < lastp; p++) {
		i = htbl->huffval[p];
		assert(! (i < 0 || i > maxsymbol));
		dtbl[i*2  ] = huffcode[p] & ((1 << huffsize[p]) - 1);
		dtbl[i*2+1] = huffsize[p];
	}
}

// Huffman table setup routines
void add_huff_table (JHUFF_TBL *htblptr, const BYTE *bits, const BYTE *val, 
					 unsigned short dtbl[256][2], int isDC)
{
	int nsymbols, len;

	// Copy the number-of-symbols-of-each-code-length counts
	memcpy(htblptr->bits, bits, sizeof(htblptr->bits));

	/* Validate the counts.  We do this here mainly so we can copy the right
	* number of symbols from the val[] array, without risking marching off
	* the end of memory.  jchuff.c will do a more thorough test later.
	*/
	nsymbols = 0;
	for (len = 1; len <= 16; len++)
		nsymbols += bits[len];

	memcpy(htblptr->huffval, val, nsymbols * sizeof(BYTE));

	jpeg_make_c_derived_tbl(htblptr, dtbl[0], isDC);
}


/* Set up the standard Huffman tables (cf. JPEG standard section K.3) */
/* IMPORTANT: these are only valid for 8-bit data precision! */
int jpg_std_huff_tables (JPEG_MARKER_PARAM *cinfo)
{
	static const BYTE bits_dc_luminance[17] = { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 
		1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
	static const BYTE val_dc_luminance[] =   { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

	static const BYTE bits_dc_chrominance[17] = { /* 0-base */ 0, 0, 3, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
	static const BYTE val_dc_chrominance[] =   { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

	static const BYTE bits_ac_luminance[17] =   { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4,
		3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
	static const BYTE val_ac_luminance[] =
	{ 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa };

	static const BYTE bits_ac_chrominance[17] =
	{ /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
	static const BYTE val_ac_chrominance[] =
	{ 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa };

	add_huff_table(&cinfo->dc_huff_tbl_ptrs[0], bits_dc_luminance, val_dc_luminance, 
		cinfo->luma_dc_derived_tbls, 1);

	add_huff_table(&cinfo->ac_huff_tbl_ptrs[0], bits_ac_luminance, val_ac_luminance, 
		cinfo->luma_ac_derived_tbls, 0);

	add_huff_table(&cinfo->dc_huff_tbl_ptrs[1], bits_dc_chrominance, val_dc_chrominance, 
		cinfo->chroma_dc_derived_tbls, 1);

	add_huff_table(&cinfo->ac_huff_tbl_ptrs[1], bits_ac_chrominance, val_ac_chrominance, 
		cinfo->chroma_ac_derived_tbls, 0);

	return 0;
}

/* Define a quantization table equal to the basic_table times
* a scale factor (given as a percentage).
* If force_baseline is TRUE, the computed quantization table entries
* are limited to 1..255 for JPEG baseline compatibility.
*/
void jpeg_add_quant_table (unsigned short * quantval, PBYTE basic_table, int scale_factor)
{
	int i;
	int temp;

	for (i = 0; i < DCTSIZE2; i++) {
		temp = (int)((basic_table[i] * scale_factor + 50U) / 100U);
		/* limit the values to the valid range */
		if (temp <= 0L ) temp = 1L;
		if (temp > 255L) temp = 255L;		/* limit to baseline range if requested */
		quantval[i] = (WORD) temp;
	}	
}


/* Set or change the 'quality' (quantization) setting, using default tables
* and a straight percentage-scaling quality scale.  In most cases it's better
* to use jpeg_set_quality (below); this entry point is provided for
* applications that insist on a linear percentage scaling.
*/
void jpeg_set_linear_quality (JPEG_MARKER_PARAM *cinfo, int scale_factor)
{
	/* These are the sample quantization tables given in JPEG spec section K.1.
	* The spec says that the values given produce "good" quality, and
	* when divided by 2, "very good" quality.
	*/
	static BYTE std_luminance_quant_tbl[DCTSIZE2] = 
	{
		16,  11,  10,  16,  24,  40,  51,  61,
		12,  12,  14,  19,  26,  58,  60,  55,
		14,  13,  16,  24,  40,  57,  69,  56,
		14,  17,  22,  29,  51,  87,  80,  62,
		18,  22,  37,  56,  68, 109, 103,  77,
		24,  35,  55,  64,  81, 104, 113,  92,
		49,  64,  78,  87, 103, 121, 120, 101,
		72,  92,  95,  98, 112, 100, 103,  99
	};
	static BYTE std_chrominance_quant_tbl[DCTSIZE2] = 
	{
		17,  18,  24,  47,  99,  99,  99,  99,
		18,  21,  26,  66,  99,  99,  99,  99,
		24,  26,  56,  99,  99,  99,  99,  99,
		47,  66,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99
	};

	// Set up two quantization tables using the specified scaling
	jpeg_add_quant_table(cinfo->quantval, std_luminance_quant_tbl  , scale_factor);
	jpeg_add_quant_table(cinfo->quantval+64, std_chrominance_quant_tbl, scale_factor);
}


/* Convert a user-specified quality rating to a percentage scaling factor
* for an underlying quantization table, using our recommended scaling curve.
* The input 'quality' factor should be 0 (terrible) to 100 (very good).
*/
int jpeg_quality_scaling (int quality)
{

	if (quality < 50)
		quality = 5000 / quality;
	else
		quality = 200 - quality * 2;

	return quality;
}


/* Set or change the 'quality' (quantization) setting, using default tables.
* This is the standard quality-adjusting entry point for typical user
* interfaces; only those who want detailed control over quantization tables
* would use the preceding three routines directly.
*/
int jpg_set_quality(JPEG_MARKER_PARAM *cinfo, int quality)
{

	// 设置HUFF码表，此函数只用运行一次，目前把它放在这里，是为了简化接口
//	std_huff_tables(cinfo);	

	// Convert user 0-100 rating to percentage scaling
	quality = jpeg_quality_scaling(quality);

	// Set up standard quality tables
	jpeg_set_linear_quality(cinfo, quality);

	return 1;
}

// Emit a marker code
void emit_marker (BS_PTR *bs, int mark)
{
	*bs->ptr ++ = 0xFF;
	*bs->ptr ++ = (BYTE)mark;
}


// Emit a 2-byte integer; these are always MSB first in JPEG files
void emit_2bytes (BS_PTR *bs, int value)
{
	*bs->ptr ++ = (BYTE)(value >> 8);
	*bs->ptr ++ = (BYTE)value;
}

// Emit a DQT marker, Returns the precision used (0 = 8bits, 1 = 16bits) for baseline checking 
void emit_dqt (BS_PTR *bs, unsigned short * qtbl, int index)
{
	int i;
	
	emit_marker(bs, M_DQT);
	
	emit_2bytes(bs, DCTSIZE2 + 1 + 2);
	
	*bs->ptr ++ = index;
	
	for (i = 0; i < DCTSIZE2; i++) // The table entries must be emitted in zigzag order.
		*bs->ptr ++ = (BYTE)qtbl[jpgenc_natural_order_mux[i]];
}


// Emit a DHT marker
void emit_dht (BS_PTR *bs, JHUFF_TBL * htbl, int index)
{
	int length, i;
	
	emit_marker(bs, M_DHT);
	
	length = 0;
	for (i = 1; i <= 16; i++)
		length += htbl->bits[i];
	
	emit_2bytes(bs, length + 2 + 1 + 16);
	*bs->ptr ++ = index;
	
	for (i = 1; i <= 16; i++)
		*bs->ptr ++ = htbl->bits[i];
	
	for (i = 0; i < length; i++)
		*bs->ptr ++ = htbl->huffval[i];
}

// Emit a SOF marker
void emit_sof (BS_PTR *bs, int code, int width, int height)
{
	int ci;
	
	emit_marker(bs, code);
	
	emit_2bytes(bs, 3 * 3/*cinfo->num_components*/ + 2 + 5 + 1); /* length */
	
	*bs->ptr ++ = 8;  //cinfo->data_precision
	emit_2bytes(bs, height);
	emit_2bytes(bs, width);
	
	*bs->ptr ++ = 3;//cinfo->num_components
	
	// Y
	*bs->ptr ++ = 0;
	*bs->ptr ++ = (2 << 4) + 2;
	*bs->ptr ++ = 0;

	// UV 
	for(ci = 1; ci < 3; ci ++)
	{
		*bs->ptr ++ = ci;
		*bs->ptr ++ = (1 << 4) + 1; // (compptr->h_samp_factor << 4) + compptr->v_samp_factor
		*bs->ptr ++ = 1; // compptr->quant_tbl_no
	}
}

// Emit a SOS marker
void emit_sos (BS_PTR *bs)
{
	int i, td, ta;
	
	emit_marker(bs, M_SOS);
	
	emit_2bytes(bs, 2 * 3 + 2 + 1 + 3); // length
	
	*bs->ptr ++ = 3;
	
	for (i = 0; i < 3; i++) 
	{
		*bs->ptr ++ = i;
		td = (i > 0); //compptr->dc_tbl_no;
		ta = (i > 0); // compptr->ac_tbl_no
		*bs->ptr ++ = (td << 4) + ta;
	}
	
	*bs->ptr ++ = 0;			// Ss
	*bs->ptr ++ = DCTSIZE2-1;	// Se
	*bs->ptr ++ = 0;			// (cinfo->Ah << 4) + cinfo->Al
}


/*
* Write a special marker.
* This is only recommended for writing COM or APPn markers.
* Must be called after jpeg_start_compress() and before
* first call to jpeg_write_scanlines() or jpeg_write_raw_data().
*/
int jpeg_write_marker (BS_PTR *bs, int marker, const BYTE *dataptr, int datalen)
{
	//write_marker_header(cinfo, marker, datalen);
	if (datalen > (unsigned int) 65533)		// safety check
		return 0;
	
	emit_marker(bs, marker);
	
	emit_2bytes(bs, (int) (datalen + 2));	// total length
	
	memcpy(bs->ptr, dataptr, datalen); 
	bs->ptr += datalen;
	return 1;
}

/*
* Write datastream header.
* This consists of an SOI and optional APPn markers.
* We recommend use of the JFIF marker, but not the Adobe marker,
* when using YCbCr or grayscale data.  The JFIF marker should NOT
* be used for any other JPEG colorspace.  The Adobe marker is helpful
* to distinguish RGB, CMYK, and YCCK colorspaces.
* Note that an application can write additional header markers after
* jpeg_start_compress returns.
*/
int jpg_write_file_hdr(JPEG_MARKER_PARAM *cinfo, BS_PTR *bs, int width, int height)
{
	emit_marker(bs, M_SOI);	// first the SOI
	
	// 目前只输入 comment 信息, 考虑增加时间、版权等信息
	if(!jpeg_write_marker (bs, M_COM, (const BYTE *)"hikvision",9))
		return 0;
	
	// write frame header
	emit_dqt(bs, cinfo->quantval   , 0); // compptr->quant_tbl_no
	emit_dqt(bs, cinfo->quantval+64, 1); // compptr->quant_tbl_no
	
	emit_sof(bs, M_SOF0, width, height); // SOF code for baseline implementation
	
	// write scan header
	// Sequential mode: need both DC and AC tables

	emit_dht(bs, &cinfo->dc_huff_tbl_ptrs[0], 0);
	emit_dht(bs, &cinfo->ac_huff_tbl_ptrs[0], 0+0x10);
	emit_dht(bs, &cinfo->dc_huff_tbl_ptrs[1], 1);
	emit_dht(bs, &cinfo->ac_huff_tbl_ptrs[1], 1+0x10);
	
	emit_sos(bs);

	return 1;
}


