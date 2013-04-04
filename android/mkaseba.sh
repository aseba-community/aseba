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

fs_abspath ../../dashel DASHELSOURCES

script_init "Build DaSHEL for Android"
script_setopt "--ndk-root|-r" "PATH" NDKROOT "/opt/android/ndk" \
  "path to the Android NDK root directory"
script_setopt "--api-level|-a" "LEVEL" ANDROIDAPILEVEL 9 \
  "level of the Android API"
script_setopt "--toolchain|-t" "FILE" CMAKETOOLCHAIN "cmake/Android.cmake" \
  "name of the CMake toolchain file"
script_setopt "--sources|-s" "PATH" DASHELSOURCES $DASHELSOURCES \
  "path to the DaSHEL sources"
script_setopt "--build-dir|-b" "PATH" DASHELBUILD "build/dashel" \
  "path to the DaSHEL build directory"
script_setopt "--build-type" "TYPE" CMAKEBUILDTYPE "Release" \
  "the type of build"
script_setopt "--jobs|-j" "" MAKEJOBS 3 "number of simultaneous make jobs"
script_setopt "--clean" "" DASHELCLEAN false "remove build directory"

script_checkopts $*

[ -d "$DASHELBUILD" ] || execute "mkdir -p $DASHELBUILD"

fs_abspath $NDKROOT NDKROOT
fs_abspath $CMAKETOOLCHAIN CMAKETOOLCHAIN
fs_abspath $DASHELSOURCES DASHELSOURCES
fs_abspath $DASHELBUILD DASHELBUILD

CMAKEOPTS="-DANDROID_NDK=$NDKROOT"
CMAKEOPTS="$CMAKEOPTS -DANDROID_NATIVE_API_LEVEL=$ANDROIDAPILEVEL"
CMAKEOPTS="$CMAKEOPTS -DCMAKE_BUILD_TYPE=$CMAKEBUILDTYPE"
CMAKEOPTS="$CMAKEOPTS -DCMAKE_TOOLCHAIN_FILE=$CMAKETOOLCHAIN"
CMAKEOPTS="$CMAKEOPTS -DLIBRARY_OUTPUT_PATH= -DEXECUTABLE_OUTPUT_PATH="

cd $DASHELBUILD && cmake $CMAKEOPTS $DASHELSOURCES && make -j $MAKEJOBS install

if true DASHELCLEAN; then
  execute "rm -rf $DASHELBUILD"
fi

log_clean
