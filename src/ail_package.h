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




#ifndef __AIL_PACKAGE_H__
#define __AIL_PACKAGE_H__

#include <sqlite3.h>
#include "ail.h"

ail_appinfo_h appinfo_create(void);
void appinfo_destroy(ail_appinfo_h ai);
void appinfo_set_stmt(ail_appinfo_h ai, sqlite3_stmt *stmt);
int _appinfo_check_installed_storage(ail_appinfo_h ai);

#endif  /* __AIL_PACKAGE_H__ */
