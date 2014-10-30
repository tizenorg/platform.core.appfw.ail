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
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/smack.h>

#include "ail.h"
#include "ail_db.h"
#include "ail_private.h"


#ifdef _E
#undef _E
#endif
#define _E(fmt, arg...) fprintf(stderr, "[AIL_INITDB][E][%s,%d] "fmt"\n", __FUNCTION__, __LINE__, ##arg);

#ifdef _D
#undef _D
#endif
#define _D(fmt, arg...) fprintf(stderr, "[AIL_INITDB][D][%s,%d] "fmt"\n", __FUNCTION__, __LINE__, ##arg);

#define SET_DEFAULT_LABEL(x) \
	if(smack_setlabel((x), "*", SMACK_LABEL_ACCESS)) _E("failed chsmack -a \"*\" %s", x) \
	else _D("chsmack -a \"*\" %s", x)

static int initdb_user_count_app(void)
{
	ail_filter_h filter;
	ail_error_e ret;
	int total = 0;

	ret = ail_filter_new(&filter);
	if (ret != AIL_ERROR_OK) {
		return -1;
	}

	ret = ail_filter_add_bool(filter, AIL_PROP_NODISPLAY_BOOL, false);
	if (ret != AIL_ERROR_OK) {
		ail_filter_destroy(filter);
		return -1;
	}
	ret = ail_filter_count_usr_appinfo(filter, &total, getuid());
	if (ret != AIL_ERROR_OK) {
		ail_filter_destroy(filter);
		return -1;
	}

	ail_filter_destroy(filter);

	return total;
}



char* _desktop_to_package(const char* desktop)
{
	char *package, *tmp;

	retv_if(!desktop, NULL);

	package = strdup(desktop);
	retv_if(!package, NULL);

	tmp = strrchr(package, '.');
	if(tmp == NULL) {
		_E("[%s] is not a desktop file", package);
		free(package);
		return NULL;
	}

	if (strcmp(tmp, ".desktop")) {
		_E("%s is not a desktop file", desktop);
		free(package);
		return NULL;
	}

	*tmp = '\0';

	return package;
}



int initdb_user_load_directory(const char *directory)
{
	DIR *dir;
	struct dirent entry, *result;
	int len, ret;
	char buf[BUFSZE];
	int total_cnt = 0;
	int ok_cnt = 0;

	// desktop file
	dir = opendir(directory);
	if (!dir) {
		if (strerror_r(errno, buf, sizeof(buf)) == 0)
			_E("Failed to access the [%s] because %s\n", directory, buf);
		return AIL_ERROR_FAIL;
	}

	len = strlen(directory) + 1;
	_D("Loading desktop files from %s", directory);

	for (ret = readdir_r(dir, &entry, &result);
			ret == 0 && result != NULL;
			ret = readdir_r(dir, &entry, &result)) {
		char *package;

		if (entry.d_name[0] == '.') continue;
		total_cnt++;
		package = _desktop_to_package(entry.d_name);
		if (!package) {
			_E("Failed to convert file to package[%s]", entry.d_name);
			continue;
		}

		if (ail_usr_desktop_add(package, getuid()) != AIL_ERROR_OK) {
			_E("Failed to add a package[%s]", package);
		} else {
			ok_cnt++;
		}
		free(package);
	}

	_D("Application-Desktop process : Success [%d], fail[%d], total[%d] \n", ok_cnt, total_cnt-ok_cnt, total_cnt);
	closedir(dir);

	return AIL_ERROR_OK;
}

static int __is_authorized()
{
	/* ail_init db should be called by an user. */

	uid_t uid = getuid();
	if ((uid_t) OWNER_ROOT != uid)
		return 1;
	else
		return 0;
}


int main(int argc, char *argv[])
{
	int ret;

	if (!__is_authorized()) {
		fprintf(stderr, "You are not an authorized user!\n");
		_D("You are root user! Please switch to a regular user\n");
		return -1;
	}
	else {
		if(remove(ail_get_app_DB(getuid())))
			_E(" %s is not removed", ail_get_app_DB(getuid()));
		if(remove(ail_get_app_DB_journal(getuid())))
			_E(" %s is not removed", ail_get_app_DB_journal(getuid()));
	}
	ret = setenv("AIL_INITDB", "1", 1);
	_D("AIL_INITDB : %d", ret);
	ret = initdb_user_count_app();
	if (ret > 0) {
		_D("Some Apps in the App Info DB.");
	}

	ret = initdb_user_load_directory(ail_get_desktop_path(getuid()));
	if (ret == AIL_ERROR_FAIL) {
		_E("cannot load usr desktop directory.");
	}

	return AIL_ERROR_OK;
}



// END
