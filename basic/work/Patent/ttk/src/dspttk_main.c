/**
 * @file    dspTestToolkit.c
 * @brief   DSP组件Web测试工具集
 *
 * @note    测试工具集缩写“dspttk”，模块内非static接口以该命名开头
 * @note    该测试工具集以网页方式访问，建议使用Chrome浏览器，网页缩放比例不超过120%
 * @note    未避免重复错误，测试模块中代码是独立的：
 * @note      ① 测试模块内仅能访问DSP组件源码工程内的无接口定义类头文件，比如sal_macro.h
 * @note      ② 测试模块编译链接时，仅能访问DSP组件成果out/目录内资源
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include <string.h>
#include "sal_type.h"
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_html.h"
#include "dspttk_panel.h"
#include "dspttk_devcfg.h"
#include "../src/base/log/log_client.h"


/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */
extern void dspttk_webserver(void);


/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */

static void dspttk_log_register(void)
{
    INT32 logsize = 10 * 1024 * 1024;
    INT32 sync_size = 2 * 1024;
    INT32 net_type = 0;
    CHAR msg[128] = {0};

    snprintf(msg, sizeof(msg), LOG_MODULE_INIT_FORMAT_STRING, LOG_MSG_MODULE_INIT, logsize, sync_size, net_type);

    log_msg_write(LOG_MODULE_APP_NAME, LOG_LEVEL_CMD, msg);
    log_msg_write(LOG_MODULE_APP_NAME, LOG_LEVEL_SYNC, " ");

    return;
}


INT32 main(INT32 argc, CHAR **argv)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();

    sRet = dspttk_devcfg_init();
    if (SAL_SOK == sRet)
    {
        PRT_INFO("dspttk_devcfg_init successfully\n");
    }
    else
    {
        PRT_INFO("oops, dspttk_devcfg_init failed\n");
        return -1;
    }

    gpHtmlHandle = dspttk_html_create_handle(DSPTTK_INDEX_HTML_FILE);
    if (NULL != gpHtmlHandle)
    {
        dspttk_build_panels(gpHtmlHandle, pstDevCfg);
        dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);
        dspttk_log_register();
        dspttk_webserver(); // 该接口永久阻塞
    }
    else
    {
        PRT_INFO("oops, dspttk_html_create_handle failed\n");
    }

    return 0;
}
