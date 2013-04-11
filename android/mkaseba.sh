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

script_init "Build ASEBA for Android"
script_setopt "--ndk-root" "PATH" ANDROIDNDKROOT "/opt/android/ndk" \
  "path to the Android NDK root directory"
script_setopt "--api-level|-a" "LEVEL" ANDROIDAPILEVEL 9 \
  "level of the Android API"
script_setopt "--abi" "ABI" ANDROIDABI "armeabi-v7a" \
  "name of the Android ABI"
script_setopt "--necessitas-root" "PATH" NECESSITASROOT "/opt/necessitas" \
  "path to the Necessitas root directory"
script_setopt "--toolchain|-t" "FILE" CMAKETOOLCHAIN "cmake/Android.cmake" \
  "name of the CMake toolchain file"
script_setopt "--sources" "PATH" ASEBASOURCES ../ \
  "path to the ASEBA sources"
script_setopt "--dashel-sources" "PATH" DASHELSOURCES ../../dashel \
  "path to the DaSHEL sources"
script_setopt "--build-root|-b" "PATH" BUILDROOT "build" \
  "path to the build root directory"
script_setopt "--debug" "" BUILDDEBUG false \
  "build debug targets instead of release"
script_setopt "--jobs|-j" "" MAKEJOBS 3 "number of simultaneous make jobs"
script_setopt "--clean" "" ASEBACLEAN false "remove build directory"

script_checkopts $*

[ -d "$BUILDROOT/aseba" ] || execute "mkdir -p $BUILDROOT/aseba"

fs_abspath $BUILDROOT BUILDROOT

if false ASEBACLEAN; then
  fs_abspath $ANDROIDNDKROOT ANDROIDNDKROOT
  fs_abspath $NECESSITASROOT NECESSITASROOT
  fs_abspath $CMAKETOOLCHAIN CMAKETOOLCHAIN
  fs_abspath $ASEBASOURCES ASEBASOURCES
  fs_abspath $DASHELSOURCES DASHELSOURCES

  CMAKEOPTS="-DANDROID_NDK=$ANDROIDNDKROOT"
  CMAKEOPTS="$CMAKEOPTS -DANDROID_NATIVE_API_LEVEL=$ANDROIDAPILEVEL"
  CMAKEOPTS="$CMAKEOPTS -DNECESSITAS=$NECESSITASROOT"
  CMAKEOPTS="$CMAKEOPTS -DCMAKE_TOOLCHAIN_FILE=$CMAKETOOLCHAIN"
  CMAKEOPTS="$CMAKEOPTS -DLIBRARY_OUTPUT_PATH_ROOT=$BUILDROOT/thymiovpl"
  CMAKEOPTS="$CMAKEOPTS -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="
  CMAKEOPTS="$CMAKEOPTS -DCMAKE_INCLUDE_PATH=$DASHELSOURCES"
  CMAKEOPTS="$CMAKEOPTS -DANDROID_ABI=$ANDROIDABI"

  if true BUILDDEBUG; then
    CMAKEOPTS="$CMAKEOPTS -DCMAKE_BUILD_TYPE=Debug"
  else
    CMAKEOPTS="$CMAKEOPTS -DCMAKE_BUILD_TYPE=Release"
  fi

  cd $BUILDROOT/aseba && cmake $CMAKEOPTS $ASEBASOURCES && \
    make -j $MAKEJOBS thymiovpl
else
  execute "rm -rf $BUILDROOT/aseba"
fi

log_clean
