/**
 * @File: pci_shm.h
 * @Module: pci-shm
 * @Author:
 * @Created:
 *
 * @Description: PCIe 共享内存
 *
 * @Note: TODO
 *
 * @Usage:TODO
 */

/**
 * HI3559AV100 BAR 空间大小 :
 *  BAR0 - 8MB;     BAR1 - 64KB;    BAR2 - 1MB;
 *  BAR3 - 1MB;     BAR4 - 64KB;    BAR5 - 4KB;
 *
 * MLU220 BAR 空间大小 :
 *  BAR0 - 64MB;    BAR2 - 64MB;    BAR4 - 64MB;
 *
 * FPGA   BAR空间大小：
 * BAR0 - 2MB；BAR1 - 64KB
 */
#ifndef __PCI_SHM_H_
#define __PCI_SHM_H_

#ifdef  __cplusplus
    #if     __cplusplus
        extern "C"{
    #endif
#endif /* __cplusplus */

#define SHM_NODE_NAME   "/dev/pci-shm"
#define PAGE_SHIFT     (12)

/*从片非FPGA平台时定义*/
typedef enum
{
    PCI_SHM_APP_0 = 0,
    PCI_SHM_APP_1 = 1,
    PCI_SHM_DSP_0 = 2,
    PCI_SHM_DSP_1 = 3,
}shm_type_t;


/*从片为FPGA时，该平台比较特殊，对外只有一个bar0*/
typedef enum
{
	PCI_SHM_DSP_FPGA = 0,
}shm_type_t_fpga;


enum
{
    PCI_NORMAL_SHM_SIZE = 0x40000, /*标准共享内存大小*/
	PCI_FPGA_SHM_SIZE = 0x200000,  /*FPGA模块共享内存大小*/
};



shm_type_t dev_type[] = {PCI_SHM_APP_0, PCI_SHM_APP_1, PCI_SHM_DSP_0, PCI_SHM_DSP_1};

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif /* __cplusplus */

#endif // __PCI_SHM_H_
