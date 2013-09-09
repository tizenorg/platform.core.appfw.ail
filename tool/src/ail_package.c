/*
Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
PROPRIETARY/CONFIDENTIAL
This software is the confidential and proprietary information of
SAMSUNG ELECTRONICS ("Confidential Information"). You agree and acknowledge that
this software is owned by Samsung and you
shall not disclose such Confidential Information and shall
use it only in accordance with the terms of the license agreement
you entered into with SAMSUNG ELECTRONICS.  SAMSUNG make no
representations or warranties about the suitability
of the software, either express or implied, including but not
limited to the implied warranties of merchantability, fitness for
a particular purpose, or non-infringement.
SAMSUNG shall not be liable for any damages suffered by licensee arising out of or
related to this software.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <ail.h>
#include "ail_private.h"

char *prop_tbl[] = {
	AIL_PROP_PACKAGE_STR,
	AIL_PROP_EXEC_STR,
	AIL_PROP_NAME_STR,
	AIL_PROP_TYPE_STR,
	AIL_PROP_ICON_STR,
	AIL_PROP_CATEGORIES_STR,
	AIL_PROP_VERSION_STR,
	AIL_PROP_MIMETYPE_STR,
	AIL_PROP_X_SLP_SERVICE_STR,
	AIL_PROP_X_SLP_PACKAGETYPE_STR,
	AIL_PROP_X_SLP_PACKAGECATEGORIES_STR,
	AIL_PROP_X_SLP_PACKAGEID_STR,
	AIL_PROP_X_SLP_SVC_STR,
	AIL_PROP_X_SLP_EXE_PATH,
	AIL_PROP_X_SLP_APPID_STR,
	AIL_PROP_X_SLP_PKGID_STR,
	AIL_PROP_X_SLP_DOMAIN_STR,
	AIL_PROP_X_SLP_SUBMODEMAINID_STR,
	AIL_PROP_NODISPLAY_BOOL,
	AIL_PROP_X_SLP_TASKMANAGE_BOOL,
	AIL_PROP_X_SLP_MULTIPLE_BOOL,
	AIL_PROP_X_SLP_REMOVABLE_BOOL,
	AIL_PROP_X_SLP_SUBMODE_BOOL,
	NULL
};


static void _print_help(const char *cmd)
{
	int i;

	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "[Get appinfo value]\n");
	fprintf(stderr, "       %s get <PACKAGE NAME> <COLUMN NAME>\n", cmd);
	fprintf(stderr, "\n");
	fprintf(stderr, "       <COLUMN NAME>\n");

	for (i = 0; prop_tbl[i]; i++) {
		fprintf(stderr, "          %s\n", prop_tbl[i]);
	}

	fprintf(stderr, "\n");
	fprintf(stderr, "       Ex) %s get com.samsung.menu-screen X_SLP_SERVICE\n", cmd);
	fprintf(stderr, "\n");
}



static int _get_property(const char *property)
{
	int i;

	if (!property) {
		return 0;
	}

	for (i = 0; prop_tbl[i]; i++) {
		if (!strcasecmp(prop_tbl[i], property)) {
			return i;
		}
	}

	fprintf(stderr, "%s is not found\n", property);

	return -1;
}



static ail_error_e _get_appinfo(const char *package, const char *property)
{
	ail_appinfo_h handle;
	ail_error_e ret;
	int prop, ival;
	bool bval;
	char *str;
	struct element e;
	struct element *p;
	int t;

	ret = ail_package_get_appinfo(package, &handle);
	if (ret != AIL_ERROR_OK) {
		return AIL_ERROR_FAIL;
	}

	prop = _get_property(property);
	if (prop < 0) {
		goto END;
	}

	e.prop = prop;
	p = &e;
	ELEMENT_TYPE(p,t);

	if (t == VAL_TYPE_STR) {
		ret = ail_appinfo_get_str(handle, property, &str);
		if (ret != AIL_ERROR_OK) {
			goto END;
		}
		fprintf(stderr, "Package[%s], Property[%s] : %s\n", package, property, str);
	} else if (t == VAL_TYPE_INT) {
		ret = ail_appinfo_get_int(handle, property, &ival);
		if (ret != AIL_ERROR_OK) {
			goto END;
		}
		fprintf(stderr, "Package[%s], Property[%s] : %d\n", package, property, ival);
	} else if (t == VAL_TYPE_BOOL) {
		ret = ail_appinfo_get_bool(handle, property, &bval);
		if (ret != AIL_ERROR_OK) {
			goto END;
		}
		fprintf(stderr, "Package[%s], Property[%s] : %d\n", package, property, bval);
	}

END:
	ret = ail_package_destroy_appinfo(handle);
	if (ret != AIL_ERROR_OK) {
		return AIL_ERROR_FAIL;
	}

	return AIL_ERROR_OK;
}

int main(int argc, char** argv)
{
	ail_error_e ret = AIL_ERROR_OK;


	if (4 == argc) {
		if (!strncmp(argv[1], "get", 3)) {
			ret = _get_appinfo(argv[2], argv[3]);
		}
	}
	else {
		_print_help(argv[0]);
		return EXIT_FAILURE;
	}

	if (ret != AIL_ERROR_OK) {
		fprintf(stderr, "There are some problems\n");
	}

	return EXIT_SUCCESS;
}


