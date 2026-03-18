/**
	@brief Source file of video decode.\n
	This file contains the functions which is related to video decode in the chip.

	@file videodec.c

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "hdal.h"
#include "vpd_ioctl.h"
#include "trig_ioctl.h"
#include "hdal.h"
#include "videodec.h"
#include "cif_common.h"
#include "vg_common.h"
#include "kflow_videodec.h"
#include "dif.h"
#include "utl.h"
#include "pif_ioctl.h"
#include "pif.h"

#define DECOUT_MAXNUM       2
#define PAGE_SHIFT          12
#define PAGE_SIZE           (1 << PAGE_SHIFT)

/* ID_SRC_FMT & ID_DST_FMT properties */
#define GET_1ST_FMT_VALUE(fmt_prop)    ((fmt_prop) & 0x000000FF)
#define GET_2ND_FMT_VALUE(fmt_prop)    (((fmt_prop) & 0x0000FF00)>>8)
#define GET_3RD_FMT_VALUE(fmt_prop)    (((fmt_prop) & 0x00FF0000)>>16)
#define GET_4TH_FMT_VALUE(fmt_prop)    (((fmt_prop) & 0xFF000000)>>24)
#define FORM_FMT_PROP_VALUE(fmt_1st, fmt_2nd, fmt_3rd, fmt_4th) \
	((fmt_4th) << 24 | (fmt_3rd) << 16 | (fmt_2nd) << 8 | (fmt_1st))

extern int vpd_fd;
extern vpd_sys_info_t platform_sys_Info;
extern UINT32 videodec_max_device_nr;
extern UINT32 videodec_max_device_in;
extern UINT32 videodec_max_device_out;
extern VDODEC_PARAM vdodec_param[VDODEC_DEV];
VDODEC_DEV_PARAM dec_cfg[VDODEC_DEV];
VDODEC_CTRL_PARAM dec_ctrl;
KFLOW_VIDEODEC_HW_SPEC_INFO vdodec_hw_spec_info;

extern int pif_alloc_user_fd_for_hdal(vpd_em_type_t type, int chip, int engine, int minor);
extern void *pif_alloc_video_buffer_for_hdal(int size, int ddr_id, char pool_name[], int alloc_tag);
extern int pif_free_video_buffer_for_hdal(void *pa, int ddr_id, int alloc_tag);
//extern unsigned int pif_create_queue_for_hdal(void);
extern uintptr_t pif_create_queue_for_hdal(void);
//extern int pif_destroy_queue_for_hdal(unsigned int queue_handle);
extern int pif_destroy_queue_for_hdal(uintptr_t queue_handle);
extern int pif_get_left_frame_count(unsigned int fd, unsigned int *p_left_count, unsigned int *p_done_count, unsigned int *p_dti_buf_cnt);
extern int pif_get_left_frame_count_with_next_fd(unsigned int fd, unsigned int next_fd,
		unsigned int *p_left_count, unsigned int *p_done_count, unsigned int *p_dti_buf_cnt);
extern int pif_get_reserved_ref_frame(unsigned int fd, unsigned int *p_enable);
extern int pif_set_reserved_ref_frame(unsigned int fd, unsigned int enable);
static INT dec_ioctl = 0;

static void get_codec_format_string(HD_VIDEO_CODEC codec, char *str)
{
	switch (codec) {
	case HD_CODEC_TYPE_JPEG:
		sprintf(str, "JPEG");
		break;
	case HD_CODEC_TYPE_H264:
		sprintf(str, "H264");
		break;
	case HD_CODEC_TYPE_H265:
		sprintf(str, "H265");
		break;
	default:
		printf("Not support for this format(%#x)\n", codec);
		sprintf(str, "Unknown");
		break;
	}
}

INT validate_dec_in_dim(HD_DIM dim, HD_VIDEO_CODEC codec_type)
{
	KFLOW_VIDEODEC_HW_SPEC_INFO *p_spec = &vdodec_hw_spec_info;
	char codec_type_str[20];

	if (codec_type == HD_CODEC_TYPE_H264) {
		if (dim.w < p_spec->h264.min_dim.w || dim.w > p_spec->h264.max_dim.w ||
		    dim.h < p_spec->h264.min_dim.h || dim.h > p_spec->h264.max_dim.h) {
			get_codec_format_string(codec_type, codec_type_str);
			HD_VIDEODEC_ERR("PATH_CONFIG: codec(%s) dim(%ux%u) is out-of-range => limit(%ux%u ~ %ux%u)\n",
					codec_type_str, dim.w, dim.h,
					p_spec->h264.min_dim.w, p_spec->h264.min_dim.h,
					p_spec->h264.max_dim.w, p_spec->h264.max_dim.h);
			goto fail_exit;
		}
	} else if (codec_type == HD_CODEC_TYPE_H265) {
		if (dim.w < p_spec->h265.min_dim.w || dim.w > p_spec->h265.max_dim.w ||
		    dim.h < p_spec->h265.min_dim.h || dim.h > p_spec->h265.max_dim.h) {
			get_codec_format_string(codec_type, codec_type_str);
			HD_VIDEODEC_ERR("PATH_CONFIG: codec(%s) dim(%ux%u) is out-of-range => limit(%ux%u ~ %ux%u)\n",
					codec_type_str, dim.w, dim.h,
					p_spec->h265.min_dim.w, p_spec->h265.min_dim.h,
					p_spec->h265.max_dim.w, p_spec->h265.max_dim.h);
			goto fail_exit;
		}
	} else if (codec_type == HD_CODEC_TYPE_JPEG) {
		if (dim.w < p_spec->jpeg.min_dim.w || dim.w > p_spec->jpeg.max_dim.w ||
		    dim.h < p_spec->jpeg.min_dim.h || dim.h > p_spec->jpeg.max_dim.h) {
			get_codec_format_string(codec_type, codec_type_str);
			HD_VIDEODEC_ERR("PATH_CONFIG: codec(%s) dim(%ux%u) is out-of-range => limit(%ux%u ~ %ux%u)\n",
					codec_type_str, dim.w, dim.h,
					p_spec->jpeg.min_dim.w, p_spec->jpeg.min_dim.h,
					p_spec->jpeg.max_dim.w, p_spec->jpeg.max_dim.h);
			goto fail_exit;
		}
	}
	return TRUE;

fail_exit:
	return FALSE;
}

INT verify_param_PATH_CONFIG(VDODEC_MAXMEM *p_maxmem)
{
	HD_DIM dim;

	dim.w = p_maxmem->max_width;
	dim.h = p_maxmem->max_height;
	if (validate_dec_in_dim(dim, p_maxmem->codec_type) == FALSE) {
		goto invalid;
	}

	return 0;
invalid:
	return -1;
}

BOOL videodec_check_addr(UINTPTR address)
{
	BOOL m_valid = FALSE;
	unsigned char vector[2];
	int ret2;
	UINTPTR start = 0;
	//check
	if (address) {
		start = (address / PAGE_SIZE) * PAGE_SIZE;
		if (start) {
			ret2 = mincore((void *)start, 0x8, &vector[0]);
			if (ret2 == 0) { //0 means ok
				m_valid = TRUE;
			} else {
				int errsv = errno;
				printf("INVALID! 0x%x\n", errsv);
				m_valid = FALSE;
			}
		} else {
			//printf("INvalid!start=0x%x\n", (unsigned int)start);
			m_valid = FALSE;
		}
	}
	return m_valid;
}

HD_RESULT videodec_new_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_VIDEODEC_BS *p_video_bitstream)
{
	HD_RESULT hd_ret = HD_OK;
	INT	ddr_id;
	CHAR pool_name[20] = {};
	VOID *addr_pa;

	ddr_id = p_video_bitstream->ddr_id;
	addr_pa = pif_alloc_video_buffer_for_hdal(p_video_bitstream->size, ddr_id, pool_name, USR_LIB);
	if (addr_pa == NULL) {
		HD_VIDEODEC_ERR("allocate in_buf failed.\n");
		hd_ret = HD_ERR_NOMEM;
		goto exit;
	}
	p_video_bitstream->phy_addr = (UINTPTR)addr_pa;

exit:
	return hd_ret;
}

HD_RESULT videodec_push_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_VIDEODEC_BS *p_video_bitstream,
				 HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	HD_VIDEO_FRAME *p_video_frame = p_user_out_video_frame;
	usr_proc_trigger_decode_t usr_dec;
	CHAR pool_name[20] = "disp_dec_out";
	UINTPTR addr_all_pa[DECOUT_MAXNUM] = {0};
	HD_VIDEO_PXLFMT hd_pxlfmt;
	UINT32 cnt = 0;
	INT idx, ddr_id, buf_size[DECOUT_MAXNUM] = {0};
	HD_DIM dim;
	INT errsv;
	INT dev = VDODEC_DEVID(self_id);
	UINTPTR next_addr;

	idx = VDODEC_INID(in_id);
	if (vdodec_param[dev].port[idx].max_mem.max_width && vdodec_param[dev].port[idx].max_mem.max_height) {
		dim.w = vdodec_param[dev].port[idx].max_mem.max_width;
		dim.h = vdodec_param[dev].port[idx].max_mem.max_height;
	} else {
		HD_VIDEODEC_ERR("Check port error, port(%#x) max wh(%u, %u) not set.\n", in_id,
				vdodec_param[dev].port[idx].max_mem.max_width, vdodec_param[dev].port[idx].max_mem.max_height);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	if (p_video_frame) {
		ddr_id = p_video_frame->ddr_id;

		// scan for cascade out_buf
		while (p_video_frame) {
			// check sign with codec_type
			if (vdodec_param[dev].port[idx].max_mem.codec_type == HD_CODEC_TYPE_H265) {
				if (p_video_frame->sign != MAKEFOURCC('2', '6', '5', 'D')) {
					HD_VIDEODEC_ERR("The current codec is (%u), sign(0x%x) should be set to MAKEFOURCC('2','6','5','D')\n",
						vdodec_param[dev].port[idx].max_mem.codec_type, p_video_frame->sign);
					hd_ret = HD_ERR_NG;
					goto exit;
				}
			} else if (vdodec_param[dev].port[idx].max_mem.codec_type == HD_CODEC_TYPE_H264) {
				if (p_video_frame->sign != MAKEFOURCC('2', '6', '4', 'D')) {
					HD_VIDEODEC_ERR("The current codec is (%u), sign(0x%x) should be set to MAKEFOURCC('2','6','4','D')\n",
						vdodec_param[dev].port[idx].max_mem.codec_type, p_video_frame->sign);
					hd_ret = HD_ERR_NG;
					goto exit;
				}
			} else if (vdodec_param[dev].port[idx].max_mem.codec_type == HD_CODEC_TYPE_JPEG) {
				if (p_video_frame->sign != MAKEFOURCC('J', 'P', 'G', 'D')) {
					HD_VIDEODEC_ERR("The current codec is (%u), sign(0x%x) should be set to MAKEFOURCC('J','P','G','D')\n",
						vdodec_param[dev].port[idx].max_mem.codec_type, p_video_frame->sign);
					hd_ret = HD_ERR_NG;
					goto exit;
				}
			} else {
				HD_VIDEODEC_ERR("check sign fail, port(%#x), unknown codec_type: %u\n", in_id,
					vdodec_param[dev].port[idx].max_mem.codec_type);
				hd_ret = HD_ERR_NG;
				goto exit;
			}
			buf_size[cnt] = hd_common_mem_calc_buf_size(p_video_frame);
			if (buf_size[cnt] == 0) {
				HD_VIDEODEC_ERR("Estimate buf_size is 0. idx(%d) wh(%u,%u), line:%d\n",
						idx, (unsigned int)p_video_frame->dim.w, (unsigned int)p_video_frame->dim.h, __LINE__);
				hd_ret = HD_ERR_NG;
				goto exit;
			}

			addr_all_pa[cnt] = p_video_frame->phy_addr[0];

			cnt += 1;
			if (cnt >= DECOUT_MAXNUM) {
				//printf("%s exceeds max %u\n", __FUNCTION__, cnt);
				p_video_frame->p_next = NULL;
				break;
			}

			next_addr = (UINTPTR)p_video_frame->p_next;
			if (p_video_frame->p_next && (videodec_check_addr(next_addr) == TRUE)) {
				p_video_frame = (HD_VIDEO_FRAME *)(p_video_frame->p_next);
			} else {
				break;
			}
		}
	} else {
		// without user out buffer
		buf_size[0] = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(dim, HD_VIDEO_PXLFMT_YUV422_ONE) + (dim.w * dim.h);
		if (buf_size[0] == 0) {
			HD_VIDEODEC_ERR("Estimate buf_size is 0. idx(%d) wh(%u,%u), line:%d\n",
					idx, (unsigned int)dim.w, (unsigned int)dim.h, __LINE__);
			hd_ret = HD_ERR_NG;
			goto exit;
		}

		if (ENTITY_FD_CHIP(utl_get_dec_entity_fd(dev, in_id, NULL)) == 1) {
			ddr_id = DDR_ID2;
		} else {
			ddr_id = DDR_ID0;
		}
		sprintf(pool_name, "disp_dec_out_ddr%d", ddr_id);
		addr_all_pa[0] = (UINTPTR)pif_alloc_video_buffer_for_hdal(buf_size[0], ddr_id, pool_name, USR_LIB);
		if (addr_all_pa[0] == 0) {
			HD_VIDEODEC_ERR("Alloca out_buf failed. buf_size(%d) ddr(%d) pool(%s)\n",
					buf_size[0], ddr_id, pool_name);
			hd_ret = HD_ERR_NOMEM;
			goto exit;
		}
	}

	memset(&usr_dec, 0x0, sizeof(usr_proc_trigger_decode_t));

#if 0
	for (i = 0; i < p_video_bitstream->pack_num; i++) {
		if (p_video_bitstream->vcodec_format == HD_CODEC_TYPE_H265) {
			if (p_video_bitstream->video_pack[i].pack_type.h265_type == H265_NALU_TYPE_SLICE ||
			    p_video_bitstream->video_pack[i].pack_type.h265_type == H265_NALU_TYPE_IDR) {
				usr_dec.in_bs_buffer.ddr_id = p_video_bitstream->ddr_id;
				usr_dec.in_bs_buffer.addr_pa = (UINT)p_video_bitstream->video_pack[i].phy_addr;
				usr_dec.in_bs_buffer.size = (UINT)p_video_bitstream->video_pack[i].length;
			} else if (p_video_bitstream->video_pack[i].pack_type.h265_type == H265_NALU_TYPE_VPS) {
				usr_dec.vps_buffer.ddr_id = p_video_bitstream->ddr_id;
				usr_dec.vps_buffer.addr_pa = (UINT)p_video_bitstream->video_pack[i].phy_addr;
				usr_dec.vps_buffer.size = (UINT)p_video_bitstream->video_pack[i].length;
			} else if (p_video_bitstream->video_pack[i].pack_type.h265_type == H265_NALU_TYPE_SPS) {
				usr_dec.sps_buffer.ddr_id = p_video_bitstream->ddr_id;
				usr_dec.sps_buffer.addr_pa = (UINT)p_video_bitstream->video_pack[i].phy_addr;
				usr_dec.sps_buffer.size = (UINT)p_video_bitstream->video_pack[i].length;
			} else if (p_video_bitstream->video_pack[i].pack_type.h265_type == H265_NALU_TYPE_PPS) {
				usr_dec.pps_buffer.ddr_id = p_video_bitstream->ddr_id;
				usr_dec.pps_buffer.addr_pa = (UINT)p_video_bitstream->video_pack[i].phy_addr;
				usr_dec.pps_buffer.size = (UINT)p_video_bitstream->video_pack[i].length;
			}
		} else {
			if (p_video_bitstream->video_pack[i].pack_type.h264_type == H264_NALU_TYPE_SLICE ||
			    p_video_bitstream->video_pack[i].pack_type.h264_type == H264_NALU_TYPE_IDR) {
				usr_dec.in_bs_buffer.ddr_id = p_video_bitstream->ddr_id;
				usr_dec.in_bs_buffer.addr_pa = (UINT)p_video_bitstream->video_pack[i].phy_addr;
				usr_dec.in_bs_buffer.size = (UINT)p_video_bitstream->video_pack[i].length;
			} else if (p_video_bitstream->video_pack[i].pack_type.h264_type == H264_NALU_TYPE_SPS) {
				usr_dec.sps_buffer.ddr_id = p_video_bitstream->ddr_id;
				usr_dec.sps_buffer.addr_pa = (UINT)p_video_bitstream->video_pack[i].phy_addr;
				usr_dec.sps_buffer.size = (UINT)p_video_bitstream->video_pack[i].length;
			} else if (p_video_bitstream->video_pack[i].pack_type.h264_type == H264_NALU_TYPE_PPS) {
				usr_dec.pps_buffer.ddr_id = p_video_bitstream->ddr_id;
				usr_dec.pps_buffer.addr_pa = (UINT)p_video_bitstream->video_pack[i].phy_addr;
				usr_dec.pps_buffer.size = (UINT)p_video_bitstream->video_pack[i].length;
			}
		}
	}
#else
	usr_dec.in_bs_buffer.ddr_id = p_video_bitstream->ddr_id;
	usr_dec.in_bs_buffer.addr_pa = p_video_bitstream->phy_addr;
	if (pif_address_ddr_sanity_check(usr_dec.in_bs_buffer.addr_pa, usr_dec.in_bs_buffer.ddr_id) < 0) {
		HD_VIDEODEC_ERR("%s:Check in_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_dec.in_bs_buffer.addr_pa, usr_dec.in_bs_buffer.ddr_id);
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	usr_dec.in_bs_buffer.size = p_video_bitstream->size;
#endif

	snprintf(usr_dec.in_bs_buffer.pool_name, MAX_POOL_NAME_LEN, "%s", pool_name);
	usr_dec.fd = vdodec_param[dev].port[idx].priv.fd;
	usr_dec.out_frame_buffer.ddr_id = ddr_id;
	usr_dec.out_frame_buffer.addr_pa = addr_all_pa[0];
	if (pif_address_ddr_sanity_check(usr_dec.out_frame_buffer.addr_pa, usr_dec.out_frame_buffer.ddr_id) < 0) {
		HD_VIDEODEC_ERR("%s:Check out_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_dec.out_frame_buffer.addr_pa,  usr_dec.out_frame_buffer.ddr_id);
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	usr_dec.out_frame_buffer.size = buf_size[0];

	// for sub-yuv
	if (cnt > 0) {
		usr_dec.out_frame_buffer.addr_pa2 = addr_all_pa[1];
		usr_dec.out_frame_buffer.size2 = buf_size[1];
	}

	if (dif_get_pxlfmt_by_codec_type(vdodec_param[dev].port[idx].max_mem.codec_type, &hd_pxlfmt, 0) != HD_OK) {
		HD_VIDEODEC_ERR("push_in: get hd_pxlfmt fail\n");
		goto exit;
	}
	usr_dec.decoded_out_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(hd_pxlfmt);;
	usr_dec.decoded_out_frame_info.dim.w = dim.w;
	usr_dec.decoded_out_frame_info.dim.h = dim.h;

	usr_dec.queue_handle = vdodec_param[dev].port[idx].priv.queue_handle;
	usr_dec.decoded_out_frame_info.pathid = p_user_out_video_frame->reserved[0];
	usr_dec.decoded_out_frame_info.timestamp = p_video_bitstream->timestamp;
	usr_dec.p_user_data = (void *) p_user_out_video_frame->blk;

	if (p_user_out_video_frame && p_user_out_video_frame->poc_info) {
		usr_dec.decoded_out_frame_info.poc_info = p_user_out_video_frame->poc_info;
	}

	usr_dec.decoded_out_frame_info.src_fmt = common_convert_hd_codec_type_to_vpd_buffer_type(vdodec_param[dev].port[idx].max_mem.codec_type);

	if (dec_cfg[dev].port[idx].sub_yuv_ratio) {
		usr_dec.decoded_out_frame_info.sub_yuv_ratio = dec_cfg[dev].port[idx].sub_yuv_ratio;
	}

	if (dec_cfg[dev].port[idx].first_out || dec_cfg[dev].port[idx].extra_out) {
		usr_dec.decoded_out_frame_info.h26x_out_mode = FORM_FMT_PROP_VALUE(dec_cfg[dev].port[idx].first_out, dec_cfg[dev].port[idx].extra_out, 0, 0);
	}

	HD_VIDEODEC_IND("%s: out_frame_buffer: ddr_id(%u), addr_pa(0x%lx), size(%u), cnt(%u), addr_pa2(0x%lx), size2(%u)\n",
			__func__, usr_dec.out_frame_buffer.ddr_id, (ULONG)usr_dec.out_frame_buffer.addr_pa, usr_dec.out_frame_buffer.size,
			cnt, (ULONG)usr_dec.out_frame_buffer.addr_pa2, usr_dec.out_frame_buffer.size2);
	HD_VIDEODEC_IND("%s: out_frame_info: hd_pxlfmt(0x%x), type(0x%x), w(%u), h(%u) h26x_out_mode(%u)\n",
			__func__, hd_pxlfmt, usr_dec.decoded_out_frame_info.type, usr_dec.decoded_out_frame_info.dim.w,
			usr_dec.decoded_out_frame_info.dim.h, usr_dec.decoded_out_frame_info.h26x_out_mode);
	HD_VIDEODEC_IND("                    pathid(0x%x), timestamp(0x%llx), src_fmt(0x%x), sub_yuv_ratio(%u)\n",
			usr_dec.decoded_out_frame_info.pathid, usr_dec.decoded_out_frame_info.timestamp,
			usr_dec.decoded_out_frame_info.src_fmt, usr_dec.decoded_out_frame_info.sub_yuv_ratio);

	if (ioctl(vpd_fd, USR_PROC_TRIGGER_DECODE, &usr_dec) < 0) {
		errsv = errno * -1;
		if (errsv == -2) {
			HD_VIDEODEC_ERR(" out_frame_buffer(phy_addr[0]=%p) is still in use."
					"Please 'pull' this buffer before push it.\n",
					(VOID *)p_video_frame->phy_addr[0]);
		}

		HD_VIDEODEC_ERR("<ioctl \"USR_PROC_TRIGGER_DECODE\" fail, error(%d)>\n", errsv);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}

exit:
	return hd_ret;
}

HD_RESULT videodec_release_in_buf_p(HD_DAL self_id, HD_IO out_id, HD_VIDEODEC_BS *p_video_bitstream)
{
	HD_RESULT hd_ret = HD_OK;
	INT ddr_id, ret;
	VOID *addr_pa;

	addr_pa = (VOID *)p_video_bitstream->phy_addr;
	ddr_id = p_video_bitstream->ddr_id;

	ret = pif_free_video_buffer_for_hdal(addr_pa, ddr_id, USR_LIB);
	if (ret < 0) {
		HD_VIDEODEC_ERR("free buffer phy_addr(%p) failed.\n", addr_pa);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}

HD_RESULT videodec_pull_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_dec_cb_info_t cb_info = {0};
	INT idx, errsv;
	INT dev = VDODEC_DEVID(self_id);

	idx = VDODEC_OUTID(out_id);

	hd_ret = dif_pull_out_buffer(self_id, out_id, p_video_frame, wait_ms); // pull_out in flow mode
	if (hd_ret == HD_OK) {
		if (p_video_frame->phy_addr[0] == 0) {
			HD_VIDEODEC_ERR("dif_pull_out_buffer fail\n");
			hd_ret = HD_ERR_NG;
			goto exit;
		}
	} else if (hd_ret == HD_ERR_NOT_AVAIL) {
		if (vdodec_param[dev].port[idx].priv.queue_handle == 0) {
			HD_VIDEODEC_ERR("output queue is not ready. self_id(%#x) out_id(%#x)\n", self_id, out_id);
			hd_ret = HD_ERR_NG;
			goto exit;
		}

		cb_info.queue_handle = vdodec_param[dev].port[idx].priv.queue_handle;
		cb_info.wait_ms = wait_ms;
		if (ioctl(vpd_fd, USR_PROC_RECV_DATA_DEC, &cb_info) < 0) { // pull_out in trigger mode
			errsv = errno * -1;
			HD_VIDEODEC_ERR("<ioctl \"USR_PROC_RECV_DATA_DEC\" fail, error(%d)>\n", errsv);
			if (errsv == -15) {
				hd_ret = HD_ERR_TIMEDOUT;
			} else {
				hd_ret = HD_ERR_SYS;
			}
			goto exit;
		}

		if (p_video_frame) {
			if (cb_info.status == 1) {
				p_video_frame->ddr_id = cb_info.ddr_id;
				p_video_frame->phy_addr[0] = cb_info.addr_pa;
				if (pif_address_ddr_sanity_check(p_video_frame->phy_addr[0], p_video_frame->ddr_id) < 0) {
					HD_VIDEODEC_ERR("%s:Check pull_out pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)p_video_frame->phy_addr[0],  p_video_frame->ddr_id);
					hd_ret = HD_ERR_PARAM;
					goto exit;
				}
				// foreground dimension
				p_video_frame->dim.w = (UINT32)cb_info.dim.w;
				p_video_frame->dim.h = (UINT32)cb_info.dim.h;
				// background dimension
				p_video_frame->reserved[0] = (UINT32)cb_info.bg_dim.w;
				p_video_frame->reserved[1] = (UINT32)cb_info.bg_dim.h;
				p_video_frame->pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(cb_info.buf_type);
				p_video_frame->poc_info = cb_info.poc_info;
				p_video_frame->timestamp = cb_info.timestamp;
				p_video_frame->blk = (typeof(p_video_frame->blk)) cb_info.p_user_data;
				if (vdodec_param[dev].port[idx].max_mem.codec_type == HD_CODEC_TYPE_H265) {
					p_video_frame->sign = MAKEFOURCC('2', '6', '5', 'D');
				} else if (vdodec_param[dev].port[idx].max_mem.codec_type == HD_CODEC_TYPE_H264) {
					p_video_frame->sign = MAKEFOURCC('2', '6', '4', 'D');
				} else if (vdodec_param[dev].port[idx].max_mem.codec_type == HD_CODEC_TYPE_JPEG) {
					p_video_frame->sign = MAKEFOURCC('J', 'P', 'G', 'D');
				} else {
					HD_VIDEODEC_ERR("invalid codec_type: 0x%x. self_id(%#x) out_id(%#x)\n",
						vdodec_param[dev].port[idx].max_mem.codec_type, self_id, out_id);
				}
			}
		}

		// for sub-yuv
		if (p_video_frame && p_video_frame->p_next && cb_info.addr_pa2) {
			HD_VIDEO_FRAME *p_next_frame = (HD_VIDEO_FRAME *)p_video_frame->p_next;
			if (cb_info.status == 1) {
				p_next_frame->ddr_id = cb_info.ddr_id;
				p_next_frame->phy_addr[0] = cb_info.addr_pa2;
				// foreground dimension
				p_next_frame->dim.w = (UINT32)cb_info.dim2.w;
				p_next_frame->dim.h = (UINT32)cb_info.dim2.h;
				// background dimension
				p_next_frame->reserved[0] = (UINT32)cb_info.bg_dim2.w;
				p_next_frame->reserved[1] = (UINT32)cb_info.bg_dim2.h;
				p_next_frame->pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(cb_info.buf_type2);
				p_next_frame->poc_info = cb_info.poc_info;
				p_next_frame->timestamp = cb_info.timestamp;
			}
		}

		if (cb_info.status == 1)  { //1:job finished, 0:ongoing, -1:fail, -2:dec_err
			hd_ret = HD_OK;
		} else if (cb_info.status == -2) {
			hd_ret = HD_ERR_FAIL;
		} else {
			hd_ret = HD_ERR_NG;
		}
	} else {
		HD_VIDEODEC_ERR("pull out buffer error, ret(%#x)\n", hd_ret);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

exit:
	return hd_ret;
}

HD_RESULT videodec_release_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT ddr_id, ret;
	VOID *addr_pa;

	hd_ret = dif_release_out_buffer(self_id, out_id, p_video_frame);
	if (hd_ret == HD_OK) {
		p_video_frame->phy_addr[0] = 0;
	} else if (hd_ret == HD_ERR_NOT_AVAIL) {
		addr_pa = (VOID *) p_video_frame->phy_addr[0];
		ddr_id = p_video_frame->ddr_id;
		ret = pif_free_video_buffer_for_hdal(addr_pa, ddr_id, USR_USR);
		if (ret < 0) {
			HD_VIDEODEC_ERR("free buffer phy_addr(%p) failed.\n", addr_pa);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		hd_ret = HD_OK;
	} else {
		HD_VIDEODEC_ERR("release out buffer error, ret(%#x)\n", hd_ret);
		hd_ret = HD_ERR_NG;
	}

exit:
	return hd_ret;
}

HD_RESULT videodec_get_status_p(HD_DAL self_id, HD_IO io_id, VDODEC_STATUS *p_status)
{
	INT ret;
	HD_RESULT hd_ret = HD_OK;
	UINT32 idx = VDODEC_INID(io_id);
	INT dev = VDODEC_DEVID(self_id);
	usr_proc_queue_status_t status;
	INT32 next_fd;

	if (is_bind(self_id, idx, 0) == HD_OK) {
		int next_dev_idx;
		int next_path_idx;

		if (dec_cfg[dev].port[idx].status_next_path_id) {
			next_dev_idx = HD_GET_DEV(dec_cfg[dev].port[idx].status_next_path_id) - HD_DAL_VIDEOPROC_BASE;
			next_path_idx = HD_GET_OUT(dec_cfg[dev].port[idx].status_next_path_id) - HD_OUT_BASE;
			next_fd = utl_get_vpe_entity_fd(next_dev_idx, next_path_idx, NULL);
		} else {
			next_fd = 0;
		}
		pif_get_left_frame_count_with_next_fd(vdodec_param[dev].port[idx].priv.fd,
								 next_fd,
								 (UINT *)&(p_status->left_frames),
								 (UINT *)&(p_status->done_frames),
								 (UINT *)&(p_status->dti_buf_cnt));
		pif_get_reserved_ref_frame(vdodec_param[dev].port[idx].priv.fd, (UINT *)&(p_status->reserved_ref_frame));
	} else {
		status.usr_proc_fd.fd = vdodec_param[dev].port[idx].priv.fd;
		status.queue_handle = vdodec_param[dev].port[idx].priv.queue_handle;
		status.left_frames = 0;
		status.done_frames = 0;
		status.dti_buf_cnt = 0;
		if (ioctl(vpd_fd, USR_PROC_QUERY_STATUS, &status) < 0) {
			ret = errno * -1;
			HD_VIDEODEC_ERR("ioctl \"USR_PROC_QUERY_STATUS\" fail(%d)\n", ret);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		p_status->left_frames = status.left_frames;
		p_status->done_frames = status.done_frames;
		p_status->dti_buf_cnt = status.dti_buf_cnt;
	}

exit:
	return hd_ret;
}

HD_RESULT videodec_set_status_p(HD_DAL self_id, HD_IO io_id, VDODEC_STATUS *p_status)
{
	UINT32 idx = VDODEC_INID(io_id);
	INT dev = VDODEC_DEVID(self_id);

	pif_set_reserved_ref_frame(vdodec_param[dev].port[idx].priv.fd, p_status->reserved_ref_frame);
	return HD_OK;
}

VDODEC_INTL_PARAM *videodec_get_param_p(HD_DAL self_id, HD_IO sub_id)
{
	INT dev = VDODEC_DEVID(self_id);
	return &(dec_cfg[dev].port[sub_id]);
}

HD_RESULT videodec_set_param_p(HD_DAL self_id, HD_IO io_id)
{
	UINT32 idx = VDODEC_INID(io_id);
	INT dev = VDODEC_DEVID(self_id);

	dec_cfg[dev].port[idx].max_mem = &vdodec_param[dev].port[idx].max_mem;
	dec_cfg[dev].port[idx].p_out_pool = &vdodec_param[dev].port[idx].out_pool;
	return HD_OK;
}

HD_RESULT videodec_check_path_id_p(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);

	if (VDODEC_DEVID(self_id) > videodec_max_device_nr) {
		HD_VIDEODEC_ERR("Error self_id(%#x)\n", self_id);
		return HD_ERR_NG;
	}
	if (HD_GET_CTRL(path_id) != HD_CTRL) {
		if (VDODEC_INID(in_id) > videodec_max_device_in) {
			HD_VIDEODEC_ERR("Check input port error, port(%#x) max(%#x)\n", in_id, videodec_max_device_in);
			return HD_ERR_NG;
		}
		if (VDODEC_OUTID(out_id) > videodec_max_device_out) {
			HD_VIDEODEC_ERR("Check output port error, port(%#x) max(%#x)\n", out_id, videodec_max_device_out);
			return HD_ERR_NG;
		}
	}
	return HD_OK;
}

HD_RESULT videodec_init_path_p(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL deviceid = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT dev_idx = bd_get_dev_subid(deviceid);
	INT port_idx = VDODEC_OUTID(out_id);
	INT dev = VDODEC_DEVID(deviceid);
	INT fd = 0, i;
	//UINT queue_handle;
	uintptr_t queue_handle;

	fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_DECODER, dev_idx, 0, port_idx);
	if (fd == 0) {
		HD_VIDEODEC_ERR("get videodec fd(%#x) failed.\n", fd);
	}
	vdodec_param[dev].port[port_idx].priv.fd = fd;

	queue_handle = pif_create_queue_for_hdal();
	if (queue_handle == 0) {
		HD_VIDEODEC_ERR("create queue_handle failed.\n");
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	vdodec_param[dev].port[port_idx].priv.queue_handle = queue_handle;

	for (i = 0; i < HD_VIDEODEC_MAX_DATA_TYPE; i++) {
		vdodec_param[dev].port[port_idx].out_pool.buf_info[i].enable = HD_BUF_AUTO;
	}
	dec_cfg[dev].port[port_idx].p_out_pool = &vdodec_param[dev].port[port_idx].out_pool;

	// init sub_yuv_ratio
	dec_cfg[dev].port[port_idx].sub_yuv_ratio = 2;

exit:
	return hd_ret;
}

HD_RESULT videodec_uninit_path_p(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT port_idx = VDODEC_OUTID(out_id);
	INT dev = VDODEC_DEVID(self_id);
	INT ret;

	/* clear pull queue */
	ret = pif_destroy_queue_for_hdal(vdodec_param[dev].port[port_idx].priv.queue_handle);
	if (ret < 0) {
		HD_VIDEODEC_ERR("destroy queue failed. ret(%d)\n", ret);
		hd_ret = HD_ERR_NG;
	}

	return hd_ret;
}

HD_RESULT videodec_init_p(void)
{
	HD_RESULT hd_ret = HD_OK;
	INT ioctl_ret;

	if (!dec_ioctl) {
		dec_ioctl = open("/dev/vdodec_ioctl", O_RDWR);
		if (!dec_ioctl) {
			HD_VIDEODEC_ERR("dec device(/dev/vdodec_ioctl) is not found\n");
			return HD_ERR_SYS;
		}
	}
	memset(dec_cfg, 0, sizeof(VDODEC_DEV_PARAM) * VDODEC_DEV);
	memset(vdodec_param, 0, sizeof(VDODEC_PARAM) * VDODEC_DEV);
	memset(&dec_ctrl, 0, sizeof(VDODEC_CTRL_PARAM));

	ioctl_ret = ioctl(dec_ioctl, KFLOW_VIDEODEC_IOC_GET_HW_SPEC_INFO, &vdodec_hw_spec_info);
	if (ioctl_ret < 0) {
		HD_VIDEODEC_ERR("ioctl \"KFLOW_VIDEODEC_IOC_GET_HW_SPEC_INFO\" return %d\n", ioctl_ret);
		hd_ret = HD_ERR_SYS;
	}

	return hd_ret;
}

HD_RESULT videodec_uninit_p(void)
{
	HD_RESULT hd_ret = HD_OK;

	if (dec_ioctl) {
		close(dec_ioctl);
		dec_ioctl = 0;
	}
	return hd_ret;
}

HD_RESULT videodec_close(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_stop_trigger_t stop_trigger_info;
	INT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT port_idx = VDODEC_OUTID(out_id);
	INT dev = VDODEC_DEVID(self_id);

	if (vdodec_param[dev].port[port_idx].priv.fd == 0) {
		HD_VIDEODEC_ERR("The fd is 0.\n");
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	memset(&stop_trigger_info, 0, sizeof(stop_trigger_info));
	stop_trigger_info.usr_proc_fd.fd = vdodec_param[dev].port[port_idx].priv.fd;
	stop_trigger_info.mode = USR_PROC_STOP_FD_BUF_CLEAR; // need to wait for buffer clear
	if ((ret = ioctl(vpd_fd, USR_PROC_STOP_TRIGGER, &stop_trigger_info)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		HD_VIDEODEC_ERR("<ioctl \"USR_PROC_STOP_TRIGGER\" fail, error(%d)>\n", ret);
		hd_ret = HD_ERR_NG;
	}
	vdodec_param[dev].port[port_idx].priv.fd = 0;

exit:
	return hd_ret;
}

