var frisby = require('frisby');

var LISTENER1 = 'dummynode-0';
var LISTENER2 = 'dummynode-1';
var LISTENER3 = 'dummynode-2';
var CLOCK = 'dummynode-3';
var CLOCK2 = 'dummynode-6';

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
