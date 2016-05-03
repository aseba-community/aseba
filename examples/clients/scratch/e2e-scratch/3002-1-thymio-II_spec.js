var frisby = require('frisby');

var ASEBASCRATCH = 'http://localhost:3002/';
var NODENUM = 'nodes/1';
var THYMIO2 = 'nodes/thymio-II';

frisby.create('Reset Thymio-II')
.get(ASEBASCRATCH + 'reset')
.expectStatus(204)
.toss();

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

frisby.create('Get node description for ' + THYMIO2)
.get(ASEBASCRATCH + THYMIO2)
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.afterJSON(function (body) {
    var node_id = body["node"]
    for (node of [THYMIO2, 'nodes/'+node_id]) {
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
})
.toss();

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
        namedVariables: { "_fwversion":2, "motor.left.target":1, acc:3, Qid:4, Qtime:4 }
    })
    .toss();
}

frisby.create('Get user variables for ' + THYMIO2)
.get(ASEBASCRATCH + THYMIO2 + '/Qid')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes([Number,Number,Number,Number])
.toss();

frisby.create('Fail to get undefined user variables for ' + THYMIO2)
.get(ASEBASCRATCH + THYMIO2 + '/whoami')
.expectStatus(404)
.toss();

frisby.create('Set/get/set the Q variables on ' + THYMIO2)
.post(ASEBASCRATCH + THYMIO2 + '/Q_add_motion', [1,500,1,1], {json: true})
.expectStatus(204)
.after(function(err, res, body) {
    frisby.create('Get the value of Qtime on ' + THYMIO2)
    .get(ASEBASCRATCH + THYMIO2 + '/Qtime')
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSONLength(4)
    .afterJSON(function (body) {
	expect(body[0]).toBeLessThan(0)
	expect(body[0]).toBeGreaterThan(-500)
    })
    //.expectJSON([1,0,0,0])
    .afterJSON(function (body) {
	frisby.create('See event streams')
	.get(ASEBASCRATCH + 'events?todo=2')
        .inspectBody()
	.expectStatus(200)
	.expectHeader('Content-Type', 'text/event-stream')
	.expectHeader('Connection', 'keep-alive')
	.expectBodyContains('data: Q_motion_ended')
	.toss()
    })
    .afterJSON(function (body) {
        frisby.create('Reset the value of Qtime on ' + THYMIO2)
	.post(ASEBASCRATCH + THYMIO2 + '/Qtime', Array(4).fill(0), {json: true})
	.expectStatus(204)
        .toss()
    })
    .toss()
})
.toss();

frisby.create('Set then reset the user variable R_state.do, using POST & JSON')
.get(ASEBASCRATCH + THYMIO2 + '/R_state.do')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_flag = body[0]
    frisby.create('Set the value of R_state.do')
    .post(ASEBASCRATCH + THYMIO2 + '/R_state.do', [143], {json: true})
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of R_state.do')
        .get(ASEBASCRATCH + THYMIO2 + '/R_state.do')
        .expectStatus(200)
        .expectJSON([143])
        .afterJSON(function (body) {
            frisby.create('Set the value of R_state.do')
            .post(ASEBASCRATCH + THYMIO2 + '/R_state.do', [old_flag], {json: true})
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of R_state.do')
                .get(ASEBASCRATCH + THYMIO2 + '/R_state.do')
                .expectStatus(200)
                .expectJSON([old_flag])
                .toss()
            })
            .toss()
        })
        .toss()
    })
    .toss()
})
.toss()

frisby.create('Set then reset the user variable mic.threshold, using GET & URI args')
.get(ASEBASCRATCH + THYMIO2 + '/mic.threshold')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_threshold = body[0]
    frisby.create('Set the value of mic.threshold')
    .get(ASEBASCRATCH + THYMIO2 + '/mic.threshold/20')
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of mic.threshold')
        .get(ASEBASCRATCH + THYMIO2 + '/mic.threshold')
        .expectStatus(200)
        .expectJSON([20])
        .afterJSON(function (body) {
            frisby.create('Set the value of mic.threshold')
            .get(ASEBASCRATCH + THYMIO2 + '/mic.threshold/' + old_threshold)
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of mic.threshold')
                .get(ASEBASCRATCH + THYMIO2 + '/mic.threshold')
                .expectStatus(200)
                .expectJSON([old_threshold])
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
.get(ASEBASCRATCH + THYMIO2 + '/prox.comm.tx')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONLength(1)
.afterJSON(function (body) {
    var old_motor_left_target = body[0]
    frisby.create('Set the value of prox.comm.tx')
    .post(ASEBASCRATCH + THYMIO2 + '/prox.comm.tx', [143], {json: true})
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of R_state')
        .get(ASEBASCRATCH + THYMIO2 + '/R_state')
        .expectStatus(200)
	.expectJSONTypes('*', Number)
	// .inspectBody()
        .afterJSON(function (body) {
	    expect(body[13]).toEqual(143)
	})
        .afterJSON(function (body) {
            frisby.create('Set the value of prox.comm.tx')
            .post(ASEBASCRATCH + THYMIO2 + '/prox.comm.tx', [old_motor_left_target], {json: true})
            .expectStatus(204)
            .after(function(err, res, body) {
                frisby.create('Get the value of prox.comm.tx')
                .get(ASEBASCRATCH + THYMIO2 + '/prox.comm.tx')
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

frisby.create('Sound shutdown')
.get(ASEBASCRATCH + THYMIO2 + '/A_sound_system/1')
.expectStatus(204)
.toss();
