
#ifndef _DSPTTK_XRAW_HEADER_H_
#define _DSPTTK_XRAW_HEADER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define PAL_RAW_EXPORT_MAGIC                    (0x40574152)    //导出信息头校验字，可用来判断数据是否有头
#define PAL_RAW_EXPORT_MAX_SVAALARM_NUM         (32)            //导出智能信息最大危险品个数
#define PAL_RAW_EXPORT_MAX_XSPUNPENALARM_NUM    (16)            //导出智能信息最大难穿透个数
#define PAL_RAW_EXPORT_MAX_XSPSUSALARM_NUM      (32)            //导出智能信息最大可疑有机物个数

/* X-Ray源能量类型数 */
typedef enum
{
    PAL_XRAY_ENERGY_UNDEF,          /* 未定义 */
    PAL_XRAY_ENERGY_SINGLE_LOW,     /* 单低能 */
    PAL_XRAY_ENERGY_SINGLE_High,    /* 单高能 */
    PAL_XRAY_ENERGY_DUAL,           /* 双能 */
} PAL_XRAY_ENERGY_NUM;

/* XRAY-RAW数据类型 */
typedef enum
{
    PAL_XRAW_DT_UNDEF = 0,           /* 未定义的数据类型，预留 */
    PAL_XRAW_DT_BOOT_CORR = 1,       /* 开机校正数据 */
    PAL_XRAW_DT_MANUAL_CORR	= 2,     /* 用户手动触发校正数据 */
    PAL_XRAW_DT_AUTO_CORR = 3,       /* 过包前自动校正数据 */
    PAL_XRAW_DT_AUTO_UPDATE	= 4,     /* 成像算法库自动更新模板 */
    PAL_XRAW_DT_PACKAGE_SCANED = 5,  /* 过包源扫描数据，未经任何处理 */
} PAL_XRAW_DT; /* XRAW DATA TYPE，数据类型 */

/* 设备基本信息，64Byte */
typedef struct
{
    unsigned int uiDevModel;  /* 设备型号 */
    char cDevSerial[48];          /* 设备序列号 */
    unsigned int uiWorkTime;  /* 设备开机后运行时长 */
    float fSpeed;         /* 传送带速度 */
    unsigned char iDetectCapSysName; /* 采集板型号 */
    char res[3];
} PAL_DEVICE_BASIC;


/****************************** XRAW文件头结构体，V0版本 ******************************/

/* 版本信息，192Byte */
typedef struct
{
    char cSoftwareVersion[32]; /* 软件版本 */
    char cMasterCtlVersion[32]; /* 主控板版 */
    char cKeyBoardVersion[32]; /* 键盘版本 */
    char cXspVersion[32]; /* 成像算法版本 */
    char res[64];
} PAL_VERSION_INFO_V0;

/* 采传信息，192Byte */
typedef struct
{
    PAL_XRAY_ENERGY_NUM enXrayEnergyNum; /* 单低能、单高能、双能，offset：512(0x200)Byte */
    unsigned int integrationTime; /* 积分时间 */
    unsigned short u16GainSet[2][40]; /* 增益，[高低能][采集板个数] */
    unsigned int iTubevoltage; /* 管电压 */
    unsigned int iTubecurrent; /* 管电流 */
    unsigned short u16LinePixelNum; /* 采集每行像素数 */
    unsigned short u16FullAutoUpRatio; /* 满载校正自动更新比例，范围[0, 100] */
    unsigned int DeviceRunTime; /*射线源累计工作时间*/
    unsigned int XRAYrunTime; /*射线源累计出束时间*/
    char res[4];
} PAL_XSENSOR_INFO_V0;

/* 设备信息，512Byte */
typedef struct
{
    PAL_DEVICE_BASIC stDevBasic;    /* 64Byte */
    PAL_VERSION_INFO_V0 stVerInfo;     /* 192Byte */
    PAL_XSENSOR_INFO_V0 stXsensorInfo; /* 192Byte，单个射线源的采传信息 */
    char res[64];
} PAL_DEVICE_INFO_V0;

/* 数据信息 64Byte */
typedef struct
{
    PAL_XRAW_DT enDataType;         /* offset：1024(0x400)Byte */
    char res0[4];
    unsigned long long stStartTime;
    char res1[48];
} PAL_XRAW_DATA_V0;

/* 头信息 2048 Byte */
typedef struct
{
    unsigned int version; /* 结构体本身的版本号 */
    unsigned int res1[63];
    /* ------------- 256Byte ------------- */

    PAL_DEVICE_INFO_V0 stDevInfo; /* 设备信息 */
    unsigned int res2[64];
    /* ------------- 1024Byte ------------- */

    PAL_XRAW_DATA_V0 stXrawData; /* 数据信息 */
    unsigned int res3[240];
} PAL_XRAW_HEADER_V0;



/****************************** XRAW文件头结构体，V1版本 ******************************/

// 版本信息 256Byte
typedef struct
{
    char cSoftwareVersion[32];//软件版本
    char cMasterCtlVersion[32];//主控板版
    char cKeyBoardVersion[32];//键盘版本
    char cXspVersion[32];//成像算法版本
    char res[128];
}PAL_VERSION_INFO_V1;

// 采传信息 256Byte
typedef struct
{
    PAL_XRAY_ENERGY_NUM enXrayEnergyNum; // 单低能、单高能、双能
    unsigned int integrationTime; //积分时间
    unsigned short u16GainSet[2][40]; //增益，[高低能][采集板个数]
    unsigned int iTubevoltage; //管电压
    unsigned int iTubecurrent; //管电流
    unsigned short u16LinePixelNum; //采集每行像素数
    unsigned short u16FullAutoUpRatio; // 满载校正自动更新比例，范围[0, 100]
    unsigned int DeviceRunTime;/*射线源累计工作时间*/
    unsigned int XRAYrunTime;/*射线源累计出束时间*/
    char res[68];
}PAL_XSENSOR_INFO_V1;

// 设备信息 640Byte
typedef struct
{
    PAL_DEVICE_BASIC stDevBasic;
    PAL_VERSION_INFO_V1 stVerInfo;
    PAL_XSENSOR_INFO_V1 stXsensorInfo;//单个射线源的采传信息
    char res[64];
}PAL_DEVICE_INFO_V1;

// XRAY-RAW危险品类型及坐标信息
typedef struct
{
    unsigned int type;                           /*危险品类型*/
    float x;                                     
    float y;                                     
    float w;                                     
    float h;                                     
}PAL_RAW_ALARM_INFO;

// RAW数据信息及危险品信息
typedef struct
{
    unsigned int uiWidth;                           /*Raw数据宽*/
    unsigned int uiHeight;                          /*Raw数据高*/
    unsigned int uiDataLen;                         /*Raw数据长度*/
    unsigned int RawZdataVer;                       /*Raw数据原子序数版本*/
    
    unsigned int       uiSvaAlarmNum;               /*智能识别危险品数量*/
    PAL_RAW_ALARM_INFO stSvaAlarm[PAL_RAW_EXPORT_MAX_SVAALARM_NUM];                /*智能识别危险品信息*/
    unsigned int       uiXspUnpenAlarmNum;          /*XSP识别难穿透数量*/
    PAL_RAW_ALARM_INFO stXspUnpenAlarm[PAL_RAW_EXPORT_MAX_XSPUNPENALARM_NUM];           /*XSP识别难穿透信息*/
    unsigned int       uiXspSusAlarmNum;            /*XSP识别可疑物数量*/
    PAL_RAW_ALARM_INFO stXspSusAlarm[PAL_RAW_EXPORT_MAX_XSPSUSALARM_NUM];             /*XSP识别可疑物数量*/
}PAL_RAW_DATA_INFO;

// 数据信息 3352Byte
typedef struct
{
    PAL_XRAW_DT enDataType;
    char res0[4];
    unsigned long long stStartTime;                    /*图片时间*/
    unsigned int uiParcelId;                           /*包裹ID*/
    unsigned int iDirection;                           /*出图方向*/
    int          iOperatorId;                          /*操作员序号*/
    char         iOperatorName[64];                    /*操作员ID名*/
    unsigned int uiRawCount;                           /*包含Raw数据个数*/
    PAL_RAW_DATA_INFO  stRawInfo[2];                   /*包裹Raw数据信息*/
}PAL_XRAW_DATA_V1;

//头信息 5120 Byte
typedef struct
{
    unsigned int magic;     // 导出信息头校验字，可用来判断数据是否有头
    unsigned int version;   // 结构体本身的版本号
    unsigned int ustSize;   // 结构体头大小
    unsigned int uFileSize; // 文件大小
    unsigned int tm_year;   // 文件时间
    unsigned int tm_mon;    // 文件时间
    unsigned int tm_mday;   // 文件时间
    unsigned int tm_hour;   // 文件时间
    unsigned int tm_min;    // 文件时间
    unsigned int tm_sec;    // 文件时间
    unsigned int res1[23]; 
    PAL_DEVICE_INFO_V1 stDevInfo;//设备信息
    unsigned int res2[33];  
    PAL_XRAW_DATA_V1 stXrawData; //数据信息
    unsigned int res3[216];
}PAL_XRAW_HEADER_V1;

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_XRAW_HEADER_H_ */

