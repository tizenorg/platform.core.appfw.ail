/*
 * ail
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>, Sewook Park <sewook7.park@samsung.com>, Jaeho Lee <jaeho81.lee@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */




#ifndef __AIL_PRIVATE_H__
#define __AIL_PRIVATE_H__

#include <stdbool.h>
#include <sqlite3.h>

#ifndef EXPORT_API
#define EXPORT_API __attribute__ ((visibility("default")))
#endif

#undef LOG_TAG
#define LOG_TAG "AIL"

#if 1
#include <dlog.h>
#define _E(fmt, arg...) LOGE("[%s,%d] "fmt,__FUNCTION__,__LINE__,##arg)
#define _D(fmt, arg...) LOGD("[%s,%d] "fmt,__FUNCTION__,__LINE__,##arg)
#else
#include <stdio.h>
#define _E(fmt, arg...) fprintf(stderr, "[%s,%d] "fmt,__FUNCTION__,__LINE__,##arg)
#define _D(fmt, arg...) fprintf(stderr, "[%s,%d] "fmt,__FUNCTION__,__LINE__,##arg)
#endif

#define retv_if(expr, val) do { \
	if(expr) { \
		_E("(%s) -> %s() return\n", #expr, __FUNCTION__); \
		return (val); \
	} \
} while (0)

#define SAFE_FREE_AND_STRDUP(from, to) do { \
	if (to) free(to); \
	to = strdup(from); \
} while (0)

#define SAFE_FREE(ptr) do { \
	if (ptr) free(ptr); \
} while (0)

struct element {
	int prop;
};

struct element_int {
	int prop;
	int value;
};

struct element_bool {
	int prop;
	bool value;
};

struct element_str {
	int prop;
	char *value;
};

enum {
	VAL_TYPE_BOOL,
	VAL_TYPE_INT,
	VAL_TYPE_STR,
};

#define ELEMENT_INT(e) ((struct element_int *)(e))
#define ELEMENT_STR(e) ((struct element_str *)(e))
#define ELEMENT_BOOL(e) ((struct element_bool *)(e))

#define AIL_SQL_QUERY_MAX_LEN	2048
#define APP_INFO_DB "/opt/dbspace/.app_info.db"

#define ELEMENT_TYPE(e, t) do { \
	if(e->prop >= E_AIL_PROP_STR_MIN && e->prop <= E_AIL_PROP_STR_MAX) t= (int)VAL_TYPE_STR; \
	else if (e->prop >= E_AIL_PROP_INT_MIN && e->prop <= E_AIL_PROP_INT_MAX) t= (int)VAL_TYPE_INT; \
	else if (e->prop >= E_AIL_PROP_BOOL_MIN && e->prop <= E_AIL_PROP_BOOL_MAX) t= (int)VAL_TYPE_BOOL; \
	else t = -1; \
} while(0)


#define PROP_TYPE(p, t) do { \
	if(p >= E_AIL_PROP_STR_MIN && p <= E_AIL_PROP_STR_MAX) t= (int)VAL_TYPE_STR; \
	else if (p >= E_AIL_PROP_INT_MIN && p <= E_AIL_PROP_INT_MAX) t= (int)VAL_TYPE_INT; \
	else if (p >= E_AIL_PROP_BOOL_MIN && p <= E_AIL_PROP_BOOL_MAX) t= (int)VAL_TYPE_BOOL; \
	else t = NULL; \
} while(0)


/**
 * @brief string type properties
 */
typedef enum {
	E_AIL_PROP_STR_MIN = 0,
	E_AIL_PROP_PACKAGE_STR = E_AIL_PROP_STR_MIN,
	E_AIL_PROP_EXEC_STR,
	E_AIL_PROP_NAME_STR,
	E_AIL_PROP_TYPE_STR,
	E_AIL_PROP_ICON_STR,
	E_AIL_PROP_CATEGORIES_STR,
	E_AIL_PROP_VERSION_STR,
	E_AIL_PROP_MIMETYPE_STR,
	E_AIL_PROP_X_SLP_SERVICE_STR,
	E_AIL_PROP_X_SLP_PACKAGETYPE_STR,
	E_AIL_PROP_X_SLP_PACKAGECATEGORIES_STR,
	E_AIL_PROP_X_SLP_PACKAGEID_STR,
	E_AIL_PROP_X_SLP_URI_STR,
	E_AIL_PROP_X_SLP_SVC_STR,
	E_AIL_PROP_X_SLP_EXE_PATH,
	E_AIL_PROP_X_SLP_APPID_STR,
	E_AIL_PROP_X_SLP_PKGID_STR,
	E_AIL_PROP_X_SLP_DOMAIN_STR,
	E_AIL_PROP_STR_MAX = E_AIL_PROP_X_SLP_DOMAIN_STR,
} ail_prop_str_e;


/**
 * @brief integer type properties
 */
typedef enum {
	E_AIL_PROP_INT_MIN = E_AIL_PROP_STR_MAX + 1,
	E_AIL_PROP_X_SLP_TEMP_INT = E_AIL_PROP_INT_MIN,
	E_AIL_PROP_X_SLP_INSTALLEDTIME_INT,
	E_AIL_PROP_INT_MAX = E_AIL_PROP_X_SLP_INSTALLEDTIME_INT,
} ail_prop_int_e;


/**
 * @brief boolean type properties
 */
typedef enum {
	E_AIL_PROP_BOOL_MIN = E_AIL_PROP_INT_MAX + 1,
	E_AIL_PROP_NODISPLAY_BOOL = E_AIL_PROP_BOOL_MIN,
	E_AIL_PROP_X_SLP_TASKMANAGE_BOOL,
	E_AIL_PROP_X_SLP_MULTIPLE_BOOL,
	E_AIL_PROP_X_SLP_REMOVABLE_BOOL,
	E_AIL_PROP_X_SLP_ISHORIZONTALSCALE_BOOL,
	E_AIL_PROP_X_SLP_ENABLED_BOOL,
	E_AIL_PROP_BOOL_MAX = E_AIL_PROP_X_SLP_ENABLED_BOOL,
} ail_prop_bool_e;

#define NUM_OF_PROP E_AIL_PROP_BOOL_MAX + 1

#endif
