# Aseba

	Copyright (C) 2007--2017:
	Stephane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	and other contributors, see authors.txt for details
	Released under LGPL3, see license.txt for details
	
Aseba is a set of tools which allow beginners to program robots easily and efficiently.
For more information, see: http://aseba.wikidot.com

## Quick building instructions

To compile Aseba, you need CMake 2.8.10 or later (http://www.cmake.org/).
The CMake website provides documentation on how to use CMake on the different
platforms: http://www.cmake.org/cmake/help/runningcmake.html

Aseba depends on the following libraries:
- Dashel (http://home.gna.org/dashel/)
- Enki (http://home.gna.org/enki/, optional, for simulators)
- Qt4 (http://qt-project.org/, optional, for simulators and IDE)
- Qwt (http://qwt.sourceforge.net/, optional, for graphs in IDE)
- libxml2 (http://www.xmlsoft.org/, optional, for switches)
- libSDL2 (https://www.libsdl.org/, optional, for joystick support)
- libudev (http://www.freedesktop.org/software/systemd/man/libudev.html, optional, for serial port enumeration on Linux)

### Linux

On Linux, all but the first two are packages available in your distribution.
For instance, on Debian derivatives such as Ubuntu, you can install them, along cmake and the compiler, with:

	sudo apt-get install libqt4-dev qt4-dev-tools libqwt5-qt4-dev libudev-dev libxml2-dev cmake g++ git make

_On other Linux distributions, please see what are the corresponding packages and install them. For instance in Fedora `libudev-dev` is replaced by `systemd-devel`_. 

Dashel and Enki are developed by us, so you need to fetch and compile them.
Then, you can fetch Aseba, tell it where it can find Dashel and Enki, and compile it.
The following script does this for you:

	# create build tree
	mkdir -p aseba/build-dashel aseba/build-enki aseba/build-aseba
	cd aseba
	# fetch and compile dashel
	git clone https://github.com/aseba-community/dashel.git
	cd build-dashel
	cmake ../dashel -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF
	make
	cd ..
	# fetch and compile enki
	git clone https://github.com/enki-community/enki.git
	cd build-enki
	cmake ../enki -DCMAKE_BUILD_TYPE=RelWithDebInfo
	make
	cd ..
	# fetch and compile aseba, telling it where to find dashel and enki
	git clone --recursive https://github.com/aseba-community/aseba.git
	# cd aseba && git checkout release-1.5.x && cd ..
	cd build-aseba
	cmake ../aseba -DCMAKE_BUILD_TYPE=RelWithDebInfo -Ddashel_DIR=../build-dashel -Denki_DIR=../build-enki
	make
	
Once this script has run, you can find the executables in `build-aseba/`, in their respective sub-directories. 
For example, you can then launch studio by typing:

    clients/studio/asebastudio

#### A note about permissions

If you will be connecting to your robot through a serial port, you might need to add yourself to the group that has permission for that port.
In many distributions, this is the "dialout" group and you can add yourself to that group and use the associated permissions by running the following commands:

    sudo usermod -a -G dialout $USER
    newgrp dialout

#### Compiling a different branch

This will compile the master version of Aseba.
If you want to compile a specific branch instead, for instance `release-1.5.x`, just uncomment the line `cd aseba && git checkout release-1.5.x && cd ..`.
Change `release-1.5.x` with the branch or tag you want to compile.

### macOS


## Understanding source tree

Directories containing source code:
* `common`: definitions common to both targets, studio and tools
  * `msg`: message serialization/de-serialization in C++
  * `utils`: some miscellaneous utility functions
* `compiler`: compiler
* `vm`: virtual machine and the standard library of native functions
* `targets`: some targets running virtual machines:
  * `dummy`: simple PC target just running a vm
  * `challenge`: competitive robot simulator using Enki
  * `playground`: generic robot simulator using Enki
  * `enki-marxbot`: simplified simulation of the marXbot robot
  * `dspic33`: common code for running aseba on dspic33
  * `can-translator`: translator UART<->CAN on dsPIC
  * `mobots-mx31`: script for Mobots robots using iMX31 board
* `transport`: different transport layers:
  * `buffer`: in-memory buffers
  * `dashel-plugins`: extensions to Dashel: socketcan on Linux and Android serial port
  * `can`: CAN bus
  * `microchip_usb`: serial-over-USB implementation for PIC24 microcontrollers
* `switches`: software switches to create networks:
  * `switch`: connect several aseba nodes and tools together
  * `medulla`: like switch, but provides a DBus interface as well
* `clients`: clients:
  * `cmd`: command line tool for sending messages
  * `dump`: tool to dump all incoming messages
  * `eventlogger`: tool to log all incoming messages
  * `exec`: tool to execute an external command upon the reception of an event
  * `replay`: can record and replay messages
  * `studio`: integrated development environment
  * `thymioupdater`: firmware updater for the Thymio II robot
  * `massloader`: very quickly load the same code to several robots
* `tests`: unit tests
* `examples/clients`: examples how to build your own clients
  * `cpp-shell`: command-line using the C++ library
  * `cpp-qt-gui`: GUI using the C++ library and Qt
  * `python-dbus`: library to access Aseba from Python using D-Bus

Other directories:
* `android`: files to build Android package
* `debian`: files to build debian package
* `docsource`: source of documentation
* `maintainer`: scripts to maintain help files & translations
* `menu`: entries for operating-system menus

## Getting in touch

If you have some problem to compile Aseba after reading the relevant
documentation, feel free to post your question on our development mailing
list. You can subscribe to it at http://gna.org/mail/?group=aseba

Enjoy Aseba!

The developers
