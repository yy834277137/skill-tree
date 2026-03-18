
#ifndef __CHAR_CODE_H_
#define __CHAR_CODE_H_

char *Tran2Unicode(UINT32 u32StrEnctype,char *szSrcBuff, UINT32 u32SrcSize, char *szUniBuff, UINT32 u32UniSize);

UINT32 GB_GetCodeNum(UINT8 *pu8String);

UINT32 UCS2LE_GetCodeNum(UINT8 *pu8String);

#endif

