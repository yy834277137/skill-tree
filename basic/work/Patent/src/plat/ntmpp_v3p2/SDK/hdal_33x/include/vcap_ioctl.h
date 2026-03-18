#ifndef _VCAP_IOC_H_
#define _VCAP_IOC_H_

#include <linux/ioctl.h>
#include "kwrap/ioctl.h"


/*************************************************************************************
 *  VCAP IOCTL Version
 *************************************************************************************/
#define VCAP_IOC_MAJOR_VER          0
#define VCAP_IOC_MINOR_VER          3
#define VCAP_IOC_BETA_VER           1
#define VCAP_IOC_VERSION            (((VCAP_IOC_MAJOR_VER)<<16)|((VCAP_IOC_MINOR_VER)<<8)|(VCAP_IOC_BETA_VER))

/*************************************************************************************
 *  VCAP IOCTL Hardware Ability Definition
 *************************************************************************************/
struct vcap_ioc_hw_ability_t {
    unsigned int version;                   ///< ioctl structure version, must set for version check

    unsigned int vi_max;                    ///< vi maximum number of each vcap
    unsigned int vi_ch_max;                 ///< ch maximum number of each vcap vi
    unsigned int scaler_max;                ///< scaler maximum number of each vcap channel
    unsigned int scaler_h_size_max;         ///< scaler horizontial maximum output pixel size
    unsigned int scaler_h_ratio_max;        ///< scaler horizontial maximum size down ratio
    unsigned int scaler_v_ratio_max;        ///< scaler vertical maximum size down ratio
    unsigned int scaler_h_ratio_qty_max;    ///< scaler horizontial maximum size down quality ratio
    unsigned int scaler_v_ratio_qty_max;    ///< scaler vertical maximum size down quality ratio
    unsigned int scaler_ability;            ///< scaler scaling ability, 0:not scaling support 1:scaling support, bit0:scaler#0 bit1:scaler#1....
    unsigned int mask_win_max;              ///< mask window maximum number of each vcap channel
    unsigned int palette_max;               ///< palette maximum number of each vcap
    unsigned int md_mb_x_num_max;           ///< md horizontal maximum mb number of each vcap channel
    unsigned int md_mb_y_num_max;           ///< md vertical maximum mb number of each vcap channel
    unsigned int patgen_ability;            ///< pattern generator ability, 0:No 1:Yes
    unsigned int dma_out_fmt_ability;       ///< dma output format ability, BIT0:YUV422 BIT1:YUV420 BIT2:YUV422_SCE BIT3:YUV420_SCE BIT4:YUV422_MB BIT5:YUV420_MB
    unsigned int reserved[6];
};

typedef enum {
    VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV422     = (0x1<<0),
    VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV420     = (0x1<<1),
    VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV422_SCE = (0x1<<2),
    VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV420_SCE = (0x1<<3),
    VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV422_MB  = (0x1<<4),
    VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV420_MB  = (0x1<<5)
} VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_T;

/*************************************************************************************
 *  VCAP IOCTL Hardware Limitation Definition
 *************************************************************************************/
struct vcap_ioc_hw_limit_t {
    unsigned int version;                   ///< ioctl structure version, must set for version check

    /* VI Channel */
    struct {
        unsigned char  w_align;             ///< channel source image width  alignment value
        unsigned char  h_align;             ///< channel source image height alignment value
        unsigned short w_min;               ///< channel source image width  minimum   value
        unsigned short h_min;               ///< channel source image height minimum   value
        unsigned short w_max;               ///< channel source image width  maximun   value
        unsigned short h_max;               ///< channel source image width  maximun   value
        unsigned int   reserved[4];
    } vi_ch;

    /* Motion Detection */
    struct {
        unsigned char  mb_x_align;          ///< motion block x start position alignment value
        unsigned char  mb_y_align;          ///< motion block y start position alignment value
        unsigned char  mb_x_size_align;     ///< motion block x size  pixel    alignment value
        unsigned char  mb_y_size_align;     ///< motion block y size  line     alignment value
        unsigned short mb_x_size_min;       ///< motion block x size  pixel    minimum   value
        unsigned short mb_y_size_min;       ///< motion block y size  line     minimum   value
        unsigned short mb_x_size_max;       ///< motion block x size  pixel    maximun   value
        unsigned short mb_y_size_max;       ///< motion block y size  line     maximun   value
        unsigned int   reserved[4];
    } md;

    /* Mask Windows */
    struct {
        unsigned char  x_align;             ///< mask window x start position alignment value
        unsigned char  y_align;             ///< mask window y start position alignment value
        unsigned char  w_align;             ///< mask window width  alignment value
        unsigned char  h_align;             ///< mask window height alignment value
        unsigned short w_min;               ///< mask window width  minimum   value
        unsigned short h_min;               ///< mask window height minimum   value
        unsigned short w_max;               ///< mask window width  maximun   value
        unsigned short h_max;               ///< mask window height maximun   value
        unsigned int   reserved[4];
    } mask_win;

    /* Source Cropping */
    struct {
        unsigned char  x_align;             ///< source cropping x start position alignment value
        unsigned char  y_align;             ///< source cropping y start position alignment value
        unsigned char  w_align;             ///< source cropping width  alignment value
        unsigned char  h_align;             ///< source cropping height alignment value
        unsigned short w_min;               ///< source cropping width  minimum   value
        unsigned short h_min;               ///< source cropping height minimum   value
        unsigned short w_max;               ///< source cropping width  maximun   value
        unsigned short h_max;               ///< source cropping height maximun   value
        unsigned int   reserved[4];
    } sc;

    /* Scaler */
    struct {
        unsigned char  w_align;             ///< scaler output width  alignment value
        unsigned char  h_align;             ///< scaler output height alignment value
        unsigned short w_min;               ///< scaler output width  minimum   value
        unsigned short h_min;               ///< scaler output height minimum   value
        unsigned short w_max;               ///< scaler output width  maximun   value
        unsigned short h_max;               ///< scaler output height maximun   value
        unsigned int   reserved[4];
    } scaler;

    /* Target Cropping */
    struct {
        unsigned char  x_align;             ///< target cropping x start position alignment value
        unsigned char  y_align;             ///< target cropping y start position alignment value
        unsigned char  w_align;             ///< target cropping width  alignment value
        unsigned char  h_align;             ///< target cropping height alignment value
        unsigned short w_min;               ///< target cropping width  minimum   value
        unsigned short h_min;               ///< target cropping height minimum   value
        unsigned short w_max;               ///< target cropping width  maximun   value
        unsigned short h_max;               ///< target cropping height maximun   value
        unsigned int   reserved[4];
    } tc;

    /* DMA */
    struct {
        unsigned char  md_addr_align;       ///< motion detection statistic buffer dma start address alignment value
        unsigned char  yuv_addr_align;      ///< yuv image buffer dma start address alignment value

        unsigned char  ext_hori_align;      ///< extend horizontial left/right pixel alignment value
        unsigned char  ext_vert_align;      ///< extend vertical    top/bottom line  alignment value
        unsigned short ext_hori_min;        ///< extend horizontial left/right pixel minimum   value
        unsigned short ext_vert_min;        ///< extend vertical    top/bottom line  minimum   value
        unsigned short ext_hori_max;        ///< extend horizontial left/right pixel maximun   value
        unsigned short ext_vert_max;        ///< extend vertical    top/bottom line  maximun   value

        unsigned char  yuv_w_align[2];      ///< yuv image raw output width  alignment value,                              [0]:YUV420 [1]:YUV422
        unsigned char  yuv_h_align[2];      ///< yuv image raw output height alignment value,                              [0]:YUV420 [1]:YUV422
        unsigned short yuv_w_min[2];        ///< yuv image raw output width (ext_hori + tc_width)  pixel minimum value,    [0]:YUV420 [1]:YUV422
        unsigned short yuv_h_min[2];        ///< yuv image raw output height(ext_vert + tc_height) line  minimum value,    [0]:YUV420 [1]:YUV422
        unsigned short yuv_w_max[2];        ///< yuv image raw output width (ext_hori + tc_width)  pixel maximun value,    [0]:YUV420 [1]:YUV422
        unsigned short yuv_h_max[2];        ///< yuv image raw output height(ext_vert + tc_height) line  maximun value,    [0]:YUV420 [1]:YUV422

        unsigned char  yuv_sce_w_align[2];  ///< yuv image sce output width  alignment value,                              [0]:YUV420 [1]:YUV422
        unsigned char  yuv_sce_h_align[2];  ///< yuv image sce output height alignment value,                              [0]:YUV420 [1]:YUV422
        unsigned short yuv_sce_w_min[2];    ///< yuv image sce output width (ext_hori + tc_width)  pixel minimum value,    [0]:YUV420 [1]:YUV422
        unsigned short yuv_sce_h_min[2];    ///< yuv image sce output height(ext_vert + tc_height) line  minimum value,    [0]:YUV420 [1]:YUV422
        unsigned short yuv_sce_w_max[2];    ///< yuv image sce output width (ext_hori + tc_width)  pixel maximun value,    [0]:YUV420 [1]:YUV422
        unsigned short yuv_sce_h_max[2];    ///< yuv image sce output height(ext_vert + tc_height) line  maximun value,    [0]:YUV420 [1]:YUV422

        unsigned char  yuv_mb_w_align[2];   ///< yuv image mbscan output width  alignment value,                           [0]:YUV420 [1]:YUV422
        unsigned char  yuv_mb_h_align[2];   ///< yuv image mbscan output height alignment value,                           [0]:YUV420 [1]:YUV422
        unsigned short yuv_mb_w_min[2];     ///< yuv image mbscan output width (ext_hori + tc_width)  pixel minimum value, [0]:YUV420 [1]:YUV422
        unsigned short yuv_mb_h_min[2];     ///< yuv image mbscan output height(ext_vert + tc_height) line  minimum value, [0]:YUV420 [1]:YUV422
        unsigned short yuv_mb_w_max[2];     ///< yuv image mbscan output width (ext_hori + tc_width)  pixel maximun value, [0]:YUV420 [1]:YUV422
        unsigned short yuv_mb_h_max[2];     ///< yuv image mbscan output height(ext_vert + tc_height) line  maximun value, [0]:YUV420 [1]:YUV422

        unsigned int   reserved[4];
    } dma;

    unsigned int reserved[8];
};

/*************************************************************************************
 *  VCAP IOCTL Host Definition
 *************************************************************************************/
#define VCAP_IOC_VI_MAX                     32
#define VCAP_IOC_VI_VPORT_MAX               2
#define VCAP_IOC_VI_CH_MAX                  4

typedef enum {
    VCAP_IOC_VI_MODE_DISABLE = 0,           ///< Disable
    VCAP_IOC_VI_MODE_1CH,                   ///< CH#0       from VI-P#0
    VCAP_IOC_VI_MODE_2CH,                   ///< CH#0,2     from VI-P#0
    VCAP_IOC_VI_MODE_4CH,                   ///< CH#0,1,2,3 from VI-P#0
    VCAP_IOC_VI_MODE_2CH_2P,                ///< CH#0       from VI-P#0, CH#2   from VI-P#1, pinmux not support in NT98313/NT98316/NT98321/NT98323
    VCAP_IOC_VI_MODE_4CH_2P,                ///< CH#0,1     from VI-P#0, CH#2,3 from VI-P#1
    VCAP_IOC_VI_MODE_3CH_2P_MIX0,           ///< CH#0       from VI-P#0, CH#2,3 from VI-P#1, pinmux not support in NT98313/NT98316/NT98321/NT98323
    VCAP_IOC_VI_MODE_3CH_2P_MIX1,           ///< CH#0,1     from VI-P#0, CH#2   from VI-P#1, pinmux not support in NT98313/NT98316/NT98321/NT98323
    VCAP_IOC_VI_MODE_1CH_2P,                ///< CH#0 Y     from VI-P#0, CbCr   from VI-P#1
    VCAP_IOC_VI_MODE_MAX
} VCAP_IOC_VI_MODE_T;

struct vcap_ioc_host_t {
    unsigned int  version;                  ///< ioctl structure version, must set for version check

    unsigned char host;                     ///< host index

    /* System VI Parameter */
    unsigned int nr_of_vi;                  ///< number of video interface
    struct {
        unsigned int chip:8;                ///< chip index
        unsigned int vcap:8;                ///< vcap index
        unsigned int vi  :8;                ///< vi   index
        unsigned int mode:8;                ///< vi   mode, value as VCAP_IOC_VI_MODE_T
        unsigned int reserved;
    } vi[VCAP_IOC_VI_MAX];

    /* System Motion Detection Parameter */
    struct {
        unsigned int enable      :1;        ///< enable/disable capture motion/tamper detection support
        unsigned int mb_x_num_max:12;       ///< specify horizontial motion block max number, 0 ~ 256, apply on buffer allocate from driver
        unsigned int mb_y_num_max:12;       ///< specify vertical motion block max number,    0 ~ 256, apply on buffer allocate from driver
        unsigned int buf_src     :1;        ///< motion detection buffer allocate source,     0:driver 1:library
        unsigned int reserved    :6;
    } md;

    /* Reserved */
    unsigned int reserved[8];
};

struct vcap_ioc_host_id_t {
    unsigned int  version;                  ///< ioctl structure version, must set for version check
    unsigned char host;                     ///< host index
};

typedef enum {
    VCAP_IOC_HOST_SCALER_RULE_1000X = 0,    ///< ~       < scaling ratio <= 1.00x
    VCAP_IOC_HOST_SCALER_RULE_0800X,        ///< 1.00x   < scaling ratio <= 1/1.25x
    VCAP_IOC_HOST_SCALER_RULE_0666X,        ///< 1/1.25x < scaling ratio <= 1/1.50x
    VCAP_IOC_HOST_SCALER_RULE_0571X,        ///< 1/1.50x < scaling ratio <= 1/1.75x
    VCAP_IOC_HOST_SCALER_RULE_0500X,        ///< 1/1.75x < scaling ratio <= 1/2.00x
    VCAP_IOC_HOST_SCALER_RULE_0444X,        ///< 1/2.00x < scaling ratio <= 1/2.25x
    VCAP_IOC_HOST_SCALER_RULE_0400X,        ///< 1/2.25x < scaling ratio <= 1/2.50x
    VCAP_IOC_HOST_SCALER_RULE_0363X,        ///< 1/2.50x < scaling ratio <= 1/2.75x
    VCAP_IOC_HOST_SCALER_RULE_0330X,        ///< 1/2.75x < scaling ratio <= 1/3.00x
    VCAP_IOC_HOST_SCALER_RULE_0307X,        ///< 1/3.00x < scaling ratio <= 1/3.25x
    VCAP_IOC_HOST_SCALER_RULE_0285X,        ///< 1/3.25x < scaling ratio <= 1/3.50x
    VCAP_IOC_HOST_SCALER_RULE_0266X,        ///< 1/3.50x < scaling ratio <= 1/3.75x
    VCAP_IOC_HOST_SCALER_RULE_0250X,        ///< 1/3.75x < scaling ratio <= 1/4.00x
    VCAP_IOC_HOST_SCALER_RULE_0200X,        ///< 1/4.00x < scaling ratio <= 1/5.00x
    VCAP_IOC_HOST_SCALER_RULE_0166X,        ///< 1/5.00x < scaling ratio <= 1/6.00x
    VCAP_IOC_HOST_SCALER_RULE_0142X,        ///< 1/6.00x < scaling ratio <= 1/7.00x
    VCAP_IOC_HOST_SCALER_RULE_0125X,        ///< 1/7.00x < scaling ratio <= 1/8.00x
    VCAP_IOC_HOST_SCALER_RULE_MAX
} VCAP_IOC_HOST_SCALER_RULE_T;

struct vcap_ioc_host_scaler_rule_t {
    unsigned int  version;                  ///< ioctl structure version, must set for version check

    unsigned char host;                     ///< host index

    struct {
        unsigned char   map_select;         ///< scaler source mapping format select, 0:without 0.5 pixel distance shift 1:with 0.5 pixel distance shift
        unsigned char   h_luma_algo;        ///< scaler horizontal luma   scaling algorithm, 0:HW_AUTO 1:BYPASS 2:LPF 3:LPFBI
        unsigned char   v_luma_algo;        ///< scaler vertical   luma   scaling algorithm, 0:HW_AUTO 1:BYPASS 2:LPF 3:LPFBI
        unsigned char   h_chroma_algo;      ///< scaler horizontal chroma scaling algorithm, 0:HW_AUTO 1:BYPASS 2:BI  3:AVG
        unsigned char   v_chroma_algo;      ///< scaler vertical   chroma scaling algorithm, 0:HW_AUTO 1:BYPASS 2:BI  3:AVG
        signed   short  h_coef[4];          ///< scaler LPF coeficient in horizontal direction (10bit signed, 2's complement)
        signed   short  v_coef[4];          ///< scaler LPF coeficient in vertical   direction (10bit signed, 2's complement)
    } rule[VCAP_IOC_HOST_SCALER_RULE_MAX];
};

typedef enum {
    VCAP_IOC_HOST_FRAME_FILTER_VI_FIFO_FULL = 0,
    VCAP_IOC_HOST_FRAME_FILTER_PIXEL_LACK,
    VCAP_IOC_HOST_FRAME_FILTER_PIXEL_FATAL_LACK,
    VCAP_IOC_HOST_FRAME_FILTER_LINE_LACK,
    VCAP_IOC_HOST_FRAME_FILTER_CM_OVF,
    VCAP_IOC_HOST_FRAME_FILTER_SD_FAIL,
    VCAP_IOC_HOST_FRAME_FILTER_SD_JOB_OVF,
    VCAP_IOC_HOST_FRAME_FILTER_DMA_OVF,
    VCAP_IOC_HOST_FRAME_FILTER_MAX
} VCAP_IOC_HOST_FRAME_FILTER_T;

struct vcap_ioc_host_frame_filter_t {
    unsigned int                 version;   ///< ioctl structure version, must set for version check

    unsigned char                host;      ///< host   index
    VCAP_IOC_HOST_FRAME_FILTER_T filter;    ///< filter index
    unsigned int                 mask;      ///< 0:umask(tag frame fail on error) 1:mask(tag frame ok on error)
};

/*************************************************************************************
 *  VCAP IOCTL VideoGraph FD Definition
 *************************************************************************************/
struct vcap_ioc_vg_fd_t {
    unsigned int  version;                  ///< ioctl structure version, must set for version check

    unsigned int  fd;                       ///< videograph fd

    unsigned char chip;                     ///< chip index
    unsigned char vcap;                     ///< vcap index
    unsigned char vi;                       ///< vi   index
    unsigned char ch;                       ///< ch   index
    unsigned char path;                     ///< path index 0 ~ 3, the path index support 0 ~ 7 if enable vi duplication
};

/*************************************************************************************
 *  VCAP IOCTL Video Interface Definition
 *************************************************************************************/

/*************************************************************************************
    <<NA51039>>
    ======================================================================================================================
    [NT98313]
    [NT98316-NT98313#0]                                           [NT98316-NT98313#1]
    ======================================================================================================================
                       [VCAP#0-VI#0]                         ||                       [VCAP#0-VI#0]
                       *----------------------------------*  ||                       *----------------------------------*
    X_CAP#0-----o--o-->|-->VI#0-P0--*-->Demux#0--*-->CH#0 |  ||    X_CAP#4-----o--o-->|-->VI#0-P0--*-->Demux#0--*-->CH#0 |
                |  |   |            |            |-->CH#1 |  ||                |  |   |            |            |-->CH#1 |
             o--(->o-->|-->VI#0-P1--*-->Demux#1--*-->CH#2 |  ||             o--(->o-->|-->VI#0-P1--*-->Demux#1--*-->CH#2 |
             |  |      |                         |-->CH#3 |  ||             |  |      |                         |-->CH#3 |
             |  |      *----------------------------------*  ||             |  |      *----------------------------------*
             |  |                                            ||             |  |
             |  |      [VCAP#0-VI#1]                         ||             |  |      [VCAP#0-VI#1]
             |  |      *----------------------------------*  ||             |  |      *----------------------------------*
    X_CAP#1--o--o----->|-->VI#1-P0--*-->Demux#0--*-->CH#0 |  ||    X_CAP#5--o--o----->|-->VI#1-P0--*-->Demux#0--*-->CH#0 |
             |         |            |            |-->CH#1 |  ||             |         |            |            |-->CH#1 |
             o-------->|-->VI#1-P1--*-->Demux#1--*-->CH#2 |  ||             o-------->|-->VI#1-P1--*-->Demux#1--*-->CH#2 |
                       |                         |-->CH#3 |  ||                       |                         |-->CH#3 |
                       *----------------------------------*  ||                       *----------------------------------*
                                                             ||
                       [VCAP#0-VI#2]                         ||                       [VCAP#0-VI#2]
                       *----------------------------------*  ||                       *----------------------------------*
    X_CAP#2-----o--o-->|-->VI#2-P0--*-->Demux#0--*-->CH#0 |  ||    X_CAP#6-----o--o-->|-->VI#2-P0--*-->Demux#0--*-->CH#0 |
                |  |   |            |            |-->CH#1 |  ||                |  |   |            |            |-->CH#1 |
             o--(->o-->|-->VI#2-P1--*-->Demux#1--*-->CH#2 |  ||             o--(->o-->|-->VI#2-P1--*-->Demux#1--*-->CH#2 |
             |  |      |                         |-->CH#3 |  ||             |  |      |                         |-->CH#3 |
             |  |      *----------------------------------*  ||             |  |      *----------------------------------*
             |  |                                            ||             |  |
             |  |      [VCAP#0-VI#3]                         ||             |  |      [VCAP#0-VI#3]
             |  |      *----------------------------------*  ||             |  |      *----------------------------------*
    X_CAP#3--o--o----->|-->VI#3-P0--*-->Demux#0--*-->CH#0 |  ||    X_CAP#7--o--o----->|-->VI#3-P0--*-->Demux#0--*-->CH#0 |
             |         |            |            |-->CH#1 |  ||             |         |            |            |-->CH#1 |
             o-------->|-->VI#3-P1--*-->Demux#1--*-->CH#2 |  ||             o-------->|-->VI#3-P1--*-->Demux#1--*-->CH#2 |
                       |                         |-->CH#3 |  ||                       |                         |-->CH#3 |
                       *----------------------------------*  ||                       *----------------------------------*
                                                             ||
                      [VCAP#0-VI#4]                          ||                      [VCAP#0-VI#4]
                      *----------------------------------*   ||                      *----------------------------------*
    X_CAP#0---------->|-->VI#4-P0--*-->Demux#0--*-->CH#0 |   ||    X_CAP#4---------->|-->VI#4-P0--*-->Demux#0--*-->CH#0 |
             |        |            |            |-->CH#1 |   ||             |        |            |            |-->CH#1 |
             o------->|-->VI#4-P1--*-->Demux#1--*-->CH#2 |   ||             o------->|-->VI#4-P1--*-->Demux#1--*-->CH#2 |
                      |                         |-->CH#3 |   ||                      |                         |-->CH#3 |
                      *----------------------------------*   ||                      *----------------------------------*
                                                             ||
                      [VCAP#0-VI#5]                          ||                      [VCAP#0-VI#5]
                      *----------------------------------*   ||                      *----------------------------------*
    X_CAP#1---------->|-->VI#5-P0--*-->Demux#0--*-->CH#0 |   ||    X_CAP#5---------->|-->VI#5-P0--*-->Demux#0--*-->CH#0 |
             |        |            |            |-->CH#1 |   ||             |        |            |            |-->CH#1 |
             o------->|-->VI#5-P1--*-->Demux#1--*-->CH#2 |   ||             o------->|-->VI#5-P1--*-->Demux#1--*-->CH#2 |
                      |                         |-->CH#3 |   ||                      |                         |-->CH#3 |
                      *----------------------------------*   ||                      *----------------------------------*
                                                             ||
                      [VCAP#0-VI#6]                          ||                      [VCAP#0-VI#6]
                      *----------------------------------*   ||                      *----------------------------------*
    X_CAP#2---------->|-->VI#6-P0--*-->Demux#0--*-->CH#0 |   ||    X_CAP#6---------->|-->VI#6-P0--*-->Demux#0--*-->CH#0 |
             |        |            |            |-->CH#1 |   ||             |        |            |            |-->CH#1 |
             o------->|-->VI#6-P1--*-->Demux#1--*-->CH#2 |   ||             o------->|-->VI#6-P1--*-->Demux#1--*-->CH#2 |
                      |                         |-->CH#3 |   ||                      |                         |-->CH#3 |
                      *----------------------------------*   ||                      *----------------------------------*
                                                             ||
                      [VCAP#0-VI#7]                          ||                      [VCAP#0-VI#7]
                      *----------------------------------*   ||                      *----------------------------------*
    X_CAP#3---------->|-->VI#7-P0--*-->Demux#0--*-->CH#0 |   ||    X_CAP#7---------->|-->VI#7-P0--*-->Demux#0--*-->CH#0 |
             |        |            |            |-->CH#1 |   ||             |        |            |            |-->CH#1 |
             o------->|-->VI#7-P1--*-->Demux#1--*-->CH#2 |   ||             o------->|-->VI#7-P1--*-->Demux#1--*-->CH#2 |
                      |                         |-->CH#3 |   ||                      |                         |-->CH#3 |
                      *----------------------------------*   ||                      *----------------------------------*
    ======================================================================================================================
    <<NA51068>>
    ======================================================================================================================
    [NT98323]                                                  [NT98321]
    ======================================================================================================================
                       [VCAP#0-VI#0]                         ||                       [VCAP#0-VI#0]
                       *----------------------------------*  ||                       *----------------------------------*
    X_CAP#0-----o--o-->|-->VI#0-P0--*-->Demux#0--*-->CH#0 |  ||    X_CAP#0-----o--o-->|-->VI#0-P0--*-->Demux#0--*-->CH#0 |
                |  |   |            |            |-->CH#1 |  ||                |  |   |            |            |-->CH#1 |
             o--(->o-->|-->VI#0-P1--*-->Demux#1--*-->CH#2 |  ||             o--(->o-->|-->VI#0-P1--*-->Demux#1--*-->CH#2 |
             |  |      |                         |-->CH#3 |  ||             |  |      |                         |-->CH#3 |
             |  |      *----------------------------------*  ||             |  |      *----------------------------------*
             |  |                                            ||             |  |
             |  |      [VCAP#0-VI#1]                         ||             |  |      [VCAP#0-VI#1]
             |  |      *----------------------------------*  ||             |  |      *----------------------------------*
    X_CAP#1--o--o----->|-->VI#1-P0--*-->Demux#0--*-->CH#0 |  ||    X_CAP#1--o--o----->|-->VI#1-P0--*-->Demux#0--*-->CH#0 |
             |         |            |            |-->CH#1 |  ||             |         |            |            |-->CH#1 |
             o-------->|-->VI#1-P1--*-->Demux#1--*-->CH#2 |  ||             o-------->|-->VI#1-P1--*-->Demux#1--*-->CH#2 |
                       |                         |-->CH#3 |  ||                       |                         |-->CH#3 |
                       *----------------------------------*  ||                       *----------------------------------*
                                                             ||
                       [VCAP#0-VI#2]                         ||                       [VCAP#0-VI#2]
    X_CAP#0            *----------------------------------*  ||                       *----------------------------------*
    X_CAP#2-----o--o-->|-->VI#2-P0--*-->Demux#0--*-->CH#0 |  ||    X_CAP#0--o--o----->|-->VI#2-P0--*-->Demux#0--*-->CH#0 |
                |  |   |            |            |-->CH#1 |  ||             |         |            |            |-->CH#1 |
             o--(->o-->|-->VI#2-P1--*-->Demux#1--*-->CH#2 |  ||             o-------->|-->VI#2-P1--*-->Demux#1--*-->CH#2 |
             |  |      |                         |-->CH#3 |  ||                       |                         |-->CH#3 |
             |  |      *----------------------------------*  ||                       *----------------------------------*
             |  |                                            ||
             |  |      [VCAP#0-VI#3]                         ||                       [VCAP#0-VI#3]
    X_CAP#1  |  |      *----------------------------------*  ||                       *----------------------------------*
    X_CAP#3--o--o----->|-->VI#3-P0--*-->Demux#0--*-->CH#0 |  ||    X_CAP#1--o--o----->|-->VI#3-P0--*-->Demux#0--*-->CH#0 |
             |         |            |            |-->CH#1 |  ||             |         |            |            |-->CH#1 |
             o-------->|-->VI#3-P1--*-->Demux#1--*-->CH#2 |  ||             o-------->|-->VI#3-P1--*-->Demux#1--*-->CH#2 |
                       |                         |-->CH#3 |  ||                       |                         |-->CH#3 |
                       *----------------------------------*  ||                       *----------------------------------*

    ======================================================================================================================
    <<NA51090>>
    ======================================================================================================================
    [NT98636]
    ======================================================================================================================
                       [VCAP#0-VI#0]                         ||                                  [VCAP#0-VI#0]
                       *----------------------------------*  ||                                  *------------------*
    X_CAP#0--o-------->|-->VI#0-P0--*-->Demux#0--*-->CH#0 |  ||                       *-->VC#0-->|         *-->CH#0 |
             |         |            |            |-->CH#1 |  ||    CSI#0--o---->|\    |-->VC#1-->|         |-->CH#1 |
    X_CAP#1--(-o------>|-->VI#0-P1--*-->Demux#1--*-->CH#2 |  ||           |     | >-->o          |-->VI#0--o        |
             | |       |                         |-->CH#3 |  ||    CSI#1--(-o-->|/    |-->VC#2-->|         |-->CH#2 |
             | |       *----------------------------------*  ||           | |         *-->VC#3-->|         *-->CH#3 |
             | |                                             ||           | |                    *------------------*
             | |       [VCAP#0-VI#1]                         ||           | |
             | |       *----------------------------------*  ||           | |                     [VCAP#0-VI#1]
             | o------>|-->VI#1-P0--*-->Demux#0--*-->CH#0 |  ||           | |                    *------------------*
             |         |            |            |-->CH#1 |  ||           | |         *-->VC#0-->|         *-->CH#0 |
             o-------->|-->VI#1-P1--*-->Demux#1--*-->CH#2 |  ||           o-(-->|\    |-->VC#1-->|         |-->CH#1 |
                       |                         |-->CH#3 |  ||             |   | >-->o          |-->VI#1--o        |
                       *----------------------------------*  ||             o-->|/    |-->VC#2-->|         |-->CH#2 |
                                                             ||                       *-->VC#3-->|         *-->CH#3 |
                       [VCAP#0-VI#2]                         ||                                  *------------------*
                       *----------------------------------*  ||
    X_CAP#2--o-------->|-->VI#2-P0--*-->Demux#0--*-->CH#0 |  ||                                  [VCAP#0-VI#2]
             |         |            |            |-->CH#1 |  ||                                  *------------------*
    X_CAP#3--(-o------>|-->VI#2-P1--*-->Demux#1--*-->CH#2 |  ||                       *-->VC#0-->|         *-->CH#0 |
             | |       |                         |-->CH#3 |  ||    CSI#2--o---->|\    |-->VC#1-->|         |-->CH#1 |
             | |       *----------------------------------*  ||           |     | >-->o          |-->VI#2--o        |
             | |                                             ||    CSI#3--(-o-->|/    |-->VC#2-->|         |-->CH#2 |
             | |       [VCAP#0-VI#3]                         ||           | |         *-->VC#3-->|         *-->CH#3 |
             | |       *----------------------------------*  ||           | |                    *------------------*
             | o------>|-->VI#3-P0--*-->Demux#0--*-->CH#0 |  ||           | |
             |         |            |            |-->CH#1 |  ||           | |                     [VCAP#0-VI#3]
             o-------->|-->VI#3-P1--*-->Demux#1--*-->CH#2 |  ||           | |                    *------------------*
                       |                         |-->CH#3 |  ||           | |         *-->VC#0-->|         *-->CH#0 |
                       *----------------------------------*  ||           o-(-->|\    |-->VC#1-->|         |-->CH#1 |
                                                             ||             |   | >-->o          |-->VI#3--o        |
                       [VCAP#1-VI#0]                         ||             o-->|/    |-->VC#2-->|         |-->CH#2 |
                       *----------------------------------*  ||                       *-->VC#3-->|         *-->CH#3 |
    X_CAP#4--o-------->|-->VI#0-P0--*-->Demux#0--*-->CH#0 |  ||                                  *------------------*
             |         |            |            |-->CH#1 |  ||
    X_CAP#5--(-o------>|-->VI#0-P1--*-->Demux#1--*-->CH#2 |  ||                                  [VCAP#1-VI#0]
             | |       |                         |-->CH#3 |  ||                                  *------------------*
             | |       *----------------------------------*  ||                       *-->VC#0-->|         *-->CH#0 |
             | |                                             ||    CSI#4--o---->|\    |-->VC#1-->|         |-->CH#1 |
             | |       [VCAP#1-VI#1]                         ||           |     | >-->o          |-->VI#0--o        |
             | |       *----------------------------------*  ||    CSI#5--(-o-->|/    |-->VC#2-->|         |-->CH#2 |
             | o------>|-->VI#1-P0--*-->Demux#0--*-->CH#0 |  ||           | |         *-->VC#3-->|         *-->CH#3 |
             |         |            |            |-->CH#1 |  ||           | |                    *------------------*
             o-------->|-->VI#1-P1--*-->Demux#1--*-->CH#2 |  ||           | |
                       |                         |-->CH#3 |  ||           | |                     [VCAP#1-VI#1]
                       *----------------------------------*  ||           | |                    *------------------*
                                                             ||           | |         *-->VC#0-->|         *-->CH#0 |
                       [VCAP#1-VI#2]                         ||           o-(-->|\    |-->VC#1-->|         |-->CH#1 |
                       *----------------------------------*  ||             |   | >-->o          |-->VI#1--o        |
    X_CAP#6--o-------->|-->VI#2-P0--*-->Demux#0--*-->CH#0 |  ||             o-->|/    |-->VC#2-->|         |-->CH#2 |
             |         |            |            |-->CH#1 |  ||                       *-->VC#3-->|         *-->CH#3 |
    X_CAP#7--(-o------>|-->VI#2-P1--*-->Demux#1--*-->CH#2 |  ||                                  *------------------*
             | |       |                         |-->CH#3 |  ||
             | |       *----------------------------------*  ||                                  [VCAP#1-VI#2]
             | |                                             ||                                  *------------------*
             | |       [VCAP#1-VI#3]                         ||                       *-->VC#0-->|         *-->CH#0 |
             | |       *----------------------------------*  ||    CSI#6--o---->|\    |-->VC#1-->|         |-->CH#1 |
             | o------>|-->VI#3-P0--*-->Demux#0--*-->CH#0 |  ||           |     | >-->o          |-->VI#2--o        |
             |         |            |            |-->CH#1 |  ||    CSI#7--(-o-->|/    |-->VC#2-->|         |-->CH#2 |
             o-------->|-->VI#3-P1--*-->Demux#1--*-->CH#2 |  ||           | |         *-->VC#3-->|         *-->CH#3 |
                       |                         |-->CH#3 |  ||           | |                    *------------------*
                       *----------------------------------*  ||           | |
                                                             ||           | |                     [VCAP#1-VI#3]
                                                             ||           | |                    *------------------*
                                                             ||           | |         *-->VC#0-->|         *-->CH#0 |
                                                             ||           o-(-->|\    |-->VC#1-->|         |-->CH#1 |
                                                             ||             |   | >-->o          |-->VI#3--o        |
                                                             ||             o-->|/    |-->VC#2-->|         |-->CH#2 |
                                                             ||                       *-->VC#3-->|         *-->CH#3 |
                                                             ||                                  *------------------*

    ---------------------------------------------------------------------------------------------------------------------
    [Mode] < BT656/BT1120 >
    ---------------------------------------------------------------------------------------------------------------------
    << Single Edge Latch >>
    1CH Bypass   : X_CAP#0->VI#0-P0------------------->CH#0     (BT656/BT656DH/BT1120)
    4CH Mux      : X_CAP#0->VI#0-P0->Demux#0---------->CH#0,1   (BT656)
                          ->VI#0-P1->Demux#1---------->CH#2,3   (BT656)
    2CH Mux      : X_CAP#0->VI#0-P0->Demux#0---------->CH#0,2   (BT656)
    2CH 2P Bypass: X_CAP#0->VI#0-P0------------------->CH#0     (BT656/BT656DH)
                 : X_CAP#1->VI#0-P1------------------->CH#2     (BT656/BT656DH)
    4CH 2P Mux   : X_CAP#0->VI#0-P0->Demux#0---------->CH#0,1   (BT656)
                 : X_CAP#1->VI#0-P1->Demux#1---------->CH#2,3   (BT656)
    3CH 2P Mix1  : X_CAP#0->VI#0-P0------------------->CH#0     (BT656/BT656DH)
                 : X_CAP#1->VI#0-P1->Demux#1---------->CH#2,3   (BT656)
    3CH 2P Mix2  : X_CAP#0->VI#0-P0->Demux#0---------->CH#0,1   (BT656)
                   X_CAP#1->VI#0-P1------------------->CH#2     (BT656/BT656DH)

    << Dual Edge Latch >>
    1CH Bypass   : X_CAP#0->VI#0-P0(rising)----------->CH#0     (BT656DE/BT1120DE)
                          ->VI#0-P1(falling)--------*           (BT656DE)
    2CH 2P Bypass: X_CAP#0->VI#0-P0(rising+falling)--->CH#0     (BT656DE)
                 : X_CAP#1->VI#0-P1(rising+falling)--->CH#2     (BT656DE)
    2CH DualEdge : X_CAP#0->VI#0-P0(rising )---------->CH#0     (BT656)
                          ->VI#0-P1(falling)---------->CH#2     (BT656)
    4CH Mux      : X_CAP#0->VI#0-P0(rising )->Demux#0->CH#0,1   (BT656)
                          ->VI#0-P1(falling)->Demux#1->CH#2,3   (BT656)

**************************************************************************************/

typedef enum {
    VCAP_IOC_VI_SRC_XCAP0 = 0,             ///< VI input source from X_CAP#0           (bt656),                support in NT98313/NT98316/NT98323/NT98321
    VCAP_IOC_VI_SRC_XCAP1,                 ///< VI input source from X_CAP#1           (bt656),                support in NT98313/NT98316/NT98323/NT98321
    VCAP_IOC_VI_SRC_XCAP2,                 ///< VI input source from X_CAP#2           (bt656),                support in NT98313/NT98316/NT98323
    VCAP_IOC_VI_SRC_XCAP3,                 ///< VI input source from X_CAP#3           (bt656),                support in NT98313/NT98316/NT98323
    VCAP_IOC_VI_SRC_XCAP4,                 ///< VI input source from X_CAP#4           (bt656),                support in NT98316
    VCAP_IOC_VI_SRC_XCAP5,                 ///< VI input source from X_CAP#5           (bt656),                support in NT98316
    VCAP_IOC_VI_SRC_XCAP6,                 ///< VI input source from X_CAP#6           (bt656),                support in NT98316
    VCAP_IOC_VI_SRC_XCAP7,                 ///< VI input source from X_CAP#7           (bt656),                support in NT98316

    VCAP_IOC_VI_SRC_XCAP0_1,               ///< VI input source from X_CAP#0 + X_CAP#1 (bt656/bt656x2/bt1120), support in NT98313/NT98316/NT98323/NT98321
    VCAP_IOC_VI_SRC_XCAP2_3,               ///< VI input source from X_CAP#2 + X_CAP#3 (bt656/bt656x2/bt1120), support in NT98313/NT98316/NT98323
    VCAP_IOC_VI_SRC_XCAP4_5,               ///< VI input source from X_CAP#4 + X_CAP#5 (bt656/bt656x2/bt1120), support in NT98316
    VCAP_IOC_VI_SRC_XCAP6_7,               ///< VI input source from X_CAP#6 + X_CAP#7 (bt656/bt656x2/bt1120), support in NT98316

    VCAP_IOC_VI_SRC_CSI0,                  ///< VI input source from CSI#0             (csi),                  support in NT98636
    VCAP_IOC_VI_SRC_CSI1,                  ///< VI input source from CSI#1             (csi),                  support in NT98636
    VCAP_IOC_VI_SRC_CSI2,                  ///< VI input source from CSI#2             (csi),                  support in NT98636
    VCAP_IOC_VI_SRC_CSI3,                  ///< VI input source from CSI#3             (csi),                  support in NT98636
    VCAP_IOC_VI_SRC_CSI4,                  ///< VI input source from CSI#4             (csi),                  support in NT98636
    VCAP_IOC_VI_SRC_CSI5,                  ///< VI input source from CSI#5             (csi),                  support in NT98636
    VCAP_IOC_VI_SRC_CSI6,                  ///< VI input source from CSI#6             (csi),                  support in NT98636
    VCAP_IOC_VI_SRC_CSI7,                  ///< VI input source from CSI#7             (csi),                  support in NT98636

    VCAP_IOC_VI_SRC_MAX
} VCAP_IOC_VI_SRC_T;

typedef enum {
    VCAP_IOC_VI_FMT_BT656 = 0,             ///< VI input source format BT656
    VCAP_IOC_VI_FMT_BT1120,                ///< VI input source format BT1120
    VCAP_IOC_VI_FMT_CSI,                   ///< VI input source format CSI
    VCAP_IOC_VI_FMT_MAX
} VCAP_IOC_VI_FMT_T;

typedef enum {
    VCAP_IOC_VI_TDM_1CH_BYPASS = 0,        ///< VI-CH#0,      rising edge from VI-P0
    VCAP_IOC_VI_TDM_4CH_MUX,               ///< VI-CH#0,1,2,3 rising edge from VI-P0
    VCAP_IOC_VI_TDM_2CH_DUALEDGE,          ///< VI-CH#0       rising edge from VI-P0 + VI-CH#2   falling edge from VI-P1
    VCAP_IOC_VI_TDM_2CH_MUX,               ///< VI-CH#0,2     rising edge from VI-P0
    VCAP_IOC_VI_TDM_2CH_2P_BYPASS,         ///< VI-CH#0       rising edge from VI-P0 + VI-CH#2   rising  edge from VI-P1, pinmux not support in NT98313/NT98316
    VCAP_IOC_VI_TDM_4CH_2P_MUX,            ///< VI-CH#0,1     rising edge from VI-P0 + VI-CH#2,3 rising  edge from VI-P1
    VCAP_IOC_VI_TDM_3CH_2P_MIX1,           ///< VI-CH#0       rising edge from VI-P0 + VI-CH#2,3 rising  edge from VI-P1, pinmux not support in NT98313/NT98316
    VCAP_IOC_VI_TDM_3CH_2P_MIX2,           ///< VI-CH#0,1     rising edge from VI-P0 + VI-CH#2   rising  edge from VI-P1, pinmux not support in NT98313/NT98316
    VCAP_IOC_VI_TDM_1CH_2P_DUP,            ///< VI-CH#0       rising edge from VI-P0 + VI-CH#2   rising  edge from VI-P1 duplicate from VI-P0
    VCAP_IOC_VI_TDM_MAX
} VCAP_IOC_VI_TDM_T;

typedef enum {
    VCAP_IOC_VI_CHID_NONE = 0,             ///< channel id extract disable
    VCAP_IOC_VI_CHID_EAV_SAV,              ///< channel id extract from fourth byte of EAV/SAV
    VCAP_IOC_VI_CHID_HORIZ_ACTIVE,         ///< channel id extract from horizontal active in vertical blank
    VCAP_IOC_VI_CHID_HORIZ_BLANK,          ///< channel id extract from horizontal blank
    VCAP_IOC_VI_CHID_MAX
} VCAP_IOC_VI_CHID_T;

typedef enum {
    VCAP_IOC_VI_LATCH_EDGE_SINGLE = 0,     ///< data latch on falling or  rising edge
    VCAP_IOC_VI_LATCH_EDGE_DUAL,           ///< data latch on falling and rising edge
    VCAP_IOC_VI_LATCH_EDGE_MAX
} VCAP_IOC_VI_LATCH_EDGE_T;

typedef enum {
    VCAP_IOC_VI_VPORT_DATA_SWAP_NONE = 0,  ///< disable data bit swap
    VCAP_IOC_VI_VPORT_DATA_SWAP_BIT,       ///< enable  data bit swap
    VCAP_IOC_VI_VPORT_DATA_SWAP_MAX
} VCAP_IOC_VI_VPORT_DATA_SWAP_T;

struct vcap_ioc_vi_t {
    unsigned int                         version;       ///< ioctl structure version, must set for version check

    unsigned char                        chip;          ///< chip index
    unsigned char                        vcap;          ///< vcap index
    unsigned char                        vi;            ///< vi   index

    /* VI Global Parameter */
    struct {
        VCAP_IOC_VI_SRC_T                src;           ///< vi input source
        VCAP_IOC_VI_FMT_T                format;        ///< vi input source format
        VCAP_IOC_VI_TDM_T                tdm;           ///< vi time division multiplexed mode
        VCAP_IOC_VI_CHID_T               id_extract;    ///< vi channel id extract mode for 2CH_MUX or 4CH_MUX
        VCAP_IOC_VI_LATCH_EDGE_T         latch_edge;    ///< vi data latch edge for 2CH_DualEdge or 4CH_2P_MUX_DualEdge
        unsigned int                     reserved[4];   ///< reserved
    } global;

    /* VI VPort Parameter */
    struct {
        VCAP_IOC_VI_VPORT_DATA_SWAP_T    data_swap;     ///< vport data swap control
        unsigned int                     clk_pin;       ///< vport pixel clock pin source
        unsigned int                     clk_inv;       ///< vport pixel clock invertion, 0:disable 1:enable
        unsigned int                     clk_dly;       ///< vport pixel clock delay
        unsigned int                     clk_pdly;      ///< vport pixel clock polarity delay, not support in NT9831x/NT9832x
        unsigned int                     reserved[4];   ///< reserved
    } vport[VCAP_IOC_VI_VPORT_MAX];                     ///< [0]=>VI#-P0, [1]=>VI#-P1
};

struct vcap_ioc_vi_id_t {
    unsigned int  version;                              ///< ioctl structure version, must set for version check
    unsigned char chip;                                 ///< chip index
    unsigned char vcap;                                 ///< vcap index
    unsigned char vi;                                   ///< vi   index
};

/*************************************************************************************
 *  VCAP IOCTL VI VPort Definition
 *************************************************************************************/
typedef enum {
    VCAP_IOC_VI_VPORT_PARAM_CLK_INV = 0,                ///< vi vport pixel clock invertion, parameter value as 0:disable 1:enable
    VCAP_IOC_VI_VPORT_PARAM_CLK_DELAY,                  ///< vi vport pixel clock delay, parameter value as 0 ~ 0x1f
    VCAP_IOC_VI_VPORT_PARAM_CLK_PDELAY,                 ///< vi vport pixel clock polarity delay, not support in NT98313/NT98316
    VCAP_IOC_VI_VPORT_PARAM_DATA_SWAP,                  ///< vi vport data swap control, parameter value as VCAP_IOC_VI_VPORT_DATA_SWAP_T
    VCAP_IOC_VI_VPORT_PARAM_MAX
} VCAP_IOC_VI_VPORT_PARAM_T;

struct vcap_ioc_vi_vport_param_t {
    unsigned int  version;                              ///< ioctl structure version, must input for version check

    unsigned char chip;                                 ///< chip  index
    unsigned char vcap;                                 ///< vcap  index
    unsigned char vi;                                   ///< vi    index
    unsigned char vport;                                ///< vport index

    unsigned int  pid;                                  ///< vport parameter index, value as VCAP_IOC_VI_VPORT_PARAM_T
    unsigned int  value;                                ///< vport parameter get/set value
    unsigned int  reserved[4];
};

/*************************************************************************************
 *  VCAP IOCTL VI Channel Definition
 *************************************************************************************/
typedef enum {
    VCAP_IOC_VI_CH_FMT_BT656 = 0,                       ///< BT656  Standard
    VCAP_IOC_VI_CH_FMT_BT1120,                          ///< BT1120 Standard
    VCAP_IOC_VI_CH_FMT_BT656DH,                         ///< BT656  DualHeader
    VCAP_IOC_VI_CH_FMT_CSI_16BIT,                       ///< CSI    16bit
    VCAP_IOC_VI_CH_FMT_MAX
} VCAP_IOC_VI_CH_FMT_T;

typedef enum {
    VCAP_IOC_VI_CH_PROG_INTERLACE = 0,                  ///< interlace   signal
    VCAP_IOC_VI_CH_PROG_PROGRESSIVE,                    ///< progressive signal
    VCAP_IOC_VI_CH_PROG_MAX
} VCAP_IOC_VI_CH_PROG_T;

typedef enum {
    VCAP_IOC_VI_CH_ORDER_ANYONE = 0,                    ///< for progressive signal
    VCAP_IOC_VI_CH_ORDER_ODD_FIRST,                     ///< for interlace   signal
    VCAP_IOC_VI_CH_ORDER_EVEN_FIRST,                    ///< for interlace   signal
    VCAP_IOC_VI_CH_ORDER_MAX
} VCAP_IOC_VI_CH_ORDER_T;

typedef enum {
    VCAP_IOC_VI_CH_DATA_RANGE_256 = 0,                  ///< output data range 256 level
    VCAP_IOC_VI_CH_DATA_RANGE_240,                      ///< output data rnage 240 level
    VCAP_IOC_VI_CH_DATA_RANGE_MAX
} VCAP_IOC_VI_CH_DATA_RANGE_T;

typedef enum {
    VCAP_IOC_VI_CH_SWAP_NONE = 0,                       ///< disable data swap
    VCAP_IOC_VI_CH_SWAP_YC,                             ///< data YC swap
    VCAP_IOC_VI_CH_SWAP_CbCr,                           ///< data CbCr swap
    VCAP_IOC_VI_CH_SWAP_YC_CbCr,                        ///< data YC + CbCr swap
    VCAP_IOC_VI_CH_SWAP_MAX
} VCAP_IOC_VI_CH_SWAP_T;

typedef enum {
    VCAP_IOC_VI_CH_DATA_RATE_SINGLE = 0,                ///< single data rate
    VCAP_IOC_VI_CH_DATA_RATE_DOUBLE,                    ///< double date rate, byte duplicate
    VCAP_IOC_VI_CH_DATA_RATE_MAX
} VCAP_IOC_VI_CH_DATA_RATE_T;

typedef enum {
    VCAP_IOC_VI_CH_DATA_LATCH_SINGLE = 0,               ///< data latch on rising or  falling
    VCAP_IOC_VI_CH_DATA_LATCH_DUAL,                     ///< data latch on rising and falling
    VCAP_IOC_VI_CH_DATA_LATCH_MAX
} VCAP_IOC_VI_CH_DATA_LATCH_T;

typedef enum {
    VCAP_IOC_VI_CH_HORIZ_DUP_OFF = 0,                   ///< video horizontal pixel duplicate off
    VCAP_IOC_VI_CH_HORIZ_DUP_2X,                        ///< video horizontal pixel duplicate 2x, pixel duplicate
    VCAP_IOC_VI_CH_HORIZ_DUP_4X,                        ///< video horizontal pixel duplicate 4x, pixel duplicate
    VCAP_IOC_VI_CH_HORIZ_DUP_2X_OVS,                    ///< video horizontal pixel duplicate 2x, pixel over sampling
    VCAP_IOC_VI_CH_HORIZ_DUP_MAX
} VCAP_IOC_VI_CH_HORIZ_DUP_T;

typedef enum {
    VCAP_IOC_VI_CH_DATA_SRC_VI = 0,                     ///< data source from external video interface
    VCAP_IOC_VI_CH_DATA_SRC_PATGEN,                     ///< data source from internal pattern generator, only support in NT98321/NT98323
    VCAP_IOC_VI_CH_DATA_SRC_MAX
} VCAP_IOC_VI_CH_DATA_SRC_T;

typedef enum {
    VCAP_IOC_VI_CH_CSI_VC_0 = 0,                        ///< data source from CSI virtual channel#0 when channel format is CSI_16BIT
    VCAP_IOC_VI_CH_CSI_VC_1,                            ///< data source from CSI virtual channel#1 when channel format is CSI_16BIT
    VCAP_IOC_VI_CH_CSI_VC_2,                            ///< data source from CSI virtual channel#2 when channel format is CSI_16BIT
    VCAP_IOC_VI_CH_CSI_VC_3,                            ///< data source from CSI virtual channel#3 when channel format is CSI_16BIT
    VCAP_IOC_VI_CH_CSI_VC_MAX
} VCAP_IOC_VI_CH_CSI_VC_T;

struct vcap_ioc_vi_ch_norm_t {
    unsigned int                    version;            ///< ioctl structure version, must set for version check

    unsigned char                   chip;               ///< chip index
    unsigned char                   vcap;               ///< vcap index
    unsigned char                   vi;                 ///< vi   index
    unsigned char                   ch;                 ///< ch   index

    unsigned int                    cap_width;          ///< channel capture width
    unsigned int                    cap_height;         ///< channel capture height
    unsigned int                    org_width;          ///< channel original width,  video source width
    unsigned int                    org_height;         ///< channel original height, video source height
    unsigned int                    fps;                ///< channel frame rate, must > 0
    VCAP_IOC_VI_CH_FMT_T            format;             ///< channel format
    VCAP_IOC_VI_CH_PROG_T           prog;               ///< channel progressive/interlace
    VCAP_IOC_VI_CH_DATA_RATE_T      data_rate;          ///< channel data rate, for specify byte duplicate mode
    VCAP_IOC_VI_CH_DATA_LATCH_T     data_latch;         ///< channel data latch mode
    VCAP_IOC_VI_CH_HORIZ_DUP_T      horiz_dup;          ///< channel horizontal pixel duplicate mode
    VCAP_IOC_VI_CH_DATA_SRC_T       data_src;           ///< channel data source
    unsigned int                    reserved[7];        ///< reserved
};

typedef enum {
    VCAP_IOC_VI_CH_PARAM_DATA_RANGE,                    ///< channel output data range, parameter value as VCAP_IOC_VI_CH_DATA_RATE_T
    VCAP_IOC_VI_CH_PARAM_YC_SWAP,                       ///< channel output ycbcr swap, parameter value as VCAP_IOC_VI_CH_SWAP_T
    VCAP_IOC_VI_CH_PARAM_FPS,                           ///< channel source frame rate, parameter value must > 0
    VCAP_IOC_VI_CH_PARAM_TIMEOUT_MS,                    ///< channel capture frame timeout ms, parameter value must > 0
    VCAP_IOC_VI_CH_PARAM_VCH_ID,                        ///< channel video index => mapping to physical connector index, means VCH index, parameter value as int
    VCAP_IOC_VI_CH_PARAM_VLOS,                          ///< channel video loss status, 0:video present 1:video loss
    VCAP_IOC_VI_CH_PARAM_CSI_VC,                        ///< channel csi virtual channel selection, parameter value as VCAP_IOC_VI_CH_CSI_VC_T
    VCAP_IOC_VI_CH_PARAM_MAX
} VCAP_IOC_VI_CH_PARAM_T;

struct vcap_ioc_vi_ch_param_t {
    unsigned int  version;                              ///< ioctl structure version, must input for version check

    unsigned char chip;                                 ///< chip index
    unsigned char vcap;                                 ///< vcap index
    unsigned char vi;                                   ///< vi   index
    unsigned char ch;                                   ///< ch   index

    unsigned int  pid;                                  ///< vi channel parameter index, value as VCAP_IOC_VI_CH_PARAM_T
    unsigned int  value;                                ///< vi channel parameter get/set value
    unsigned int  reserved[4];
};

struct vcap_ioc_vi_ch_id_t {
    unsigned int  version;                              ///< ioctl structure version, must set for version check

    unsigned char chip;                                 ///< chip index
    unsigned char vcap;                                 ///< vcap index
    unsigned char vi;                                   ///< vi   index
    unsigned char ch;                                   ///< ch   index
};

/*************************************************************************************
 *  VCAP IOCTL VI Channel Path Definition
 *************************************************************************************/
typedef enum {
    VCAP_IOC_VI_CH_PATH_DMA_OVF_DROP_AUTO = 0,          ///< dma overflow drop auto, base on dma overflow retrieve mode, per-path
    VCAP_IOC_VI_CH_PATH_DMA_OVF_DROP_FORCE_LINE,        ///< dma overflow drop line,  per-path
    VCAP_IOC_VI_CH_PATH_DMA_OVF_DROP_FORCE_FRAME,       ///< dma overflow drop frame, per-path
    VCAP_IOC_VI_CH_PATH_DMA_OVF_DROP_MAX
} VCAP_IOC_VI_CH_PATH_DMA_OVF_DROP_T;

typedef enum {
    VCAP_IOC_VI_CH_PATH_PARAM_DMA_OVF_DROP = 0,         ///< channel path dma overflow drop mode, parameter value as VCAP_IOC_VI_CH_PATH_DMA_OVF_DROP_T
    VCAP_IOC_VI_CH_PATH_PARAM_MAX
} VCAP_IOC_VI_CH_PATH_PARAM_T;

struct vcap_ioc_vi_ch_path_param_t {
    unsigned int  version;                              ///< ioctl structure version, must input for version check

    unsigned char chip;                                 ///< chip index
    unsigned char vcap;                                 ///< vcap index
    unsigned char vi;                                   ///< vi   index
    unsigned char ch;                                   ///< ch   index
    unsigned char path;                                 ///< path index 0 ~ 3, the path index support 0 ~ 7 if enable vi duplication

    unsigned int  pid;                                  ///< vi channel path parameter index, value as VCAP_IOC_VI_CH_PATH_PARAM_T
    unsigned int  value;                                ///< vi channel path parameter get/set value
    unsigned int  reserved[4];
};

/*************************************************************************************
 *  VCAP IOCTL Scaler Definition
 *************************************************************************************/
typedef enum {
    VCAP_IOC_SCALER_FIELD_ORDER_SWAP_OFF = 0,           ///< the field is the top field when frame count value is even
    VCAP_IOC_SCALER_FIELD_ORDER_SWAP_ON,                ///< the field is the top field when frame count value is odd
    VCAP_IOC_SCALER_FIELD_ORDER_SWAP_MAX
} VCAP_IOC_SCALER_FIELD_ORDER_SWAP_T;

typedef enum {
    VCAP_IOC_SCALER_LUMA_SHRINK_LVL_2PIXEL = 0,         ///< shrink 2 pixels
    VCAP_IOC_SCALER_LUMA_SHRINK_LVL_4PIXEL,             ///< shrink 4 pixels
    VCAP_IOC_SCALER_LUMA_SHRINK_LVL_6PIXEL,             ///< shrink 6 pixels
    VCAP_IOC_SCALER_LUMA_SHRINK_LVL_MAX
} VCAP_IOC_SCALER_LUMA_SHRINK_LVL_T;

typedef enum {
    VCAP_IOC_SCALER_PATH_LUMA_SHRINK_OFF = 0,           ///< disable mask shrink in YUV420 mode
    VCAP_IOC_SCALER_PATH_LUMA_SHRINK_ON,                ///< enable  mask shrink in YUV420 mode
    VCAP_IOC_SCALER_PATH_LUMA_SHRINK_MAX
} VCAP_IOC_SCALER_PATH_LUMA_SHRINK_T;

typedef enum {
    VCAP_IOC_SCALER_PATH_V_LUMA_ALG_AUTO = 0,           ///< judge by HW algorithm
    VCAP_IOC_SCALER_PATH_V_LUMA_ALG_BYPASS,             ///< bypass
    VCAP_IOC_SCALER_PATH_V_LUMA_ALG_LPF,
    VCAP_IOC_SCALER_PATH_V_LUMA_ALG_LPPBI,
    VCAP_IOC_SCALER_PATH_V_LUMA_ALG_MAX
} VCAP_IOC_SCALER_PATH_V_LUMA_ALG_T;

typedef enum {
    VCAP_IOC_SCALER_PATH_H_LUMA_ALG_AUTO = 0,           ///< judge by HW algorithm
    VCAP_IOC_SCALER_PATH_H_LUMA_ALG_BYPASS,             ///< bypass
    VCAP_IOC_SCALER_PATH_H_LUMA_ALG_LPF,
    VCAP_IOC_SCALER_PATH_H_LUMA_ALG_LPPBI,
    VCAP_IOC_SCALER_PATH_H_LUMA_ALG_MAX
} VCAP_IOC_SCALER_PATH_H_LUMA_ALG_T;

typedef enum {
    VCAP_IOC_SCALER_PATH_V_CHROMA_ALG_AUTO = 0,         ///< judge by HW algorithm
    VCAP_IOC_SCALER_PATH_V_CHROMA_ALG_BYPASS,           ///< bypass
    VCAP_IOC_SCALER_PATH_V_CHROMA_ALG_BI,               ///< bi-inter
    VCAP_IOC_SCALER_PATH_V_CHROMA_ALG_AVG,              ///< average
    VCAP_IOC_SCALER_PATH_V_CHROMA_ALG_MAX
} VCAP_IOC_SCALER_PATH_V_CHROMA_ALG_T;

typedef enum {
    VCAP_IOC_SCALER_PATH_H_CHROMA_ALG_AUTO = 0,         ///< judge by HW algorithm
    VCAP_IOC_SCALER_PATH_H_CHROMA_ALG_BYPASS,           ///< bypass
    VCAP_IOC_SCALER_PATH_H_CHROMA_ALG_BI,               ///< bi-inter
    VCAP_IOC_SCALER_PATH_H_CHROMA_ALG_AVG,              ///< average
    VCAP_IOC_SCALER_PATH_H_CHROMA_ALG_MAX
} VCAP_IOC_SCALER_PATH_H_CHROMA_ALG_T;

typedef enum {
    VCAP_IOC_SCALER_PATH_MAP_SELECT_0 = 0,              ///< without 0.5 pixel distance shift, apply on vertical scaling up
    VCAP_IOC_SCALER_PATH_MAP_SELECT_1,                  ///< with    0.5 pixel distance shift, apply on vertical scaling down
    VCAP_IOC_SCALER_PATH_MAP_SELECT_MAX
} VCAP_IOC_SCALER_PATH_MAP_SELECT_T;

typedef enum {
    VCAP_IOC_SCALER_PATH_CHROMA_SRC_METHOD_A = 0,      ///< chroma channel scaling input source method A
    VCAP_IOC_SCALER_PATH_CHROMA_SRC_METHOD_B,          ///< chroma channel scaling input source method B
    VCAP_IOC_SCALER_PATH_CHROMA_SRC_MAX
} VCAP_IOC_SCALER_PATH_CHROMA_SRC_T;

typedef enum {
    VCAP_IOC_SCALER_PATH_MAP_ADJUSTMENT_AUTO = 0,       ///< auto   adjustment scaler map
    VCAP_IOC_SCALER_PATH_MAP_ADJUSTMENT_MANUAL,         ///< manual adjustment scaler map
    VCAP_IOC_SCALER_PATH_MAP_ADJUSTMENT_MAX
} VCAP_IOC_SCALER_PATH_MAP_ADJUSTMENT_T;

typedef enum {
    VCAP_IOC_SCALER_PATH_ALG_ADJUSTMENT_AUTO = 0,       ///< auto   adjustment scaler h/v algorithm
    VCAP_IOC_SCALER_PATH_ALG_ADJUSTMENT_MANUAL,         ///< manual adjustment scaler h/v algorithm
    VCAP_IOC_SCALER_PATH_ALG_ADJUSTMENT_MANUAL_V,       ///< manual adjustment scaler v   algorithm
    VCAP_IOC_SCALER_PATH_ALG_ADJUSTMENT_MANUAL_H,       ///< manual adjustment scaler h   algorithm
    VCAP_IOC_SCALER_PATH_ALG_ADJUSTMENT_MAX
} VCAP_IOC_SCALER_PATH_ALG_ADJUSTMENT_T;

typedef enum {
    VCAP_IOC_SCALER_PATH_COEF_ADJUSTMENT_AUTO = 0,      ///< auto   adjustment scaler h/v coeficient
    VCAP_IOC_SCALER_PATH_COEF_ADJUSTMENT_MANUAL,        ///< manual adjustment scaler h/v coeficient
    VCAP_IOC_SCALER_PATH_COEF_ADJUSTMENT_MANUAL_V,      ///< manual adjustment scaler v   coeficient
    VCAP_IOC_SCALER_PATH_COEF_ADJUSTMENT_MANUAL_H,      ///< manual adjustment scaler h   coeficient
    VCAP_IOC_SCALER_PATH_COEF_ADJUSTMENT_MAX
} VCAP_IOC_SCALER_PATH_COEF_ADJUSTMENT_T;

typedef enum {
    VCAP_IOC_SCALER_PARAM_FIELD_ORDER_SWAP = 0,         ///< scaler field order swap, per-channel, 0:disable 1:enable
    VCAP_IOC_SCALER_PARAM_LUMA_SHRINK_LVL,              ///< scaler luma mask shrink level, per-channel, 0:2pixel 1:4pixel 2:6pixel
    VCAP_IOC_SCALER_PARAM_PATH_LUMA_SHRINK,             ///< scaler luma mask shrink for yuv420 mode
    VCAP_IOC_SCALER_PARAM_PATH_V_LUMA_ALG,              ///< scaler vertical   luma   algorithm
    VCAP_IOC_SCALER_PARAM_PATH_H_LUMA_ALG,              ///< scaler horizontal luma   algorithm
    VCAP_IOC_SCALER_PARAM_PATH_V_CHROMA_ALG,            ///< scaler vertical   chroma algorithm
    VCAP_IOC_SCALER_PARAM_PATH_H_CHROMA_ALG,            ///< scaler horizontal chroma algorithm
    VCAP_IOC_SCALER_PARAM_PATH_V_COEF_0,                ///< scaler LPF coeficient#0 in vertical   direction
    VCAP_IOC_SCALER_PARAM_PATH_V_COEF_1,                ///< scaler LPF coeficient#1 in vertical   direction
    VCAP_IOC_SCALER_PARAM_PATH_V_COEF_2,                ///< scaler LPF coeficient#2 in vertical   direction
    VCAP_IOC_SCALER_PARAM_PATH_V_COEF_3,                ///< scaler LPF coeficient#3 in vertical   direction
    VCAP_IOC_SCALER_PARAM_PATH_H_COEF_0,                ///< scaler LPF coeficient#0 in horizontal direction
    VCAP_IOC_SCALER_PARAM_PATH_H_COEF_1,                ///< scaler LPF coeficient#1 in horizontal direction
    VCAP_IOC_SCALER_PARAM_PATH_H_COEF_2,                ///< scaler LPF coeficient#2 in horizontal direction
    VCAP_IOC_SCALER_PARAM_PATH_H_COEF_3,                ///< scaler LPF coeficient#3 in horizontal direction
    VCAP_IOC_SCALER_PARAM_PATH_MAP_SELECT,              ///< scaler source mapping format, 0 or 1
    VCAP_IOC_SCALER_PARAM_PATH_HSTEP_OFFSET,            ///< scaler horizontal step offset
    VCAP_IOC_SCALER_PARAM_PATH_PRESCAL_THRES,           ///< scaler pre-scaling enable threshold, 0, 1 is disable, 2x ~ 8x scaling down ratio to enable
    VCAP_IOC_SCALER_PARAM_PATH_COEF_ADJUSTMENT,         ///< scaler coeficient adjustment mode, 0:auto 1:manual 2:manual_v 3:manual_h
    VCAP_IOC_SCALER_PARAM_PATH_ALG_ADJUSTMENT,          ///< scaler algorithm  adjustment mode, 0:auto 1:manual 2:manual_v 3:manual_h
    VCAP_IOC_SCALER_PARAM_PATH_MAP_ADJUSTMENT,          ///< scaler source mapping adjustment mode, 0:auto 1:manual
    VCAP_IOC_SCALER_PARAM_PATH_CHROMA_SRC,              ///< scaler chroma source select

    VCAP_IOC_SCALER_PARAM_MAX
} VCAP_IOC_SCALER_PARAM_T;

struct vcap_ioc_scaler_param_t {
    unsigned int  version;                              ///< ioctl structure version, must set for version check

    unsigned char chip;                                 ///< chip index
    unsigned char vcap;                                 ///< vcap index
    unsigned char vi;                                   ///< vi   index
    unsigned char ch;                                   ///< ch   index
    unsigned char path;                                 ///< path index, scaler path 0 ~ 3, the path index support 0 ~ 7 if enable vi duplication

    unsigned int  pid;                                  ///< scaler parameter index, value as VCAP_IOC_SCALER_PARAM_T
    unsigned int  value;                                ///< scaler parameter get/set value
    unsigned int  reserved[4];
};

/*************************************************************************************
 *  VCAP IOCTL DMA Definition
 *************************************************************************************/
typedef enum {
    VCAP_IOC_DMA_CH_0 = 0,                              ///< dma channel for image
    VCAP_IOC_DMA_CH_1,                                  ///< dma channel for image
    VCAP_IOC_DMA_CH_2,                                  ///< dma channel for motion data
    VCAP_IOC_DMA_CH_MAX
} VCAP_IOC_DMA_CH_T;

typedef enum {
    VCAP_IOC_DMA_PARAM_LINE_BURST_INTERVAL = 0,         ///< DMA wait interval for one line burst, only dma#0,1, value 0 ~ 0xffff
    VCAP_IOC_DMA_PARAM_WRITE_BURST_INTERVAL,            ///< DMA write channel wait interval for each DMA write burst, only dma#0,1,2, value 0 ~ 0xffff
    VCAP_IOC_DMA_PARAM_READ_BURST_INTERVAL,             ///< DMA read channel  wait interval for each DMA read burst, only dma#2, value 0 ~ 0xffff
    VCAP_IOC_DMA_PARAM_FIFO_WATERMARK_LOW_THRES,        ///< DMA image FIFO watermark low threshold, only dma#0,1, value 0 ~ 0xff
    VCAP_IOC_DMA_PARAM_FIFO_WATERMARK_HIGH_THRES,       ///< DMA image FIFO watermark high threshold, only dma#0,1, value 0 ~ 0xff
    VCAP_IOC_DMA_PARAM_JOB_WATERMARK_LOW_THRES,         ///< DMA job FIFO watermark low threshold, only dma#0,1, value 0 ~ 0x7f
    VCAP_IOC_DMA_PARAM_JOB_WATERMARK_HIGH_THRES,        ///< DMA job FIFO watermark high threshold, only dma#0,1, value 0 ~ 0x7f
    VCAP_IOC_DMA_PARAM_TRAFFIC_CTRL,                    ///< DMA traffic control, only dma#0,1, value 0:off 1:on, BIT#0=>DDR#0, BIT#1=>DDR#1
    VCAP_IOC_DMA_PARAM_MAX
} VCAP_IOC_DMA_PARAM_T;

struct vcap_ioc_dma_param_t {
    unsigned int        version;                        ///< ioctl structure version, must set for version check

    unsigned char       chip;                           ///< chip index
    unsigned char       vcap;                           ///< vcap index
    VCAP_IOC_DMA_CH_T   dma_ch;                         ///< dma channel index

    unsigned int        pid;                            ///< dma channel parameter index, value as VCAP_IOC_DMA_PARAM_T
    unsigned int        value;                          ///< dma channel parameter get/set value
    unsigned int        reserved[4];
};

/*************************************************************************************
 *  VCAP IOCTL Motion Detection Definition
 *************************************************************************************/
#define VCAP_IOC_MD_MB_LEVEL_MAX                4

struct vcap_ioc_md_ctrl_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index
    unsigned char vi;                           ///< vi   index
    unsigned char ch;                           ///< ch   index

    unsigned int  enable;                       ///< motion detection 0:disable 1:enable
    unsigned int  reserved[4];                  ///< reserved words
};

struct vcap_ioc_md_reset_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index
    unsigned char vi;                           ///< vi   index
    unsigned char ch;                           ///< ch   index

    unsigned int  sta_clear;                    ///< motion detection statistic clear 0:none 1:clear
    unsigned int  reserved[4];                  ///< reserved words
};

struct vcap_ioc_md_mb_size_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index
    unsigned char vi;                           ///< vi   index
    unsigned char ch;                           ///< ch   index

    unsigned int  x_size;                       ///< motion detection macro block width,  16/32 pixel, only support x_size = y_size
    unsigned int  y_size;                       ///< motion detection macro block height, 16/32 pixel, only support x_size = y_size
    unsigned int  reserved[2];                  ///< reserved words
};

struct vcap_ioc_md_info_t {
    unsigned int        version;                ///< ioctl structure version, must set for version check

    unsigned char       chip;                   ///< chip index
    unsigned char       vcap;                   ///< vcap index
    unsigned char       vi;                     ///< vi   index
    unsigned char       ch;                     ///< ch   index

    unsigned int        enable;                 ///< md enable, 0: disable 1: enable
    unsigned int        mb_x_start;             ///< macro block x start pixel, 0 ~ 4095
    unsigned int        mb_y_start;             ///< macro block y start line,  0 ~ 4095
    unsigned int        mb_x_num;               ///< horizontal macro block number, 2 ~ 256
    unsigned int        mb_y_num;               ///< vertical macro block number,   2 ~ 256
    unsigned int        mb_x_size;              ///< macro block width,  16/32 pixel
    unsigned int        mb_y_size;              ///< macro block height, 16/32 pixel
    uintptr_t           event_pa;               ///< event buffer physical address, data size depend on 32/64bit system
    uintptr_t           event_va;               ///< event buffer virtual  address, data size depend on 32/64bit system
    unsigned int        event_size;             ///< event buffer size (Byte)
    unsigned int        event_ddrid;            ///< event buffer ddr id
    uintptr_t           level_pa;               ///< level buffer physical address, data size depend on 32/64bit system
    uintptr_t           level_va;               ///< level buffer virtual  address, data size depend on 32/64bit system
    unsigned int        level_size;             ///< level buffer size (Byte)
    unsigned int        level_ddrid;            ///< level buffer ddr id
    uintptr_t           sta_pa;                 ///< sta   buffer physical address, data size depend on 32/64bit system
    uintptr_t           sta_va;                 ///< sta   buffer virtual  address, data size depend on 32/64bit system
    unsigned int        sta_size;               ///< sta   buffer size (Byte)
    unsigned int        sta_ddrid;              ///< sta   buffer ddr id

    unsigned int        sync_ctrl;              ///< sync  buffer data 0:none 1:cache flush + sync EP data to RC, BIT[0]:Event, BIT[1]:STA

    unsigned int        reserved[3];            ///< reserved words
};

/*
 * MD event data format:
 *    event value => 2 bits for one motion block(MB), bit0:Learning Base bit1:Difference Base => 0:foreground 1:background
 *
 * MSB----------------------> 512 bits <-------------------------LSB
 *  |-------------------------------------------------------------|
 *  | MB255 | MB254 |...........................| MB2 | MB1 | MB0 |  ==> row#0
 *  |-------------------------------------------------------------|
 *  | MB255 | MB254 |...........................| MB2 | MB1 | MB0 |  ==> row#1
 *  |-------------------------------------------------------------|
 *  | MB255 | MB254 |...........................| MB2 | MB1 | MB0 |  ==> row#2
 *  |-------------------------------------------------------------|
 *  | MB255 | MB254 |...........................| MB2 | MB1 | MB0 |  ==> row#3
 *  |-------------------------------------------------------------|
 *  | ........................................................... |   :
 *  | ........................................................... |   :
 *  | ........................................................... |  ==> row#255
 * MSB-----------------------------------------------------------LSB
 *
 */

typedef enum {
    VCAP_IOC_MD_OUTPUT_MODE_ANYONE = 0,         ///< md gaussian/event data output for each frame/field
    VCAP_IOC_MD_OUTPUT_MODE_ODD_FIELD,          ///< md gaussian/event data output when interlace field is odd (field_bit = 0), output anyone for progressive frame
    VCAP_IOC_MD_OUTPUT_MODE_EVEN_FIELD,         ///< md gaussian/event data output when interlace field is even(field_bit = 1), output anyone for progressive frame
    VCAP_IOC_MD_OUTPUT_MODE_SKIP_NEXT,          ///< md gaussian/event data output one frame/field then skip next frame/field
    VCAP_IOC_MD_OUTPUT_MODE_MAX
} VCAP_IOC_MD_OUTPUT_MODE_T;

typedef enum {
    VCAP_IOC_MD_PARAM_TIME_PERIOD = 0,          ///< time period of event updation for difference based , 0 ~ 0x1f
    VCAP_IOC_MD_PARAM_DXDY,                     ///< CU to MD 1/(dx * dy), 0 ~ 0xffff
    VCAP_IOC_MD_PARAM_TBG,                      ///< 0 ~ 0x7fff
    VCAP_IOC_MD_PARAM_SCENE_CHANGE_THRES,       ///< scene change threshold, 0 ~ 0xff
    VCAP_IOC_MD_PARAM_OUTPUT_MODE,              ///< md gaussian/event output mode, 0 ~ 3, must to disable md before switch output mode
    VCAP_IOC_MD_PARAM_LVL_ALPHA,                ///< speed of update, 0 ~ 0x7fff
    VCAP_IOC_MD_PARAM_LVL_ONE_MIN_ALPHA,        ///< 2^15 - alpha, 0 ~ 0x7fff
    VCAP_IOC_MD_PARAM_LVL_INIT_WEIGHT,          ///< initial weight, 0 ~ 0x3fff
    VCAP_IOC_MD_PARAM_LVL_MODEL_UPDATE,         ///< initial value for the mixture of Gaussians, 0 ~ 1
    VCAP_IOC_MD_PARAM_LVL_TB,                   ///< threshold on the squared Mahalan, 0 ~ 0x1f
    VCAP_IOC_MD_PARAM_LVL_SIGMA,                ///< initial standard deviation for the newly generated components, 0 ~ 0x1f
    VCAP_IOC_MD_PARAM_LVL_TG,                   ///< threshold on the squared Mahalan, 0 ~ 0x1f
    VCAP_IOC_MD_PARAM_LVL_PRUNE,                ///< prune = -(alpha * CT)*8191/256, 0 ~ 0xffff;
    VCAP_IOC_MD_PARAM_LVL_LUMA_DIFF_THRES,      ///< luma difference threshold, 0 ~ 0xff
    VCAP_IOC_MD_PARAM_LVL_TEXT_DIFF_THRES,      ///< texture difference threshold, 0 ~ 0x7ff
    VCAP_IOC_MD_PARAM_LVL_TEXT_THRES,           ///< texture threshold, 0 ~ 0x7ff
    VCAP_IOC_MD_PARAM_LVL_TEXT_RATIO_THRES,     ///< texture ratio threshold, 0 ~ 0xff
    VCAP_IOC_MD_PARAM_LVL_GM_MD2_THRES,         ///< md Threshold for difference base, 0 ~ 0xfff
    VCAP_IOC_MD_PARAM_MAX
} VCAP_IOC_MD_PARAM_T;

struct vcap_ioc_md_param_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index
    unsigned char vi;                           ///< vi   index
    unsigned char ch;                           ///< ch   index

    unsigned int  level;                        ///< md level index, 0~3
    unsigned int  pid;                          ///< md parameter index, value as VCAP_IOC_MD_PARAM_T
    unsigned int  value;                        ///< md parameter value
    unsigned int  reserved[4];                  ///< reserved words
};

struct vcap_ioc_md_mb_level_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index
    unsigned char vi;                           ///< vi   index
    unsigned char ch;                           ///< ch   index

    unsigned int  x_start;                      ///< start x mb position
    unsigned int  y_start;                      ///< start y mb position
    unsigned int  x_num;                        ///< number of horizontal mb
    unsigned int  y_num;                        ///< number of vertical mb
    unsigned int  level;                        ///< mb level, 0 ~ 3
    unsigned int  reserved[4];                  ///< reserved words
};

struct vcap_ioc_dma_buf_t {
    uintptr_t    paddr;                         ///< buffer physical address, data size depend on 32/64bit system
    unsigned int size;                          ///< buffer size (Byte)
    unsigned int ddr_id;                        ///< RC=>0,1 EP0=>2, EP1=>3,4, EP2=>5
};

struct vcap_ioc_md_buf_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index
    unsigned char vi;                           ///< vi   index
    unsigned char ch;                           ///< ch   index

    unsigned int mb_x_num_max;                  ///< max motion block x number for this md buffer, 2 ~ 256
    unsigned int mb_y_num_max;                  ///< max motion block y number for this md buffer, 2 ~ 256

    struct vcap_ioc_dma_buf_t sta;              ///< md sta   dma buffer, start address must 16byte alignment, from RC local memory
    struct vcap_ioc_dma_buf_t event;            ///< md event dma buffer, start address must 16byte alignment, from RC local memory
    struct vcap_ioc_dma_buf_t level;            ///< md level dma buffer, start address must 16byte alignment, from RC local memory

    struct vcap_ioc_dma_buf_t ep_sta;           ///< md sta   dma buffer, start address must 16byte alignment, from EP local memory
    struct vcap_ioc_dma_buf_t ep_event;         ///< md event dma buffer, start address must 16byte alignment, from EP local memory
    struct vcap_ioc_dma_buf_t ep_level;         ///< md level dma buffer, start address must 16byte alignment, from EP local memory
};

/*************************************************************************************
 *  VCAP IOCTL Tamper Detection Definition
 *************************************************************************************/
typedef enum {
    VCAP_IOC_TD_TYPE_EDGE,                      ///< edge      based tamper detection
    VCAP_IOC_TD_TYPE_INTENSITY,                 ///< intensity based tamper detection
    VCAP_IOC_TD_TYPE_MAX
} VCAP_IOC_TD_TYPE_T;

struct vcap_ioc_td_info_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index
    unsigned char vi;                           ///< vi   index
    unsigned char ch;                           ///< ch   index

    VCAP_IOC_TD_TYPE_T  type;                   ///< tamper operation type, 0: edge base 1: intensity base
    unsigned int        alarm;                  ///< tamper alarm status,   0: Non-Alarm 1: Alarm
    unsigned int        reserved[4];            ///< Reserved words
};

typedef enum {
    VCAP_IOC_TD_PARAM_TYPE = 0,                 ///< Tamper operation type, 0: edge based 1: intensity based
    VCAP_IOC_TD_PARAM_EDGE_TEX_THRES,           ///< Edge Tamper Texture Threshold, apply on EDGE type, (0 ~ 255), means edge strength
    VCAP_IOC_TD_PARAM_EDGE_WIN_THRES,           ///< Edge Tamper Window Threshold, apply on EDGE type, (0 ~ 128)*100%, over 128 means disable
    VCAP_IOC_TD_PARAM_AVG_TEX_THRES,            ///< Average Tamper Texture Threshold, apply on INTENSITY type, (0 ~ 255), means Y value
    VCAP_IOC_TD_PARAM_AVG_WIN_THRES,            ///< Average Tamper Window Threshold, apply on INTENSITY type, (0~128)*100%, over 128 means disable
    VCAP_IOC_TD_PARAM_MAX
} VCAP_IOC_TD_PARAM_T;

struct vcap_ioc_td_param_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index
    unsigned char vi;                           ///< vi   index
    unsigned char ch;                           ///< ch   index

    unsigned int  pid;                          ///< tamper parameter index, value as VCAP_IOC_TD_PARAM_T
    unsigned int  value;                        ///< tamper parameter value
    unsigned int  reserved[4];                  ///< reserved words
};

/*************************************************************************************
 *  VCAP IOCTL Palette Definition
 *************************************************************************************/
#define VCAP_IOC_PALETTE_MAX                    8

/* capture palette control */
struct vcap_ioc_palette_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index

    unsigned int  p_id;                         ///< palette index, 0 ~ 7
    unsigned int  color_yuv;                    ///< palette color YUV => Y[7:0] Cb[15:8] Cr[23:16]
    unsigned int  reserved;                     ///< reserved words
};

/*************************************************************************************
 *  VCAP IOCTL Mask Definition
 *************************************************************************************/
#define VCAP_IOC_MASK_WIN_MAX                   16
#define VCAP_IOC_MASK_COLOR_MAX                 VCAP_IOC_PALETTE_MAX
#define VCAP_IOC_MASK_WIDTH_MAX                 4096
#define VCAP_IOC_MASK_HEIGHT_MAX                4096

typedef enum {
    VCAP_IOC_MASK_ALPHA_0 = 0,                  ///< alpha 0%
    VCAP_IOC_MASK_ALPHA_25,                     ///< alpha 25%
    VCAP_IOC_MASK_ALPHA_37_5,                   ///< alpha 37.5%
    VCAP_IOC_MASK_ALPHA_50,                     ///< alpha 50%
    VCAP_IOC_MASK_ALPHA_62_5,                   ///< alpha 62.5%
    VCAP_IOC_MASK_ALPHA_75,                     ///< alpha 75%
    VCAP_IOC_MASK_ALPHA_87_5,                   ///< alpha 87.5%
    VCAP_IOC_MASK_ALPHA_100,                    ///< alpha 100%
    VCAP_IOC_MASK_ALPHA_MAX
} VCAP_IOC_MASK_ALPHA_T;

struct vcap_ioc_mask_ctrl_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index
    unsigned char vi;                           ///< vi   index
    unsigned char ch;                           ///< ch   index
    unsigned char path;                         ///< path index 0 ~ 3, the path index support 0 ~ 7 if enable vi duplication

    unsigned int  win_idx;                      ///< mask window index, 0 ~ 15
    unsigned int  win_enb;                      ///< mask window 0:disable 1:enable
    unsigned int  reserved[4];                  ///< reserved words
};

struct vcap_ioc_mask_win_t {
    unsigned int            version;            ///< ioctl structure version, must set for version check

    unsigned char           chip;               ///< chip index
    unsigned char           vcap;               ///< vcap index
    unsigned char           vi;                 ///< vi   index
    unsigned char           ch;                 ///< ch   index

    unsigned int            win_idx;            ///< mask window index, 0 ~ 15
    unsigned int            x_start;            ///< mask window x start position, base on virtual plane
    unsigned int            y_start;            ///< mark window y start position, base on virtual plane
    unsigned int            width;              ///< mask window width,  base on virtual plane
    unsigned int            height;             ///< mask window height, base on virtual plane
    unsigned int            bg_width;           ///< mask virtual plane background width
    unsigned int            bg_height;          ///< mask virtual plane background height
    unsigned int            color;              ///< mask window color, palette index 0 ~ 7
    VCAP_IOC_MASK_ALPHA_T   alpha;              ///< mask window alpha
    unsigned int            reserved[4];        ///< reserved words
};

/*************************************************************************************
 *  VCAP IOCTL VideoGraph Information Definition
 *************************************************************************************/
#define VCAP_IOC_VG_VCH_MAX                     32

struct vcap_ioc_vg_info_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char host;                         ///< host index
    unsigned int  nr_of_vch :8;                 ///< number of video channel
    unsigned int  nr_of_path:4;                 ///< number of video output path for each video channel
    unsigned int  rsvd      :20;                ///< reserved
    struct {
        unsigned int vch_id:8;                  ///< video channel index
        unsigned int chip  :4;                  ///< chip index of this video channel
        unsigned int vcap  :4;                  ///< vcap index of this video channel from chip
        unsigned int vi    :4;                  ///< vi   index of this video channel from vcap
        unsigned int ch    :4;                  ///< ch   index of this video channel from vi
        unsigned int fps   :8;                  ///< video channel source framerate

        unsigned int width :14;                 ///< video channel source width
        unsigned int height:14;                 ///< video channel source height
        unsigned int prog  :1;                  ///< video channel source progressive, 0: interlace 1: progressive
        unsigned int vloss :1;                  ///< video channel source video loss,  0: present   1: vloss
        unsigned int rsvd  :2;                  ///< reserved
    } vch[VCAP_IOC_VG_VCH_MAX];
};

/*************************************************************************************
 *  VCAP IOCTL PatGen Definition
 *************************************************************************************/
typedef enum {
    VCAP_IOC_PATGEN_NORM_640X480P25 = 0,
    VCAP_IOC_PATGEN_NORM_640X480P30,
    VCAP_IOC_PATGEN_NORM_1280X720P25,
    VCAP_IOC_PATGEN_NORM_1280X720P30,
    VCAP_IOC_PATGEN_NORM_1920X1080P25,
    VCAP_IOC_PATGEN_NORM_1920X1080P30,
    VCAP_IOC_PATGEN_NORM_960X1080P25,
    VCAP_IOC_PATGEN_NORM_960X1080P30,
    VCAP_IOC_PATGEN_NORM_1920X1080P15,
    VCAP_IOC_PATGEN_NORM_MAX
} VCAP_IOC_PATGEN_NORM_T;

typedef enum {
    VCAP_IOC_PATGEN_MODE_FIX_PATTERN = 0,       ///< fix    pattern mode
    VCAP_IOC_PATGEN_MODE_LFSR                   ///< random pattern mode
} VCAP_IOC_PATGEN_MODE_T;

struct vcap_ioc_patgen_info_t {
    unsigned int           version;             ///< ioctl structure version, must set for version check

    unsigned char          chip;                ///< chip index
    unsigned char          vcap;                ///< vcap index

    unsigned int           enable;              ///< patgen enable, 0:disable 1:enable
    VCAP_IOC_PATGEN_NORM_T norm;                ///< patgen output norm
    VCAP_IOC_PATGEN_MODE_T mode;                ///< patgen output mode
    unsigned int           width;               ///< patgen output width
    unsigned int           height;              ///< patgen output height
    unsigned int           frame_rate;          ///< patgen output frame rate
    unsigned int           fix_pattern;         ///< patgen output fix pattern value
    unsigned int           lfsr_init_tap;       ///< patgen output LFSR init tap value
    unsigned int           reserved[4];         ///< reserved words
};

typedef enum {
    VCAP_IOC_PATGEN_PARAM_NORM = 0,             ///< patgen output norm
    VCAP_IOC_PATGEN_PARAM_ENABLE,               ///< patgen enable control, 0:disable 1:enable
    VCAP_IOC_PATGEN_PARAM_MODE,                 ///< patgen output mode
    VCAP_IOC_PATGEN_PARAM_FIX_PATTERN,          ///< patgen fix pattern value
    VCAP_IOC_PATGEN_PARAM_LFSR_INIT_TAP,        ///< patgen LFSR initial tap
    VCAP_IOC_PATGEN_PARAM_MAX
} VCAP_IOC_PATGEN_PARAM_T;

struct vcap_ioc_patgen_param_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index

    unsigned int  pid;                          ///< patgen parameter index, value as VCAP_IOC_PATGEN_PARAM_T
    unsigned int  value;                        ///< patgen parameter value
    unsigned int  reserved[4];                  ///< reserved words
};

/*************************************************************************************
 *  VCAP IOCTL Misc Definition
 *************************************************************************************/
typedef enum {
    VCAP_IOC_MISC_PARAM_MD_TIMESTAMP = 0,       ///< MD timestamp output control, 0:disable 1:timestamp padding after last mb of sta buffer
    VCAP_IOC_MISC_PARAM_MAX
} VCAP_IOC_MISC_PARAM_T;

struct vcap_ioc_misc_param_t {
    unsigned int  version;                      ///< ioctl structure version, must set for version check

    unsigned char chip;                         ///< chip index
    unsigned char vcap;                         ///< vcap index

    unsigned int  pid;                          ///< patgen parameter index, value as VCAP_IOC_MISC_PARAM_T
    unsigned int  value;                        ///< patgen parameter value
    unsigned int  reserved[4];                  ///< reserved words
};

/*************************************************************************************
 *  VCAP IOCTL
 *************************************************************************************/
#define VCAP_IOC_MAGIC                      'V'

#define VCAP_IOC_VER                        _VOS_IOR(VCAP_IOC_MAGIC,  1, unsigned int)
#define VCAP_IOC_HOST_INIT                  _VOS_IOW(VCAP_IOC_MAGIC,  2, struct vcap_ioc_host_t)
#define VCAP_IOC_HOST_UNINIT                _VOS_IOW(VCAP_IOC_MAGIC,  3, struct vcap_ioc_host_id_t)
#define VCAP_IOC_VG_FD_MAKE                 _VOS_IOR(VCAP_IOC_MAGIC,  4, struct vcap_ioc_vg_fd_t)
#define VCAP_IOC_VG_FD_CONVERT              _VOS_IOR(VCAP_IOC_MAGIC,  5, struct vcap_ioc_vg_fd_t)
#define VCAP_IOC_VG_GET_INFO                _VOS_IOR(VCAP_IOC_MAGIC,  6, struct vcap_ioc_vg_info_t)
#define VCAP_IOC_HW_ABILITY                 _VOS_IOR(VCAP_IOC_MAGIC,  7, struct vcap_ioc_hw_ability_t)
#define VCAP_IOC_HW_LIMIT                   _VOS_IOR(VCAP_IOC_MAGIC,  8, struct vcap_ioc_hw_limit_t)

#define VCAP_IOC_VI_REGISTER                _VOS_IOW(VCAP_IOC_MAGIC, 10, struct vcap_ioc_vi_t)
#define VCAP_IOC_VI_DEREGISTER              _VOS_IOW(VCAP_IOC_MAGIC, 11, struct vcap_ioc_vi_id_t)
#define VCAP_IOC_VI_VPORT_GET_PARAM         _VOS_IOR(VCAP_IOC_MAGIC, 12, struct vcap_ioc_vi_vport_param_t)
#define VCAP_IOC_VI_VPORT_SET_PARAM         _VOS_IOW(VCAP_IOC_MAGIC, 13, struct vcap_ioc_vi_vport_param_t)
#define VCAP_IOC_VI_GET_INFO                _VOS_IOR(VCAP_IOC_MAGIC, 14, struct vcap_ioc_vi_t)

#define VCAP_IOC_VI_CH_GET_NORM             _VOS_IOR(VCAP_IOC_MAGIC, 20, struct vcap_ioc_vi_ch_norm_t)
#define VCAP_IOC_VI_CH_SET_NORM             _VOS_IOW(VCAP_IOC_MAGIC, 21, struct vcap_ioc_vi_ch_norm_t)
#define VCAP_IOC_VI_CH_GET_PARAM            _VOS_IOR(VCAP_IOC_MAGIC, 22, struct vcap_ioc_vi_ch_param_t)
#define VCAP_IOC_VI_CH_SET_PARAM            _VOS_IOW(VCAP_IOC_MAGIC, 23, struct vcap_ioc_vi_ch_param_t)
#define VCAP_IOC_VI_CH_PATH_GET_PARAM       _VOS_IOR(VCAP_IOC_MAGIC, 24, struct vcap_ioc_vi_ch_path_param_t)
#define VCAP_IOC_VI_CH_PATH_SET_PARAM       _VOS_IOW(VCAP_IOC_MAGIC, 25, struct vcap_ioc_vi_ch_path_param_t)

#define VCAP_IOC_DMA_GET_PARAM              _VOS_IOR(VCAP_IOC_MAGIC, 30, struct vcap_ioc_dma_param_t)
#define VCAP_IOC_DMA_SET_PARAM              _VOS_IOW(VCAP_IOC_MAGIC, 31, struct vcap_ioc_dma_param_t)
#define VCAP_IOC_PALETTE_GET_COLOR          _VOS_IOR(VCAP_IOC_MAGIC, 32, struct vcap_ioc_palette_t)
#define VCAP_IOC_PALETTE_SET_COLOR          _VOS_IOW(VCAP_IOC_MAGIC, 33, struct vcap_ioc_palette_t)

#define VCAP_IOC_SCALER_GET_PARAM           _VOS_IOR(VCAP_IOC_MAGIC, 40, struct vcap_ioc_scaler_param_t)
#define VCAP_IOC_SCALER_SET_PARAM           _VOS_IOW(VCAP_IOC_MAGIC, 41, struct vcap_ioc_scaler_param_t)

#define VCAP_IOC_MD_CTRL                    _VOS_IOW(VCAP_IOC_MAGIC, 50, struct vcap_ioc_md_ctrl_t)
#define VCAP_IOC_MD_GET_INFO                _VOS_IOR(VCAP_IOC_MAGIC, 51, struct vcap_ioc_md_info_t)
#define VCAP_IOC_MD_GET_PARAM               _VOS_IOR(VCAP_IOC_MAGIC, 52, struct vcap_ioc_md_param_t)
#define VCAP_IOC_MD_SET_PARAM               _VOS_IOW(VCAP_IOC_MAGIC, 53, struct vcap_ioc_md_param_t)
#define VCAP_IOC_MD_SET_MB_LEVEL            _VOS_IOW(VCAP_IOC_MAGIC, 54, struct vcap_ioc_md_mb_level_t)
#define VCAP_IOC_MD_BUFFER_ASSIGN           _VOS_IOW(VCAP_IOC_MAGIC, 55, struct vcap_ioc_md_buf_t)
#define VCAP_IOC_MD_BUFFER_RELEASE          _VOS_IOW(VCAP_IOC_MAGIC, 56, struct vcap_ioc_vi_ch_id_t)
#define VCAP_IOC_MD_RESET                   _VOS_IOW(VCAP_IOC_MAGIC, 57, struct vcap_ioc_md_reset_t)
#define VCAP_IOC_MD_GET_MB_SIZE             _VOS_IOR(VCAP_IOC_MAGIC, 58, struct vcap_ioc_md_mb_size_t)
#define VCAP_IOC_MD_SET_MB_SIZE             _VOS_IOW(VCAP_IOC_MAGIC, 59, struct vcap_ioc_md_mb_size_t)

#define VCAP_IOC_TD_GET_INFO                _VOS_IOR(VCAP_IOC_MAGIC, 60, struct vcap_ioc_td_info_t)
#define VCAP_IOC_TD_GET_PARAM               _VOS_IOR(VCAP_IOC_MAGIC, 61, struct vcap_ioc_td_param_t)
#define VCAP_IOC_TD_SET_PARAM               _VOS_IOW(VCAP_IOC_MAGIC, 62, struct vcap_ioc_td_param_t)

#define VCAP_IOC_MASK_CTRL                  _VOS_IOW(VCAP_IOC_MAGIC, 70, struct vcap_ioc_mask_ctrl_t)
#define VCAP_IOC_MASK_GET_WIN               _VOS_IOR(VCAP_IOC_MAGIC, 71, struct vcap_ioc_mask_win_t)
#define VCAP_IOC_MASK_SET_WIN               _VOS_IOW(VCAP_IOC_MAGIC, 72, struct vcap_ioc_mask_win_t)

#define VCAP_IOC_HOST_GET_SCALER_RULE       _VOS_IOR(VCAP_IOC_MAGIC, 80, struct vcap_ioc_host_scaler_rule_t)
#define VCAP_IOC_HOST_SET_SCALER_RULE       _VOS_IOW(VCAP_IOC_MAGIC, 81, struct vcap_ioc_host_scaler_rule_t)
#define VCAP_IOC_HOST_GET_FRAME_FILTER      _VOS_IOR(VCAP_IOC_MAGIC, 82, struct vcap_ioc_host_frame_filter_t)
#define VCAP_IOC_HOST_SET_FRAME_FILTER      _VOS_IOW(VCAP_IOC_MAGIC, 83, struct vcap_ioc_host_frame_filter_t)

#define VCAP_IOC_PATGEN_GET_INFO            _VOS_IOR(VCAP_IOC_MAGIC, 90, struct vcap_ioc_patgen_info_t)
#define VCAP_IOC_PATGEN_GET_PARAM           _VOS_IOR(VCAP_IOC_MAGIC, 91, struct vcap_ioc_patgen_param_t)
#define VCAP_IOC_PATGEN_SET_PARAM           _VOS_IOW(VCAP_IOC_MAGIC, 92, struct vcap_ioc_patgen_param_t)

#define VCAP_IOC_MISC_GET_PARAM             _VOS_IOR(VCAP_IOC_MAGIC, 100, struct vcap_ioc_misc_param_t)
#define VCAP_IOC_MISC_SET_PARAM             _VOS_IOW(VCAP_IOC_MAGIC, 101, struct vcap_ioc_misc_param_t)

#endif  /* _VCAP_IOC_H_ */
