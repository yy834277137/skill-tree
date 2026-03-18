/**
 * @file   encrypt_proc.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#ifdef ENABLE_HW_ENCRYPT 
#include <string.h>
#include <platform_hal.h>

#include "sal.h"
#include "encrypt_proc.h"

/* 研究院黄崇基提供,每个平台固定 */
/* static unsigned char aes_enc_key_buf[16] = {0xc0,0xc6,0x3e,0x06,0x18,0xc6,0xff,0x4e,0x7f,0x66,0x1a,0x80,0x19,0x4a,0xf8,0x6b}; */

static unsigned char aes_enc_key_buf[16] = {0x15, 0xef, 0x74, 0x54, 0x24, 0x9c, 0x1d, 0x65, 0xf4, 0x74, 0x02, 0x51, 0x06, 0xb7, 0x54, 0x5a};

/* 解密模块测试: 输入密文, 32字节 */
unsigned char aes_test_input_data[32] = {0x8D, 0xC9, 0xAD, 0xB2, 0x96, 0x33, 0x2B, 0xEE, 0x1C, 0x82, 0x3C, 0x6D, 0x04, 0xA4, 0x5E, 0xA5,
                                         0x88, 0x75, 0x92, 0xDA, 0x84, 0x18, 0xC0, 0x47, 0xE1, 0xE5, 0x45, 0xCF, 0x00, 0x00, 0x00, 0x00};

/* 解密模块测试: 目标输出明文, 32字节 */
unsigned char aes_test_check_data[32] = {0x99, 0x75, 0x87, 0x9E, 0x39, 0xE1, 0x2A, 0xBB, 0x43, 0x87, 0x2C, 0xCC, 0xAF, 0xD8, 0xF6, 0xA7,
                                         0xBE, 0x75, 0x72, 0x8E, 0x25, 0x5A, 0x5B, 0xFC, 0x44, 0x29, 0xEA, 0x97, 0x8C, 0x22, 0x5F, 0x1D};

#if 0

/***********************************************************************************************************************
* 功  能: 对齐分配mmz内存空间
* 参  数:
*         phy_addr             -I            地址指针
*         size                 -I            申请大小
*         align                -I            对齐值
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
HI_VOID *alloc_memory_align(void **phy_addr, int size, int align)
{
    HI_U64 phy_base;
    HI_VOID *vir_base;
    char *vir_use;
    int align_size;
    int align_off;
    int ret;

    if (align % 4)
    {
        return 0;
    }

    align_size = size + align + 2 * sizeof(char *);
    align_off = 0;

    ret = HI_MPI_SYS_MmzAlloc(&phy_base, &vir_base, "User", HI_NULL, align_size);
    if (ret)
    {
        printf("err: HI_MPI_SYS_MmzAlloc_Cached \n");
        SAL_ERROR("err: HI_MPI_SYS_MmzAlloc_Cached ret is 0x%x\n", ret);
        return 0;
    }

    vir_use = (char *)vir_base + 2 * sizeof(char *);
    while ((size_t)vir_use % align)
    {
        vir_use++;
        align_off++;
    }

    *phy_addr = (void *)(unsigned long)(phy_base + 2 * sizeof(char *) + align_off);

    /* 记录需要释放的虚拟地址 */
    ((void **)vir_use)[-1] = (void *)vir_base;

    /* 记录需要释放的物理地址 */
    ((void **)vir_use)[-2] = (void *)(unsigned long)phy_base;


    return vir_use;
}

/***********************************************************************************************************************
* 功  能: 对齐释放mmz内存空间
* 参  数:
*         phy_addr             -I            物理地址
*         vir_addr             -I            虚拟地址
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
int free_memory_align(HI_U64 phy_addr, HI_VOID *vir_addr)
{
    HI_U64 phy_base;
    HI_VOID *vir_base;
    char *vir_use;
    int ret = 0;

    if (phy_addr != 0 && vir_addr != 0)
    {
        vir_use = (char *)vir_addr;

        phy_base = (HI_U64)(unsigned long)(*(char **)(vir_use - 2 * sizeof(void *)));
        vir_base = *(char **)(vir_use - 1 * sizeof(void *));
        ret = HI_MPI_SYS_MmzFree(phy_base, vir_base);
    }

    return ret;
}

#endif

/***********************************************************************************************************************
* 功  能: printBuffer
* 参  数:
*         string               -I            物理地址
*         pu8Input             -I            虚拟地址
*         u32Length            -I            字符长度
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
static INT32 printBuffer(CHAR *string, UINT8 *pu8Input, UINT32 u32Length)
{
    UINT32 i = 0;

    if (NULL != string)
    {
        DSP_OTP_LOG("%s\n", string);
    }

    for (i = 0; i < u32Length; i++)
    {
        if ((i % 16 == 0) && (i != 0))
            DSP_OTP_LOG("\n");

        DSP_OTP_LOG("0x%02x ", pu8Input[i]);
    }

    DSP_OTP_LOG("\n");

    return SAL_SOK;
}

#if 0

/***********************************************************************************************************************
* 功  能: Setconfiginfo
* 参  数:

* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
HI_S32 Setconfiginfo(HI_HANDLE chnHandle, HI_BOOL bKeyByCA, HI_UNF_CIPHER_ALG_E alg, HI_UNF_CIPHER_WORK_MODE_E mode, HI_UNF_CIPHER_KEY_LENGTH_E keyLen,
                     const UINT8 u8KeyBuf[16], const UINT8 u8IVBuf[16])
{
    INT32 s32Ret = SAL_SOK;
    HI_UNF_CIPHER_CTRL_S CipherCtrl;

    memset(&CipherCtrl, 0, sizeof(HI_UNF_CIPHER_CTRL_S));
    CipherCtrl.enAlg = alg;
    CipherCtrl.enWorkMode = mode;
    CipherCtrl.enBitWidth = HI_UNF_CIPHER_BIT_WIDTH_128BIT;
    CipherCtrl.enKeyLen = keyLen;
    CipherCtrl.bKeyByCA = bKeyByCA;
    CipherCtrl.enCaType = HI_UNF_CIPHER_KEY_SRC_KLAD_1;
    if (CipherCtrl.enWorkMode != HI_UNF_CIPHER_WORK_MODE_ECB)
    {
        /* must set for CBC , CFB mode */
        CipherCtrl.stChangeFlags.bit1IV = 1;
        memcpy(CipherCtrl.u32IV, u8IVBuf, 16);
    }

    memcpy(CipherCtrl.u32Key, u8KeyBuf, 16);

    s32Ret = HI_UNF_CIPHER_ConfigHandle(chnHandle, &CipherCtrl);
    if (HI_SUCCESS != s32Ret)
    {
        DSP_OTP_ERR(" HI_UNF_CIPHER_ConfigHandle!return %d \n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

#endif

/***********************************************************************************************************************
* 功  能: CBC_AES128
* 参  数:

* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
int CBC_AES128(DSP_OTP_CTRL *pCtrl)
{
    INT32 s32Ret = SAL_SOK;

    UINT8 u8Key[32];
    /* int i = 0; */
    /* unsigned char *input_key = pCtrl->ciper_key_vir_addr; */
    /* unsigned char *output_key= pCtrl->plain_key_vir_addr; */
    unsigned int length = ALG_KEY_LEN;

    /* For decrypt */
    memcpy(u8Key, aes_enc_key_buf, sizeof(aes_enc_key_buf));

#if 0
    s32Ret = Setconfiginfo(pCtrl->hi_unf_cipher_handle,
                           HI_TRUE,
                           HI_UNF_CIPHER_ALG_AES,
                           HI_UNF_CIPHER_WORK_MODE_ECB,
                           HI_UNF_CIPHER_KEY_AES_128BIT,
                           u8Key,
                           NULL);
#else
    s32Ret = cipher_hal_setCipherCfg(pCtrl->hi_unf_cipher_handle, u8Key, NULL);
#endif
    if (SAL_SOK != s32Ret)
    {
        DSP_OTP_ERR("Set OTP AES config info failed.\n");
        return OTP_FAILED;
    }

/*******************************************************************************************
   HI_UNF_CIPHER_Decrypt:该接口可能需要使用MMZ物理内存
   HI_UNF_CIPHER_DecryptVir:该接口SDK可能没有,是BSP提供
*******************************************************************************************/
    s32Ret = cipher_hal_decrypt(pCtrl->hi_unf_cipher_handle, pCtrl->ciper_key_phy_addr, pCtrl->plain_key_phy_addr, length);
    if (SAL_SOK != s32Ret)
    {
        DSP_OTP_ERR("Cipher decrypt failed.\n");
        return OTP_E_DICIPHER_ERR;
    }
    else
    {
        printBuffer("#######AES decode:", pCtrl->plain_key_vir_addr, length);
    }

    return OTP_OK;
}

/**************************************************************************
* 功  能：硬件解密处理
* 参  数:
*         handle             -I        句柄
* 返回值：状态码
**************************************************************************/
int encrypt_proc(void *handle)
{
    INT32 rt = -1;
    DSP_OTP_CTRL *pCtrl = (DSP_OTP_CTRL *)handle;

    DSP_OTP_ASSERT_RET(pCtrl == NULL, OTP_FAILED);
    DSP_OTP_ASSERT_RET(pCtrl->ciper_key_vir_addr == NULL, OTP_FAILED);
    DSP_OTP_ASSERT_RET(pCtrl->plain_key_vir_addr == NULL, OTP_FAILED);
    DSP_OTP_ASSERT_RET(pCtrl->magic != DSP_OTP_CTRL_MAGIC, OTP_FAILED);

    rt = CRY_GetCipher(pCtrl->ciper_key_vir_addr, 0);

#if 0  /* 测试解密模块是否功能正常: 密文地址内填充测试数据 */
    memcpy(pCtrl->ciper_key_vir_addr, aes_test_input_data, sizeof(aes_test_input_data));
#endif

    if (CRY_OK != rt)    /* 注意:返回1表示成功 */
    {
        DSP_OTP_ERR("CRY_GetCipher error!rt = %d\n", rt);
        return OTP_E_GET_KEY_ERR;
    }

    DSP_OTP_ERR(" ======================================== \n");

    rt = CBC_AES128(pCtrl);
    if (OTP_OK != rt)
    {
        DSP_OTP_ERR(" CBC_AES128 error!rt = %d\n", rt);
        return OTP_E_DICIPHER_ERR;
    }

    DSP_OTP_ERR(" ======================================== \n");

#if 0  /* 测试解密模块是否功能正常: 打印测试字节、实际明文字节和目标字节 */
    int i = 0;
    for (i = 0; i < 32; i++)
    {
        printf("i %d before %.2x after %.2x key %.2x\n",
               i, (int)aes_test_input_data[i], *(char *)(pCtrl->plain_key_vir_addr + i * sizeof(unsigned char)), (int)aes_test_check_data[i]);
    }

#endif

    rt = CRY_SetDataInfo(pCtrl->plain_key_vir_addr, ALG_KEY_LEN);
    if (CRY_OK != rt)
    {
        DSP_OTP_ERR(" CRY_GetCipher error!rt = %d\n", rt);
        return OTP_E_SET_KEY_ERR;
    }

    DSP_OTP_ERR(" ======================================== \n");
    return OTP_OK;
}

/**
 * @function    get_phy_from_vir
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static int get_phy_from_vir(VOID *pVirAddr, UINT64 *pu64PhyAddr)
{
    INT32 s32Ret = SAL_SOK;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    SYSTEM_FRAME_INFO stSysFrameInfo = {0};

    if (NULL == pVirAddr || NULL == pu64PhyAddr)
    {
        SAL_LOGE("invalid input ptr! %p, %p \n", pVirAddr, pu64PhyAddr);
        return -1;
    }

    s32Ret = sys_hal_allocVideoFrameInfoSt(&stSysFrameInfo);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("alloc video frame info st failed! \n");
        return SAL_FAIL;
    }

    stVideoFrmBuf.virAddr[0] = (PhysAddr)pVirAddr;
    stVideoFrmBuf.virAddr[1] = (PhysAddr)pVirAddr;
    stVideoFrmBuf.virAddr[2] = (PhysAddr)pVirAddr;
    s32Ret = sys_hal_buildVideoFrame(&stVideoFrmBuf, &stSysFrameInfo);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("build video frame info st failed! \n");
        return SAL_FAIL;
    }

    s32Ret = sys_hal_getVideoFrameInfo(&stSysFrameInfo, &stVideoFrmBuf);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("get video frame info st failed! \n");
        return SAL_FAIL;
    }

    (VOID)sys_hal_rleaseVideoFrameInfoSt(&stSysFrameInfo);

    *pu64PhyAddr = (UINT64)stVideoFrmBuf.phyAddr[0];
    return 0;
}

/**************************************************************************
* 功  能：硬件解密初始化
* 参  数:
*         handle             -I        句柄
*
* 返回值：状态码
**************************************************************************/
int encrypt_init(void **handle)
{
    INT32 s32Ret = SAL_SOK;
    CIPHER_ATTR_S stCipherAttr = {0};
    DSP_OTP_CTRL *pCtrl = NULL;
    UINT64 phy_base = 0;

    /* 申请句柄所需内存 */
    s32Ret = mem_hal_mmzAlloc(sizeof(DSP_OTP_CTRL), "encrypt", "handle", NULL, SAL_FALSE, &phy_base, (VOID **)&pCtrl);
    if (0 != s32Ret)
    {
        SAL_LOGE("hal alloc mmz failed! \n");
        return OTP_E_HISI_FUNC_ERR;
    }

    /* 申请密文内存 */
    s32Ret = mem_hal_mmzAlloc(ALG_KEY_LEN, "encrypt", "cipher_key", NULL, SAL_FALSE, &phy_base, (VOID **)&pCtrl->ciper_key_vir_addr);
    if (0 != s32Ret)
    {
        SAL_LOGE("hal alloc mmz failed! \n");
        return OTP_E_HISI_FUNC_ERR;
    }

    if (0 != get_phy_from_vir((VOID *)pCtrl->ciper_key_vir_addr, &phy_base))
    {
        SAL_LOGE("get phy from vir failed! \n");
        return SAL_FAIL;
    }

    pCtrl->ciper_key_phy_addr = phy_base;

    /* 申请明文内存 */
    s32Ret = mem_hal_mmzAlloc(ALG_KEY_LEN, "encrypt", "plain_key", NULL, SAL_FALSE, &phy_base, (VOID **)&pCtrl->plain_key_vir_addr);
    if (0 != s32Ret)
    {
        SAL_LOGE("hal alloc mmz failed! \n");
        return OTP_E_HISI_FUNC_ERR;
    }

    if (0 != get_phy_from_vir((VOID *)pCtrl->plain_key_vir_addr, &phy_base))
    {
        SAL_LOGE("get phy from vir failed! \n");
        return SAL_FAIL;
    }

    pCtrl->plain_key_phy_addr = phy_base;

    pCtrl->magic = DSP_OTP_CTRL_MAGIC;

    /* 初始化底层解密接口 */
    s32Ret = cipher_hal_init();
    if (SAL_SOK != s32Ret)
    {
        DSP_OTP_ERR(" HI_UNF_CIPHER_Init error! s32Ret:%d \n", s32Ret);
        return OTP_E_HISI_FUNC_ERR;
    }

    stCipherAttr.enCipherType = CIPHER_TYPE_NORMAL;
    s32Ret = cipher_hal_createHandle(&pCtrl->hi_unf_cipher_handle, &stCipherAttr);
    if (SAL_SOK != s32Ret)
    {
        DSP_OTP_ERR(" HI_UNF_CIPHER_CreateHandle error!s32Ret:%d \n", s32Ret);
        (void)cipher_hal_deinit();
        return OTP_E_HISI_FUNC_ERR;
    }

    *handle = pCtrl;

    return 0;
}

/**************************************************************************
* 功  能：硬件解密去初始化
* 参  数:
*         handle             -I        句柄
*
* 返回值：状态码
**************************************************************************/
int encrypt_deinit(void *handle)
{
    DSP_OTP_CTRL *pCtrl = (DSP_OTP_CTRL *)handle;

    /* void         *temp = NULL; */

    cipher_hal_destroyHandle(pCtrl->hi_unf_cipher_handle);
    cipher_hal_deinit();

    mem_hal_mmzFree((Ptr)pCtrl->ciper_key_vir_addr, "encrypt", "cipher_key");
    mem_hal_mmzFree((Ptr)pCtrl->plain_key_vir_addr, "encrypt", "plain_key");
    mem_hal_mmzFree((Ptr)pCtrl, "encrypt", "handle");

    return OTP_OK;
}
#endif
