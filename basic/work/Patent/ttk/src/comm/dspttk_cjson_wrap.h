
#ifndef _DSPTTK_CJOSN_WRAP_H_
#define _DSPTTK_CJOSN_WRAP_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "sal_type.h"
#include "dspttk_cjson.h"

CJSON *dspttk_cjson_start(char *pJsonStr);

void dspttk_cjson_end(CJSON *pJson);

int dspttk_cjson_get_object(CJSON *pJson, char *key, void *value);

int dspttk_cjson_get_array_size(CJSON *pJson);

CJSON *dspttk_cjson_get_array_item(CJSON *pJson, UINT32 idx);

SAL_STATUS dspttk_cjson_get_value(CJSON *pJson, CHAR *jsonKey, CHAR *valKey, INT32 valType, VOID *pBuf, UINT32 bufLen);

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_CJOSN_WRAP_H_ */

