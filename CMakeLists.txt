cmake_minimum_required(VERSION 3.28)
project(kuickshow)

set(QT_MIN_VERSION "6.0.0")
set(KF_MIN_VERSION "6.0.0")
set(ECM_MIN_VERSION ${KF_MIN_VERSION})

find_package(ECM ${ECM_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# If both options are set, then Imlib2 has priority
option(USE_IMLIB1 "Use Imlib1 for image processing" OFF)
option(USE_IMLIB2 "Use Imlib2 for image processing" OFF)

if (USE_IMLIB2)
	find_package(IMLIB2)
endif ()
if (USE_IMLIB1 AND NOT IMLIB2_FOUND)
	find_package(IMLIB)
endif ()
if (IMLIB_FOUND OR IMLIB2_FOUND)
	find_package(X11 REQUIRED)
endif ()

set(REQUIRED_QT_MODULES
	Core
	Gui
	PrintSupport
)
if(IMLIB_FOUND OR IMLIB2_FOUND)
	list(APPEND REQUIRED_QT_MODULES X11Extras)
endif ()
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED ${REQUIRED_QT_MODULES})

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
	DocTools
	I18n
	IconThemes
	KIO
	WindowSystem
	XmlGui
)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(FeatureSummary)

set_package_properties(IMLIB PROPERTIES
	URL "https://download.gnome.org/sources/imlib/"
	DESCRIPTION "Image reading and processing library"
	TYPE OPTIONAL
)
set_package_properties(IMLIB2 PROPERTIES
	URL "https://sourceforge.net/projects/enlightenment/files/imlib2-src/"
	DESCRIPTION "Updated image reading and processing library"
	TYPE OPTIONAL
)

add_subdirectory( src ) 
add_subdirectory( misc ) 
add_subdirectory( pics ) 
add_subdirectory( icons )
add_subdirectory( doc )

ki18n_install(po)
if(KF6DocTools_FOUND)
    kdoctools_install(po)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
