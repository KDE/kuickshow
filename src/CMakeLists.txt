
# KuickShow main application

set(CMAKE_AUTOMOC ON)
set(kuickshow_KDEINIT_SRCS
   kuickshow.cpp 
   aboutwidget.cpp 
   generalwidget.cpp 
   kuickconfigdlg.cpp 
   kuickfile.cpp
   filecache.cpp
   main.cpp 
   defaultswidget.cpp 
   imagewindow.cpp 
   kuickdata.cpp 
   kuickimage.cpp
   imdata.cpp 
   filefinder.cpp 
   kuickurlwidget.cpp
   filewidget.cpp 
   kuick.cpp 
   imlibwidget.cpp 
   slideshowwidget.cpp 
   imagemods.cpp
   printing.cpp
   openfilesanddirsdialog.cpp
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

add_definitions("-DHOMEPAGE_URL=\"https://userbase.kde.org/KuickShow\"")

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
	${X11_LIBRARIES}
	${IMLIB_LIBRARIES}
)

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
