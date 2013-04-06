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

fs_abspath thymiovpl THYMIOVPLSOURCES

script_init "Build Thymio VPL for Android"
script_setopt "--sdk-root" "PATH" ANDROIDSDKROOT "/opt/android/sdk" \
  "path to the Android SDK root directory"
script_setopt "--sources|-s" "PATH" THYMIOVPLSOURCES $THYMIOVPLSOURCES \
  "path to the Thymio VPL sources"
script_setopt "--api-level|-a" "LEVEL" ANDROIDAPILEVEL 17 \
  "level of the Android API"
script_setopt "--build-dir|-b" "PATH" THYMIOVPLBUILD "build" \
  "path to the Thymio VPL build root"
script_setopt "--debug" "" ANTBUILDDEBUG false \
  "build debug target instead of release"
script_setopt "--install" "" THYMIOVPLINSTALL false \
  "install package to Android device"
script_setopt "--clean" "" THYMIOVPLCLEAN false "remove build directory"

script_checkopts $*

[ -d "$THYMIOVPLBUILD/thymiovpl" ] || \
  execute "mkdir -p $THYMIOVPLBUILD/thymiovpl"

fs_abspath $ANDROIDSDKROOT ANDROIDSDKROOT
fs_abspath $THYMIOVPLSOURCES THYMIOVPLSOURCES
fs_abspath $THYMIOVPLBUILD THYMIOVPLBUILD

if true ANTBUILDDEBUG; then
  ANTBUILDTARGET=debug
  THYMIOVPLPKG=thymiovpl-debug-unaligned.apk
else
  ANTBUILDTARGET=release
  THYMIOVPLPKG=thymiovpl-release-unsigned.apk
fi

cd $THYMIOVPLSOURCES && \
  export ANDROID_SDK_ROOT=$ANDROIDSDKROOT && \
  export ANDROID_API_LEVEL=$ANDROIDAPILEVEL && \
  export THYMIO_VPL_BUILD=$THYMIOVPLBUILD/thymiovpl && \
  ant $ANTBUILDTARGET && \
  ANTBUILDSUCCESS=true

if true ANTBUILDSUCCESS; then
  if true THYMIOVPLINSTALL; then
    $ANDROIDSDKROOT/platform-tools/adb -d install -r \
      $THYMIOVPLBUILD/thymiovpl/$THYMIOVPLPKG
  fi
fi

if true THYMIOVPLCLEAN; then
  execute "rm -rf $THYMIOVPLBUILD/thymiovpl"
fi

log_clean
