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

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "ail.h"
#include "ail_private.h"

enum {
	_CMD_UNKNOWN,
	_CMD_COUNT,
	_CMD_FILTER,
};

static void usage(const char *name)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "  Usage: %s -c command [options]\n", name);
	fprintf(stderr, "    command:\n");
	fprintf(stderr, "      c : count appinfos by option\n");
	fprintf(stderr, "      f : filter appinfos by option\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    option:\n");
	fprintf(stderr, "      --package=<pacakge name>\n");
	fprintf(stderr, "      --exec=<exec file>\n");
	fprintf(stderr, "      --name=<title name>\n");
	fprintf(stderr, "      --type=<type>\n");
	fprintf(stderr, "      --icon=<icon file>\n");
	fprintf(stderr, "      --category=<category>\n");
	fprintf(stderr, "      --version=<version>\n");
	fprintf(stderr, "      --mimetype=<mimetype>\n");
	fprintf(stderr, "      --nodisplay=<{0|1}>\n");
	fprintf(stderr, "      --service=<service>\n");
	fprintf(stderr, "      --packagetype=<package type>\n");
	fprintf(stderr, "      --packagecategories=<package category>\n");
	fprintf(stderr, "      --packageid=<package id>\n");
	fprintf(stderr, "      --svc=<action:scheme:mime>\n");
	fprintf(stderr, "      --taskmanage=<{true|false}>\n");
	fprintf(stderr, "      --multiple=<{true|false}>\n");
	fprintf(stderr, "      --removable=<{true|false}>\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    Example:\n");
	fprintf(stderr, "       %s -n menu-screen -r 1\n", name);
	fprintf(stderr, "\n");
}

struct _ail_map_t {
	int prop;
	const char *property;
};

static struct _ail_map_t prop_map[] = {
	{E_AIL_PROP_PACKAGE_STR, 		AIL_PROP_PACKAGE_STR},
	{E_AIL_PROP_EXEC_STR, 			AIL_PROP_EXEC_STR},
	{E_AIL_PROP_NAME_STR, 			AIL_PROP_NAME_STR},
	{E_AIL_PROP_TYPE_STR, 			AIL_PROP_TYPE_STR},
	{E_AIL_PROP_ICON_STR, 			AIL_PROP_ICON_STR},
	{E_AIL_PROP_CATEGORIES_STR, 		AIL_PROP_CATEGORIES_STR},
	{E_AIL_PROP_VERSION_STR, 		AIL_PROP_VERSION_STR},
	{E_AIL_PROP_MIMETYPE_STR, 		AIL_PROP_MIMETYPE_STR},
	{E_AIL_PROP_X_SLP_SERVICE_STR,		AIL_PROP_X_SLP_SERVICE_STR},
	{E_AIL_PROP_X_SLP_PACKAGETYPE_STR, 	AIL_PROP_X_SLP_PACKAGETYPE_STR},
	{E_AIL_PROP_X_SLP_PACKAGECATEGORIES_STR, AIL_PROP_X_SLP_PACKAGECATEGORIES_STR},
	{E_AIL_PROP_X_SLP_PACKAGEID_STR, 	AIL_PROP_X_SLP_PACKAGEID_STR},
	{E_AIL_PROP_X_SLP_SVC_STR, 		AIL_PROP_X_SLP_SVC_STR},
	{E_AIL_PROP_X_SLP_EXE_PATH, 		AIL_PROP_X_SLP_EXE_PATH},
	{E_AIL_PROP_NODISPLAY_BOOL, AIL_PROP_NODISPLAY_BOOL},
	{E_AIL_PROP_X_SLP_TASKMANAGE_BOOL, AIL_PROP_NODISPLAY_BOOL},
	{E_AIL_PROP_X_SLP_MULTIPLE_BOOL, AIL_PROP_X_SLP_MULTIPLE_BOOL},
	{E_AIL_PROP_X_SLP_REMOVABLE_BOOL, AIL_PROP_X_SLP_REMOVABLE_BOOL},
	{E_AIL_PROP_X_SLP_APPID_STR, AIL_PROP_X_SLP_APPID_STR},
	{E_AIL_PROP_X_SLP_PKGID_STR, AIL_PROP_X_SLP_PKGID_STR},
	{E_AIL_PROP_X_SLP_DOMAIN_STR, AIL_PROP_X_SLP_DOMAIN_STR},
	{E_AIL_PROP_X_SLP_SUBMODEMAINID_STR, AIL_PROP_X_SLP_SUBMODEMAINID_STR},
	{E_AIL_PROP_X_SLP_INSTALLEDSTORAGE_STR, AIL_PROP_X_SLP_INSTALLEDSTORAGE_STR},
	{E_AIL_PROP_X_SLP_TEMP_INT, AIL_PROP_X_SLP_TEMP_INT},
	{E_AIL_PROP_X_SLP_INSTALLEDTIME_INT, AIL_PROP_X_SLP_INSTALLEDTIME_INT},
	{E_AIL_PROP_NODISPLAY_BOOL, AIL_PROP_NODISPLAY_BOOL},
	{E_AIL_PROP_X_SLP_TASKMANAGE_BOOL, AIL_PROP_X_SLP_TASKMANAGE_BOOL},
	{E_AIL_PROP_X_SLP_MULTIPLE_BOOL, AIL_PROP_X_SLP_MULTIPLE_BOOL},
	{E_AIL_PROP_X_SLP_REMOVABLE_BOOL, AIL_PROP_X_SLP_REMOVABLE_BOOL},
	{E_AIL_PROP_X_SLP_ISHORIZONTALSCALE_BOOL, AIL_PROP_X_SLP_ISHORIZONTALSCALE_BOOL},
	{E_AIL_PROP_X_SLP_ENABLED_BOOL, AIL_PROP_X_SLP_ENABLED_BOOL},
	{E_AIL_PROP_X_SLP_SUBMODE_BOOL, AIL_PROP_X_SLP_SUBMODE_BOOL}
};

static const char *_ail_convert_to_property(int prop)
{
	int i = 0;

	if (prop < E_AIL_PROP_STR_MIN || prop > E_AIL_PROP_BOOL_MAX)
		return NULL;

	for (i=0 ; i < (E_AIL_PROP_BOOL_MAX + 1) ; i++) {
		if (prop == prop_map[i].prop) {
			return prop_map[i].property;
		}
	}

	return NULL;
}



static int _get_cmd(const char *arg)
{
	int r;
	int a;

	if(!arg)
		a = 0;
	else
		a = (int)*arg;

	switch (a) {
	case 'c':
	case 'C':
		r = _CMD_COUNT;
		break;
	case 'f':
	case 'F':
		r = _CMD_FILTER;
		break;
	default:
		r = _CMD_UNKNOWN;
		break;
	}

	return r;
}

ail_cb_ret_e appinfo_list_func(const ail_appinfo_h appinfo,  void *user_data)
{
	char *rs = NULL;
	int t=-1;
	bool b = 0;
	int n = 0;
	struct element e;
	int i=0;
	struct element *p;
	ail_error_e error = AIL_ERROR_OK;
	ail_cb_ret_e ret = AIL_CB_RET_CONTINUE;
	p = &e;

	bool err = false;
	ret = AIL_CB_RET_CONTINUE;
	for(i = 0; i< E_AIL_PROP_BOOL_MAX+1 && err==false; i ++) {
		e.prop = i;
		ELEMENT_TYPE(p, t);
		switch(t) {
			case VAL_TYPE_BOOL:
				error = ail_appinfo_get_bool(appinfo, _ail_convert_to_property(i), &b);
				if (error) ret = AIL_CB_RET_CANCEL;
				printf("%s|",b?"true":"false");
				break;
			case VAL_TYPE_INT:
				ail_appinfo_get_int(appinfo, _ail_convert_to_property(i), &n);
				if (error) ret = AIL_CB_RET_CANCEL;
				printf("%d|", n);
				break;
			case VAL_TYPE_STR:
				ail_appinfo_get_str(appinfo, _ail_convert_to_property(i), &rs);
				if (error) ret = AIL_CB_RET_CANCEL;
				printf("%s|", rs);
				break;
			default:
				fprintf(stderr, "$$$\n");
				err = true;
				break;
		}
	}
	printf("\n");
	return ret;
}



int main(int argc, char *argv[])
{
	int o;
	bool err;
	ail_filter_h f;
	int oi;
	int c;
	static struct element e;
	int t;

	f = NULL;
	oi = -1;
	c = _CMD_UNKNOWN;

	static const struct option longopts[] = {
		{ "help", 0, NULL, 'h' },
		{ "command", 1, NULL, 'c' },
		{ "package", 1, &(e.prop), E_AIL_PROP_PACKAGE_STR},
		{ "name", 1, &(e.prop), E_AIL_PROP_NAME_STR },
		{ "mimetype", 1, &(e.prop), E_AIL_PROP_MIMETYPE_STR},
		{ "removable", 1, &(e.prop), E_AIL_PROP_X_SLP_REMOVABLE_BOOL },
		{ "exec", 1, &(e.prop), E_AIL_PROP_EXEC_STR},
		{ "type", 1, &(e.prop), E_AIL_PROP_TYPE_STR},
		{ "icon", 1, &(e.prop), E_AIL_PROP_ICON_STR},
		{ "categories", 1, &(e.prop), E_AIL_PROP_CATEGORIES_STR},
		{ "version", 1, &(e.prop), E_AIL_PROP_VERSION_STR},
		{ "mimetype", 1, &(e.prop), E_AIL_PROP_MIMETYPE_STR},
		{ "nodisplay", 1, &(e.prop), E_AIL_PROP_NODISPLAY_BOOL},
		{ "service", 1, &(e.prop), E_AIL_PROP_X_SLP_SERVICE_STR},
		{ "packagetype", 1, &(e.prop), E_AIL_PROP_X_SLP_PACKAGETYPE_STR},
		{ "packagecategories", 1, &(e.prop), E_AIL_PROP_X_SLP_PACKAGECATEGORIES_STR},
		{ "packageid", 1, &(e.prop), E_AIL_PROP_X_SLP_PACKAGEID_STR},
		{ "svc", 1, &(e.prop), E_AIL_PROP_X_SLP_SVC_STR},
		{ "taskmanage", 1, &(e.prop), E_AIL_PROP_X_SLP_TASKMANAGE_BOOL},
		{ "multiple", 1, &(e.prop), E_AIL_PROP_X_SLP_MULTIPLE_BOOL},
		{ "removable", 1, &(e.prop), E_AIL_PROP_X_SLP_REMOVABLE_BOOL},
		{ "appid", 1, &(e.prop), E_AIL_PROP_X_SLP_APPID_STR},
		{ "pkgid", 1, &(e.prop), E_AIL_PROP_X_SLP_PKGID_STR},
		{ "submode", 1, &(e.prop), E_AIL_PROP_X_SLP_SUBMODE_BOOL},
		{ "submodemainid", 1, &(e.prop), E_AIL_PROP_X_SLP_SUBMODEMAINID_STR},
		{ "installedstorage", 1, &(e.prop), E_AIL_PROP_X_SLP_INSTALLEDSTORAGE_STR},
		{ "domain", 1, &(e.prop), E_AIL_PROP_X_SLP_DOMAIN_STR},
		{ 0, 0, 0, 0 },
	};

	if (ail_filter_new(&f) != AIL_ERROR_OK)
		return -1;

	err = false;

	while (!err && (o = getopt_long(argc, argv, "c:", longopts, &oi)) >= 0) {
		bool b;
		struct element *p = &e;

		ELEMENT_TYPE(p, t);

		switch (o) {
			case 'c':
				c = _get_cmd(optarg);
				break;
			case 0:
				switch (t) {
					case VAL_TYPE_BOOL:
						if(!strcasecmp(optarg, "true") || !strcasecmp(optarg, "1"))
							b = true;
						else if (!strcasecmp(optarg, "false") || !strcasecmp(optarg, "0"))
							b = false;
						else {
							err = true;
							break;
						}
						if (ail_filter_add_bool(f, _ail_convert_to_property(e.prop), b) != AIL_ERROR_OK)
							err = true;
						break;

					case VAL_TYPE_INT:
						if (ail_filter_add_int(f, _ail_convert_to_property(e.prop), atoi(optarg)) != AIL_ERROR_OK)
							err = true;
						break;

					case VAL_TYPE_STR:
						if (ail_filter_add_str(f, _ail_convert_to_property(e.prop), optarg) != AIL_ERROR_OK)
							err = true;
						break;

					default:
						err = true;
						break;
				}
				break;
			default:
				err = true;
				break;
		}
	}

	if (err) {
		usage(argv[0]);
	}
	else {
		int n = -1;
		switch (c) {
			case _CMD_COUNT:
				if (ail_filter_count_appinfo(f, &n) != AIL_ERROR_OK){
					fprintf(stderr, "Error: failed to count appinfo\n");
				}
				else
					fprintf(stderr, "count=%d\n", n);
				break;
			case _CMD_FILTER:
				ail_filter_list_appinfo_foreach(f, appinfo_list_func, NULL);
				break;
			default:
				break;
		}
	}

	if (f)
		ail_filter_destroy(f);

	return 0;
}
