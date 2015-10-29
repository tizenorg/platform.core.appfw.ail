Name:           ail
Version:        0.2.80
Release:        0
License:        Apache-2.0
Summary:        Application Information Library
Group:          Application Framework/Libraries
Source0:        %{name}-%{version}.tar.gz
Source1001:     ail.manifest
BuildRequires:  cmake
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(libsmack)
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(xdgmime)
BuildRequires:  pkgconfig(libtzplatform-config)
Provides:       libail = %{version}-%{release}

%description
Application Information Library package

%package devel
Summary:        Application Information Library Development files
Requires:       libail = %{version}-%{release}
Requires:       pkgconfig(libtzplatform-config)
Requires:       pkgconfig(libsmack)

%description devel
Application Information Library Development files package

%package vconf-devel
Summary:        Application Information Library Development files
Requires:       libail = %{version}-%{release}

%description vconf-devel
Application Information Library Development files package
This developement file purpose concerns the issue TC-2399
https://bugs.tizen.org/jira/browse/TC-2399


%prep
%setup -q
cp %{SOURCE1001} .

%build
CFLAGS="$CFLAGS -fpic"

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS ?DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif

%cmake .  -DTZ_SYS_RW_DESKTOP_APP=%TZ_SYS_RW_DESKTOP_APP \
          -DBUILD_PKGTYPE=rpm

%__make %{?_smp_mflags}

%install
%make_install

%post
ldconfig

ail_createdb 2>/dev/null
ail_syncdb 2>/dev/null
chsmack -a '*' %{TZ_SYS_DB}/.app_info.db*
 
%postun
/sbin/ldconfig
if [ $1 == 0 ]; then
    rm -f %{TZ_SYS_DB}/.app_info.db*
fi

%files
%manifest %{name}.manifest
%license LICENSE
%attr(06775,root,root) %{_bindir}/ail_createdb
%attr(0775,root,root) %{_bindir}/ail_createdb_user
%attr(06775,root,root) %{_bindir}/ail_syncdb
%attr(0775,root,root) %{_bindir}/ail_syncdb_user
#obsolete tools
%attr(06775,root,root) %{_bindir}/ail_initdb
%attr(0775,root,root) %{_bindir}/ail_initdb_user
%{_bindir}/ail_fota
%{_bindir}/ail_desktop
%{_bindir}/ail_filter
%{_bindir}/ail_list
%{_bindir}/ail_package
%{_datadir}/install-info/*
%{_libdir}/libail.so.0
%{_libdir}/libail.so.0.1.0

%files devel
%manifest %{name}.manifest
%{_includedir}/ail.h
%{_libdir}/libail.so
%{_libdir}/pkgconfig/ail.pc

%files vconf-devel
%{_includedir}/ail_vconf.h
