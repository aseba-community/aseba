# End-to-end tests for `asebahttp`

**Asebahttp** is a switch that connects together a set of Aseba
targets, and provides a REST interface for communicating with them
using HTTP over a TCP/IP connection. The tests in this directory
verify these interconnections.

There are four test specifications:
* `1-http_spec.js` verifies that through the HTTP REST interface one
can inspect and command Aseba nodes,
* `2-interaction_spec.js` verifies that Aseba nodes can communicate
over the Aseba bus implemented by **asebahttp**, 
* `3-remap_spec.js` verifies that **asebahttp** will remap node
identifiers to avoid conflicts. 
* `4-load-aesl_spec.js` verifies that **asebahttp** can upload a
new aesl program to a node.

## Installation

CMake should have installed the necessary NPM modules and created a
test runner `run-e2e-http.sh`. The command

`npm install`

will reinstall the necessary NPM modules.

## Test scenario

The test runner will start nine nodes with **asebadummynode**. The
first four, **dummynode-**0—3, are connected using **asebaswitch**,
which is itself connected to an **asebahttp** listening to port 3000.
The next five, **dummynode-**4—8, are individually connected to
an **asebahttp** listening to port 3001.

Each set is sent the file `ping0123.aesl`, which contains four Aseba programs:
* Program 1 listens for `wakeup` events addressed to 1000, and emits a
`ping` event containing a timestamp from 0—999, as well as the last
broadcast message that it observed on the bus. 
* Program 2 is as above, but listens for `wakeup` events addressed to
2000. 
* Program 3 is as above, for address 3000, but is broken: it never
increments its timestamp counter. 
* Program 4 is a global clock, that emits a `wakeup` event to a random
address (1000,2000,3000) every second. 

The nodes **dummynode-**0—3 have each a different “asebaId” and play
one of the roles described above. 
The nodes **dummynode-**4—8 also test identifier remapping: 
1. **dummynode-4** (node 5) asks to be known as nodeId 1, and so gains
the listener 1 role 
2. **dummynode-5** (node 6) keeps its nodeId but asks for aeslId 1 and
so gains the listener 1 role, too 
3. **dummynode-6** (node 7) asks for aeslId 4 and thus the clock role.
4. **dummynode-7** (node 8) asks to be known as nodeId 6, which is a
conflict with **dummynode-5** and forces it to be renumbered, but it
also asks for aeslId 2 and so gains the listener 2 role. 

Node **dummynode-8** is used to test loading aesl programs using the
HTTP PUT method of the REST interface.

## Implementation

The tests are defined in a behavior-driven style (BDD) using
the *Frisby.js* module and *Jasmine* expectations. A test is
constructed using Frisby methods then executed using *toss*. 
Test specifications run concurrently.
Dependencies between tests are defined through nesting:
a *frisby.create* inside of an *after* will be executed sequentially
(and can refer to variables that were assigned values from the result
of the outer *frisby.create* request). 
