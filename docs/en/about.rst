About Aseba
===========

Aseba is a set of tools which allow novices to program robots easily and efficiently.
For these reasons, Aseba is well-suited for robotic education and research.
Aseba is an open-source software created and maintained by
Dr. Stéphane Magnenat with contributions from the community.

More Specifically, Aseba is an event-based architecture for real-time
distributed control of mobile robots.It targets integrated
multi-processor robots or groups of single-processor units, real or
simulated.

The core of Aseba is a lightweight virtual machine tiny
enough to run even on microcontrollers. With Aseba, we program robots in
a user-friendly programming language using a cozy integrated development
environment.

Node
----

In Aseba, there can be several robots or a robot with multiple
`processors <http://en.wikipedia.org/wiki/Central_processing_unit>`__
running in the same network. This network can be software
(`TCP <http://en.wikipedia.org/wiki/Transmission_Control_Protocol>`__),
hardware
(`CAN <http://en.wikipedia.org/wiki/Controller_area_network>`__), or a
mix of both. Each processor within a network runs a small `virtual
machine <http://en.wikipedia.org/wiki/Virtual_machine>`__ and is called
a *node*.

Each node has its own tab in Aseba Studio, allowing you to program them independently,
but with possible interactions through events.

Event
-----

Aseba is an `event-based
architecture <http://en.wikipedia.org/wiki/Event-driven_programming>`__,
which means that events trigger code execution asynchronously. Events
have an identifier and optional payload data.

Aseba nodes can exchange events, and these can be of two types.

-  The events that Aseba nodes exchange within an Aseba network are
   called **global events**.
-  The events that are internal to a node are called **local events**.
   An example of a local event is the one emitted by a sensor that
   provides updated data.

If the code for receiving an event is defined, then the corresponding
block of code is executed when the event is received. Code can also
`emit events <https://aseba.wikidot.com/en:asebalanguage#toc2>`__, which
can trigger the execution of code on another node or enable
communication with an external program.

To start the execution of related code upon receiving new events,
programs must not block and thus must not contain any infinite loops.

For instance, in the context of robotics, where a traditional robot
control program would do some processing inside an infinite loop,
an Aseba program would just do the processing inside a sensor-related event.
