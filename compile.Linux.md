# Compile Aseba on Linux

Aseba depends on [several open-source libraries](compile.deps.md).
On Linux, most are available as packages in your distribution.
For instance, on Debian derivatives such as Ubuntu, you can install them, along cmake and the compiler, with:

	sudo apt-get install qttools5-dev-tools qtbase5-dev libqt5opengl5-dev libqt5svg5-dev libqt5webkit5-dev libqt5x11extras5-dev libqwt-qt5-dev libudev-dev libxml2-dev libsdl2-dev libavahi-compat-libdnssd-dev cmake g++ git make

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
You can install them system-wide by running:

    sudo make install
    
Or launch them directly from their built location; for example, you can then launch studio by typing:

    clients/studio/asebastudio

## A note about permissions

If you will be connecting to your robot through a serial port, you might need to add yourself to the group that has permission for that port.
In many distributions, this is the "dialout" group and you can add yourself to that group and use the associated permissions by running the following commands:

    sudo usermod -a -G dialout $USER
    newgrp dialout

## Compiling a different branch

This will compile the master version of Aseba.
If you want to compile a specific branch instead, for instance `release-1.5.x`, just uncomment the line `cd aseba && git checkout release-1.5.x && cd ..`.
Change `release-1.5.x` with the branch or tag you want to compile.

## Building packages

### Debian-based

On Debian-based distributions (Debian, Ubuntu, etc.), you can build *deb* packages.
First, install the necessary build scripts:

    sudo apt-get install build-essential devscripts equivs
    
Then, install the build dependencies for Dashel and Enki, build them as packages and install them, install the additional build dependencies for Aseba, and build the Aseba package.

    # build Dashel package and install it
    cd dashel
    sudo mk-build-deps -i         # install dependencies
    debuild -i -us -uc -b         # build package
    cd ..
    sudo dpkg -i libdashel*.deb   # install package
    # build Enki package and install it
    cd enki
    sudo mk-build-deps -i         # install dependencies
    debuild -i -us -uc -b         # build package
    cd ..
    sudo dpkg -i libenki*.deb     # install package
    # build Aseba package
    cd aseba
    sudo mk-build-deps -i         # install dependencies
    debuild -i -us -uc -b         # build package
    cd ..
