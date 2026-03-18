/*******************************************************************************
 * hal_bufTransform.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : wutao19 <wutao19@hikvision.com>
 * Version: V1.0.0  2020~{Dj~}2~{TB~}20~{HU~} Create
 *
 * Description : ~{8y>]F=L(LX5c#,J9SC;c1`#,~}
                 ~{4&@mJSF5J}>]~}: ~{8qJ=W*;;!";-O_!"~}osd~{5cUsIz3I5H~}
 * Modification:
 *******************************************************************************/

#include <sal.h>
#include <platform_sdk.h>
#include <platform_hal.h>
#include "hal_comm.h"



//~{=+~}nv21~{8qJ=~}yuv~{W*3I~}rgb888~{#,R;4N4&@m~}width~{8vJ}>]~}width~{PhR*~}8~{6TFk~}
void Hal_CommNv21ToRgb24RowAlign8(const char* src_y,
                         const char* src_vu,
                         char* dst_rgb24,
                         int width) {
/* 
     YUV_CONSTANTS* yuvconstants = &yuvI601Constants;
  asm volatile (
    
	  "ld1r       {v24.8h}, [%[kUVBiasBGR]], #2  \n" \
	  "ld1r       {v25.8h}, [%[kUVBiasBGR]], #2  \n" \
	  "ld1r       {v26.8h}, [%[kUVBiasBGR]]      \n" \
	  "ld1r       {v31.4s}, [%[kYToRgb]]         \n" \
	  "ld2        {v27.8h, v28.8h}, [%[kUVToRB]] \n" \
	  "ld2        {v29.8h, v30.8h}, [%[kUVToG]]  \n"
      "1:                                          \n"
    
	  "ld1        {v0.8b}, [%0], #8              \n"  //~{=+~}64bit~{8v~}y~{1#4f5=~}v0 
	  "ld1        {v2.8b}, [%1], #8              \n"  //~{=+=;V/5D~}64bit ~{8v~}vu~{1#4f5=~}v2
	  "uzp1       v3.8b, v2.8b, v2.8b            \n"  //~{=+~}v2~{5DE<J}N;VC5DOqKX<4~}v~{1#4f5=~}v3 ~{92~}32bit
	  "uzp2       v1.8b, v2.8b, v2.8b            \n"  //~{=+~}v2~{5DFfJ}J}N;VC5DOqKX<4~}u~{1#4f5=~}v1 ~{92~}32bit
	  "ins        v1.s[1], v3.s[0]               \n"   //~{=+~} v3~{5D~}32bit ~{<4~}32bit~{5D~}v~{1#4f5=~}v1~{5D8_~}32~{N;#,~}v1~{J}>]1#4fJG~}vvvv-uuuu~{#(5XV74S8_5=5M#)~}
    
	  "uxtl       v0.8h, v0.8b                   \n"  //~{=+~}8~{8v~}8bit~{5DJ}>]VXTXN*~}8~{8v~}16bit~{5DJ}>]#,=+~}v0 ~{<4~}y ~{WsRF~}0~{:s1#4f5=~}v0~{5D~}128bit~{#,~}y~{OqKXV5G?VFN*~}16~{N;~}
	  "shll       v2.8h, v1.8b, #8               \n"  //~{=+~}8~{8v~}8bit~{5DJ}>]WsRF~}8~{N;#,VXTXN*~}8~{8v~}16bit~{5DJ}>]#,=+~}v1~{5D5M~}8~{8vOqKX#,=xPPC?R;8vOqKXWsRF~}8~{N;1#4f5=~}v2~{VP#,@)U9N*~}16~{N;~} v0v0v0v0u0u0u0u0
	  "ushll2     v3.4s, v0.8h, #0               \n"  //~{=+~}v0~{5D8_~}8~{8vOqKXWsRF~}0~{N;#,VXTXN*~}32bit~{5DJ}>]#,1#4f5=~}v3~{VP~}
	  "ushll      v0.4s, v0.4h, #0               \n"  // Y ~{=+~}v0~{5D~} ~{5Z~}8~{8vOqKXWsRF~}0~{N;!"VXTXN*~}32bit~{5DJ}>]#,4K~}2~{PPD?5DJG=+~}8~{8v~}y~{1#4fTZA=8v<D4fFwVPW*N*~}int~{@`PM7=1c:sPx3K~}/                  \
	  "mul        v3.4s, v3.4s, v31.4s           \n"  // y * yg                   \
	  "mul        v0.4s, v0.4s, v31.4s           \n"  // y * yg                   \
	  "sqshrun    v0.4h, v0.4s, #16              \n"  //~{=+~}v0~{VP5D~}4~{8v~}32~{N;5DT*KXWsRF~}16~{N;2"GR=X6ON*~}16bit,~{1#4f5=~}v0~{5D5M~}64~{N;#,2"=+8_~}64~{Ge?U~}
	  "sqshrun2   v0.8h, v3.4s, #16              \n"  // Y -- (y * 0x0101 * yg) >> 16;/   //~{=+~}v3~{VP5D~}4~{8v~}32~{N;5DT*KXWsRF~}16~{N;2"GR=X6ON*~}16bit,~{1#4f5=~}v0~{5D8_~}64~{N;~}  
	  "uaddw      v1.8h, v2.8h, v1.8b            \n"  // Replicate UV  ~{=+~}v1~{5DJ}>]#(~}8bit~{#):M~}v2~{5DJ}>]#(~}16bit~{#)O`<S#,2"8x~}v1~{#(~}0x80+0xff00= 0xff80~{#)~},~{WnVU~}v1~{5DJ}>]JG~}vvvvvvvvuuuuuuuu/ \
	  "mov        v2.d[0], v1.d[1]               \n"  // Extract ~{=+~}8bit~{J}>]W*N*~}16bit V .~{=+~}v1~{5D8_~}64bit~{838x~}v2~{5D5Z~}64bit~{Wn:s~}v2~{JG~}vvvvvvvv/    \
	  "uxtl       v2.8h, v2.8b                   \n"                    \
	  "uxtl       v1.8h, v1.8b                   \n"  // Extract U ~{=+~}8bit~{J}>]W*N*~}16bit/    \
	  "mul        v3.8h, v1.8h, v27.8h           \n"                    \
	  "mul        v5.8h, v1.8h, v29.8h           \n"                    \
	  "mul        v6.8h, v2.8h, v30.8h           \n"                    \
	  "mul        v7.8h, v2.8h, v28.8h           \n"                    \
	  "sqadd      v6.8h, v6.8h, v5.8h            \n"                    \
	  "sqadd      v20.8h, v24.8h, v0.8h      \n" / B -- y1 + bb/                              \
	  "sqadd      v21.8h, v25.8h, v0.8h      \n" / G -- y1 + bg/                              \
	  "sqadd      v22.8h, v26.8h, v0.8h      \n" / R -- y1 + br/                              \
	  
	  "sqadd      v20.8h, v20.8h, v3.8h      \n" / B /                                          \
	  "sqsub      v21.8h, v21.8h, v6.8h  \n" / G /                                          \
	  "sqadd      v22.8h, v22.8h, v7.8h  \n" / R /                                          \
	  
	  "sqshrun    v20.8b, v20.8h, #6     \n" / B /                                          \
	  "sqshrun    v21.8b, v21.8h, #6     \n" / G /            \
	  "sqshrun    v22.8b, v22.8h, #6     \n" / R /
  
      "subs       %w3, %w3, #8                   \n"
      "st3        {v20.8b,v21.8b,v22.8b}, [%2], #24     \n"
      "b.gt       1b                             \n"
    : "+r"(src_y),     // %0
      "+r"(src_vu),    // %1
      "+r"(dst_rgb24),  // %2
      "+r"(width)      // %3
    : [kUVToRB]"r"(&yuvconstants->kUVToRB),
      [kUVToG]"r"(&yuvconstants->kUVToG),
      [kUVBiasBGR]"r"(&yuvconstants->kUVBiasBGR),
      [kYToRgb]"r"(&yuvconstants->kYToRgb)
    : "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v20",
      "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30"
      
  );*/
}


//~{TZ~}yuv~{K.F=7=Or;-O_:/J}!"R;4N;-~}width,width~{PhR*04UU~}8~{6TFk~}
//colorY[8]~{JGQUI+5D~}y~{7VA?V5~},8~{8v~}y~{7VA?6<R;VB~},short colorVu[4]~{JGQUI+5D~}Vu~{7VA?V5~},4~{8v~}Vu~{7VA?6<R;VB~}
 void Hal_commDrawNv21LineAlign8H(UINT8* pAddr_y,UINT8* pAddr_vu,UINT32 width,UINT8 colorY[8],UINT16 colorVu[4])
{
/*
	asm volatile (
	           
				 "ld1 {v0.8b}, [%3]              \n" //~{=+~}8~{8v~}y~{:M~}4~{8v~}vu~{<STX5=<D4fFw~}v0~{:M~}v1~{VP~}
				 "ld1 {v1.4h}, [%4]              \n" 
				 "h8:                           \n"
				 "st1 {v0.8b} ,[%0] ,#8        \n"
				 "st1 {v1.8b} ,[%1] ,#8        \n"
				 "subs %w2,%w2      ,#8             \n"
				 "b.gt h8                     \n"
				 :"+r"(pAddr_y), // %0
				   "+r"(pAddr_vu),//% 1
				   "+r"(width),  //% 2
				   "+r"(colorY),// %3
				   "+r"(colorVu)//%4
				 : 
				 :"cc","memory","v0","v1"
				 );
				 */
}

  //~{TZ~}yuv~{49V17=Or;-O_:/J}!"R;4N;-~}2~{8vJzO_#,8_JG~}hight,~{O_?mJG~}2
 //colorY[2]~{JGQUI+5D~}y~{7VA?V5~},2~{8v~}y~{7VA?6<R;VB~},short colorVu~{JGQUI+5D~}Vu~{7VA?V5~}
 void Hal_commDrawNv21Line2VDouble(UINT8* pAddr_y,UINT8* pAddr_vu,UINT32 s,UINT32 w,UINT32 hight,UINT8 colorY[8],UINT16 colorVu[4])
 {
 /*
	 asm volatile (
				  "ldrh    w7, [%3] 			 \n" //~{=+~}2~{8v~}y~{:M~}1~{8v~}vu~{<STX5=<D4fFw~}w7~{:M~}w6~{VP~}
				  "ldrh    w6, [%4] 			 \n" 
				  "vl2d:						  \n"
				  "strh  w7 ,[%0]		 \n"
				  "strh  w7 ,[%0, %6]    \n"
				  "add	 %0 , %0,	 %5  \n"
				  "strh  w7 ,[%0]		 \n"
				  "strh  w7 ,[%0, %6]		 \n"
				  "add	 %0 , %0,	 %5  \n"
				  "strh  w6 ,[%1]		 \n"
				  "strh  w6 ,[%1,%6]		\n"
				  "add	 %1 , %1,	 %5  \n"
				  "subs %w2 , %w2,	 #2  \n"
				  "b.gt vl2d		  \n"
				  : "+r"(pAddr_y),	//%0
					"+r"(pAddr_vu), //%1
					"+r"(hight),	//%2
					"+r"(colorY),	//%3
					"+r"(colorVu)	//%4
				  : "r"(s), 		 //%5
					"r"(w)			 //%6
				  :"cc","memory"
				  );
				  */
 
 }

//~{TZ~}yuv~{49V17=Or;-O_:/J}!"R;4N;-~}hight,~{#,O_?mJG~}2
//colorY[2]~{JGQUI+5D~}y~{7VA?V5~},2~{8v~}y~{7VA?6<R;VB~},short colorVu~{JGQUI+5D~}Vu~{7VA?V5~}
void Hal_commDrawNv21Line2V(UINT8* pAddr_y,UINT8* pAddr_vu,UINT32 s,UINT32 hight,UINT8 colorY[8],UINT16 colorVu[4])
{
/*
	asm volatile (
				 "ldrh    w7, [%3]              \n" //~{=+~}2~{8v~}y~{:M~}1~{8v~}vu~{<STX5=<D4fFw~}w7~{:M~}w6~{VP~}
				 "ldrh    w6, [%4]              \n" 
				 "vl2:                          \n"
				 "strh  w7 ,[%0]        \n"
				 "add   %0 , %0,    %5  \n"
				 "strh  w7 ,[%0]        \n"
				 "add   %0 , %0,    %5  \n"
				 "strh  w6 ,[%1]        \n"
				 "add   %1 , %1,    %5  \n"
				 "subs %w2 , %w2,   #2  \n"
				 "b.gt vl2          \n"
				 : "+r"(pAddr_y),  //%0
				   "+r"(pAddr_vu), //%1
				   "+r"(hight),    //%2
				   "+r"(colorY),   //%3
				   "+r"(colorVu)   //%4
				 : "r"(s)          //%5
				 :"cc","memory"
				 );
*/
}

//~{TZ~}yuv~{K.F=7=Or;-O_:/J}!"R;4N;-~}width,width~{PhR*04UU~}16~{6TFk~}
//colorY[8]~{JGQUI+5D~}y~{7VA?V5~},8~{8v~}y~{7VA?6<R;VB~},short colorVu[4]~{JGQUI+5D~}Vu~{7VA?V5~},4~{8v~}Vu~{7VA?6<R;VB~}
void Hal_commDrawNv21LineAlign16H(UINT8* pAddr_y,UINT8* pAddr_vu,UINT32 width,UINT8 colorY[8],UINT16 colorVu[4])
{
/*
	asm volatile (
				 "ld1        {v0.8b}, [%3]              \n" //~{=+~}8~{8v~}y~{:M~}4~{8v~}vu~{<STX5=<D4fFw~}v0~{:M~}v1~{VP~}
				 "ld1        {v1.4h}, [%4]              \n" 
				 "shll       v2.8h, v0.8b, #8      \n"  //~{=+~}y~{@)U9N*~}16bit
				 "shll       v3.4s, v1.4h, #16     \n"  //~{=+~}vu~{@)U9N*~}32bit
				 "uaddw      v0.8h, v2.8h, v0.8b   \n"  //~{=+~}y~{@)U9N*~}16~{8v~}
				 "uaddw      v1.4s, v3.4s, v1.4h   \n"  //~{=+~}uv~{@)U9N*~}8~{8v~}
			     "h16:                           \n"
				 "st1 {v0.16b} ,[%0] ,#16        \n"
				 "st1 {v1.16b} ,[%1] ,#16        \n"
				 "subs %w2,%w2,    #16        \n"
				 "b.gt h8                     \n"
				 : "+r"(pAddr_y),                       // 0
				   "+r"(pAddr_vu),                      // 1
				   "+r"(width),                         // 2
				   "+r"(colorY),                        // 3              
				   "+r"(colorVu)                        // 4
				 :
				 : "cc","memory","v0","v1","v2","v3"
				 );
				 */
}


//~{TZ~}y~{K.F=7=Or;-O_:/J}!"R;4N;-~}width,width~{PhR*04UU~}8~{6TFk~}
//colorY[8]~{JGQUI+5D~}y~{7VA?V5~},8~{8v~}y~{7VA?6<R;VB~}
void Hal_commDrawYLineAlign8H(UINT8* pAddr_y,UINT32 width,UINT8 colorY[8])
{
/*
	asm volatile (
				 "ld1 {v0.8b}, [%2]              \n" //~{=+~}8~{8v~}y~{<STX5=<D4fFw~}v0~{VP~}
				 "hy8:                         \n"
				 "st1 {v0.8b} ,[%0] ,#8        \n"
				 "subs %w1,%w1,#8\n"
				 "b.gt hy8\n"
				 :"+r"(pAddr_y), //0
				  "+r"(width), //1
				  "+r"(colorY) //2
				 :
				 :"cc","memory","v0"
				 );
				 */
}

//~{TZ~}y~{K.F=7=Or;-O_:/J}!"R;4N;-~}width,width~{PhR*04UU~}16~{6TFk~}
//colorY[8]~{JGQUI+5D~}y~{7VA?V5~},8~{8v~}y~{7VA?6<R;VB~}
void Hal_commDrawYLineAlign16H(UINT8* pAddr_y,UINT16 width,UINT8 colorY[8])
{
/*
	asm volatile (
				 "ld1        {v0.8b}, [%2]              \n" //~{=+~}8~{8v~}y~{:M~}4~{8v~}vu~{<STX5=<D4fFw~}v0~{:M~}v1~{VP~}
				 "shll       v2.8h, v0.8b, #8      \n"  //~{=+~}y~{@)U9N*~}16bit
				 "uaddw      v0.8h, v2.8h, v0.8b   \n"  //~{=+~}y~{@)U9N*~}16~{8v~}
			     "hy16:                           \n"
				 "st1 {v0.16b} ,[%0] ,#16        \n"
				 "subs %w1,%w1,    #16        \n"
				 "b.gt hy16                     \n"
				 :"+r"(pAddr_y), //0
				   "+r"(width),
				   "+r"(colorY)
				 :
				 :"cc","memory","v0","v2"
				 );
				 */
}

void Hal_commDrawCharBgBlackH1_Neon(UINT8 *pFont, UINT8 * pDst, UINT8 *pOsdColorTalbe,UINT8 colorTb[16],UINT32 w)
{
/*
    UINT8 * tempTable = NULL;
	UINT32    offset   = 0;
    asm volatile(
		"ld1      {v0.8h},  [%3]                   \n" // fgcolor~{?=145=~}v0~{5D~}8~{8vT*KXVP~}
		"h1v1:                                     \n"
		"ldrb     %w4,     [%0],      #1           \n" // offset = pFont[i]  pFont ~{My:sF+RF~}
		
		"add      %5,    %6,  %4,  lsl #4          \n" //offset = offset * 16,tempTable = osdColorTalbe + offset
		"ld1      {v1.16b},  [%5]                  \n" //~{<STX~}osdColorTalbe~{6TS&5D~}16~{8vV55=~}v1~{VP~}
		"subs     %w1,      %w1,      #1           \n"
		"tbl      v2.16b,   {v0.16b}, v1.16b       \n"
		
		"st1      {v2.8h},  [%2],     #16          \n" //~{=+J}>]<STX5=AYJ1~}tempDst~{VPGR~}tempDst +16
		
		"b.gt     h1v1                             \n"
	    :"+r"(pFont),          // 0
	     "+r"(w),              // 1
	     "+r"(pDst),           // 2
	     "+r"(colorTb),        // 3
	     "+r"(offset),         // 4
	     "+r"(tempTable),      // 5
	     "+r"(pOsdColorTalbe)  // 6
		:
		:"cc","memory","v0","v1","v2"
	   );
	#if 0
       if (tempTable)
	   printf("%d,{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}\n",offset,
	    tempTable[0],tempTable[1],tempTable[2],tempTable[3],tempTable[4],tempTable[5],tempTable[6],tempTable[7],
	    pOsdColorTalbe[offset],pOsdColorTalbe[offset + 1],pOsdColorTalbe[offset + 2],
	    pOsdColorTalbe[offset + 3],pOsdColorTalbe[offset+ 4],pOsdColorTalbe[offset + 5],
	    pOsdColorTalbe[offset + 6],pOsdColorTalbe[offset + 7],
	    colorTb[0],colorTb[1],colorTb[2],colorTb[3],colorTb[4],colorTb[5],colorTb[6],colorTb[7],
	    pDst[0 - 16],pDst[1- 16],pDst[2- 16],pDst[3- 16],pDst[4- 16],pDst[5- 16],pDst[6- 16],pDst[7- 16]);
	#endif
	*/
}

//~{K.F=7=Or@)4s~}2~{16~}
void Hal_commDrawCharBgBlackH2_Neon(UINT8 *pFont, UINT8 * pDst, UINT8 *pOsdColorTalbe,UINT8 colorTb[16],UINT32 w)
{
/*
    UINT8 * tempTable = NULL;
	UINT32    offset   = 0;
    asm volatile(
		"ld1      {v0.8h},  [%3]                   \n" // fgcolor~{?=145=~}v0~{5D~}8~{8vT*KXVP~}
		"h2:                                     \n"
		"ldrb     %w4,     [%0],      #1           \n" // offset = pFont[i]  pFont ~{My:sF+RF~}
		
		"add      %5,    %6,  %4,  lsl #4          \n" //offset = offset * 16,tempTable = osdColorTalbe + offset
		"ld1      {v1.16b},  [%5]                  \n" //~{<STX~}osdColorTalbe~{6TS&5D~}16~{8vV55=~}v1~{VP~}
		"subs     %w1,      %w1,      #1           \n"
		"tbl      v2.16b,   {v0.16b}, v1.16b       \n"
		"mov      v3.16b,    v2.16b       \n"
		"st2      {v2.8h,v3.8h},  [%2],     #32    \n" //~{=+J}>]<STX5=AYJ1~}tempDst~{VPGR~}tempDst +32
		
		"b.gt     h2                             \n"
	    :"+r"(pFont),          // 0
	     "+r"(w),              // 1
	     "+r"(pDst),           // 2
	     "+r"(colorTb),        // 3
	     "+r"(offset),         // 4
	     "+r"(tempTable),      // 5
	     "+r"(pOsdColorTalbe)  // 6
		:
		:"cc","memory","v0","v1","v2","v3"
	   );
	#if 0
       if (tempTable)
	   printf("%d,{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}\n",offset,
	    tempTable[0],tempTable[1],tempTable[2],tempTable[3],tempTable[4],tempTable[5],tempTable[6],tempTable[7],
	    pOsdColorTalbe[offset],pOsdColorTalbe[offset + 1],pOsdColorTalbe[offset + 2],
	    pOsdColorTalbe[offset + 3],pOsdColorTalbe[offset+ 4],pOsdColorTalbe[offset + 5],
	    pOsdColorTalbe[offset + 6],pOsdColorTalbe[offset + 7],
	    colorTb[0],colorTb[1],colorTb[2],colorTb[3],colorTb[4],colorTb[5],colorTb[6],colorTb[7],
	    pDst[0 - 16],pDst[1- 16],pDst[2- 16],pDst[3- 16],pDst[4- 16],pDst[5- 16],pDst[6- 16],pDst[7- 16]);
	#endif
	*/
}

//~{K.F=7=Or@)4s~}3~{16~}
void Hal_commDrawCharBgBlackH3_Neon(UINT8 *pFont, UINT8 * pDst, UINT8 *pOsdColorTalbe,UINT8 colorTb[16],UINT32 w)
{
/*
    UINT8 * tempTable = NULL;
	UINT32    offset   = 0;
    asm volatile(
		"ld1      {v0.8h},  [%3]                   \n" // fgcolor~{?=145=~}v0~{5D~}8~{8vT*KXVP~}
		"h3:                                     \n"
		"ldrb     %w4,     [%0],      #1           \n" // offset = pFont[i]  pFont ~{My:sF+RF~}
		
		"add      %5,    %6,  %4,  lsl #4          \n" //offset = offset * 16,tempTable = osdColorTalbe + offset
		"ld1      {v1.16b},  [%5]                  \n" //~{<STX~}osdColorTalbe~{6TS&5D~}16~{8vV55=~}v1~{VP~}
		"subs     %w1,      %w1,      #1           \n"
		"tbl      v2.16b,   {v0.16b}, v1.16b       \n"
		"mov      v3.16b,    v2.16b                \n"
		"mov      v4.16b,    v2.16b       \n"
		"st3      {v2.8h,v3.8h,v4.8h},  [%2],     #48    \n" //~{=+J}>]<STX5=AYJ1~}tempDst~{VPGR~}tempDst +32
		
		"b.gt     h3                             \n"
	    :"+r"(pFont),          // 0
	     "+r"(w),              // 1
	     "+r"(pDst),           // 2
	     "+r"(colorTb),        // 3
	     "+r"(offset),         // 4
	     "+r"(tempTable),      // 5
	     "+r"(pOsdColorTalbe)  // 6
		:
		:"cc","memory","v0","v1","v2","v3","v4"
	   );
	#if 0
       if (tempTable)
	   printf("%d,{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}\n",offset,
	    tempTable[0],tempTable[1],tempTable[2],tempTable[3],tempTable[4],tempTable[5],tempTable[6],tempTable[7],
	    pOsdColorTalbe[offset],pOsdColorTalbe[offset + 1],pOsdColorTalbe[offset + 2],
	    pOsdColorTalbe[offset + 3],pOsdColorTalbe[offset+ 4],pOsdColorTalbe[offset + 5],
	    pOsdColorTalbe[offset + 6],pOsdColorTalbe[offset + 7],
	    colorTb[0],colorTb[1],colorTb[2],colorTb[3],colorTb[4],colorTb[5],colorTb[6],colorTb[7],
	    pDst[0 - 16],pDst[1- 16],pDst[2- 16],pDst[3- 16],pDst[4- 16],pDst[5- 16],pDst[6- 16],pDst[7- 16]);
	#endif
	*/
}

//~{K.F=7=Or@)4s~}4~{16~}
void Hal_commDrawCharBgBlackH4_Neon(UINT8 *pFont, UINT8 * pDst, UINT8 *pOsdColorTalbe,UINT8 colorTb[16],UINT32 w)
{
/*
    UINT8 * tempTable = NULL;
	UINT32    offset   = 0;
    asm volatile(
		"ld1      {v0.8h},  [%3]                   \n" // fgcolor~{?=145=~}v0~{5D~}8~{8vT*KXVP~}
		"h4:                                     \n"
		"ldrb     %w4,     [%0],      #1           \n" // offset = pFont[i]  pFont ~{My:sF+RF~}
		
		"add      %5,    %6,  %4,  lsl #4          \n" //offset = offset * 16,tempTable = osdColorTalbe + offset
		"ld1      {v1.16b},  [%5]                  \n" //~{<STX~}osdColorTalbe~{6TS&5D~}16~{8vV55=~}v1~{VP~}
		"subs     %w1,      %w1,      #1           \n"
		"tbl      v2.16b,   {v0.16b}, v1.16b       \n"
		"mov      v3.16b,    v2.16b                \n"
		"mov      v4.16b,    v2.16b       \n"
		"mov      v5.16b,    v2.16b       \n"
		"st4      {v2.8h,v3.8h,v4.8h,v5.8h},  [%2], #64    \n" //~{=+J}>]<STX5=AYJ1~}tempDst~{VPGR~}tempDst +32
		
		"b.gt     h4                             \n"
	    :"+r"(pFont),          // 0
	     "+r"(w),              // 1
	     "+r"(pDst),           // 2
	     "+r"(colorTb),        // 3
	     "+r"(offset),         // 4
	     "+r"(tempTable),      // 5
	     "+r"(pOsdColorTalbe)  // 6
		:
		:"cc","memory","v0","v1","v2","v3","v4","v5"
	   );
	#if 0
       if (tempTable)
	   printf("%d,{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}{%x,%x,%x,%x,%x,%x,%x,%x}\n",offset,
	    tempTable[0],tempTable[1],tempTable[2],tempTable[3],tempTable[4],tempTable[5],tempTable[6],tempTable[7],
	    pOsdColorTalbe[offset],pOsdColorTalbe[offset + 1],pOsdColorTalbe[offset + 2],
	    pOsdColorTalbe[offset + 3],pOsdColorTalbe[offset+ 4],pOsdColorTalbe[offset + 5],
	    pOsdColorTalbe[offset + 6],pOsdColorTalbe[offset + 7],
	    colorTb[0],colorTb[1],colorTb[2],colorTb[3],colorTb[4],colorTb[5],colorTb[6],colorTb[7],
	    pDst[0 - 16],pDst[1- 16],pDst[2- 16],pDst[3- 16],pDst[4- 16],pDst[5- 16],pDst[6- 16],pDst[7- 16]);
	#endif
	*/
}


void Hal_commDrawCharBgBlackH1V1(UINT8 *pFont, UINT8 * pDst, UINT32 w, UINT32 h, UINT32 pitch, UINT16 color,UINT32 bGcolor)
{
    UINT32 i;
	UINT8  * tempDst   = NULL;
	UINT8  * tempSrc   = NULL;
	UINT16  colorTb[8] = {0};

	colorTb[0]=colorTb[2]=colorTb[4]=colorTb[6] = bGcolor;
	colorTb[1]=colorTb[3]=colorTb[5]=colorTb[7] = color;
	
	
    for(i = 0; i < h; i++)
    {
        tempDst = pDst + i * pitch *2;
		tempSrc = pFont + i * w / 8;
        Hal_commDrawCharBgBlackH1_Neon(tempSrc,tempDst,(UINT8 *)osdColorTalbe,(UINT8 *)colorTb,w / 8);
#if 0
        UINT32 j;
        UINT16 * tempTable = NULL;
		for(j = 0;j < w ;j++)
		{
		    tempTable = (UINT16 *)tempDst;
		    if (*(tempTable + j) != 0xC210)
		    {
		       //printf("%x",*(tempTable + j));
			   printf("1");
		    }
			else
			{
			   printf(" ");
			}
		}
	    printf("\n");
#endif
    }

}

void Hal_commDrawCharBgBlackH2V2(UINT8 *pFont, UINT8 * pDst, UINT32 w, UINT32 h, UINT32 pitch, UINT16 color,UINT32 bGcolor)
{
    UINT32 i,looCnt = h / 2;
	UINT8  * tempDst   = NULL;
	UINT8  * tempSrc   = NULL;
	UINT16  colorTb[8] = {0};

	colorTb[0]=colorTb[2]=colorTb[4]=colorTb[6] = bGcolor;
	colorTb[1]=colorTb[3]=colorTb[5]=colorTb[7] = color;
	
    for(i = 0; i < looCnt; i++)
    {
        tempDst = pDst + 2 * i * pitch *2;
		tempSrc = pFont + i * w / 16;
        Hal_commDrawCharBgBlackH2_Neon(tempSrc,tempDst,(UINT8 *)osdColorTalbe,(UINT8 *)colorTb,w / 16);
		tempDst = pDst + (2 * i + 1) * pitch *2;
        Hal_commDrawCharBgBlackH2_Neon(tempSrc,tempDst,(UINT8 *)osdColorTalbe,(UINT8 *)colorTb,w / 16);
    }
}

void Hal_commDrawCharBgBlackH3V3(UINT8 *pFont, UINT8 * pDst, UINT32 w, UINT32 h, UINT32 pitch, UINT16 color,UINT32 bGcolor)
{
    UINT32 i,looCnt = h / 3;
	UINT8  * tempDst   = NULL;
	UINT8  * tempSrc   = NULL;
	UINT16  colorTb[8] = {0};

	colorTb[0]=colorTb[2]=colorTb[4]=colorTb[6] = bGcolor;
	colorTb[1]=colorTb[3]=colorTb[5]=colorTb[7] = color;
	
    for(i = 0; i < looCnt; i++)
    {
        tempDst = pDst + 3 * i * pitch *2;
		tempSrc = pFont + i * w / 24;
        Hal_commDrawCharBgBlackH3_Neon(tempSrc,tempDst,(UINT8 *)osdColorTalbe,(UINT8 *)colorTb,w / 24);
        tempDst = pDst + (3 * i + 1)* pitch *2;
		Hal_commDrawCharBgBlackH3_Neon(tempSrc,tempDst,(UINT8 *)osdColorTalbe,(UINT8 *)colorTb,w / 24);
		tempDst = pDst + (3 * i + 2)* pitch *2;
		Hal_commDrawCharBgBlackH3_Neon(tempSrc,tempDst,(UINT8 *)osdColorTalbe,(UINT8 *)colorTb,w / 24);
    }
}

void Hal_commDrawCharBgBlackH4V4(UINT8 *pFont, UINT8 * pDst, UINT32 w, UINT32 h, UINT32 pitch, UINT16 color,UINT32 bGcolor)
{
    UINT32 i,looCnt = h / 3;
	UINT8  * tempDst   = NULL;
	UINT8  * tempSrc   = NULL;
	UINT16  colorTb[8] = {0};

	colorTb[0]=colorTb[2]=colorTb[4]=colorTb[6] = bGcolor;
	colorTb[1]=colorTb[3]=colorTb[5]=colorTb[7] = color;
	
    for(i = 0; i < looCnt; i++)
    {
        tempDst = pDst + 4 * i * pitch *2;
		tempSrc = pFont + i * w / 32;
        Hal_commDrawCharBgBlackH4_Neon(tempSrc,tempDst,(UINT8 *)osdColorTalbe,(UINT8 *)colorTb,w / 32);
		tempDst = pDst + (4 * i + 1)* pitch *2;
		Hal_commDrawCharBgBlackH4_Neon(tempSrc,tempDst,(UINT8 *)osdColorTalbe,(UINT8 *)colorTb,w / 32);
		tempDst = pDst + (4 * i + 2)* pitch *2;
		Hal_commDrawCharBgBlackH4_Neon(tempSrc,tempDst,(UINT8 *)osdColorTalbe,(UINT8 *)colorTb,w / 32);
		tempDst = pDst + (4 * i + 3)* pitch *2;
		Hal_commDrawCharBgBlackH4_Neon(tempSrc,tempDst,(UINT8 *)osdColorTalbe,(UINT8 *)colorTb,w / 32);
    }
}

void Hal_commNv21ToRgbAlign8(const char* src_y,const char* src_vu, char* rgb_buf,
                                           int width,int height,int stried)
{
   int y = 0;
   rgb_buf = rgb_buf + (3 * width * (height - 1));
   SAL_INFO("wxh %d,%d,stried %d\n",width,height,stried);
   for (y = 0; y < height; ++y) 
   { 
       Hal_CommNv21ToRgb24RowAlign8(src_y, src_vu, rgb_buf, width);
       rgb_buf -= 3 * width;
       src_y += stried;
       if (y & 1) 
	   {
           src_vu += stried;
       }
   }
}


