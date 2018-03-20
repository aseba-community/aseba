Building Aseba
==============

Requierements & Dependencies
----------------------------

Aseba requieres a C++14 compatible compiler. We recommand ``GCC 6``,
``Clang 5`` or ``MSVC 19`` (Visual Studio 17).

Aseba depends on Qt5.9 or greater. You will also need ``cmake`` 3.1 or
greater, we recommend you use the latest version available.

Getting the source code
-----------------------

The `source code of Aseba <https://github.com/aseba-community/aseba>`_
is hosted on github.
To fetch the source, you must first `install Git <https://git-scm.com/book/en/v2/Getting-Started-Installing-Git>`_
.

Then, to clone aseba, execute:

::

    git clone --recursive https://github.com/aseba-community/aseba.git
    cd aseba


All the commands given in the rest of this document assume the current path is the root folder of the cloned repository.


Getting Started on Windows with MSVC
------------------------------------

Aseba Builds on Windows Vista or greater.

-  Install ``Bonjour``. You will find the installer in
   ``third_party/bonjour/bonjoursdksetup.exe``
-  Install Qt from Qt's website
   https://download.qt.io/official\_releases/qt/. Once installed, th
   ``bin`` repertory of Qt must be in your path.
-  Install Visual Studio
-  Install ``CMake``

In a developer prompt run:

::

    mkdir build;
    cd build
    cmake -DBUILD\_SHARED\_LIBS=OFF -DCMAKE\_BUILD\_TYPE=Release -DCMAKE\_PREFIX\_PATH=";" ..
    nmake

Additional Dependencies
~~~~~~~~~~~~~~~~~~~~~~~

Aseba can be improved with several other dependencies \* SDL (Joystick
Support) \* QWT (Charts) \* libxml2

Getting Started on Windows with MingW
-------------------------------------

An easy way to compile Aseba on Windows is to use
`msys2 <http://www.msys2.org>`__.

Preliminary: download, install and update msys2
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Download and install msys2 from
`www.msys2.org <http://www.msys2.org>`__. If you have a 64-bit version
of Windows, take the ``x86_64`` installer, otherwise the ``i686`` one.

Once msys2 is installed, start the shell by running the ``MSYS2 MSYS``
application. In the shell, update msys2 by typing:

::

    pacman -Syu

Then restart the shell and update the packages by typing:

::

    pacman -Su

Install the dependencies
~~~~~~~~~~~~~~~~~~~~~~~~

In the msys2 shell, install the dependencies by typing:

::

    pacman -S mingw-w64-i686-{toolchain,cmake,qt5,qwt-qt5,libxml2,SDL2} git make

If you want to build Windows 64 binaries, replace ``i686`` by ``x86_64``

Building Aseba
~~~~~~~~~~~~~~

In a msys2 prompt, run

::

    mkdir build && cd build
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<path of qt>;<path of bonjour>" ..
    make

Getting Started on OSX
----------------------

You will need OSX 10.11 or greater

-  Install `Homebrew <https://brew.sh/>`__.
-  In the cloned repository run

::

   brew update brew tap homebrew/bundle brew bundle

Then you can create a build directory and build Aseba

::

    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ..
    make

Getting Started on Linux
------------------------

Dependencies On Ubuntu & Debian
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    sudo apt-get install qttools5-dev-tools \
                         qtbase5-dev \
                         qt5-qmake \
                         libqt5opengl5-dev \
                         libqt5svg5-dev \
                         libqt5x11extras5-dev \
                         libqwt-qt5-dev \
                         libudev-dev \
                         libxml2-dev \
                         libsdl2-dev \
                         libavahi-compat-libdnssd-dev \
                         cmake \
                         g++ \
                         git \
                         make \

Building Aseba
~~~~~~~~~~~~~~

::

    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ..
    make

A note about permissions
~~~~~~~~~~~~~~~~~~~~~~~~

If you will be connecting to your robot through a serial port, you might
need to add yourself to the group that has permission for that port. In
many distributions, this is the "dialout" group and you can add yourself
to that group and use the associated permissions by running the
following commands:

::

    sudo usermod -a -G dialout $USER
    newgrp dialout



Advanced Setup
--------------

Running tests
~~~~~~~~~~~~~

Once the build is complete, you can run ``ctest`` in the build directory
to run the tests.

Ninja
~~~~~

The compilation of Aseba can be significantly speed up using ``ninja``
insteadf of make. Refer to the documentation of ``cmake`` and ``ninja``.
