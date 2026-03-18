/*****************************************************************************
** 版权信息：Copyright (c) 2007, 杭州海康威视数字技术有限公司
**
** 文件名称：G711codec.c
**
** 文件标识：HIKVISION_G711CODEC_C
**
** 文件摘要：ITU G711编解码库
**
** 当前版本：2.1
** 作    者：黄添喜
** 日    期：2007年12月21号
**
** 备    注：
*****************************************************************************/

#include <stdio.h>


/******************************************************************************
* 函数名  : alaw_compress
*
* 功  能  : ALaw encoding rule according ITU-T Rec. G.711.
*
* 参  数:
*           lseg   -    (In)  number of samples
*
*           linbuf -    (In)  buffer with linear samples (only 12 MSBits are takeninto account)
*
*           logbuf -    (Out) buffer with compressed samples (8 bit right justified, without sign extension)
*
* 返回值  : none
*
* 备  注  ：
******************************************************************************/
static void alaw_compress(unsigned int lseg, short *linbuf, unsigned char *logbuf)
{
    short ix, iexp;
    unsigned int n;

    for(n = 0; n < lseg; n++)
    {
        ix = linbuf[n] < 0              /* 0 <= ix < 2048 */
             ? ((~linbuf[n]) >> 4)      /* 1's complement for negative values */
             : ((linbuf[n]) >> 4);

        /* Do more, if exponent > 0 */
        if(ix > 15)                     /* exponent=0 for ix <= 15 */
        {
            iexp = 1;                   /* first step: */

            while(ix > 16 + 15)         /* find mantissa and exponent */
            {
                ix >>= 1;
                iexp++;
            }

            ix -= 16;                   /* second step: remove leading '1' */
            ix += iexp << 4;            /* now compute encoded value */
        }

        if(linbuf[n] >= 0)
        {
            ix |= (0x0080);             /* add sign bit */
        }

        /* toggle even bits */
        logbuf[n] = (unsigned char)(ix ^ (0x0055));
    }
}

/******************************************************************************
* 函数名  : alaw_compress_16k
*
* 功  能  : ALaw encoding rule according ITU-T Rec. G.711.
*
* 参  数:
*           lseg   -    (In)  number of samples
*
*           linbuf -    (In)  buffer with linear samples (only 12 MSBits are takeninto account)
*
*           logbuf -    (Out) buffer with compressed samples (8 bit right justified, without sign extension)
*
* 返回值  : none
*
* 备  注  ：
******************************************************************************/
static void alaw_compress_16k(unsigned int lseg, short *linbuf, unsigned char *logbuf)
{
    short ix, iexp;
    unsigned int n;

    for(n = 0; n < lseg; n++)
    {
        if(n % 2 != 0)
        {
            continue;
        }

        ix = linbuf[n] < 0              /* 0 <= ix < 2048 */
             ? ((~linbuf[n]) >> 4)      /* 1's complement for negative values */
             : ((linbuf[n]) >> 4);

        /* Do more, if exponent > 0 */
        if(ix > 15)                     /* exponent=0 for ix <= 15 */
        {
            iexp = 1;                   /* first step: */

            while(ix > 16 + 15)         /* find mantissa and exponent */
            {
                ix >>= 1;
                iexp++;
            }

            ix -= 16;                   /* second step: remove leading '1' */
            ix += iexp << 4;            /* now compute encoded value */
        }

        if(linbuf[n] >= 0)
        {
            ix |= (0x0080);             /* add sign bit */
        }

        /* toggle even bits */
        logbuf[n / 2] = (unsigned char)(ix ^ (0x0055));
    }
}

/******************************************************************************
* 函数名  :  alaw_expand
*
* 功  能  :  ALaw decoding rule according ITU-T Rec. G.711.
*
* 参  数  :  lseg   -   (In)  number of samples
*
*            logbuf -   (In)  buffer with compressed samples (8 bit right justified, without sign extension)
*
*            linbuf -   (Out) buffer with linear samples (13 bits left justified)
*
* 返回值  :  none
*
* 备  注  :
******************************************************************************/
static void alaw_expand(unsigned int lseg, unsigned char *logbuf, short *linbuf)

{
    short ix, mant, iexp;
    unsigned int n;

    for(n = 0; n < lseg; n++)
    {
        ix = logbuf[n] ^ (0x0055);      /* re-toggle toggled bits */
        ix &= (0x007F);                 /* remove sign bit */

        iexp = ix >> 4;                 /* extract exponent */
        mant = ix & (0x000F);           /* now get mantissa */

        if(iexp > 0)
        {
            mant = mant + 16;           /* add leading '1', if exponent > 0 */
        }

        mant = (mant << 4) + (0x0008);  /* now mantissa left justified and */

        /* 1/2 quantization step added */
        if(iexp > 1)                    /* now left shift according exponent */
        {
            mant = mant << (iexp - 1);
        }

        /* invert, if negative sample */
        linbuf[n] = logbuf[n] > 127 ? mant : -mant;


    }
}

/******************************************************************************
* 函数名  : ulaw_compress
*
* 功  能  :  Mu law encoding rule according ITU-T Rec. G.711.
*
* 参  数  :  lseg    -  (In)  number of samples
*
*            linbuf  -  (In)  buffer with linear samples (only 12 MSBits are taken into account)
*
*            logbuf  -  (Out) buffer with compressed samples (8 bit right justified, without sign extension)
*
* 返回值  : none
*
* 备  注  :
******************************************************************************/
static void ulaw_compress(unsigned int lseg, short *linbuf, unsigned char *logbuf)
{
    unsigned int n;                     /* samples's count */
    short i;                            /* aux.var. */
    /* absolute value of linear (input) sample */
    short absno;
    short segno;                        /* segment (Table 2/G711, column 1) */
    short low_nibble;                   /* low  nibble of log companded sample */
    short high_nibble;                  /* high nibble of log companded sample */

    for(n = 0; n < lseg; n++)
    {
        /* -------------------------------------------------------------------- */
        /* Change from 14 bit left justified to 14 bit right justified */
        /* Compute absolute value; adjust for easy processing */
        /* -------------------------------------------------------------------- */

        absno = linbuf[n] < 0           /* compute 1's complement in case of  */
                /* negative samples */
                ? (((~linbuf[n]) >> 2) + 33)
                /* NB: 33 is the difference value */
                : (((linbuf[n]) >> 2) + 33);

        /* between the thresholds for */
        /* A-law and u-law. */
        if(absno > (0x1FFF))            /* limitation to "absno" < 8192 */
        {
            absno = (0x1FFF);
        }

        /* Determination of sample's segment */
        i = absno >> 6;
        segno = 1;

        while(i != 0)
        {
            segno++;
            i >>= 1;
        }

        /* Mounting the high-nibble of the log-PCM sample */
        high_nibble = (0x0008) - segno;

        /* Mounting the low-nibble of the log PCM sample */
        /* right shift of mantissa and */
        low_nibble = (absno >> segno) & (0x000F);

        /* masking away leading '1' */
        low_nibble = (0x000F) - low_nibble;

        /* Joining the high-nibble and the low-nibble of the log PCM sample */
        logbuf[n] = (unsigned char)((high_nibble << 4) | low_nibble);


        if(linbuf[n] >= 0)
        {
            /* Add sign bit */
            logbuf[n] = logbuf[n] | (0x0080);
        }
    }
}

/******************************************************************************
* 函数名  : ulaw_compress_16k
*
* 功  能  :  Mu law encoding rule according ITU-T Rec. G.711.
*
* 参  数  :  lseg    -  (In)  number of samples
*
*            linbuf  -  (In)  buffer with linear samples (only 12 MSBits are taken into account)
*
*            logbuf  -  (Out) buffer with compressed samples (8 bit right justified, without sign extension)
*
* 返回值  : none
*
* 备  注  :
******************************************************************************/
static void ulaw_compress_16k(unsigned int lseg, short *linbuf, unsigned char *logbuf)
{
    unsigned int n;                     /* samples's count */
    short i;                            /* aux.var. */
    /* absolute value of linear (input) sample */
    short absno;
    short segno;                        /* segment (Table 2/G711, column 1) */
    short low_nibble;                   /* low  nibble of log companded sample */
    short high_nibble;                  /* high nibble of log companded sample */

    for(n = 0; n < lseg; n++)
    {
        /* -------------------------------------------------------------------- */
        /* Change from 14 bit left justified to 14 bit right justified */
        /* Compute absolute value; adjust for easy processing */
        /* -------------------------------------------------------------------- */
        if(n % 2 != 0)
        {
            continue;
        }

        absno = linbuf[n] < 0           /* compute 1's complement in case of  */
                /* negative samples */
                ? (((~linbuf[n]) >> 2) + 33)
                /* NB: 33 is the difference value */
                : (((linbuf[n]) >> 2) + 33);

        /* between the thresholds for */
        /* A-law and u-law. */
        if(absno > (0x1FFF))            /* limitation to "absno" < 8192 */
        {
            absno = (0x1FFF);
        }

        /* Determination of sample's segment */
        i = absno >> 6;
        segno = 1;

        while(i != 0)
        {
            segno++;
            i >>= 1;
        }

        /* Mounting the high-nibble of the log-PCM sample */
        high_nibble = (0x0008) - segno;

        /* Mounting the low-nibble of the log PCM sample */
        /* right shift of mantissa and */
        low_nibble = (absno >> segno) & (0x000F);

        /* masking away leading '1' */
        low_nibble = (0x000F) - low_nibble;

        /* Joining the high-nibble and the low-nibble of the log PCM sample */
        logbuf[n / 2] = (unsigned char)((high_nibble << 4) | low_nibble);


        if(linbuf[n] >= 0)
        {
            /* Add sign bit */
            logbuf[n / 2] = logbuf[n / 2] | (0x0080);
        }
    }
}

/******************************************************************************
* 函数名  : ulaw_expand
*
* 功  能  : Mu law decoding rule according ITU-T Rec. G.711.
*
* 参  数  : lseg   -    (In)  number of samples
*
*           logbuf -    (In)  buffer with compressed samples (8 bit right justified,without sign extension)
*
*           linbuf -    (Out) buffer with linear samples (14 bits left justified)
*
* 返回值  : none
*
* 备  注  :
******************************************************************************/
static void ulaw_expand(unsigned int lseg, unsigned char *logbuf, short *linbuf)
{
    unsigned int n;                     /* aux.var. */
    short segment;                      /* segment (Table 2/G711, column 1) */
    short mantissa;                     /* low  nibble of log companded sample */
    short exponent;                     /* high nibble of log companded sample */
    short sign;                         /* sign of output sample */
    short step;

    for(n = 0; n < lseg; n++)
    {
        /* sign-bit = 1 for positiv values */
        sign = logbuf[n] < (0x0080) ? -1 : 1;

        mantissa = ~logbuf[n];          /* 1's complement of input value */
        /* extract exponent */
        exponent = (mantissa >> 4) & (0x0007);
        segment = exponent + 1;         /* compute segment number */
        mantissa = mantissa & (0x000F); /* extract mantissa */

        /* Compute Quantized Sample (14 bit left justified!) */
        step = (4) << segment;          /* position of the LSB */
        /* = 1 quantization step) */
        linbuf[n] = sign                /* sign */
                    /* '1', preceding the mantissa */
                    * (((0x0080) << exponent)
                       + step * mantissa/* left shift of mantissa */
                       + step / 2       /* 1/2 quantization step */
                       - 4 * 33
                      );
    }
}

/******************************************************************************
* 函数名  : G711ENC_Encode()
*
* 功  能  : 函数调用一次,编码sample_num个样点
*
* 输入参数:
*            type             -      指定A law 编码,type取值为非零,指定 Mu law 编码,type取值为 0
*
*            sample_num       -      指定每次处理的样点数,由用户指定
*
*            samples          -      输入缓存区指针,存放PCM样点,大小不小于sample_num*(sizeof(short)) byte
*
* 输出参数:
*
*            bitstream        -     输出缓存区指针,存放编码后的码流,不小于sample_num byte
*
* 返回值  : 状态码
******************************************************************************/
void G711ENC_Encode(unsigned int type, unsigned int sample_num,
                    unsigned char *samples, unsigned char *bitstream)
{

    short *InDatBuf = (short *)samples;

    if(type == 0)
    {
        ulaw_compress(sample_num, InDatBuf, bitstream);
    }
    else
    {
        alaw_compress(sample_num, InDatBuf, bitstream);
    }
}

/******************************************************************************
* 函数名  : G711ENC_Encode_16K()
*
* 功  能  : 函数调用一次,编码sample_num个样点
*
* 输入参数:
*            type             -      指定A law 编码,type取值为非零,指定 Mu law 编码,type取值为 0
*
*            sample_num       -      指定每次处理的样点数,由用户指定
*
*            samples          -      输入缓存区指针,存放PCM样点,大小不小于sample_num*(sizeof(short)) byte
*
* 输出参数:
*
*            bitstream        -     输出缓存区指针,存放编码后的码流,不小于sample_num byte
*
* 返回值  : 状态码
******************************************************************************/
void G711ENC_Encode_16K(unsigned int type, unsigned int sample_num,
                        unsigned char *samples, unsigned char *bitstream)
{

    short *InDatBuf = (short *)samples;

    if(type == 0)
    {
        ulaw_compress_16k(sample_num, InDatBuf, bitstream);
    }
    else
    {
        alaw_compress_16k(sample_num, InDatBuf, bitstream);
    }
}

/******************************************************************************
* 函数名  : G711DEC_Decode()
*
* 功  能  : 函数调用一次,解码出sample_num个PCM样点
*
* 输入参数:
*             type            -     指定A law 编码,type取值为非零,指定 Mu law 编码,type取值为 0
*
*             sample_num      -     指定每次处理的样点数,由用户指定
*
*             bitstream       -     输入缓存区指针,待解码码流,大小不小于 sample_num byte
*
* 输出参数:
*
*             samples         -     输出缓存区指针,存放解码后的PCM数据,大小不小于sample_num*(sizeof(short)) byte
*
* 返回值  : 状态码
******************************************************************************/
void G711DEC_Decode(unsigned int type, unsigned int sample_num,
                    unsigned char *samples, unsigned char *bitstream)
{

    short *OutDatbuf = (short *)samples;

    if(type == 0)
    {
        ulaw_expand(sample_num, bitstream, OutDatbuf);
    }
    else
    {
        alaw_expand(sample_num, bitstream, OutDatbuf);
    }
}

