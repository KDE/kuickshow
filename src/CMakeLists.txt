kde4_header()




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

target_link_libraries(kdeinit_kuickshow  ${KDE4_KDE3SUPPORT_LIBS} kdeprint )

install_targets(${LIB_INSTALL_DIR} kdeinit_kuickshow )

target_link_libraries( kuickshow kdeinit_kuickshow )
install_targets(/bin kuickshow )

########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kuickshow.desktop )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )

kde4_footer()



#original Makefile.am contents follow:

### Makefile.am for kuickshow
#
#bin_PROGRAMS =
#lib_LTLIBRARIES =
#kdeinit_LTLIBRARIES = kuickshow.la
#
## set the include path for X, qt and KDE
#INCLUDES = $(all_includes)
#KDE_CXXFLAGS = $(IMLIB_CFLAGS)
#METASOURCES = AUTO
#
#kuickshow_la_LDFLAGS = $(all_libraries) -module -avoid-version
#kuickshow_la_LIBADD  = $(LIB_KDEPRINT) $(LIB_IMLIB)
#kuickshow_la_SOURCES =	kuickshow.cpp \
#	aboutwidget.cpp     generalwidget.cpp  kuickconfigdlg.cpp  main.cpp \
#	defaultswidget.cpp  imagewindow.cpp    kuickdata.cpp  \
#	imdata.cpp filefinder.cpp kurlwidget.cpp filewidget.cpp \
#	kuickio.cpp kuick.cpp imlibwidget.cpp slideshowwidget.cpp printing.cpp
#
## if you "make distclean", this files get removed. If you want to remove
## them while "make clean", use CLEANFILES
#DISTCLEANFILES = $(METASOURCES)
#
#messages:
#	$(XGETTEXT) *.cpp *.h -o $(podir)/kuickshow.pot
#
#KDE_ICON = kuickshow
#
# # this is where the kdelnk file will go 
#xdg_apps_DATA = kuickshow.desktop
