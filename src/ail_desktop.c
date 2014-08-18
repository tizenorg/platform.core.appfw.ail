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
#include <errno.h>
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
#include <grp.h>
#include <pwd.h>

#include "ail_private.h"
#include "ail_db.h"
#include "ail_sql.h"
#include "ail.h"

#define BUFSIZE 4096
#define GLOBAL_USER 0

#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#define argsdelimiter	" \t"

#define SQL_INSERT_LOCALNAME_STR "insert into localname (package, locale, name) values "
#define SQL_INSERT_LOCALNAME_STR_LEN (sizeof(SQL_INSERT_LOCALNAME_STR)-1)

#define SQL_INSERT_LOCALNAME_INIT_STR  SQL_INSERT_LOCALNAME_STR"( ?, ?, ?) "

#define SQL_LOCALNAME_TRIPLET_STR  ", ( ?, ?, ?)"
#define SQL_LOCALNAME_TRIPLET_STR_LEN (sizeof(SQL_LOCALNAME_TRIPLET_STR)-1)

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
	char*		x_slp_pkgid;
	char*		x_slp_domain;
	char*		x_slp_submodemainid;
	char*		x_slp_installedstorage;
	int		x_slp_baselayoutwidth;
	int		x_slp_installedtime;
	int		nodisplay;
	int		x_slp_taskmanage;
	int		x_slp_multiple;
	int		x_slp_removable;
	int		x_slp_ishorizontalscale;
	int		x_slp_enabled;
	int		x_slp_submode;
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
_get_icon_with_path(char* icon, uid_t uid)
{
	retv_if(!icon, NULL);

	if (index(icon, '/') == NULL) {
		char* package;
		char* theme = NULL;
		char* icon_with_path = NULL;
		int len;

		package = _get_package_from_icon(icon);
		retv_if(!package, NULL);

/* "db/setting/theme" is not exist */
#if 0
		theme = vconf_get_str("db/setting/theme");
		if (!theme) {
			theme = strdup("default");
			if(!theme) {
				free(package);
				return NULL;
			}
		}
#else
		theme = strdup("default");
#endif

		len = (0x01 << 7) + strlen(icon) + strlen(package) + strlen(theme);
		icon_with_path = malloc(len);
		if(icon_with_path == NULL) {
			_E("icon_with_path == NULL\n");
			free(package);
			free(theme);
			return NULL;
		}

		memset(icon_with_path, 0, len);
		if (uid != GLOBAL_USER)
			sqlite3_snprintf( len, icon_with_path, "%s%q", ail_get_icon_path(uid), icon);
		else
			sqlite3_snprintf( len, icon_with_path, "%s/%q/small/%q", ail_get_icon_path(GLOBAL_USER), theme, icon);
		if (!access (icon_with_path, F_OK))
			sqlite3_snprintf( len, icon_with_path, "%s/%q/res/icons/%q/small/%q", tzplatform_getenv(TZ_SYS_RO_APP), package, theme, icon);
		else if (!access (icon_with_path, F_OK))
			sqlite3_snprintf( len, icon_with_path, "%s/%q/res/icons/%q/small/%q", tzplatform_getenv(TZ_SYS_RW_APP), package, theme, icon);
		else
			_D("Cannot find icon path");
		free(theme);
		free(package);
		_D("Icon path : %s", icon_with_path);


		return icon_with_path;
	} else {
		char* confirmed_icon = NULL;

		confirmed_icon = strdup(icon);
		retv_if(!confirmed_icon, NULL);
		return confirmed_icon;
	}
}


static ail_error_e _read_icon(void *data, char *tag, char *value, uid_t uid)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	info->icon = _get_icon_with_path(value, uid);

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

static ail_error_e _read_x_slp_submodemainid(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_submodemainid);
	retv_if(!info->x_slp_submodemainid, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}

static ail_error_e _read_x_slp_installedstorage(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_installedstorage);
	retv_if(!info->x_slp_installedstorage, AIL_ERROR_OUT_OF_MEMORY);

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


static ail_error_e _read_x_slp_submode(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	info->x_slp_submode = !strcasecmp(value, "true");

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


static ail_error_e _read_x_slp_pkgid(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_pkgid);
	retv_if(!info->x_slp_pkgid, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}


static ail_error_e _read_x_slp_domain(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	SAFE_FREE_AND_STRDUP(value, info->x_slp_domain);
	retv_if(!info->x_slp_appid, AIL_ERROR_OUT_OF_MEMORY);

	return AIL_ERROR_OK;
}


static ail_error_e _read_x_slp_enabled(void *data, char *tag, char *value)
{
	desktop_info_s *info = data;

	retv_if(!data, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	info->x_slp_enabled = !strcasecmp(value, "true");

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
		.field = "x-tizen-submodemainid",
		.value_cb = _read_x_slp_submodemainid,
	},
	{
		.field = "x-tizen-installedstorage",
		.value_cb = _read_x_slp_installedstorage,
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
		.field = "x-tizen-enabled",
		.value_cb = _read_x_slp_enabled,
	},
	{
		.field = "x-tizen-submode",
		.value_cb = _read_x_slp_submode,
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
		.field = "x-tizen-pkgid",
		.value_cb = _read_x_slp_pkgid,
	},
	{
		.field = "x-tizen-domain",
		.value_cb = _read_x_slp_domain,
	},
	{
		.field = "x-tizen-enabled",
		.value_cb = _read_x_slp_domain,
	},
	{
		.field = NULL,
		.value_cb = NULL,
	},
};



/* Utility functions */
static int _count_all(uid_t uid)
{
	ail_error_e ret;
	int count;

	if (uid != GLOBAL_USER)
		ret = ail_filter_count_usr_appinfo(NULL, &count, uid);
	else
		ret = ail_filter_count_appinfo(NULL, &count);	
	if(ret != AIL_ERROR_OK) {
		_E("cannot count appinfo");
		count = -1;
	}

	retv_if(ret != AIL_ERROR_OK, -1);

	return count;
}

char *_pkgname_to_desktop(const char *package, uid_t uid)
{
	char *desktop;
	char *desktop_path;
	int size;

	retv_if(!package, NULL);

  desktop_path = al_get_desktop_path(uid);

	size = strlen(desktop_path) + strlen(package) + 10;
	desktop = malloc(size);
	retv_if(!desktop, NULL);

  snprintf(desktop, size, "%s/%s.desktop", desktop_path, package);

  _D("uid: %d / desktop: [%s]\n",  uid, desktop);

	return desktop;
}

static inline int _bind_local_info(desktop_info_s* info, sqlite3_stmt * stmt)
{
	int ret = 0;
	unsigned long i = 0;
	struct name_item *item;
	GSList*	localname;
	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!info->localname, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);
	localname = info->localname;
	while (localname) {
		item = (struct name_item *)	localname->data;
		if (item && item->locale && item->name)	{
			// Bind values for a triplet : package, locale, name
			retv_if(db_bind_text(stmt, i+1, info->package) != AIL_ERROR_OK, AIL_ERROR_DB_FAILED);
			retv_if(db_bind_text(stmt, i+2, item->locale) != AIL_ERROR_OK, AIL_ERROR_DB_FAILED);
			retv_if(db_bind_text(stmt, i+3, item->name) != AIL_ERROR_OK, AIL_ERROR_DB_FAILED);
			i += 3;
		}
		localname = g_slist_next(localname);
	}
	return AIL_ERROR_OK;
}


static inline int _len_local_info(desktop_info_s* info)
{
	int len = 0;
	struct name_item *item;
	GSList*	localname;
	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);
	if(info->localname)	{
		localname = info->localname;
		while (localname) {
			item = (struct name_item *)	localname->data;
			if (item && item->locale && item->name)
				len ++;
			localname = g_slist_next(localname);
		}
	}
	return len;
}


static inline int _insert_local_info(desktop_info_s* info, uid_t uid)
{
	int len_query = SQL_INSERT_LOCALNAME_STR_LEN;
	int nb_locale_args;
	char *query;
	int ret = AIL_ERROR_OK;
	sqlite3_stmt *stmt = NULL;
	int i = 0;
	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!info->localname, AIL_ERROR_INVALID_PARAMETER);

	nb_locale_args = _len_local_info(info);

	retv_if(!nb_locale_args, AIL_ERROR_INVALID_PARAMETER);

	len_query += SQL_LOCALNAME_TRIPLET_STR_LEN*nb_locale_args +1;

	query = (char *) malloc(len_query);
	retv_if(!query, AIL_ERROR_OUT_OF_MEMORY);
	stpncpy(query, SQL_INSERT_LOCALNAME_INIT_STR, len_query);
	for (i = 0; i <  nb_locale_args - 1; i++)
		strcat(query, SQL_LOCALNAME_TRIPLET_STR);

	do {
		if(uid != GLOBAL_USER)
			ret = db_prepare_rw(query, &stmt);
		else 
			ret = db_prepare_globalrw(query, &stmt);
		if (ret < 0) break;

		ret = _bind_local_info(info, stmt);
		if (ret < 0) {
			_E("Can't bind locale information to this query - %s. ",query);
			db_finalize(stmt);
			break;
		}
		ret = db_step(stmt);
		if (ret != AIL_ERROR_NO_DATA) {
			/* Insert Request doesn't return any data.
			 * db_step should returns AIL_ERROR_NO_DATA in this case. */
			_E("Can't execute this query - %s. ",query);
			db_finalize(stmt);
			break;
		}
		ret = db_finalize(stmt);
	} while(0);

	free(query);
	return ret;
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
	if (info->x_slp_submodemainid) len += strlen(info->x_slp_submodemainid);
	if (info->x_slp_installedstorage) len += strlen(info->x_slp_installedstorage);

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
static ail_error_e _init_desktop_info(desktop_info_s *info, const char *package, uid_t uid)
{
	static int is_initdb = -1;

  _D("package - [%s].", package);

	if(is_initdb == -1)
		is_initdb = __is_ail_initdb();

	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!package, AIL_ERROR_INVALID_PARAMETER);

	/* defaults */
	info->package = package;

	info->x_slp_taskmanage = 1;
	info->x_slp_removable = 1;
	info->x_slp_submode = 0;

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

	info->x_slp_enabled = 1;

	info->desktop = _pkgname_to_desktop(package, uid);
	retv_if(!info->desktop, AIL_ERROR_FAIL);

  _D("desktop - [%s].", info->desktop);

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

		if (!field || !field_name || !tag || !value) {
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
			if (!g_ascii_strcasecmp(entry_parsers[idx].field, field_name) && entry_parsers[idx].value_cb) {
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


static ail_error_e _retrieve_all_column_to_desktop_info(desktop_info_s* info, sqlite3_stmt *stmt)
{
	int i, j;
	ail_error_e err;
	char **values;
	char *col;

	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);

	values = calloc(NUM_OF_PROP, sizeof(char *));
	retv_if(!values, AIL_ERROR_OUT_OF_MEMORY);

	for (i = 0; i < NUM_OF_PROP; i++) {
		err = db_column_str(stmt, i, &col);
		if (AIL_ERROR_OK != err)
			break;

		if (!col) {
			values[i] = NULL;
		} else {
			values[i] = strdup(col);
			if (!values[i]) {
				err = AIL_ERROR_OUT_OF_MEMORY;
				goto NEXT;
			}
		}
	}

	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_EXEC_STR], info->exec);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_NAME_STR], info->name);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_TYPE_STR], info->type);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_ICON_STR], info->icon);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_CATEGORIES_STR], info->categories);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_VERSION_STR], info->version);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_MIMETYPE_STR], info->mimetype);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_SERVICE_STR], info->x_slp_service);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_PACKAGETYPE_STR], info->x_slp_packagetype);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_PACKAGECATEGORIES_STR], info->x_slp_packagecategories);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_PACKAGEID_STR], info->x_slp_packageid);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_URI_STR], info->x_slp_uri);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_SVC_STR], info->x_slp_svc);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_EXE_PATH], info->x_slp_exe_path);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_APPID_STR], info->x_slp_appid);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_PKGID_STR], info->x_slp_pkgid);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_DOMAIN_STR], info->x_slp_domain);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_SUBMODEMAINID_STR], info->x_slp_submodemainid);
	SAFE_FREE_AND_STRDUP(values[E_AIL_PROP_X_SLP_INSTALLEDSTORAGE_STR], info->x_slp_installedstorage);

	info->x_slp_installedtime = atoi(values[E_AIL_PROP_X_SLP_INSTALLEDTIME_INT]);

	info->nodisplay = atoi(values[E_AIL_PROP_NODISPLAY_BOOL]);
	info->x_slp_taskmanage = atoi(values[E_AIL_PROP_X_SLP_TASKMANAGE_BOOL]);
	info->x_slp_multiple = atoi(values[E_AIL_PROP_X_SLP_MULTIPLE_BOOL]);
	info->x_slp_removable = atoi(values[E_AIL_PROP_X_SLP_REMOVABLE_BOOL]);
	info->x_slp_ishorizontalscale = atoi(values[E_AIL_PROP_X_SLP_ISHORIZONTALSCALE_BOOL]);
	info->x_slp_enabled = atoi(values[E_AIL_PROP_X_SLP_ENABLED_BOOL]);
	info->x_slp_submode = atoi(values[E_AIL_PROP_X_SLP_SUBMODE_BOOL]);

	err = AIL_ERROR_OK;

NEXT:
	for (j = 0; j < i; ++j) {
		if (values[j])
			free(values[j]);
	}
	if (values)
		free(values);
	return err;
}


static ail_error_e _load_desktop_info(desktop_info_s* info, uid_t uid)
{
	ail_error_e ret;
	char query[AIL_SQL_QUERY_MAX_LEN];
	sqlite3_stmt *stmt = NULL;
	char w[AIL_SQL_QUERY_MAX_LEN];

	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);

	snprintf(w, sizeof(w), sql_get_filter(E_AIL_PROP_X_SLP_APPID_STR), info->package);

	snprintf(query, sizeof(query), "SELECT %s FROM %s WHERE %s",SQL_FLD_APP_INFO, SQL_TBL_APP_INFO, w);

	do {
		ret = db_open(DB_OPEN_RO, uid);
		if (ret < 0) break;
//is_admin
		ret = db_prepare(query, &stmt);
		//ret = db_prepare_globalro(query, &stmt);
		if (ret < 0) break;

		ret = db_step(stmt);
		if (ret < 0) {
			db_finalize(stmt);
			break;
		}

		ret = _retrieve_all_column_to_desktop_info(info, stmt);
		if (ret < 0) {
			db_finalize(stmt);
			break;
		}

		ret = db_finalize(stmt);
		if (ret < 0) break;

		return AIL_ERROR_OK;
	} while(0);

	return ret;
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
		case E_AIL_PROP_X_SLP_ENABLED_BOOL:
			info->x_slp_enabled = (int)value;
			break;
		default:
			return AIL_ERROR_FAIL;
	}

	return AIL_ERROR_OK;
}


static ail_error_e _modify_desktop_info_str(desktop_info_s* info,
						  const char *property,
						  const char *value)
{
	ail_prop_bool_e prop;
	int val;

	retv_if(!info, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!property, AIL_ERROR_INVALID_PARAMETER);

	prop = _ail_convert_to_prop_str(property);

	if (prop < E_AIL_PROP_STR_MIN || prop > E_AIL_PROP_STR_MAX)
		return AIL_ERROR_INVALID_PARAMETER;

	switch (prop) {
		case E_AIL_PROP_NAME_STR:
			SAFE_FREE_AND_STRDUP(value, info->name);
			retv_if (!info->name, AIL_ERROR_OUT_OF_MEMORY);
			break;
		case E_AIL_PROP_X_SLP_SVC_STR:
			SAFE_FREE_AND_STRDUP(value, info->x_slp_svc);
			retv_if (!info->x_slp_svc, AIL_ERROR_OUT_OF_MEMORY);
			break;
		case E_AIL_PROP_X_SLP_INSTALLEDSTORAGE_STR:
			SAFE_FREE_AND_STRDUP(value, info->x_slp_installedstorage);
			retv_if (!info->x_slp_installedstorage, AIL_ERROR_OUT_OF_MEMORY);
			break;
		default:
			_E("prop[%d] is not defined\n", prop);
			return AIL_ERROR_FAIL;
	}

	return AIL_ERROR_OK;
}


static inline void _insert_localname(gpointer data, gpointer user_data, uid_t uid)
{
	char query[512];

	struct name_item *item = (struct name_item *)data;
	desktop_info_s *info = (desktop_info_s *)user_data;

	sqlite3_snprintf(sizeof(query), query, "insert into localname (package, locale, name, x_slp_pkgid) "
			"values ('%q', '%q', '%q', '%q');",
			info->package, item->locale, item->name, info->x_slp_pkgid);
	if(uid != GLOBAL_USER) {
		if (db_exec_usr_rw(query) < 0)
			_E("Failed to insert local name of package[%s]",info->package);
	} else {
		if (db_exec_glo_rw(query) < 0)
			_E("Failed to insert local name of package[%s]",info->package);
	}
}

static ail_error_e _insert_desktop_info(desktop_info_s *info, uid_t uid)
{
	char *query;
	int len;
	ail_error_e ret;

	len = _strlen_desktop_info(info) + (0x01 << 10);
	query = calloc(1, len);
	retv_if(!query, AIL_ERROR_OUT_OF_MEMORY);

	sqlite3_snprintf(len, query, "insert into app_info ("
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
		"x_slp_pkgid, "
		"x_slp_domain, "
		"x_slp_submodemainid, "
		"x_slp_installedstorage, "
		"x_slp_baselayoutwidth, "
		"x_slp_installedtime, "
		"nodisplay, "
		"x_slp_taskmanage, "
		"x_slp_multiple, "
		"x_slp_removable, "
		"x_slp_ishorizontalscale, "
		"x_slp_enabled, "
		"x_slp_submode, "
		"desktop) "
		"values "
		"('%q', '%q', '%q', '%q', '%q', "
		"'%q', '%q', '%q', '%q', '%q', "
		"'%q', '%q', '%q', '%q', '%q', "
		"'%q', '%q', '%q', '%q', '%q', "
		"%d, %d, %d, %d, %d, %d, %d,"
		"%d, %d, "
		"'%q');",
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
		info->x_slp_pkgid,
		info->x_slp_domain,
		info->x_slp_submodemainid,
		info->x_slp_installedstorage,
		info->x_slp_baselayoutwidth,
		info->x_slp_installedtime,
		info->nodisplay,
		info->x_slp_taskmanage,
		info->x_slp_multiple,
		info->x_slp_removable,
		info->x_slp_ishorizontalscale,
		info->x_slp_enabled,
		info->x_slp_submode,
		info->desktop
		);

	ret = db_open(DB_OPEN_RW, uid);
	if(ret != AIL_ERROR_OK) {
		_E("(tmp == NULL) return\n");
		free(query);
		return AIL_ERROR_DB_FAILED;
	}
	if (uid != GLOBAL_USER)
		ret = db_exec_usr_rw(query);
	else
		ret = db_exec_glo_rw(query);
	
	free(query);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_DB_FAILED);

	if (info->localname)
		_insert_local_info(info, uid);

	_D("Add (%s).", query);

	return AIL_ERROR_OK;
}



static ail_error_e _update_desktop_info(desktop_info_s *info, uid_t uid)
{
	char *query;
	int len;

	retv_if (NULL == info, AIL_ERROR_INVALID_PARAMETER);

	if (db_open(DB_OPEN_RW, uid) < 0) {
		return AIL_ERROR_DB_FAILED;
	}

	len = _strlen_desktop_info(info) + (0x01 << 10);
	query = calloc(1, len);
	retv_if(!query, AIL_ERROR_OUT_OF_MEMORY);

	sqlite3_snprintf ( len, query, "update app_info set "
		"exec='%q', "
		"name='%q', "
		"type='%q', "
		"icon='%q', "
		"categories='%q', "
		"version='%q', "
		"mimetype='%q', "
		"x_slp_service='%q', "
		"x_slp_packagetype='%q', "
		"x_slp_packagecategories='%q', "
		"x_slp_packageid='%q', "
		"x_slp_uri='%q', "
		"x_slp_svc='%q', "
		"x_slp_exe_path='%q', "
		"x_slp_appid='%q', "
		"x_slp_pkgid='%q', "
		"x_slp_domain='%q', "
		"x_slp_submodemainid='%q', "
		"x_slp_installedstorage='%q', "
		"x_slp_baselayoutwidth=%d, "
		"x_slp_installedtime=%d, "
		"nodisplay=%d, "
		"x_slp_taskmanage=%d, "
		"x_slp_multiple=%d, "
		"x_slp_removable=%d, "
		"x_slp_ishorizontalscale=%d, "
		"x_slp_enabled=%d, "
		"x_slp_submode=%d, "
		"desktop='%q'"
		"where package='%q'",
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
		info->x_slp_pkgid,
		info->x_slp_domain,
		info->x_slp_submodemainid,
		info->x_slp_installedstorage,
		info->x_slp_baselayoutwidth,
		info->x_slp_installedtime,
		info->nodisplay,
		info->x_slp_taskmanage,
		info->x_slp_multiple,
		info->x_slp_removable,
		info->x_slp_ishorizontalscale,
		info->x_slp_enabled,
		info->x_slp_submode,
		info->desktop,
		info->package);

	if(uid != GLOBAL_USER) {
		if (db_exec_usr_rw(query) < 0) {
			free (query);
			return AIL_ERROR_DB_FAILED;
		}
	} else {
		if (db_exec_glo_rw(query) < 0) {
			free (query);
			return AIL_ERROR_DB_FAILED;
		}
	}
	snprintf(query, len, "delete from localname where package = '%s'", info->package);
	if (uid != GLOBAL_USER) {
		if (db_exec_usr_rw(query) < 0) {
			free (query);
			return AIL_ERROR_DB_FAILED;
		}
	} else {
		if (db_exec_glo_rw(query) < 0) {
			free (query);
			return AIL_ERROR_DB_FAILED;
		}
	}
	if (info->localname)
		_insert_local_info(info, uid);

	_D("Update (%s).", info->package);

	free(query);

	return AIL_ERROR_OK;
}



static ail_error_e _remove_package(const char* package, uid_t uid)
{
	char *query;
	int size;

	retv_if(!package, AIL_ERROR_INVALID_PARAMETER);

	if (db_open(DB_OPEN_RW, uid) < 0) {
		return AIL_ERROR_DB_FAILED;
	}

	size = strlen(package) + (0x01 << 10);
	query = calloc(1, size);
	retv_if(!query, AIL_ERROR_OUT_OF_MEMORY);

	snprintf(query, size, "delete from app_info where package = '%s'", package);

	if(uid != GLOBAL_USER) {
		if (db_exec_usr_rw(query) < 0) {
			free(query);
			return AIL_ERROR_DB_FAILED;
		}
	} else {
		if (db_exec_glo_rw(query) < 0) {
			free(query);
			return AIL_ERROR_DB_FAILED;
		}
	}
	snprintf(query, size, "delete from localname where package = '%s'", package);
	_D("query=%s",query);
	
	if(uid != GLOBAL_USER) {
		if (db_exec_usr_rw(query) < 0) {
			free(query);
			return AIL_ERROR_DB_FAILED;
		}
	} else {
		if (db_exec_glo_rw(query) < 0) {
			free(query);
			return AIL_ERROR_DB_FAILED;
		}
	}
	_D("Remove (%s).", package);
	free(query);

	return AIL_ERROR_OK;
}

static ail_error_e _clean_pkgid_data(const char* pkgid, uid_t uid)
{
	char *query;
	int size;

	retv_if(!pkgid, AIL_ERROR_INVALID_PARAMETER);

	if (db_open(DB_OPEN_RW, uid) ){
		return AIL_ERROR_DB_FAILED;
	}

	size = strlen(pkgid) + (0x01 << 10);
	query = calloc(1, size);
	retv_if(!query, AIL_ERROR_OUT_OF_MEMORY);

	snprintf(query, size, "delete from app_info where x_slp_pkgid = '%s'", pkgid);

	if(uid != GLOBAL_USER) {
		if (db_exec_usr_rw(query) < 0) {
			free(query);
			return AIL_ERROR_DB_FAILED;
		}
	} else {
		if (db_exec_glo_rw(query) < 0) {
			free(query);
			return AIL_ERROR_DB_FAILED;
		}
	}
	snprintf(query, size, "delete from localname where x_slp_pkgid = '%s'", pkgid);
	_D("query=%s",query);

	if(uid != GLOBAL_USER) {
		if (db_exec_usr_rw(query) < 0) {
			free(query);
			return AIL_ERROR_DB_FAILED;
		}
	} else {
		if (db_exec_glo_rw(query) < 0) {
			free(query);
			return AIL_ERROR_DB_FAILED;
		}	
	}
	_D("Clean pkgid data (%s).", pkgid);
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

	size = snprintf(NULL, 0, "%s:%s:%u", type_string, package, getuid());
	noti_string = (char*) calloc(size + 1, sizeof(char));
	retv_if(!noti_string, AIL_ERROR_OUT_OF_MEMORY);

	snprintf(noti_string, size + 1, "%s:%s:%u", type_string, package, getuid());
	vconf_set_str(VCONFKEY_AIL_INFO_STATE, noti_string);
	vconf_set_str(VCONFKEY_MENUSCREEN_DESKTOP, noti_string); // duplicate, will be removed
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
	SAFE_FREE(info->x_slp_pkgid);
	SAFE_FREE(info->x_slp_domain);
	SAFE_FREE(info->x_slp_submodemainid);
	SAFE_FREE(info->x_slp_installedstorage);
	SAFE_FREE(info->desktop);
	if (info->localname) {
		g_slist_free_full(info->localname, _name_item_free_func);
		info->localname = NULL;
	}

	return;
}

static int __is_authorized()
{
	uid_t uid = getuid();
	if ((uid_t) 0 == uid )
		return 1;
	else
		return 0;
}


/* Public functions */
EXPORT_API ail_error_e ail_usr_desktop_add(const char *appid, uid_t uid)
{
	desktop_info_s info = {0,};
	ail_error_e ret;

	retv_if(!appid, AIL_ERROR_INVALID_PARAMETER);

	ret = _init_desktop_info(&info, appid, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _read_desktop_info(&info);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _insert_desktop_info(&info, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _send_db_done_noti(NOTI_ADD, appid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	_fini_desktop_info(&info);

	return AIL_ERROR_OK;
}

EXPORT_API ail_error_e ail_desktop_add(const char *appid)
{
	return ail_usr_desktop_add(appid,GLOBAL_USER);
}

EXPORT_API ail_error_e ail_usr_desktop_update(const char *appid, uid_t uid)
{
	desktop_info_s info = {0,};
	ail_error_e ret;

	retv_if(!appid, AIL_ERROR_INVALID_PARAMETER);

	ret = _init_desktop_info(&info, appid, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _read_desktop_info(&info);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _update_desktop_info(&info, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _send_db_done_noti(NOTI_UPDATE, appid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	_fini_desktop_info(&info);

	return AIL_ERROR_OK;
}

EXPORT_API ail_error_e ail_desktop_update(const char *appid)
{
	return ail_usr_desktop_update(appid,GLOBAL_USER);
}


EXPORT_API ail_error_e ail_usr_desktop_remove(const char *appid, uid_t uid)
{
	ail_error_e ret;

	retv_if(!appid, AIL_ERROR_INVALID_PARAMETER);

	ret = _remove_package(appid, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _send_db_done_noti(NOTI_REMOVE, appid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	return AIL_ERROR_OK;
}

EXPORT_API ail_error_e ail_desktop_remove(const char *appid)
{
	return ail_usr_desktop_remove(appid, GLOBAL_USER);
}


EXPORT_API ail_error_e ail_usr_desktop_clean(const char *pkgid, uid_t uid)
{
	ail_error_e ret;

	retv_if(!pkgid, AIL_ERROR_INVALID_PARAMETER);

	_D("ail_desktop_clean=%s",pkgid);

	ret = _clean_pkgid_data(pkgid, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	return AIL_ERROR_OK;
}

EXPORT_API ail_error_e ail_desktop_clean(const char *pkgid)
{
	return ail_usr_desktop_clean(pkgid, GLOBAL_USER);
}

EXPORT_API ail_error_e ail_usr_desktop_fota(const char *appid, uid_t uid)
{
	desktop_info_s info = {0,};
	ail_error_e ret;

	retv_if(!appid, AIL_ERROR_INVALID_PARAMETER);

	ret = _init_desktop_info(&info, appid, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _read_desktop_info(&info);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _insert_desktop_info(&info, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	_fini_desktop_info(&info);

	return AIL_ERROR_OK;
}

EXPORT_API ail_error_e ail_desktop_fota(const char *appid)
{
	return ail_usr_desktop_fota(appid, GLOBAL_USER);
}


EXPORT_API ail_error_e ail_desktop_appinfo_modify_usr_bool(const char *appid,
							     const char *property,
							     bool value,
							     bool broadcast, uid_t uid)
{
	desktop_info_s info = {0,};
	ail_error_e ret;

	retv_if(!appid, AIL_ERROR_INVALID_PARAMETER);

	retv_if(strcmp(property, AIL_PROP_X_SLP_ENABLED_BOOL),
		AIL_ERROR_INVALID_PARAMETER);

	ret = _init_desktop_info(&info, appid, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _load_desktop_info(&info, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _modify_desktop_info_bool(&info, property, value);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _update_desktop_info(&info, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	if (broadcast) {
		ret = _send_db_done_noti(NOTI_UPDATE, appid);
		retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);
	}

	_fini_desktop_info(&info);

	return AIL_ERROR_OK;
}

EXPORT_API ail_error_e ail_desktop_appinfo_modify_bool(const char *appid,
							     const char *property,
							     bool value,
							     bool broadcast)
{
	return ail_desktop_appinfo_modify_usr_bool(appid, property, value, broadcast,
			GLOBAL_USER);
}


EXPORT_API ail_error_e ail_desktop_appinfo_modify_usr_str(const char *appid, uid_t uid,
							     const char *property,
							     const char *value,
							     bool broadcast)
{
	desktop_info_s info = {0,};
	ail_error_e ret;

	retv_if(!appid, AIL_ERROR_INVALID_PARAMETER);

	ret = _init_desktop_info(&info, appid, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	ret = _load_desktop_info(&info, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	_D("info.name [%s], value [%s]", info.name, value);
	ret = _modify_desktop_info_str(&info, property, value);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);
	_D("info.name [%s], value [%s]", info.name, value);

	ret = _update_desktop_info(&info, uid);
	retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);

	if (broadcast) {
		ret = _send_db_done_noti(NOTI_UPDATE, appid);
		retv_if(ret != AIL_ERROR_OK, AIL_ERROR_FAIL);
	}

	_fini_desktop_info(&info);

	return AIL_ERROR_OK;
}

EXPORT_API ail_error_e ail_desktop_appinfo_modify_str(const char *appid,
							     const char *property,
							     const char *value,
							     bool broadcast)
{
	return ail_desktop_appinfo_modify_usr_str(appid, GLOBAL_USER, property, value,
				broadcast);
}

// End of File
