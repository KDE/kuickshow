
include_directories(${IMLIB_INCLUDE_DIR})

########### next target ###############

set(kuickshow_KDEINIT_SRCS 
   kuickshow.cpp 
   aboutwidget.cpp 
   generalwidget.cpp 
   kuickconfigdlg.cpp 
   main.cpp 
   defaultswidget.cpp 
   imagewindow.cpp 
   kuickdata.cpp 
   imdata.cpp 
   filefinder.cpp 
   kurlwidget.cpp 
   filewidget.cpp 
   kuickio.cpp 
   kuick.cpp 
   imlibwidget.cpp 
   slideshowwidget.cpp 
   printing.cpp )

kde4_automoc(${kuickshow_KDEINIT_SRCS})

kde4_add_kdeinit_executable( kuickshow ${kuickshow_KDEINIT_SRCS})

target_link_libraries(kdeinit_kuickshow ${IMLIB_LIBRARIES} ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KDEPRINT_LIBS} ${KDE4_KFILE_LIBS})

install(TARGETS kdeinit_kuickshow  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kuickshow kdeinit_kuickshow )
install(TARGETS kuickshow  DESTINATION ${BIN_INSTALL_DIR} )

########### install files ###############

install( FILES kuickshow.desktop  DESTINATION  ${XDG_APPS_DIR} )

kde4_install_icons(${ICON_INSTALL_DIR})
