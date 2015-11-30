========
Examples
========

This directory contains several examples to help users build their own program
using Aseba, enabling them to communicate with Aseba nodes using customized
interfaces.

Here is a short summary of the content:

- cpp-qt-dbus

    This is a C++/QT project showing how to interface with Aseba (medulla) using
    D-Bus (QDBus). The class dbusinterface contains methods that establish the D-Bus
    interface with an Aseba Network. An interface with Thymio II is shown in the 
    main.cpp as an example, in which an Aseba script (ScriptDBusThymio.aesl) is
    loaded on Thymio II and the user can then manage the Aseba variables/events. 
    The project is compatible with QT4/QT5 and uses some C++11 functionalities.


- cpp-shell

    This is a basic command-line shell done in pure C++, using Dashel and libxml2.

- cpp-qt-gui

    This is a very basic GUI done in C++ / Qt to exchange messages with Aseba nodes.
    This program can be compiled either by using cmake, either by using qmake with the
    provided .pro file. The latest is useful when you create an out-of-tree program. In
    this case, you will need to comment the "#include <xxx.moc>" from the .cpp files.
    The .pro file was tested under windows and unix.

- python-dbus

    This is a Python script showing how to interface with Aseba (medulla) using D-Bus.

