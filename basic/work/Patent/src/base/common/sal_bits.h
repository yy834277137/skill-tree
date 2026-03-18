/***
 * @file   sal_bits.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  位操作
 * @author cuifeng5
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef __SAL_BITS_H_
#define __SAL_BITS_H_

#define BITS_OF_BYTE (8)

/* 操作一位 */
#define BIT_SET(val, bit, type)     ((val) |= ((type)(0x01) << (bit)))
#define BIT_CLEAR(val, bit, type)   ((val) &= ~((type)(0x01) << (bit)))
#define BIT_IS_SET(val, bit, type)  ((val) & (type)(0x01 << (bit)))

/* 将src(8位)的bits位赋值到dst */
#define BITS_SET(dst, src, dst_start, src_start, bits) \
    do \
    { \
        if ((src_start) + (bits) > BITS_OF_BYTE) break; \
        if ((dst_start) + (bits) > BITS_OF_BYTE) break; \
        (dst) &= ~((0xFF >> (BITS_OF_BYTE - (bits))) << (dst_start)); \
        (dst) |= (((0xFF >> (BITS_OF_BYTE - (bits))) & ((src) >> (src_start))) << (dst_start)); \
    } while (0)


/* 将msb(8位)与lsb(8位)对应的位数拼接起来赋值给dst */
#define BITS_CAT(dst, msb, msb_start, msb_bits, lsb, lsb_start, lsb_bits) \
    do \
    { \
        (dst) = ((0xFF >> (BITS_OF_BYTE - (msb_bits))) << (msb_start)) & (msb); \
        (dst) >>= (msb_start); \
        (dst) <<= (lsb_bits); \
        (dst) |= ((((0xFF >> (BITS_OF_BYTE - (lsb_bits))) << (lsb_start)) & (lsb)) >> (lsb_start)); \
    } while (0)


#endif


