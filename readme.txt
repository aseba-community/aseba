Aseba - an event-based framework for distributed robot control
Copyright (C) 2007--2009:
	Stephane Magnenat <stephane at magnenat dot net>
	(http://stephane.magnenat.net)
	and other contributors, see authors.txt for details
	Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne

To compile Aseba, you need CMake 2.6 or later (http://www.cmake.org/).
The CMake website provides documentation on how to use CMake on the different
platforms:
	http://www.cmake.org/cmake/help/runningcmake.html

Aseba depends on the following libraries:
	- Dashel (http://home.gna.org/dashel/)
	- Boost
	- Qt4 (http://www.qtsoftware.com/products/, for IDE)
	- Qwt (http://qwt.sourceforge.net/, optional, for graphs)
	- Enki (http://home.gna.org/enki/, optional, for simulators)

If you are on Unix and lazy, and just want to compile Aseba without any
additional thinking, try the following commands in Aseba sources directory:
	cmake . && make
If this does not work, probably because you have to specify the path of some libraries,
read the CMake documentation page.

If you still have some problem to compile Aseba after reading the relevant
documentation, feel free to post your question on our development mailing
list. You can subscribe to the latter at:
	http://gna.org/mail/?group=aseba

Enjoy Aseba!

	The developers