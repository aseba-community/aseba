# Aseba

	Copyright (C) 2007--2017:
	Stephane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	and other contributors, see authors.txt for details
	Released under LGPL3, see license.txt for details
	
Aseba is a set of tools which allow beginners to program robots easily and efficiently.
For more information, see: http://aseba.wikidot.com

## Supproted platforms

The following platforms are supported:
- Linux ([compilation instructions](compile.Linux.md))
- macOS ([compilation instructions](compile.macOS.md))
- Windows (compilation instructions coming soon)
- embedded systems ([see the dependency list](compile.deps.md))

## Enjoy Aseba

TODO: write this part

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
