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

script_init "Debug Thymio VPL on Android"
script_setopt "--ndk-root" "PATH" ANDROIDNDKROOT "/opt/android/ndk" \
  "path to the Android NDK root directory"
script_setopt "--sdk-root" "PATH" ANDROIDSDKROOT "/opt/android/sdk" \
  "path to the Android SDK root directory"
script_setopt "--necessitas-root" "PATH" NECESSITASROOT "/opt/necessitas" \
  "path to the Necessitas root directory"
script_setopt "--build-root|-b" "PATH" BUILDROOT "build" \
  "path to the build root directory"
script_setopt "--debug-port|-p" "PORT" DEBUGPORT 5039 \
  "network port used for debugging"

script_checkopts $*

fs_abspath $ANDROIDNDKROOT ANDROIDNDKROOT
fs_abspath $ANDROIDSDKROOT ANDROIDSDKROOT
fs_abspath $BUILDROOT BUILDROOT

AWKSCRIPTS=$ANDROIDNDKROOT/build/awk
MANIFEST=$BUILDROOT/thymiovpl/AndroidManifest.xml
ADB=$ANDROIDSDKROOT/platform-tools/adb

adb_var_shell()
{
  local CMDOUT RET OUTPUT VARNAME REDIRECTSTDERR
  REDIRECTSTDERR=$1
  VARNAME=$2
  shift; shift;

  CMDOUT=`mktemp /tmp/ndk-gdb-cmdout-XXXXXX`

  if [ "$REDIRECTSTDERR" != 0 ]; then
    $ADB shell "$@" ";" echo \$? | sed -e 's![[:cntrl:]]!!g' > $CMDOUT 2>&1
  else
    $ADB shell "$@" ";" echo \$? | sed -e 's![[:cntrl:]]!!g' > $CMDOUT
  fi

  RET=`sed -e '$!d' $CMDOUT`
  OUT=`sed -e '$d' $CMDOUT`
  rm -f $CMDOUT

  eval $VARNAME=\"\$OUT\"
  return $RET
}

PACKAGENAME=`awk -f $AWKSCRIPTS/extract-package-name.awk $MANIFEST`
LAUNCHABLENAME=`awk -f $AWKSCRIPTS/extract-launchable.awk $MANIFEST | sed 2q`
DASHELOUTPATH=`grep "LIBRARY_OUTPUT_PATH:" $BUILDROOT/dashel/CMakeCache.txt | \
  sed s/"LIBRARY_OUTPUT_PATH:PATH=\(.*\)"/"\1"/`
ASEBAOUTPATH=`grep "LIBRARY_OUTPUT_PATH:" $BUILDROOT/aseba/CMakeCache.txt | \
  sed s/"LIBRARY_OUTPUT_PATH:PATH=\(.*\)"/"\1"/`
QTLIBRARYDIR=`grep "QT_LIBRARY_DIR:" $BUILDROOT/aseba/CMakeCache.txt | \
  sed s/"QT_LIBRARY_DIR:PATH=\(.*\)"/"\1"/`
DEBUGOUTPATH=$BUILDROOT/thymiovpl/debug

if `awk -f $AWKSCRIPTS/extract-debuggable.awk $MANIFEST` != "true"; then
  message_exit "Package is not debuggable"
fi

adb_var_shell 1 DEVICEGDBSERVER ls /data/data/${PACKAGENAME}/lib/gdbserver
if [ $? != 0 ]; then
  message_exit "Application installed on device is not debuggable"
fi

adb_var_shell 1 DATADIR run-as $PACKAGENAME /system/bin/sh -c pwd
if [ $? != 0 -o -z "$DATADIR" ]; then
  message_exit "Could not determine the application's data directory"
fi

$ADB shell am start -n $PACKAGENAME/$LAUNCHABLENAME
if [ $? != 0 ]; then
  message_exit "Could not launch the application"
fi
$ADB shell sleep 2

PID=`$ADB shell ps | awk -f $AWKSCRIPTS/extract-pid.awk -v PACKAGE="$PACKAGENAME"`
if [ $? != 0 -o "$PID" = "0" ]; then
  message_exit "Could not determine the application's PID"
fi

GDBPID=`$ADB shell ps | awk -f $AWKSCRIPTS/extract-pid.awk -v \
  PACKAGE="lib/gdbserver"`
if [ "$GDBPID" != "0" ]; then
  message_warn "Debugging session already active on device"
  $ADB shell run-as $PACKAGENAME kill -9 $GDBPID
fi

DEBUGSOCKET=debug-socket
$ADB shell run-as $PACKAGENAME lib/gdbserver +$DEBUGSOCKET --attach $PID &
if [ $? != 0 ]; then
  message_exit "Failed to launch the debugging server on the device"
fi

$ADB forward tcp:$DEBUGPORT localfilesystem:$DATADIR/$DEBUGSOCKET
if [ $? != 0 ]; then
  message_exit "Failed to setup network redirection to debugging server"
fi

$ADB pull /system/bin/app_process $DEBUGOUTPATH/app_process
$ADB pull /system/bin/linker $DEBUGOUTPATH/linker
$ADB pull /system/lib/libc.so $DEBUGOUTPATH/libc.so
$ADB pull /system/lib/libstdc++.so $DEBUGOUTPATH/libstdc++.so

GDBSETUP=$DEBUGOUTPATH/gdb.setup
echo -n > $GDBSETUP
echo -n "set solib-search-path $DEBUGOUTPATH:$DASHELOUTPATH:" >> $GDBSETUP
echo ":$ASEBAOUTPATH:$QTLIBRARYDIR" >> $GDBSETUP
echo "file $DEBUGOUTPATH/app_process" >> $GDBSETUP
echo "target remote :$DEBUGPORT" >> $GDBSETUP

GDBCLIENT=$DEBUGOUTPATH/gdbclient
$GDBCLIENT -x $GDBSETUP

log_clean
