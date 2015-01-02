asebahttp - a switch to bridge HTTP to Aseba

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

Server-side event (SSE) streams are updated as events arrive.
If a variable and an event have the same name, it is the EVENT the is called.
On a local machine the server can handle 600 requests/sec with 10 concurrent connections,
more (up to 2.5 times more) if the requests are pipelined as is the HTTP/1.1 default.

A typical execution is: asebahttp --port 3000 --aesl mycode.aesl 'ser:name=Thymio-II' &
After myevents.aesl is compiled and uploaded, check with 'curl http://127.0.0.1:3000/nodes/thymio-II'

Start an 'asebadummynode 0' and run 'make test' to execute some basic unit tests.
Run the script 'do_valgrind_macosx.sh' to valgrind the server using curl and siege.
An example [Node-RED](http://nodered.org) flow can be copy/pasted from test-nodered-simple.json.

DONE (mostly):
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
- remove dependency on libjson? Our output is trivial and input syntax is limited to unsigned vectors

This code borrows from the rest of Aseba, especially switches/medulla and examples/clients/cpp-shell,
which bear the copyright and LGPL 3 licensing notice below.


/*
Aseba - an event-based framework for distributed robot control
Copyright (C) 2007--2012:
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