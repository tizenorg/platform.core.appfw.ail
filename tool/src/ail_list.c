/*
 * ail_list.c is based on ail_filter.c
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "ail.h"
#include "ail_private.h"


static void usage(const char *name)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "  Usage: %s\n", name);
	fprintf(stderr, "\n");
}


ail_cb_ret_e appinfo_list_appid_namefunc(const ail_appinfo_h appinfo,  void *user_data)
{
	char* package_str_name = NULL;
    char* package_str_appid = NULL;
    char* package_str_x_slp_exe = NULL;
	ail_appinfo_get_str(appinfo, AIL_PROP_X_SLP_APPID_STR, &package_str_appid);
	ail_appinfo_get_str(appinfo, AIL_PROP_NAME_STR, &package_str_name);
	ail_appinfo_get_str(appinfo, AIL_PROP_X_SLP_EXE_PATH, &package_str_x_slp_exe);
		
	printf("'%s' '%s' '%s'\n",package_str_appid, package_str_name, package_str_x_slp_exe);
	return AIL_CB_RET_CONTINUE;
}



int main(int argc, char *argv[])
{
	int o;
	bool err;
	if(getuid() == 0) {
		printf("Please use it as non root user\n");
		return;
	} 
	
	printf("Application List for user %lu\n", (long)getuid());
	printf("User's Application \n");
	printf("APPID    NAME   EXEPATH \n");
	ail_filter_list_usr_appinfo_foreach(NULL, appinfo_list_appid_namefunc, NULL, getuid());
	printf("Global's / Common Applications \n");
	printf("APPID    NAME   EXEPATH \n");
	ail_filter_list_appinfo_foreach(NULL, appinfo_list_appid_namefunc, NULL);
	printf("=================================================\n");
	return 0;
}
