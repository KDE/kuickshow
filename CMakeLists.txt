project(kuickshow)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(KDE4 REQUIRED)
find_package(IMLIB REQUIRED)

include(KDE4Defaults)

add_subdirectory( src ) 
add_subdirectory( misc ) 
add_subdirectory( pics ) 
add_subdirectory( icons )


########### install files ###############





#original Makefile.am contents follow:

#SUBDIRS = src misc pics
#
#kuickshow-package:
#	@export CVSROOT=`cat CVS/Root`; \
#	kuickdir=$$PWD; \
#	kuickver=`cat src/version.h | grep define | cut -f3 -d' ' | sed -e 's/"//g'`; \
#	cd /tmp; \
#	cvs2dist `dirname $$kuickdir` kuickshow --remove-hidden --version $$kuickver $$kuickdir/TODO $$kuickdir/BUGS $$kuickdir/kuickshow.lsm $$kuickdir/kuickshow.spec $$kuickshow/../Makefile.cvs
