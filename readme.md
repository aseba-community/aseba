=====
Aseba
=====

Aseba is a set of tools which allow beginners to program robots easily and efficiently.
For more information, see: http://aseba.wikidot.com

	Copyright (C) 2007--2016:
	Stephane Magnenat <stephane at magnenat dot net>
	(http://stephane.magnenat.net)
	and other contributors, see authors.txt for details


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
	cd build-aseba
	export dashel_DIR=../build-dashel
	cmake ../aseba -DCMAKE_BUILD_TYPE=RelWithDebInfo -Ddashel_DIR=../build-dashel -DDASHEL_INCLUDE_DIR=../dashel -DDASHEL_LIBRARY=../build-dashel/libdashel.a -DENKI_INCLUDE_DIR=../enki -DENKI_LIBRARY=../build-enki/enki/libenki.a -DENKI_VIEWER_LIBRARY=../build-enki/viewer/libenkiviewer.a
	make

Once this script has run, you can find the executables in `build-aseba/`, in their respective sub-directories. 

If you have some problem to compile Aseba after reading the relevant
documentation, feel free to post your question on our development mailing
list. You can subscribe to the latter at http://gna.org/mail/?group=aseba

Enjoy Aseba!

The developers
