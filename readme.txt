Aseba - an event-based framework for distributed robot control
	http://aseba.wikidot.com

Copyright (C) 2007--2011:
	Stephane Magnenat <stephane at magnenat dot net>
	(http://stephane.magnenat.net)
	and other contributors, see authors.txt for details


To compile Aseba, you need CMake 2.6 or later (http://www.cmake.org/).
The CMake website provides documentation on how to use CMake on the different
platforms:
	http://www.cmake.org/cmake/help/runningcmake.html

Aseba depends on the following libraries:
	- Dashel (http://home.gna.org/dashel/)
	- Boost (http://www.boost.org/)
	- Qt4 (http://www.qtsoftware.com/products/, for IDE)
	- Qwt (http://qwt.sourceforge.net/, optional, for graphs)
	- Enki (http://home.gna.org/enki/, optional, for simulators)

And optionally, to enumerate serial ports properly on linux:
	- libudev (http://www.kernel.org/pub/linux/utils/kernel/hotplug/libudev/)


On Linux, the first four libraries are packages available in your distribution. For instance, on Ubuntu, you can install them, along cmake and the compiler, with:

sudo apt-get install libboost-dev libqt4-dev libqwt5-qt4-dev libudev-dev cmake g++

Then you have to fetch Dashel, Enki, compile them, and then you can fetch Aseba and tell it where it can find Dashel and Enki. Then you can compile Aseba. The following script does this for you:

# create build tree
mkdir -p aseba/build-dashel aseba/build-enki aseba/build-aseba
cd aseba
# fetch and compile dashel
svn co svn://svn.gna.org/svn/dashel/trunk dashel
cd build-dashel
cmake ../dashel -DCMAKE_BUILD_TYPE=RelWithDebInfo
make
cd ..
# fetch and compile enki
svn co svn://svn.gna.org/svn/enki/trunk enki
cd build-enki
cmake ../enki -DCMAKE_BUILD_TYPE=RelWithDebInfo
make
cd ..
# fetch and compile aseba, telling it where to find dashel and enki
svn co svn://svn.gna.org/svn/aseba/trunk aseba
cd build-aseba
cmake ../aseba -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDASHEL_INCLUDE_DIR=../dashel -DDASHEL_LIBRARY=../build-dashel/libdashel.a -DENKI_INCLUDE_DIR=../enki -DENKI_LIBRARY=../build-enki/enki/libenki.a -DENKI_VIEWER_LIBRARY=../build-enki/viewer/libenkiviewer.a
make

Once this script has run, you can find the executables in build-aseba/, in their respective sub-directories. 

If you have some problem to compile Aseba after reading the relevant
documentation, feel free to post your question on our development mailing
list. You can subscribe to the latter at:
	http://gna.org/mail/?group=aseba

Enjoy Aseba!

	The developers
