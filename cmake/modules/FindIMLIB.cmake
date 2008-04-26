# - Try to find the Imlib graphics library
# Once done this will define
#
#  IMLIB_FOUND - system has IMLIB
#  IMLIB_INCLUDE_DIR - the IMLIB include directory
#  IMLIB_LIBRARIES - Link these to use IMLIB
#  IMLIB_DEFINITIONS - Compiler switches required for using IMLIB

# Copyright (c) 2006, Dirk Mueller, <mueller@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


IF (DEFINED CACHED_IMLIB)

  # in cache already
  IF ("${CACHED_IMLIB}" STREQUAL "YES")
    SET(IMLIB_FOUND TRUE)
  ENDIF ("${CACHED_IMLIB}" STREQUAL "YES")

ELSE (DEFINED CACHED_IMLIB)

IF (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  INCLUDE(UsePkgConfig)

  PKGCONFIG(imlib _IMLIBIncDir _IMLIBLinkDir _IMLIBLinkFlags _IMLIBCflags)
  set(IMLIB_DEFINITIONS ${_IMLIBCflags})

ELSE (NOT WIN32)

  FIND_PATH(IMLIB_INCLUDE_DIR Imlib.h
    ${_IMLIBIncDir}
  )
  set(IMLIB_DEFINITIONS "-I${IMLIB_INCLUDE_DIR}")
ENDIF (NOT WIN32)

  FIND_LIBRARY(IMLIB_LIBRARIES NAMES Imlib libImlib
    PATHS
    ${_IMLIBLinkDir}
  )

  if (IMLIB_DEFINITIONS AND IMLIB_LIBRARIES)
     SET(IMLIB_FOUND true)
  endif (IMLIB_DEFINITIONS AND IMLIB_LIBRARIES)
 
  if (IMLIB_FOUND)
    set(CACHED_IMLIB "YES")
    if (NOT IMLIB_FIND_QUIETLY)
      message(STATUS "Found IMLIB Libraries: ${IMLIB_LIBRARIES}")
      message(STATUS "Found IMLIB Defs     : ${IMLIB_DEFINITIONS}")
    endif (NOT IMLIB_FIND_QUIETLY)
  else (IMLIB_FOUND)
    set(CACHED_IMLIB "NO")
    if (NOT IMLIB_FIND_QUIETLY)
      message(STATUS "didn't find IMLIB")
    endif (NOT IMLIB_FIND_QUIETLY)
    if (IMLIB_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find IMLIB")
    endif (IMLIB_FIND_REQUIRED)
  endif (IMLIB_FOUND)
  
  MARK_AS_ADVANCED(IMLIB_DEFINITIONS IMLIB_LIBRARIES)
  
  set(CACHED_IMLIB ${CACHED_IMLIB} CACHE INTERNAL "If liImlib was checked")

ENDIF (DEFINED CACHED_IMLIB)
