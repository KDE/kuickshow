
# KuickShow main application

set(CMAKE_AUTOMOC ON)
set(kuickshow_KDEINIT_SRCS
   kuickshow.cpp 
   aboutwidget.cpp 
   generalwidget.cpp 
   kuickconfig.cpp
   kuickconfigdlg.cpp 
   kuickfile.cpp
   filecache.cpp
   main.cpp 
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

ki18n_wrap_ui(kuickshow_KDEINIT_SRCS
   aboutwidget.ui
   printing_page.ui
   generalwidget.ui
   defaultswidget.ui
   slideshowwidget.ui
)

ecm_qt_declare_logging_category(kuickshow_KDEINIT_SRCS
   HEADER kuickshow_debug.h
   IDENTIFIER KUICKSHOW_LOG
   CATEGORY_NAME org.kde.kuickshow
   DEFAULT_SEVERITY Warning
   EXPORT kuickshow
   DESCRIPTION "KuickShow image viewer"
)

if (IMLIB_FOUND)
  add_definitions(${IMLIB_DEFINITIONS})
  add_compile_definitions("IMAGE_LIBRARY_VERSION=\"${IMLIB_VERSION}\"")
  list(APPEND kuickshow_KDEINIT_SRCS imlib_imlib1.cpp)
elseif (IMLIB2_FOUND)
  add_definitions(${IMLIB2_DEFINITIONS})
  add_compile_definitions("IMAGE_LIBRARY_VERSION=\"${IMLIB2_VERSION}\"")
  list(APPEND kuickshow_KDEINIT_SRCS imlib_imlib2.cpp)
else ()
  list(APPEND kuickshow_KDEINIT_SRCS imlib_qt.cpp)
endif ()

add_definitions("-DHOMEPAGE_URL=\"https://userbase.kde.org/KuickShow\"")
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

# target kdeinit_kuickshow
kf5_add_kdeinit_executable(kuickshow ${kuickshow_KDEINIT_SRCS})
target_compile_options(kdeinit_kuickshow PRIVATE -std=c++11 -Wall --pedantic)
target_compile_definitions(kdeinit_kuickshow PRIVATE ${IMLIB_DEFINITIONS} -DQT_NO_URL_CAST_FROM_STRING)
target_include_directories(kdeinit_kuickshow PRIVATE ${IMLIB_INCLUDE_DIRS})

target_link_libraries(kdeinit_kuickshow
	Qt5::Core
	Qt5::Gui
	Qt5::PrintSupport
	Qt5::X11Extras
	KF5::I18n
	KF5::IconThemes
	KF5::KIOCore
	KF5::KIOWidgets
	KF5::KIOFileWidgets
	KF5::WindowSystem
	KF5::XmlGui
)

if (IMLIB_FOUND)
target_link_libraries(kdeinit_kuickshow
	${X11_LIBRARIES}
	${IMLIB_LIBRARIES})
endif ()
if (IMLIB2_FOUND)
target_link_libraries(kdeinit_kuickshow
	${IMLIB2_LIBRARIES})
endif ()

install(TARGETS kdeinit_kuickshow  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

# target kuickshow
target_link_libraries( kuickshow kdeinit_kuickshow )
install(TARGETS kuickshow  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( PROGRAMS org.kde.kuickshow.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

ecm_qt_install_logging_categories(
  EXPORT kuickshow
  DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)
