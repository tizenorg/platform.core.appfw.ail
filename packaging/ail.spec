Name:       ail
Summary:    Application Information Library
Version:    0.2.29
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
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DBUILD_PKGTYPE=rpm

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post

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
mkdir -p /opt/dbspace/
ail_initdb
update_DAC_for_db_file /opt/dbspace/.app_info.db
update_DAC_for_db_file /opt/dbspace/.app_info.db-journal

%postun

%files
/usr/lib/libail.so.0
/usr/lib/libail.so.0.1.0
/usr/bin/ail_initdb
/usr/share/install-info/*

%files devel
/usr/include/ail.h
/usr/lib/libail.so
/usr/lib/pkgconfig/ail.pc
