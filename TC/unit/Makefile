CC ?= gcc

TARGETS = utc_ApplicationFW_ail_filter_new_func \
	utc_ApplicationFW_ail_filter_destroy_func \
	utc_ApplicationFW_ail_filter_add_str_func \
	utc_ApplicationFW_ail_filter_add_int_func \
	utc_ApplicationFW_ail_filter_add_bool_func \
	utc_ApplicationFW_ail_filter_count_appinfo_func \
	utc_ApplicationFW_ail_filter_list_appinfo_foreach_func \
	utc_ApplicationFW_ail_package_get_appinfo_func \
	utc_ApplicationFW_ail_package_destroy_appinfo_func \
	utc_ApplicationFW_ail_appinfo_get_bool_func \
	utc_ApplicationFW_ail_appinfo_get_str_func \
	utc_ApplicationFW_ail_appinfo_get_int_func \
	utc_ApplicationFW_ail_desktop_add_func \
	utc_ApplicationFW_ail_desktop_remove_func \
	utc_ApplicationFW_ail_desktop_update_func

PKGS = glib-2.0 dlog ail

LDFLAGS = `pkg-config --libs $(PKGS)`
LDFLAGS += $(TET_ROOT)/lib/tet3/tcm_s.o
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -ltcm_s
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -lapi_s
LDFLAGS += -L/usr/lib -lpthread

CFLAGS = -I. `pkg-config --cflags $(PKGS)`
CFLAGS += -I$(TET_ROOT)/inc/tet3
CFLAGS += -Wall

all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGETS)
