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
#include <errno.h>
#include <glib.h>
#include <grp.h>
#include <pwd.h>
#include <sys/smack.h>
#include <sys/stat.h>
#include <tzplatform_config.h>
#include "ail_private.h"
#include "ail_db.h"

#define GLOBAL_USER tzplatform_getuid(TZ_SYS_GLOBALAPP_USER)
#define BUFSIZE 4096
#define QUERY_ATTACH "attach database '%s' as Global"
#define QUERY_CREATE_VIEW_APP "CREATE temp VIEW app_info as select distinct * from (select  * from main.app_info m union select * from Global.app_info g)"

#define QUERY_CREATE_VIEW_LOCAL "CREATE temp VIEW localname as select distinct * from (select  * from main.localname m union select * from Global.localname g)"

#define SET_SMACK_LABEL(x,uid) \
	if(smack_setlabel((x), (((uid) == GLOBAL_USER)?"*":"User"), SMACK_LABEL_ACCESS)) _E("failed chsmack -a \"User/*\" %s", x); \
	else _D("chsmack -a \"User/*\" %s", x);
	
	
#define retv_with_dbmsg_if(expr, val) do { \
	if (expr) { \
		_E("db_info.dbUserro: %s", sqlite3_errmsg(db_info.dbUserro)); \
		_E("db_info.dbGlobalro: %s", sqlite3_errmsg(db_info.dbGlobalro)); \
		_E("db_info.dbUserrw: %s", sqlite3_errmsg(db_info.dbUserrw)); \
		_E("db_info.dbGlobalrw: %s", sqlite3_errmsg(db_info.dbGlobalrw)); \
		_E("db_info.dbUserro errcode: %d", sqlite3_extended_errcode(db_info.dbUserro)); \
		_E("db_info.dbGlobalro errcode: %d", sqlite3_extended_errcode(db_info.dbGlobalro)); \
		_E("db_info.dbUserrw errcode: %d", sqlite3_extended_errcode(db_info.dbUserrw)); \
		_E("db_info.dbGlobalrw errcode: %d", sqlite3_extended_errcode(db_info.dbGlobalrw)); \
		return (val); \
	} \
} while (0)

static __thread struct {
        sqlite3         *dbUserro;
        sqlite3         *dbGlobalro;
        sqlite3         *dbUserrw;
        sqlite3         *dbGlobalrw;
} db_info = {
        .dbUserro = NULL,
        .dbGlobalro = NULL,
        .dbUserrw = NULL,
        .dbGlobalrw = NULL
};
  static __thread      sqlite3         *dbInit = NULL;

static int ail_db_change_perm(const char *db_file, uid_t uid)
{
	char buf[BUFSIZE];
	char journal_file[BUFSIZE];
	char *files[3];
	int ret, i;
	struct passwd *userinfo = NULL;
	files[0] = (char *)db_file;
	files[1] = journal_file;
	files[2] = NULL;

	retv_if(!db_file, AIL_ERROR_FAIL);
	if(getuid() != OWNER_ROOT) //At this time we should be root to apply this
			return AIL_ERROR_OK;
    userinfo = getpwuid(uid);
    if (!userinfo) {
		_E("FAIL: user %d doesn't exist", uid);
		return AIL_ERROR_FAIL;
	}
	snprintf(journal_file, sizeof(journal_file), "%s%s", db_file, "-journal");

	for (i = 0; files[i]; i++) {
		// Compare git_t type and not group name
		ret = chown(files[i], uid, userinfo->pw_gid);
		SET_SMACK_LABEL(files[i],uid)
		if (ret == -1) {
			strerror_r(errno, buf, sizeof(buf));
			_E("FAIL : chown %s %d.%d, because %s", db_file, uid, userinfo->pw_gid, buf);
			return AIL_ERROR_FAIL;
		}

		ret = chmod(files[i], S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (ret == -1) {
			strerror_r(errno, buf, sizeof(buf));
			_E("FAIL : chmod %s 0664, because %s", db_file, buf);
			return AIL_ERROR_FAIL;
		}
	}

	return AIL_ERROR_OK;
}

char* ail_get_icon_path(uid_t uid)
{
	char *result = NULL;
	struct group *grpinfo = NULL;
	char *dir = NULL;
	struct passwd *userinfo = getpwuid(uid);

	if (uid == 0) {
		_E("FAIL : Root is not allowed user! please fix it replacing with DEFAULT_USER");
		return NULL;
	}
	if (uid != GLOBAL_USER) {
		if (userinfo == NULL) {
			_E("getpwuid(%d) returns NULL !", uid);
			return NULL;
		}
		grpinfo = getgrnam("users");
		if (grpinfo == NULL) {
			_E("getgrnam(users) returns NULL !");
			return NULL;
		}
		// Compare git_t type and not group name
		if (grpinfo->gr_gid != userinfo->pw_gid) {
			_E("UID [%d] does not belong to 'users' group!", uid);
			return NULL;
		}
		asprintf(&result, "%s/.applications/icons/", userinfo->pw_dir);
	} else {
		result = tzplatform_mkpath(TZ_SYS_RW_ICONS, "/");
	}
	int ret;
	mkdir(result, S_IRWXU | S_IRGRP | S_IXGRP | S_IXOTH);
	if (getuid() == OWNER_ROOT) {
		ret = chown(result, uid, ((grpinfo)?grpinfo->gr_gid:0));
		SET_SMACK_LABEL(result,uid)
		if (ret == -1) {
			char buf[BUFSIZE];
			strerror_r(errno, buf, sizeof(buf));
			_E("FAIL : chown %s %d.%d, because %s", result, uid, ((grpinfo)?grpinfo->gr_gid:0), buf);
		}
	}
	return result;
}

char* ail_get_app_DB_journal(uid_t uid)
{

	char *app_path = ail_get_app_DB(uid);
	char* result = NULL;

	asprintf(&result, "%s-journal", app_path);
	return  result;
}

char* ail_get_app_DB(uid_t uid)
{
	char *result = NULL;
	struct group *grpinfo = NULL;
	char *dir = NULL;
	struct passwd *userinfo = getpwuid(uid);

	if (uid == 0) {
		_E("FAIL : Root is not allowed! switch to DEFAULT_USER");
		return NULL;
	}
	if (uid != GLOBAL_USER) {
		if (userinfo == NULL) {
			_E("getpwuid(%d) returns NULL !", uid);
			return NULL;
		}
		grpinfo = getgrnam("users");
		if (grpinfo == NULL) {
			_E("getgrnam(users) returns NULL !");
			return NULL;
		}
		// Compare git_t type and not group name
		if (grpinfo->gr_gid != userinfo->pw_gid) {
			_E("UID [%d] does not belong to 'users' group!", uid);
			return NULL;
		}
		asprintf(&result, "%s/.applications/dbspace/.app_info.db", userinfo->pw_dir);
	} else {
			result = strdup(APP_INFO_DB_FILE);
	}
	char *temp = strdup(result);
	dir = strrchr(temp, '/');
	if(!dir)
	{
		free(temp);
		return result;
	}
	*dir = 0;
	if ((uid != GLOBAL_USER)||((uid == GLOBAL_USER)&& (geteuid() == 0 ))) {
		int ret;
		mkdir(temp, S_IRWXU | S_IRGRP | S_IXGRP | S_IXOTH);
		if (getuid() == OWNER_ROOT) {
			ret = chown(temp, uid, ((grpinfo)?grpinfo->gr_gid:0));
			SET_SMACK_LABEL(temp,uid)
			if (ret == -1) {
				char buf[BUFSIZE];
				strerror_r(errno, buf, sizeof(buf));
				_E("FAIL : chown %s %d.%d, because %s", temp, uid, ((grpinfo)?grpinfo->gr_gid:0), buf);
			}
	}
	}
	free(temp);
	return result;
}

char* al_get_desktop_path(uid_t uid)
{
	char *result = NULL;
	struct group *grpinfo = NULL;
	char *dir = NULL;
	struct passwd *userinfo = getpwuid(uid);

	if (uid == 0) {
		_E("FAIL : Root is not allowed user! please fix it replacing with DEFAULT_USER");
		return NULL;
	}
	if (uid != GLOBAL_USER) {
		if (userinfo == NULL) {
			_E("getpwuid(%d) returns NULL !", uid);
			return NULL;
		}
		grpinfo = getgrnam("users");
		if (grpinfo == NULL) {
			_E("getgrnam(users) returns NULL !");
			return NULL;
		}
		// Compare git_t type and not group name
		if (grpinfo->gr_gid != userinfo->pw_gid) {
			_E("UID [%d] does not belong to 'users' group!", uid);
			return NULL;
		}
		asprintf(&result, "%s/.applications/desktop/", userinfo->pw_dir);
	} else {
		result = tzplatform_mkpath(TZ_SYS_RW_DESKTOP_APP, "/");
	}
	if ((uid != GLOBAL_USER)||((uid == GLOBAL_USER)&& (geteuid() == 0 ))) {
		int ret;
		mkdir(result, S_IRWXU | S_IRGRP | S_IXGRP | S_IXOTH);
		ret = chown(result, uid, ((grpinfo)?grpinfo->gr_gid:0));
		SET_SMACK_LABEL(result,uid)
		if (ret == -1) {
			char buf[BUFSIZE];
			strerror_r(errno, buf, sizeof(buf));
			_E("FAIL : chown %s %d.%d, because %s", result, uid, ((grpinfo)?grpinfo->gr_gid:0), buf);
		}
	}
	return result;
}


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

ail_error_e db_open(db_open_mode mode, uid_t uid)
{
	int ret;
	int changed = 0;
	int i;
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
		"x_slp_pkgid TEXT, "
		"x_slp_domain TEXT, "
		"x_slp_submodemainid TEXT, "
		"x_slp_installedstorage TEXT, "
		"x_slp_baselayoutwidth INTEGER DEFAULT 0, "
		"x_slp_installedtime INTEGER DEFAULT 0, "
		"nodisplay INTEGER DEFAULT 0, "
		"x_slp_taskmanage INTEGER DEFAULT 1, "
		"x_slp_multiple INTEGER DEFAULT 0, "
		"x_slp_removable INTEGER DEFAULT 1, "
		"x_slp_ishorizontalscale INTEGER DEFAULT 0, "
		"x_slp_enabled INTEGER DEFAULT 1, "
		"x_slp_submode INTEGER DEFAULT 0, "
		"desktop TEXT UNIQUE NOT NULL);",
		"CREATE TABLE localname (package TEXT NOT NULL, "
		"locale TEXT NOT NULL, "
		"name TEXT NOT NULL, "
		"x_slp_pkgid TEXT NOT NULL, PRIMARY KEY (package, locale));",

		NULL
	};

	char *db = ail_get_app_DB(uid);
	char *global_db = ail_get_app_DB(GLOBAL_USER);

	if (access(db, F_OK)) {
		if (AIL_ERROR_OK == db_util_open_with_options(db, &dbInit, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL))
		{
			for (i = 0; tbls[i] != NULL; i++) {
				ret = do_db_exec(tbls[i], dbInit);
				retv_if(ret != AIL_ERROR_OK, AIL_ERROR_DB_FAILED);
			}
			if(AIL_ERROR_OK != ail_db_change_perm(db, uid)) {
				_E("Failed to change permission\n");
			}
		} else {
			dbInit = NULL;
			_E("Failed to create table %s\n", db);
		}
	}
	if(dbInit) {
		ret = sqlite3_close(dbInit);
		retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);
		dbInit = NULL;
	}
	if(mode & DB_OPEN_RO) {
		if(uid != GLOBAL_USER) {
			if (!db_info.dbUserro) {
				db_util_open_with_options(db, &db_info.dbUserro, SQLITE_OPEN_READONLY, NULL);
				char query_attach[AIL_SQL_QUERY_MAX_LEN];
				char query_view_app[AIL_SQL_QUERY_MAX_LEN];
				char query_view_local[AIL_SQL_QUERY_MAX_LEN];
				snprintf(query_attach, AIL_SQL_QUERY_MAX_LEN, QUERY_ATTACH, global_db);
				if (db_exec_usr_ro(query_attach) < 0) {
					_D("executing query_attach : %s", query_attach );
					goto error;
				}
				snprintf(query_view_app, AIL_SQL_QUERY_MAX_LEN, QUERY_CREATE_VIEW_APP);
				if (db_exec_usr_ro(query_view_app) < 0) {
					_D("executing query_attach : %s", query_view_app );
					goto error;
				}

				snprintf(query_view_local, AIL_SQL_QUERY_MAX_LEN, QUERY_CREATE_VIEW_LOCAL);
				if (db_exec_usr_ro(query_view_local) < 0) {
					_D("executing query_attach : %s", query_view_local );
					goto error;
				}
			}
		} else {
			if (!db_info.dbGlobalro) {
				ret = db_util_open_with_options(global_db, &db_info.dbGlobalro, SQLITE_OPEN_READONLY, NULL);
				retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);
			}
		}
	}
	if(mode & DB_OPEN_RW) {
		if(uid != GLOBAL_USER) {
			if(!db_info.dbUserrw){
				ret = db_util_open(db, &db_info.dbUserrw, 0);
			}
		} else {
			if(!db_info.dbGlobalrw){
				ret = db_util_open(global_db, &db_info.dbGlobalrw, 0);
			}
		}
		retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);
	}

	free(global_db);
	free(db);

	return AIL_ERROR_OK;

error:
	free(global_db);
	free(db);

	return AIL_ERROR_DB_FAILED;
}


ail_error_e db_prepare(const char *query, sqlite3_stmt **stmt)
{
	return db_do_prepare(db_info.dbUserro, query, stmt);
}

ail_error_e db_prepare_globalro(const char *query, sqlite3_stmt **stmt)
{
	return db_do_prepare(db_info.dbGlobalro, query, stmt);
}

ail_error_e db_prepare_rw(const char *query, sqlite3_stmt **stmt)
{
	return db_do_prepare(db_info.dbUserrw, query, stmt);
}


ail_error_e db_prepare_globalrw(const char *query, sqlite3_stmt **stmt)
{
	return db_do_prepare(db_info.dbGlobalrw, query, stmt);
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



ail_error_e do_db_exec(const char *query, sqlite3 * fileSQL)
{
	int ret;
	char *errmsg = NULL;

	retv_if(!query, AIL_ERROR_INVALID_PARAMETER);
	retv_if(!fileSQL, AIL_ERROR_DB_FAILED);

	ret = sqlite3_exec(fileSQL, query, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		_E("Cannot execute this query - %s. because %s",
				query, errmsg? errmsg:"uncatched error");
		if(errmsg)
			sqlite3_free(errmsg);
		return AIL_ERROR_DB_FAILED;
	}

	return AIL_ERROR_OK;
}



ail_error_e db_exec_usr_rw(const char *query)
{
	return do_db_exec(query, db_info.dbUserrw);
}


ail_error_e db_exec_usr_ro(const char *query)
{
	return do_db_exec(query, db_info.dbUserro);
}

ail_error_e db_exec_glo_ro(const char *query)
{
	return do_db_exec(query, db_info.dbGlobalro);
}

ail_error_e db_exec_glo_rw(const char *query)
{
	return do_db_exec(query, db_info.dbGlobalrw);
}


ail_error_e db_close(void)
{
	int ret;

	if(db_info.dbUserro) {
		ret = sqlite3_close(db_info.dbUserro);
		retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);

		db_info.dbUserro = NULL;
	}
	if(db_info.dbGlobalrw) {
		ret = sqlite3_close(db_info.dbGlobalrw);
		retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);

		db_info.dbGlobalrw = NULL;
	}
	if(db_info.dbUserrw) {
		ret = sqlite3_close(db_info.dbUserrw);
		retv_with_dbmsg_if(ret != SQLITE_OK, AIL_ERROR_DB_FAILED);

		db_info.dbUserrw = NULL;
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
	if(db_info.dbGlobalro) {
		if (SQLITE_OK !=
			sqlite3_exec(db_info.dbGlobalro, query, callback, data, &error_message)) {
			_E("Don't execute query = %s error message = %s\n", query,
				error_message);
			sqlite3_free(error_message);
			return -1;
		}
	}
	if(db_info.dbUserro) {
		if (SQLITE_OK !=
			sqlite3_exec(db_info.dbUserro, query, callback, data, &error_message)) {
			_E("Don't execute query = %s error message = %s\n", query,
				error_message);
			sqlite3_free(error_message);
			return -1;
		}
	}
	sqlite3_free(error_message);
	return 0;
}

// End of file.
