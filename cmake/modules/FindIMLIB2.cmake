# Copyright (c) 2006, Dirk Mueller, <mueller@kde.org>
# Copyright (c) 2017, Christian Gerloff <chrgerloff@gmx.net>
# Copyright (c) 2022, Jonathan Marten <jjm@keelhaul.me.uk>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


# FindIMLIB2
# ----------
#
# Find the Imlib 2 graphics library.  This is different enough from the
# Imlib 1 library that different checks and variables are neeeded.  For
# historical reasons the Imlib 1 variables are still named IMLIB_* while
# the new Imlib 2 variables are all named IMLIB2_* as below.
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# IMLIB2_FOUND (boolean)
#     Is true if the library has been found, otherwise false.
# IMLIB2_DEFINITIONS
#     The compiler flags required to use the library.
#     Use them with target_compile_definitions(<target> <scope> ${IMLIB2_DEFINITIONS})
# IMLIB2_INCLUDE_DIRS
#     The directories where the include files are located.
#     Use them with target_include_directories(<target> <scope> ${IMLIB2_INCLUDE_DIRS})
# IMLIB2_LIBRARIES
#     The required libraries to link against.
#     Use them with target_link_libraries(<target> <scope> ${IMLIB2_LIBRARIES})
#
# libImlib2_*
#     These variables are INTERNAL to this cmake module.
#     You should not depend on their values (or even existence) outside this file.


# check if we've already tried to find the library
if (NOT "${IMLIB2_CACHED}" STREQUAL "")
	if ("${IMLIB2_CACHED}" STREQUAL "YES")
		set(IMLIB2_FOUND true)
	endif()
else()
	# first, try to use Imlib 2's pkg-config to get the flags, paths and libs
	include(FindPkgConfig OPTIONAL RESULT_VARIABLE _fpc)
	if(NOT "${_fpc}" STREQUAL "NOTFOUND")
		pkg_check_modules(libImlib2 imlib2>=1.7)
		set(libImlib2_LIBDIR ${libImlib2_LIBRARY_DIRS})
	endif()
	unset(_fpc)

	# if that failed, try a manual approach (last resort)
	if (NOT libImlib2_FOUND)
		find_path(libImlib2_INCLUDEDIR Imlib2.h)
		find_library(libImlib2_LIBRARIES NAMES Imlib2 libImlib2)

		# how do we get this with find_library(...) ? (for now, make sure it's empty)
		set(libImlib2_LIBDIR "")
		set(libImlib2_VERSION "unknown")

		if (libImlib2_INCLUDEDIR AND libImlib2_LIBRARIES)
			set(libImlib2_FOUND true)
		endif()
	endif()

	# next, set the exported variables
	if (libImlib2_FOUND)
		# for some reason, <libImlib2_CFLAGS> now also contains include directives ("-I...");
		# filter these out here, or they'd later create compiler arguments like "-D-I..." which would fail the build
		set(_cflags "${libImlib2_CFLAGS}")
		list(FILTER _cflags EXCLUDE REGEX "^-I")

		# the list of libraries must include the library paths
		set(_libs "")
		foreach(libdir ${libImlib2_LIBDIR})
			list(APPEND _libs "-L${libdir}")
		endforeach()
		list(APPEND _libs ${libImlib2_LIBRARIES})

		# the exported variables need to be cached
		set(_CACHED "YES")
		set(IMLIB2_FOUND true)
		set(IMLIB2_INCLUDE_DIRS "${libImlib2_INCLUDEDIR}" CACHE PATH "Include path for Imlib2.h")
		set(IMLIB2_DEFINITIONS "${_cflags}" CACHE STRING "Compiler definitions for Imlib2")
		set(IMLIB2_LIBRARIES "${_libs}" CACHE STRING "Libraries for Imlib2")
		set(IMLIB2_VERSION "${libImlib2_VERSION}" CACHE STRING "Version of Imlib2")
		unset(_libs)
		unset(_cflags)

		if (NOT IMLIB2_FIND_QUIETLY)
			message(STATUS "IMLIB2 definitions: ${IMLIB2_DEFINITIONS}")
			message(STATUS "IMLIB2 includes:    ${IMLIB2_INCLUDE_DIRS}")
			message(STATUS "IMLIB2 libraries:   ${IMLIB2_LIBRARIES}")
		endif()

		mark_as_advanced(IMLIB2_DEFINITIONS IMLIB2_INCLUDE_DIRS IMLIB2_LIBRARIES)
	else()
		# (optionally) display error and (optionally) fail
		set(_CACHED "NO")
		if (IMLIB2_FIND_REQUIRED)
			message(FATAL_ERROR "Could not find IMLIB2")
		elseif (NOT IMLIB_FIND_QUIETLY)
			message(STATUS "Could not find IMLIB2")
		endif()
	endif()

	set(IMLIB2_CACHED ${_CACHED} CACHE INTERNAL "If Imlib2 has been found")
	unset(_CACHED)
endif()
