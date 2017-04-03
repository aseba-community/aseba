var frisby = require('frisby');

var ASEBASCRATCH = 'http://localhost:3002/';
var THYMIO2 = 'nodes/thymio-II';

frisby.create('Reset Thymio-II')
.get(ASEBASCRATCH + 'reset')
.expectStatus(204)
.toss();

frisby.create('Ignore duplicate requests')
.get(ASEBASCRATCH + THYMIO2 + '/Q_add_motion/101/100/10/-10')
.expectStatus(204)
.after(function(err, res, body) {
    frisby.create('Request that should be ignored')
    .get(ASEBASCRATCH + THYMIO2 + '/Q_add_motion/101/100/10/-10')
    .expectStatus(204)
    .after(function(err, res, body) {
        frisby.create('Get the value of Qid')
        .get(ASEBASCRATCH + THYMIO2 + '/Qid')
        .expectStatus(200)
        .expectHeader('Content-Type', 'application/json')
        .afterJSON(function (body) {
            var Qid = body;
            var first = Qid.indexOf(101);
            var second = Qid.indexOf(101,first+1);
            
            runs(function() {
                expect(first).toBeGreaterThan(-1)
                expect(second).toEqual(-1);
            })
            frisby.create('See event streams')
            .get(ASEBASCRATCH + 'events?todo=40')
            .expectStatus(200)
            .after(function(err, res, body) {
                var num = body.match(/Q_motion_ended/gm).length;
                runs(function() {
                    expect(num).toEqual(1)
                });
            })
            .toss();
        })
        .toss();
    })
    .toss()
})
.toss();
