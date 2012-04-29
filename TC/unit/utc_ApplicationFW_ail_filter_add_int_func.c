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

static void utc_ApplicationFW_ail_filter_add_int_func_01(void);
static void utc_ApplicationFW_ail_filter_add_int_func_02(void);
static void utc_ApplicationFW_ail_filter_add_int_func_03(void);
static void utc_ApplicationFW_ail_filter_add_int_func_04(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{utc_ApplicationFW_ail_filter_add_int_func_01, POSITIVE_TC_IDX},
	{utc_ApplicationFW_ail_filter_add_int_func_02, NEGATIVE_TC_IDX},
	{utc_ApplicationFW_ail_filter_add_int_func_03, NEGATIVE_TC_IDX},
	{utc_ApplicationFW_ail_filter_add_int_func_04, NEGATIVE_TC_IDX},
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
 * ail_error_e ail_filter_add_int(ail_filter_h filter,
 *						ail_prop_int_e prop, const int value)
 *
 *@param[in] filter	a filter handle which can be create with ail_filter_new()
 * @param[in] prop		a property type in ail_prop_int_e
 * @param[in] value		the value to filter by
 *
 * @return 0 if success, negative value(<0) if fail\n
 * @retval	AIL_ERROR_OK					success
 * @retval	AIL_ERROR_INVALID_PARAMETER		invalid parameter
 * @retval	AIL_ERROR_OUT_OF_MEMORY			out of memory
 *
 */

/**
 * @brief Positive test case of ail_filter_add_int()
 */
static void utc_ApplicationFW_ail_filter_add_int_func_01(void)
{
	ail_filter_h filter;
	ail_error_e r;
	r = ail_filter_new(&filter);
	if (r != AIL_ERROR_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = ail_filter_add_int(filter, AIL_PROP_X_SLP_BASELAYOUTWIDTH_INT, 480);
	if (r != AIL_ERROR_OK) {
		tet_infoline
		    ("ail_filter_add_int() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	ail_filter_destroy(filter);
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case 01 of ail_filter_add_int()
 */
static void utc_ApplicationFW_ail_filter_add_int_func_02(void)
{
	ail_filter_h filter;
	ail_error_e r;
	ail_prop_int_e prop = AIL_PROP_INT_MIN - 1;
	r = ail_filter_new(&filter);
	if (r != AIL_ERROR_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = ail_filter_add_int(filter, prop, 480);
	if (r != AIL_ERROR_INVALID_PARAMETER) {
		tet_infoline
		    ("ail_filter_add_int() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	ail_filter_destroy(filter);
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case 02 of ail_filter_add_int()
 */
static void utc_ApplicationFW_ail_filter_add_int_func_03(void)
{
	ail_filter_h filter;
	ail_error_e r;
	ail_prop_int_e prop = AIL_PROP_INT_MAX + 1;
	r = ail_filter_new(&filter);
	if (r != AIL_ERROR_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = ail_filter_add_int(filter, prop, 480);
	if (r != AIL_ERROR_INVALID_PARAMETER) {
		tet_infoline
		    ("ail_filter_add_int() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	ail_filter_destroy(filter);
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case 03 of ail_filter_add_int()
 */
static void utc_ApplicationFW_ail_filter_add_int_func_04(void)
{
	ail_filter_h filter;
	ail_error_e r;
	r = ail_filter_new(&filter);
	if (r != AIL_ERROR_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = ail_filter_add_int(NULL, AIL_PROP_X_SLP_BASELAYOUTWIDTH_INT, 480);
	if (r != AIL_ERROR_INVALID_PARAMETER) {
		tet_infoline
		    ("ail_filter_add_int() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	ail_filter_destroy(filter);
	tet_result(TET_PASS);
}
