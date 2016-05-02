var frisby = require('frisby');

var LISTENER1 = 'dummynode-0';
var CLOCK = 'dummynode-3';
var AESL_LISTENER1 = '1';
var AESL_CLOCK = '4';

frisby.create('Get an initial list of nodes')
.get('http://localhost:3005/nodes')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes('*', { name: String, protocolVersion: Number })
.afterJSON(function (body) {
    expect(body.length).toBeGreaterThan(1)
})
.toss();

for (node of [LISTENER1,CLOCK,AESL_LISTENER1,AESL_CLOCK]) {
    frisby.create('Get node description for ' + node)
    .get('http://localhost:3005/nodes/' + node)
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

frisby.create('Get the id variable')
.get('http://localhost:3005/nodes/' + AESL_CLOCK + '/id')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes([Number])
.toss()
