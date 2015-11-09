/*
 * ail
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 * Copyright (C) 2013-2014 Intel Corporation.
 *
 * Contact: Sabera Djelti <sabera.djelti@open.eurogiciel.org>,
 * Jayoun Lee <airjany@samsung.com>, Sewook Park <sewook7.park@samsung.com>, Jaeho Lee <jaeho81.lee@samsung.com>
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

static int createdb_change_perm(const char *db_file)
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


static int __is_authorized(void)
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
	} else {
		if (remove(APP_INFO_DB_FILE))
			_E(" %s is not removed", APP_INFO_DB_FILE);
		if (remove(APP_INFO_DB_FILE_JOURNAL))
			_E(" %s is not removed", APP_INFO_DB_FILE_JOURNAL);
	}

	ret = setenv("AIL_INITDB", "1", 1);
	_D("AIL_INITDB : %d", ret);

	if (setresuid(GLOBAL_USER, GLOBAL_USER, OWNER_ROOT) != 0)
		_E("setresuid() is failed");

	if (db_open(DB_OPEN_RW, GLOBAL_USER) != AIL_ERROR_OK) {
		_E("Fail to create system databases");
		return AIL_ERROR_DB_FAILED;
	}

	if (setuid(OWNER_ROOT) != 0)
		_E("setuid() is failed.");

	ret = createdb_change_perm(APP_INFO_DB_FILE);
	if (ret == AIL_ERROR_FAIL)
		_E("cannot chown.");

	SET_DEFAULT_LABEL(APP_INFO_DB_FILE);
	SET_DEFAULT_LABEL(APP_INFO_DB_FILE_JOURNAL);

	return AIL_ERROR_OK;
}
