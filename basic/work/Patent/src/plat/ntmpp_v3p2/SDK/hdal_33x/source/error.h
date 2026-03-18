/*
 *   @file   error.h
 *
 *   @brief  error message header file.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#ifndef __ERROR_H__   /* prevent multiple inclusion of the header file */
#define __ERROR_H__

typedef struct {
	int err_code;
	char *err_str;
} pif_err_code_info_t;

/* system error code */
#define ERR_INVALID_LIB_VER         0x81000001 /* library version error */
#define ERR_FAILED_OPEN_VPD_FILE    0x81000002 /* open "/dev/vpd" fail */
#define ERR_SYS_NOT_READY           0x81000003 /* Platform not ready */
#define ERR_INVALID_PLATFORM_TYPE   0x81000004 /* platformType(%d) error */
#define ERR_NOT_INIT_PLATFORM       0x81000005 /* gmlib not init */
#define ERR_INVALID_OPERATION_GROUPFD_NO_DELETE     0x81000006 /*  groupfd not release */
#define ERR_INVALID_OPERATION_OBJECT_NO_DELETE      0x81000007 /*  objects not release */
#define ERR_BAD_SYS_INFO_POINTER    0x81000008 /* Input system_info is NULL Pointer */
#define ERR_DUPLICATE_CVBS          0x81000009 /* duplicate CVBS error */
#define ERR_GRAPH_EXIT              0x8100000A /* duplicate graph exit error */
#define ERR_APPLY_FAIL              0x8100000B /* apply fail */
#define ERR_VER_OVERFLOW            0x8100000C /* vpd version overflow */
#define ERR_CREATE_THREAD           0x8100000D /* create thread fail */

/* object error */
#define ERR_BAD_OBJECT_POINTER      0x82000001 /* object type(%#x) error or obj is NULL Pointer */
#define ERR_BAD_IN_OBJECT_POINTER   0x82000002 /* In Object is NULL pointer,
                                                  in object illegal(%p,%#x)!
                                                  please check gm_new_obj() flow. */
#define ERR_BAD_OUT_OBJECT_POINTER  0x82000003 /* Out Object is NULL pointer,  out object illegal
                                                  (%p,%#x)! please check gm_new_obj() flow. */

#define ERR_INVALID_OBJTYPE         0x82000004 /* obj_type(%#x) error */
#define ERR_FAILED_BINDING          0x82000005 /* in(%s) out(%s) binding error! */
#define ERR_FAILED_DEL_OBJ_BY_NO_UNBIND           0x83000006 /* Delete obj error. bindfd no unbind */
#define ERR_FAILED_NEW_OBJ_BY_INVALID_OBJTYPE     0x83000007 /* Invalid object type */

/* group error */
#define ERR_BAD_GROUPFD_POINTER             0x83000001 /* Input groupfd is NULL pointer */
#define ERR_FAILED_OPERATION_GROUP_EMPTY    0x83000002 /* This group is empy */

/* binding error */
#define ERR_BAD_BINDFD_POINTER              0x84000001 /* Input bindfd is NULL pointer
                                                          or bindfd is not exist*/
#define ERR_FAILED_UNBIND_ALREADY_UNBIND    0x84000002 /* this bindfd(%p) already unbinded */
#define ERR_FAILED_UNBIND_BINDFD_NOT_EXIST  0x84000003 /* this bindfd(%p) bindfd is not exist */
#define ERR_BINDFD_IS_NOT_BIND_COMPLETE     0x84000004 /* bindfd is not bind complete */

/* attribute error */
#define ERR_BAD_ATTR_POINTER        0x85000001 /* attr(\"%s\") is not exist or null point */
#define ERR_INVALID_ATTR_TYPE       0x85000002 /* attr type invalid */
#define ERR_CHECK_TWO_PATH          0x85000003 /* attr type invalid */

/* tx/rx bitstream error */
#define ERR_INVALID_POLL_FDS_POINTER   0x86000001 /* Invalid poll fds pointer. */
#define ERR_INVALID_NUM_FDS_VALUE      0x86000002 /* Invalid number of fd. */
#define ERR_INVALID_SEND_BUF_POINTER   0x86000003 /* Invalid send buf point.*/
#define ERR_INVALID_SEND_BUF_LEN_VALUE 0x86000004 /* Invalid value of send buf lenght.*/
#define ERR_INVALID_RCV_BS_POINTER     0x86000005 /* Invalid received bs point.*/

/* osg */
#define ERR_FAILED_OSD_MARK            0x87000004 /* osd_mark failed */

/* user parameter error */
#define ERR_LCD_VCH_NEGATIVE_VALUE     0x88000001 /* lcd_vch was negative value */
#define ERR_NULL_STRUCTURE             0x88000002 /* structure pointer was NULL */
#define ERR_USER_PARAMETER             0x88000003 /* user's parameter has error */
#define ERR_NULL_BUF                   0x88000004 /* buffer pointer was NULL */
#define ERR_BUF_LEN_0                  0x88000005 /* buffer length was 0 */

/* general error */
#define ERR_NOT_SUPPORT                0x88888888 /* not support by lite version */
#define ERR_FAILED                     -1 /* general error */

#endif //#ifndef __ERROR_H__

