/*
 *   @file   utl.c
 *
 *   @brief  utility function.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <assert.h>
#include "cif_list.h"
#include "cif_common.h"
#include "vpd_ioctl.h"
#include "pif.h"
#include "hd_type.h"
#include "bind.h"
#include "hd_videoprocess.h"
#include "videoenc.h"
#include "videodec.h"
#include "videoprocess.h"
#include "utl.h"

/*-----------------------------------------------------------------------------*/
/* HDAL						                                                   */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define UTL_MAX_VAL(a,b)        ((a) > (b) ? (a) : (b))

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern vpd_sys_info_t platform_sys_Info;
extern unsigned int gmlib_dbg_level;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*MAX_DATAIN_FD_NUM : video 64ch x 2(dual-chip) + audio 32ch + vpe 16ch(MAX_DATAIN_FOR_VPE_FD_NUM) */
#define MAX_DATAIN_FOR_VPE_FD_NUM    16
#define MAX_DATAIN_FD_NUM	(64 * 2 + 32 + MAX_DATAIN_FOR_VPE_FD_NUM)
#define MAX_DATAIN_FOR_VPE_FD_OFFSET (MAX_DATAIN_FD_NUM - MAX_DATAIN_FOR_VPE_FD_NUM)

UTL_FD_TABLE datain_fd_vpe_table[MAX_DATAIN_FOR_VPE_FD_NUM];

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

UINT32 utl_get_au_grab_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	UINT32 entity_fd = 0;

	//                    entity_type       chip  engine        minor
	entity_fd = ENTITY_FD(VPD_TYPE_AU_GRAB, chip, device_subid, 0);
	return entity_fd;
}

UINT32 utl_get_au_render_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	UINT32 entity_fd = 0;

	//                    entity_type         chip  engine        minor
	entity_fd = ENTITY_FD(VPD_TYPE_AU_RENDER, chip, device_subid, 0);
	return entity_fd;
}

UINT32 utl_get_cap_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	UINT32 entity_fd = 0, chip, engine, minor;

	chip = platform_sys_Info.cap_info[device_subid].chipid;
	engine = ENTITY_FD_ENGINE(platform_sys_Info.cap_info[device_subid].engine_minor);
	minor = ENTITY_FD_MINOR(platform_sys_Info.cap_info[device_subid].engine_minor);

	//                    entity_type   chip  engine  minor
	entity_fd = ENTITY_FD(VPD_TYPE_CAPTURE, chip, engine, (minor + out_subid));
	return entity_fd;
}

UINT32 utl_get_lcd_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	UINT32  lcd_type, entity_fd = 0;



	lcd_type = platform_sys_Info.lcd_info[device_subid].lcd_type;
	//                    entity_type chip  engine minor
	entity_fd = ENTITY_FD(lcd_type,   chip, 0,     0);
	return entity_fd;
}

UINT32 utl_get_di_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	UINT32 entity_fd = 0, chip, minor;

	chip = device_subid / HD_DAL_VIDEOPROC_CHIP_DEV_COUNT;
	minor = device_subid % HD_DAL_VIDEOPROC_CHIP_DEV_COUNT;

	if (chip >= platform_sys_Info.chip_num) {
		GMLIB_ERROR_P("Error, chip_%d is not exist, %s_%d\n", chip,
			      get_device_name(HD_DAL_VIDEOPROC_BASE + device_subid), device_subid);
	}

	//                    entity_type  chip  engine  minor
	entity_fd = ENTITY_FD(VPD_TYPE_DI, chip, 0,      minor);
	return entity_fd;
}

#define VPE_EXTEND_MINOR_MAX_NUM	256
static UINT vpe_exten_minor_max_idx[VPD_MAX_CHIP_NUM] = {0}; // record a max idx which we have used.
static UINT vpe_exten_minor_mapping[VPD_MAX_CHIP_NUM][VPE_EXTEND_MINOR_MAX_NUM];
static INT utl_lookup_vpe_minor(HD_DAL device_subid, HD_IO out_subid)
{
	INT minor_idx = -1;
	UINT idx, key, chip;

	chip = device_subid / HD_DAL_VIDEOPROC_CHIP_DEV_COUNT;
	if (chip >= platform_sys_Info.chip_num || chip >= VPD_MAX_CHIP_NUM) {
		GMLIB_ERROR_P("Error, chip_%d is not exist, %s_%d\n", chip,
			      get_device_name(HD_DAL_VIDEOPROC_BASE + device_subid), device_subid);
		goto ext;
	}
	if (out_subid >= VODPROC_OUTPUT_PORT) {
		GMLIB_ERROR_P("Error, out_subid(%d) is over %d, %s_%d\n", out_subid, (VODPROC_OUTPUT_PORT - 1),
			      get_device_name(HD_DAL_VIDEOPROC_BASE + device_subid), device_subid);
		goto ext;
	}

	/* lookup already assignment */
	key = (device_subid << 16) | (out_subid & 0x00ff);
	for (idx = 0; idx < (vpe_exten_minor_max_idx[chip] + 1); idx++) {
		if (key == vpe_exten_minor_mapping[chip][idx]) {
			minor_idx = idx;
			break;
		}
	}

	/* lookup a new one */
	if (minor_idx == -1) {
		for (idx = vpe_exten_minor_max_idx[chip]; idx < VPE_EXTEND_MINOR_MAX_NUM; idx++) {
			if (vpe_exten_minor_mapping[chip][idx] == 0xffffffff) {
				vpe_exten_minor_mapping[chip][idx] = key;
				vpe_exten_minor_max_idx[chip] = UTL_MAX_VAL(vpe_exten_minor_max_idx[chip], idx);
				minor_idx = idx;
				break;
			}
		}
	}

	/* check err */
	if (minor_idx == -1) {
		GMLIB_ERROR_P("Error, vpe_exten_minor_mapping array is not enough, %s_%d, key(%#x)\n",
			      get_device_name(HD_DAL_VIDEOPROC_BASE + device_subid), device_subid);
		for (idx = 0; idx < VPE_EXTEND_MINOR_MAX_NUM; idx++) {
			GMLIB_ERROR_P("    chip(%d) idx(%d) key(%#x)\n", chip, idx, vpe_exten_minor_mapping[chip][idx]);
		}
	}
ext:
	GMLIB_L1_P("vpe dev_subid(%d) out_subid(%d) chip(%d) minor_idx(%#x)!\n", device_subid, out_subid, chip, minor_idx);
	return minor_idx;
}

INT utl_get_vpe_minor(HD_DAL device_subid, HD_IO out_subid)
{
	INT minor;

	minor = utl_lookup_vpe_minor(device_subid, out_subid);
	return minor;
}

UINT32 utl_get_vpe_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	UINT32 entity_fd = 0, chip, minor, engine;
	INT fd_idx;

	fd_idx = device_subid;
	engine = 0; /* VPE Full engine */
	//engine = 1; /* VPE Lite engine */

	chip = fd_idx / HD_DAL_VIDEOPROC_CHIP_DEV_COUNT;

	minor = utl_get_vpe_minor(device_subid, out_subid);
	if (minor < 0) {
		goto ext;
	}

	//                    entity_type   chip  engine        minor
	entity_fd = ENTITY_FD(VPD_TYPE_VPE, chip, engine, minor);
ext:
	return entity_fd;
}

#define DISP_OSG_START		0
#define DISP_OSG_END		9
#define ENC_OSG_START		10
#define ENC_OSG_END			64

UINT32 utl_get_disp_osg_index(HD_DAL device_subid)
{
	if (device_subid > DISP_OSG_END)
		GMLIB_PRINT_P("%s: Error to get osg index %d\n", __FUNCTION__, device_subid);
	return device_subid;
}

UINT32 utl_get_enc_osg_index(HD_DAL device_subid)
{
	if (device_subid > ENC_OSG_END - ENC_OSG_START)
		GMLIB_PRINT_P("%s: Error to get osg index %d( + %d)\n", __FUNCTION__, device_subid, ENC_OSG_START);
	return device_subid + ENC_OSG_START;
}


UINT32 utl_get_disp_osg_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	return ENTITY_FD(VPD_TYPE_OSG, chip, 0, utl_get_disp_osg_index(device_subid));
}

UINT32 utl_get_enc_osg_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	return ENTITY_FD(VPD_TYPE_OSG, chip, 0, utl_get_enc_osg_index(out_subid));
}

UINT32 utl_get_enc_osg_entity_fd2(HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	return ENTITY_FD(VPD_TYPE_OSG, device_subid, 0, (utl_get_enc_osg_index(device_subid) + out_subid));
}

UINT32 utl_get_dec_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	UINT32 entity_fd = 0, chip;

	chip = device_subid;
	if (chip >= platform_sys_Info.chip_num) {
		GMLIB_ERROR_P("Error, chip_%ld is not exist, %s_%d\n", chip,
			      get_device_name(HD_DAL_VIDEODEC_BASE + device_subid), device_subid);
	}
	//                    entity_type       chip  engine minor
	entity_fd = ENTITY_FD(VPD_TYPE_DECODER, chip, 0,     out_subid);  // NOTE:use the device_subid = chipid
	return entity_fd;
}

UINT32 utl_get_enc_entity_fd(HD_DAL device_subid, HD_IO in_subid, VOID *param)
{
	UINT32 entity_fd = 0, enc_type = (UINT32)(uintptr_t) param, chip;

	chip = device_subid;
	if (chip >= platform_sys_Info.chip_num) {
		GMLIB_ERROR_P("Error, chip_%ld is not exist, %s_%d\n", chip,
			      get_device_name(HD_DAL_VIDEOENC_BASE + device_subid), device_subid);
	}

	switch (enc_type) {
	case 0:  // h26x
		//                    entity_type        chip  engine minor
		entity_fd = ENTITY_FD(VPD_TYPE_H26X_ENC, chip, 0,     in_subid);  // NOTE:use the device_subid = chipid
		break;
	case 1:  // mjpeg
		//                    entity_type        chip  engine minor
		entity_fd = ENTITY_FD(VPD_TYPE_JPEG_ENC, chip, 0,     in_subid);  // NOTE:use the device_subid = chipid
		break;
	}
	return entity_fd;
}

static UINT32 utl_get_datain_for_vpe_entity_fd(HD_PATH_ID path_id)
{
	UINT32 entity_fd = 0;
	INT i, is_find = 0, is_add = 0;

	for (i = 0; i < MAX_DATAIN_FOR_VPE_FD_NUM; i++) {
		if (datain_fd_vpe_table[i].path_id == path_id) {
			entity_fd = datain_fd_vpe_table[i].entity_fd;
			is_find = 1;
			goto exit;
		}
	}

	for (i = 0; i < MAX_DATAIN_FOR_VPE_FD_NUM; i++) {
		if (datain_fd_vpe_table[i].path_id == 0) {
			datain_fd_vpe_table[i].path_id = path_id;
			datain_fd_vpe_table[i].entity_fd = ENTITY_FD(VPD_TYPE_DATAIN, 0, 0, (i + MAX_DATAIN_FOR_VPE_FD_OFFSET));
			entity_fd = datain_fd_vpe_table[i].entity_fd;
			is_add = 1;
			goto exit;
		}
	}

exit:
	if ((is_find == 0) && (is_add == 0)) {
		GMLIB_ERROR_P("datain for vpe dynamic fd exceed. path_id(%#lx)\n", path_id);
		entity_fd = 0;
	}
	return entity_fd;
}

UINT32 utl_get_datain_entity_fd(HD_PATH_ID path_id)
{
	UINT32 entity_fd = 0, chip, minor, device_subid, out_subid;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id;

	device_subid = bd_get_dev_subid(dev_id);
	out_id = HD_GET_OUT(path_id);
	out_subid = BD_GET_OUT_SUBID(out_id);

	/* video 128ch x 2(dual-chip) + audio 32ch + vpe 16ch(MAX_DATAIN_FOR_VPE_FD_NUM) */
	switch (bd_get_dev_baseid(dev_id)) {
	case HD_DAL_VIDEOPROC_BASE:
		entity_fd = utl_get_datain_for_vpe_entity_fd(path_id);
		break;
	case HD_DAL_VIDEODEC_BASE:
		chip = device_subid; // chip is same with device_subid
		minor = (platform_sys_Info.chip_num * out_subid) + chip;

		//                    entity_type      chip  engine minor
		entity_fd = ENTITY_FD(VPD_TYPE_DATAIN, 0,    0,     minor);  // follow decoder setting
		break;
	case HD_DAL_AUDIODEC_BASE:
		minor = out_subid;

		//                    entity_type      chip  engine  minor
		entity_fd = ENTITY_FD(VPD_TYPE_DATAIN, 0,    0, (minor + (VDODEC_OUTPUT_PORT * 2)));
		break;
	default:
		minor = out_subid;
		//                    entity_type      chip  engine minor
		entity_fd = ENTITY_FD(VPD_TYPE_DATAIN, 0,    0,     minor);
		GMLIB_ERROR_P("internal_err: Not support for this deviceid(%d)\n", dev_id);
		break;
	}
	return entity_fd;
}

UINT32 utl_get_dataout_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param)
{
	UINT32 entity_fd = 0, media_type, chip, minor;

	if (param == NULL) {
		// video data_out
		media_type = EM_TYPE_VIDEO_DEVICE;
		chip = device_subid; // chip is same with device_subid
		minor = (platform_sys_Info.chip_num * out_subid) + chip;
		if (minor >= (HD_VIDEOENC_MAX_IN * 2)) {
			GMLIB_ERROR_P("dataout minor over %d, videoenc: device_subid(%d) out_subid(%d) entity_fd(%#x)\n",
							HD_VIDEOENC_MAX_IN * 2, device_subid, out_subid, entity_fd);
			goto ext;
		}
	} else {
		// audio data_out
		media_type = (UINT32)(uintptr_t) param;
		minor = out_subid;
	}

	if (media_type == EM_TYPE_AUDIO_DEVICE) {
		//                    entity_type       chip engine minor
		entity_fd = ENTITY_FD(VPD_TYPE_DATAOUT, 0,   0, (minor + (HD_VIDEOENC_MAX_IN * 2)));      //Dual case: MAX_VDOENC_PORT x 2
	} else if (media_type == EM_TYPE_VIDEO_DEVICE) { // EM_TYPE_VIDEO_DEVICE
		//                    entity_type       chip engine minor
		entity_fd = ENTITY_FD(VPD_TYPE_DATAOUT, 0,   0,     minor);  // chip alway 0
	} else {
		GMLIB_ERROR_P("internal_err: media_type(%lx)\n", media_type);
		entity_fd = ENTITY_FD(VPD_TYPE_DATAOUT, 0,   0,     minor);  // chip alway 0
	}
ext:
	return entity_fd;
}

HD_RESULT utl_init(void)
{
	UINT chip;

	memset(datain_fd_vpe_table, 0x0, sizeof(datain_fd_vpe_table));
	memset(vpe_exten_minor_mapping, 0xff, sizeof(vpe_exten_minor_mapping));
	for (chip = 0; chip < VPD_MAX_CHIP_NUM; chip++) {
		vpe_exten_minor_max_idx[chip] = 0;
	}
	return HD_OK;
}

HD_RESULT utl_uninit(void)
{
	memset(datain_fd_vpe_table, 0x0, sizeof(datain_fd_vpe_table));
	return HD_OK;
}
