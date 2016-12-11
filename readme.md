Aseba
=====

Aseba is a set of tools which allow beginners to program robots easily and efficiently.
For more information, see: http://aseba.wikidot.com

	Copyright (C) 2007--2016:
	Stephane Magnenat <stephane at magnenat dot net>
	(http://stephane.magnenat.net)
	and other contributors, see authors.txt for details

Compilation from sources
------------------------

### Windows

To build Aseba you need to install those dependencies

- CMake, version at lease 2.8 (http://www.cmake.org/cmake/resources/software.html)
- MinGW (https://sourceforge.net/projects/mingw/files/)
- Qt4 SDK (http://download.qt-project.org/archive/qt/4.8/4.8.6/qt-opensource-windows-x86-mingw482-4.8.6-1.exe)
- Git for windows (https://git-for-windows.github.io)

Make sure that all installed libraries are in your's [PATH](http://www.computerhope.com/issues/ch000549.htm).

First, open Git Bash programm and copy-pasete those commands

    git clone --recursive https://github.com/aseba-community/aseba.git
    cd aseba
    mkdir build
    cd build
    cmake -G "MinGW Makefiles" ..
    make
    
All files are now in %HOMEPATH%/aseba/build directory (you can copy-paste this path into file browsers location bar)
    
Some information are from [Thymio site](https://www.thymio.org/en:sourceinstall)

### Linux

To compile Aseba, you need CMake 2.6 or later (http://www.cmake.org/).
The CMake website provides documentation on how to use CMake on the different
platforms: http://www.cmake.org/cmake/help/runningcmake.html

Aseba depends on the following libraries:
- Dashel (http://home.gna.org/dashel/)
- Enki (http://home.gna.org/enki/, optional, for simulators)
- Qt4 (http://qt-project.org/, for IDE)
- Boost (http://www.boost.org/)
- Qwt (http://qwt.sourceforge.net/, optional, for graphs)
- libxml2 (http://www.xmlsoft.org/, optional, for switches)
- libudev (http://www.freedesktop.org/software/systemd/man/libudev.html, optional, for serial port enumeration on Linux)

On Linux, the Qt4, Qwt, libxml2 and udev are packages available in your distribution. For instance, on Ubuntu, you can install them, along cmake and the compiler, with:

	sudo apt-get install libboost-dev libqt4-dev qt4-dev-tools libqwt5-qt4-dev libudev-dev libxml2-dev cmake g++ git make

Then you have to fetch Dashel, Enki, compile them, and then you can fetch Aseba and tell it where it can find Dashel and Enki. Then you can compile Aseba. The following script does this for you:

    git clone --recursive https://github.com/aseba-community/aseba.git
    cd aseba
    mkdir build
    cd build
    cmake ..
    make

Once this script has run, you can find the executables in `build-aseba/`, in their respective sub-directories. 

### OS X

You need to have Xcode installed and agreed with license. To do so, you have to do

    xcodebuild -license
    
Then, install some dependencies using Homebrew. First, you'll need Homebrew

    /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    
After that, install required dependencies (this list isn't necessarily complete)
    
    brew install qt
    
And now, you can clone this repository and compile

    git clone --recursive https://github.com/aseba-community/aseba.git
    cd aseba
    mkdir build
    cd build
    cmake ..
    make

If you have some problem to compile Aseba after reading the relevant
documentation, feel free to post your question on our development mailing
list. You can subscribe to the latter at http://gna.org/mail/?group=aseba

Enjoy Aseba!

The developers
