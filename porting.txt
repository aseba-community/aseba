Porting ASEBA

Porting ASEBA can mean two different things: 1. porting the development and support tools to a new desktop platform; 2. porting the virtual machine to a new microcontroller.

1. Porting the development and support tools

Porting the development tools only requires a compilation; providing that cmake, Qt and Dashel support your platform, which is the case for all major desktop platforms. Some tools, such as Medulla, depend on D-Bus and thus will only run under Unix. That should not bother you, as these tools usually will run in a Linux-powered robot or device.

2. Porting the virtual machine to a new microcontroller

The virtual machine itself is straightforward to port, as it is plain C. The interfaces to the external world require slightly more work.

First, the interface to the ASEBA network will depend on the type of bus you use. Currently, ASEBA supports the CAN bus and direct links (usually serial). The "transport/" directory contains adapters, that one one end provide functions to the virtual machine for communication and on the other end provide on interface to your communication bus. A CAN adapter is available in "transport/can", and a generic adapter for stream-based communication link is available in "transport/buffer".

Second, the interfaces to your sensors, actuators, and execution-flow sources (typically interrupts) depend on the feature-set of your microcontroller and on your the application. The directory "targets/" contains several examples. The directories "targets/challenge" and "targets/playground" provide examples of virtual machine controlling virtual e-puck robots inside the Enki simulator. The directory "targets/e-puck" contains the implementation of ASEBA on the real e-puck robot. All these examples use the buffer transport interface. In opposite, the directory "targets/dspic33" contains a generic structure for running ASEBA inside a dspic33 using CAN as a communication layer.

The directory "targets/can-translator/" contains the code and the schematics of a CAN to UART translator using a dspic33. This code uses Molole [1] to access the dspic's peripherals.

In summary, to port ASEBA to a new microcontroller, you first have to select the type of communication you are interested in, and then choose an example close to your need, and start hacking from there. The development mailing list are available, should you have any question.

Enjoy ASEBA !

[1]     http://mobots.epfl.ch/molole.html