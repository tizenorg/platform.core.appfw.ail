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
#include <db-util.h>
#include <glib.h>
#include "ail_private.h"
#include "ail_db.h"

#define retv_with_dbmsg_if(expr, val) do { \
	if (expr) { \
		_E("db_info.dbro: %s", sqlite3_errmsg(db_info.dbro)); \
		_E("db_info.dbrw: %s", sqlite3_errmsg(db_info.dbrw)); \
		return (val); \
	} \
} while (0)


static __thread struct {
        sqlite3         *dbro;
        sqlite3         *dbrw;
} db_info = {
        .dbro = NULL,
	.dbrw = NULL
};


static ail_error_e db_do_prepare(sqlite3 *db, const char *query, sqlite3_stmt **stmt)
{
	int ret;

	retv_if(!query, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!db, AIL_ERROR_DB_FAILED);

	ret = sqlite3_prepare_v2(db, query, strlen(query), stmt, NULL);
	if (ret != SQLITE_OK) {
		_E("%s\n", sqlite3_errmsg(db));
		return AIL_ERROR_DB_FAILED;
	} else
		return AIL_ERROR_OK;
}

ail_error_e db_open(db_open_mode mode)
{
	int ret;
	int changed = 0;

	if(mode & DB_OPEN_RO) {
		if (!db_info.dbro) {
			ret = db_util_open_with_options(APP_INFO_DB, &db_info.dbro, SQLITE_OPEN_READONLY, NULL);
			retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);
		}
	}

	if(mode & DB_OPEN_RW) {
		if (!db_info.dbrw) {
			ret = db_util_open(APP_INFO_DB, &db_info.dbrw, DB_UTIL_REGISTER_HOOK_METHOD);
			retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);
		}
	}

	return AIL_ERROR_OK;
}

ail_error_e db_prepare(const char *query, sqlite3_stmt **stmt)
{
	return db_do_prepare(db_info.dbro, query, stmt);
}

ail_error_e db_prepare_rw(const char *query, sqlite3_stmt **stmt)
{
	return db_do_prepare(db_info.dbrw, query, stmt);
}


ail_error_e db_bind_bool(sqlite3_stmt *stmt, int idx, bool value)
{
	int ret;

	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);

	ret = sqlite3_bind_int(stmt, idx, (int) value);
	retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);

	return AIL_ERROR_OK;
}



ail_error_e db_bind_int(sqlite3_stmt *stmt, int idx, int value)
{
	int ret;

	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);

	ret = sqlite3_bind_int(stmt, idx, value);
	retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);

	return AIL_ERROR_OK;
}

ail_error_e db_bind_text(sqlite3_stmt *stmt, int idx, char* value)
{
	int ret;

	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);

	ret = sqlite3_bind_text(stmt, idx, value, strlen(value), 0);
	retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);

	return AIL_ERROR_OK;
}


ail_error_e db_step(sqlite3_stmt *stmt)
{
	int ret;

	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);

	ret = sqlite3_step(stmt);
	switch (ret) {
		case SQLITE_DONE:
			return AIL_ERROR_NO_DATA;
		case SQLITE_ROW:
			return AIL_ERROR_OK;
	}

	retv_with_dbmsg_if(1, AIL_ERROR_DB_FAILED);
}



ail_error_e db_column_bool(sqlite3_stmt *stmt, int index, bool *value)
{
	int out_val;

	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	out_val = sqlite3_column_int(stmt, index);
	*value = (out_val == 1)? true:false;

	return AIL_ERROR_OK;
}



ail_error_e db_column_int(sqlite3_stmt *stmt, int index, int *value)
{
	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!value, AIL_ERROR_INVALID_PARAMETER);

	*value = sqlite3_column_int(stmt, index);

	return AIL_ERROR_OK;
}



ail_error_e db_column_str(sqlite3_stmt *stmt, int index, char **str)
{
	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!str, AIL_ERROR_INVALID_PARAMETER);

	*str = (char *)sqlite3_column_text(stmt, index);

	return AIL_ERROR_OK;
}



ail_error_e db_reset(sqlite3_stmt *stmt)
{
	int ret;

	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);

	sqlite3_clear_bindings(stmt);

	ret = sqlite3_reset(stmt);
	retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);

	return AIL_ERROR_OK;
}



ail_error_e db_finalize(sqlite3_stmt *stmt)
{
	int ret;

	retv_if(!stmt, AIL_ERROR_INVALID_PARAMETER);

	ret = sqlite3_finalize(stmt);
	retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);

	return AIL_ERROR_OK;
}



ail_error_e db_exec(const char *query)
{
	int ret;
	char *errmsg;

	retv_if(!query, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!db_info.dbrw, AIL_ERROR_DB_FAILED);

	ret = sqlite3_exec(db_info.dbrw, query, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		_E("Cannot execute this query - %s. because %s",
				query, errmsg? errmsg:"uncatched error");
		sqlite3_free(errmsg);
		return AIL_ERROR_DB_FAILED;
	}

	return AIL_ERROR_OK;
}



ail_error_e db_close(void)
{
	int ret;

	if(db_info.dbro) {
		ret = sqlite3_close(db_info.dbro);
		retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);

		db_info.dbro = NULL;
	}
	if(db_info.dbrw) {
		ret = sqlite3_close(db_info.dbrw);
		retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);

		db_info.dbrw = NULL;
	}

	return AIL_ERROR_OK;
}

EXPORT_API ail_error_e ail_db_close(void)
{
	return db_close();
}

int db_exec_sqlite_query(char *query, sqlite_query_callback callback, void *data)
{
	char *error_message = NULL;
	if (SQLITE_OK !=
	    sqlite3_exec(db_info.dbro, query, callback, data, &error_message)) {
		_E("Don't execute query = %s error message = %s\n", query,
		       error_message);
		sqlite3_free(error_message);
		return -1;
	}
	sqlite3_free(error_message);
	return 0;
}

// End of file.
