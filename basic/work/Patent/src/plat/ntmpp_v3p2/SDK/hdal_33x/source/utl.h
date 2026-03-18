/*
 *   @file   utl.h
 *
 *   @brief  utility function header.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#ifndef __UTL_H__     /* prevent multiple inclusion of the header file */
#define __UTL_H__

/*-----------------------------------------------------------------------------*/
/* HDAL						                                                   */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
/*
Bitmap:  same as em/common.h
|---type(8)----+--reserved(4)-+--chip(4)--+----engine(8)----+---minor(8)---|
*/

#define EM_TYPE_AUDIO_DEVICE 1
#define EM_TYPE_VIDEO_DEVICE 2

#define EM_TYPE_OFFSET      24
#define EM_TYPE_BITMAP      0xFF000000
#define EM_TYPE_MAX_VALUE   (EM_TYPE_BITMAP >> EM_TYPE_OFFSET)

#define EM_CHIP_OFFSET      20
#define EM_CHIP_BITMAP      0x00F00000
#define EM_CHIP_MAX_VALUE   (EM_CHIP_BITMAP >> EM_CHIP_OFFSET)

#define EM_ENGINE_OFFSET    12
#define EM_ENGINE_BITMAP    0x000FF000
#define EM_ENGINE_MAX_VALUE (EM_ENGINE_BITMAP >> EM_ENGINE_OFFSET)

#define EM_MINOR_OFFSET     0
#define EM_MINOR_BITMAP     0x00000FFF
#define EM_MINOR_MAX_VALUE  (EM_MINOR_BITMAP >> EM_MINOR_OFFSET)

#define ENTITY_FD(type, chip, engine, minor) \
	((type << EM_TYPE_OFFSET) | (chip << EM_CHIP_OFFSET) | (engine << EM_ENGINE_OFFSET) | minor)

#define ENTITY_FD_TYPE(fd)      ((fd & EM_TYPE_BITMAP) >> EM_TYPE_OFFSET)
#define ENTITY_FD_CHIP(fd)      ((fd & EM_CHIP_BITMAP) >> EM_CHIP_OFFSET)
#define ENTITY_FD_ENGINE(fd)    ((fd & EM_ENGINE_BITMAP) >> EM_ENGINE_OFFSET)
#define ENTITY_FD_MINOR(fd)     ((fd & EM_MINOR_BITMAP) >> EM_MINOR_OFFSET)


/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _UTL_FD_TABLE {
	HD_PATH_ID path_id;
	UINT32 entity_fd;
} UTL_FD_TABLE;

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them
/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
UINT32 utl_get_au_grab_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO out_subid, VOID *param);
UINT32 utl_get_au_render_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO out_subid, VOID *param);
UINT32 utl_get_cap_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param);
UINT32 utl_get_lcd_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO out_subid, VOID *param);
UINT32 utl_get_di_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param);
INT utl_get_vpe_minor(HD_DAL device_subid, HD_IO out_subid);
UINT32 utl_get_vpe_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param);
UINT32 utl_get_disp_osg_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO out_subid, VOID *param);
UINT32 utl_get_enc_osg_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO out_subid, VOID *param);
UINT32 utl_get_enc_osg_entity_fd2(HD_DAL device_subid, HD_IO out_subid, VOID *param);
UINT32 utl_get_dec_entity_fd(HD_DAL device_subid, HD_IO in_subid, VOID *param);
UINT32 utl_get_datain_entity_fd(HD_PATH_ID path_id);
UINT32 utl_get_enc_entity_fd(HD_DAL device_subid, HD_IO in_subid, VOID *param);
UINT32 utl_get_dataout_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param);
HD_RESULT utl_init(void);
HD_RESULT utl_uninit(void);
#endif //#ifndef __UTL_H__

