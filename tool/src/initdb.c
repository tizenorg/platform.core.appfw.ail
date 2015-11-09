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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/smack.h>

#include "ail.h"
#include "ail_private.h"
#include "ail_db.h"

#ifdef _E
#undef _E
#endif
#define _E(fmt, arg...) fprintf(stderr, "[AIL_INITDB][E][%s,%d] "fmt"\n", __FUNCTION__, __LINE__, ##arg)

#ifdef _D
#undef _D
#endif
#define _D(fmt, arg...) fprintf(stderr, "[AIL_INITDB][D][%s,%d] "fmt"\n", __FUNCTION__, __LINE__, ##arg)

#define SET_DEFAULT_LABEL(x) \
	do { \
		if (smack_setlabel((x), "*", SMACK_LABEL_ACCESS)) \
			_E("failed chsmack -a \"*\" %s", x); \
		else \
			_D("chsmack -a \"*\" %s", x); \
	} while (0)

char *_desktop_to_package(const char* desktop)
{
	char *package, *tmp;

	retv_if(!desktop, NULL);

	package = strdup(desktop);
	retv_if(!package, NULL);

	tmp = strrchr(package, '.');
	if (tmp == NULL) {
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

int initdb_load_directory(const char *directory)
{
	DIR *dir;
	struct dirent entry, *result;
	int ret;
	char buf[BUFSZE];
	int total_cnt = 0;
	int ok_cnt = 0;

	/* desktop file */
	dir = opendir(directory);
	if (!dir) {
		if (strerror_r(errno, buf, sizeof(buf)) == 0)
			_E("Failed to access the [%s] because %s\n", directory, buf);
		return AIL_ERROR_FAIL;
	}

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

		if (ail_desktop_add(package) != AIL_ERROR_OK)
			_E("Failed to add a package[%s]", package);
		else
			ok_cnt++;

		free(package);
	}

	_D("Application-Desktop process : Success [%d], fail[%d], total[%d] \n", ok_cnt, total_cnt-ok_cnt, total_cnt);
	closedir(dir);

	return AIL_ERROR_OK;
}

static int initdb_change_perm(const char *db_file)
{
	char journal_file[BUFSZE];
	char *files[3];
	int ret, i;

	files[0] = (char *)db_file;
	files[1] = journal_file;
	files[2] = NULL;

	retv_if(!db_file, AIL_ERROR_FAIL);

	snprintf(journal_file, sizeof(journal_file), "%s%s", db_file, "-journal");

	for (i = 0; files[i]; i++) {
		ret = chown(files[i], GLOBAL_USER, OWNER_ROOT);
		if (ret == -1) {
			_E("FAIL : chown %s %d.%d, because %d", db_file, OWNER_ROOT, OWNER_ROOT, errno);
			return AIL_ERROR_FAIL;
		}

		ret = chmod(files[i], S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (ret == -1) {
			_E("FAIL : chmod %s 0664, because %d", db_file, errno);
			return AIL_ERROR_FAIL;
		}
	}

	return AIL_ERROR_OK;
}

static int __is_authorized()
{
	/* ail_init db should be called by as root privilege. */
	uid_t uid = getuid();
	/* euid need to be root to allow smack label changes during initialization */
	/* uid_t euid = geteuid(); */
	if ((uid_t) OWNER_ROOT == uid)
		return 1;
	else
		return 0;
}

int xsystem(const char *argv[])
{
	int status = 0;
	pid_t pid;
	pid = fork();
	switch (pid) {
	case -1:
		perror("fork failed");
		return -1;
	case 0:
		/* child */
		execvp(argv[0], (char *const *)argv);
		_exit(-1);
	default:
		/* parent */
		break;
	}
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid failed");
		return -1;
	}
	if (WIFSIGNALED(status)) {
		perror("signal");
		return -1;
	}
	if (!WIFEXITED(status)) {
		/* shouldn't happen */
		perror("should not happen");
		return -1;
	}
	return WEXITSTATUS(status);
}

int main(int argc, char *argv[])
{
	int ret;

	if (!__is_authorized()) {
		fprintf(stderr, "You are not an authorized user!\n");
		_D("You are not root user!\n");
		return -1;
	} else {
		if (remove(APP_INFO_DB_FILE))
			_E(" %s is not removed", APP_INFO_DB_FILE);
		if (remove(APP_INFO_DB_FILE_JOURNAL))
			_E(" %s is not removed", APP_INFO_DB_FILE_JOURNAL);
	}
	ret = setenv("AIL_INITDB", "1", 1);
	_D("AIL_INITDB : %d", ret);

	if (setresuid(GLOBAL_USER, GLOBAL_USER, OWNER_ROOT) != 0)
		_E("setresuid() is failed.");

	if (db_open(DB_OPEN_RW, GLOBAL_USER) != AIL_ERROR_OK) {
		_E("Fail to create system databases");
		return AIL_ERROR_DB_FAILED;
	}
	ret = initdb_load_directory(USR_DESKTOP_DIRECTORY);
	if (ret == AIL_ERROR_FAIL)
		_E("cannot load usr desktop directory.");

	if (setuid(OWNER_ROOT) != 0)
		_E("setuid() is failed.");

	ret = initdb_change_perm(APP_INFO_DB_FILE);
	if (ret == AIL_ERROR_FAIL)
		_E("cannot chown.");

	SET_DEFAULT_LABEL(APP_INFO_DB_FILE);
	SET_DEFAULT_LABEL(APP_INFO_DB_FILE_JOURNAL);

	return AIL_ERROR_OK;
}
