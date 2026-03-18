#ifndef __HIK_APT_SRL_SYNTAX_H__
#define __HIK_APT_SRL_SYNTAX_H__

#include <stdint.h>

#ifndef HIK_APT_SRLM_ENABLE

/* syntax to indicate the declaration should be serialized */
#define SRLM_DECORATOR
#define HIK_APT_SRLM_DECORATOR
#define HIK_APT_SRLM_ENUM enum
#define HIK_APT_SRLM_STRUCT struct

/* syntax for plugin definition */
#define HIK_APT_SRLM_VEC_SIZE_PLUGIN_DECLARE(unique_name, srl_plugin_header_file_name, get_ele_num_func_name, util_plugin_header_file_name, random_init_func_name)
#define HIK_APT_SRLM_GENERAL_PLUGIN_DECLARE(unique_name, \
                                    srl_plugin_header_file_name, \
                                    serialize_func_name, \
                                    deserialize_func_name, \
                                    get_srl_size_func_name, \
                                    get_srl_max_size_func_name, \
                                    get_build_size_func_name, \
                                    get_build_max_size_func_name, \
                                    util_plugin_header_file_name, \
                                    random_init_func_name, \
                                    compare_func_name)


/* syntax for field in structure declaration */
#define HIK_APT_SRLM_FIELD_VEC(dim_id, num_field_name, max_size)

#define HIK_APT_SRLM_FIELD_BIND_VEC_SIZE_PLUGIN(dim_id, unique_name, max_size) 

#define HIK_APT_SRLM_FIELD_IGNORE()
#define HIK_APT_SRLM_FIELD_BIND_GENERAL_PLUGIN(unique_name)

#define HIK_APT_SRLM_FIELD_VOID_AS_TYPE(type)
#define HIK_APT_SRLM_FIELD_VOID_AS_BYTESTREAM()

#define HIK_APT_SRLM_FIELD_VEC_AS_STRING(dim_id, max_size)
#define HIK_APT_SRLM_FIELD_STRING(max_size)

#else
#include "hik_apt_srl_syntax_internal.h"
#endif


#endif