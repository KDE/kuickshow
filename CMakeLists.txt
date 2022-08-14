cmake_minimum_required(VERSION 3.0)
project(kuickshow)

set(QT_MIN_VERSION "5.4.0")
set(KF5_MIN_VERSION "5.10.0")
set(ECM_MIN_VERSION "5.68.0")

find_package(ECM ${ECM_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

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

find_package(IMLIB REQUIRED)

add_subdirectory( src ) 
add_subdirectory( misc ) 
add_subdirectory( pics ) 
add_subdirectory( icons )
add_subdirectory( doc )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
