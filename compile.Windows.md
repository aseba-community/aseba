# Compile Aseba on Windows

Aseba depends on [several open-source libraries](compile.deps.md).
An easy way to compile Aseba on Windows is to use [msys2](http://www.msys2.org).

## Preliminary: download, install and update msys2

Download and install msys2 from [www.msys2.org](http://www.msys2.org).
If you have a 64-bit version of Windows, take the `x86_64` installer, otherwise the `i686` one.

Once msys2 is installed, start the shell by running the `MSYS2 MSYS` application.
In the shell, update msys2 by typing:

	pacman -Syu
 
Then restart the shell and update the packages by typing:
 
	pacman -Su

## Install the dependencies

In the msys2 shell, install the dependencies by typing:

	pacman -S mingw-w64-i686-{toolchain,cmake,qt5,qwt-qt5,libxml2,SDL2} git make

If you want to build Windows 64 binaries, replace `i686` by `x86_64`

## Fetch and compile Aseba

Dashel and Enki are developed by us, so you need to fetch and compile them.
Then, you can fetch Aseba, tell it where it can find Dashel and Enki, and compile it.
The following script does this for you:

	# change mwing32 to mingw64 if you installed `x86_64` versions at prev. step
	export PATH=$PATH:/mingw32/bin
	# create build tree
	mkdir -p aseba/build-dashel aseba/build-enki aseba/build-aseba
	cd aseba
	# fetch and compile dashel
	git clone https://github.com/aseba-community/dashel.git
	cd build-dashel
	cmake ../dashel -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF
	make
	cd ..
	# fetch and compile enki
	git clone https://github.com/enki-community/enki.git
	cd build-enki
	cmake ../enki -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=RelWithDebInfo
	make
	cd ..
	# fetch and compile aseba, telling it where to find dashel and enki
	git clone --recursive https://github.com/aseba-community/aseba.git
	# cd aseba && git checkout release-1.5.x && cd ..
	cd build-aseba
	cmake ../aseba -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=RelWithDebInfo -Ddashel_DIR=../build-dashel -Denki_DIR=../build-enki
	make

Once this script has run, you can find the executables in `build-aseba/`, in their respective sub-directories.
You can launch them directly from their built location; for example, you can then launch studio by typing:

	clients/studio/asebastudio

## Compiling a different branch

This will compile the master version of Aseba.
If you want to compile a specific branch instead, for instance `release-1.5.x`, just uncomment the line `cd aseba && git checkout release-1.5.x && cd ..`.
Change `release-1.5.x` with the branch or tag you want to compile.
