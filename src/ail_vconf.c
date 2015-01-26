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

#if !defined(NO_VCONF_BUXTON_FALLBACK)

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <vconf.h>
#include "ail_vconf.h"
#include "ail_private.h"

#define VCONFTOOL		"/usr/bin/vconftool"
#define CMD_VCONF_GET_STR	VCONFTOOL " -q get '%s'"
#define CMD_VCONF_SET_STR	VCONFTOOL " -q set -t string '%s' '%s' -f"

/*

 Reads the content of the input 'stream' (it should be a text without nul
characters) in a feshly allocated buffer, using allocations of 'block_size'
increment.

 Returns the read string or NULL in case of error.
*/
static char *_pread_(FILE *stream, size_t block_size)
{
	char *result = NULL;
	size_t alloc = 0;
	size_t length = 0;
	char *string;

	for(;;) {
		/* is on error ? */
		if (ferror(stream) != 0) {
			free(result);
			return NULL;
		}
		/* allocate enough memory */
		/* assert(length <= alloc) */
		if (length >= alloc) {
			alloc += block_size;
			string = realloc(result, alloc + 1); /* one more for ending null */
			if (string == NULL) {
				free(result);
				return NULL;
			}
			result = string;
		}
		/* assert(length < alloc) */
		/* assert(result != NULL); */
		/* is at end ? */
		if (feof(stream) != 0) {
			result[length] = 0;
			return result;
		}
		length += fread(result + length, 1, alloc - length, stream);
	}
}

/*
 Runs the command given by 'cmddef' and its arguments formated as printf.

 The resulting output stream of the command is return as a freshly allocated
string in '*readen'.

 Retruns 0 in case of success or -1 in case of error.
*/
static int _ail_vconf_exec_(char **readen, const char *cmddef, ...)
{
	int result;
	FILE *stream;
	char *command;
	va_list ap;

	*readen = NULL;
	va_start(ap, cmddef);
	result = vasprintf(&command, cmddef, ap);
	va_end(ap);
	if (result >= 0) {
		result = -1;
		stream = popen(command, "r");
		if (stream != NULL) {
			*readen = _pread_(stream, 1024);
			if (pclose(stream) != -1 && *readen != NULL) {
				result = 0;
			}
		}
		free(command);
	}
	return result;
}

/*
 vconf_get_str with fallback to the command vconftool.
*/
EXPORT_API char *ail_vconf_get_str(const char *keyname)
{
	char *result;
	char *data;
	int status;
	size_t length;

	result = vconf_get_str(keyname);
	if (result == NULL) {
		status = _ail_vconf_exec_(&data, CMD_VCONF_GET_STR, keyname);
		if (status == 0) {
			/* the string data is of the form 'key, value = ...\n' */
			result = strstr(data, " = ");
			if (result != NULL) {
				/* skips the prefix " = " */
				result = result + 3;
				/* remove trailing '\n' */
				length = strlen(result);
				if (length > 0 && result[length-1] == '\n') {
					result[length-1] = 0;
				}
				/* get the final result */
				result = strdup(result);
			}
		}
		free(data);
	}
	return result;
}

/*
 vconf_set_str with fallback to the command vconftool.
*/
EXPORT_API int ail_vconf_set_str(const char *keyname, const char *strval)
{
	int result;
	char *data;

	result = vconf_set_str(keyname, strval);
	if (result != 0) {
		result = _ail_vconf_exec_(&data, CMD_VCONF_SET_STR, keyname, strval);
		free(data);
	}
	return result;
}

#endif



