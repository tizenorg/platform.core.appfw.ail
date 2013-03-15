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



/**
 @ingroup SLP_PG
 @defgroup SLP_PG_APPLICATION INFORMATION LIBRARY AIL
 @{

<h1 class="pg">Introduction</h1>

<h2 class="pg">Purpose of this document</h2>
 The purpose of this document is to describe how applications can use Application information library APIs to store/retrive the application's informations. This document gives programming guidelines to the application engineers.

<h2 class="pg">Scope</h2>
The scope of this document is limited to AIL API usage.


<h1 class="pg">Architecture</h1>

<h2 class="pg">Architecture overview</h2>
Application information library manages the application information such as application name, type, icon path, exe path etc.

@image html SLP_ail_logical_view.png

<h2 class="pg">SLP Features</h2>
- The Application information library provides installed applications informations.
- The Application information library exposes the APIs to filter the applcation based on certain fields.
- The Application information library uses DB to store the informations of the application.
- The Application information library can process the desktop file to store/retrive the informations.


<h1 class="pg">Application information library properties</h1>

<h2 class="pg">AIL Functionality</h2>
There are three types of API's provided by AIL
- ail_appinfo_xxx
	- Get the informations of the given package.
- ail_filter_xxx
	- Get the Application informations matched to filter condition.
- ail_desktop_xxx
	- add, update or remove package informations in DB.

<h2 class="pg">Software Components</h2>
- Query Requester
	- It provides the interface to the DB. It is responsible for forming the query and excute the query in db.
- Desktop file Reader/parser
	- It can read the application's desktop file from the file system (/opt/share/application/).
- Filter
	- It can filter the query based on the given condition and it returns the filtered output to the application.
- Get App info
	- It can get the information of the application using appid.

<h1 class="pg"> Software module Details </h2>

<h2 class="pg"> Query Requester </h2>
	- Query Requester module is provides the interface to sql DB in AIL.
	- It is responsible for creating connection to the DB, form the sql query and execute the query in DB.
	- It uses the libsql API to access the DB. (Eg) db_xx() API.

@code

@brief string type properties

#define AIL_PROP_PACKAGE_STR                    "AIL_PROP_PACKAGE_STR"
#define AIL_PROP_EXEC_STR                       "AIL_PROP_EXEC_STR"
#define AIL_PROP_NAME_STR                       "AIL_PROP_NAME_STR"
#define AIL_PROP_TYPE_STR                       "AIL_PROP_TYPE_STR"
#define AIL_PROP_ICON_STR                       "AIL_PROP_ICON_STR"
#define AIL_PROP_CATEGORIES_STR                 "AIL_PROP_CATEGORIES_STR"
#define AIL_PROP_VERSION_STR                    "AIL_PROP_VERSION_STR"
#define AIL_PROP_MIMETYPE_STR                   "AIL_PROP_MIMETYPE_STR"
#define AIL_PROP_X_SLP_SERVICE_STR              "AIL_PROP_X_SLP_SERVICE_STR"
#define AIL_PROP_X_SLP_PACKAGETYPE_STR          "AIL_PROP_X_SLP_PACKAGETYPE_STR"
#define AIL_PROP_X_SLP_PACKAGECATEGORIES_STR    "AIL_PROP_X_SLP_PACKAGECATEGORIES_STR"
#define AIL_PROP_X_SLP_PACKAGEID_STR            "AIL_PROP_X_SLP_PACKAGEID_STR"
#define AIL_PROP_X_SLP_SVC_STR                  "AIL_PROP_X_SLP_SVC_STR"
#define AIL_PROP_X_SLP_EXE_PATH                 "AIL_PROP_X_SLP_EXE_PATH"
#define AIL_PROP_X_SLP_APPID_STR                "AIL_PROP_X_SLP_APPID_STR"


@brief integer type properties

#define AIL_PROP_X_SLP_TEMP_INT                 "AIL_PROP_X_SLP_TEMP_INT"
#define AIL_PROP_X_SLP_INSTALLEDTIME_INT        "AIL_PROP_X_SLP_INSTALLEDTIME_INT"

@brief boolean type properties

#define AIL_PROP_NODISPLAY_BOOL                 "AIL_PROP_NODISPLAY_BOOL"
#define AIL_PROP_X_SLP_TASKMANAGE_BOOL          "AIL_PROP_X_SLP_TASKMANAGE_BOOL"
#define AIL_PROP_X_SLP_MULTIPLE_BOOL            "AIL_PROP_X_SLP_MULTIPLE_BOOL"
#define AIL_PROP_X_SLP_REMOVABLE_BOOL           "AIL_PROP_X_SLP_REMOVABLE_BOOL"
#define AIL_PROP_X_SLP_ENABLED_BOOL         "AIL_PROP_X_SLP_ENABLED_BOOL"


@brief A handle for filters

typedef struct ail_filter *ail_filter_h;

@brief A handle for appinfos

typedef struct ail_appinfo *ail_appinfo_h;

@endcode


<h2 class="pg">Desktop file Reader/parser</h2>
	- This module provides set of APIs to add update delete application's desktop files contents in DB.
	- It is responsible for validate the desktop file contents are in standard format.
	- The major functionality of this module is add, update and delete desktop file contents in DB Eg.(ail_desktop_xxx())
	<h3 class="pg"> Desktop file specification </h3>
		- http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#introduction
		- Desktop entry files should have the .desktop extension
		- Entries in the file are {key,value} pairs in the format: Key=Value
		- Standard Keys
		- Type, Version, Name, NoDisplay, Icon, Hidden, Exec, Path, MimeType, Categories, URL, etc
		- The Exec key must contain a command line. A command line consists of an executable program optionally followed by one or more arguments.
		- The executable program can either be specified with its full path or with the name of the executable only.
		- If no full path is provided the executable is looked up in the $PATH environment variable used by the desktop environment.
		- The name or path of the executable program may not contain the equal sign ("="). Arguments are separated by a space.

@image html SLP_AIL_desktop.png

<h2 class="pg"> Adding desktop file to DB </h2>
@image html SLP_AIL_add.png
<h2 class="pg" > Sample code </h2>
@code
static ail_error_e _add_desktop(const char *package)
{
        ail_error_e ret;

        if (!package) {
                return AIL_ERROR_FAIL;
        }

        ret = ail_desktop_add(package);
        if (ret != AIL_ERROR_OK) {
                return AIL_ERROR_FAIL;
        }

        return AIL_ERROR_OK;
}

static ail_error_e _remove_desktop(const char *package)
{
        ail_error_e ret;

        if (!package) {
                return AIL_ERROR_FAIL;
        }

        ret = ail_desktop_remove(package);
        if (ret != AIL_ERROR_OK) {
                return AIL_ERROR_FAIL;
        }

        return AIL_ERROR_OK;
}

@endcode

<h2 class="pg">Filter</h2>
This module provides set of APIs to get the information from the DB based on the given matched condtion.
It is reponsbile for giving the filtered output to the applications. Eg.(ail_filter_xxx())  These following are some of the filtering condition.
	- Filter integer type only
	- Filter string type only
	- Filter bool type only

<h3 class="pg" > Filter sample code </h3>
@code
ail_cb_ret_e appinfo_func(const ail_appinfo_h appinfo, void *user_data)
{
        int *i = (int *)user_data;
        char *package;

        ail_appinfo_get_str(appinfo, AIL_PROP_PACKAGE_STR, &package);
        printf("i=%d %s\n", (*i)++, package);

        if (*i > 30)
                return AIL_CB_RET_CANCEL;

        return AIL_CB_RET_CONTINUE;
}

int list_packages()
{
        ail_filter_h filter;
        ail_error_e ret;
        int i=0;

        ret = ail_filter_new(&filter);
        if (ret != AIL_ERROR_OK) {
                return -1;
        }

        ret = ail_filter_add_bool(filter, AIL_PROP_X_SLP_REMOVABLE_BOOL, true);
        if (ret != AIL_ERROR_OK) {
                return -1;
        }

        ret = ail_filter_add_str(filter, AIL_PROP_TYPE_STR, "Application");
        if (ret != AIL_ERROR_OK) {
                return -1;
        }

        printf("List packages which are removable and 'Application' typed\n");
        ail_filter_list_appinfo_foreach(filter, appinfo_func, (void *)&i);

        ail_filter_destroy(filter);

        return 0;
}

@endcode

<h2 class="pg">Get Application info</h2>
	- This module provides set of APIs to get informations of the installed packages.
	- This module internally uses the query Requester module to get the information of the requested packages in DB. Eg.(ail_appinfo_xxx())
@image html SLP_AIL_get.png

@code
static ail_error_e _get_name(const char *package)
{
        ail_appinfo_h handle;
        ail_error_e ret;
        char *str;

        ret = ail_package_get_appinfo(package, &handle);
        if (ret != AIL_ERROR_OK) {
                return AIL_ERROR_FAIL;
        }

        ret = ail_appinfo_get_str(handle, AIL_PROP_NAME_STR, &str);
        if (ret != AIL_ERROR_OK) {
                return AIL_ERROR_FAIL;
        }
        fprintf(stderr, "Package[%s], Property[%s] : %s\n", package, property, str);

        ret = ail_package_destroy_appinfo(handle);
        if (ret != AIL_ERROR_OK) {
                return AIL_ERROR_FAIL;
        }

        return AIL_ERROR_OK;
}

@endcode


 @}
**/
