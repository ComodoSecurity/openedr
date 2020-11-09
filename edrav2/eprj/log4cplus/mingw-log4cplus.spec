Name:           log4cplus
Version:        2.0.3
Release:        1%{?dist}
Summary:        log4cplus, C++ logging library

License:        Apache
Group:          Development/Libraries
URL:            http://log4cplus.sourceforge.net/
Source0:        http://downloads.sourceforge.net/project/log4cplus/log4cplus-stable/2.0.3/log4cplus-2.0.3.tar.gz

BuildArch:      noarch

BuildRequires:  mingw32-filesystem >= 95
BuildRequires:  mingw32-gcc
BuildRequires:  mingw32-binutils
BuildRequires:  mingw32-gettext
BuildRequires:  mingw32-win-iconv
BuildRequires:  mingw32-zlib

BuildRequires:  mingw64-filesystem >= 95
BuildRequires:  mingw64-gcc
BuildRequires:  mingw64-binutils
BuildRequires:  mingw64-gettext
BuildRequires:  mingw64-win-iconv
BuildRequires:  mingw64-zlib

%description
log4cplus is a simple to use C++ logging API providing thread-safe, 
flexible, and arbitrarily granular control over log management and 
configuration. It is modeled after the Java log4j API.

# Strip removes essential information from the .dll.a file, so disable it
%define __strip /bin/true

# Win32
%package -n mingw32-log4cplus
Summary:    MinGW compiled log4cplus library for the Win32 target

%description -n mingw32-log4cplus
MinGW compiled log4cplus library for the Win32 target

%package -n mingw32-log4cplus-devel
Summary:    Headers for the MinGW compiled log4cplus library for the Win32 target

%description -n mingw32-log4cplus-devel
Headers for the MinGW compiled log4cplus library for the Win32 target

# Win64
%package -n mingw64-log4cplus
Summary:    MinGW compiled log4cplus library for the Win64 target

%description -n mingw64-log4cplus
MinGW compiled log4cplus library for the Win64 target

%package -n mingw64-log4cplus-devel
Summary:    Headers for the MinGW compiled log4cplus library for the Win64 target

%description -n mingw64-log4cplus-devel
Headers for the MinGW compiled log4cplus library for the Win64 target

%prep
%setup -q -n log4cplus-%{version}

%build
%mingw_configure
%mingw_make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
%mingw_make_install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name "*.la" -delete
find $RPM_BUILD_ROOT -name "stamp-*" -delete
find $RPM_BUILD_ROOT -name "*.in" -delete
find $RPM_BUILD_ROOT -name macosx.h -delete
find $RPM_BUILD_ROOT -name windowsh-inc.h -delete
find $RPM_BUILD_ROOT -name cygwin-win32.h -delete
find $RPM_BUILD_ROOT -name syncprims-win32.h -delete
# find $RPM_BUILD_ROOT -name liblog4cplus.a -delete
find $RPM_BUILD_ROOT -name .svn -type d -exec find '{}' -delete \;

# Win32
%files -n mingw32-log4cplus
%{mingw32_bindir}/lib*.dll

%files -n mingw32-log4cplus-devel
%defattr(-,root,root,755)
%{mingw32_includedir}/log4cplus/*.h
%{mingw32_includedir}/log4cplus/helpers/*.h
%{mingw32_includedir}/log4cplus/spi/*.h
%{mingw32_includedir}/log4cplus/boost/*.hxx
%{mingw32_includedir}/log4cplus/config.hxx
%{mingw32_includedir}/log4cplus/config/defines.hxx
%{mingw32_includedir}/log4cplus/config/win32.h
%{mingw32_includedir}/log4cplus/internal/*.h
%{mingw32_includedir}/log4cplus/thread/impl/*.h
%{mingw32_includedir}/log4cplus/thread/*.h
%attr(644,root,root) 
%{mingw32_libdir}/*.dll.a
%{mingw32_libdir}/liblog4cplus.a
%{mingw32_libdir}/pkgconfig/log4cplus.pc

# Win64
%files -n mingw64-log4cplus
%{mingw64_bindir}/lib*.dll

%files -n mingw64-log4cplus-devel
%defattr(-,root,root,755)
%{mingw64_includedir}/log4cplus/*.h
%{mingw64_includedir}/log4cplus/helpers/*.h
%{mingw64_includedir}/log4cplus/spi/*.h
%{mingw64_includedir}/log4cplus/boost/*.hxx
%{mingw64_includedir}/log4cplus/config.hxx
%{mingw64_includedir}/log4cplus/config/defines.hxx
%{mingw64_includedir}/log4cplus/config/win32.h
%{mingw64_includedir}/log4cplus/internal/*.h
%{mingw64_includedir}/log4cplus/thread/impl/*.h
%{mingw64_includedir}/log4cplus/thread/*.h
%attr(644,root,root) 
%{mingw64_libdir}/*.dll.a
%{mingw64_libdir}/liblog4cplus.a
%{mingw64_libdir}/pkgconfig/log4cplus.pc

%changelog
* Wed Aug 14 2013 John Smits <japsmits@gmail.com> - 1.1.1-1
- Initial release
