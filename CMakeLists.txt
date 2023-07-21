cmake_minimum_required(VERSION 3.16)
project(kuickshow)

set(QT_MIN_VERSION "5.4.0")
set(KF5_MIN_VERSION "5.82.0")
set(ECM_MIN_VERSION ${KF5_MIN_VERSION})

find_package(ECM ${ECM_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# If both options are set, then Imlib2 has priority
option(USE_IMLIB2 "Use Imlib2 for image processing" ON)
option(USE_IMLIB1 "Use Imlib1 for image processing" ON)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED
	Core
	Gui
	PrintSupport
	X11Extras
)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
	DocTools
	I18n
	IconThemes
	Init
	KIO
	WindowSystem
	XmlGui
)

set_package_properties(IMLIB PROPERTIES
                       URL "https://download.gnome.org/sources/imlib/"
                       DESCRIPTION "Image reading and processing library"
                       TYPE OPTIONAL)
set_package_properties(IMLIB2 PROPERTIES
                       URL "https://sourceforge.net/projects/enlightenment/files/imlib2-src/"
                       DESCRIPTION "Updated image reading and processing library"
                       TYPE OPTIONAL)

if (USE_IMLIB2)
  find_package(IMLIB2)
endif ()

if (NOT IMLIB2_FOUND)
  if (USE_IMLIB1)
    find_package(IMLIB)
    find_package(X11 REQUIRED)				# required at build time
  endif ()
endif ()

if ((NOT IMLIB_FOUND) AND (NOT IMLIB2_FOUND))
  message(STATUS "")
  message(STATUS "Neither the IMLIB nor IMLIB2 image library was found or was")
  message(STATUS "  configured to be used.")
  message(STATUS "KuickShow will use Qt only, image modifications (brightness,")
  message(STATUS "  contrast, gamma) will not be available.")
  message(STATUS "")
endif ()

add_subdirectory( src ) 
add_subdirectory( misc ) 
add_subdirectory( pics ) 
add_subdirectory( icons )
add_subdirectory( doc )

ki18n_install(po)
if(KF5DocTools_FOUND)
    kdoctools_install(po)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
