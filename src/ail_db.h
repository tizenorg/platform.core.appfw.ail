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




#ifndef __AIL_DB_H__
#define __AIL_DB_H__

#include <sqlite3.h>
#include "ail.h"

#define AIL_SQL_QUERY_MAX_LEN	2048

ail_error_e db_open(void);
ail_error_e db_prepare(const char *query, sqlite3_stmt **stmt);

ail_error_e db_bind_bool(sqlite3_stmt *stmt, int idx, bool value);
ail_error_e db_bind_int(sqlite3_stmt *stmt, int idx, int value);

ail_error_e db_step(sqlite3_stmt *stmt);

ail_error_e db_column_bool(sqlite3_stmt *stmt, int index, bool *value);
ail_error_e db_column_int(sqlite3_stmt *stmt, int index, int *value);
ail_error_e db_column_str(sqlite3_stmt *stmt, int index, char **str);

ail_error_e db_reset(sqlite3_stmt *stmt);
ail_error_e db_finalize(sqlite3_stmt *stmt);


ail_error_e db_exec(const char *query);
ail_error_e db_close(void);

#endif
// End of file
