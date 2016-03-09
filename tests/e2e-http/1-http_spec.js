var frisby = require('frisby');

var LISTENER1 = 'dummynode-0';
var LISTENER2 = 'dummynode-1';
var LISTENER3 = 'dummynode-2';
var CLOCK = 'dummynode-3';
var CLOCK2 = 'dummynode-6';

for (port of [3000,3001]) {
    frisby.create('Verify no root endpoint')
    .get('http://localhost:' + port + '/')
    .expectStatus(404)
    .toss();

    frisby.create('Get an initial list of nodes')
    .get('http://localhost:' + port + '/nodes')
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSONTypes('*', { name: String, protocolVersion: Number })
    .afterJSON(function (body) {
        expect(body.length).toBeGreaterThan(3)
    })
    .toss();
}

for (node of [1,CLOCK]) {
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

frisby.create('Fail to connect to nonexistant node')
.get('http://localhost:3000/nodes/' + 'does-not-exist')
.expectStatus(404)
.toss();

frisby.create('Get the id variable')
.get('http://localhost:3000/nodes/' + CLOCK + '/id')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes([Number])
.toss()

frisby.create('Get/set the unused variable on ' + CLOCK)
.get('http://localhost:3000/nodes/' + CLOCK + '/vec10')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(10)
.afterJSON(function (body) {
    // Assume no events are running!
    frisby.create('Set the value of vec10 on ' + CLOCK)
    .post('http://localhost:3000/nodes/' + CLOCK + '/vec10', Array(10).fill(1), {json: true})
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of vec10 on ' + CLOCK)
        .get('http://localhost:3000/nodes/' + CLOCK + '/vec10')
        .expectStatus(200)
        .expectJSONLength(10)
        .expectJSON([1,1,1,1,1,1,1,1,1,1])
        .expectJSON(Array(10).fill(1))
        .toss()
    })
    .toss()
})
.toss()

for (node of [LISTENER1,LISTENER2]) {
    frisby.create('Check user variables in node description for ' + node)
    .get('http://localhost:3000/nodes/' + node)
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSON({
        namedVariables: { id:1, whoami:1 },
    })
    .toss();
}

for (node of [CLOCK]) {
    frisby.create('Check user variables in node description for ' + node)
    .get('http://localhost:3000/nodes/' + node)
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSON({
        namedVariables: { id:1, running:1 },
    })
    .toss();
}

frisby.create('Get user variables for ' + CLOCK)
.get('http://localhost:3000/nodes/' + CLOCK + '/running')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes([Number])
.toss();

frisby.create('Fail to get undefined user variables for ' + CLOCK)
.get('http://localhost:3000/nodes/' + CLOCK + '/whoami')
.expectStatus(404)
.toss();

frisby.create('Set then reset the user variable whoami, using POST & JSON')
.get('http://localhost:3000/nodes/' + LISTENER1 + '/whoami')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_whoami = body[0]
    frisby.create('Set the value of whoami')
    .post('http://localhost:3000/nodes/' + LISTENER1 + '/whoami', [27182], {json: true})
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of whoami')
        .get('http://localhost:3000/nodes/' + LISTENER1 + '/whoami')
        .expectStatus(200)
        .expectJSON([27182])
        .afterJSON(function (body) {
            frisby.create('Set the value of whoami')
            .post('http://localhost:3000/nodes/' + LISTENER1 + '/whoami', [old_whoami], {json: true})
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of whoami')
                .get('http://localhost:3000/nodes/' + LISTENER1 + '/whoami')
                .expectStatus(200)
                .expectJSON([old_whoami])
                .toss()
            })
            .toss()
        })
        .toss()
    })
    .toss()
})
.toss()

frisby.create('Set then reset the user variable whoami, using GET & URI args')
.get('http://localhost:3000/nodes/' + LISTENER1 + '/whoami')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_whoami = body[0]
    frisby.create('Set the value of whoami')
    .get('http://localhost:3000/nodes/' + LISTENER1 + '/whoami/27182')
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of whoami')
        .get('http://localhost:3000/nodes/' + LISTENER1 + '/whoami')
        .expectStatus(200)
        .expectJSON([27182])
        .afterJSON(function (body) {
            frisby.create('Set the value of whoami')
            .get('http://localhost:3000/nodes/' + LISTENER1 + '/whoami/' + old_whoami)
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of whoami')
                .get('http://localhost:3000/nodes/' + LISTENER1 + '/whoami')
                .expectStatus(200)
                .expectJSON([old_whoami])
                .toss()
            })
            .toss()
        })
        .toss()
    })
    .toss()
})
.toss()

frisby.create('Changing whoami on ' + LISTENER2 + ' doesn\'t affect ' + LISTENER3)
.get('http://localhost:3000/nodes/' + LISTENER3 + '/whoami')
.expectStatus(200)
.expectJSONLength(1)
.afterJSON(function (body) {
    var whoami_1 = body[0]
    frisby.create('Set the value of whoami on ' + LISTENER2)
    .post('http://localhost:3000/nodes/' + LISTENER2 + '/whoami', [whoami_1 + 1], {json: true})
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of whoami on ' + LISTENER3)
        .get('http://localhost:3000/nodes/' + LISTENER3 + '/whoami')
        .expectStatus(200)
        .expectJSON([whoami_1])
        .afterJSON(function (body) {
            frisby.create('Set the value of whoami')
            .post('http://localhost:3000/nodes/' + LISTENER2 + '/whoami', [2000], {json: true})
            .expectStatus(204)
            .toss()
        })
        .toss()
    })
    .toss()
})
.toss()

frisby.create('Change the runnning variable on ' + CLOCK2 + ' through start and stop events')
.get('http://localhost:3001/nodes/' + CLOCK2 + '/running')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_running = body[0]
    frisby.create('Stop the clock')
    .get('http://localhost:3001/nodes/' + CLOCK2 + '/stop')
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Check that running = 0')
        .get('http://localhost:3001/nodes/' + CLOCK2 + '/running')
        .expectStatus(200)
        .expectJSON([0])
        .afterJSON(function (body) {
            frisby.create('Set the value of whoami')
            .get('http://localhost:3001/nodes/' + CLOCK2 + '/start')
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Check that running = 1')
                .get('http://localhost:3001/nodes/' + CLOCK2 + '/running')
                .expectStatus(200)
                .expectJSON([1])
                .toss()
            })
            .toss()
        })
        .toss()
    })
    .toss()
})
.toss()
