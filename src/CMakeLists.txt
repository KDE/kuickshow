
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
   kuickurlwidget.cpp
   filewidget.cpp 
   kuick.cpp 
   imlibwidget.cpp 
   slideshowwidget.cpp 
   imagemods.cpp
   printing.cpp )

kde4_add_ui_files(kuickshow_KDEINIT_SRCS
   aboutwidget.ui
   printing_page.ui
   generalwidget.ui
   defaultswidget.ui
   slideshowwidget.ui
)


kde4_add_kdeinit_executable( kuickshow ${kuickshow_KDEINIT_SRCS})

target_link_libraries(kdeinit_kuickshow ${X11_LIBRARIES} ${IMLIB_LIBRARIES} ${KDE4_KIO_LIBS} ${KDE4_KFILE_LIBS})

install(TARGETS kdeinit_kuickshow  ${INSTALL_TARGETS_DEFAULT_ARGS} )

target_link_libraries( kuickshow kdeinit_kuickshow )
install(TARGETS kuickshow  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( PROGRAMS kuickshow.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

kde4_install_icons(${ICON_INSTALL_DIR})
