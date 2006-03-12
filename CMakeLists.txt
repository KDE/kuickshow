kde4_header()

add_subdirectory( src ) 
add_subdirectory( misc ) 
add_subdirectory( pics ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = src misc pics
#
#kuickshow-package:
#	@export CVSROOT=`cat CVS/Root`; \
#	kuickdir=$$PWD; \
#	kuickver=`cat src/version.h | grep define | cut -f3 -d' ' | sed -e 's/"//g'`; \
#	cd /tmp; \
#	cvs2dist `dirname $$kuickdir` kuickshow --remove-hidden --version $$kuickver $$kuickdir/TODO $$kuickdir/BUGS $$kuickdir/kuickshow.lsm $$kuickdir/kuickshow.spec $$kuickshow/../Makefile.cvs
