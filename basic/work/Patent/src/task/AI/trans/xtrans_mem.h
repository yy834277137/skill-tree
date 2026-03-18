#ifndef _XTRANS_MEM_H_
#define _XTRANS_MEM_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */



#ifdef __cplusplus
extern "C" {
#endif


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#define xtrans_memAlloc(size)              xtrans_memUAllocAlign((size), 0)
#define xtrans_memCalloc(size)             xtrans_memUCallocAlign((size), 0)
#define xtrans_memFree(ptr)                xtrans_memUFree((ptr))
#define xtrans_memCpy(dst, src)            xtrans_memCpySize((dst), (src), sizeof(*(src)))

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */
/*******************************************************************************
* 函数名  : xtrans_memUAllocAlign
* 描  述  : 该函数负责在linux用户态中申请内存
*
* 输  入  : - size:  要申请的内存大小
*           - align: 用户自定义对齐的字节数, 若为0表示不进行自定义对齐
*                    该参数必须为4的整数倍, 否则函数将返回NULL
*
* 输  出  : 无。
* 返回值  : 非NULL: 申请成功
*           NULL:   申请失败
*******************************************************************************/
Ptr xtrans_memUAllocAlign(HPR_UINT32 size, HPR_UINT32 align);

/*******************************************************************************
* 函数名  : xtrans_memUCallocAlign
* 描  述  : 该函数在xtrans_memUAllocAlign的基础上,增加对内存清0的动作
*
* 输  入  : - size:  要申请的内存大小
*           - align: 用户自定义对齐的字节数, 若为0表示不进行自定义对齐
*                    该参数必须为4的整数倍, 否则函数将返回NULL
*
* 输  出  : 无。
* 返回值  : 非NULL:  申请成功
*           NULL:    申请失败
*******************************************************************************/
Ptr xtrans_memUCallocAlign(HPR_UINT32 size, HPR_UINT32 align);


/*******************************************************************************
* 函数名  : xtrans_memUFree
* 描  述  : 该函数负责在linux用户态中释放内存
*
* 输  入  : - pPtr:    要释放的内存指针
* 输  出  : 无
* 返回值  : xtrans_SOK:   成功,内存已释放
*           xtrans_EFAIL: 失败, 内存未释放
*******************************************************************************/
HPR_INT32 xtrans_memUFree(Ptr pPtr);

#define xtrans_memCpySize(dst, src, size)  memcpy((dst), (src), (size))

#ifdef __cplusplus
}
#endif

#endif  /*  _xtrans_MEM_H_  */


