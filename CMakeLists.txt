cmake_minimum_required(VERSION 3.0)
project(kuickshow)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(FeatureSummary)

find_package(Qt5 REQUIRED
	Core
	Gui
	PrintSupport
	X11Extras
)
find_package(KF5 REQUIRED COMPONENTS
	DocTools
	I18n
	IconThemes
	Init
	KIO
	WindowSystem
	XmlGui
)

find_package(IMLIB REQUIRED)

add_subdirectory( src ) 
add_subdirectory( misc ) 
add_subdirectory( pics ) 
add_subdirectory( icons )
add_subdirectory( doc )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
