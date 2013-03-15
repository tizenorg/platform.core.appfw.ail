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



#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ail.h"
#include "ail_private.h"
#include "ail_convert.h"


struct _ail_str_map_t {
	ail_prop_str_e prop;
	const char *property;
};

static struct _ail_str_map_t str_prop_map[] = {
	{E_AIL_PROP_PACKAGE_STR, 		AIL_PROP_PACKAGE_STR},
	{E_AIL_PROP_EXEC_STR, 			AIL_PROP_EXEC_STR},
	{E_AIL_PROP_NAME_STR, 			AIL_PROP_NAME_STR},
	{E_AIL_PROP_TYPE_STR, 			AIL_PROP_TYPE_STR},
	{E_AIL_PROP_ICON_STR, 			AIL_PROP_ICON_STR},
	{E_AIL_PROP_CATEGORIES_STR, 		AIL_PROP_CATEGORIES_STR},
	{E_AIL_PROP_VERSION_STR, 		AIL_PROP_VERSION_STR},
	{E_AIL_PROP_MIMETYPE_STR, 		AIL_PROP_MIMETYPE_STR},
	{E_AIL_PROP_X_SLP_SERVICE_STR,		AIL_PROP_X_SLP_SERVICE_STR},
	{E_AIL_PROP_X_SLP_PACKAGETYPE_STR, 	AIL_PROP_X_SLP_PACKAGETYPE_STR},
	{E_AIL_PROP_X_SLP_PACKAGECATEGORIES_STR, AIL_PROP_X_SLP_PACKAGECATEGORIES_STR},
	{E_AIL_PROP_X_SLP_PACKAGEID_STR, 	AIL_PROP_X_SLP_PACKAGEID_STR},
/*	{E_AIL_PROP_X_SLP_URI_STR, 		AIL_PROP_X_SLP_URI_STR}, */
	{E_AIL_PROP_X_SLP_SVC_STR, 		AIL_PROP_X_SLP_SVC_STR},
	{E_AIL_PROP_X_SLP_EXE_PATH, 		AIL_PROP_X_SLP_EXE_PATH},
	{E_AIL_PROP_X_SLP_APPID_STR, 		AIL_PROP_X_SLP_APPID_STR},
	{E_AIL_PROP_X_SLP_PKGID_STR, 		AIL_PROP_X_SLP_PKGID_STR},
	{E_AIL_PROP_X_SLP_DOMAIN_STR,		AIL_PROP_X_SLP_DOMAIN_STR}
};


struct _ail_int_map_t {
	ail_prop_int_e prop;
	const char *property;
};

static struct _ail_int_map_t int_prop_map[] = {
	{E_AIL_PROP_X_SLP_TEMP_INT, AIL_PROP_X_SLP_TEMP_INT},
	{E_AIL_PROP_X_SLP_INSTALLEDTIME_INT, AIL_PROP_X_SLP_INSTALLEDTIME_INT}
};


struct _ail_bool_map_t {
	ail_prop_bool_e prop;
	const char *property;
};

static struct _ail_bool_map_t bool_prop_map[] = {
	{E_AIL_PROP_NODISPLAY_BOOL, AIL_PROP_NODISPLAY_BOOL},
	{E_AIL_PROP_X_SLP_TASKMANAGE_BOOL, AIL_PROP_X_SLP_TASKMANAGE_BOOL},
	{E_AIL_PROP_X_SLP_MULTIPLE_BOOL, AIL_PROP_X_SLP_MULTIPLE_BOOL},
	{E_AIL_PROP_X_SLP_REMOVABLE_BOOL, AIL_PROP_X_SLP_REMOVABLE_BOOL},
/*	{E_AIL_PROP_X_SLP_ISHORIZONTALSCALE_BOOL, AIL_PROP_X_SLP_ISHORIZONTALSCALE_BOOL}, */
	{E_AIL_PROP_X_SLP_ENABLED_BOOL, AIL_PROP_X_SLP_ENABLED_BOOL}
};


inline ail_prop_str_e _ail_convert_to_prop_str(const char *property)
{
	int i = 0;
	ail_prop_str_e prop = -1;

	retv_if(!property, AIL_ERROR_INVALID_PARAMETER);

	for (i=0 ; i<(E_AIL_PROP_STR_MAX - E_AIL_PROP_STR_MIN + 1) ; i++) {
		if (strcmp(property, str_prop_map[i].property) == 0) {
			prop =	str_prop_map[i].prop;
			break;
		}
	}

	return prop;
}

inline ail_prop_int_e _ail_convert_to_prop_int(const char *property)
{
	int i = 0;
	ail_prop_int_e prop = -1;

	retv_if(!property, AIL_ERROR_INVALID_PARAMETER);

	for (i=0 ; i<(E_AIL_PROP_INT_MAX - E_AIL_PROP_INT_MIN + 1) ; i++) {
		if (strcmp(property, int_prop_map[i].property) == 0) {
			prop =	int_prop_map[i].prop;
			break;
		}
	}

	return prop;
}

inline ail_prop_bool_e _ail_convert_to_prop_bool(const char *property)
{
	int i = 0;
	ail_prop_bool_e prop = -1;

	retv_if(!property, AIL_ERROR_INVALID_PARAMETER);

	for (i=0 ; i<(E_AIL_PROP_BOOL_MAX - E_AIL_PROP_BOOL_MIN + 1) ; i++) {
		if (strcmp(property, bool_prop_map[i].property) == 0) {
			prop = 	bool_prop_map[i].prop;
			break;
		}
	}

	return prop;
}

