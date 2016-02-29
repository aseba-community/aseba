Name:           aseba

# Update the following lines to reflect the source release version you will be
# referencing below
%global source_major 1
%global source_minor 5
%global source_patch 1
Version:        %{source_major}.%{source_minor}.%{source_patch}

# Update the following line with the git commit hash of the revision to use
# for example by running git show-ref -s --tags RELEASE_TAG
%global commit 635a92648a98eb586ba1daac118b7be1b6ee98d1
%global shortcommit %(c=%{commit}; echo ${c:0:7})

# Update the following line with the git commit has of the revision of Catch
# associated with the Catch submodule. Run 'git submodule' to find it.
%global catchcommit 3b18d9e962835100d7e12ac80d22882e408e40dc

# Update the following line to set commit_is_tagged_as_source_release to 0 if
# and only if the commit hash is not from a git tag for an existing source
# release (i.e. it is a commit hash for a pre-release or post-release
# revision). Otherwise set it to 1.
%global commit_is_tagged_as_source_release 1
%if %{commit_is_tagged_as_source_release} == 0
  %global snapshot .%(date +%%Y%%m%%d)git%{shortcommit}
%endif

# Update the number(s) in the "Release:" line below as follows. If this is 
# the first RPM release for a particular source release version, then set the
# number to 0. If this is the first RPM pre-release for a future source
# release version (i.e. the "Version:" line above refers to a future
# source release version), then set the number to 0.0. Otherwise, leave the
# the number unchanged. It will get bumped when you run rpmdev-bumpspec.
Release:        1%{?snapshot}%{?dist}
Summary:        A set of tools which allow beginners to program robots easily and efficiently

%global lib_pkg_name lib%{name}%{source_major}

%if 0%{?suse_version}
%global buildoutdir build
%else
%global buildoutdir .
%endif

%if 0%{?suse_version}
License:        LGPL-3.0
%else
License:        LGPLv3
%endif
URL:            http://aseba.wikidot.com
Source0:        https://github.com/aseba-community/aseba/archive/%{commit}.tar.gz
Source1:        https://github.com/philsquared/Catch/archive/%{catchcommit}.tar.gz
Patch0:         aseba-rpm.patch

BuildRequires: ImageMagick
BuildRequires: ImageMagick-devel
BuildRequires: SDL-devel
BuildRequires: binutils
BuildRequires: cmake
BuildRequires: dashel-devel >= 1.1.0
BuildRequires: desktop-file-utils
BuildRequires: elfutils
BuildRequires: enki-devel
BuildRequires: file
BuildRequires: gdb
BuildRequires: glibc-devel
BuildRequires: kernel-headers
BuildRequires: libstdc++-devel
BuildRequires: libxml2-devel
%if 0%{?suse_version}
BuildRequires: hicolor-icon-theme
BuildRequires: Mesa-libGL-devel
BuildRequires: Mesa-libGLU-devel
# SUSE puts qcollectiongenerator in qt-devel-doc instead of qt-devel
BuildRequires: qt-devel-doc
%else
BuildRequires: mesa-libGL-devel
BuildRequires: mesa-libGLU-devel
%endif
BuildRequires: qt-devel
BuildRequires: qwt-devel
BuildRequires: gcc-c++
BuildRequires: doxygen


%description
Aseba is an event-based architecture for real-time distributed control of 
mobile robots. It targets integrated multiprocessor robots or groups of 
single-processor units, real or simulated. The core of aseba is a 
lightweight virtual machine tiny enough to run even on microcontrollers. 
With aseba, we program robots in a user-friendly programming language 
using a cozy integrated development environment.

%package -n %{lib_pkg_name}
Summary:        Libraries for %{name}
Group: System/Libraries
Requires:       libdashel1 >= 1.1.0

%description  -n %{lib_pkg_name}
The %{lib_pkg_name} package contains libraries running applications that use
%{name}.

%package        devel
Summary:        Development files for %{name}
Requires:       %{lib_pkg_name}%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%prep
%setup -q -n %{name}-%{commit}
%setup -q -T -D -b 1 -n %{name}-%{commit}
rm -rf tests/externals/Catch
mv ../Catch-%{catchcommit} tests/externals/Catch
%patch0 -p1

%build
%cmake
make 
doxygen %{_builddir}/%{buildsubdir}/Doxyfile

%install
rm -rf $RPM_BUILD_ROOT
cd %{buildoutdir}
make install DESTDIR=$RPM_BUILD_ROOT
rm -rf ${RPM_BUILD_ROOT}%{_bindir}/asebatest
rm -rf ${RPM_BUILD_ROOT}%{_bindir}/aseba-test-natives-count
rm -rf ${RPM_BUILD_ROOT}%{_bindir}/asebashell
rm -rf ${RPM_BUILD_ROOT}%{_bindir}/aseba-qt-gui
rm -rf ${RPM_BUILD_ROOT}%{_bindir}/aseba-qt-dbus

cd examples
make clean
rm -rf *.cmake CMakeFiles Makefile 
rm -rf clients/*.cmake clients/CMakeFiles
rm -rf clients/*/*.cmake clients/*/CMakeFiles
cd %{_builddir}/%{buildsubdir}

install -d ${RPM_BUILD_ROOT}%{_datadir}/applications
install -d ${RPM_BUILD_ROOT}%{_datadir}/icons/hicolor/48x48/apps
install -d ${RPM_BUILD_ROOT}%{_datadir}/icons/hicolor/scalable/apps
cp menu/freedesktop/*.desktop ${RPM_BUILD_ROOT}%{_datadir}/applications
cp menu/freedesktop/48x48/*.png ${RPM_BUILD_ROOT}%{_datadir}/icons/hicolor/48x48/apps
cp menu/src/*.svg ${RPM_BUILD_ROOT}%{_datadir}/icons/hicolor/scalable/apps

desktop-file-install --remove-category="Aseba" --dir=${RPM_BUILD_ROOT}%{_datadir}/applications ${RPM_BUILD_ROOT}%{_datadir}/applications/*.desktop

%check
#ctest

%post
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :

%post -n %{lib_pkg_name}
/sbin/ldconfig

%postun
if [ $1 -eq 0 ] ; then
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    /usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi

%postun -n %{lib_pkg_name}
/sbin/ldconfig

%posttrans
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files
%doc debian/copyright debian/changelog readme.md targets/playground/unifr.playground targets/challenge/examples/challenge-goto-energy.aesl debian/README.Debian
%{_bindir}/*
%{_datadir}/applications/*
%{_datadir}/icons/hicolor/48x48/apps/*
%{_datadir}/icons/hicolor/scalable/apps/*

%files -n %{lib_pkg_name}
%doc debian/copyright
%{_libdir}/*.so.*

%files devel
%doc %{buildoutdir}/doc/* examples/*
%{_includedir}/*
%{_libdir}/*.so

%changelog
* Mon Feb 29 2016 Dean Brettle <dean@brettle.com> - 1.5.1-1
- Sync with upstream 1.5.1

* Thu Nov 19 2015 Dean Brettle <dean@brettle.com> - 1.5.0-0.4.20151119git2fae615
- Require libdashel1 >= 1.1.0

* Tue Oct 13 2015 Dean Brettle <dean@brettle.com> - 1.4.0-0.3.20151013gitb2255c6
- Incorporate fixes needed by other platforms and package systems

* Fri Sep 11 2015 Dean Brettle <dean@brettle.com> - 1.4.0-0.2.20150910git04b42d4
- Sync with latest upstream master and fix build errors.

* Wed Sep 09 2015 Dean Brettle <dean@brettle.com> - 1.4.0-0.1.20150909git10ef28e
- Sync with latest upstream master

* Fri Jun 20 2014 Dean Brettle <dean@brettle.com> - 1.4.0-0.3.20140620gitaaf2e88
- Sync with latest upstream master.
- Make build require at least 1.0.8 of dashel-devel to get required
  dashelConfig.cmake file.

* Tue May 06 2014 Dean Brettle <dean@brettle.com> - 1.4.0-0.2.20140505gitaaf2e88
- Removed dashel's dependencies since linking to dashel should pull them in
  automatically.

* Fri Apr 18 2014 Dean Brettle <dean@brettle.com> - 1.4.0-0.1.20140419git38a1847
- Using 1.4.0-0.xYYYYMMDDgitSHORTCOMMIT for releases based on master snapshots.

* Wed Apr 16 2014 Dean Brettle <dean@brettle.com> - 1.3.2-0.5.20140416git96fb018
- Updated to latest 1.3.2 prerelease, added asebavmbuffer shared lib and headers,
  and made examples/clients buildable after installation.

* Mon Apr 14 2014 Dean Brettle <dean@brettle.com> - 1.3.2-0.4.20140414git8358704
- Added vm/*.h to the aseba-devel package.

* Fri Apr 11 2014 Dean Brettle <dean@brettle.com> - 1.3.2-0.4.20140411git8358704
- Updated to latest 1.3.2 prerelease and made some libs static and not installed.

* Mon Mar 03 2014 Dean Brettle <dean@brettle.com> - 1.3.2-0.3.20140303gitf44652d
- Updated spec to build on openSUSE and put libs in libaseba1 package.

* Sat Mar 01 2014 Dean Brettle <dean@brettle.com> - 1.3.2-0.2.20140228gitf44652d
- Changed SO_VERSION to SOVERSION so that libs only use major version number and
  added rpm directory with spec file and RPM building instructions.

* Thu Feb 27 2014 Dean Brettle <dean@brettle.com> - 1.3.2-0.1.20140223gitf44652d
- Built shared libs for additional -devel subpackage

* Sun Feb 23 2014 Dean Brettle <dean@brettle.com> - 1.3-1.20140223gitf44652d
- Initial release
