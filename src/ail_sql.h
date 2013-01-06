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



#ifndef __AIL_SQL_H__
#define __AIL_SQL_H__

#include "ail.h"

#define SQL_TBL_APP_INFO "app_info"
#define SQL_TBL_APP_INFO_WITH_LOCALNAME "app_info LEFT OUTER JOIN localname " \
					"ON app_info.package=localname.package " \
					"and locale='%s'"

#define SQL_FLD_APP_INFO "app_info.PACKAGE," \
			"app_info.EXEC," \
			"app_info.NAME," \
			"app_info.TYPE," \
			"app_info.ICON," \
			"app_info.CATEGORIES," \
			"app_info.VERSION," \
			"app_info.MIMETYPE," \
			"app_info.X_SLP_SERVICE," \
			"app_info.X_SLP_PACKAGETYPE," \
			"app_info.X_SLP_PACKAGECATEGORIES," \
			"app_info.X_SLP_PACKAGEID," \
			"app_info.X_SLP_URI," \
			"app_info.X_SLP_SVC," \
			"app_info.X_SLP_EXE_PATH," \
			"app_info.X_SLP_APPID," \
			"''," \ 
			"app_info.X_SLP_BASELAYOUTWIDTH," \
			"app_info.X_SLP_INSTALLEDTIME," \
			"app_info.NODISPLAY," \
			"app_info.X_SLP_TASKMANAGE," \
			"app_info.X_SLP_MULTIPLE," \
			"app_info.X_SLP_REMOVABLE," \
			"app_info.X_SLP_ISHORIZONTALSCALE," \
			"app_info.X_SLP_INACTIVATED"


#define SQL_FLD_APP_INFO_WITH_LOCALNAME SQL_FLD_APP_INFO",""localname.name"
#define SQL_LOCALNAME_IDX NUM_OF_PROP + 0

const char *sql_get_filter(int prop);
char *sql_get_locale();
int sql_get_app_info_idx(int prop);

#endif  /* __AIL_SQL_H__ */
