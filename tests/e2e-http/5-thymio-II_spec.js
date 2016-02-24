var frisby = require('frisby');

var ASEBASCRATCH = 'http://localhost:3002/';
var NODENUM = 'nodes/1';
var THYMIO2 = 'nodes/thymio-II';

frisby.create('Verify no root endpoint')
.get(ASEBASCRATCH)
.expectStatus(404)
.toss();

frisby.create('Get an initial list of nodes')
.get(ASEBASCRATCH + 'nodes')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes('*', { name: String, protocolVersion: Number })
.expectJSONLength(1)
.toss();

for (node of [NODENUM,THYMIO2]) {
    frisby.create('Get node description for ' + node)
    .get(ASEBASCRATCH + node)
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
.get(ASEBASCRATCH + 'does-not-exist')
.expectStatus(404)
.toss();

frisby.create('Get the _id variable')
.get(ASEBASCRATCH + THYMIO2 + '/_id')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes([Number])
.toss();

frisby.create('Sound startup')
.get(ASEBASCRATCH + THYMIO2 + '/A_sound_system/0')
.expectStatus(204)
.toss();

for (node of [THYMIO2]) {
    frisby.create('Check user variables in node description for ' + node)
    .get(ASEBASCRATCH + node)
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSON({
        namedVariables: { "_fwversion":2, "motor.left.target":1, acc:3, leds_circle:8 }
    })
    .toss();
}

frisby.create('Get user variables for ' + THYMIO2)
.get(ASEBASCRATCH + THYMIO2 + '/leds_circle')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes([Number,Number,Number,Number,Number,Number,Number,Number])
.toss();

frisby.create('Fail to get undefined user variables for ' + THYMIO2)
.get(ASEBASCRATCH + THYMIO2 + '/whoami')
.expectStatus(404)
.toss();

frisby.create('Set/get/set the LED circle variable on ' + THYMIO2)
.post(ASEBASCRATCH + THYMIO2 + '/V_leds_circle', Array(8).fill(16), {json: true})
.expectStatus(204)
.after(function(err, res, body) {
    // Assume no events are running!
    frisby.create('Get the value of V_leds_circle on ' + THYMIO2)
    .get(ASEBASCRATCH + THYMIO2 + '/leds_circle')
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSONLength(8)
    .expectJSON([16,16,16,16,16,16,16,16])
    .afterJSON(function (body) {
        frisby.create('Reset the value of V_leds_circle on ' + THYMIO2)
	.post(ASEBASCRATCH + THYMIO2 + '/V_leds_circle', Array(8).fill(0), {json: true})
	.expectStatus(204)
        .toss()
    })
    .toss()
})
.toss();

frisby.create('Set then reset the user variable motor.left.target, using POST & JSON')
.get(ASEBASCRATCH + THYMIO2 + '/motor.left.target')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_motor_left_target = body[0]
    frisby.create('Set the value of motor.left.target')
    .post(ASEBASCRATCH + THYMIO2 + '/motor.left.target', [143], {json: true})
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of motor.left.target')
        .get(ASEBASCRATCH + THYMIO2 + '/motor.left.target')
        .expectStatus(200)
        .expectJSON([143])
        .afterJSON(function (body) {
            frisby.create('Set the value of motor.left.target')
            .post(ASEBASCRATCH + THYMIO2 + '/motor.left.target', [old_motor_left_target], {json: true})
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of motor.left.target')
                .get(ASEBASCRATCH + THYMIO2 + '/motor.left.target')
                .expectStatus(200)
                .expectJSON([old_motor_left_target])
                .toss()
            })
            .toss()
        })
        .toss()
    })
    .toss()
})
.toss()

frisby.create('Set then reset the user variable motor.right.target, using GET & URI args')
.get(ASEBASCRATCH + THYMIO2 + '/motor.right.target')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_motor_right_target = body[0]
    frisby.create('Set the value of motor.right.target')
    .get(ASEBASCRATCH + THYMIO2 + '/motor.right.target/143')
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of motor.right.target')
        .get(ASEBASCRATCH + THYMIO2 + '/motor.right.target')
        .expectStatus(200)
        .expectJSON([143])
        .afterJSON(function (body) {
            frisby.create('Set the value of motor.right.target')
            .get(ASEBASCRATCH + THYMIO2 + '/motor.right.target/' + old_motor_right_target)
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of motor.right.target')
                .get(ASEBASCRATCH + THYMIO2 + '/motor.right.target')
                .expectStatus(200)
                .expectJSON([old_motor_right_target])
                .toss()
            })
            .toss()
        })
        .toss()
    })
    .toss()
})
.toss()

frisby.create('Check that state changes are seen in R_state')
.get(ASEBASCRATCH + THYMIO2 + '/motor.left.target')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_motor_left_target = body[0]
    frisby.create('Set the value of motor.left.target')
    .post(ASEBASCRATCH + THYMIO2 + '/motor.left.target', [143], {json: true})
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of R_state')
        .get(ASEBASCRATCH + THYMIO2 + '/R_state')
        .expectStatus(200)
	.expectJSONTypes('*', Number)
	// .inspectBody()
        .afterJSON(function (body) {
	    expect(body[5]).toEqual(143)
	})
        .afterJSON(function (body) {
            frisby.create('Set the value of motor.left.target')
            .post(ASEBASCRATCH + THYMIO2 + '/motor.left.target', [old_motor_left_target], {json: true})
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of motor.left.target')
                .get(ASEBASCRATCH + THYMIO2 + '/motor.left.target')
                .expectStatus(200)
                .expectJSON([old_motor_left_target])
                .toss()
            })
            .toss()
        })
        .toss()
    })
    .toss()
})
.toss()

frisby.create('See event streams')
.get(ASEBASCRATCH + 'events?todo=4')
.expectStatus(200)
.expectHeader('Content-Type', 'text/event-stream')
.expectHeader('Connection', 'keep-alive')
.expectBodyContains('data: Q_motion_started')
.expectBodyContains('data: Q_motion_ended')
.toss()

frisby.create('Sound shutdown')
.get(ASEBASCRATCH + THYMIO2 + '/A_sound_system/1')
.expectStatus(204)
.toss();
