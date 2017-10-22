
include_directories(${IMLIB_INCLUDE_DIR} ${KDE4_INCLUDES})

########### next target ###############

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

set(CMAKE_AUTOMOC ON)
kde4_add_kdeinit_executable( kuickshow ${kuickshow_KDEINIT_SRCS})

target_link_libraries(kdeinit_kuickshow ${X11_LIBRARIES} ${IMLIB_LIBRARIES} ${KDE4_KIO_LIBS} ${KDE4_KFILE_LIBS})

install(TARGETS kdeinit_kuickshow  ${INSTALL_TARGETS_DEFAULT_ARGS} )

target_link_libraries( kuickshow kdeinit_kuickshow )
install(TARGETS kuickshow  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( PROGRAMS kuickshow.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

kde4_install_icons(${ICON_INSTALL_DIR})
