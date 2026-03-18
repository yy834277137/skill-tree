#ifndef __SSENIF_IOCTL_CMD_H_
#define __SSENIF_IOCTL_CMD_H_


#include <linux/ioctl.h>

#define SSENIF_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int ui_addr;
	unsigned int ui_value;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int ui_count;
	REG_INFO reg_list[SSENIF_REG_LIST_NUM];
} REG_INFO_LIST;

typedef struct csi_drv_sensor_type {
	unsigned int eng_id;                    // CSI engine id
	unsigned int sensor_type;               // Sensor type, value as KDRV_SSENIF_SENSORTYPE
} CSI_DRV_SENSOR_TYPE;

typedef struct csi_drv_target_mclk {
	unsigned int eng_id;                    // CSI engine id
	unsigned int target_mclk;               // Sensor normal operation target MCLK frequency
} CSI_DRV_TARGET_MCLK;

typedef struct csi_drv_real_mclk {
	unsigned int eng_id;                    // CSI engine id
	unsigned int real_mclk;                 // Sensor real operation MCLK frequency provided
} CSI_DRV_REAL_MCLK;

typedef struct csi_drv_dlane_number {
	unsigned int eng_id;                    // CSI engine id
	unsigned int dlane_number;              // Data lane number, 1/2/4 Data lane
} CSI_DRV_DLANE_NUMBER;

typedef struct csi_drv_vc_valid_height {
	unsigned int eng_id;                    // CSI engine id
	unsigned int vch_index;                 // CSI virtual channel index, 0~3
	unsigned int valid_height;              // Set valid height as virtual channel frame end line count
} CSI_DRV_VC_VALID_HEIGHT;

typedef struct csi_drv_clane_switch {
	unsigned int eng_id;                    // CSI engine id
	unsigned int clane_switch;              // Clock lane switch
} CSI_DRV_CLANE_SWITCH;

typedef struct csi_drv_sensor_real_hsclk {
	unsigned int eng_id;                    // CSI engine id
	unsigned int sensor_real_hsclk;         // Sensor real operation MIPI-HS frequency
} CSI_DRV_SENSOR_REAL_HSCLK;

typedef struct csi_drv_iadj {
	unsigned int eng_id;                    // CSI engine id
	unsigned int iadj;                      // Select IADJ level, value: 0~3
} CSI_DRV_IADJ;

typedef struct csi_drv_vc_id {
	unsigned int eng_id;                    // CSI engine id
	unsigned int vch_index;                 // CSI virtual channel index, 0~3
	unsigned int id;                        // Set the active channel id, 0~3
} CSI_DRV_VC_ID;

/**
	CSI Input Pad Pin definition

	For CSI / CSI3 / CSI5 / CSI7:
		0: From input Pad pin CSI_D0P/CSI_D0N.
		1: From input Pad pin CSI_D1P/CSI_D1N.
		2: From input Pad pin CSI_D2P/CSI_D2N.
		3: From input Pad pin CSI_D3P/CSI_D3N.

	For CSI2 / CSI4 / CSI6 / CSI8:
		0: From input Pad pin CSI_D2P/CSI_D2N.
		1: From input Pad pin CSI_D3P/CSI_D3N.
*/
typedef struct csi_drv_data_lane_pin {
	unsigned int eng_id;                    // CSI engine id
	unsigned int data_lane0;                // Specify sensor's data lane-0 is layout to ISP's which pad pin.
	unsigned int data_lane1;                // Specify sensor's data lane-1 is layout to ISP's which pad pin.
	unsigned int data_lane2;                // Specify sensor's data lane-2 is layout to ISP's which pad pin.
	unsigned int data_lane3;                // Specify sensor's data lane-3 is layout to ISP's which pad pin.
} CSI_DRV_DATA_LANE_PIN;

typedef struct csi_drv_hsdatao_dly {
	unsigned int eng_id;                    // CSI engine id
	unsigned int hs_data_out_dly;           // HS data out delay
	unsigned int hs_data_out_edge;          // Select OCLK edge to tune EN_HSDATAO.  0: Positive edge; 1: Negative edge
} CSI_DRV_HSDATAO_DLY;

typedef struct csi_timeout_period {
	unsigned int eng_id;                    // CSI engine id
	unsigned int timeout_period;            // CSI interrupt timeout period. Unit in ms. Default is 1000 ms.
} CSI_DRV_TIMEOUT_PERIOD;

typedef struct csi_hd_gating {
	unsigned int eng_id;                    // CSI engine id
	unsigned int hd_gating_en;              // Configure HD gating method. 0: disable; 1: enable
} CSI_DRV_HD_GATING;

typedef struct csi_force_clk_hs {
	unsigned int eng_id;                    // CSI engine id
	unsigned int force_clk_hs;              // Configure CSI Clock Lane forcing at High Speed State. 0: disable; 1: enable
} CSI_DRV_FORCE_CLK_HS;

typedef struct csi_hd_gen_method {
	unsigned int eng_id;                    // CSI engine id
	unsigned int hd_gen_mothod;             // Configure Hsync generation method from packet header or Hsync short packet.
} CSI_DRV_HD_GEN_METHOD;

typedef struct csi_pxclk_sel {
	unsigned int eng_id;                    // CSI engine id
	unsigned int pxclk_sel;                 // CSI pixel clock select
	unsigned int pxclk_en;                  // CSI pixel clock enable / disable
} CSI_DRV_PXCLK_SEL;

typedef struct csi_field_en {
	unsigned int eng_id;                    // CSI engine id
	unsigned int field_en;                  // CSI Field enable / disable
	unsigned int field_bit;                 // Set Field bit is on WC bit[n], n = 0x0 ~ 0xF
} CSI_DRV_FIELD_EN;

//============================================================================
// IOCTL command
//============================================================================
#define SSENIF_IOC_COMMON_TYPE 'M'
#define SSENIF_IOC_START                           _IO(SSENIF_IOC_COMMON_TYPE, 1)
#define SSENIF_IOC_STOP                            _IO(SSENIF_IOC_COMMON_TYPE, 2)

#define SSENIF_IOC_READ_REG                        _IOWR(SSENIF_IOC_COMMON_TYPE, 3, void*)
#define SSENIF_IOC_WRITE_REG                       _IOWR(SSENIF_IOC_COMMON_TYPE, 4, void*)
#define SSENIF_IOC_READ_REG_LIST                   _IOWR(SSENIF_IOC_COMMON_TYPE, 5, void*)
#define SSENIF_IOC_WRITE_REG_LIST                  _IOWR(SSENIF_IOC_COMMON_TYPE, 6, void*)

#define SSENIF_IOC_SET_CSI_SENSORTYPE              _IOWR(SSENIF_IOC_COMMON_TYPE,  7, CSI_DRV_SENSOR_TYPE)
#define SSENIF_IOC_SET_CSI_SENSOR_TARGET_MCLK      _IOWR(SSENIF_IOC_COMMON_TYPE,  8, CSI_DRV_TARGET_MCLK)
#define SSENIF_IOC_SET_CSI_SENSOR_REAL_MCLK        _IOWR(SSENIF_IOC_COMMON_TYPE,  9, CSI_DRV_REAL_MCLK)

#define SSENIF_IOC_SET_CSI_DLANE_NUMBER            _IOWR(SSENIF_IOC_COMMON_TYPE, 10, CSI_DRV_DLANE_NUMBER)
#define SSENIF_IOC_SET_CSI_CLANE_SWITCH            _IOWR(SSENIF_IOC_COMMON_TYPE, 12, CSI_DRV_CLANE_SWITCH)

#define SSENIF_IOC_SET_CSI_SENSOR_REAL_HSCLK       _IOWR(SSENIF_IOC_COMMON_TYPE, 13, CSI_DRV_SENSOR_REAL_HSCLK)
#define SSENIF_IOC_SET_CSI_IADJ                    _IOWR(SSENIF_IOC_COMMON_TYPE, 14, CSI_DRV_IADJ)

#define SSENIF_IOC_CSI_START                       _IOWR(SSENIF_IOC_COMMON_TYPE, 15, unsigned int)
#define SSENIF_IOC_CSI_STOP                        _IOWR(SSENIF_IOC_COMMON_TYPE, 16, unsigned int)
#define SSENIF_IOC_SET_CSI_FIELD_EN                _IOWR(SSENIF_IOC_COMMON_TYPE, 17, CSI_DRV_FIELD_EN)

#define SSENIF_IOC_SET_CSI_HSDATAO_DLY             _IOWR(SSENIF_IOC_COMMON_TYPE, 19, CSI_DRV_HSDATAO_DLY)
#define SSENIF_IOC_SET_CSI_PXCLK_SEL               _IOWR(SSENIF_IOC_COMMON_TYPE, 20, CSI_DRV_PXCLK_SEL)
#define SSENIF_IOC_SET_CSI_TIMEOUT_PERIOD          _IOWR(SSENIF_IOC_COMMON_TYPE, 21, CSI_DRV_TIMEOUT_PERIOD)
#define SSENIF_IOC_SET_CSI_HD_GATING               _IOWR(SSENIF_IOC_COMMON_TYPE, 22, CSI_DRV_HD_GATING)
#define SSENIF_IOC_SET_CSI_FORCE_CLK_HS            _IOWR(SSENIF_IOC_COMMON_TYPE, 23, CSI_DRV_FORCE_CLK_HS)
#define SSENIF_IOC_SET_CSI_HD_GEN_METHOD           _IOWR(SSENIF_IOC_COMMON_TYPE, 24, CSI_DRV_HD_GEN_METHOD)

#define SSENIF_IOC_SET_CSI_VC_VALID_HEIGHT         _IOWR(SSENIF_IOC_COMMON_TYPE, 30, CSI_DRV_VC_VALID_HEIGHT)
#define SSENIF_IOC_SET_CSI_VC_ID                   _IOWR(SSENIF_IOC_COMMON_TYPE, 31, CSI_DRV_VC_ID)
#define SSENIF_IOC_SET_CSI_DATA_LANE_PIN           _IOWR(SSENIF_IOC_COMMON_TYPE, 32, CSI_DRV_DATA_LANE_PIN)


#endif
