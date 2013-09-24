Name:       ail
Summary:    Application Information Library
Version:    0.2.73
Release:    1
Group:      System/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	ail.manifest
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
cp %{SOURCE1001} .

%build
CFLAGS+=" -fpic"
%cmake .  -DBUILD_PKGTYPE=rpm

make %{?jobs:-j%jobs}

%install
%make_install

mkdir -p %{buildroot}/opt/dbspace/
mkdir -p %{buildroot}/opt/share/applications/

%post
vconftool set -t string db/ail/ail_info "0" -f -s system::vconf_inhouse
vconftool set -t string db/menuscreen/desktop "0" -f -s system::vconf_inhouse
vconftool set -t string db/menu_widget/language "en_US.utf8" -f -s system::vconf_inhouse

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
ail_initdb
update_DAC_for_db_file /opt/dbspace/.app_info.db
update_DAC_for_db_file /opt/dbspace/.app_info.db-journal

%postun
if [ $1 == 0 ]; then
rm -f /opt/dbspace/.app_info.db*
fi

%files
%manifest %{name}.manifest
%{_libdir}/libail.so.0
%{_libdir}/libail.so.0.1.0
/opt/share/applications
/usr/bin/ail_initdb
/usr/share/install-info/*

%files devel
%manifest %{name}.manifest
/usr/include/ail.h
%{_libdir}/libail.so
%{_libdir}/pkgconfig/ail.pc
