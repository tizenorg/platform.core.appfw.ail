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



#include <string.h>
#include <stdlib.h>
#include <db-util.h>
#include <vconf.h>
#include "ail.h"
#include "ail_private.h"
#include "ail_convert.h"
#include "ail_db.h"
#include "ail_sql.h"
#include "ail_package.h"
#include <assert.h>
#include <unistd.h>

#define LANGUAGE_LENGTH 2
#define DEFAULT_LOCALE		"No Locale"
#define MAX_QUERY_LEN	4096

struct ail_appinfo {
	char **values;
	sqlite3_stmt *stmt;
};

typedef struct _pkgmgr_locale_x {
	char *locale;
} pkgmgr_locale_x;


/* get the first locale value*/
static int __fallback_locale_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_locale_x *info = (pkgmgr_locale_x *)data;

	if (ncols >= 1)
		info->locale = strdup(coltxt[0]);
	else
		info->locale = NULL;

	return 0;
}

static int __check_validation_of_qurey_cb(void *data, int ncols, char **coltxt, char **colname)
{
	int *p = (int*)data;
	*p = atoi(coltxt[0]);
	return 0;
}

static int __check_app_locale_from_app_localized_info_by_exact(const char *appid, const char *locale)
{
	int result_query = -1;
	char query[MAX_QUERY_LEN];

	snprintf(query, MAX_QUERY_LEN, "select exists(select locale from localname where package='%s' and locale='%s')", appid, locale);
	db_exec_sqlite_query(query, __check_validation_of_qurey_cb, (void *)&result_query);

	return result_query;
}

static int __check_app_locale_from_app_localized_info_by_fallback(const char *appid, const char *locale)
{
	int result_query = -1;
	char wildcard[2] = {'%','\0'};
	char query[MAX_QUERY_LEN];
	char lang[3] = {'\0'};
	strncpy(lang, locale, LANGUAGE_LENGTH);

	snprintf(query, MAX_QUERY_LEN, "select exists(select locale from localname where package='%s' and locale like '%s%s')", appid, lang, wildcard);
	db_exec_sqlite_query(query, __check_validation_of_qurey_cb, (void *)&result_query);

	return result_query;
}

static char* __get_app_locale_from_app_localized_info_by_fallback(const char *appid, const char *locale)
{
	char wildcard[2] = {'%','\0'};
	char lang[3] = {'\0'};
	char query[MAX_QUERY_LEN];
	char *locale_new = NULL;
	pkgmgr_locale_x *info = NULL;

	info = (pkgmgr_locale_x *)malloc(sizeof(pkgmgr_locale_x));
	if (info == NULL) {
		_E("Out of Memory!!!\n");
		return NULL;
	}
	memset(info, NULL, sizeof(*info));

	strncpy(lang, locale, 2);
	snprintf(query, MAX_QUERY_LEN, "select locale from localname where package='%s' and locale like '%s%s'", appid, lang, wildcard);
	db_exec_sqlite_query(query, __fallback_locale_cb, (void *)info);
	locale_new = info->locale;
	free(info);

	return locale_new;
}

static char* __convert_syslocale_to_manifest_locale(char *syslocale)
{
	char *locale = malloc(6);
	if (!locale) {
		_E("Malloc Failed\n");
		return NULL;
	}

	sprintf(locale, "%c%c_%c%c", syslocale[0], syslocale[1], toupper(syslocale[3]), toupper(syslocale[4]));
	return locale;
}

static char* __get_app_locale_by_fallback(const char *appid, const char *syslocale)
{
	assert(appid);
	assert(syslocale);

	char *locale = NULL;
	char *locale_new = NULL;
	int check_result = 0;

	locale = __convert_syslocale_to_manifest_locale(syslocale);

	/*check exact matching */
	check_result = __check_app_locale_from_app_localized_info_by_exact(appid, locale);

	/* Exact found */
	if (check_result == 1) {
		_D("%s find exact locale(%s)\n", appid, locale);
		return locale;
	}

	/* fallback matching */
	check_result = __check_app_locale_from_app_localized_info_by_fallback(appid, locale);
	if(check_result == 1) {
		   locale_new = __get_app_locale_from_app_localized_info_by_fallback(appid, locale);
		   _D("%s found (%s) language-locale in DB by fallback!\n", appid, locale_new);
		   free(locale);
		   if (locale_new == NULL)
			   locale_new =  strdup(DEFAULT_LOCALE);
		   return locale_new;
	}

	/* default locale */
	free(locale);
	_D("%s DEFAULT_LOCALE)\n", appid);
	return	strdup(DEFAULT_LOCALE);
}

void appinfo_set_stmt(ail_appinfo_h ai, sqlite3_stmt *stmt)
{
	ai->stmt = stmt;
}

ail_appinfo_h appinfo_create(void)
{
	ail_appinfo_h ai;
	ai = calloc(1, sizeof(struct ail_appinfo));
	retv_if (NULL == ai, NULL);
	ai->stmt = NULL;

	return ai;
}

void appinfo_destroy(ail_appinfo_h ai)
{
	if (ai) 
		free(ai);
}



static ail_error_e _retrieve_all_column(ail_appinfo_h ai)
{
	int i, j;
	ail_error_e err;
	char *col;

	retv_if(!ai, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!ai->stmt, AIL_ERROR_INVALID_PARAMETER);

	ai->values = calloc(NUM_OF_PROP, sizeof(char *));
	retv_if(!ai->values, AIL_ERROR_OUT_OF_MEMORY);

	for (i = 0; i < NUM_OF_PROP; i++) {
		err = db_column_str(ai->stmt, i, &col);
		if (AIL_ERROR_OK != err) 
			break;

		if (!col) {
			ai->values[i] = NULL;
		} else {
			ai->values[i] = strdup(col);
			if (!ai->values[i]) {
				err = AIL_ERROR_OUT_OF_MEMORY;
				break;
			}
		}
	}

	if (err < 0) {
		for (j = 0; j < i; ++j) {
			if (ai->values[j])
				free(ai->values[j]);
		}
		if (ai->values)
			free(ai->values);
		return err;
	} else
		return AIL_ERROR_OK;
}


EXPORT_API ail_error_e ail_package_destroy_appinfo(ail_appinfo_h ai)
{
	return ail_destroy_appinfo(ai);
}

EXPORT_API ail_error_e ail_destroy_appinfo(ail_appinfo_h ai)
{
	int i;

	retv_if(!ai, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!ai->values, AIL_ERROR_INVALID_PARAMETER);

	for (i = 0; i < NUM_OF_PROP; i++) {
		if (ai->values[i]) {
			free(ai->values[i]);
		}
	}

	free(ai->values);
	free(ai);
	db_close();

	return AIL_ERROR_OK;
}


EXPORT_API ail_error_e ail_package_get_appinfo(const char *package, ail_appinfo_h *ai)
{
	return ail_get_appinfo(package, ai);
}


EXPORT_API ail_error_e ail_get_appinfo(const char *appid, ail_appinfo_h *ai)
{
	ail_error_e ret;
	char query[AIL_SQL_QUERY_MAX_LEN];
	sqlite3_stmt *stmt = NULL;
	char w[AIL_SQL_QUERY_MAX_LEN];

	retv_if(!appid, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!ai, AIL_ERROR_INVALID_PARAMETER);

	*ai = appinfo_create();
	retv_if(!*ai, AIL_ERROR_OUT_OF_MEMORY);

	snprintf(w, sizeof(w), sql_get_filter(E_AIL_PROP_X_SLP_APPID_STR), appid);

	snprintf(query, sizeof(query), "SELECT %s FROM %s WHERE %s",SQL_FLD_APP_INFO, SQL_TBL_APP_INFO, w);

	do {
		ret = db_open(DB_OPEN_RO);
		if (ret < 0) break;

		ret = db_prepare(query, &stmt);
		if (ret < 0) break;

		ret = db_step(stmt);
		if (ret < 0) {
			db_finalize(stmt);
			break;
		}

		(*ai)->stmt = stmt;

		ret = _retrieve_all_column(*ai);
		if (ret < 0) {
			db_finalize((*ai)->stmt);
			break;
		}

		ret = db_finalize((*ai)->stmt);
		if (ret < 0) break;
		(*ai)->stmt = NULL;

		return AIL_ERROR_OK;
	} while(0);

	appinfo_destroy(*ai);

	return ret;
}


EXPORT_API ail_error_e ail_appinfo_get_bool(const ail_appinfo_h ai, const char *property, bool *value)
{
	ail_prop_bool_e prop;
	int val;

	retv_if(!ai, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!property, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	prop = _ail_convert_to_prop_bool(property);

	if (prop < E_AIL_PROP_BOOL_MIN || prop > E_AIL_PROP_BOOL_MAX)
		return AIL_ERROR_INVALID_PARAMETER;
	
	if (ai->stmt) {
		int index;
		index = sql_get_app_info_idx(prop);
		if (db_column_bool(ai->stmt, index, value) < 0)
			return AIL_ERROR_DB_FAILED;
	} else {
		val = atoi(ai->values[prop]);
		*value = (val == 0? false : true);
	}
	return AIL_ERROR_OK;
}



EXPORT_API ail_error_e ail_appinfo_get_int(const ail_appinfo_h ai, const char *property, int *value)
{
	ail_prop_int_e prop;

	retv_if(!ai, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!property, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	prop = _ail_convert_to_prop_int(property);

	if (prop < E_AIL_PROP_INT_MIN || prop > E_AIL_PROP_INT_MAX)
		return AIL_ERROR_INVALID_PARAMETER;

	if (ai->stmt) {
		int index; 
		index = sql_get_app_info_idx(prop);
		if (db_column_int(ai->stmt, index, value) < 0)
			return AIL_ERROR_DB_FAILED;
	} else
		*value = atoi(ai->values[prop]);

	return AIL_ERROR_OK;
}

#define QUERY_GET_LOCALNAME "select name from localname where package='%s' and locale='%s'"

char *appinfo_get_localname(const char *package, char *locale)
{
	db_open(DB_OPEN_RO);
	sqlite3_stmt *stmt;
	char *str = NULL;
	char *localname;
	char query[512];
	
	snprintf(query, sizeof(query), QUERY_GET_LOCALNAME, package, locale);

//	_D("Query = %s",query);
	retv_if (db_prepare(query, &stmt) < 0, NULL);

	do {
		if (db_step(stmt) < 0)
			break;
		if (db_column_str(stmt, 0, &str) < 0)
			break;
		if (str)
			localname = strdup(str);
		else
			localname = NULL;

		db_finalize(stmt);

		return localname;
	} while(0);

	db_finalize(stmt);
	return NULL;
}


EXPORT_API ail_error_e ail_appinfo_get_str(const ail_appinfo_h ai, const char *property, char **str)
{
	int index;
	char *value;
	char *pkg;
	char *pkg_type;
	char *locale, *localname;
	ail_prop_str_e prop;
	char *locale_new;

	retv_if(!ai, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!property, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!str, AIL_ERROR_INVALID_PARAMETER);

	prop = _ail_convert_to_prop_str(property);

	if (prop < E_AIL_PROP_STR_MIN || prop > E_AIL_PROP_STR_MAX)
		return AIL_ERROR_INVALID_PARAMETER;

	localname = NULL;

	if (E_AIL_PROP_NAME_STR == prop) {
		if (ai->stmt) {
			if (db_column_str(ai->stmt, E_AIL_PROP_X_SLP_PACKAGETYPE_STR, &pkg_type) < 0)
				return AIL_ERROR_DB_FAILED;
			if(pkg_type && (strcasecmp(pkg_type, "tpk") ==0))
			{
				locale = sql_get_locale();
				retv_if (NULL == locale, AIL_ERROR_FAIL);

				if (db_column_str(ai->stmt, E_AIL_PROP_PACKAGE_STR, &pkg) < 0){
					free(locale);
					return AIL_ERROR_DB_FAILED;
				}
				if (pkg == NULL){
					free(locale);
					return AIL_ERROR_DB_FAILED;
				}

				locale_new = __get_app_locale_by_fallback(pkg, locale);
				localname = (char *)appinfo_get_localname(pkg,locale_new);
				free(locale);
				free(locale_new);
			} else {
				if (db_column_str(ai->stmt, SQL_LOCALNAME_IDX, &localname) < 0)
					return AIL_ERROR_DB_FAILED;
			}
		} else {
			pkg_type = ai->values[E_AIL_PROP_X_SLP_PACKAGETYPE_STR];
			pkg = ai->values[E_AIL_PROP_PACKAGE_STR];
			retv_if (NULL == pkg, AIL_ERROR_FAIL);

			locale = sql_get_locale();
			retv_if (NULL == locale, AIL_ERROR_FAIL);

			if(pkg_type && (strcasecmp(pkg_type, "tpk") ==0))
			{
				locale_new = __get_app_locale_by_fallback(pkg, locale);
				localname = (char *)appinfo_get_localname(pkg,locale_new);
				free(locale);
				free(locale_new);
			} else {
				localname = (char *)appinfo_get_localname(pkg,locale);
				free(locale);
			}
		}

		if (localname) {
			if (!ai->stmt) {
				if (ai->values) {
					if (ai->values[prop])
						free(ai->values[prop]);
					ai->values[prop] = localname;
				}
			}
			*str = localname;
			return AIL_ERROR_OK;
		}
	}

	if (ai->stmt) {
		index = sql_get_app_info_idx(prop);
		if (db_column_str(ai->stmt, index, &value) < 0){
			return AIL_ERROR_DB_FAILED;
		}
		*str = value;
	} else
		*str = ai->values[prop];

	return AIL_ERROR_OK;
}

EXPORT_API ail_error_e ail_close_appinfo_db(void)
{
	return db_close();
}

// End of file
