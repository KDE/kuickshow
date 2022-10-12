# Copyright (c) 2006, Dirk Mueller, <mueller@kde.org>
# Copyright (c) 2017, Christian Gerloff <chrgerloff@gmx.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


# FindIMLIB
# ---------
#
# Find the Imlib graphics library.
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# IMLIB_FOUND (boolean)
#     Is true if the library has been found, otherwise false.
# IMLIB_DEFINITIONS
#     The compiler flags required to use the library.
#     Use them with target_compile_definitions(<target> <scope> ${IMLIB_DEFINITIONS})
# IMLIB_INCLUDE_DIRS
#     The directories where the include files are located.
#     Use them with target_include_directories(<target> <scope> ${IMLIB_INCLUDE_DIRS})
# IMLIB_LIBRARIES
#     The required libraries to link against.
#     Use them with target_link_libraries(<target> <scope> ${IMLIB_LIBRARIES})
#
# libImlib_*
#     These variables are INTERNAL to this cmake module.
#     You should not depend on their values (or even existence) outside this file.


# check if we've already tried to find the library
if (NOT "${IMLIB_CACHED}" STREQUAL "")
	if ("${IMLIB_CACHED}" STREQUAL "YES")
		set(IMLIB_FOUND true)
	endif()
else()
	# first, try to use imlib's pkg-config to get the flags, paths and libs
	include(FindPkgConfig OPTIONAL RESULT_VARIABLE _fpc)
	if(NOT "${_fpc}" STREQUAL "NOTFOUND")
		pkg_check_modules(libImlib imlib>=1.9)
		set(libImlib_LIBDIR ${libImlib_LIBRARY_DIRS})
	endif()
	unset(_fpc)

	# if that failed, try a manual approach (last resort)
	if (NOT libImlib_FOUND)
		find_path(libImlib_INCLUDEDIR Imlib.h)
		find_library(libImlib_LIBRARIES NAMES Imlib libImlib)

		# how do we get this with find_library(...) ? (for now, make sure it's empty)
		set(libImlib_LIBDIR "")
		set(libImlib_VERSION "unknown")

		if (libImlib_INCLUDEDIR AND libImlib_LIBRARIES)
			set(libImlib_FOUND true)
		endif()
	endif()


	# next, set the exported variables
	if (libImlib_FOUND)
		# the list of libraries must include the library paths
		set(_libs "")
		foreach(libdir ${libImlib_LIBDIR})
			list(APPEND _libs "-L${libdir}")
		endforeach()
		list(APPEND _libs ${libImlib_LIBRARIES})

		# the exported variables need to be cached
		set(_CACHED "YES")
		set(IMLIB_FOUND true)
		set(IMLIB_INCLUDE_DIRS "${libImlib_INCLUDEDIR}" CACHE PATH "Include path for Imlib.h")
		set(IMLIB_DEFINITIONS "${libImlib_CFLAGS}" CACHE STRING "Compiler definitions for Imlib")
		set(IMLIB_LIBRARIES "${_libs}" CACHE STRING "Libraries for Imlib")
		set(IMLIB_VERSION "${libImlib_VERSION}" CACHE STRING "Version of Imlib")
		unset(_libs)

		if (NOT IMLIB_FIND_QUIETLY)
			message(STATUS "IMLIB definitions: ${IMLIB_DEFINITIONS}")
			message(STATUS "IMLIB includes:    ${IMLIB_INCLUDE_DIRS}")
			message(STATUS "IMLIB libraries:   ${IMLIB_LIBRARIES}")
		endif()

		mark_as_advanced(IMLIB_DEFINITIONS IMLIB_INCLUDE_DIRS IMLIB_LIBRARIES)
	else()
		# (optionally) display error and (optionally) fail
		set(_CACHED "NO")
		if (IMLIB_FIND_REQUIRED)
			message(FATAL_ERROR "Could not find IMLIB")
		elseif (NOT IMLIB_FIND_QUIETLY)
			message(STATUS "Could not find IMLIB")
		endif()
	endif()

	set(IMLIB_CACHED ${_CACHED} CACHE INTERNAL "If imlib has been found")
	unset(_CACHED)
endif()
