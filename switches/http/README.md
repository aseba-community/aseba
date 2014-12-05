asebahttp - a switch to bridge Aseba to HTTP

2015-01-01 David James Sherman <david dot sherman at inria dot fr>

Provide a simple REST interface with introspection for Aseba devices.

- GET  /nodes                                 - JSON list of all known nodes
- GET  /nodes/:NODENAME                       - JSON attributes for :NODENAME
- PUT  /nodes/:NODENAME                       - write new Aesl program (file= in multipart/form-data)
- GET  /nodes/:NODENAME/:VARIABLE             - retrieve JSON value for :VARIABLE
- POST /nodes/:NODENAME/:VARIABLE             - send new values(s) for :VARIABLE
- POST /nodes/:NODENAME/:EVENT                - call an event :EVENT
- GET  /events\[/:EVENT\]*                      - create SSE stream for all known nodes
- GET  /nodes/:NODENAME/events\[/:EVENT\]*      - create SSE stream for :NODENAME

Typical use: `asebahttp --port 3000 --aesl vmcode.aesl ser:name=Thymio-II &`
After vmcode.aesl is compiled and uploaded, check with `curl http://127.0.0.1:3000/nodes/thymio-II`
                
Substitute appropriate values for :NODENAME, :VARIABLE, :EVENT and their parameters. In most cases,
:NODENAME is `thymio-II`) For example.
- `curl http://localhost:3000/nodes/thymio-II/motor.left.target/100`
  sets the speed of the left motor
- `curl http://nodes/thymio-II/sound_system/4/10`
  might record sound number 4 for 10 seconds, if such an event were defined in AESL.
                
Variables and events are learned from the node description and parsed from AESL source when provided.
Server-side event (SSE) streams are updated as events arrive.
If a variable and an event have the same name, it is the EVENT that is called.
On a local machine the server can handle 600 requests/sec with 10 concurrent connections,
more (up to 2.5 times more) if the requests are pipelined as is the HTTP/1.1 default.

Start an 'asebadummynode 0' and run 'make test' to execute some basic unit tests.
Example [Node-RED](http://nodered.org) flows can be found in ../../examples/http/node-red.

DONE:
- Dashel connection to one Thymio-II and round-robin scheduling between Aseba and HTTP connections
- read Aesl program at launch, upload to Thymio-II, and record interface for introspection
- GET /nodes, GET /nodes/:NODENAME with introspection
- POST /nodes/:NODENAME/:VARIABLE (sloppily allows GET /nodes/:NODENAME/:VARIABLE/:VALUE\[/:VALUE\]*)
- POST /nodes/:NODENAME/:EVENT (sloppily allows GET /nodes/:NODENAME/:EVENT\[/:VALUE\]*)
- form processing for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)
- handle asynchronous variable reporting (GET /nodes/:NODENAME/:VARIABLE)
- JSON format for variable reporting (GET /nodes/:NODENAME/:VARIABLE)
- implement SSE streams and event filtering (GET /events) and (GET /nodes/:NODENAME/events)
- Aesl program bytecode upload (PUT /nodes/:NODENAME)
  use curl --data-ascii "file=$(cat vmcode.aesl)" -X PUT http://127.0.0.1:3000/nodes/thymio-II
- accept JSON payload rather than HTML form for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)

TODO:
- handle more than just one Thymio-II node
- gracefully shut down TCP/IP connections (half-close, wait, close)

This code borrows from the rest of Aseba, especially switches/medulla and examples/clients/cpp-shell,
which bear the copyright and LGPL 3 licensing notice below.


/*
Aseba - an event-based framework for distributed robot control
Copyright (C) 2007--2015:
Stephane Magnenat <stephane at magnenat dot net>
(http://stephane.magnenat.net)
and other contributors, see authors.txt for details

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/