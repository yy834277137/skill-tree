
/*******************************************************************************
 * bt_timing.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : cuifeng5
 * Version: V1.0.0  2020年09月07日 Create
 *
 * Description : 时序相关操作
 * Modification: 
 *******************************************************************************/

#include "platform_hal.h"

/*******************************************************
理论计算以下时序在VGA中可能出现混淆

                时序          总行数    垂直同步 行频
         VESA_640X350P85     445       3   37861
         VESA_640X400P85     445       3   37861

          GTF_640X480P60     497       3   29820
          GTF_720X480P60     497       3   29820

          GTF_640X480P73     501       3   36573
        GTF_848X480P71_9     501       3   36021

          CVT_848X480P70     503       5   34981    可以通过降低误差来区别
        CVT_848X480P71_9     503       5   36147

         GTF_1024X768P60     795       3   47700
         GTF_1280X768P60     795       3   47699

         GTF_1024X768P72     801       3   57671
         GTF_1280X768P72     801       3   57671

         GTF_1024X768P75     802       3   60150
         GTF_1280X768P75     802       3   60150

         GTF_1152X900P76     940       3   71439    可以通过降低误差来区别
         GTF_1440X900P75     940       3   70500

        VESA_1280X800P75     838       6   62795
         CVT_1280X800P75     838       6   62794

        CVT_1280X1024P75    1072       7   80292    可以通过降低误差来区别
        CVT_1280X1024P76    1072       7   81452

        GTF_1400X1050P60    1087       3   65220
        GTF_1680X1050P60    1087       3   65220

         GTF_1440X900P60     932       3   55920
         GTF_1600X900P60     932       3   55919

        GTF_1600X1200P60    1242       3   74520
        GTF_1920X1200P60    1242       3   74520

     CVT_1920X1440P60_RB    1481       4   88822
        GTF_1920X1440P60    1481       4   88822
*****************************************************/

#define BT_TIMING_CVT_640X350P50 \
    { \
        640, 350, 50, 49.119, 18125, 14500000, \
        16, 64, 80, 3, 10, 6, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_640X350P85 \
    { \
        640, 350, 85, 85.080, 37861, 31500000, \
        32, 64, 96, 32, 3, 60, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_VESA_640X400P85 \
    { \
        640, 400, 85, 85.080, 37861, 31500000, \
        32, 64, 96, 1, 3, 41, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_640X480P60 \
    { \
        640, 480, 60, 59.940, 31469, 25175000, \
        16, 96, 48, 10, 2, 33, POLARITY_NEG, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_640X480P60 \
    { \
        640, 480, 60, 59.38, 29690, 23750000, \
        24, 56, 80, 3, 4, 13, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_640X480P60_RB \
    { \
        640, 480, 60, 59.46, 29373, 23500000, \
        48, 32, 80, 3, 4, 7, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_640X480P60 \
    { \
        640, 480, 60, 60, 29820, 23860000, \
        16, 64, 80, 1, 3, 13, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_640X480P72 \
    { \
        640, 480, 72, 72.809, 37861, 31500000, \
        16, 40, 120, 1, 3, 20, POLARITY_NEG, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_640X480P73 \
    { \
        640, 480, 73, 72.338, 36458, 29750000, \
        24, 64, 88, 3, 4, 17, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_640X480P73 \
    { \
        640, 480, 73, 73, 36573, 29843570, \
        24, 64, 88, 1, 3, 17, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_640X480P75 \
    { \
        640, 480, 75, 75, 37500, 31500000, \
        16, 64, 120, 1, 3, 16, POLARITY_NEG, POLARITY_NEG, \
    }

#define BT_TIMING_VESA_640X480P85 \
    { \
        640, 480, 85, 85.008, 43269, 36000000, \
        56, 56, 80, 1, 3, 25, POLARITY_NEG, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_640X500P58 \
    { \
        640, 500, 58, 57.692, 30000, 24000000, \
        16, 64, 80, 3, 10, 7, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_640X500P58 \
    { \
        640, 500, 58, 58, 30044, 24035200, \
        16, 64, 80, 1, 3, 14, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_720X400P85 \
    { \
        720, 400, 85, 85.039, 37927, 35500000, \
        36, 72, 108, 1, 3, 42, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_720X480P60 \
    { \
        720, 480, 60, 59.710, 29854, 26750000, \
        24, 64, 88, 3, 10, 7, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_720X480P60_RB \
    { \
        720, 480, 60, 59.779, 29829, 26250000, \
        48, 32, 80, 3, 10, 6, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_720X480P60 \
    { \
        720, 480, 60, 60, 29820, 26718721, \
        16, 72, 88, 1, 3, 13, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_720X576P50 \
    { \
        720, 576, 50, 49.624, 29575, 26500000, \
        24, 64, 88, 3, 7, 10, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_720X576P50 \
    { \
        720, 576, 50, 50, 29649, 26566398, \
        16, 72, 88, 1, 3, 13, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_720X576P60 \
    { \
        720, 576, 60, 59.950, 35910, 32750000, \
        24, 72, 96, 3, 7, 13, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_720X576P60_RB \
    { \
        720, 576, 60, 59.884, 35511, 31250000, \
        48, 32, 80, 3, 7, 7, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_720X576P60 \
    { \
        720, 576, 60, 60, 35819, 32667839, \
        24, 72, 96, 1, 3, 17, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_800X600P56 \
    { \
        800, 600, 56, 56.250, 35156, 36000000, \
        24, 72, 128, 1, 2, 22, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_VESA_800X600P60 \
    { \
        800, 600, 60, 60.317, 37879, 40000000, \
        40, 128, 88, 1, 4, 23, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_800X600P60 \
    { \
        800, 600, 60, 59.86, 37350, 38250000, \
        32, 80, 112, 3, 4, 17, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_800X600P60_RB \
    { \
        800, 600, 60, 59.84, 36980, 35500000, \
        48, 32, 80, 3, 4, 11, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_800X600P60 \
    { \
        800, 600, 60, 60, 37320, 38220000, \
        32, 80, 112, 1, 3, 18, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_800X600P67 \
    { \
        800, 600, 67, 66.584, 41748, 42750000, \
        32, 80, 112, 3, 4, 20, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_800X600P67 \
    { \
        800, 600, 67, 67, 41807, 43480319, \
        40, 80, 120, 1, 3, 20, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_800X600P70 \
    { \
        800, 600, 70, 69.666, 43750, 45500000, \
        40, 80, 120, 3, 4, 21, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_800X600P70 \
    { \
        800, 600, 70, 70, 43750, 45500000, \
        40, 80, 120, 1, 3, 21, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_800X600P72 \
    { \
        800, 600, 72, 72.188, 48077, 50000000, \
        56, 120, 64, 37, 6, 23, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_VESA_800X600P75 \
    { \
        800, 600, 75, 75, 46875, 75000000, \
        16, 80, 160, 1, 3, 21, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_VESA_800X600P85 \
    { \
        800, 600, 85, 85.061, 53674, 56250000, \
        32, 64, 152, 1, 3, 27, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_800X600P120_RB \
    { \
        800, 600, 120, 119.972, 76302, 73250000, \
        48, 32, 80, 3, 4, 29, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_832X624P75 \
    { \
        832, 624, 75, 74.836, 48943, 53250000, \
        48, 80, 128, 3, 4, 23, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_832X624P75 \
    { \
        832, 624, 75, 75, 48900, 53203201, \
        40, 88, 128, 1, 3, 24, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_848X480P60 \
    { \
        848, 480, 60, 60, 31020, 33750000, \
        16, 112, 112, 6, 8, 23, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_848X480P60 \
    { \
        848, 480, 60, 59.659, 29829, 31500000, \
        24, 80, 104, 3, 5, 12, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_848X480P60_RB \
    { \
        848, 480, 60, 59.745, 29513, 29750000, \
        48, 32, 80, 3, 5, 6, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_848X480P60 \
    { \
        848, 480, 60, 60, 29820, 31489921, \
        16, 88, 104, 1, 3, 13, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_848X480P70 \
    { \
        848, 480, 70, 69.545, 34981, 37500000, \
        32, 80, 112, 3, 5, 15, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_848X480P70 \
    { \
        848, 480, 70, 70, 35000, 37520000, \
        24, 88, 112, 1, 3, 16, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_848X480P71_9 \
    { \
        848, 480, 72, 71.864, 36147, 38750000, \
        32, 80, 112, 3, 5, 15, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_848X480P71_9 \
    { \
        848, 480, 72, 71.9, 36021, 39191829, \
        32, 88, 120, 1, 3, 17, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_848X480P75 \
    { \
        848, 480, 75, 74.769, 37683, 41000000, \
        40, 80, 120, 3, 5, 16, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_848X480P75 \
    { \
        848, 480, 75, 75, 37650, 40963199, \
        32, 88, 120, 1, 3, 18, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_848X480P85 \
    { \
        848, 480, 85, 84.751, 42968, 46750000, \
        40, 80, 120, 3, 5, 19, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_848X480P85 \
    { \
        848, 480, 85, 85, 42924, 47389194, \
        40, 88, 128, 1, 3, 21, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1024X768P60 \
    { \
        1024, 768, 60, 60.004, 48363, 65000000, \
        24, 136, 160, 3, 6, 29, POLARITY_NEG, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1024X768P60 \
    { \
        1024, 768, 60, 59.92, 47816, 63500000, \
        48, 104, 142, 3, 4, 23, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1024X768P60_RB \
    { \
        1024, 768, 60, 59.87, 47297, 56000000, \
        48, 32, 80, 3, 4, 15, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1024X768P60 \
    { \
        1024, 768, 60, 60, 47700, 64110000, \
        56, 104, 160, 1, 3, 23, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1024X768P70 \
    { \
        1024, 768, 70, 70.069, 56476, 75000000, \
        24, 136, 144, 3, 6, 29, POLARITY_NEG, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1024X768P72 \
    { \
        1024, 768, 72, 71.881, 57720, 78500000, \
        64, 104, 168, 3, 4, 28, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1024X768P72 \
    { \
        1024, 768, 72, 72, 57671, 78433914, \
        56, 112, 168, 1, 3, 29, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1024X768P75 \
    { \
        1024, 768, 75, 75.029, 60023, 78750000, \
        16, 96, 176, 1, 3, 28, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1024X768P75 \
    { \
        1024, 768, 75, 74.90, 60294, 82000000, \
        64, 104, 168, 3, 4, 30, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1024X768P75 \
    { \
        1024, 768, 75, 75, 60150, 81800000, \
        56, 112, 168, 1, 3, 30, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1024X768P85 \
    { \
        1024, 768, 85, 84.997, 68677, 94500000, \
        48, 96, 208, 1, 3, 36, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1024X768P120_RB \
    { \
        1024, 768, 120, 119.989, 97551, 115500000, \
        48, 32, 80, 3, 4, 38, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1152X864P60 \
    { \
        1152, 864, 60, 59.959, 53782, 81750000, \
        64, 120, 184, 3, 4, 26, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1152X864P60_RB \
    { \
        1152, 864, 60, 59.801, 53163, 69750000, \
        48, 32, 80, 3, 4, 18, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1152X864P60 \
    { \
	    1152, 864, 60, 60, 53700, 81624000, \
        64, 120, 184, 1, 3, 27, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1152X864P70 \
    { \
	    1152, 864, 70, 70, 62999, 96767997, \
        72, 120, 192, 1, 3, 32, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1152X864P75 \
    { \
        1152, 864, 75, 75, 67500, 108000000, \
        64, 128, 256, 1, 3, 32, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1152X864P75 \
    { \
        1152, 864, 75, 74.816, 67708, 104000000, \
        72, 120, 192, 3, 4, 34, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1152X864P75 \
    { \
        1152, 864, 75, 75, 67650, 104992805, \
        72, 128, 200, 1, 3, 34, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1152X870P75 \
    { \
        1152, 870, 75, 74.859, 68196, 104750000, \
        72, 120, 192, 3, 10, 28, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1152X870P75 \
    { \
        1152, 870, 75, 75, 68100, 105691207, \
        72, 128, 200, 1, 3, 34, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1152X900P66 \
    { \
        1152, 900, 66, 65.764, 61686, 94750000, \
        72, 120, 192, 3, 10, 25, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1152X900P66 \
    { \
        1152, 900, 66, 66, 61710, 94786567, \
        72, 120, 192, 1, 3, 31, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1152X900P76 \
    { \
        1152, 900, 76, 75.844, 71520, 111000000, \
        80, 120, 200, 3, 10, 30, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1152X900P76 \
    { \
        1152, 900, 76, 76, 71439, 110874877, \
        72, 128, 200, 1, 3, 36, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1176X664P60 \
    { \
        1176, 664, 60, 59.907, 41335, 62500000, \
        48, 120, 168, 3, 5, 18, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1176X664P60_RB \
    { \
        1176, 664, 60, 59.727, 40793, 54500000, \
        48, 32, 80, 3, 5, 11, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1176X664P60 \
    { \
        1176, 664, 60, 60, 41279, 63075836, \
        56, 120, 176, 1, 3, 20, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1280X720P50 \
    { \
        1280, 720, 50, 49.94, 37500, 74250000, \
        440, 40, 220, 5, 5, 20, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X720P50 \
    { \
        1280, 720, 50, 49.827, 37071, 60500000, \
        48, 128, 176, 3, 5, 16, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X720P50 \
    { \
        1280, 720, 50, 50, 37049, 60465599, \
        48, 128, 176, 1, 3, 17, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1280X720P60 \
    { \
        1280, 720, 60, 60, 45000, 74250000, \
        110, 40, 220, 5, 5, 20, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X720P60 \
    { \
        1280, 720, 60, 59.86, 44775, 74500000, \
        64, 128, 192, 3, 5, 20, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X720P60_RB \
    { \
        1280, 720, 60, 59.74, 44267, 63750000, \
        48, 32, 80, 3, 5, 13, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1280X720P60 \
    { \
        1280, 720, 60, 60, 44760, 74480000, \
        56, 136, 192, 1, 3, 22, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X720P72 \
    { \
        1280, 720, 72, 71.934, 54166, 91000000, \
        72, 128, 200, 3, 5, 25, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X720P72 \
    { \
        1280, 720, 72, 72, 54071, 91706108, \
        72, 136, 208, 1, 3, 27, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X720P75 \
    { \
        1280, 720, 75, 74.777, 56456, 95750000, \
        80, 128, 208, 3, 5, 27, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X720P75 \
    { \
        1280, 720, 75, 75, 56400, 95654403, \
        72, 136, 208, 1, 3, 28, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X768P60 \
    { \
        1280, 768, 60, 59.87, 47776, 79500000, \
        64, 128, 192, 3, 7, 20, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X768P60_RB \
    { \
        1280, 768, 60, 59.77, 47220, 68000000, \
        48, 32, 80, 3, 7, 12, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1280X768P60 \
    { \
        1280, 768, 60, 60, 47699, 80135993, \
        64, 136, 200, 1, 3, 23, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X768P72 \
    { \
        1280, 768, 72, 71.959, 57783, 98000000, \
        80, 128, 208, 3, 7, 25, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X768P72 \
    { \
        1280, 768, 72, 72, 57671, 97811706, \
        72, 136, 208, 1, 3, 29, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1280X768P75 \
    { \
        1280, 768, 75, 74.893, 60289, 102250000, \
        80, 128, 208, 3, 7, 27, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X768P75 \
    { \
        1280, 768, 75, 75, 60289, 102250000, \
        80, 128, 208, 3, 10, 24, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X768P75 \
    { \
        1280, 768, 75, 75, 60150, 102980000, \
        80, 136, 216, 1, 3, 30, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1280X768P85 \
    { \
        1280, 768, 85, 84.837, 68633, 117500000, \
        80, 136, 216, 3, 7, 31, POLARITY_NEG, POLARITY_POS, \
    } 

#define BT_TIMING_CVT_1280X768P120_RB \
    { \
        1280, 768, 120, 119.798, 97396, 140250000, \
        48, 32, 80, 3, 7, 35, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1280X800P60 \
    { \
        1280, 800, 60, 59.81, 49702, 83500000, \
        72, 128, 200, 3, 6, 22, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X800P60_RB \
    { \
        1280, 800, 60, 59.910, 49306, 71000000, \
        48, 32, 80, 3, 6, 14, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1280X800P60 \
    { \
        1280, 800, 60, 60, 49680, 83462402, \
        64, 136, 200, 1, 3, 24, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X800P70 \
    { \
        1280, 800, 70, 69.824, 58372, 99000000, \
        80, 128, 208, 3, 6, 27, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X800P70 \
    { \
        1280, 800, 70, 70, 58309, 98893760, \
        72, 136, 208, 1, 3, 29, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X800P72 \
    { \
        1280, 800, 72, 71.854, 60141, 102000000, \
        80, 128, 208, 3, 6, 28, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X800P72 \
    { \
        1280, 800, 72, 72, 60047, 102802169, \
        80, 136, 216, 1, 3, 30, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X800P75 \
    { \
        1280, 800, 75, 74.934, 62794, 106500000, \
        80, 128, 208, 3, 6, 29, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X800P75 \
    { \
        1280, 800, 75, 75, 62625, 107214004, \
        80, 136, 216, 1, 3, 31, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1280X800P85 \
    { \
        1280, 800, 85, 84.880, 71554, 122500000, \
        80, 136, 216, 3, 6, 34, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X800P120_RB \
    { \
        1280, 800, 120, 119.909, 101563, 146250000, \
        48, 32, 80, 3, 6, 38, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_VESA_1280X960P60 \
    { \
        1280, 960, 60, 60, 60000, 108000000, \
        96, 112, 312, 1, 3, 36, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1280X960P85 \
    { \
        1280, 960, 85, 85.002, 85938, 148500000, \
        64, 160, 224, 1, 3, 47, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X960P120_RB \
    { \
        1280, 960, 120, 119.838, 121875, 175500000, \
        48, 32, 80, 3, 4, 50, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_VESA_1280X1024P60 \
    { \
        1280, 1024, 60, 60.02, 63981, 108000000, \
        48, 112, 248, 1, 3, 38, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X1024P60 \
    { \
        1280, 1024, 60, 59.89, 63663, 109000000, \
        88, 128, 216, 3, 7, 29, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X1024P60_RB \
    { \
        1280, 1024, 60, 59.79, 63018, 90750000, \
        48, 32, 80, 3, 7, 20, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1280X1024P60 \
    { \
        1280, 1024, 60, 60, 63600, 108880000, \
        80, 136, 216, 1, 3, 32, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X1024P70 \
    { \
        1280, 1024, 70, 69.83, 74650, 129000000, \
        88, 136, 224, 3, 7, 35, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X1024P70_RB \
    { \
        1280, 1024, 70, 69.86, 73980, 106500000, \
        48, 32, 80, 3, 7, 25, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1280X1024P70 \
    { \
        1280, 1024, 70, 70, 74620, 128940000, \
        88, 136, 224, 1, 3, 38, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X1024P72 \
    { \
        1280, 1024, 72, 71.932, 76967, 133000000, \
        88, 136, 224, 3, 7, 36, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X1024P72 \
    { \
        1280, 1024, 72, 72, 76823, 132751876, \
        88, 136, 224, 1, 3, 39, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1280X1024P75 \
    { \
        1280, 1024, 75, 75.025, 79976, 135000000, \
        16, 144, 248, 1, 3, 38, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X1024P75 \
    { \
        1280, 1024, 75, 74.9, 80292, 138750000, \
        88, 136, 224, 3, 7, 38, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X1024P75 \
    { \
        1280, 1024, 75, 75, 80175, 138540000, \
        88, 136, 224, 1, 3, 41, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X1024P76 \
    { \
        1280, 1024, 76, 75.982, 81452, 140750000, \
        88, 136, 224, 3, 7, 38, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1280X1024P76 \
    { \
        1280, 1024, 76, 76, 81319, 141822067, \
        96, 136, 232, 1, 3, 42, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1280X1024P85 \
    { \
        1280, 1024, 85, 85.024, 91146, 157500000, \
        64, 160, 224, 1, 3, 44, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1280X1024P120_RB \
    { \
        1280, 1024, 120, 119.958, 130035, 187250000, \
        48, 32, 80, 3, 7, 50, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_VESA_1360X768P60 \
    { \
        1360, 768, 60, 60.015, 47712, 85500000, \
        64, 112, 256, 3, 6, 18, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1360X768P120_RB \
    { \
        1360, 768, 120, 119.967, 97533, 148250000, \
        48, 32, 80, 3, 5, 37, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_VESA_1366X768P60 \
    { \
        1366, 768, 60, 59.79, 47712, 85500000, \
        70, 143, 213, 3, 3, 24, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1366X768P60_RB \
    { \
        1366, 768,  60, 60, 48000, 72000000, \
        14, 56, 64, 1, 3, 28, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1400X1050P60 \
    { \
        1400, 1050,  60, 59.978, 65317, 121750000, \
        88, 144, 232, 3, 4, 32, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1400X1050P60_RB \
    { \
        1400, 1050, 60, 59.948, 64744, 101000000, \
        48, 32, 80, 3, 4, 23, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1400X1050P60 \
    { \
        1400, 1050, 60, 60, 65220, 122610000, \
        88, 152, 240, 1, 3, 33, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1400X1050P75 \
    { \
        1400, 1050, 75, 74.867, 82278, 156000000, \
        104, 144, 248, 3, 4, 42, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1400X1050P75 \
    { \
        1400, 1050, 75, 75, 82199, 155851196, \
        96, 152, 248, 1, 3, 42, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1400X1050P85 \
    { \
        1400, 1050, 85, 84.960, 93881, 179500000, \
        104, 152, 256, 3, 4, 48, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1400X1050P120_RB \
    { \
        1400, 1050, 120, 119904, 133333, 208000000, \
        48, 32, 80, 3, 4, 55, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1440X900P60 \
    { \
        1440, 900, 60, 59.887, 55935, 106500000, \
        80, 152, 232, 3, 6, 25, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1440X900P60_RB \
    { \
        1440, 900, 60, 59.901, 55469, 88750000, \
        48, 32, 80, 3, 6, 17, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1440X900P60 \
    { \
        1440, 900, 60, 60, 55920, 106470000, \
        80, 152, 232, 1, 3, 28, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1440X900P75 \
    { \
        1440, 900, 75, 74.984, 70635, 136750000, \
        96, 152, 248, 3, 6, 33, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1440X900P75 \
    { \
        1440, 900, 75, 75, 70500, 136488006, \
        96, 152, 248, 1, 3, 36, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1440X900P85 \
    { \
        1440, 900, 85, 84.842, 80430, 157000000, \
        104, 152, 256, 3, 6, 39, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1440X900P120_RB \
    { \
        1440, 900, 120, 119.852, 114219, 182750000, \
        48, 32, 80, 3, 6, 44, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1600X900P50 \
    { \
        1600, 900, 50, 49.940, 46394, 96500000, \
        80, 160, 240, 3, 5, 21, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1600X900P50 \
    { \
        1600, 900, 50, 50, 46299, 97044800, \
        80, 168, 248, 1, 3, 22, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X900P60 \
    { \
        1600, 900, 60, 59.946, 55989, 118250000, \
        88, 168, 256, 3, 5, 26, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X900P60_RB \
    { \
        1600, 900, 60, 59.978, 55540, 97750000, \
        48, 32, 80, 3, 5, 18, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1600X900P60 \
    { \
        1600, 900, 60, 60, 55919, 118997756, \
        96, 168, 264, 1, 3, 28, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X1000P60 \
    { \
        1600, 1000, 60, 59.872, 62147, 132250000, \
        96, 168, 264, 3, 6, 29, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X1000P60_RB \
    { \
        1600, 1000, 60, 59.910, 61647, 108500000, \
        48, 32, 80, 3, 6, 20, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1600X1000P60 \
    { \
        1600, 1000, 60, 60, 62099, 133142395, \
        104, 168, 272, 1, 3, 31, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X1000P75 \
    { \
        1600, 1000, 75, 74.839, 78356, 169250000, \
        112, 168, 280, 3, 6, 38, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1600X1000P75 \
    { \
        1600, 1000, 75, 75, 78300, 169128005, \
        104, 176, 280, 1, 3, 40, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1600X1200P60 \
    { \
        1600, 1200, 60, 60, 75000, 162000000, \
        64, 192, 304, 1, 3, 46, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X1200P60 \
    { \
        1600, 1200, 60, 59.87, 74538, 161000000, \
        112, 168, 280, 3, 4, 38, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X1200P60_RB \
    { \
        1600, 1200, 60, 59.92, 74001, 130250000, \
        48, 32, 80, 3, 4, 28, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1600X1200P60 \
    { \
        1600, 1200, 60, 60, 74520, 160960000, \
        104, 176, 280, 1, 3, 38, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1600X1200P65 \
    { \
        1600, 1200, 65, 65, 81250, 175500000, \
        64, 192, 304, 1, 3, 46, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X1200P65 \
    { \
        1600, 1200, 65, 64.919, 81018, 175000000, \
        112, 168, 280, 3, 4, 41, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1600X1200P65 \
    { \
        1600, 1200, 65, 65, 80990, 176234252, \
        112, 176, 288, 1, 3, 42, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1600X1200P70 \
    { \
        1600, 1200, 70, 70, 87500, 189000000, \
        64, 192, 304, 1, 3, 46, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X1200P70 \
    { \
        1600, 1200, 70, 69.925, 87545, 190500000, \
        120, 168, 288, 3, 4, 45, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1600X1200P70 \
    { \
        1600, 1200, 70, 70, 87429, 190250000, \
        112, 176, 288, 1, 3, 45, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1600X1200P75 \
    { \
        1600, 1200, 75, 75, 92750, 202500000, \
        64, 192, 304, 1, 3, 46, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X1200P75 \
    { \
        1600, 1200, 75, 74.976, 94094, 204750000, \
        120, 168, 288, 3, 4, 48, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1600X1200P75 \
    { \
        1600, 1200, 75, 75, 93974, 205993194, \
        120, 176, 296, 1, 3, 49, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1600X1200P85 \
    { \
        1600, 1200, 85, 85, 106250, 229500000, \
        64, 192, 304, 1, 3, 46, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X1200P120_RB \
    { \
        1600, 1200, 120, 119.917, 152415, 268250000, \
        48, 32, 80, 3, 4, 64, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1600X1280P60 \
    { \
        1600, 1280, 60, 59.920, 79513, 171750000, \
        112, 168, 280, 3, 7, 37, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1600X1280P60_RB \
    { \
        1600, 1280, 60, 59.968, 78977, 139000000, \
        48, 32, 80, 3, 7, 27, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1600X1280P60 \
    { \
        1600, 1280, 60, 60, 79500, 172992004, \
        112, 176, 288, 1, 3, 41, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1680X1050P60 \
    { \
        1680, 1050,  60, 59.954, 65290, 146250000, \
        104, 176, 280, 3, 6, 30, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1680X1050P60_RB \
    { \
        1680, 1050, 60, 59.883, 64674, 119000000, \
        48, 32, 80, 3, 6, 21, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1680X1050P60 \
    { \
        1680, 1050, 60, 60, 65220, 147140000, \
        104, 184, 288, 1, 3, 33, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1680X1050P75 \
    { \
        1680, 1050, 75, 74.892, 82306, 187000000, \
        120, 176, 296, 3, 6, 40, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1680X1050P75 \
    { \
        1680, 1050, 75, 74.999, 81599, 143615005, \
        8, 32, 40, 24, 8, 6, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1680X1050P85 \
    { \
        1680, 1050, 85, 85.941, 93859, 214750000, \
        128, 176, 304, 3, 6, 46, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1680X1050P120_RB \
    { \
        1680, 1050, 120, 119.986, 133424, 245500000, \
        48, 32, 80, 3, 6, 53, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_VESA_1792X1344P60 \
    { \
        1792, 1344, 60, 60, 83640, 204750000, \
        128, 200, 328, 1, 3, 46, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1792X1344P75 \
    { \
        1792, 1344, 75, 74.997, 106270, 261000000, \
        96, 216, 352, 1, 3, 69, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1792X1344P120_RB \
    { \
        1792, 1344, 120, 119.974, 172722, 333250000, \
        48, 32, 80, 3, 4, 72, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_VESA_1856X1392P60 \
    { \
        1856, 1392, 60, 59.995, 86333, 218250000, \
        96, 224, 352, 1, 3, 43, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1856X1392P60 \
    { \
        1856, 1392, 60, 59.934, 86484, 217250000, \
        128, 200, 328, 3, 4, 44, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1856X1392P60_RB \
    { \
        1856, 1392, 60, 59.926, 85813, 173000000, \
        48, 32, 80, 3, 4, 33, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1856X1392P60 \
    { \
        1856, 1392, 60, 60, 86459, 218570880, \
        136, 200, 336, 1, 3, 45, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1856X1392P75 \
    { \
        1856, 1392, 75, 75, 112500, 288000000, \
        128, 224, 352, 1, 3, 104, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1856X1392P120_RB \
    { \
        1856, 1392, 120, 119.970, 176835, 356500000, \
        48, 32, 80, 3, 4, 75, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_VESA_1920X1080P50 \
    { \
        1920, 1080, 50, 50, 56000, 148500000, \
        528, 44, 148, 4, 5, 36, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1080P50 \
    { \
        1920, 1080, 50, 49.93, 55622, 141500000, \
        112, 200, 312, 3, 5, 26, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1920X1080P50 \
    { \
        1920, 1080, 50, 50, 55600, 141450000, \
        112, 200, 312, 1, 3, 28, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1920X1080P60 \
    { \
        1920, 1080, 60, 60, 67500, 148500000, \
        88, 44, 148, 4, 5, 36, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1080P60 \
    { \
        1920, 1080, 60, 59.96, 67155, 173000000, \
        128, 200, 328, 3, 5, 32, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1080P60_RB \
    { \
        1920, 1080, 60, 59.93, 66582, 138500000, \
        48, 32, 80, 3, 5, 23, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1920X1080P60 \
    { \
        1920, 1080, 60, 60, 67080, 172800000, \
        120, 208, 328, 1, 3, 34, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1080P72 \
    { \
        1920, 1080, 72, 71.910, 81114, 210250000, \
        136, 200, 336, 3, 5, 40, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1080P72_R2 \
    { \
        1920, 1080, 72, 72, 80424, 160848000, \
        8, 32, 40, 23, 8, 6, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1920X1080P72 \
    { \
        1920, 1080, 72, 72, 81071, 211435760, \
        136, 208, 344, 1, 3, 42, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1080P75 \
    { \
        1920, 1080, 75, 74.906, 84643, 220750000, \
        136, 208, 344, 3, 5, 42, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_GTF_1920X1080P75 \
    { \
        1920, 1080, 75, 75, 84600, 220636810, \
        136, 208, 344, 1, 3, 44, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1920X1080P90 \
    { \
        1920, 1080, 90, 90, 101250, 222750000, \
        88, 44, 148, 4, 5, 36, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_VESA_1920X1080P100 \
    { \
        1920, 1080, 100, 100, 112500, 247500000, \
        88, 44, 148, 4, 5, 36, POLARITY_POS, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1080P120_RB \
    { \
        1920, 1080, 120, 119.88, 137.14, 285250000, \
        48, 32, 80, 3, 5, 56, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1920X1080P144_RB \
    { \
        1920, 1080, 144, 144.000, 166.61, 346540000, \
        48, 32, 80, 3, 5, 69, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1200P50 \
    { \
        1920, 1200, 50, 49.932, 61816, 158250000, \
        120, 200, 320, 3, 6, 29, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1200P50_R2 \
    { \
        1920, 1200, 50, 50.000, 61449, 122899002, \
        8, 32, 40, 15, 8, 6, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1920X1200P50 \
    { \
        1920, 1200, 50, 50, 61750, 158080001, \
        112, 208, 320, 1, 3, 31, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1200P60 \
    { \
        1920, 1200, 60, 59.885, 74556, 193250000, \
        136, 200, 336, 3, 6, 36, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1920X1200P60_RB \
    { \
        1920, 1200, 60, 59.950, 74038, 154000000, \
        48, 32, 80, 3, 6, 26, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1920X1200P60 \
    { \
        1920, 1200, 60, 60, 74520, 193155838, \
        128, 208, 336, 1, 3, 38, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1200P75 \
    { \
        1920, 1200, 75, 74.930, 94038, 245250000, \
        136, 208, 344, 3, 6, 46, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1200P85 \
    { \
        1920, 1200, 85, 84.932, 107184, 281250000, \
        144, 208, 352, 3, 6, 53, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1200P120_RB \
    { \
        1920, 1200, 120, 119.909, 152404, 317000000, \
        48, 32, 80, 3, 6, 62, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1920X1200P144_RB \
    { \
        1920, 1200, 144, 144.000, 185.18, 385180000, \
        48, 32, 80, 3, 6, 77, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1440P60 \
    { \
        1920, 1440, 60, 59.968, 89532, 233500000, \
        136, 208, 344, 3, 4, 46, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1440P60_RB \
    { \
        1920, 1440, 60, 59.974, 88822, 184750000, \
        48, 32, 80, 3, 4, 34, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_GTF_1920X1440P60 \
    { \
        1920, 1440, 60, 59.974, 88822, 184750000, \
        48, 32, 80, 3, 4, 34, POLARITY_POS, POLARITY_NEG, \
    }

#define BT_TIMING_CVT_1920X1440P75_RB \
    { \
        1920, 1440, 75, 75.000, 111.90, 232750000, \
        48, 32, 80, 3, 4, 45, POLARITY_NEG, POLARITY_POS, \
    }

#define BT_TIMING_CVT_1920X1440P120_RB \
    { \
        1920, 1440, 120, 120.000, 183.00, 380640000, \
        48, 32, 80, 3, 4, 78, POLARITY_NEG, POLARITY_POS,\
    }

#define BT_TIMING_CVT_1920X1440P144_RB \
    { \
        1920, 1440, 144, 144.000, 222.19, 462160000, \
        48, 32, 80, 3, 4, 96, POLARITY_NEG, POLARITY_POS,\
    }

#define BT_TIMING_BUTT \
    { \
        0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, POLARITY_BUTT, POLARITY_BUTT, \
    }

/* 时序集合 */
const BT_TIMING_MAP_S gastTimingMap[] = {
    [TIMING_CVT_640X350P50]         = {TIMING_CVT_640X350P50,           BT_TIMING_CVT_640X350P50,       "CVT_640X350P50"},
    [TIMING_VESA_640X350P85]        = {TIMING_VESA_640X350P85,          BT_TIMING_VESA_640X350P85,      "VESA_640X350P85"},
    [TIMING_VESA_640X400P85]        = {TIMING_VESA_640X400P85,          BT_TIMING_VESA_640X400P85,      "VESA_640X400P85"},
    [TIMING_VESA_640X480P60]        = {TIMING_VESA_640X480P60,          BT_TIMING_VESA_640X480P60,      "VESA_640X480P60"},
    [TIMING_CVT_640X480P60]         = {TIMING_CVT_640X480P60,           BT_TIMING_CVT_640X480P60,       "CVT_640X480P60"},
    [TIMING_CVT_640X480P60_RB]      = {TIMING_CVT_640X480P60_RB,        BT_TIMING_CVT_640X480P60_RB,    "CVT_640X480P60_RB"},
    [TIMING_GTF_640X480P60]         = {TIMING_GTF_640X480P60,           BT_TIMING_GTF_640X480P60,       "GTF_640X480P60"},
    [TIMING_VESA_640X480P72]        = {TIMING_VESA_640X480P72,          BT_TIMING_VESA_640X480P72,      "VESA_640X480P72"},
    [TIMING_CVT_640X480P73]         = {TIMING_CVT_640X480P73,           BT_TIMING_CVT_640X480P73,       "CVT_640X480P73"},
    [TIMING_GTF_640X480P73]         = {TIMING_GTF_640X480P73,           BT_TIMING_GTF_640X480P73,       "GTF_640X480P73"},
    [TIMING_VESA_640X480P75]        = {TIMING_VESA_640X480P75,          BT_TIMING_VESA_640X480P75,      "VESA_640X480P75"},
    [TIMING_VESA_640X480P85]        = {TIMING_VESA_640X480P85,          BT_TIMING_VESA_640X480P85,      "VESA_640X480P85"},
    [TIMING_CVT_640X500P58]         = {TIMING_CVT_640X500P58,           BT_TIMING_CVT_640X500P58,       "CVT_640X500P58"},
    [TIMING_GTF_640X500P58]         = {TIMING_GTF_640X500P58,           BT_TIMING_GTF_640X500P58,       "GTF_640X500P58"},
    [TIMING_VESA_720X400P85]        = {TIMING_VESA_720X400P85,          BT_TIMING_VESA_720X400P85,      "VESA_720X400P85"},
    [TIMING_CVT_720X480P60]         = {TIMING_CVT_720X480P60,           BT_TIMING_CVT_720X480P60,       "CVT_720X480P60"},
    [TIMING_CVT_720X480P60_RB]      = {TIMING_CVT_720X480P60_RB,        BT_TIMING_CVT_720X480P60_RB,    "CVT_720X480P60_RB"},
    [TIMING_GTF_720X480P60]         = {TIMING_GTF_720X480P60,           BT_TIMING_GTF_720X480P60,       "GTF_720X480P60"},
    [TIMING_CVT_720X576P50]         = {TIMING_CVT_720X576P50,           BT_TIMING_CVT_720X576P50,       "CVT_720X576P50"},
    [TIMING_GTF_720X576P50]         = {TIMING_GTF_720X576P50,           BT_TIMING_GTF_720X576P50,       "GTF_720X576P50"},
    [TIMING_CVT_720X576P60]         = {TIMING_CVT_720X576P60,           BT_TIMING_CVT_720X576P60,       "CVT_720X576P60"},
    [TIMING_CVT_720X576P60_RB]      = {TIMING_CVT_720X576P60_RB,        BT_TIMING_CVT_720X576P60_RB,    "CVT_720X576P60_RB"},
    [TIMING_GTF_720X576P60]         = {TIMING_GTF_720X576P60,           BT_TIMING_GTF_720X576P60,       "GTF_720X576P60"},
    [TIMING_VESA_800X600P56]        = {TIMING_VESA_800X600P56,          BT_TIMING_VESA_800X600P56,      "VESA_800X600P56"},
    [TIMING_VESA_800X600P60]        = {TIMING_VESA_800X600P60,          BT_TIMING_VESA_800X600P60,      "VESA_800X600P60"},
    [TIMING_CVT_800X600P60]         = {TIMING_CVT_800X600P60,           BT_TIMING_CVT_800X600P60,       "CVT_800X600P60"},
    [TIMING_CVT_800X600P60_RB]      = {TIMING_CVT_800X600P60_RB,        BT_TIMING_CVT_800X600P60_RB,    "CVT_800X600P60_RB"},
    [TIMING_GTF_800X600P60]         = {TIMING_GTF_800X600P60,           BT_TIMING_GTF_800X600P60,       "GTF_800X600P60"},
    [TIMING_CVT_800X600P67]         = {TIMING_CVT_800X600P67,           BT_TIMING_CVT_800X600P67,       "CVT_800X600P67"},
    [TIMING_GTF_800X600P67]         = {TIMING_GTF_800X600P67,           BT_TIMING_GTF_800X600P67,       "GTF_800X600P67"},
    [TIMING_CVT_800X600P70]         = {TIMING_CVT_800X600P70,           BT_TIMING_CVT_800X600P70,       "CVT_800X600P70"},
    [TIMING_GTF_800X600P70]         = {TIMING_GTF_800X600P70,           BT_TIMING_GTF_800X600P70,       "GTF_800X600P70"},
    [TIMING_VESA_800X600P72]        = {TIMING_VESA_800X600P72,          BT_TIMING_VESA_800X600P72,      "VESA_800X600P72"},
    [TIMING_VESA_800X600P75]        = {TIMING_VESA_800X600P75,          BT_TIMING_VESA_800X600P75,      "VESA_800X600P75"},
    [TIMING_VESA_800X600P85]        = {TIMING_VESA_800X600P85,          BT_TIMING_VESA_800X600P85,      "VESA_800X600P85"},
    [TIMING_CVT_800X600P120_RB]     = {TIMING_CVT_800X600P120_RB,       BT_TIMING_CVT_800X600P120_RB,   "CVT_800X600P120_RB"},
    [TIMING_CVT_832X624P75]         = {TIMING_CVT_832X624P75,           BT_TIMING_CVT_832X624P75,       "CVT_832X624P75"},
    [TIMING_GTF_832X624P75]         = {TIMING_GTF_832X624P75,           BT_TIMING_GTF_832X624P75,       "GTF_832X624P75"},
    [TIMING_VESA_848X480P60]        = {TIMING_VESA_848X480P60,          BT_TIMING_VESA_848X480P60,      "VESA_848X480P60"},
    [TIMING_CVT_848X480P60]         = {TIMING_CVT_848X480P60,           BT_TIMING_CVT_848X480P60,       "CVT_848X480P60"},
    [TIMING_CVT_848X480P60_RB]      = {TIMING_CVT_848X480P60_RB,        BT_TIMING_CVT_848X480P60_RB,    "CVT_848X480P60_RB"},
    [TIMING_GTF_848X480P60]         = {TIMING_GTF_848X480P60,           BT_TIMING_GTF_848X480P60,       "GTF_848X480P60"},
    [TIMING_CVT_848X480P70]         = {TIMING_CVT_848X480P70,           BT_TIMING_CVT_848X480P70,       "CVT_848X480P70"},
    [TIMING_GTF_848X480P70]         = {TIMING_GTF_848X480P70,           BT_TIMING_GTF_848X480P70,       "GTF_848X480P70"},
    [TIMING_CVT_848X480P71_9]       = {TIMING_CVT_848X480P71_9,         BT_TIMING_CVT_848X480P71_9,     "CVT_848X480P71_9"},
    [TIMING_GTF_848X480P71_9]       = {TIMING_GTF_848X480P71_9,         BT_TIMING_GTF_848X480P71_9,     "GTF_848X480P71_9"},
    [TIMING_CVT_848X480P75]         = {TIMING_CVT_848X480P75,           BT_TIMING_CVT_848X480P75,       "CVT_848X480P75"},
    [TIMING_GTF_848X480P75]         = {TIMING_GTF_848X480P75,           BT_TIMING_GTF_848X480P75,       "GTF_848X480P75"},
    [TIMING_CVT_848X480P85]         = {TIMING_CVT_848X480P85,           BT_TIMING_CVT_848X480P85,       "CVT_848X480P85"},
    [TIMING_GTF_848X480P85]         = {TIMING_GTF_848X480P85,           BT_TIMING_GTF_848X480P85,       "GTF_848X480P85"},
    [TIMING_VESA_1024X768P60]       = {TIMING_VESA_1024X768P60,         BT_TIMING_VESA_1024X768P60,     "VESA_1024X768P60"},
    [TIMING_CVT_1024X768P60]        = {TIMING_CVT_1024X768P60,          BT_TIMING_CVT_1024X768P60,      "CVT_1024X768P60"},
    [TIMING_CVT_1024X768P60_RB]     = {TIMING_CVT_1024X768P60_RB,       BT_TIMING_CVT_1024X768P60_RB,   "CVT_1024X768P60_RB"},
    [TIMING_GTF_1024X768P60]        = {TIMING_GTF_1024X768P60,          BT_TIMING_GTF_1024X768P60,      "GTF_1024X768P60"},
    [TIMING_VESA_1024X768P70]       = {TIMING_VESA_1024X768P70,         BT_TIMING_VESA_1024X768P70,     "VESA_1024X768P70"},
    [TIMING_CVT_1024X768P72]        = {TIMING_CVT_1024X768P72,          BT_TIMING_CVT_1024X768P72,      "CVT_1024X768P72"},
    [TIMING_GTF_1024X768P72]        = {TIMING_GTF_1024X768P72,          BT_TIMING_GTF_1024X768P72,      "GTF_1024X768P72"},
    [TIMING_VESA_1024X768P75]       = {TIMING_VESA_1024X768P75,         BT_TIMING_VESA_1024X768P75,     "VESA_1024X768P75"},
    [TIMING_CVT_1024X768P75]        = {TIMING_CVT_1024X768P75,          BT_TIMING_CVT_1024X768P75,      "CVT_1024X768P75"},
    [TIMING_GTF_1024X768P75]        = {TIMING_GTF_1024X768P75,          BT_TIMING_GTF_1024X768P75,      "GTF_1024X768P75"},
    [TIMING_VESA_1024X768P85]       = {TIMING_VESA_1024X768P85,         BT_TIMING_VESA_1024X768P85,     "VESA_1024X768P85"},
    [TIMING_CVT_1024X768P120_RB]    = {TIMING_CVT_1024X768P120_RB,      BT_TIMING_CVT_1024X768P120_RB,  "CVT_1024X768P120_RB"},
    [TIMING_CVT_1152X864P60]        = {TIMING_CVT_1152X864P60,          BT_TIMING_CVT_1152X864P60,      "CVT_1152X864P60"},
    [TIMING_CVT_1152X864P60_RB]     = {TIMING_CVT_1152X864P60_RB,       BT_TIMING_CVT_1152X864P60_RB,   "CVT_1152X864P60_RB"},
    [TIMING_GTF_1152X864P60]        = {TIMING_GTF_1152X864P60,          BT_TIMING_GTF_1152X864P60,      "GTF_1152X864P60"},
    [TIMING_GTF_1152X864P70]        = {TIMING_GTF_1152X864P70,          BT_TIMING_GTF_1152X864P70,      "GTF_1152X864P70"},
    [TIMING_VESA_1152X864P75]       = {TIMING_VESA_1152X864P75,         BT_TIMING_VESA_1152X864P75,     "VESA_1152X864P75"},
    [TIMING_CVT_1152X864P75]        = {TIMING_CVT_1152X864P75,          BT_TIMING_CVT_1152X864P75,      "CVT_1152X864P75"},
    [TIMING_GTF_1152X864P75]        = {TIMING_GTF_1152X864P75,          BT_TIMING_GTF_1152X864P75,      "GTF_1152X864P75"},
    [TIMING_CVT_1152X870P75]        = {TIMING_CVT_1152X870P75,          BT_TIMING_CVT_1152X870P75,      "CVT_1152X870P75"},
    [TIMING_GTF_1152X870P75]        = {TIMING_GTF_1152X870P75,          BT_TIMING_GTF_1152X870P75,      "GTF_1152X870P75"},
    [TIMING_CVT_1152X900P66]        = {TIMING_CVT_1152X900P66,          BT_TIMING_CVT_1152X900P66,      "CVT_1152X900P66"},
    [TIMING_GTF_1152X900P66]        = {TIMING_GTF_1152X900P66,          BT_TIMING_GTF_1152X900P66,      "GTF_1152X900P66"},
    [TIMING_CVT_1152X900P76]        = {TIMING_CVT_1152X900P76,          BT_TIMING_CVT_1152X900P76,      "CVT_1152X900P76"},
    [TIMING_GTF_1152X900P76]        = {TIMING_GTF_1152X900P76,          BT_TIMING_GTF_1152X900P76,      "GTF_1152X900P76"},
    [TIMING_CVT_1176X664P60]        = {TIMING_CVT_1176X664P60,          BT_TIMING_CVT_1176X664P60,      "CVT_1176X664P60"},
    [TIMING_CVT_1176X664P60_RB]     = {TIMING_CVT_1176X664P60_RB,       BT_TIMING_CVT_1176X664P60_RB,   "CVT_1176X664P60_RB"},
    [TIMING_GTF_1176X664P60]        = {TIMING_GTF_1176X664P60,          BT_TIMING_GTF_1176X664P60,      "GTF_1176X664P60"},
    [TIMING_VESA_1280X720P50]       = {TIMING_VESA_1280X720P50,         BT_TIMING_VESA_1280X720P50,     "VESA_1280X720P50"},
    [TIMING_CVT_1280X720P50]        = {TIMING_CVT_1280X720P50,          BT_TIMING_CVT_1280X720P50,      "CVT_1280X720P50"},
    [TIMING_GTF_1280X720P50]        = {TIMING_GTF_1280X720P50,          BT_TIMING_GTF_1280X720P50,      "GTF_1280X720P50"},
    [TIMING_VESA_1280X720P60]       = {TIMING_VESA_1280X720P60,         BT_TIMING_VESA_1280X720P60,     "VESA_1280X720P60"},
    [TIMING_CVT_1280X720P60]        = {TIMING_CVT_1280X720P60,          BT_TIMING_CVT_1280X720P60,      "CVT_1280X720P60"},
    [TIMING_CVT_1280X720P60_RB]     = {TIMING_CVT_1280X720P60_RB,       BT_TIMING_CVT_1280X720P60_RB,   "CVT_1280X720P60_RB"},
    [TIMING_GTF_1280X720P60]        = {TIMING_GTF_1280X720P60,          BT_TIMING_GTF_1280X720P60,      "GTF_1280X720P60"},
    [TIMING_CVT_1280X720P72]        = {TIMING_CVT_1280X720P72,          BT_TIMING_CVT_1280X720P72,      "CVT_1280X720P72"},
    [TIMING_GTF_1280X720P72]        = {TIMING_GTF_1280X720P72,          BT_TIMING_GTF_1280X720P72,      "GTF_1280X720P72"},
    [TIMING_CVT_1280X720P75]        = {TIMING_CVT_1280X720P75,          BT_TIMING_CVT_1280X720P75,      "CVT_1280X720P75"},
    [TIMING_GTF_1280X720P75]        = {TIMING_GTF_1280X720P75,          BT_TIMING_GTF_1280X720P75,      "GTF_1280X720P75"},
    [TIMING_CVT_1280X768P60]        = {TIMING_CVT_1280X768P60,          BT_TIMING_CVT_1280X768P60,      "CVT_1280X768P60"},
    [TIMING_CVT_1280X768P60_RB]     = {TIMING_CVT_1280X768P60_RB,       BT_TIMING_CVT_1280X768P60_RB,   "CVT_1280X768P60_RB"},
    [TIMING_GTF_1280X768P60]        = {TIMING_GTF_1280X768P60,          BT_TIMING_GTF_1280X768P60,      "GTF_1280X768P60"},
    [TIMING_CVT_1280X768P72]        = {TIMING_CVT_1280X768P72,          BT_TIMING_CVT_1280X768P72,      "CVT_1280X768P72"},
    [TIMING_GTF_1280X768P72]        = {TIMING_GTF_1280X768P72,          BT_TIMING_GTF_1280X768P72,      "GTF_1280X768P72"},
    [TIMING_VESA_1280X768P75]       = {TIMING_VESA_1280X768P75,         BT_TIMING_VESA_1280X768P75,     "VESA_1280X768P75"},
    [TIMING_CVT_1280X768P75]        = {TIMING_CVT_1280X768P75,          BT_TIMING_CVT_1280X768P75,      "CVT_1280X768P75"},
    [TIMING_GTF_1280X768P75]        = {TIMING_GTF_1280X768P75,          BT_TIMING_GTF_1280X768P75,      "GTF_1280X768P75"},
    [TIMING_VESA_1280X768P85]       = {TIMING_VESA_1280X768P85,         BT_TIMING_VESA_1280X768P85,     "VESA_1280X768P85"},
    [TIMING_CVT_1280X768P120_RB]    = {TIMING_CVT_1280X768P120_RB,      BT_TIMING_CVT_1280X768P120_RB,  "CVT_1280X768P120_RB"},
    [TIMING_CVT_1280X800P60]        = {TIMING_CVT_1280X800P60,          BT_TIMING_CVT_1280X800P60,      "CVT_1280X800P60"},
    [TIMING_CVT_1280X800P60_RB]     = {TIMING_CVT_1280X800P60_RB,       BT_TIMING_CVT_1280X800P60_RB,   "CVT_1280X800P60_RB"},
    [TIMING_GTF_1280X800P60]        = {TIMING_GTF_1280X800P60,          BT_TIMING_GTF_1280X800P60,      "GTF_1280X800P60"},
    [TIMING_CVT_1280X800P70]        = {TIMING_CVT_1280X800P70,          BT_TIMING_CVT_1280X800P70,      "CVT_1280X800P70"},
    [TIMING_GTF_1280X800P70]        = {TIMING_GTF_1280X800P70,          BT_TIMING_GTF_1280X800P70,      "GTF_1280X800P70"},
    [TIMING_CVT_1280X800P72]        = {TIMING_CVT_1280X800P72,          BT_TIMING_CVT_1280X800P72,      "CVT_1280X800P72"},
    [TIMING_GTF_1280X800P72]        = {TIMING_GTF_1280X800P72,          BT_TIMING_GTF_1280X800P72,      "GTF_1280X800P72"},
    [TIMING_CVT_1280X800P75]        = {TIMING_CVT_1280X800P75,          BT_TIMING_CVT_1280X800P75,      "CVT_1280X800P75"},
    [TIMING_GTF_1280X800P75]        = {TIMING_GTF_1280X800P75,          BT_TIMING_GTF_1280X800P75,      "GTF_1280X800P75"},
    [TIMING_VESA_1280X800P85]       = {TIMING_VESA_1280X800P85,         BT_TIMING_VESA_1280X800P85,     "VESA_1280X800P85"},
    [TIMING_CVT_1280X800P120_RB]    = {TIMING_CVT_1280X800P120_RB,      BT_TIMING_CVT_1280X800P120_RB,  "CVT_1280X800P120_RB"},
    [TIMING_VESA_1280X960P60]       = {TIMING_VESA_1280X960P60,         BT_TIMING_VESA_1280X960P60,     "VESA_1280X960P60"},
    [TIMING_VESA_1280X960P85]       = {TIMING_VESA_1280X960P85,         BT_TIMING_VESA_1280X960P85,     "VESA_1280X960P85"},
    [TIMING_CVT_1280X960P120_RB]    = {TIMING_CVT_1280X960P120_RB,      BT_TIMING_CVT_1280X960P120_RB,  "CVT_1280X960P120_RB"},
    [TIMING_VESA_1280X1024P60]      = {TIMING_VESA_1280X1024P60,        BT_TIMING_VESA_1280X1024P60,    "VESA_1280X1024P60"},
    [TIMING_CVT_1280X1024P60]       = {TIMING_CVT_1280X1024P60,         BT_TIMING_CVT_1280X1024P60,     "CVT_1280X1024P60"},
    [TIMING_CVT_1280X1024P60_RB]    = {TIMING_CVT_1280X1024P60_RB,      BT_TIMING_CVT_1280X1024P60_RB,  "CVT_1280X1024P60_RB"},
    [TIMING_GTF_1280X1024P60]       = {TIMING_GTF_1280X1024P60,         BT_TIMING_GTF_1280X1024P60,     "GTF_1280X1024P60"},
    [TIMING_CVT_1280X1024P70]       = {TIMING_CVT_1280X1024P70,         BT_TIMING_CVT_1280X1024P70,     "CVT_1280X1024P70"},
    [TIMING_CVT_1280X1024P70_RB]    = {TIMING_CVT_1280X1024P70_RB,      BT_TIMING_CVT_1280X1024P70_RB,  "CVT_1280X1024P70_RB"},
    [TIMING_GTF_1280X1024P70]       = {TIMING_GTF_1280X1024P70,         BT_TIMING_GTF_1280X1024P70,     "GTF_1280X1024P70"},
    [TIMING_CVT_1280X1024P72]       = {TIMING_CVT_1280X1024P72,         BT_TIMING_CVT_1280X1024P72,     "CVT_1280X1024P72"},
    [TIMING_GTF_1280X1024P72]       = {TIMING_GTF_1280X1024P72,         BT_TIMING_GTF_1280X1024P72,     "GTF_1280X1024P72"},
    [TIMING_VESA_1280X1024P75]      = {TIMING_VESA_1280X1024P75,        BT_TIMING_VESA_1280X1024P75,    "VESA_1280X1024P75"},
    [TIMING_CVT_1280X1024P75]       = {TIMING_CVT_1280X1024P75,         BT_TIMING_CVT_1280X1024P75,     "CVT_1280X1024P75"},
    [TIMING_GTF_1280X1024P75]       = {TIMING_GTF_1280X1024P75,         BT_TIMING_GTF_1280X1024P75,     "GTF_1280X1024P75"},
    [TIMING_CVT_1280X1024P76]       = {TIMING_CVT_1280X1024P76,         BT_TIMING_CVT_1280X1024P76,     "CVT_1280X1024P76"},
    [TIMING_GTF_1280X1024P76]       = {TIMING_GTF_1280X1024P76,         BT_TIMING_GTF_1280X1024P76,     "GTF_1280X1024P76"},
    [TIMING_VESA_1280X1024P85]      = {TIMING_VESA_1280X1024P85,        BT_TIMING_VESA_1280X1024P85,    "VESA_1280X1024P85"},
    [TIMING_CVT_1280X1024P120_RB]   = {TIMING_CVT_1280X1024P120_RB,     BT_TIMING_CVT_1280X1024P120_RB, "CVT_1280X1024P120_RB"},
    [TIMING_VESA_1360X768P60]       = {TIMING_VESA_1360X768P60,         BT_TIMING_VESA_1360X768P60,     "VESA_1360X768P60"},
    [TIMING_CVT_1360X768P120_RB]    = {TIMING_CVT_1360X768P120_RB,      BT_TIMING_CVT_1360X768P120_RB,  "CVT_1360X768P120_RB"},
    [TIMING_VESA_1366X768P60]       = {TIMING_VESA_1366X768P60,         BT_TIMING_VESA_1366X768P60,     "VESA_1366X768P60"},
    [TIMING_VESA_1366X768P60_RB]    = {TIMING_VESA_1366X768P60_RB,      BT_TIMING_VESA_1366X768P60_RB,  "VESA_1366X768P60_RB"},
    [TIMING_CVT_1400X1050P60]       = {TIMING_CVT_1400X1050P60,         BT_TIMING_CVT_1400X1050P60,     "CVT_1400X1050P60"},
    [TIMING_CVT_1400X1050P60_RB]    = {TIMING_CVT_1400X1050P60_RB,      BT_TIMING_CVT_1400X1050P60_RB,  "CVT_1400X1050P60_RB"},
    [TIMING_GTF_1400X1050P60]       = {TIMING_GTF_1400X1050P60,         BT_TIMING_GTF_1400X1050P60,     "GTF_1400X1050P60"},
    [TIMING_CVT_1400X1050P75]       = {TIMING_CVT_1400X1050P75,         BT_TIMING_CVT_1400X1050P75,     "CVT_1400X1050P75"},
    [TIMING_GTF_1400X1050P75]       = {TIMING_GTF_1400X1050P75,         BT_TIMING_GTF_1400X1050P75,     "GTF_1400X1050P75"},
    [TIMING_CVT_1400X1050P85]       = {TIMING_CVT_1400X1050P85,         BT_TIMING_CVT_1400X1050P85,     "CVT_1400X1050P85"},
    [TIMING_CVT_1400X1050P120_RB]   = {TIMING_CVT_1400X1050P120_RB,     BT_TIMING_CVT_1400X1050P120_RB, "CVT_1400X1050P120_RB"},
    [TIMING_CVT_1440X900P60]        = {TIMING_CVT_1440X900P60,          BT_TIMING_CVT_1440X900P60,      "CVT_1440X900P60"},
    [TIMING_CVT_1440X900P60_RB]     = {TIMING_CVT_1440X900P60_RB,       BT_TIMING_CVT_1440X900P60_RB,   "CVT_1440X900P60_RB"},
    [TIMING_GTF_1440X900P60]        = {TIMING_GTF_1440X900P60,          BT_TIMING_GTF_1440X900P60,      "GTF_1440X900P60"},
    [TIMING_CVT_1440X900P75]        = {TIMING_CVT_1440X900P75,          BT_TIMING_CVT_1440X900P75,      "CVT_1440X900P75"},
    [TIMING_GTF_1440X900P75]        = {TIMING_GTF_1440X900P75,          BT_TIMING_GTF_1440X900P75,      "GTF_1440X900P75"},
    [TIMING_CVT_1440X900P85]        = {TIMING_CVT_1440X900P85,          BT_TIMING_CVT_1440X900P85,      "CVT_1440X900P85"},
    [TIMING_CVT_1440X900P120_RB]    = {TIMING_CVT_1440X900P120_RB,      BT_TIMING_CVT_1440X900P120_RB,  "CVT_1440X900P120_RB"},
    [TIMING_CVT_1600X900P50]        = {TIMING_CVT_1600X900P50,          BT_TIMING_CVT_1600X900P50,      "CVT_1600X900P50"},
    [TIMING_GTF_1600X900P50]        = {TIMING_GTF_1600X900P50,          BT_TIMING_GTF_1600X900P50,      "GTF_1600X900P50"},
    [TIMING_CVT_1600X900P60]        = {TIMING_CVT_1600X900P60,          BT_TIMING_CVT_1600X900P60,      "CVT_1600X900P60"},
    [TIMING_CVT_1600X900P60_RB]     = {TIMING_CVT_1600X900P60_RB,       BT_TIMING_CVT_1600X900P60_RB,   "CVT_1600X900P60_RB"},
    [TIMING_GTF_1600X900P60]        = {TIMING_GTF_1600X900P60,          BT_TIMING_GTF_1600X900P60,      "GTF_1600X900P60"},
    [TIMING_CVT_1600X1000P60]       = {TIMING_CVT_1600X1000P60,         BT_TIMING_CVT_1600X1000P60,     "CVT_1600X1000P60"},
    [TIMING_CVT_1600X1000P60_RB]    = {TIMING_CVT_1600X1000P60_RB,      BT_TIMING_CVT_1600X1000P60_RB,  "CVT_1600X1000P60_RB"},
    [TIMING_GTF_1600X1000P60]       = {TIMING_GTF_1600X1000P60,         BT_TIMING_GTF_1600X1000P60,     "GTF_1600X1000P60"},
    [TIMING_CVT_1600X1000P75]       = {TIMING_CVT_1600X1000P75,         BT_TIMING_CVT_1600X1000P75,     "CVT_1600X1000P75"},
    [TIMING_GTF_1600X1000P75]       = {TIMING_GTF_1600X1000P75,         BT_TIMING_GTF_1600X1000P75,     "GTF_1600X1000P75"},
    [TIMING_VESA_1600X1200P60]      = {TIMING_VESA_1600X1200P60,        BT_TIMING_VESA_1600X1200P60,    "VESA_1600X1200P60"},
    [TIMING_CVT_1600X1200P60]       = {TIMING_CVT_1600X1200P60,         BT_TIMING_CVT_1600X1200P60,     "CVT_1600X1200P60"},
    [TIMING_CVT_1600X1200P60_RB]    = {TIMING_CVT_1600X1200P60_RB,      BT_TIMING_CVT_1600X1200P60_RB,  "CVT_1600X1200P60_RB"},
    [TIMING_GTF_1600X1200P60]       = {TIMING_GTF_1600X1200P60,         BT_TIMING_GTF_1600X1200P60,     "GTF_1600X1200P60"},
    [TIMING_VESA_1600X1200P65]      = {TIMING_VESA_1600X1200P65,        BT_TIMING_VESA_1600X1200P65,    "VESA_1600X1200P65"},
    [TIMING_CVT_1600X1200P65]       = {TIMING_CVT_1600X1200P65,         BT_TIMING_CVT_1600X1200P65,     "CVT_1600X1200P65"},
    [TIMING_GTF_1600X1200P65]       = {TIMING_GTF_1600X1200P65,         BT_TIMING_GTF_1600X1200P65,     "GTF_1600X1200P65"},
    [TIMING_VESA_1600X1200P70]      = {TIMING_VESA_1600X1200P70,        BT_TIMING_VESA_1600X1200P70,    "VESA_1600X1200P70"},
    [TIMING_CVT_1600X1200P70]       = {TIMING_CVT_1600X1200P70,         BT_TIMING_CVT_1600X1200P70,     "CVT_1600X1200P70"},
    [TIMING_GTF_1600X1200P70]       = {TIMING_GTF_1600X1200P70,         BT_TIMING_GTF_1600X1200P70,     "GTF_1600X1200P70"},
    [TIMING_VESA_1600X1200P75]      = {TIMING_VESA_1600X1200P75,        BT_TIMING_VESA_1600X1200P75,    "VESA_1600X1200P75"},
    [TIMING_CVT_1600X1200P75]       = {TIMING_CVT_1600X1200P75,         BT_TIMING_CVT_1600X1200P75,     "CVT_1600X1200P75"},
    [TIMING_GTF_1600X1200P75]       = {TIMING_GTF_1600X1200P75,         BT_TIMING_GTF_1600X1200P75,     "GTF_1600X1200P75"},
    [TIMING_VESA_1600X1200P85]      = {TIMING_VESA_1600X1200P85,        BT_TIMING_VESA_1600X1200P85,    "VESA_1600X1200P85"},
    [TIMING_CVT_1600X1200P120_RB]   = {TIMING_CVT_1600X1200P120_RB,     BT_TIMING_CVT_1600X1200P120_RB, "CVT_1600X1200P120_RB"},
    [TIMING_CVT_1600X1280P60]       = {TIMING_CVT_1600X1280P60,         BT_TIMING_CVT_1600X1280P60,     "CVT_1600X1280P60"},
    [TIMING_CVT_1600X1280P60_RB]    = {TIMING_CVT_1600X1280P60_RB,      BT_TIMING_CVT_1600X1280P60_RB,  "CVT_1600X1280P60_RB"},
    [TIMING_GTF_1600X1280P60]       = {TIMING_GTF_1600X1280P60,         BT_TIMING_GTF_1600X1280P60,     "GTF_1600X1280P60"},
    [TIMING_CVT_1680X1050P60]       = {TIMING_CVT_1680X1050P60,         BT_TIMING_CVT_1680X1050P60,     "CVT_1680X1050P60"},
    [TIMING_CVT_1680X1050P60_RB]    = {TIMING_CVT_1680X1050P60_RB,      BT_TIMING_CVT_1680X1050P60_RB,  "CVT_1680X1050P60_RB"},
    [TIMING_GTF_1680X1050P60]       = {TIMING_GTF_1680X1050P60,         BT_TIMING_GTF_1680X1050P60,     "GTF_1680X1050P60"},
    [TIMING_CVT_1680X1050P75]       = {TIMING_CVT_1680X1050P75,         BT_TIMING_CVT_1680X1050P75,     "CVT_1680X1050P75"},
    [TIMING_GTF_1680X1050P75]       = {TIMING_GTF_1680X1050P75,         BT_TIMING_GTF_1680X1050P75,     "GTF_1680X1050P75"},
    [TIMING_CVT_1680X1050P85]       = {TIMING_CVT_1680X1050P85,         BT_TIMING_CVT_1680X1050P85,     "CVT_1680X1050P85"},
    [TIMING_CVT_1680X1050P120_RB]   = {TIMING_CVT_1680X1050P120_RB,     BT_TIMING_CVT_1680X1050P120_RB, "CVT_1680X1050P120_RB"},
    [TIMING_VESA_1792X1344P60]      = {TIMING_VESA_1792X1344P60,        BT_TIMING_VESA_1792X1344P60,    "VESA_1792X1344P60"},
    [TIMING_VESA_1792X1344P75]      = {TIMING_VESA_1792X1344P75,        BT_TIMING_VESA_1792X1344P75,    "VESA_1792X1344P75"},
    [TIMING_CVT_1792X1344P120_RB]   = {TIMING_CVT_1792X1344P120_RB,     BT_TIMING_CVT_1792X1344P120_RB, "CVT_1792X1344P120_RB"},
    [TIMING_VESA_1856X1392P60]      = {TIMING_VESA_1856X1392P60,        BT_TIMING_VESA_1856X1392P60,    "VESA_1856X1392P60"},
    [TIMING_CVT_1856X1392P60]       = {TIMING_CVT_1856X1392P60,         BT_TIMING_CVT_1856X1392P60,     "CVT_1856X1392P60"},
    [TIMING_CVT_1856X1392P60_RB]    = {TIMING_CVT_1856X1392P60_RB,      BT_TIMING_CVT_1856X1392P60_RB,  "CVT_1856X1392P60_RB"},
    [TIMING_GTF_1856X1392P60]       = {TIMING_GTF_1856X1392P60,         BT_TIMING_GTF_1856X1392P60,     "GTF_1856X1392P60"},
    [TIMING_VESA_1856X1392P75]      = {TIMING_VESA_1856X1392P75,        BT_TIMING_VESA_1856X1392P75,    "VESA_1856X1392P75"},
    [TIMING_CVT_1856X1392P120_RB]   = {TIMING_CVT_1856X1392P120_RB,     BT_TIMING_CVT_1856X1392P120_RB, "CVT_1856X1392P120_RB"},
    [TIMING_VESA_1920X1080P50]      = {TIMING_VESA_1920X1080P50,        BT_TIMING_VESA_1920X1080P50,    "VESA_1920X1080P50"},
    [TIMING_CVT_1920X1080P50]       = {TIMING_CVT_1920X1080P50,         BT_TIMING_CVT_1920X1080P50,     "CVT_1920X1080P50"},
    [TIMING_GTF_1920X1080P50]       = {TIMING_GTF_1920X1080P50,         BT_TIMING_GTF_1920X1080P50,     "GTF_1920X1080P50"},
    [TIMING_VESA_1920X1080P60]      = {TIMING_VESA_1920X1080P60,        BT_TIMING_VESA_1920X1080P60,    "VESA_1920X1080P60"},
    [TIMING_CVT_1920X1080P60]       = {TIMING_CVT_1920X1080P60,         BT_TIMING_CVT_1920X1080P60,     "CVT_1920X1080P60"},
    [TIMING_CVT_1920X1080P60_RB]    = {TIMING_CVT_1920X1080P60_RB,      BT_TIMING_CVT_1920X1080P60_RB,  "CVT_1920X1080P60_RB"},
    [TIMING_GTF_1920X1080P60]       = {TIMING_GTF_1920X1080P60,         BT_TIMING_GTF_1920X1080P60,     "GTF_1920X1080P60"},
    [TIMING_CVT_1920X1080P72]       = {TIMING_CVT_1920X1080P72,         BT_TIMING_CVT_1920X1080P72,     "CVT_1920X1080P72"},
    [TIMING_CVT_1920X1080P72_R2]    = {TIMING_CVT_1920X1080P72_R2,      BT_TIMING_CVT_1920X1080P72_R2,  "CVT_1920X1080P72_R2"},
    [TIMING_GTF_1920X1080P72]       = {TIMING_GTF_1920X1080P72,         BT_TIMING_GTF_1920X1080P72,     "GTF_1920X1080P72"},
    [TIMING_CVT_1920X1080P75]       = {TIMING_CVT_1920X1080P75,         BT_TIMING_CVT_1920X1080P75,     "CVT_1920X1080P75"},
    [TIMING_GTF_1920X1080P75]       = {TIMING_GTF_1920X1080P75,         BT_TIMING_GTF_1920X1080P75,     "GTF_1920X1080P75"},
    [TIMING_VESA_1920X1080P90]      = {TIMING_VESA_1920X1080P90,        BT_TIMING_VESA_1920X1080P90,    "VESA_1920X1080P90"},
    [TIMING_VESA_1920X1080P100]     = {TIMING_VESA_1920X1080P100,       BT_TIMING_VESA_1920X1080P100,   "VESA_1920X1080P100"},
    [TIMING_CVT_1920X1080P120_RB]   = {TIMING_CVT_1920X1080P120_RB,     BT_TIMING_CVT_1920X1080P120_RB, "CVT_1920X1080P120_RB"},
    [TIMING_CVT_1920X1200P50]       = {TIMING_CVT_1920X1200P50,         BT_TIMING_CVT_1920X1200P50,     "CVT_1920X1200P50"},
    [TIMING_CVT_1920X1200P50_R2]    = {TIMING_CVT_1920X1200P50_R2,      BT_TIMING_CVT_1920X1200P50_R2,  "CVT_1920X1200P50_R2"},
    [TIMING_GTF_1920X1200P50]       = {TIMING_GTF_1920X1200P50,         BT_TIMING_GTF_1920X1200P50,     "GTF_1920X1200P50"},
    [TIMING_CVT_1920X1200P60]       = {TIMING_CVT_1920X1200P60,         BT_TIMING_CVT_1920X1200P60,     "CVT_1920X1200P60"},
    [TIMING_CVT_1920X1200P60_RB]    = {TIMING_CVT_1920X1200P60_RB,      BT_TIMING_CVT_1920X1200P60_RB,  "CVT_1920X1200P60_RB"},
    [TIMING_GTF_1920X1200P60]       = {TIMING_GTF_1920X1200P60,         BT_TIMING_GTF_1920X1200P60,     "GTF_1920X1200P60"},
    [TIMING_CVT_1920X1200P75]       = {TIMING_CVT_1920X1200P75,         BT_TIMING_CVT_1920X1200P75,     "CVT_1920X1200P75"},
    [TIMING_CVT_1920X1200P85]       = {TIMING_CVT_1920X1200P85,         BT_TIMING_CVT_1920X1200P85,     "CVT_1920X1200P85"},
    [TIMING_CVT_1920X1200P120_RB]   = {TIMING_CVT_1920X1200P120_RB,     BT_TIMING_CVT_1920X1200P120_RB, "CVT_1920X1200P120_RB"},
    [TIMING_CVT_1920X1440P60]       = {TIMING_CVT_1920X1440P60,         BT_TIMING_CVT_1920X1440P60,     "CVT_1920X1440P60"},
    [TIMING_CVT_1920X1440P60_RB]    = {TIMING_CVT_1920X1440P60_RB,      BT_TIMING_CVT_1920X1440P60_RB,  "CVT_1920X1440P60_RB"},
    [TIMING_GTF_1920X1440P60]       = {TIMING_GTF_1920X1440P60,         BT_TIMING_GTF_1920X1440P60,     "GTF_1920X1440P60"},
    [TIMING_BUTT]                   = {TIMING_BUTT,                     BT_TIMING_BUTT,                 "BUTT"}
};


/*******************************************************************************
* 函数名  : BT_GetTimingNum
* 描  述  : 获取总的时序
* 输  入  : 
* 输  出  : 
* 返回值  : 
*******************************************************************************/
inline UINT32 BT_GetTimingNum(VOID)
{
    return sizeof(gastTimingMap)/sizeof(gastTimingMap[0]);
}


/*******************************************************************************
* 函数名  : BT_GetTimingNum
* 描  述  : 获取总的时序
* 输  入  : 
* 输  出  : 
* 返回值  : 
*******************************************************************************/
inline const BT_TIMING_MAP_S* BT_GetTiming(BT_TIMING_E enTiming)
{
    return gastTimingMap + enTiming;
}

/*******************************************************************************
* 函数名  : BT_GetHTotal
* 描  述  : 获取一行的总的像素点个数
* 输  入  : const BT_TIMING_S *pstTiming
* 输  出  : 像素点总数
* 返回值  : 
*******************************************************************************/
inline UINT32 BT_GetHTotal(const BT_TIMING_S *pstTiming)
{
    return pstTiming->u32Width + pstTiming->u32HBackPorch + pstTiming->u32HSync + pstTiming->u32HFrontPorch;
}


/*******************************************************************************
* 函数名  : BT_GetVTotal
* 描  述  : 获取一帧的总行数
* 输  入  : const BT_TIMING_S *pstTiming
* 输  出  : 总行数
* 返回值  : 
*******************************************************************************/
inline UINT32 BT_GetVTotal(const BT_TIMING_S *pstTiming)
{
    return pstTiming->u32Height + pstTiming->u32VBackPorch + pstTiming->u32VSync + pstTiming->u32VFrontPorch;
}


