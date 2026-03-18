/**
 * @file   dual_pkg_match_test_demo.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  双视角包裹匹配测试代码
 * @author sunzelin
 * @date   2022/6/6
 * @note
 * @note \n History
   1.日    期: 2022/6/6
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#include "dual_pkg_match.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define TEST_CHECK_RET(cond, str, val) \
    { \
        if (cond) \
        { \
            printf("%s, ret: %d \n", str, val); \
            return val;\
        } \
    }

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/
static bool get_exit_flag(void)
{
	int flag = 0;

	printf("another round? (0: no  1: yes): ");
	scanf("%d", &flag);
	
exit:
	return !!flag;
}

static int demo_deinit(void)
{
	return dpm_deinit();
}

DUAL_PKG_MATCH_RECT_INFO_S stRect = {0};

static int demo_get_rect(void *pXsiRawOutInfo,
                         DUAL_PKG_MATCH_VIEW_TYPE_E enViewType,
                         int iPkgId,
                         DUAL_PKG_MATCH_RECT_INFO_S *pstRect)
{
	float fViewGap = 0.0f;
	float fViewVerStart = 0.0f;

	/*
	   说明: 测试demo中包裹宽度为64，高度为32


							 ______ ______
		   MainVerStart---->|      |      |
						    |   0  |   1  |   ...       主视角
							|______|______|

							<--->
							 gap
							      ______ ______
		   SideVerStart ---->	 |      |      |
					             |   0  |   1  |  ...   侧视角
						         |______|______|
	*/
	
	fViewGap = (enViewType == DUAL_PKG_MATCH_MAIN_VIEW ? 0.0f : 2.0f);
	fViewVerStart = (enViewType == DUAL_PKG_MATCH_MAIN_VIEW ? 100.0f : 200.0f);

	pstRect->x = (fViewGap + iPkgId * 64.0) / 1280.0;
	pstRect->y = (fViewVerStart) / 1024.0;
	pstRect->w = (64.0) / 1280.0;
	pstRect->h = (32.0) / 1024.0;

	printf("get rect: [%f, %f] [%f, %f] \n", pstRect->x, pstRect->y, pstRect->w, pstRect->h);
	return 0;
}

static int demo_match(void)
{
	int r = -1;
	
	DUAL_PKG_MATCH_CTRL_PRM_S stMatchCtrl = {0};
	DUAL_PKG_MATCH_RESULT_S stMatchRslt = {0};

	stMatchCtrl.pRawOutInfo = (void *)1;  /* dbg: avoid ptr null checker */
	stMatchCtrl.pProcFunc = (dpm_get_pkg_rect_from_id_f)demo_get_rect;

	r = dpm_do_pkg_match(&stMatchCtrl, &stMatchRslt);
	if (r == 0)
	{
		printf("get match rslt: %d, main: %d, side: %d \n", 
			    stMatchRslt.bHasMatch, stMatchRslt.auiMatchId[0], stMatchRslt.auiMatchId[1]);
	}
	
	return r;
}

static int demo_insert(void)
{
	int pkg_id, r;
	DUAL_PKG_MATCH_VIEW_TYPE_E enMatchView = INVALID_VIEW_TYPE;
	DUAL_PKG_MATCH_OUTER_PKG_INFO_S stMatchOutPkgInfo = {0};

	pkg_id = 0;
	r = 0;
	while(pkg_id < 16)
	{
		enMatchView = DUAL_PKG_MATCH_MAIN_VIEW;
		stMatchOutPkgInfo.uiPkgId = pkg_id;
		r |= dpm_insert_pkg_queue(enMatchView, &stMatchOutPkgInfo);

		enMatchView = DUAL_PKG_MATCH_SIDE_VIEW;
		stMatchOutPkgInfo.uiPkgId = pkg_id;
		r |= dpm_insert_pkg_queue(enMatchView, &stMatchOutPkgInfo);

		pkg_id++;
	}

	return r;
}
	
static int demo_init(void)
{
	DUAL_PKG_MATCH_INIT_PRM_S stInitPrm = {0};

	stInitPrm.iInitChnCnt = 2;
	stInitPrm.astInitChnPrm[0].enViewType = DUAL_PKG_MATCH_MAIN_VIEW;
	stInitPrm.astInitChnPrm[0].stQuePrm.uiQueDepth = 16;
	
	stInitPrm.astInitChnPrm[1].enViewType = DUAL_PKG_MATCH_SIDE_VIEW;
	stInitPrm.astInitChnPrm[1].stQuePrm.uiQueDepth = 16;

	return dpm_init(&stInitPrm);
}

int main(void)
{	
	int r = -1;

	while(get_exit_flag())
	{
		// init 
		r = demo_init();
		TEST_CHECK_RET(r != 0, "demo init err!", -1); 

		printf("after demo init! \n");

		// insert 
		r = demo_insert();
		TEST_CHECK_RET(r != 0, "demo insert err!", -1);	

		printf("after demo insert! \n");

		// match
		r = demo_match();
		TEST_CHECK_RET(r != 0, "demo match err!", -1);	

		printf("after demo match! \n");

		// deinit
		r = demo_deinit();
		TEST_CHECK_RET(r != 0, "demo deinit err!", -1);	

		printf("after demo deinit! \n");
	}

	printf("main exit! \n");
	return 0;
}

