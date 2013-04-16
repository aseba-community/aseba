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

script_init "Build Thymio VPL and dependencies for Android"
script_setopt "--ndk-root" "PATH" ANDROIDNDKROOT "/opt/android/ndk" \
  "path to the Android NDK root directory"
script_setopt "--sdk-root" "PATH" ANDROIDSDKROOT "/opt/android/sdk" \
  "path to the Android SDK root directory"
script_setopt "--api-level|-a" "LEVEL" ANDROIDAPILEVEL 12 \
  "level of the Android API"
script_setopt "--abi" "ABI" ANDROIDABI "armeabi-v7a" \
  "name of the Android ABI"
script_setopt "--necessitas-root" "PATH" NECESSITASROOT "/opt/necessitas" \
  "path to the Necessitas root directory"
script_setopt "--dashel-sources" "PATH" DASHELSOURCES ../../dashel \
  "path to the DaSHEL sources"
script_setopt "--aseba-sources" "PATH" ASEBASOURCES ../ \
  "path to the ASEBA sources"
script_setopt "--thymiovpl-sources" "PATH" THYMIOVPLSOURCES thymiovpl \
  "path to the Thymio VPL sources"
script_setopt "--build-root|-b" "PATH" BUILDROOT "build" \
  "path to the build root directory"
script_setopt "--debug" "" BUILDDEBUG false \
  "build debug targets instead of release"
script_setopt "--emu" "" USEEMU false \
  "use emulator instead of real device"
script_setopt "--install" "" ALLINSTALL false \
  "install package to Android device"
script_setopt "--clean" "" ALLCLEAN false \
  "remove build directories"
script_setopt "--antprop" "" ANTPROP "ant.properties" \
  "ant.properties file for signing"


script_checkopts $*

if false ALLCLEAN; then
  COMMONARGS="--build-root $BUILDROOT"
  COMMONARGS="$COMMONARGS --api-level $ANDROIDAPILEVEL"
  if true BUILDDEBUG; then
    COMMONARGS="$COMMONARGS --debug"
  fi
  if true ALLINSTALL; then
    THYMIOVPLARGS="--install"
  fi
  if true USEEMU; then
    THYMIOVPLARGS="$THYMIOVPLARGS --emu"
  fi

  ./mkdashel.sh $COMMONARGS --ndk-root $ANDROIDNDKROOT --abi $ANDROIDABI && \
  ./mkaseba.sh $COMMONARGS --ndk-root $ANDROIDNDKROOT --necessitas-root \
    $NECESSITASROOT --abi $ANDROIDABI && \
  ./mkthymiovpl.sh $COMMONARGS $THYMIOVPLARGS --sdk-root $ANDROIDSDKROOT --antprop $ANTPROP
else
  ./mkdashel.sh --clean && \
  ./mkaseba.sh --clean && \
  ./mkthymiovpl.sh --clean
fi

log_clean
