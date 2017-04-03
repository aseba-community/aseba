# Understanding the Aseba source tree

This file describes the source tree of Aseba.

## Conventions

Aseba is written in C++11, with the exception of embedded code such as the VM which is C99.
While the existing code is not fully migrated to C++11 features yet, all new code should make best use of modern C++11 features.

## Content of Aseba sources

These directories contain source code:
* `common`: definitions common to both targets, studio and tools
  * `msg`: message serialization/de-serialization in C++
  * `utils`: some miscellaneous utility functions
  * `zeroconf`: Aseba target discovery through zeroconf/Bonjour
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
  * `joy`: joystick client using SDL2
  * `thymioupdater`: firmware updater for the Thymio II robot
  * `thymiownetconfig`: GUI to configure the network settings of Wireless Thymio
  * `massloader`: very quickly load the same code to several robots
* `tests`: unit tests
  * `compiler`: for compiler
  * `simulator`: for the integration with the Enki simulator
  * `vm`: for the virtual machine
* `examples`
  * `clients`: examples how to build your own clients
    * `cpp-shell`: command-line using the C++ library
    * `cpp-qt-gui`: GUI using the C++ library and Qt
    * `python-dbus`: library to access Aseba from Python using D-Bus
  * `zeroconf`: examples on how to use zeroconf to advertise or discover targets
  * `switches/botspeak`: an example of switch that compiles a sequencial BASIC-like script into AESL

These directories contain support files:
* `CMakeModules`: detection support for DNSSD and SDL2 and version parsing module
* `android`: files to build Aseda for Android
* `debian`: files to build a DEB package
* `rpm`: files to build a RMP package
* `docsource`: some source files to generate documentation
* `maintainer`: scripts to maintain help files & translations
* `menu`: entries for operating system menus
