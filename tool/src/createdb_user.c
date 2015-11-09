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

static int __is_authorized(void)
{
	/* ail_init db should be called by an user. */
	uid_t uid = getuid();
	if ((uid_t)OWNER_ROOT != uid)
		return 1;
	else
		return 0;
}

int main(int argc, char *argv[])
{
	int ret;
	char *db;

	if (!__is_authorized()) {
		fprintf(stderr, "You are not an authorized user!\n");
		_E("You are root user! Please switch to a regular user");
		return -1;
	}

	db = ail_get_app_DB(getuid());
	if (db) {
		if (remove(db))
			_E("%s is not removed", db);

		free(db);
	}

	db = ail_get_app_DB_journal(getuid());
	if (db) {
		if (remove(db))
			_E("%s is not removed", db);

		free(db);
	}

	ret = setenv("AIL_INITDB", "1", 1);
	_D("AIL_INITDB : %d", ret);

	if (db_open(DB_OPEN_RW, getuid()) != AIL_ERROR_OK) {
		_E("Fail to create system databases");
		return AIL_ERROR_DB_FAILED;
	}

	return AIL_ERROR_OK;
}
