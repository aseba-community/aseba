                Android build system for ASEBA

Requirements (using Necessitas bundled SDK and NDK):

  Download the Necessitas Qt Source Development Kit installer from
  http://community.kde.org/Necessitas . Run the installer to deploy
  Necessitas in a location of choice, e.g., /opt/necessitas.
  Choose to install at least API level 12.

  Install ubash. On Ubuntu, use the ASL launchpad repository ppa:kralf/asl:
  > (sudo) add-apt-repository ppa:kralf/asl
  > (sudo) apt-get update
  > (sudo) apt-get install ubash
  On other platform, use the source from:
  https://launchpad.net/~kralf/+archive/asl/+files/ubash_0.1-56%7Elucid.tar.gz
  
  Install the ant build system, on Ubuntu:
  > (sudo) apt-get install ant
  
Requirements (using separated SDK and NDK)

  In addition to the previous step, install these:

  Download the official Android Native Development Kit (NDK) from
  http://developer.android.com/tools/sdk/ndk . Untar the archive in
  a location of choice, e.g., /opt/android/ndk.

  Download the official Android Source Development Kit (SDK) from
  http://developer.android.com/sdk . Untar the archive in
  a location of choice, e.g., /opt/android/sdk.

Build instructions:

  Assuming the DaSHEL and ASEBA source trees to be located under the
  same root directory such as /path/to/dashel and /path/to/aseba,
  make.sh should do the job just fine. Simply launch the script as
  follows:

  > ./make.sh --ndk-root /path/to/android/ndk
      --sdk-root /path/to/android/sdk
      --necessitas-root /path/to/necessitas
      [--debug] [--install]

  Make sure to provide the correct paths under which the above build
  requirements can be found in your system. To perform a debug build,
  use the --debug option. To install the resulting application
  package to an Android device connected via USB, pass --install. For
  additional help concerning the command line parameters, issue

  > ./make.sh --help

  Note that all builds are performed out-of-source and that the
  generated binaries can be found under the specified build root,
  i.e., a directory named build unless overridden by the command
  line parameters.

  The convenience build script make.sh first runs mkdashel.sh to
  cross-compile DaSHEL for Android. Next, mkaseba.sh is invoked to
  generate the native library libthymiovpl.so from the ASEBA build
  configuration. Both these steps require the cross-compile toolchain
  defined in cmake/Android.cmake and run CMake to configure the build.
  On success, you will find libdashel.so and libthymiovpl.so below
  /path/to/build/root/thymiovpl/libs. The Android application
  package is then compiled by calling mkthymiovpl.sh which simply
  launches ant in the thymiovpl source directory.

Debug instructions:

  To prepare the application for debugging, make sure to append the
  --debug flag for all build scripts. If you wish to debug Java-based
  API code, use the command

  > /path/to/android/sdk/platform-tools/adb logcat

  whilst the Android device is connected via USB, and launch the
  application installed on the device. In case of a Java exception,
  the log will contain the usual backtrace output.

  If you wish to debug native code, issue the helper script

  > ./debug.sh --ndk-root /path/to/android/ndk
      --sdk-root /path/to/android/sdk
      --necessitas-root /path/to/necessitas

  Just as for the build scripts, make sure to provide the correct paths
  under which the build requirements can be found in your system.
  Otherwise, gdb may fail to resolve and display the symbols.
