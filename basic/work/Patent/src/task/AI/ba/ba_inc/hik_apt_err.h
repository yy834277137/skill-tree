#ifndef __HIK_APT_ERR_H__
#define __HIK_APT_ERR_H__

#define HIK_APT_TRUE    1
#define HIK_APT_FALSE   0

#define HIK_APT_S_OK    1
#define HIK_APT_S_FAIL  0

// ONLY 0x8DBA0000 to 0x8DBAFFFF

/******* invalid param (common part) *******/
#define HIK_APT_ERR_INVALID_PARAM               0x8DBA1001
#define HIK_APT_ERR_NULL_PTR                    0x8DBA1002
#define HIK_APT_ERR_TOO_LARGE                   0x8DBA1003
#define HIK_APT_ERR_TOO_SMALL                   0x8DBA1004


/******* invalid param (for more specific scenario) *******/
// invalid param for size
#define HIK_APT_ERR_SIZE_TOO_SMALL              0x8DBA1101
#define HIK_APT_ERR_SIZE_TOO_LARGE              0x8DBA1102

// invalid param for string
#define HIK_APT_ERR_STR_INVALID                 0x8DBA1201
#define HIK_APT_ERR_STR_TOO_LONG                0x8DBA1202
#define HIK_APT_ERR_STR_TOO_SHORT               0x8DBA1203
#define HIK_APT_ERR_STR_EMPTY                   0x8DBA1204


// invalid param for addr
#define HIK_APT_ERR_ADDR_BAD                    0x8DBA1301

// invalid param for other things
#define HIK_APT_ERR_INVALID_TYPE                0x8DBA1401
#define HIK_APT_ERR_INVALID_MODE                0x8DBA1402
#define HIK_APT_ERR_INVALID_CHN_ID              0x8DBA1403
#define HIK_APT_ERR_INVALID_DEV_ID              0x8DBA1404
#define HIK_APT_ERR_INVALID_NAME                0x8DBA1405
#define HIK_APT_ERR_INVALID_FILE                0x8DBA1406


/******* key/name error *******/
#define HIK_APT_ERR_NOT_FOUND                   0x8DBA1501
#define HIK_APT_ERR_CONFLICT                    0x8DBA1502

/******* implementation error *******/
#define HIK_APT_ERR_NOT_SUPPORT                 0x8DBA1601
#define HIK_APT_ERR_NOT_IMPLEMENTED_YET         0x8DBA1602

/******* permission error *******/
#define HIK_APT_ERR_NOT_ALLOWED                 0x8DBA1701

/******* status error *******/
#define HIK_APT_ERR_NOT_INITIALIZED             0x8DBA1801
#define HIK_APT_ERR_NOT_CONFIG                  0x8DBA1802
#define HIK_APT_ERR_NOT_READY                   0x8DBA1803
#define HIK_APT_ERR_NOT_RELEASED                0x8DBA1804
#define HIK_APT_ERR_BUSY                        0x8DBA1805
#define HIK_APT_ERR_TIMEOUT                     0x8DBA1806
#define HIK_APT_ERR_BLOCKED                     0x8DBA1807
#define HIK_APT_ERR_EMPTY                       0x8DBA1808
#define HIK_APT_ERR_FULL                        0x8DBA1809
#define HIK_APT_ERR_SHOULD_NOT_HAPPEN           0x8DBA180A
#define HIK_APT_ERR_SHOULD_NOT_BE_HERE          0x8DBA180B
#define HIK_APT_ERR_NOT_SET                     0x8DBA180C

/******* compare related error *******/
#define HIK_APT_ERR_NOT_IDENTICAL               0x8DBA1901
#define HIK_APT_ERR_CHECKSUM_NOT_IDENTICAL      0x8DBA1902
#define HIK_APT_ERR_MAGICNUM_NOT_IDENTICAL      0x8DBA1903
#define HIK_APT_ERR_VERSION_CHECK_FAILED        0x8DBA1904


/******* HKANN related error *******/
#define HIK_APT_ERR_HKNN_FAILED                 0x8DBA1A01
#define HIK_APT_ERR_HKNN_CONFIG_FAILED          0x8DBA1A02
#define HIK_APT_ERR_HKNN_PROCESS_FAILED         0x8DBA1A03

/******* platform related error *******/
#define HIK_APT_ERR_PLAT_NOT_CORRECT            0x8DBA1B01

/******* memory related error *******/
#define HIK_APT_ERR_MEM_INVALID_ATTR            0x8DBA1C01
#define HIK_APT_ERR_MEM_INVALID_PLAT            0x8DBA1C02
#define HIK_APT_ERR_MEM_INVALID_ALIGN           0x8DBA1C03
#define HIK_APT_ERR_MEM_LACK_OF_MEM             0x8DBA1C04
#define HIK_APT_ERR_MEM_INVALID_SPACE           0x8DBA1C05
#define HIK_APT_ERR_MEM_LEAK                    0x8DBA1C06
#define HIK_APT_ERR_MEM_UNINIT                  0x8DBA1C07
#define HIK_APT_ERR_MEM_ILLEGAL_OVERWRITTEN     0x8DBA1C08

/******* json related error *******/
#define HIK_APT_ERR_JSON_NULL_PTR               0x8DBA1D01
#define HIK_APT_ERR_JSON_FILE_LEN               0x8DBA1D02
#define HIK_APT_ERR_JSON_PARAM_ERR              0x8DBA1D03

/******* mutex related error *******/
#define HIK_APT_ERR_LOCK_FAIL                   0x8DBA1E01


/******* deprecated (do not use anymore)*******/
#define HIK_APT_MEM_INVALID_ATTR                HIK_APT_ERR_MEM_INVALID_ATTR 
#define HIK_APT_MEM_INVALID_PLAT                HIK_APT_ERR_MEM_INVALID_PLAT 
#define HIK_APT_MEM_INVALID_ALIGN               HIK_APT_ERR_MEM_INVALID_ALIGN
#define HIK_APT_MEM_LACK_OF_MEM                 HIK_APT_ERR_MEM_LACK_OF_MEM  
#define HIK_APT_MEM_INVALID_SPACE               HIK_APT_ERR_MEM_INVALID_SPACE

#define HIK_APT_JSON_NULL_PTR                   HIK_APT_ERR_JSON_NULL_PTR
#define HIK_APT_JSON_FILE_LEN                   HIK_APT_ERR_JSON_FILE_LEN
#define HIK_APT_JSON_PARAM_ERR                  HIK_APT_ERR_JSON_PARAM_ERR
#endif