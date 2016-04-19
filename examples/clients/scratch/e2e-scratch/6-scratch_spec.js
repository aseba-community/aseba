var frisby = require('frisby');

var ASEBASCRATCH = 'http://localhost:3002/';
var THYMIO2 = 'nodes/thymio-II';

frisby.create('Reset Thymio-II')
.get(ASEBASCRATCH + 'reset')
.expectStatus(204)
.toss();

frisby.create('Poll ' + THYMIO2)
.get(ASEBASCRATCH + 'poll')
.expectStatus(200)
.expectHeader('Content-Type', 'text/plain')
.expectBodyContains('_busy')
.after(function(err, res, body) {
    frisby.create('Poll ' + THYMIO2)
    .get(ASEBASCRATCH + 'poll')
    .expectBodyContains('motor.left.target')
    .toss()
})
.toss();

frisby.create('Sound rising')
.get(ASEBASCRATCH + THYMIO2 + '/A_sound_system/7')
.expectStatus(204)
.toss();

for (node of [THYMIO2]) {
    frisby.create('Check description for ' + node)
    .get(ASEBASCRATCH + node)
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSON({
        namedVariables: { "_fwversion":2, "motor.left.target":1 },
    events: { "A_sound_system":1, "V_leds_circle":8,
        "scratch_start":2,
        "scratch_change_speed":2,
        "scratch_stop":0,
        "scratch_set_dial":1,
        "scratch_next_dial_limit":1,
        "scratch_next_dial":0,
        "scratch_clear_leds":0,
        "scratch_set_leds":2,
        "scratch_change_leds":2,
        "scratch_move":2,
        "scratch_turn":2,
        "scratch_arc":3,
		}
    })
    .toss();
}

frisby.create('Start motor then stop motor, checking motor.left.target')
.get(ASEBASCRATCH + THYMIO2 + '/scratch_start/10/0')
.expectStatus(204)
.after(function(err, res, body) {
    frisby.create('Get the value of motor.left.target after scratch_start')
    .get(ASEBASCRATCH + THYMIO2 + '/motor.left.target')
    .expectStatus(200)
    //.inspectBody()
    .expectJSON([32]) // speed 32 is 10 mm/sec
    .afterJSON(function (body) {
        frisby.create('Stop motors, check motor.left.target')
        .get(ASEBASCRATCH + THYMIO2 + '/scratch_stop')
        .expectStatus(204)
        .after(function(err, res, body) {
            frisby.create('Get the value of motor.left.target after scratch_stop')
            .get(ASEBASCRATCH + THYMIO2 + '/motor.left.target')
            .expectStatus(200)
            .expectJSON([0])
            .after(function(err, res, body) {
                frisby.create('Check that state changes are seen in R_state')
                .get(ASEBASCRATCH + THYMIO2 + '/motor.right.target')
                .expectStatus(200)
                .expectHeader('Content-Type', 'application/json')
                .expectJSONLength(1)
                //.inspectBody()
                .afterJSON(function (body) {
                    var old_motor_right_target = body[0]
                    frisby.create('Change motor.right.target')
                    .get(ASEBASCRATCH + THYMIO2 + '/scratch_change_speed/0/10')
                    .expectStatus(204)
                    .after(function(err, res, body) {
                        frisby.create('Get the value of R_state after scratch_change_speed')
                        .get(ASEBASCRATCH + THYMIO2 + '/R_state')
                        .expectStatus(200)
                        .expectJSONTypes('*', Number)
                        //.inspectBody()
                        .afterJSON(function (body) {
                            expect(body[6]).toEqual(32 + old_motor_right_target)
                        })
                        .afterJSON(function (body) {
                            frisby.create('Reverse to zero right motor')
                            .get(ASEBASCRATCH + THYMIO2 + '/scratch_change_speed/0/-10')
                            .expectStatus(204)
                            .after(function(err, res, body) {
                                frisby.create('Get the value of motor.right.target after scratch_change_speed')
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
            })
            .toss()
        })
        .toss()
    })
    .toss()
})
.toss()

frisby.create('Check that dial turns')
.get(ASEBASCRATCH + THYMIO2 + '/scratch_set_dial/0')
.expectStatus(204)
.after(function(err, res, body) {
    for (i of Array(20).fill(1)) {
	frisby.create('Next dial')
	.get(ASEBASCRATCH + THYMIO2 + '/scratch_next_dial')
	.expectStatus(204)
	.toss()
    }
    // should check dial value, but no reporter
})
.toss()

frisby.create('Check that colors change')
.get(ASEBASCRATCH + THYMIO2 + '/scratch_set_leds/0/7')
.expectStatus(204)
.after(function(err, res, body) {
    for (i of Array(50).fill(8)) {
        frisby.create('Next color')
        .get(ASEBASCRATCH + THYMIO2 + '/scratch_change_leds/' + i + '/7')
        .expectStatus(204)
        .toss()
    }
    frisby.create('Clear colors')
    .get(ASEBASCRATCH + THYMIO2 + '/scratch_clear_leds')
    .expectStatus(204)
    .toss()
    // should check color value, but no reporter
})
.toss()
