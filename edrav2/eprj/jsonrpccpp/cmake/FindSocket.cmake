# - This module determines the socket library of the system.
# The following variables are set
#  Socket_LIBRARIES     - the socket library
#  Socket_FOUND

if (${CMAKE_SYSTEM} MATCHES "Windows")
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		set(Socket_LIBRARIES ws2_32)
	else()
		set(Socket_LIBRARIES wsock32 ws2_32)
	endif()
	set(Socket_FOUND 1)
elseif(${CMAKE_SYSTEM} MATCHES "INtime")
	set(Socket_LIBRARIES netlib)
	set(Socket_FOUND 1)
else()
	set(Socket_LIBRARIES)
	set(Socket_FOUND 1)
endif()

