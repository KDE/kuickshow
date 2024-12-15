
# KuickShow application

# the target
set(MAIN_TARGET kuickshow)

# compile flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

# source files
set(SOURCES
   main.cpp
   kuickshow.cpp 
   aboutwidget.cpp 
   generalwidget.cpp 
   kuickconfig.cpp
   kuickconfigdlg.cpp 
   kuickfile.cpp
   filecache.cpp
   defaultswidget.cpp 
   imagewindow.cpp 
   kuickimage.cpp
   filefinder.cpp 
   kuickurlwidget.cpp
   filewidget.cpp 
   kuick.cpp 
   imlibwidget.cpp 
   slideshowwidget.cpp 
   imagemods.cpp
   printing.cpp
   openfilesanddirsdialog.cpp
   imagecache.cpp
   imlib.cpp
)

ki18n_wrap_ui(SOURCES
   aboutwidget.ui
   printing_page.ui
   generalwidget.ui
   defaultswidget.ui
   slideshowwidget.ui
)

# Qt / KF specific settings
set(CMAKE_AUTOMOC ON)

ecm_qt_declare_logging_category(SOURCES
   HEADER kuickshow_debug.h
   IDENTIFIER KUICKSHOW_LOG
   CATEGORY_NAME org.kde.kuickshow
   DEFAULT_SEVERITY Warning
   EXPORT kuickshow
   DESCRIPTION "KuickShow image viewer"
)

if (IMLIB_FOUND)
  list(APPEND SOURCES imlib_imlib1.cpp)
elseif (IMLIB2_FOUND)
  list(APPEND SOURCES imlib_imlib2.cpp)
else ()
  list(APPEND SOURCES imlib_qt.cpp)
endif ()

add_definitions("-DHOMEPAGE_URL=\"https://userbase.kde.org/KuickShow\"")
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

# the target
add_executable(${MAIN_TARGET} ${SOURCES})
target_compile_options(${MAIN_TARGET} PRIVATE -Wall --pedantic)
target_compile_definitions(${MAIN_TARGET} PRIVATE -DQT_NO_URL_CAST_FROM_STRING)

target_link_libraries(${MAIN_TARGET}
	Qt6::Core
	Qt6::Gui
	Qt6::PrintSupport
	KF6::I18n
	KF6::IconThemes
	KF6::KIOCore
	KF6::KIOWidgets
	KF6::KIOFileWidgets
	KF6::WindowSystem
	KF6::XmlGui
)

if (IMLIB_FOUND)
	target_compile_definitions(${MAIN_TARGET} PRIVATE ${IMLIB_DEFINITIONS} "IMAGE_LIBRARY_VERSION=\"${IMLIB_VERSION}\"")
	target_include_directories(${MAIN_TARGET} PRIVATE ${IMLIB_INCLUDE_DIRS})
	target_link_libraries(${MAIN_TARGET}
		Qt6::X11Extras
		${X11_LIBRARIES}
		${IMLIB_LIBRARIES})
elseif (IMLIB2_FOUND)
	target_compile_definitions(${MAIN_TARGET} PRIVATE ${IMLIB2_DEFINITIONS} "IMAGE_LIBRARY_VERSION=\"${IMLIB2_VERSION}\"")
	target_include_directories(${MAIN_TARGET} PRIVATE ${IMLIB2_INCLUDE_DIRS})
	target_link_libraries(${MAIN_TARGET}
		Qt6::X11Extras
		${IMLIB2_LIBRARIES})
endif ()

install(TARGETS ${MAIN_TARGET} ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( PROGRAMS org.kde.kuickshow.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

ecm_qt_install_logging_categories(
  EXPORT kuickshow
  DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)
