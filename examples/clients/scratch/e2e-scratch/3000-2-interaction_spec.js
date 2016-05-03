var frisby = require('frisby');

var LISTENER1 = 'dummynode-0';
var LISTENER2 = 'dummynode-1';
var LISTENER3 = 'dummynode-2';
var CLOCK = 'dummynode-3';

frisby.create('Get an initial list of nodes')
.get('http://localhost:3000/nodes')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes('*', { name: String, protocolVersion: Number })
.expectJSONLength(4)
.toss();

for (node of [LISTENER1,LISTENER2,LISTENER3,CLOCK]) {
    frisby.create('Get node description for ' + node)
    .get('http://localhost:3000/nodes/' + node)
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSONTypes({
        name: String,
        protocolVersion: Number,
        bytecodeSize: Number,
        variablesSize: Number,
        stackSize: Number,
        namedVariables: Object,
        localEvents: Object,
        constants: Object,
        events: Object
    })
    .toss();
}


frisby.create('See event streams')
.post('http://localhost:3000/nodes/' + CLOCK + '/running', [1], {json: true})
.expectStatus(204)
.after(function(err, res, body) {
    frisby.create('Get the value of running')
    .get('http://localhost:3000/nodes/' + CLOCK + '/running')
    .expectStatus(200)
    .expectJSON([1])
    .after(function(err, res, body) {
        frisby.create('Open event stream')
        .get('http://localhost:3000/events?todo=5')
        .expectStatus(200)
        .expectHeader('Content-Type', 'text/event-stream')
        .expectHeader('Connection', 'keep-alive')
        .expectBodyContains('data: wakeup')
        .expectBodyContains('data: ping')
        .toss()
    })
    .toss()
})
.toss()
