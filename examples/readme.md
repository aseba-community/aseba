========
Examples
========

This directory contains several examples to help users build their own program
using Aseba, enabling them to communicate with Aseba nodes using customized
interfaces.

Here is a short summary of the content:

- cpp-qt-gui
    This is a very basic GUI done in C++ / Qt to exchange messages with Aseba nodes.
    This program can be compiled either by using cmake, either by using qmake with the
    provided .pro file. The latest is useful when you create an out-of-tree program. In
    this case, you will need to comment the "#include <xxx.moc>" from the .cpp files.
    The .pro file was tested under windows and unix.

- python-dbus: a Python script showing how to interface with Aseba (medulla) using D-Bus

