Name:           ail
Version:        0.2.80
Release:        0
License:        Apache-2.0
Summary:        Application Information Library
Group:          Application Framework/Libraries
Source0:        %{name}-%{version}.tar.gz
Source1001:     ail.manifest
BuildRequires:  cmake
BuildRequires:  vconf-keys-devel
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(libsmack)
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(xdgmime)
BuildRequires:  pkgconfig(libtzplatform-config)
Provides:       libail = %{version}-%{release}
Requires:  		libcap-tools

%description
Application Information Library package

%package devel
Summary:        Application Information Library Development files
Requires:       libail = %{version}-%{release}
Requires:       pkgconfig(libtzplatform-config)

%description devel
Application Information Library Development files package

%prep
%setup -q
cp %{SOURCE1001} .

%build
CFLAGS+=" -fpic"

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS ?DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif

%cmake .  -DTZ_SYS_RW_DESKTOP_APP=%TZ_SYS_RW_DESKTOP_APP \
          -DBUILD_PKGTYPE=rpm \
		  -DSMACK=Off

%__make %{?_smp_mflags}

%install
%make_install

%post
/sbin/ldconfig
vconftool set -t string db/ail/ail_info "0" -f -s system::vconf_inhouse
vconftool set -t string db/menuscreen/desktop "0" -f -s system::vconf_inhouse
vconftool set -t string db/menu_widget/language "en_US.utf8" -f -s system::vconf_inhouse
chsmack -a User %TZ_SYS_CONFIG/db/ail
chsmack -a User %TZ_SYS_CONFIG/db/ail/ail_info
chsmack -a User %TZ_SYS_CONFIG/db/menuscreen
chsmack -a User %TZ_SYS_CONFIG/db/menuscreen/desktop
chsmack -a User %TZ_SYS_CONFIG/db/menu_widget
chsmack -a User %TZ_SYS_CONFIG/db/menu_widget/language

mkdir -p %{TZ_SYS_RO_DESKTOP_APP}
mkdir -p %{TZ_SYS_RW_DESKTOP_APP}
mkdir -p %{TZ_SYS_RW_APP}
mkdir -p %{TZ_SYS_DB}

chsmack -a '*' %{TZ_SYS_DB}
chsmack -a '*' %{TZ_SYS_RW_APP}
chsmack -a '*' %{TZ_SYS_RW_DESKTOP_APP}
chsmack -a '*' %{TZ_SYS_RO_DESKTOP_APP}

chmod g+w {TZ_SYS_RW_DESKTOP_APP}
chmod g+w {TZ_SYS_RO_DESKTOP_APP}

ail_initdb 2>/dev/null
chsmack -a '*' %{TZ_SYS_DB}/.app_info.db*
 
%postun
/sbin/ldconfig
if [ $1 == 0 ]; then
    rm -f %{TZ_SYS_DB}/.app_info.db*
fi

%files
%manifest %{name}.manifest
%license LICENSE
%attr(06775,root,root) %{_bindir}/ail_initdb
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
