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
#include <stdio.h>
#include <stdlib.h>
#include "ail.h"
#include "ail_sql.h"
#include "ail_db.h"
#include "vconf.h"
#include "ail_private.h"

static const char *filter[] = {
	"app_info.PACKAGE='%s'",
	"app_info.EXEC='%s'",
	"((localname.name is NULL and app_info.name like '%%%s%%') or (localname.name like '%%%s%%'))",
	"app_info.TYPE like '%%%s%%'",
	"app_info.ICON='%s'",
	"app_info.CATEGORIES like '%%%s%%'",
	"app_info.VERSION='%s'",
	"app_info.MIMETYPE like '%%%s%%'",
	"app_info.X_SLP_SERVICE like '%%%s%%'",
	"app_info.X_SLP_PACKAGETYPE='%s'",
	"app_info.X_SLP_PACKAGECATEGORIES like '%%%s%%'",
	"app_info.X_SLP_PACKAGEID='%s'",
	"app_info.X_SLP_URI='%s'",
	"app_info.X_SLP_SVC like '%%%s%%'",
	"app_info.X_SLP_EXE_PATH='%s'",
	"app_info.X_SLP_APPID='%s'",
	"app_info.EXEC='%s' and app_info.NODISPLAY=0",  //add one extra item for the case like /usr/bin/dialer to dialer and phone
	"app_info.X_SLP_BASELAYOUTWIDTH=%d",
	"app_info.X_SLP_INSTALLEDTIME=%d",
	"app_info.NODISPLAY=%d",
	"app_info.X_SLP_TASKMANAGE=%d",
	"app_info.X_SLP_MULTIPLE=%d",
	"app_info.X_SLP_REMOVABLE=%d",
	"app_info.X_SLP_ISHORIZONTALSCALE=%d",
	"app_info.X_SLP_INACTIVATED=%d",
	NULL,
};


inline const char *sql_get_filter(int prop)
{
	retv_if(prop < 0 || prop >= NUM_OF_PROP , NULL);
	return filter[prop];
}


inline char *sql_get_locale(void)
{
	char *l;
	char *r;
	char buf[6];

	retv_if ((l = vconf_get_str(VCONFKEY_LANGSET)) == NULL, NULL);
	snprintf(buf, sizeof(buf), "%s", l);
	free(l);

	r = strdup(buf);

	return r;
}

inline int sql_get_app_info_idx(int prop)
{
	return prop;
}

// End of file
