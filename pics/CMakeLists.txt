kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kuickshow/pics FILES calibrate.png kuickshow-day.jpg kuickshow-night.jpg logo.png  	imageviewer-medium.png imageviewer-small.png handcursor.png  	ksslide.png about.png )

kde4_footer()



#original Makefile.am contents follow:

## Add all of your pixmaps here
#pics_DATA = calibrate.png kuickshow-day.jpg kuickshow-night.jpg logo.png \
#	imageviewer-medium.png imageviewer-small.png handcursor.png \
#	ksslide.png about.png
#
## This is where it will all be installed
#picsdir = $(kde_datadir)/kuickshow/pics
