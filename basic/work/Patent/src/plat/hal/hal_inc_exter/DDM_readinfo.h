/* @file DDM_readinfo.h
 * @note HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 * @brief
 *
 * @author   Coordinates
 * @date     2017-02-21
 * @version  v1.0.0
 *
 * @note Hik-vision platform io configuration driver header file
 * @note History:        
 * @note     <Coordinates>   <2017-02-21>    <v1.0.0 >   <desc>
 * @note  
 * @warning
 */
#ifndef _DDM_READINFO_H_
#define _DDM_READINFO_H_

/*pcp driver ioctl magic define macro*/
#define DDM_IOC_PCP_BASE	            'P'
#define DDM_IOC_GET_HARDWRE_INFO        _IOW(DDM_IOC_PCP_BASE, 2, cpld_info)

#define DDR_4G_4x8Gb    15
#define DDR_4G_2x16Gb   5
#define DDR_8G_8x8Gb    14
#define DDR_8G_4x16Gb   2
#define DDR_2G_4x4Gb    4

/*Hik-vision define boot parameter encrypted by specific device*/
#ifndef CFG_MAGIC
#define  CFG_MAGIC          0x484b5753  /*HKWS */
#endif

#ifndef MAX_ETHERNET
#define MAX_ETHERNET		2
#endif

#ifndef MACADDR_LEN
#define MACADDR_LEN         6
#endif

#define PRODDATELEN         8			//生产日期
#define PRODNUMLEN          9			//9位数的序列号
#define MODEL_LEN		  64			//自定义产品型号长度

#define SEC_CHECK_HOST      5
#define SEC_CHECK_SLAVE     6

/* reserved feature numbers */
#define RESERVED_FEA_NUMS	16


#if defined(CONFIG_SWANN) || defined(CONFIG_SWANN_V224)
#define UID_BYTES 20 /*number of uid words */
#endif
#define BASE_UID_BYTES 1000 /*number of base line uid words */



#define RANDCODE_OFFSET 216 /*民用产品随机码在BOOT_PARAMS中的存储位置*/

#define RAND_LEN  6  /*民用产品随机码字节长*/

typedef struct{
/* 0  */	unsigned int	magicNumber;				           	//幻数                                              
/* 4  */	unsigned int	paraChecksum;					        //检查和                                            
/* 8  */	unsigned int	paraLength;						        //结构长度  检查和、长度从'encryptVer'开始计算      
/* 12 */	unsigned int	encryptVer;						        //加密版本:用于控制此结构的更改                     
	/*
		以下4项用户升级控制:必须与升级文件包中的内容一致
	*/

/* 16 */	unsigned int	language;						        //支持语言                                      
/* 20 */	unsigned int	deviceClass;					        //产品类型，1 -- DS9000 DVR, ...                
/* 24 */	unsigned int	oemCode;						        //oem代码：1---代表自己                         
/* 28 */	unsigned char	reservedFeature[RESERVED_FEA_NUMS];	    //保留产品特性，用于升级控制                    
/* 44 */	unsigned short	encodeChans;						    //编码路数                                      
/* 46 */	unsigned short	decodeChans;						    //解码路数                                      
/* 48 */	unsigned short	ipcChans;						        //IPC通道数                                     
/* 50 */	unsigned short	ivsChans;						        //智能通道数                                    
/* 52 */	unsigned char	picFormat;						        //编码最大分辨率 0 -- CIF, 1 -- 2CIF, 2 -- 4CIF 
/* 53 */	unsigned char	macAddr[MAX_ETHERNET][MACADDR_LEN];	    //设备MAC地址                                   
/* 65 */	unsigned char	prodDate[PRODDATELEN];				    //设备生产日期,ASCII                            
/* 73 */	unsigned char	prodNo[PRODNUMLEN];					    //设备序列号                                    
/* 82 */	unsigned char	devHigh;						        //设备高度                                      
/* 83 */	unsigned char	cpuFreq;						        //CPU频率                                       
/* 84 */	unsigned char	dspFreq;						        //DSP频率                                       
/* 85 */	unsigned char	zone;							        //销售地区                                      
/* 86 */	unsigned char	webSupport;						        //支持WEB                                       
/* 87 */	unsigned char	voipSupport;					        //支持VOIP                                      
/* 88 */	unsigned char	usbNums;						        //USB个数                                       
/* 89 */	unsigned char	lcdSupport;						        //支持LCD                                       
/* 90 */	unsigned char   voNums;							        //支持vo个数: 0,1,2                             
/* 91 */	unsigned char	vgaNums;						        //VGA个数: 0，1，2                              
/* 92 */	unsigned char   vtSupport;						        //支持语音对进                                  
/* 93 */	unsigned char   videoMaxtrix;					        //视频矩阵输出：0 -- 无，1 -- 16进4出           
/* 94 */	unsigned char   extendedDecoder;				        //多路解码扩展板                                
/* 95 */	unsigned char   extendedIVS;					        //智能视频分析扩展板                            
/* 96 */	unsigned char   extendedAlarmOut;				        //报警输出扩展板                                
/* 97 */    unsigned char	res1[3];                                //预留                                                
/*100 */	unsigned int	devType;						        //设备类型，4个字节                                                                                              
/*104 */	unsigned int	ubootAdrs;			                    //uboot存放flash地址                            
/*108 */	unsigned int	ubootSize;                              //uboot大小                                     
/*112 */	unsigned int	ubootCheckSum;		                    //uboot校验值                                   
/*116 */    unsigned int	tinyKernelAdrs;		                    //tinyKernel存放flash地址                       
/*120 */    unsigned int	tinyKernelSize;                         //tinyKernel大小                                
/*124 */	unsigned int	tinyKernelCheckSum;	                    //tinyKernel校验值                              
/*128 */    unsigned char	devModel[MODEL_LEN];		            //产品型号:考虑国标型号，扩充到64字节
/*192 */	unsigned char	writeDate[PRODDATELEN];				    //设备信息更新日期,ASCII                        
/*200 */	unsigned int	crypto;								    //是否加密过 1：是，0：否                       
/*204 */    unsigned int    kmem;                       			//内核使用的内存大小
/*212 */    unsigned int    dspmem;                   				//dsp使用的内存大小
/*212 */    unsigned int    product_t;                              //产品类型 成品or半成品
/*216 */	unsigned char	rand_code[RAND_LEN];	                //SWAN系列产品UID
/*236 */	unsigned char 	res3[32];                               //网络数据校验和                          	
/*242 */	unsigned char   lcdProducter;							//lcd屏厂家，三目宝、利尔达、广宁等
/*248*/ 	unsigned char   rfChip;                                 //rf射频芯片类型125K、13.56M等
} boot_params;

#ifdef CONFIG_HIK_SEC
struct sec_verify{
    unsigned int cpld_offset;
    unsigned int boot_offset;
    unsigned int cpld_vol;
    unsigned int probe_vol;
    boot_params chk;
    struct proc_dir_entry *fs;
};
#endif
/*end@Hik-vision define boot parameter encrypted by specific device*/

/*Hik-vision define cpld info in media or specific cpld device*/
#ifndef CPLD_MAGIC
#define  CPLD_MAGIC          0x444c5043  /*CPLD*/
#endif

#ifndef DLPC_MAGIC
#define  DLPC_MAGIC          0x43504c44  /*DLPC*/
#endif


typedef union{
    struct{
        unsigned short    ddr_bitw           :4;  //[0..3]
        unsigned short    ddr_freq           :4;  //[4..7]
        unsigned short    arm_freq           :4; //[8..11]
        unsigned short    arm_chip           :4; //[12..15]
    } bits;
    unsigned short    reg;
} u_sys;

typedef union{
    struct{
        unsigned short    fpga_chip            :4;   //[0..3]
        unsigned short    encrypt_chip       :4;   //[4..7]
        unsigned short    flash_volume       :4;   //[8..11]
        unsigned short    flash_type           :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_flash;

typedef union{
    struct{
        unsigned short day_u             :4;//[3..0]
        unsigned short day_d             :2;//[5..4]
        unsigned short month             :4;//[9..6]
        unsigned short year_u            :4;//[13..10]
        unsigned short year_d            :2;//[15..14]
    }bits;
    unsigned short reg;
} u_firmv;

typedef union{
    struct{
        unsigned short    cpld_date           :4;   //[0..3]
        unsigned short    rtc_inner           :4;   //[4..7]
        unsigned short    device_type         :4;   //[8..11]
        unsigned short    pad_type            :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_peri;

typedef union{
    struct{
        unsigned short    audio_intercom         :4;   //[0..3]
        unsigned short    ad_type                :4;   //[4..7]
        unsigned short    audio_input_num        :8;   //[8..15]
    } bits;
    unsigned short    reg;
} u_auin;

typedef union{
    struct{
        unsigned short    reserved0               :4;    //[0..3]
        unsigned short    aec_type               :4;    //[4..7]
        unsigned short    da_type                   :4;   //[8..11]
        unsigned short    audio_output_num  :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_auout;

typedef union{
    struct{
        unsigned short    reserve0             :4;   //[0..3]
        unsigned short    reserve1             :4;   //[4..7]
        unsigned short    mac1_mode            :4;   //[8..11]
        unsigned short    mac0_mode            :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_mac;

typedef union{
    struct{
        unsigned short    phy1_chip            :8;   //[0..7]
        unsigned short    phy0_chip            :8;   //[8..15]
    } bits;
    unsigned short    reg;
} u_phytype;

typedef union{
    struct{
        unsigned short    phy1_addr            :8;   //[0..7]
        unsigned short    phy0_addr            :8;   //[8..15]
    } bits;
    unsigned short    reg;
} u_phyaddr;

typedef union{
    struct{
        unsigned short    esata_num               :4;   //[0..3]
        unsigned short    sata_pm                 :4;   //[4..7]
        unsigned short    sata_num                :8;   //[8..15]
    } bits;
    unsigned short    reg;
} u_sata;

typedef union{
    struct{
        unsigned short    bl_type               :4;   //[0..3]
        unsigned short    gprs_type             :4;   //[4..7]
        unsigned short    hub_type              :4;   //[8..11]
        unsigned short    usb_num               :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_usb;

typedef union{
    struct{
        unsigned short    alarm_out              :8;   //[0..7]
        unsigned short    alarm_in               :8;   //[8..15]
    } bits;
    unsigned short    reg;
} u_alarm;

typedef union{
    struct{
        unsigned short    vou_type                :2;   //[0..1]
        unsigned short    vou_num                :2;   //[2..3]
        unsigned short    cvbs_num                :4;   //[4..7]
        unsigned short    hdmi_num                :4;   //[8..11]
        unsigned short    vga_num                :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_vou;

typedef union{
    struct{
        unsigned short    reserved0                :4;   //[0..3]
        unsigned short    rs485_full_num      :4;   //[4..7]
        unsigned short    rs485_half_num       :4;   //[8..11]
        unsigned short    rs232_num                :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_serial;

typedef union{
    struct{
        unsigned short    reserve0                :4;   //[0..3]
        unsigned short    reserve1                :4;   //[4..7]
        unsigned short    reserve2                :4;   //[8..11]
        unsigned short    pcie_type               :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_pcie;

typedef union{
    struct{
        unsigned short    da_type           :5;   //[0..4]
        unsigned short    da_num          :3;   //[5..7]
        unsigned short    ad_type           :5;   //[8..12]
        unsigned short    ad_num          :3;   //[13..15]
    } bits;
    unsigned short    reg;
} u_video;

typedef union{
    struct{
        unsigned short    mcu_chip              :8;   //[0..7]
        unsigned short    reserve0              :4;   //[8..11]
        unsigned short    enc_type              :2;   //[12..13]
        unsigned short    ddr_wr                :2;   //[14..15]
    } bits;
    unsigned short    reg;
} u_misc1;

typedef union{
    struct{
        unsigned short    reserve0             :4;   //[0..3]
        unsigned short    mcu_eerom            :4;   //[4..7]
        unsigned short    arm_eerom            :4;   //[8..11]
        unsigned short    mcu_flash            :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_misc2;

typedef union{
    struct{
        unsigned short    sub1g_rf_chip         :5;   //[0..4]
        unsigned short    wifi_probe            :3;   //[5..7]
        unsigned short    wifi_chip             :8;   //[8..15]
    } bits;
    unsigned short    reg;
} u_misc3;

typedef union{
    struct{
        unsigned short    sensor_chip           :4;   //[0..3]
        unsigned short    touch_chip            :4;   //[4..7]
        unsigned short    rf_chip               :4;   //[8..11]
        unsigned short    lcd_chip              :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_misc4;

typedef union{
    struct{
        unsigned short    reserve0                :4;   //[0..3]
        unsigned short    reserve1                :4;   //[4..7]
        unsigned short    reserve2                :4;   //[8..11]
        unsigned short    reserve3                :4;   //[12..15]
    } bits;
    unsigned short    reg;
} u_reserve;

typedef struct{
    unsigned int         magic;                  /*0x00*/
    unsigned short         pcb;                    /*0x04*/
    unsigned char          reserved[8];            /*0x06*/
    unsigned short         board;                  /*0x0E*/
    u_sys       sys;                    /*0x10*/
    u_flash     attr_flash;             /*0x12*/
    u_firmv     firmv;                  /*0x14*/
    u_peri      peri;                   /*0x16*/
    u_auin      audio_in;               /*0x18*/
    u_auout     audio_out;              /*0x1A*/
    u_mac       mac_mode;               /*0x1C*/
    u_phytype   phy_type;               /*0x1E*/
    u_phyaddr   phy_addr;               /*0x20*/
    u_sata      attr_sata;              /*0x22*/
    u_usb       attr_usb;               /*0x24*/
    u_alarm     alarm;                  /*0x26*/
    u_vou       video_out;              /*0x28*/
    u_serial    serial;                 /*0x2A*/
    u_pcie      pcie_flags;             /*0x2C*/
    u_video     video;                  /*0x2E*/
    u_misc1     misc1;                  /*0x30*/
    u_misc2     misc2;                  /*0x32*/
    u_misc3     misc3;                  /*0x34*/
    u_misc4     misc4;                  /*0x36*/
    unsigned short         expand[8];              /*0x38*/
} cpld_info __attribute__ ((aligned (1)));

typedef enum tagPTD_boardType
{
    DB_DS50018_V1_0     = 0x1900,   // 主板型号DS-50018(V1.0)，二代智能安检分析仪主板
    DB_DS50019_V1_0     = 0x2000,   // 主板型号DS-50019(V1.0)，智能安检机分析仪主板
    DB_DS8255_V1_0      = 0x3000,   // 主板型号DS-8255(V1.0)，智能安检机分析仪从片
    DB_DS50024_V1_1     = 0x1201,   // 主板型号DS-50024(V1.1)，安检智能终端主板
    DB_DS50024_4P_V1_1  = 0x2102,   // 主板型号DS-50024(1.V1)-4P，安检智能终端主板，带4G和POE功能
    DB_DS50018_V2_0     = 0x2001,   // 主板型号DS-50018(V2.0)，二代智能安检分析仪主板 
    DB_RS20002_V1_0     = 0x7000,   // 主板型号RS-20002(V1.0)，5030-H7智能终端
    DB_RS20007_V1_0     = 0x6000,   // 主板型号RS-20007(V1.0)，二代智能安检分析仪主板
    DB_RS20007_A_V1_0   = 0x6100,   // 主板型号RS-20007-A(V1.0)，二代智能安检分析仪主板。//从股份转为睿影之后采用的新主板型号，实际的硬件主板就是黄色的DS-50018(V1.1)
    DB_RS20001_V1_0     = 0x8000,   // 主板型号RS-20001(V1.0)，智能分析仪R5
    DB_RS20006_V1_0     = 0x5000,   // 主板型号RS-20006(V1.0)，智能终端主板
    DB_RS20005_V1_0     = 0x4000,   // 主板型号RS-20005(V1.0)，智能终端拓展板
    DB_RS20001_V1_1     = 0x8001,   // 主板型号RS-20001(V1.1)，智能分析仪R5
    DB_RS20011_V1_0     = 0x8011,   // 主板型号RS-20011(V1.0)，二代智能安检分析仪R6主板,带4G 
    DB_RS20016_V1_0     = 0x0002,     // 主板型号RS-20016(V1.0)，二代智能安检分析仪R7主板
    DB_TS3637_V1_0      = 0x0003,     // 主板型号TS-3637(V1.0)，二代智能安检分析仪SD3403主板
    DB_RS20016_V1_1     = 0x0005,   // 主板型号RS-20016(V1.1)，二代智能安检分析仪R7主板
    DB_RS20016_V1_1_F303= 0x0006,   // 主板型号RS-20016(V1.1)_F303，二代智能安检分析仪R7主板,(mcu换为f303)
    DB_RS20025_V1_0     = 0x0008,  //5030D
    DB_RS20033_V1_0     = 0x000a,  //5030s降本
    DB_RS20032_V1_0     = 0X0011,   //主板型号RS-20032(V1.0),安检联网盒子主板
    DB_TS3719_V1_0      = 0x000E,   // 主板型号TS-3719(V1.0)，分析仪RK3588主板
    DB_RS20046_V1_0     = 0x000F,   // 主板型号RS20046(V1.0)，分析仪RK3588主板
    DB_INVALID          = 0xFFFFFFFF,
}PTD_boardType;


typedef enum
{
	DVR_TYPE = 0,
    DVS_TYPE,
    NVR_TYPE
} DEVICE_CLASS;
/*end@Hik-vision define cpld info in media or specific cpld device*/

#define ENV_SIZE            0x1000                 /*include ENV_HEADER_SIZE */

/*Hik-vision for storing uboot environment */
typedef struct {
    unsigned int crc;
    unsigned char data[ENV_SIZE];
} uboot_env;
/*end@Hik-vision for storing uboot environment */

/*Hik-vision define function for cpld & bootpara validity*/
static inline int cpld_vaild(cpld_info *info)
{
	return (info->magic == CPLD_MAGIC || info->magic == DLPC_MAGIC);
}

static inline int bootpara_vaild(boot_params *para)
{
	return para->magicNumber == CFG_MAGIC;
}
/*end@Hik-vision define assert for cpld & bootpara validity*/


#endif
