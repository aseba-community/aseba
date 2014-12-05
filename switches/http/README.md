asebahttp - a switch to bridge HTTP to Aseba
2014-12-01 David James Sherman <david dot sherman at inria dot fr>

Provide a simple REST interface with introspection for Aseba devices.

GET  /nodes                                 - JSON list of all known nodes
GET  /nodes/:NODENAME                       - JSON attributes for :NODENAME
PUT  /nodes/:NODENAME                       - write new Aesl program (file= in multipart/form-data)
GET  /nodes/:NODENAME/:VARIABLE             - retrieve JSON value for :VARIABLE
POST /nodes/:NODENAME/:VARIABLE             - send new values(s) for :VARIABLE
POST /nodes/:NODENAME/:EVENT                - call an event :EVENT
GET  /events[/:EVENT]*                      - create SSE stream for all known nodes
GET  /nodes/:NODENAME/events[/:EVENT]*      - create SSE stream for :NODENAME

Server-side event (SSE) streams are updated as events arrive.
SSE stream for reserved event "poll" sends variables at approx 10 Hz.
If a variable and an events have the same name, it is the EVENT the is called.

DONE (mostly):
- Dashel connection to one Thymio-II and round-robin scheduling between Dashel and Mongoose
- read Aesl program at launch, upload to Thymio-II, and record interface for introspection
- GET /nodes, GET /nodes/:NODENAME with introspection
- POST /nodes/:NODENAME/:VARIABLE (fixme: sloppily allows GET and values in the request)
- POST /nodes/:NODENAME/:EVENT (fixme: sloppily allows GET and values in the request)
- handle asynchronous variable reporting (GET /nodes/:NODENAME/:VARIABLE)
- JSON format for variable reporting (GET /nodes/:NODENAME/:VARIABLE)

TODO:
- allow only POST requests for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)
- form processing for updates and events (POST /.../:VARIABLE) and (POST /.../:EVENT)
- handle more than just one Thymio-II node!
- program flashing (PUT  /nodes/:NODENAME)
- implement SSE streams and event filtering (GET /events) and (GET /nodes/:NODENAME/events)

This code includes source code from the Mongoose embedded web server from Cesanta
Software Limited, Dublin, Ireland, and licensed under the GPL 2.

This code borrows heavily from the rest of Aseba, especially switches/medulla and
examples/clients/cpp-shell, which bear the copyright and LGPL 3 licensing notice below.


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