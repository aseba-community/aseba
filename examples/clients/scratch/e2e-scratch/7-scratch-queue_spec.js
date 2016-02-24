var frisby = require('frisby');

var ASEBASCRATCH = 'http://localhost:3002/';
var THYMIO2 = 'nodes/thymio-II';

frisby.create('Check motion queue')
.get(ASEBASCRATCH + THYMIO2 + '/scratch_move/99/20')
.expectStatus(200)
.expectBodyContains('99')
.afterJSON(function (body) {
    frisby.create('Get the value of motor.left.target after scratch_move')
    .get(ASEBASCRATCH + THYMIO2 + '/motor.left.target')
    .expectStatus(200)
    .expectJSON([64]) // speed 64 is 20 mm/sec
    .after(function(err, res, body) {
        frisby.create('See event streams')
        .get(ASEBASCRATCH + 'events?todo=2')
        .expectStatus(200)
        .expectHeader('Content-Type', 'text/event-stream')
        .expectHeader('Connection', 'keep-alive')
        .expectBodyContains('data: Q_motion_ended')
        .expectBodyContains('data: Q_motion_noneleft')
        .after(function(err, res, body) {
            frisby.create('Get the value of motor.left.target after Q_motion_ended')
            .get(ASEBASCRATCH + THYMIO2 + '/motor.left.target')
            .expectStatus(200)
            .expectJSON([0])
            .toss()
        })
        .toss()
    })
    .toss()
})
.toss()
