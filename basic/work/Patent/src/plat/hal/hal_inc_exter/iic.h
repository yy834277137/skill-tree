
#ifndef __IIC_H_
#define __IIC_H_

#define IIC_INVALID     (0xFFFFFFFF)

/* IIC外设地址定义 */
typedef struct
{
    UINT32 u32IIC;
    UINT32 u32DevAddr;
} IIC_DEV_S;

/* IIC地址与赋值的对应关系 */
typedef struct
{
    UINT8 u8Addr;
    UINT8 u8Value;
} IIC_ADDR_MAP_S;

INT32 IIC_Open(UINT32 u32IIC);
INT32 IIC_Close(UINT32 u32IIC);
INT32 IIC_Read(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32Reg, UINT8 *pu8Data);
INT32 IIC_ReadArray(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32RegNum, const UINT8 *pu8Reg, UINT8 *pu8Data);
INT32 IIC_ReadBytes(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32Reg, UINT32 u32RegNum, UINT32 u32Step, UINT8 *pu8Data);
INT32 IIC_Write(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32Reg, UINT8 u8Data);
INT32 IIC_WriteBytes(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32Reg, UINT32 u32RegNum, UINT32 u32Step, UINT8 *pu8Data);
INT32 IIC_WriteArray(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32RegNum, const IIC_ADDR_MAP_S *pstMap);

#endif

