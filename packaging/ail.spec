#sbs-git:slp/pkgs/a/ail ail 0.2.22 29ac1f2c98453cad647cca6a92abc7da3dbb047b
Name:       ail
Summary:    Application Information Library
Version:    0.2.79
Release:    1
Group:      System/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  cmake
BuildRequires:  vconf-keys-devel
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(xdgmime)

%description
Application Information Library

%package devel
Summary:    Application Information Library Development files
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Application Information Library (devel)

%prep
%setup -q

%build
CFLAGS+=" -fpic"

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS ?DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif

%cmake .  -DBUILD_PKGTYPE=rpm

make %{?jobs:-j%jobs}

%install
%make_install

%post
vconftool set -t string db/ail/ail_info "0" -f -s system::vconf_inhouse
vconftool set -t string db/menuscreen/desktop "0" -f -s system::vconf_inhouse

CHDBGID="6010"

update_DAC_for_db_file()
{
        if [ ! -f $@ ]; then
                touch $@
        fi

        chown :$CHDBGID $@ 2>/dev/null
        if [ $? -ne 0 ]; then
                echo "Failed to change the owner of $@"
        fi
        chmod 664 $@ 2>/dev/null
        if [ $? -ne 0 ]; then
                echo "Failed to change the perms of $@"
        fi
}
mkdir -p /usr/share/applications
mkdir -p /opt/share/applications
mkdir -p /opt/dbspace/

update_DAC_for_db_file /opt/dbspace/.app_info.db
update_DAC_for_db_file /opt/dbspace/.app_info.db-journal

%postun

%files
%manifest ail.manifest
%{_libdir}/libail.so.0
%{_libdir}/libail.so.0.1.0
/usr/bin/ail_initdb
/usr/bin/ail_fota
/usr/bin/ail_desktop
/usr/bin/ail_filter
/usr/bin/ail_package
/usr/share/install-info/*

%files devel
/usr/include/ail.h
%{_libdir}/libail.so
%{_libdir}/pkgconfig/ail.pc
