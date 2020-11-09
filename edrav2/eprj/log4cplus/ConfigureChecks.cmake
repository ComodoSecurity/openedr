if (NOT CMAKE_VERSION VERSION_LESS 3.1.0)
  cmake_policy(SET CMP0054 NEW)
endif()

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

check_include_files(dlfcn.h       HAVE_DLFCN_H)
check_include_files(errno.h       LOG4CPLUS_HAVE_ERRNO_H )
check_include_files(iconv.h       LOG4CPLUS_HAVE_ICONV_H )
check_include_files(limits.h      LOG4CPLUS_HAVE_LIMITS_H )
check_include_files(sys/types.h   LOG4CPLUS_HAVE_SYS_TYPES_H )
check_include_files("sys/types.h;sys/socket.h"  LOG4CPLUS_HAVE_SYS_SOCKET_H )
check_include_files(sys/syscall.h LOG4CPLUS_HAVE_SYS_SYSCALL_H )
check_include_files("sys/types.h;sys/time.h"    LOG4CPLUS_HAVE_SYS_TIME_H )
check_include_files("sys/types.h;sys/timeb.h"   LOG4CPLUS_HAVE_SYS_TIMEB_H )
check_include_files("sys/types.h;sys/stat.h"    LOG4CPLUS_HAVE_SYS_STAT_H )
check_include_files(sys/file.h    LOG4CPLUS_HAVE_SYS_FILE_H )
check_include_files(syslog.h      LOG4CPLUS_HAVE_SYSLOG_H )
check_include_files(arpa/inet.h   LOG4CPLUS_HAVE_ARPA_INET_H )
check_include_files(netinet/in.h  LOG4CPLUS_HAVE_NETINET_IN_H )
check_include_files("sys/types.h;netinet/tcp.h" LOG4CPLUS_HAVE_NETINET_TCP_H )
check_include_files(netdb.h       LOG4CPLUS_HAVE_NETDB_H )
check_include_files(unistd.h      LOG4CPLUS_HAVE_UNISTD_H )
check_include_files(fcntl.h       LOG4CPLUS_HAVE_FCNTL_H )
check_include_files(stdio.h       LOG4CPLUS_HAVE_STDIO_H )
check_include_files(stdarg.h      LOG4CPLUS_HAVE_STDARG_H )
check_include_files(stdlib.h      LOG4CPLUS_HAVE_STDLIB_H )
check_include_files(time.h        LOG4CPLUS_HAVE_TIME_H )
check_include_files(wchar.h       LOG4CPLUS_HAVE_WCHAR_H )
check_include_files(poll.h        LOG4CPLUS_HAVE_POLL_H )


check_include_files(inttypes.h    HAVE_INTTYPES_H )
check_include_files(memory.h      HAVE_MEMORY_H )
check_include_files(stdint.h      HAVE_STDINT_H )
check_include_files(strings.h     HAVE_STRINGS_H )
check_include_files(string.h      HAVE_STRING_H )


check_include_files("stdlib.h;stdio.h;stdarg.h;string.h;float.h" STDC_HEADERS )

find_library(LIBADVAPI32 advapi32)
find_library(LIBKERNEL32 kernel32)
find_library(LIBNSL nsl)
find_library(LIBRT rt)
find_library(LIBICONV iconv)
find_library(LIBPOSIX4 posix4)
find_library(LIBCPOSIX cposix)
find_library(LIBSOCKET socket)
find_library(LIBWS2_32 ws2_32)

check_function_exists(gmtime_r      LOG4CPLUS_HAVE_GMTIME_R )
check_function_exists(localtime_r   LOG4CPLUS_HAVE_LOCALTIME_R )
check_function_exists(gettimeofday  LOG4CPLUS_HAVE_GETTIMEOFDAY )
check_function_exists(getpid        LOG4CPLUS_HAVE_GETPID )
check_function_exists(poll          LOG4CPLUS_HAVE_POLL )
check_function_exists(pipe          LOG4CPLUS_HAVE_PIPE )
check_function_exists(pipe2         LOG4CPLUS_HAVE_PIPE2 )
check_function_exists(ftime         LOG4CPLUS_HAVE_FTIME )
check_function_exists(stat          LOG4CPLUS_HAVE_STAT )
check_function_exists(lstat         LOG4CPLUS_HAVE_LSTAT )
check_function_exists(fcntl         LOG4CPLUS_HAVE_FCNTL )
check_function_exists(lockf         LOG4CPLUS_HAVE_FLOCK )
check_function_exists(flock         LOG4CPLUS_HAVE_LOCKF )
check_function_exists(htons         LOG4CPLUS_HAVE_HTONS )
check_function_exists(ntohs         LOG4CPLUS_HAVE_NTOHS )
check_function_exists(htonl         LOG4CPLUS_HAVE_HTONL )
check_function_exists(ntohl         LOG4CPLUS_HAVE_NTOHL )
check_function_exists(shutdown      LOG4CPLUS_HAVE_SHUTDOWN )
check_function_exists(vsnprintf     LOG4CPLUS_HAVE_VSNPRINTF )
check_function_exists(_vsnprintf    LOG4CPLUS_HAVE__VSNPRINTF )
check_function_exists(vsprintf_s    LOG4CPLUS_HAVE_VSPRINTF_S )
check_function_exists(vswprintf_s   LOG4CPLUS_HAVE_VSWPRINTF_S )
check_function_exists(vfprintf_s    LOG4CPLUS_HAVE_VFPRINTF_S )
check_function_exists(vfwprintf_s   LOG4CPLUS_HAVE_VFWPRINTF_S )
check_function_exists(_vsnprintf_s  LOG4CPLUS_HAVE__VSNPRINTF_S )
check_function_exists(_vsnwprintf_s LOG4CPLUS_HAVE__VSNWPRINTF_S )
check_function_exists(mbstowcs      LOG4CPLUS_HAVE_MBSTOWCS )
check_function_exists(wcstombs      LOG4CPLUS_HAVE_WCSTOMBS )


check_symbol_exists(ENAMETOOLONG          errno.h       LOG4CPLUS_HAVE_ENAMETOOLONG )
check_symbol_exists(SYS_gettid            sys/syscall.h LOG4CPLUS_HAVE_GETTID )
check_symbol_exists(__FUNCTION__          ""            LOG4CPLUS_HAVE_FUNCTION_MACRO )
check_symbol_exists(__PRETTY_FUNCTION__   ""            LOG4CPLUS_HAVE_PRETTY_FUNCTION_MACRO )
check_symbol_exists(__func__              ""            LOG4CPLUS_HAVE_FUNC_SYMBOL )

# clock_gettime() needs -lrt here
# TODO AC says this exists
if (LIBRT)
  check_library_exists("${LIBRT}" clock_gettime ""
    LOG4CPLUS_HAVE_CLOCK_GETTIME )
  check_library_exists("${LIBRT}" clock_nanosleep ""
    LOG4CPLUS_HAVE_CLOCK_NANOSLEEP )
  check_library_exists("${LIBRT}" nanosleep ""
    LOG4CPLUS_HAVE_NANOSLEEP )
else ()
  check_function_exists(clock_gettime LOG4CPLUS_HAVE_CLOCK_GETTIME )
  check_function_exists(clock_nanosleep LOG4CPLUS_HAVE_CLOCK_NANOSLEEP )
  check_function_exists(nanosleep LOG4CPLUS_HAVE_NANOSLEEP )
endif ()

# iconv functions may require iconv library (on OS X for example)
if(LOG4CPLUS_WITH_ICONV)
  if(LIBICONV)
    check_library_exists("${LIBICONV}" iconv_open  "" LOG4CPLUS_HAVE_ICONV_OPEN )
    check_library_exists("${LIBICONV}" iconv_close "" LOG4CPLUS_HAVE_ICONV_CLOSE )
    check_library_exists("${LIBICONV}" iconv       "" LOG4CPLUS_HAVE_ICONV )
  else()
    check_function_exists(iconv_open  LOG4CPLUS_HAVE_ICONV_OPEN )
    check_function_exists(iconv_close LOG4CPLUS_HAVE_ICONV_CLOSE )
    check_function_exists(iconv       LOG4CPLUS_HAVE_ICONV )
  endif()
endif()

check_function_exists(gethostbyname_r LOG4CPLUS_HAVE_GETHOSTBYNAME_R) # TODO more complicated test in AC
check_function_exists(getaddrinfo     LOG4CPLUS_HAVE_GETADDRINFO ) # TODO more complicated test in AC


# check for declspec stuff
if(NOT DEFINED LOG4CPLUS_DECLSPEC_EXPORT)
  check_c_source_compiles(
    "#if defined (__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 1))
     # error Please fail.
     #endif

     __attribute__((visibility(\"default\"))) int x = 0;
     __attribute__((visibility(\"default\"))) int foo();
     int foo() { return 0; }
     __attribute__((visibility(\"default\"))) int bar() { return x; }
     __attribute__((visibility(\"hidden\"))) int baz() { return 1; }

     int main(void) { return 0; }"
   HAVE_ATTRIBUTE_VISIBILITY
  )
  if(HAVE_ATTRIBUTE_VISIBILITY)
    set(LOG4CPLUS_DECLSPEC_EXPORT "__attribute__ ((visibility(\"default\")))" )
    set(LOG4CPLUS_DECLSPEC_IMPORT "__attribute__ ((visibility(\"default\")))" )
    set(LOG4CPLUS_DECLSPEC_PRIVATE "__attribute__ ((visibility(\"hidden\")))" )
  endif()
endif()

if(NOT DEFINED LOG4CPLUS_DECLSPEC_EXPORT)
  check_c_source_compiles(
    "#if defined (__clang__)
     // Here the problem is that Clang only warns that it does not support
     // __declspec(dllexport) but still compiles the executable.
     # error Please fail.
     #endif

     __declspec(dllexport) int x = 0;
     __declspec(dllexport) int foo ();
     int foo () { return 0; }
     __declspec(dllexport) int bar () { return x; }

     int main(void) { return 0; }"
    HAVE_DECLSPEC_DLLEXPORT
  )
  if(HAVE_DECLSPEC_DLLEXPORT)
    set(LOG4CPLUS_DECLSPEC_EXPORT "__declspec(dllexport)" )
    set(LOG4CPLUS_DECLSPEC_IMPORT "__declspec(dllimport)" )
    set(LOG4CPLUS_DECLSPEC_PRIVATE "" )
  endif()
endif()

if(NOT DEFINED LOG4CPLUS_DECLSPEC_EXPORT)
  check_c_source_compiles(
    "__global int x = 0;
     __global int foo();
     int foo() { return 0; }
     __global int bar() { return x; }
     __hidden int baz() { return 1; }

     int main(void) { return 0; }"
    HAVE_GLOBAL_AND_HIDDEN
  )
  if(HAVE_GLOBAL_AND_HIDDEN)
    set(LOG4CPLUS_DECLSPEC_EXPORT "__global" )
    set(LOG4CPLUS_DECLSPEC_IMPORT "__global" )
    set(LOG4CPLUS_DECLSPEC_PRIVATE "__hidden" )
  endif()
endif()

if(NOT DEFINED LOG4CPLUS_DECLSPEC_EXPORT OR NOT ENABLE_SYMBOLS_VISIBILITY)
  set(LOG4CPLUS_DECLSPEC_EXPORT "")
  set(LOG4CPLUS_DECLSPEC_IMPORT "")
  set(LOG4CPLUS_DECLSPEC_PRIVATE "")
endif()

# check for thread-local stuff
if(NOT DEFINED LOG4CPLUS_HAVE_TLS_SUPPORT)
  # TODO: requires special compiler switch on GCC and Clang
  # Currently it is assumed that they are provided in
  # CMAKE_CXX_FLAGS
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")
  check_cxx_source_compiles(
    "extern thread_local int x;
     thread_local int * ptr = 0;
     int foo() { ptr = &x; return x; }
     thread_local int x = 1;

     int main()
     {
         x = 2;
         foo();
         return 0;
     }"
    HAVE_CXX11_THREAD_LOCAL
  )
  set(CMAKE_REQUIRED_FLAGS "")
  if(HAVE_CXX11_THREAD_LOCAL)
    set(LOG4CPLUS_HAVE_TLS_SUPPORT 1)
    set(LOG4CPLUS_THREAD_LOCAL_VAR "thread_local")
  endif()
endif()

if(NOT DEFINED LOG4CPLUS_HAVE_TLS_SUPPORT)
  check_cxx_source_compiles(
    "#if defined (__NetBSD__)
     #include <sys/param.h>
     #if ! __NetBSD_Prereq__(5,1,0)
     #error NetBSD __thread support does not work before 5.1.0. It is missing __tls_get_addr.
     #endif
     #endif

     extern __thread int x;
     __thread int * ptr = 0;
     int foo() { ptr = &x; return x; }
     __thread int x = 1;

     int main()
     {
         x = 2;
         foo();
         return 0;
     }"
    HAVE_GCC_THREAD_EXTENSION
  )
  if(HAVE_GCC_THREAD_EXTENSION)
    set(LOG4CPLUS_HAVE_TLS_SUPPORT 1)
    set(LOG4CPLUS_THREAD_LOCAL_VAR "__thread")
  endif()
endif()

if(NOT DEFINED LOG4CPLUS_HAVE_TLS_SUPPORT)
  check_cxx_source_compiles(
    "#if defined (__GNUC__)
     #error Please fail.
     #endif

     extern __declspec(thread) int x;
     __declspec(thread) int * ptr = 0;
     int foo() { ptr = &x; return x; }
     __declspec(thread) int x = 1;

     int main()
     {
         x = 2;
         foo();
         return 0;
     }"
    HAVE_DECLSPEC_THREAD
  )
  if(HAVE_DECLSPEC_THREAD)
    set(LOG4CPLUS_HAVE_TLS_SUPPORT 1)
    set(LOG4CPLUS_THREAD_LOCAL_VAR "__declspec(thread)")
  endif()
endif()

if(CYGWIN)
  # Cygwin passes the compilation and link test here but fails during linking
  # of the real code, so we want to avoid it too. See GCC PR64697.
  unset(LOG4CPLUS_HAVE_TLS_SUPPORT)
  unset(LOG4CPLUS_THREAD_LOCAL_VAR)
endif()

set(CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
check_type_size(socklen_t _SOCKLEN_SIZE)
if (_SOCKLEN_SIZE)
  set(socklen_t)
else()
  set(socklen_t TRUE)
endif()

macro(PATH_TO_HAVE _pathVar )
  if (${_pathVar})
    set(HAVE_${_pathVar} TRUE)
  else ()
    set(HAVE_${_pathVar} FALSE)
  endif ()
endmacro()


path_to_have(LIBADVAPI32)
path_to_have(LIBKERNEL32)
path_to_have(LIBNSL)
path_to_have(LIBRT)
path_to_have(LIBPOSIX4)
path_to_have(LIBCPOSIX)
path_to_have(LIBSOCKET)
path_to_have(LIBWS2_32)


set(HAVE_STDLIB_H              ${LOG4CPLUS_HAVE_STDLIB_H} )
set(HAVE_SYS_STAT_H            ${LOG4CPLUS_HAVE_SYS_STAT_H} )
set(HAVE_SYS_TYPES_H           ${LOG4CPLUS_HAVE_SYS_TYPES_H} )
set(HAVE_SYS_FILE_H            ${LOG4CPLUS_HAVE_SYS_FILE_H} )
set(HAVE_UNISTD_H              ${LOG4CPLUS_HAVE_UNISTD_H} )


set(HAVE_FTIME                 ${LOG4CPLUS_HAVE_FTIME} )
set(HAVE_GETPID                ${LOG4CPLUS_HAVE_GETPID} )
set(HAVE_GETTIMEOFDAY          ${LOG4CPLUS_HAVE_GETTIMEOFDAY} )
set(HAVE_GETADDRINFO           ${LOG4CPLUS_HAVE_GETADDRINFO} )
set(HAVE_GETHOSTBYNAME_R       ${LOG4CPLUS_HAVE_GETHOSTBYNAME_R} )
set(HAVE_GMTIME_R              ${LOG4CPLUS_HAVE_GMTIME_R} )
set(HAVE_HTONL                 ${LOG4CPLUS_HAVE_HTONL} )
set(HAVE_HTONS                 ${LOG4CPLUS_HAVE_HTONS} )
set(HAVE_ICONV_OPEN            ${LOG4CPLUS_HAVE_ICONV_OPEN} )
set(HAVE_ICONV_CLOSE           ${LOG4CPLUS_HAVE_ICONV_CLOSE} )
set(HAVE_ICONV                 ${LOG4CPLUS_HAVE_ICONV} )
set(HAVE_LSTAT                 ${LOG4CPLUS_HAVE_LSTAT} )
set(HAVE_FCNTL                 ${LOG4CPLUS_HAVE_FCNTL} )
set(HAVE_LOCKF                 ${LOG4CPLUS_HAVE_LOCKF} )
set(HAVE_FLOCK                 ${LOG4CPLUS_HAVE_FLOCK} )
set(HAVE_LOCALTIME_R           ${LOG4CPLUS_HAVE_LOCALTIME_R} )
set(HAVE_NTOHL                 ${LOG4CPLUS_HAVE_NTOHL} )
set(HAVE_NTOHS                 ${LOG4CPLUS_HAVE_NTOHS} )
set(HAVE_STAT                  ${LOG4CPLUS_HAVE_STAT} )

set(HAVE_VFPRINTF_S            ${LOG4CPLUS_HAVE_VFPRINTF_S} )
set(HAVE_VFWPRINTF_S           ${LOG4CPLUS_HAVE_VFWPRINTF_S} )
set(HAVE_VSNPRINTF             ${LOG4CPLUS_HAVE_VSNPRINTF} )
set(HAVE_VSPRINTF_S            ${LOG4CPLUS_HAVE_VSPRINTF_S} )
set(HAVE_VSWPRINTF_S           ${LOG4CPLUS_HAVE_VSWPRINTF_S} )
set(HAVE__VSNPRINTF            ${LOG4CPLUS_HAVE__VSNPRINTF} )
set(HAVE__VSNPRINTF_S          ${LOG4CPLUS_HAVE__VSNPRINTF_S} )
set(HAVE__VSNWPRINTF_S         ${LOG4CPLUS_HAVE__VSNWPRINTF_S} )

set(HAVE_FUNCTION_MACRO        ${LOG4CPLUS_HAVE_FUNCTION_MACRO} )
set(HAVE_PRETTY_FUNCTION_MACRO ${LOG4CPLUS_HAVE_PRETTY_FUNCTION_MACRO} )

set(HAVE___SYNC_ADD_AND_FETCH  ${LOG4CPLUS_HAVE___SYNC_ADD_AND_FETCH} )
set(HAVE___SYNC_SUB_AND_FETCH  ${LOG4CPLUS_HAVE___SYNC_SUB_AND_FETCH} )
