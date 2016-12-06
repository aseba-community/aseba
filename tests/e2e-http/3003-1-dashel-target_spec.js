var frisby = require('frisby');

var LISTENER1 = 'dummynode-0';
var LISTENER2 = 'dummynode-1';
var LISTENER3 = 'dummynode-2';
var CLOCK = 'dummynode-3';

frisby.create('Get an initial list of nodes')
.get('http://localhost:3003/nodes')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes('*', { node: Number, name: String, aeslId: Number, protocolVersion: Number })
.toss();

for (node of [LISTENER1,LISTENER2,LISTENER3,CLOCK]) {
    frisby.create('Get node description for ' + node)
    .get('http://localhost:3003/nodes/' + node)
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

frisby.create('Fail to connect to nonexistant node')
.get('http://localhost:3003/nodes/' + 'does-not-exist')
.expectStatus(404)
.toss();

for (node of [LISTENER1,LISTENER2,LISTENER3]) {
    frisby.create('Get the whoami variable')
    .get('http://localhost:3003/nodes/' + node + '/whoami')
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSONTypes([Number])
    .toss()
}
