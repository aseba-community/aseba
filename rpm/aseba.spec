Name:           aseba

# Update the following line to reflect the source release version you will be
# referencing below
Version:        1.3.2

# Update the following line with the git commit hash of the revision to use
# for example by running git show-ref -s --tags RELEASE_TAG
%global commit f44652ddf4c3aed2ef8e89e0ff0e3441cb51deef
%global shortcommit %(c=%{commit}; echo ${c:0:7})

# Update the following line to set commit_is_tagged_as_source_release to 0 if
# and only if the commit hash is not from a git tag for an existing source
# release (i.e. it is a commit hash for a pre-release or post-release
# revision). Otherwise set it to 1.
%global commit_is_tagged_as_source_release 0
%if %{commit_is_tagged_as_source_release} == 0
  %global snapshot .%(date +%%Y%%m%%d)git%{shortcommit}
%endif

# Update the number(s) in the "Release:" line below as follows. If this is 
# the first RPM release for a particular source release version, then set the
# number to 0. If this is the first RPM pre-release for a future source
# release version (i.e. the "Version:" line above refers to a future
# source release version), then set the number to 0.0. Otherwise, leave the
# the number unchanged. It will get bumped when you run rpmdev-bumpspec.
Release:        0.2%{?snapshot}%{?dist}
Summary:        A set of tools which allow beginners to program robots easily and efficiently

License:        LGPLv3
URL:            http://aseba.wikidot.com
Source0:        https://github.com/aseba-community/aseba/archive/%{commit}/%{name}-%{version}-%{shortcommit}.tar.gz
Patch0:         aseba-rpm.patch

BuildRequires: ImageMagick
BuildRequires: ImageMagick-libs
BuildRequires: SDL-devel
BuildRequires: binutils
BuildRequires: ccache
BuildRequires: cmake
BuildRequires: dashel-devel
BuildRequires: desktop-file-utils
BuildRequires: dwz
BuildRequires: elfutils
BuildRequires: enki-devel
BuildRequires: file
BuildRequires: gdb
BuildRequires: glibc-devel
BuildRequires: glibc-headers
BuildRequires: kernel-headers
BuildRequires: libstdc++-devel
BuildRequires: libxml2-devel
BuildRequires: mesa-libGL-devel
BuildRequires: qt-devel
BuildRequires: qwt-devel

%description
Aseba is an event-based architecture for real-time distributed control of 
mobile robots. It targets integrated multiprocessor robots or groups of 
single-processor units, real or simulated. The core of aseba is a 
lightweight virtual machine tiny enough to run even on microcontrollers. 
With aseba, we program robots in a user-friendly programming language 
using a cozy integrated development environment.

%package        devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%prep
%setup -q -n %{name}-%{commit}
%patch0 -p1

%build
%cmake .
make 
doxygen

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
rm -rf ${RPM_BUILD_ROOT}%{_bindir}/asebatest
rm -rf ${RPM_BUILD_ROOT}%{_bindir}/aseba-test-natives-count
rm -rf ${RPM_BUILD_ROOT}%{_bindir}/asebashell
rm -rf ${RPM_BUILD_ROOT}%{_bindir}/qt-gui

cd examples
make clean
rm -rf *.cmake CMakeFiles Makefile 
rm -rf clients/*.cmake clients/CMakeFiles
rm -rf clients/*/*.cmake clients/*/CMakeFiles
cd ..

install -d ${RPM_BUILD_ROOT}%{_datadir}/applications
install -d ${RPM_BUILD_ROOT}%{_datadir}/icons/hicolor/48x48/apps
cp menu/freedesktop/*.desktop ${RPM_BUILD_ROOT}%{_datadir}/applications
cp menu/freedesktop/*.png ${RPM_BUILD_ROOT}%{_datadir}/icons/hicolor/48x48/apps
convert ${RPM_BUILD_ROOT}%{_datadir}/icons/hicolor/48x48/apps/thymiovpl.png -resize 48x48 ${RPM_BUILD_ROOT}%{_datadir}/icons/hicolor/48x48/apps/thymiovpl.png
for file in ${RPM_BUILD_ROOT}%{_datadir}/applications/*.desktop; do
   sed -i 's|Icon=\(.*\).png|Icon=\1|g' $file
done

desktop-file-install --remove-category="Aseba" --dir=${RPM_BUILD_ROOT}%{_datadir}/applications ${RPM_BUILD_ROOT}%{_datadir}/applications/*.desktop

%check
#ctest

%post
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :
/sbin/ldconfig

%postun
if [ $1 -eq 0 ] ; then
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    /usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi
/sbin/ldconfig

%posttrans
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files
%doc debian/copyright debian/changelog readme.md targets/playground/unifr.playground targets/challenge/examples/challenge-goto-energy.aesl debian/README.Debian
%{_bindir}/*
%{_libdir}/*.so.*
%{_datadir}/applications/*
%{_datadir}/icons/hicolor/48x48/apps/*

%files devel
%doc doc/* examples/*
%{_includedir}/*
%{_libdir}/*.so

%changelog
* Sat Mar 01 2014 Dean Brettle <dean@brettle.com> - 1.3.2-0.2.20140228gitf44652d
- Changed SO_VERSION to SOVERSION so that libs only use major version number and
  added rpm directory with spec file and RPM building instructions.

* Thu Feb 27 2014 Dean Brettle <dean@brettle.com> - 1.3.2-0.1.20140223gitf44652d
- Built shared libs for additional -devel subpackage

* Sun Feb 23 2014 Dean Brettle <dean@brettle.com> - 1.3-1.20140223gitf44652d
- Initial release
