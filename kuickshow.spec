%define version 0.8.5
%define release 1
%define serial  1
%define prefix /opt/kde3

Name:      kuickshow
Summary:   KuickShow -- A very fast image viewer/browser
Version:   %{version}
Release:   %{release}
Serial:    %{serial}
Source:    http://devel-home.kde.org/~pfeiffer/kuickshow/kuickshow-%{version}.tar.gz
URL:       http://devel-home.kde.org/~pfeiffer/kuickshow/
Copyright: GPL
Packager:  Carsten Pfeiffer <pfeiffer@kde.org>
Group:     X11/KDE/Graphics
BuildRoot: /tmp/kuickshow-%{version}-root
Prefix:    %{prefix}

%description
KuickShow is a very fast image viewer, that lets you easily
browse large galleries. A builtin filebrowser and manager
is also available. Usage is somewhat inspired by ACDSee.
It supports many fileformats, e.g. jpeg, gif, png, psd, bmp, 
tiff, xpm, xbm, xcf, eim, ...

KuickShow has a nice user interface, that allows you to browse large amounts
of images in a short time. It can zoom, mirror, rotate images, adjust
brightness, contrast and gamma and can do a slideshow, of course.
It is fully configurable through dialogs.
 
Besides that, it offers a nice filebrowser with basic filemanager capabilities
like renaming, deleting, creating directories, ...
 
Install with '--prefix $KDEDIR' unless you have the kde-config program.

%prep
rm -rf $RPM_BUILD_ROOT

%setup -n kuickshow-%{version}

%build
PREFIX=""
which kde-config || PREFIX=%{prefix}
if test -z "$PREFIX"; then
  PREFIX=`kde-config --prefix`
fi

export KDEDIR="$PREFIX"
CXXFLAGS="$RPM_OPT_FLAGS -fno-exceptions -pipe" LDFLAGS=-s ./configure --prefix="$PREFIX" --enable-final --disable-debug
mkdir -p $RPM_BUILD_ROOT
make

%install
make install DESTDIR=$RPM_BUILD_ROOT

cd $RPM_BUILD_ROOT
 
find . -type d | sed '1,2d;s,^\.,\%attr(-\,root\,root) \%dir ,' > $RPM_BUILD_DIR/file.list.%{name}
 
find . -type f | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.%{name}
 
find . -type l | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.%{name}

%clean
rm -rf $RPM_BUILD_ROOT
rm -f $RPM_BUILD_DIR/file.list.%{name}

%files -f ../file.list.%{name}

