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


#include <tet_api.h>
#include <ail.h>

static void startup(void);
static void cleanup(void);

void (*tet_startup) (void) = startup;
void (*tet_cleanup) (void) = cleanup;

static void utc_ApplicationFW_ail_filter_list_appinfo_foreach_func_01(void);
static void utc_ApplicationFW_ail_filter_list_appinfo_foreach_func_02(void);
static ail_cb_ret_e appinfo_func(const ail_appinfo_h appinfo, void *user_data);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{utc_ApplicationFW_ail_filter_list_appinfo_foreach_func_01,
	 POSITIVE_TC_IDX},
	{utc_ApplicationFW_ail_filter_list_appinfo_foreach_func_02,
	 NEGATIVE_TC_IDX},
	{NULL, 0},
};

static void startup(void)
{
}

static void cleanup(void)
{
}

/**
 * API Prototype
 * ail_error_e ail_filter_list_appinfo_foreach(ail_filter_h filter,
 *					ail_list_appinfo_cb func, void *user_data)
 *
 * @param[in] filter		a filter handle
 * @param[in] func			the function to call with each package's appinfo
 * @param[in] user_data		user_data to pass to the function
 *
 * @return  0 if success, negative value(<0) if fail\n
 * @retval AIL_ERROR_OK					success
 * @retval AIL_ERROR_DB_FAILED				database error
 * @retval AIL_ERROR_INVALID_PARAMETER		invalid parameter
 *
 */

static ail_cb_ret_e appinfo_func(const ail_appinfo_h appinfo, void *user_data)
{
	return AIL_CB_RET_CONTINUE;
}

/**
 * @brief Positive test case of ail_filter_list_appinfo_foreach()
 */
static void utc_ApplicationFW_ail_filter_list_appinfo_foreach_func_01(void)
{
	ail_filter_h filter;
	ail_error_e r;
	r = ail_filter_new(&filter);
	if (r != AIL_ERROR_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = ail_filter_add_bool(filter, AIL_PROP_NODISPLAY_BOOL, 1);
	if (r != AIL_ERROR_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = ail_filter_list_appinfo_foreach(filter, appinfo_func, NULL);
	if (r != AIL_ERROR_OK) {
		tet_infoline
		    ("ail_filter_list_appinfo_foreach()"
		     " failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	ail_filter_destroy(filter);
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ail_filter_list_appinfo_foreach()
 */
static void utc_ApplicationFW_ail_filter_list_appinfo_foreach_func_02(void)
{
	ail_filter_h filter;
	ail_error_e r;
	r = ail_filter_new(&filter);
	if (r != AIL_ERROR_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = ail_filter_add_bool(filter, AIL_PROP_NODISPLAY_BOOL, 1);
	if (r != AIL_ERROR_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = ail_filter_list_appinfo_foreach(filter, NULL, NULL);
	if (r != AIL_ERROR_INVALID_PARAMETER) {
		tet_infoline
		    ("ail_filter_list_appinfo_foreach()"
		     " failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	ail_filter_destroy(filter);
	tet_result(TET_PASS);
}
