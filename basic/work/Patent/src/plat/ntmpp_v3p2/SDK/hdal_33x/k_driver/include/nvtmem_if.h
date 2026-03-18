#ifndef __SOC_NVT_IVOT_NVTMEM_IF_H
#define __SOC_NVT_IVOT_NVTMEM_IF_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

typedef enum {
	ALLOC_MAP_AUTO		= 0x0, /* for fpga purpose: non hdal area will alloc from cma */
	ALLOC_CACHEABLE		= 0x1, /* Cachable */
	ALLOC_NONCACHABLE	= 0x2, /* non-cachable, non-bufferable */
	ALLOC_BUFFERABLE	= 0x4, /* bufferable, but non-cache */
	ALLOC_MMU_TYPE_MASK	= 0xfff,	/* MMU type mask */
	ALLOC_MAP_ONLY		= 0x1000, /* Mapping only: for all address area will do mapping only */
} alloc_type_t;

enum dma_data_dir {
	DMA_DIR_BIDIRECTIONAL = 0,
	DMA_DIR_TO_DEVICE = 1,
	DMA_DIR_FROM_DEVICE = 2,
	DMA_DIR_NONE = 3,
};

struct nvtmem_vaddr_info {
	uintptr_t vaddr;			/* must be 64 bytes(cache line) alignment */
	unsigned long length;		/* must be 64 bytes(cache line) alignment */
	/* DMA_BIDIRECTIONAL, it means clean and invalidate operation.
	 * DMA_TO_DEVICE, it means clean operation.
	 * DMA_FROM_DEVICE, it means invalidate operation.
	 */
	enum dma_data_dir dir;
};

/* ddr id of whole system across pcie chips
 */


#define MAX_DDR_NR	8

/* get whole system DDR information */
struct nvtmem_ddr_all {
	unsigned int nr_banks;	/* how many ddrs */
	struct {
		uintptr_t       start;
		unsigned long   size;
		int             ddrid;
	} bank[MAX_DDR_NR];
};

struct nvtmem_hdal_base {
	uintptr_t ddr_id[MAX_DDR_NR];
	uintptr_t base[MAX_DDR_NR];
	unsigned long size[MAX_DDR_NR];
};

#define NVTMEM_IOC_MAGIC         'M'
#define NVTMEM_GIVE_FREEPAGES    _IOW(NVTMEM_IOC_MAGIC, 2, int)
#define NVTMEM_FORCE_FREEPAGES   _IOW(NVTMEM_IOC_MAGIC, 3, int)
#define NVTMEM_ALLOC_TYPE        _IOW(NVTMEM_IOC_MAGIC, 4, uintptr_t)
#define NVTMEM_ADDR_V2P          _IOWR(NVTMEM_IOC_MAGIC, 5, uintptr_t)
#define NVTMEM_MEMORY_SYNC       _IOWR(NVTMEM_IOC_MAGIC, 6, struct nvtmem_vaddr_info)
#define NVTMEM_GET_DDR_MAP_ALL   _IOWR(NVTMEM_IOC_MAGIC, 7, struct nvtmem_ddr_all)
#define NVTMEM_GET_DTS_HDAL_BASE _IOR(NVTMEM_IOC_MAGIC, 8, struct nvtmem_hdal_base)

#endif /* __SOC_NVT_IVOT_NVTMEM_IF_H */
