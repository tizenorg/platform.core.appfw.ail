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

static void utc_ApplicationFW_ail_package_destroy_appinfo_func_01(void);
static void utc_ApplicationFW_ail_package_destroy_appinfo_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{utc_ApplicationFW_ail_package_destroy_appinfo_func_01,
	 POSITIVE_TC_IDX},
	{utc_ApplicationFW_ail_package_destroy_appinfo_func_02,
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
 * ail_error_e ail_package_destroy_appinfo(const ail_appinfo_h handle)
 *
 * @param[in] handle destroy all resources related to the handle.
 *
 * @return 0 if success, negative value(<0) if fail\n
 * @retval	AIL_ERROR_OK					success
 * @retval	AIL_ERROR_DB_FAILED				database error
 * @retval	AIL_ERROR_INVALID_PARAMETER		invalid parameter
 *
 */

/**
 * @brief Positive test case of ail_package_destroy_appinfo()
 */
static void utc_ApplicationFW_ail_package_destroy_appinfo_func_01(void)
{
	ail_error_e r;
	ail_appinfo_h handle;
	r = ail_package_get_appinfo("org.tizen.calculator", &handle);
	if (r != AIL_ERROR_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = ail_package_destroy_appinfo(handle);
	if (r != AIL_ERROR_OK) {
		tet_infoline
		    ("ail_package_destroy_appinfo()"
		     " failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ail_package_destroy_appinfo()
 */
static void utc_ApplicationFW_ail_package_destroy_appinfo_func_02(void)
{
	ail_error_e r;
	r = ail_package_destroy_appinfo(NULL);
	if (r != AIL_ERROR_INVALID_PARAMETER) {
		tet_infoline
		    ("ail_package_destroy_appinfo()"
		     " failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
