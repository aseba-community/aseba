#!/bin/bash
############################################################################
#    Copyright (C) 2013 by Ralf Kaestner                                   #
#    ralf.kaestner@gmail.com                                               #
#                                                                          #
#    This program is free software; you can redistribute it and or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

. ubash

script_init "Build Thymio VPL for Android"
script_setopt "--sdk-root" "PATH" ANDROIDSDKROOT "/opt/android/sdk" \
  "path to the Android SDK root directory"
script_setopt "--api-level|-a" "LEVEL" ANDROIDAPILEVEL 12 \
  "level of the Android API"
script_setopt "--sources" "PATH" THYMIOVPLSOURCES thymiovpl \
  "path to the Thymio VPL sources"
script_setopt "--build-root|-b" "PATH" BUILDROOT "build" \
  "path to the build root directory"
script_setopt "--debug" "" BUILDDEBUG false \
  "build debug targets instead of release"
script_setopt "--emu" "" USEEMU false \
  "use emulator instead of real device"
script_setopt "--install" "" THYMIOVPLINSTALL false \
  "install package to Android device"
script_setopt "--clean" "" THYMIOVPLCLEAN false \
  "remove build directory"
script_setopt "--antprop" "" ANTPROP "ant.properties" \
  "ant.properties file for signing"

script_checkopts $*

[ -d "$BUILDROOT/thymiovpl" ] || execute "mkdir -p $BUILDROOT/thymiovpl"

fs_abspath $BUILDROOT BUILDROOT

if false THYMIOVPLCLEAN; then
  fs_abspath $ANDROIDSDKROOT ANDROIDSDKROOT
  fs_abspath $THYMIOVPLSOURCES THYMIOVPLSOURCES

  if true BUILDDEBUG; then
    ANTBUILDTARGET=debug
    THYMIOVPLPKG=thymiovpl-debug.apk
  else
    ANTBUILDTARGET=release
    THYMIOVPLPREPKG=thymiovpl-release-unsigned.apk
    THYMIOVPLPKG=thymiovpl-release.apk
  fi

  cp -aT $THYMIOVPLSOURCES $BUILDROOT/thymiovpl && \
    cd $BUILDROOT/thymiovpl && \
    export ANDROID_SDK_ROOT=$ANDROIDSDKROOT && \
    export ANDROID_API_LEVEL=$ANDROIDAPILEVEL && \
    export DEBUGGABLE=$BUILDDEBUG && \
    ant -propertyfile $ANTPROP $ANTBUILDTARGET && \
    ANTBUILDSUCCESS=true
  
  if true ANTBUILDSUCCESS; then
    if true THYMIOVPLINSTALL; then
      if true USEEMU; then
        echo "Installing to emulator"
        $ANDROIDSDKROOT/platform-tools/adb -e install -r \
          $BUILDROOT/thymiovpl/bin/$THYMIOVPLPKG      
      else
        echo "Installing to real device"
        $ANDROIDSDKROOT/platform-tools/adb -d install -r \
          $BUILDROOT/thymiovpl/bin/$THYMIOVPLPKG
      fi
    fi
  fi
else
  execute "rm -rf $BUILDROOT/thymiovpl"
fi

log_clean
