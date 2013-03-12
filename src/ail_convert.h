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


#ifndef __AIL_CONVERT_H__
#define __AIL_CONVERT_H__

#include "ail_private.h"

ail_prop_str_e _ail_convert_to_prop_str(const char *property);
ail_prop_int_e _ail_convert_to_prop_int(const char *property);
ail_prop_bool_e _ail_convert_to_prop_bool(const char *property);

#endif  /* __AIL_CONVERT_H__ */
