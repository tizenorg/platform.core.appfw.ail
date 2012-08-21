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



#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <xdgmime.h>

#include <vconf.h>
#include <glib.h>

#include "ail_private.h"
#include "ail_db.h"
#include "ail.h"

#define OPT_DESKTOP_DIRECTORY "/opt/share/applications"
#define USR_DESKTOP_DIRECTORY "/usr/share/applications"
#define BUFSZE 4096

#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#define argsdelimiter	" \t"

typedef enum {
	NOTI_ADD,
	NOTI_UPDATE,
	NOTI_REMOVE,
	NOTI_MAX,
} noti_type;

struct entry_parser {
	const char *field;
	ail_error_e (*value_cb)(void *data, char *tag, char *value);
};

inline static char *_ltrim(char *str)
{
	if (!str) return NULL;

	while (*str == ' ' || *str == '\t' || *str == '\n') str ++;

	return str;
}



inline static int _rtrim(char *str)
{
	int len;

	len = strlen(str);
	while (--len >= 0 && (str[len] == ' ' || str[len] == '\n' || str[len] == '\t')) str[len] = '\0';

	return len;
}

struct name_item {
	char *locale;
	char *name;
};

typedef struct {
	const char*	package;
	char*		exec;
	char*		name;
	char*		type;
	char*		icon;
	char*		categories;
	char*		version;
	char*		mimetype;
	char*		x_slp_service;
	char*		x_slp_packagetype;
	char*		x_slp_packagecategories;
	char*		x_slp_packageid;
	char*		x_slp_uri;
	char*		x_slp_svc;
	char*		x_slp_exe_path;
	char*		x_slp_appid;
	int		x_slp_baselayoutwidth;
	int		x_slp_installedtime;
	int		nodisplay;
	int		x_slp_taskmanage;
	int		x_slp_multiple;
	int		x_slp_removable;
	int		x_slp_ishorizontalscale;
	int		x_slp_inactivated;
	char*		desktop;
	GSList*		localname;
} desktop_info_s;



static ail_error_e _read_exec(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;
	char *token_exe_path;
	char *save_ptr;
	char *temp_exec;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->exec);
	retv_if(!info->exec, AIL_ERROR_OUT_OF_MEMORY);

	temp_exec = strdup(value);
	if(!temp_exec) {
		free(info->exec);
		return AIL_ERROR_OUT_OF_MEMORY;
	}

	token_exe_path = strtok_r(temp_exec, argsdelimiter, &save_ptr);

	info->x_slp_exe_path = strdup(token_exe_path);
	if(!info->x_slp_exe_path) {
		free(info->exec);
		info->exec = NULL;
		free(temp_exec);
		return AIL_ERROR_OUT_OF_MEMORY;
	}

	free(temp_exec);

	return AIL_ERROR_OK;
}



static ail_error_e _read_name(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);
	retv_if(0 == strlen(value), AIL_ERROR_FAIL);

	if (tag && strlen(tag) > 0) {
		struct name_item *item;
		item = (struct name_item *)calloc(1, sizeof(struct name_item));
		retv_if (NULL == item, AIL_ERROR_OUT_OF_MEMORY);

		SAFE_FREE_AND_STRDUP(tag, item->locale);
		if(NULL == item->locale) {
			_E("(NULL == item->locale) return\n");
			free(item);
			return AIL_ERROR_OUT_OF_MEMORY;
		}

		SAFE_FREE_AND_STRDUP(value, item->name);
		if(NULL == item->name) {
			_E("(NULL == item->name) return\n");
			free(item->locale);
			free(item);
			return AIL_ERROR_OUT_OF_MEMORY;
		}

		info->localname = g_slist_append(info->localname, item);

		return AIL_ERROR_OK;
	} else {
		SAFE_FREE_AND_STRDUP(value, info->name);
		retv_if (!info->name, AIL_ERROR_OUT_OF_MEMORY);

		return AIL_ERROR_OK;
	}
}



static ail_error_e _read_type(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->type);
	retv_if (!info->type, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}


static char*
_get_package_from_icon(char* icon)
{
	char* package;
	char* extension;

	retv_if(!icon, NULL);

	package = strdup(icon);
	retv_if(!package, NULL);
	extension = rindex(package, '.');
	if (extension) {
		*extension = '\0';
	} else {
		_E("cannot extract from icon [%s] to package.", icon);
	}

	return package;
}


static char*
_get_icon_with_path(char* icon)
{
	retv_if(!icon, NULL);

	if (index(icon, '/') == NULL) {
		char* package;
		char* theme = NULL;
		char* icon_with_path = NULL;
		int len;

		package = _get_package_from_icon(icon);
		retv_if(!package, NULL);

		theme = vconf_get_str("db/setting/theme");
		if (!theme) {
			theme = strdup("default");
			if(!theme) {
				free(package);
				return NULL;
			}
		}

		len = (0x01 << 7) + strlen(icon) + strlen(package) + strlen(theme);
		icon_with_path = malloc(len);
		if(icon_with_path == NULL) {
			_E("(icon_with_path == NULL) return\n");
			free(package);
			free(theme);
			return NULL;
		}

		memset(icon_with_path, 0, len);

		snprintf(icon_with_path, len, "/opt/share/icons/%s/small/%s", theme, icon);
		do {
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/usr/share/icons/%s/small/%s", theme, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			_D("cannot find icon %s", icon_with_path);
			snprintf(icon_with_path, len, "/opt/share/icons/default/small/%s", icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/usr/share/icons/default/small/%s", icon);
			if (access(icon_with_path, R_OK) == 0) break;

			#if 1 /* this will be remove when finish the work for moving icon path */
			_E("icon file must be moved to %s", icon_with_path);
			snprintf(icon_with_path, len, "/opt/apps/%s/res/icons/%s/small/%s", package, theme, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/usr/apps/%s/res/icons/%s/small/%s", package, theme, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			_D("cannot find icon %s", icon_with_path);
			snprintf(icon_with_path, len, "/opt/apps/%s/res/icons/default/small/%s", package, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/usr/apps/%s/res/icons/default/small/%s", package, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			#endif
		} while (0);

		free(theme);
		free(package);

		_D("Icon path : %s ---> %s", icon, icon_with_path);

		return icon_with_path;
	} else {
		char* confirmed_icon = NULL;

		confirmed_icon = strdup(icon);
		retv_if(!confirmed_icon, NULL);
		return confirmed_icon;
	}
}


static ail_error_e _read_icon(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	info->icon = _get_icon_with_path(value);

	retv_if (!info->icon, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}



static ail_error_e _read_categories(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->categories);
	retv_if (!info->categories, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}



static ail_error_e _read_version(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->version);
	retv_if (!info->version, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}



static ail_error_e _read_mimetype(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;
	int size, total_len = 0;
	char *mimes_origin, *mimes_changed, *token_unalias, *save_ptr;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!strlen(value), AIL_ERROR_FAIL);

	mimes_origin = strdup(value);
	retv_if(!mimes_origin, AIL_ERROR_OUT_OF_MEMORY);

	size = getpagesize();
	mimes_changed = calloc(1, size);
	if(mimes_changed == NULL) {
		_E("(mimes_changed == NULL) return\n");
		free(mimes_origin);
		return AIL_ERROR_OUT_OF_MEMORY;
	}

	token_unalias = strtok_r(mimes_origin, ";", &save_ptr);

	while (token_unalias) {
		int token_len;
		const char *token_alias;

		_rtrim(token_unalias);
		token_unalias = _ltrim(token_unalias);

		token_alias = xdg_mime_unalias_mime_type(token_unalias);
		if (!token_alias) continue;

		token_len = strlen(token_alias);
		if (total_len + token_len + (1<<1) >= size) {
			char *tmp;
			size *= 2;
			tmp = realloc(mimes_changed, size);
			if(!tmp) {
				free(mimes_changed);
				return AIL_ERROR_OUT_OF_MEMORY;
			}
			mimes_changed = tmp;
		}

		strncat(mimes_changed, token_alias, size-1);
		total_len += token_len;

		token_unalias = strtok_r(NULL, ";", &save_ptr);
		if (token_unalias) {
			strncat(mimes_changed, ";", size-strlen(mimes_changed)-1);
		}
	}

	SAFE_FREE(info->mimetype);
	info->mimetype = mimes_changed;

	return AIL_ERROR_OK;
}



static ail_error_e _read_nodisplay(void *data, char *tag, char *value)
{
	desktop_info_s* info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	info->nodisplay = !strcasecmp(value, "true");

	return AIL_ERROR_OK;
}



static ail_error_e _read_x_slp_service(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_service);
	retv_if(!info->x_slp_service, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}



static ail_error_e _read_x_slp_packagetype(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_packagetype);
	retv_if(!info->x_slp_packagetype, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}



static ail_error_e _read_x_slp_packagecategories(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_packagecategories);
	retv_if(!info->x_slp_packagecategories, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}



static ail_error_e _read_x_slp_packageid(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_packageid);
	retv_if(!info->x_slp_packageid, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}



static ail_error_e _read_x_slp_uri(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_uri);
	retv_if(!info->x_slp_uri, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}



static ail_error_e _read_x_slp_svc(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_svc);
	retv_if(!info->x_slp_svc, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}



static ail_error_e _read_x_slp_taskmanage(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	info->x_slp_taskmanage = !strcasecmp(value, "true");

	return AIL_ERROR_OK;
}



static ail_error_e _read_x_slp_multiple(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	info->x_slp_multiple = !strcasecmp(value, "true");

	return AIL_ERROR_OK;
}



static ail_error_e _read_x_slp_removable(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	info->x_slp_removable = !strcasecmp(value, "true");

	return AIL_ERROR_OK;
}


static ail_error_e _read_x_slp_appid(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_appid);
	retv_if(!info->x_slp_appid, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}

static struct entry_parser entry_parsers[] = {
	{
		.field = "exec",
		.value_cb = _read_exec,
	},
	{
		.field = "name",
		.value_cb = _read_name,
	},
	{
		.field = "type",
		.value_cb = _read_type,
	},
	{
		.field = "icon",
		.value_cb = _read_icon,
	},
	{
		.field = "categories",
		.value_cb = _read_categories,
	},
	{
		.field = "version",
		.value_cb = _read_version,
	},
	{
		.field = "mimetype",
		.value_cb = _read_mimetype,
	},
	{
		.field = "x-tizen-service",
		.value_cb = _read_x_slp_service,
	},
	{
		.field = "x-tizen-packagetype",
		.value_cb = _read_x_slp_packagetype,
	},
	{
		.field = "x-tizen-packagecategories",
		.value_cb = _read_x_slp_packagecategories,
	},
	{
		.field = "x-tizen-packageid",
		.value_cb = _read_x_slp_packageid,
	},
	{
		.field = "x-tizen-uri",
		.value_cb = _read_x_slp_uri,
	},
	{
		.field = "x-tizen-svc",
		.value_cb = _read_x_slp_svc,
	},
	{
		.field = "nodisplay",
		.value_cb = _read_nodisplay,
	},
	{
		.field = "x-tizen-taskmanage",
		.value_cb = _read_x_slp_taskmanage,
	},
	{
		.field = "x-tizen-multiple",
		.value_cb = _read_x_slp_multiple,
	},
	{
		.field = "x-tizen-removable",
		.value_cb = _read_x_slp_removable,
	},
	{
		.field = "x-tizen-appid",
		.value_cb = _read_x_slp_appid,
	},
	{
		.field = NULL,
		.value_cb = NULL,
	},
};



/* Utility functions */
static int _count_all(void)
{
	ail_error_e ret;
	int count;

	ret = ail_filter_count_appinfo(NULL, &count);
	if(ret != AIL_ERROR_OK) {
		_E("cannot count appinfo");
		count = -1;
	}

	retv_if(ret != AIL_ERROR_OK, -1);

	return count;
}



char *_pkgname_to_desktop(const char *package)
{
	char *desktop;
	int size;

	retv_if(!package, NULL);

	size = strlen(OPT_DESKTOP_DIRECTORY) + strlen(package) + 10;
	desktop = malloc(size);
	retv_if(!desktop, NULL);

	snprintf(desktop, size, OPT_DESKTOP_DIRECTORY"/%s.desktop", package);

	if (access(desktop, R_OK) == 0)
		return desktop;

	snprintf(desktop, size, USR_DESKTOP_DIRECTORY"/%s.desktop", package);

	return desktop;
}



static inline int _strlen_desktop_info(desktop_info_s* info)
{
	int len = 0;

	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);

	if (info->package) len += strlen(info->package);
	if (info->exec) len += strlen(info->exec);
	if (info->name) len += strlen(info->name);
	if (info->type) len += strlen(info->type);
	if (info->icon) len += strlen(info->icon);
	if (info->categories) len += strlen(info->categories);
	if (info->version) len += strlen(info->version);
	if (info->mimetype) len += strlen(info->mimetype);
	if (info->x_slp_service) len += strlen(info->x_slp_service);
	if (info->x_slp_packagetype) len += strlen(info->x_slp_packagetype);
	if (info->x_slp_packagecategories) len += strlen(info->x_slp_packagecategories);
	if (info->x_slp_packageid) len += strlen(info->x_slp_packageid);
	if (info->x_slp_uri) len += strlen(info->x_slp_uri);
	if (info->x_slp_svc) len += strlen(info->x_slp_svc);
	if (info->x_slp_exe_path) len += strlen(info->x_slp_exe_path);
	if (info->x_slp_appid) len += strlen(info->x_slp_appid);
	if (info->desktop) len += strlen(info->desktop);

	return len;
}


int __is_ail_initdb(void)
{
	if( getenv("AIL_INITDB") || getenv("INITDB") )
		return 1;
	else
		return 0;
}



/* Manipulating desktop_info functions */
static ail_error_e _init_desktop_info(desktop_info_s *info, const char *package)
{
	static int is_initdb = -1;

	if(is_initdb == -1)
		is_initdb = __is_ail_initdb();

	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!package, AIL_ERROR_INVALID_PARAMETER);

	/* defaults */
	info->package = package;

	info->x_slp_taskmanage = 1;
	info->x_slp_removable = 1;

	if(is_initdb)
		info->x_slp_installedtime = 0;
	else
		info->x_slp_installedtime = time(NULL);

#ifdef PKGTYPE
	info->x_slp_packagetype = strdup(PKGTYPE);
#else
	info->x_slp_packagetype = strdup("rpm");
#endif
	retv_if(!info->x_slp_packagetype, AIL_ERROR_OUT_OF_MEMORY);

	info->x_slp_packageid = strdup(package);
	retv_if(!info->x_slp_packageid, AIL_ERROR_OUT_OF_MEMORY);
	info->x_slp_appid = strdup(package);
	retv_if(!info->x_slp_appid, AIL_ERROR_OUT_OF_MEMORY);

	info->desktop = _pkgname_to_desktop(package);
	retv_if(!info->desktop, AIL_ERROR_FAIL);

	return AIL_ERROR_OK;
}



static ail_error_e _read_desktop_info(desktop_info_s* info)
{
	char *line = NULL;
	FILE *fp;
	size_t size = 0;
	ssize_t read;

	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);

	fp = fopen(info->desktop, "r");
	retv_if(!fp, AIL_ERROR_FAIL);

	while ((read = getline(&line, &size, fp)) != -1) {
		int len, idx;
		char *tmp, *field, *field_name, *tag, *value;

		tmp = _ltrim(line);
		if(tmp == NULL) continue;
		if (*tmp == '#') continue;
		if (_rtrim(tmp) <= 0) continue;

		len = strlen(line) + 1;
		field = calloc(1, len);
		field_name = calloc(1, len);
		tag = calloc(1, len);
		value = calloc(1, len);

		if (!field || !field || !tag || !value) {
			goto NEXT;
		}

		sscanf(tmp, "%[^=]=%[^\n]", field, value);
		_rtrim(field);
		tmp = _ltrim(value);

		sscanf(field, "%[^[][%[^]]]", field_name, tag);

		if (!field_name || !strlen(field_name)){
			goto NEXT;
		}

		for (idx = 0; entry_parsers[idx].field; idx ++) {
			if (!strcasecmp(entry_parsers[idx].field, field_name) && entry_parsers[idx].value_cb) {
				if (entry_parsers[idx].value_cb(info, tag, tmp) != AIL_ERROR_OK) {
					_E("field - [%s] is wrong.", field_name);
				}
				break;
			}
		}
NEXT:
		SAFE_FREE(field);
		SAFE_FREE(field_name);
		SAFE_FREE(tag);
		SAFE_FREE(value);
	}

	_D("Read (%s).", info->package);
	fclose(fp);

	return AIL_ERROR_OK;
}



static ail_error_e _modify_desktop_info_bool(desktop_info_s* info,
						  const char *property,
						  bool value)
{
	ail_prop_bool_e prop;
	int val;

	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!property, AIL_ERROR_INVALID_PARAMETER);

	prop = _ail_convert_to_prop_bool(property);

	if (prop < E_AIL_PROP_BOOL_MIN || prop > E_AIL_PROP_BOOL_MAX)
		return AIL_ERROR_INVALID_PARAMETER;

	switch (prop) {
		case E_AIL_PROP_X_SLP_INACTIVATED_BOOL:
			info->x_slp_inactivated = (int)value;
			break;
		default:
			return AIL_ERROR_FAIL;
	}

	return AIL_ERROR_OK;
}



static ail_error_e _create_table(void)
{
	int i;
	ail_error_e ret;
	const char *tbls[3] = {
		"CREATE TABLE app_info "
		"(package TEXT PRIMARY KEY, "
		"exec TEXT DEFAULT 'No Exec', "
		"name TEXT DEFAULT 'No Name', "
		"type TEXT DEFAULT 'Application', "
		"icon TEXT DEFAULT 'No Icon', "
		"categories TEXT, "
		"version TEXT, "
		"mimetype TEXT, "
		"x_slp_service TEXT, "
		"x_slp_packagetype TEXT, "
		"x_slp_packagecategories TEXT, "
		"x_slp_packageid TEXT, "
		"x_slp_uri TEXT, "
		"x_slp_svc TEXT, "
		"x_slp_exe_path TEXT, "
		"x_slp_appid TEXT, "
		"x_slp_baselayoutwidth INTEGER DEFAULT 0, "
		"x_slp_installedtime INTEGER DEFAULT 0, "
		"nodisplay INTEGER DEFAULT 0, "
		"x_slp_taskmanage INTEGER DEFAULT 1, "
		"x_slp_multiple INTEGER DEFAULT 0, "
		"x_slp_removable INTEGER DEFAULT 1, "
		"x_slp_ishorizontalscale INTEGER DEFAULT 0, "
		"x_slp_inactivated INTEGER DEFAULT 0, "
		"desktop TEXT UNIQUE NOT NULL);",
		"CREATE TABLE localname (package TEXT NOT NULL, "
		"locale TEXT NOT NULL, "
		"name TEXT NOT NULL, PRIMARY KEY (package, locale));",
		NULL
	};

	ret = db_open(DB_OPEN_RW);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_DB_FAILED);

	for (i = 0; tbls[i] != NULL; i++) {
		ret = db_exec(tbls[i]);
		retv_if(ret != AIL_ERROR_OK, AIL_ERROR_DB_FAILED);
	}

	return AIL_ERROR_OK;
}


static inline void _insert_localname(gpointer data, gpointer user_data)
{
	char query[512];

	struct name_item *item = (struct name_item *)data;
	desktop_info_s *info = (desktop_info_s *)user_data;

	snprintf(query, sizeof(query), "insert into localname (package, locale, name) "
			"values ('%s', '%s', '%s');", 
			info->package, item->locale, item->name);
	if (db_exec(query) < 0)
		_E("Failed to insert local name of package[%s]",info->package);
}

static ail_error_e _insert_desktop_info(desktop_info_s *info)
{
	char *query;
	int len;
	ail_error_e ret;

	len = _strlen_desktop_info(info) + (0x01 << 10);
	query = calloc(1, len);
	retv_if(!query, AIL_ERROR_OUT_OF_MEMORY);

	snprintf(query, len, "insert into app_info ("
		"package, "
		"exec, name, "
		"type, "
		"icon, "
		"categories, "
		"version, "
		"mimetype, "
		"x_slp_service, "
		"x_slp_packagetype, "
		"x_slp_packagecategories, "
		"x_slp_packageid, "
		"x_slp_uri, "
		"x_slp_svc, "
		"x_slp_exe_path, "
		"x_slp_appid, "
		"x_slp_baselayoutwidth, "
		"x_slp_installedtime, "
		"nodisplay, "
		"x_slp_taskmanage, "
		"x_slp_multiple, "
		"x_slp_removable, "
		"x_slp_ishorizontalscale, "
		"x_slp_inactivated, "
		"desktop) "
		"values "
		"('%s', '%s', '%s', '%s', '%s', "
		"'%s', '%s', '%s', '%s', '%s', "
		"'%s', '%s', '%s', '%s', '%s', '%s', "
		"%d, %d, %d, %d, %d, %d, "
		"%d, %d, "
		"'%s');",
		info->package,
		info->exec,
		info->name,
		info->type,
		info->icon,
		info->categories,
		info->version,
		info->mimetype,
		info->x_slp_service,
		info->x_slp_packagetype,
		info->x_slp_packagecategories,
		info->x_slp_packageid,
		info->x_slp_uri,
		info->x_slp_svc,
		info->x_slp_exe_path,
		info->x_slp_appid,
		info->x_slp_baselayoutwidth,
		info->x_slp_installedtime,
		info->nodisplay,
		info->x_slp_taskmanage,
		info->x_slp_multiple,
		info->x_slp_removable,
		info->x_slp_ishorizontalscale,
		info->x_slp_inactivated,
		info->desktop
		);

	ret = db_open(DB_OPEN_RW);
	if(ret != AIL_ERROR_OK) {
		_E("(tmp == NULL) return\n");
		free(query);
		return AIL_ERROR_DB_FAILED;
	}

	ret = db_exec(query);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_DB_FAILED);

	if (info->localname)
		g_slist_foreach(info->localname, _insert_localname, info);

	_D("Add (%s).", info->package);

	return AIL_ERROR_OK;
}



static ail_error_e _update_desktop_info(desktop_info_s *info)
{
	char *query;
	int len;

	retv_if (NULL == info, AIL_ERROR_INVALID_PARAMETER);

	if (db_open(DB_OPEN_RW) < 0) {
		return AIL_ERROR_DB_FAILED;
	}

	len = _strlen_desktop_info(info) + (0x01 << 10);
	query = calloc(1, len);
	retv_if(!query, AIL_ERROR_OUT_OF_MEMORY);

	snprintf (query, len, "update app_info set "
		"exec='%s', "
		"name='%s', "
		"type='%s', "
		"icon='%s', "
		"categories='%s', "
		"version='%s', "
		"mimetype='%s', "
		"x_slp_service='%s', "
		"x_slp_packagetype='%s', "
		"x_slp_packagecategories='%s', "
		"x_slp_packageid='%s', "
		"x_slp_uri='%s', "
		"x_slp_svc='%s', "
		"x_slp_exe_path='%s', "
		"x_slp_appid='%s', "
		"x_slp_baselayoutwidth=%d, "
		"x_slp_installedtime=%d, "
		"nodisplay=%d, "
		"x_slp_taskmanage=%d, "
		"x_slp_multiple=%d, "
		"x_slp_removable=%d, "
		"x_slp_ishorizontalscale=%d, "
		"x_slp_inactivated=%d, "
		"desktop='%s'"
		"where package='%s'",
		info->exec,
		info->name,
		info->type,
		info->icon,
		info->categories,
		info->version,
		info->mimetype,
		info->x_slp_service,
		info->x_slp_packagetype,
		info->x_slp_packagecategories,
		info->x_slp_packageid,
		info->x_slp_uri,
		info->x_slp_svc,
		info->x_slp_exe_path,
		info->x_slp_appid,
		info->x_slp_baselayoutwidth,
		info->x_slp_installedtime,
		info->nodisplay,
		info->x_slp_taskmanage,
		info->x_slp_multiple,
		info->x_slp_removable,
		info->x_slp_ishorizontalscale,
		info->x_slp_inactivated,
		info->desktop,
		info->package);

	if (db_exec(query) < 0) {
		free (query);
		return AIL_ERROR_DB_FAILED;
	}

	snprintf(query, len, "delete from localname where package = '%s'", info->package);

	if (db_exec(query) < 0) {
		free (query);
		return AIL_ERROR_DB_FAILED;
	}

	if (info->localname)
		g_slist_foreach(info->localname, _insert_localname, info);

	_D("Update (%s).", info->package);

	free(query);

	return AIL_ERROR_OK;
}



static ail_error_e _remove_package(const char* package)
{
	char *query;
	int size;

	retv_if(!package, AIL_ERROR_INVALID_PARAMETER);

	if (db_open(DB_OPEN_RW) < 0) {
		return AIL_ERROR_DB_FAILED;
	}

	size = strlen(package) + (0x01 << 10);
	query = calloc(1, size);
	retv_if(!query, AIL_ERROR_OUT_OF_MEMORY);

	snprintf(query, size, "delete from app_info where package = '%s'", package);

	if (db_exec(query) < 0) {
		free(query);
		return AIL_ERROR_DB_FAILED;
	}

	snprintf(query, size, "delete from localname where package = '%s'", package);
	_D("query=%s",query);

	if (db_exec(query) < 0) {
		free(query);
		return AIL_ERROR_DB_FAILED;
	}

	_D("Remove (%s).", package);
	free(query);

	return AIL_ERROR_OK;
}



static ail_error_e _send_db_done_noti(noti_type type, const char *package)
{
	char *type_string, *noti_string;
	int size;

	retv_if(!package, AIL_ERROR_INVALID_PARAMETER);

	switch (type) {
		case NOTI_ADD:
			type_string = "create";
			break;
		case NOTI_UPDATE:
			type_string = "update";
			break;
		case NOTI_REMOVE:
			type_string = "delete";
			break;
		default:
			return AIL_ERROR_FAIL;
	}

	size = strlen(package) + 8;
	noti_string = calloc(1, size);
	retv_if(!noti_string, AIL_ERROR_OUT_OF_MEMORY);

	snprintf(noti_string, size, "%s:%s", type_string, package);
	vconf_set_str("memory/menuscreen/desktop", noti_string);
	_D("Noti : %s", noti_string);

	free(noti_string);

	return AIL_ERROR_OK;
}


static void inline _name_item_free_func(gpointer data)
{
	struct name_item *item = (struct name_item *)data;
	if (item){
		SAFE_FREE(item->locale);
		item->locale = NULL;
		SAFE_FREE(item->name);
		item->name = NULL;
	}
	SAFE_FREE(item);
}

static void _fini_desktop_info(desktop_info_s *info)
{
	SAFE_FREE(info->exec);
	SAFE_FREE(info->name);
	SAFE_FREE(info->type);
	SAFE_FREE(info->icon);
	SAFE_FREE(info->categories);
	SAFE_FREE(info->version);
	SAFE_FREE(info->mimetype);
	SAFE_FREE(info->x_slp_service);
	SAFE_FREE(info->x_slp_packagetype);
	SAFE_FREE(info->x_slp_packagecategories);
	SAFE_FREE(info->x_slp_packageid);
	SAFE_FREE(info->x_slp_uri);
	SAFE_FREE(info->x_slp_svc);
	SAFE_FREE(info->x_slp_exe_path);
	SAFE_FREE(info->x_slp_appid);
	SAFE_FREE(info->desktop);
	if (info->localname) {
		g_slist_free_full(info->localname, _name_item_free_func);
		info->localname = NULL;
	}

	return;
}



/* Public functions */
EXPORT_API ail_error_e ail_desktop_add(const char *package)
{
	desktop_info_s info = {0,};
	ail_error_e ret;
	int count;

	retv_if(!package, AIL_ERROR_INVALID_PARAMETER);

	count = _count_all();
	if (count <= 0) {
		ret = _create_table();
		if (ret != AIL_ERROR_OK) {
			_D("Cannot create a table. Maybe there is already a table.");
		}
	}

	ret = _init_desktop_info(&info, package);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _read_desktop_info(&info);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _insert_desktop_info(&info);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _send_db_done_noti(NOTI_ADD, package);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	_fini_desktop_info(&info);

	return AIL_ERROR_OK;
}



EXPORT_API ail_error_e ail_desktop_update(const char *package)
{
	desktop_info_s info = {0,};
	ail_error_e ret;

	retv_if(!package, AIL_ERROR_INVALID_PARAMETER);

	ret = _init_desktop_info(&info, package);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _read_desktop_info(&info);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _update_desktop_info(&info);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _send_db_done_noti(NOTI_UPDATE, package);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	_fini_desktop_info(&info);

	return AIL_ERROR_OK;
}



EXPORT_API ail_error_e ail_desktop_remove(const char *package)
{
	ail_error_e ret;

	retv_if(!package, AIL_ERROR_INVALID_PARAMETER);

	ret = _remove_package(package);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _send_db_done_noti(NOTI_REMOVE, package);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	return AIL_ERROR_OK;
}


EXPORT_API ail_error_e ail_desktop_appinfo_modify_bool(const char *package,
							     const char *property,
							     bool value)
{
	desktop_info_s info = {0,};
	ail_error_e ret;
	ail_prop_bool_e prop;

	retv_if(!package, AIL_ERROR_INVALID_PARAMETER);

	retv_if(strcmp(property, AIL_PROP_X_SLP_INACTIVATED_BOOL),
		AIL_ERROR_INVALID_PARAMETER);

	ret = _init_desktop_info(&info, package);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _modify_desktop_info_bool(&info, property, value);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _update_desktop_info(&info);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _send_db_done_noti(NOTI_UPDATE, package);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	_fini_desktop_info(&info);

	return AIL_ERROR_OK;
}




// End of File
