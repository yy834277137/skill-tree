/*******************************************************************************
 * harfbuzz.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : fangzuopeng
 * Version: V2.6.8  2023年6月26日 Create
 *
 * Description : harfbuzz字符整性相关接口
 * Modification:
 *******************************************************************************/
#include "harfbuzz_func.h"

hb_font_t *g_hbFont = NULL;
hb_face_t *g_hbFace = NULL;
hb_buffer_t *g_hbBuffer = NULL;

/*******************************************************************************
 * 函数名  : HB_Init
 * 描  述  : HB初始化
 * 输  入  :
 * 输  出  : 无
 * 返回值  : 0        : 成功
 *         -1      : 失败
 *******************************************************************************/
INT32 HB_Init(const char *pcfont, UINT32 u32FontLen)
{
    void *userdata = (void *)pcfont;
    hb_blob_t *blob = NULL;
    unsigned int upem = 0;

    blob = hb_blob_create(pcfont, u32FontLen, HB_MEMORY_MODE_WRITABLE, userdata, free);

    g_hbFace = hb_face_create(blob, 0 /* first face */);

    hb_blob_destroy(blob);

    upem = hb_face_get_upem(g_hbFace);

    g_hbFont = hb_font_create(g_hbFace);

    hb_font_set_scale(g_hbFont, upem, upem);
    // hb_font_set_funcs(font);

    g_hbBuffer = hb_buffer_create();

    return SAL_SOK;
}

/*******************************************************************************
 * 函数名  : HB_PlasticCode
 * 描  述  : 字符整形
 * 输  入  :
 * 输  出  : 无
 * 返回值  : 0        : 成功
 *         -1      : 失败
 *******************************************************************************/
INT32 HB_PlasticCode(char *pu8Unicode, UINT32 *pu32CodeNum, hb_glyph_info_t **pstHbGlyphInfos, ENCODING_FORMAT_E enEncFormat)
{
    if (g_hbFont == NULL)
    {
        FREETYPE_LOGE("HB is not init\n");
        return SAL_FALSE;
    }

    hb_buffer_clear_contents(g_hbBuffer);

    /* usc-2le存入缓冲区 */
    hb_buffer_add_utf16(g_hbBuffer, (UINT16 *)pu8Unicode, -1, 0, -1);

    /* 设置缓冲区脚本、语言和方向 */
    if (enEncFormat == ENC_FMT_ISO_8859_6)
    {
        hb_buffer_set_direction(g_hbBuffer, HB_DIRECTION_RTL);
        hb_buffer_set_script(g_hbBuffer, HB_SCRIPT_ARABIC);
    }
    else
    {
        hb_buffer_set_direction(g_hbBuffer, HB_DIRECTION_TTB);
        hb_buffer_set_script(g_hbBuffer, HB_SCRIPT_COMMON);
    }

    // hb_buffer_set_language(buf, hb_language_from_string("en", -1));

    /* 字符串分段处理 */
    hb_buffer_guess_segment_properties(g_hbBuffer);

    /* 字符串整形 */
    hb_shape(g_hbFont, g_hbBuffer, NULL, 0);

    /* 得到长度 */
    *pu32CodeNum = hb_buffer_get_length(g_hbBuffer);

    /* 得到代码点 */
    *pstHbGlyphInfos = hb_buffer_get_glyph_infos(g_hbBuffer, NULL);

    return SAL_SOK;
}

/*******************************************************************************
 * 函数名  : HB_Deinit
 * 描  述  : HB反初始化
 * 输  入  :
 * 输  出  : 无
 * 返回值  : 无
 *******************************************************************************/
VOID HB_Deinit()
{
    hb_buffer_destroy(g_hbBuffer);
    hb_font_destroy(g_hbFont);
    hb_face_destroy(g_hbFace);

    return;
}