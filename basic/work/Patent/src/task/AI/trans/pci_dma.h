/**
 * @File:
 * @Module:
 * @Author:
 * @Created:
 *
 * @Description:
 *
 * @Usage:TODO
 */

#ifndef __PCI_DMA_H_
#define __PCI_DMA_H_


/*********************************************************************
 ******************************* 头文件  *******************************
 *********************************************************************/




/*********************************************************************
 ******************************* 宏枚举定义  ***************************
 *********************************************************************/
#ifndef pci_dma_addr_t
	typedef uint64_t pci_dma_addr_t;
#endif

#define PCI_DMA_DEV        "/dev/pci-dma"

#define PCI_DMA_BASE      'D'

/*连续物理地址or用户态虚拟地址dma接口*/
#define PCI_DMA_START       		_IOWR(PCI_DMA_BASE, 60, pci_dma_arg_t *)

/*不连续物理地址dma接口:目前仅dsp用*/
#define PCI_SCATTER_DMA_START       _IOWR(PCI_DMA_BASE, 61, pci_dma_multi_args_t *)

/*获取当前上线设备信息*/
#define PCI_GET_ALL_DEVICE       	_IOWR(PCI_DMA_BASE, 62, pci_device_info *)


#define MAX_DEVICE_NUM  				(32)


struct hi_device_info
{
	unsigned int id;        /*设备所在总线号*/
	unsigned int dev_type;  /*设备的vendor+device_id*/
};

/*pci总线下挂载设备信息获取*/
typedef struct pci_device_info
{
	struct hi_device_info device_info[MAX_DEVICE_NUM];
	int num;        		/*设备个数*/
	int resvered[3];
}pci_device_info;


typedef enum
{
    PCI_ADDR_USER,  /*用户态虚拟地址*/
    PCI_ADDR_PHYS,  /*物理地址*/
    PCI_ADDR_MAX,
}pci_addr_type_t;


typedef enum
{
    PCI_DMA_READ = 0,	/*DMA读操作*/
    PCI_DMA_WRITE,		/*DMA写操作*/
}pci_dir_type_t;

/*********************************************************************
 ******************************* 结构体  *****************************
 *********************************************************************/
/*DMA操作三要素*/
typedef struct pci_dma_param
{
	pci_dma_addr_t src;			/*源数据地址*/
	pci_dma_addr_t dst;			/*目的地址*/
	size_t		  length;		/*数据长度*/
	uint32_t	   reserv[5];
}dma_param_t;


/*连续物理地址或用户态虚拟地址dma操作所用数据结构*/
typedef struct pci_dma_arg
{
	pci_dma_addr_t	src;		/*源数据地址*/
	pci_dma_addr_t	dst;		/*目的地址*/
	size_t			length; 	/*数据长度*/
	pci_addr_type_t type;		/*数据类型*/
	pci_dir_type_t	dir;		/*读写方向*/
	uint32_t		id;			/*总线号*/
	uint32_t		reserved[32];
}pci_dma_arg_t;

/*不连续物理地址dma操作所用数据结构*/
typedef struct pci_dma_multi_args
{
	dma_param_t   dma_data[8];   /*dma数据集*/
	uint32_t	   count;		/*数据块个数*/
	pci_addr_type_t type;		/*数据类型*/
	pci_dir_type_t	dir;	   /*读写方向*/
	uint32_t	   id;         /*总线号*/
	uint32_t	   reserved[20];
}pci_dma_multi_args_t;

/*********************************************************************
 ******************************* 全局变量  ***************************
 *********************************************************************/











#endif
