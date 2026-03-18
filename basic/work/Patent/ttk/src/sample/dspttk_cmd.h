
#ifndef _DSPTTK_CMD_H_
#define _DSPTTK_CMD_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "dspttk_cmd_xray.h"

#define DSPTTK_CHECK_PTR(ptr, ret)    {if (ptr == NULL) {PRT_INFO("Ptr (The address is empty or Value is [%p] )\n", ptr); return (ret);} }
#define DSPTTK_CHECK_CHAN(chan, chanMax, ret) {if (chan > (chanMax)) {PRT_INFO("Chan %d (Illegal parameters)\n", chan); return (ret); }}
#define DSPTTK_CHECK_RET(ret, _check_) {if (ret == _check_) {PRT_INFO("error ret %d\n", ret); return (ret); }}
#define DSPTTK_CHECK_PRM(value, _check_, _ret_)    {if (value == _check_) {PRT_INFO("(The Value is %d )\n", value); return (_ret_); }}

#define CMD_NO_OF(s64Ret)           (s64Ret >> 32) // 64位返回值中取命令号（高32位）
#define CMD_RET_OF(s64Ret)          (s64Ret & 0xFFFFFFFF) // 64位返回值中取错误号（低32位）
#define CMD_RET_MIX(s32Cmd, s32Ret) (((UINT64)s32Cmd << 32) | (UINT32)s32Ret) // 合并32位命令号与错误号到64位

#define U32_FIRST_OF(s32Ret)        ((s32Ret >> 16) & 0xFFFF) //32位数取高16位
#define U32_LAST_OF(s32Ret)         (s32Ret & 0xFFFF) //32位数取低16位
#define U32_COMBINE(s32_0, s32_1, s32Ret) (s32Ret = (s32_0 << 16) | s32_1) //将输出设备号和窗口号合并在一个U32位数中,其中高16位为输出设备号,低16位为窗口号
#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_CMD_H_ */
