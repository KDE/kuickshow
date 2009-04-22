
include_directories(${IMLIB_INCLUDE_DIR} ${KDE4_INCLUDES})

add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

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
   imagemods.cpp
   printing.cpp )

kde4_add_kdeinit_executable( kuickshow ${kuickshow_KDEINIT_SRCS})

target_link_libraries(kdeinit_kuickshow ${X11_LIBRARIES} ${IMLIB_LIBRARIES} ${QT_QT3SUPPORT_LIBRARY} ${KDE4_KIO_LIBS} ${KDE4_KFILE_LIBS})

install(TARGETS kdeinit_kuickshow  ${INSTALL_TARGETS_DEFAULT_ARGS} )

target_link_libraries( kuickshow kdeinit_kuickshow )
install(TARGETS kuickshow  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES kuickshow.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

kde4_install_icons(${ICON_INSTALL_DIR})
