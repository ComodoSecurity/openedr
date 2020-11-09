Name: log4cplus
Version: 2.0.3
Release: 1

Summary: log4cplus, C++ logging library
License: Apache
Group: Development/Libraries
Vendor: log4cplus project 
Packager: Yusuke SATO <y-sato@y-sa.to>
Url: http://log4cplus.sourceforge.net/

Source: %name-%version.tar.gz

Prefix: %_prefix
BuildRoot: %_tmppath/%name-%version-root

%description
log4cplus is a simple to use C++ logging API providing thread-safe, 
flexible, and arbitrarily granular control over log management and 
configuration. It is modeled after the Java log4j API.

%package devel
Summary: log4cplus headers, static libraries
Group: Development/Libraries
Requires: %name = %version

%description devel
log4cplus is a simple to use C++ logging API providing thread-safe, 
flexible, and arbitrarily granular control over log management and 
configuration. It is modeled after the Java log4j API.

%prep
rm -rf $RPM_BUILD_ROOT

%setup
./configure --prefix=%{prefix} --libdir=%{_libdir} --with-working-locale CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS"

%build
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT%{prefix}/include/
cp -rp include/log4cplus $RPM_BUILD_ROOT%{prefix}/include/
rm -f $RPM_BUILD_ROOT%{prefix}/include/log4cplus/config/stamp-*
rm -f $RPM_BUILD_ROOT%{prefix}/include/log4cplus/config/*.in
rm -f $RPM_BUILD_ROOT%{prefix}/include/log4cplus/stamp-*
rm -f $RPM_BUILD_ROOT%{prefix}/include/log4cplus/config/macosx.h
rm -f $RPM_BUILD_ROOT%{prefix}/include/log4cplus/config/win32.h
rm -f $RPM_BUILD_ROOT%{prefix}/include/log4cplus/config/windowsh-inc.h
rm -f $RPM_BUILD_ROOT%{prefix}/include/log4cplus/internal/cygwin-win32.h
rm -f $RPM_BUILD_ROOT%{prefix}/include/log4cplus/thread/impl/syncprims-win32.h
find %{buildroot} -name .svn -type d -exec find '{}' -delete \;
find %{buildroot} -name "*.in" -delete


%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,755)
%attr(755,root,root) %_libdir/lib*.so*
%attr(644,root,root) %_libdir/*.la

%files devel
%defattr(-,root,root,755)
%prefix/include/log4cplus/*.h
%prefix/include/log4cplus/helpers/*.h
%prefix/include/log4cplus/spi/*.h
%prefix/include/log4cplus/boost/*.hxx
%prefix/include/log4cplus/config.hxx
%prefix/include/log4cplus/config/defines.hxx
%prefix/include/log4cplus/internal/*.h
%prefix/include/log4cplus/thread/impl/*.h
%prefix/include/log4cplus/thread/*.h
%attr(644,root,root) 
%_libdir/*.a
%_libdir/pkgconfig/log4cplus.pc
