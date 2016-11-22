# End-to-end tests for `asebascratch`

**Asebascratch** is a switch based on **asebaswitch** that connects
together a set of Aseba targets, and provides a REST interface for
communicating with them using HTTP over a TCP/IP connection. The tests
in this directory verify these interconnections.

There are four test specifications:
* `1-http_spec.js` verifies that through the HTTP REST interface one
can inspect and command Aseba nodes,
* `2-interaction_spec.js` verifies that Aseba nodes can communicate
over the Aseba bus implemented by **asebascratch**, 
* `3-remap_spec.js` verifies that **asebascratch** will remap node
identifiers to avoid conflicts. 
* `4-load-aesl_spec.js` verifies that **asebascratch** can upload a
new aesl program to a node.

## Installation

CMake should have installed the necessary NPM modules and created a
test runner `run-e2e-scratch.sh`. The command

`npm install`

will reinstall the necessary NPM modules.

## Test scenarios

The first test scenario is the same as for **asebahttp**. See the
documentation in `../../../../tests/e2e-http/`.  The test runner will
start nine nodes with **asebadummynode**. The first four,
**dummynode-**0—3, are connected using **asebaswitch**, which is
itself connected to an **asebascratch** listening to port 3000.  The next
five, **dummynode-**4—8, are individually connected to an
**asebascratch** listening to port 3001.

The second test scenario needs a Thymio-II robot connected through a
serial port, either by USB cable or by the Thymio RF dongle.

## Implementation

The tests are defined in a behavior-driven style (BDD) using
the *Frisby.js* module and *Jasmine* expectations. See the
documentation in `../../../../tests/e2e-http/` for details.
