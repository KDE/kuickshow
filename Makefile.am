SUBDIRS = src misc pics

kuickshow-package:
	@export CVSROOT=`cat CVS/Root`; \
	kuickdir=$$PWD; \
	kuickver=`cat src/version.h | grep define | cut -f3 -d' ' | sed -e 's/"//g'`; \
	cd /tmp; \
	cvs2dist `dirname $$kuickdir` kuickshow --remove-hidden --version $$kuickver $$kuickdir/TODO $$kuickdir/BUGS $$kuickdir/kuickshow.lsm $$kuickdir/kuickshow.spec $$kuickshow/../Makefile.cvs
