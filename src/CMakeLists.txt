
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
   kurlwidget.cpp 
   filewidget.cpp 
   kuick.cpp 
   imlibwidget.cpp 
   slideshowwidget.cpp 
   imagemods.cpp
   printing.cpp )


# target kdeinit_kuickshow
kde4_add_kdeinit_executable( kuickshow ${kuickshow_KDEINIT_SRCS})
target_compile_options(kdeinit_kuickshow PRIVATE -std=c++11 -Wall --pedantic)
target_compile_definitions(kdeinit_kuickshow PRIVATE ${IMLIB_DEFINITIONS})
target_include_directories(kdeinit_kuickshow PRIVATE ${IMLIB_INCLUDE_DIRS} ${KDE4_INCLUDES})
target_link_libraries(kdeinit_kuickshow ${X11_LIBRARIES} ${IMLIB_LIBRARIES} ${KDE4_KIO_LIBS} ${KDE4_KFILE_LIBS})

install(TARGETS kdeinit_kuickshow  ${INSTALL_TARGETS_DEFAULT_ARGS} )


# target kuickshow
target_link_libraries( kuickshow kdeinit_kuickshow )
install(TARGETS kuickshow  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS kuickshow.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

kde4_install_icons(${ICON_INSTALL_DIR})
