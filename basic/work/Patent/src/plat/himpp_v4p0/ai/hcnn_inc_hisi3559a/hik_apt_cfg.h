/***************************************************************************************************
* 版权信息：版权所有(c) , 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称: hik_apt_cfg.h
* 文件标识: __HIK_APT_CFG_H__
* 摘    要: 通用配置文件操作，用于读取、保存配置信息
*
* 当前版本: 0.1.0
* 作    者: 崔莫磊
* 日    期: 2019-06-18
* 备    注: 新建
* 
***************************************************************************************************/
#ifndef __HIK_APT_CFG_H__
#define __HIK_APT_CFG_H__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hik_apt_dbg.h"
#include "hik_apt_err.h"

/* USAGE
@CODE START{

#define HIK_APT_CFG_TAB \
X(test_image_list, "C:\test\test_image_list"); \
X(test_image_list, "C:\test\test_image_list"); \
.... //Any param that you want to save in config file
#include "hik_apt_cfg.h"


}@CODE END
*/

#ifndef HIK_APT_CFG_TAB
#define HIK_APT_CFG_TAB \
	X(image_list, NULL) \
	X(adse_pack_path, NULL) \
	X(context_path, NULL) \
	X(camera_inf, NULL) \
	X(rst_path, NULL) \
	X(width, "0") \
	X(height, "0") \
	X(is_async, "0") 
#endif


typedef struct _HIK_APT_CFG
{
#define X(name, def_value) char * name;
	HIK_APT_CFG_TAB
#undef X
	char *p_str_buf;
}HIK_APT_CFG;


static int hik_apt_cfg_init(HIK_APT_CFG *cfg)
{
#define X(name, def_value)  cfg->name = def_value;
	HIK_APT_CFG_TAB
#undef X
	cfg->p_str_buf = NULL;
	return HIK_APT_S_OK;
}

static int hik_apt_cfg_load(const char* cfg_path, HIK_APT_CFG *cfg)
{
	int idx = 0;
	char * p_char = NULL;
	char * p_equal = NULL;
	size_t len = 0;
	FILE *fp = fopen(cfg_path, "r");
	HIK_APT_CHECK_ERR(fp == NULL, -1);
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	cfg->p_str_buf = (char *)malloc(len + 1);
	fread(cfg->p_str_buf, 1, len, fp);
	cfg->p_str_buf[len] = '\0';
	fclose(fp);

	p_char = strtok(cfg->p_str_buf, "\n");
	do
	{
		p_equal = strchr(p_char, '=');
		if (p_equal == NULL)
		{
			break;
		}
		else
		{
			idx = strlen(p_equal);
			if (p_equal[idx - 1] == '\r')
			{
				p_equal[idx - 1] = (char)'\0';
			}
			*p_equal = (char)'\0';
		}
#define X(name, def_value)  if(strcmp(#name, p_char) == 0){ cfg->name = p_equal + 1; }
		HIK_APT_CFG_TAB
#undef X
	} while ((p_char = strtok(NULL, "\n")));


	return HIK_APT_S_OK;
}


static int hik_apt_cfg_save(const char* cfg_path, HIK_APT_CFG *cfg)
{
	FILE *fp = fopen(cfg_path, "w");
	HIK_APT_CHECK_ERR(fp == NULL, -1);
#define X(name, def_value)  fprintf(fp, #name"=%s\n", cfg->name);
	HIK_APT_CFG_TAB
#undef X
	fclose(fp);
	return HIK_APT_S_OK;
}


static int hik_apt_cfg_free(HIK_APT_CFG *cfg)
{
	if (cfg->p_str_buf)
	{
		free(cfg->p_str_buf);
	}
	return HIK_APT_S_OK;
}

#endif
