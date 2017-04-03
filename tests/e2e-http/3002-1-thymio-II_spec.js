var frisby = require('frisby');

var ASEBAHTTP = 'http://localhost:3002/';
var NODENUM = 'nodes/1';
var THYMIO2 = 'nodes/thymio-II';

frisby.create('Verify no root endpoint')
.get(ASEBAHTTP)
.expectStatus(404)
.toss();

frisby.create('Get an initial list of nodes')
.get(ASEBAHTTP + 'nodes')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes('*', { name: String, protocolVersion: Number })
.expectJSONLength(1)
.toss();

frisby.create('Get node description for ' + THYMIO2)
.get(ASEBAHTTP + THYMIO2)
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.afterJSON(function (body) {
    var node_id = body["node"]
    for (node of [THYMIO2, 'nodes/'+node_id]) {
	frisby.create('Get node description for ' + node)
	.get(ASEBAHTTP + node)
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
})
.toss();

frisby.create('Fail to connect to nonexistant node')
.get(ASEBAHTTP + 'does-not-exist')
.expectStatus(404)
.toss();

frisby.create('Get the _id variable')
.get(ASEBAHTTP + THYMIO2 + '/_id')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes([Number])
.toss();

frisby.create('Sound startup')
.get(ASEBAHTTP + THYMIO2 + '/A_sound_system/0')
.expectStatus(204)
.toss();

for (node of [THYMIO2]) {
    frisby.create('Check user variables in node description for ' + node)
    .get(ASEBAHTTP + node)
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSON({
        namedVariables: { "_fwversion":2, "motor.left.target":1, acc:3, leds_circle:8 }
    })
    .toss();
}

frisby.create('Fail to get undefined user variables for ' + THYMIO2)
.get(ASEBAHTTP + THYMIO2 + '/whoami')
.expectStatus(404)
.toss();

frisby.create('Get/set/get/set the LED circle variable on ' + THYMIO2)
.get(ASEBAHTTP + THYMIO2 + '/leds_circle')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes([Number,Number,Number,Number,Number,Number,Number,Number])
.afterJSON(function (body) {
    frisby.create('Call event ' + THYMIO2 + '/V_leds_circle using POST to set LEDs')
    .post(ASEBAHTTP + THYMIO2 + '/V_leds_circle', Array(8).fill(16), {json: true})
    .expectStatus(204)
    .after(function(err, res, body) {
        // Assume no events are running!
        frisby.create('Check the values of ' + THYMIO2+ ' leds_circle, after POST V_leds_circle')
        .get(ASEBAHTTP + THYMIO2 + '/leds_circle')
        .expectStatus(200)
        .expectHeader('Content-Type', 'application/json')
        .expectJSONLength(8)
        .expectJSON([16,16,16,16,16,16,16,16])
        .afterJSON(function (body) {
            frisby.create('Call event ' + THYMIO2 + '/V_leds_circle using GET to set LEDs')
            .get(ASEBAHTTP + THYMIO2 + '/V_leds_circle/8/8/8/8/8/8/8/8')
            .expectStatus(204)
            .after(function(err, res, body) {
                // Assume no events are running!
                frisby.create('Check the values of ' + THYMIO2+ ' leds_circle, after GET V_leds_circle')
                .get(ASEBAHTTP + THYMIO2 + '/leds_circle')
                .expectStatus(200)
                .expectHeader('Content-Type', 'application/json')
                .expectJSONLength(8)
                .expectJSON([8,8,8,8,8,8,8,8])
                .afterJSON(function (body) {
                    frisby.create('Call event ' + THYMIO2 + '/V_leds_circle to reset the values of leds_circle')
                    .post(ASEBAHTTP + THYMIO2 + '/V_leds_circle', Array(8).fill(0), {json: true})
                    .expectStatus(204)
                    .toss()
                })
                .toss()
            })
            .toss();
        })
        .toss();
    })
    .toss();
})
.toss();

frisby.create('Set then reset the user variable motor.left.target, using POST & JSON')
.get(ASEBAHTTP + THYMIO2 + '/motor.left.target')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_motor_left_target = body[0]
    frisby.create('Set the value of motor.left.target')
    .post(ASEBAHTTP + THYMIO2 + '/motor.left.target', [143], {json: true})
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of motor.left.target')
        .get(ASEBAHTTP + THYMIO2 + '/motor.left.target')
        .expectStatus(200)
        .expectJSON([143])
        .afterJSON(function (body) {
            frisby.create('Set the value of motor.left.target')
            .post(ASEBAHTTP + THYMIO2 + '/motor.left.target', [old_motor_left_target], {json: true})
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of motor.left.target')
                .get(ASEBAHTTP + THYMIO2 + '/motor.left.target')
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
.get(ASEBAHTTP + THYMIO2 + '/motor.right.target')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_motor_right_target = body[0]
    frisby.create('Set the value of motor.right.target')
    .get(ASEBAHTTP + THYMIO2 + '/motor.right.target/143')
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of motor.right.target')
        .get(ASEBAHTTP + THYMIO2 + '/motor.right.target')
        .expectStatus(200)
        .expectJSON([143])
        .afterJSON(function (body) {
            frisby.create('Set the value of motor.right.target')
            .get(ASEBAHTTP + THYMIO2 + '/motor.right.target/' + old_motor_right_target)
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of motor.right.target')
                .get(ASEBAHTTP + THYMIO2 + '/motor.right.target')
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
.get(ASEBAHTTP + THYMIO2 + '/motor.left.target')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_motor_left_target = body[0]
    frisby.create('Set the value of motor.left.target')
    .post(ASEBAHTTP + THYMIO2 + '/motor.left.target', [143], {json: true})
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of R_state')
        .get(ASEBAHTTP + THYMIO2 + '/R_state')
        .expectStatus(200)
	.expectJSONTypes('*', Number)
	// .inspectBody()
        .afterJSON(function (body) {
	    expect(body[5]).toEqual(143)
	})
        .afterJSON(function (body) {
            frisby.create('Set the value of motor.left.target')
            .post(ASEBAHTTP + THYMIO2 + '/motor.left.target', [old_motor_left_target], {json: true})
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of motor.left.target')
                .get(ASEBAHTTP + THYMIO2 + '/motor.left.target')
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
.get(ASEBAHTTP + 'events?todo=4')
.expectStatus(200)
.expectHeader('Content-Type', 'text/event-stream')
.expectHeader('Connection', 'keep-alive')
.expectBodyContains('data: Q_motion_started')
.expectBodyContains('data: Q_motion_ended')
.toss()

frisby.create('Sound shutdown')
.get(ASEBAHTTP + THYMIO2 + '/A_sound_system/1')
.expectStatus(204)
.toss();
