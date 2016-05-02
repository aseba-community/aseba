var frisby = require('frisby');

// Asebadummynode 6 gave target node 7, which we remapped to aesl node 4 using
// dashel target tcp:;port=33339;remapAesl=4;remapTarget=7'.
// Since it is aesl node 4 it will be the clock from ping0123.aesl.

var NODE4 = 'dummynode-4';
var NODE5 = 'dummynode-5';
var NODE6 = 'dummynode-6';
var NODE7 = 'dummynode-7';
var NODECLOCK = 7;
var CLOCK = 4;

frisby.create('Get an initial list of nodes')
.get('http://localhost:3001/nodes')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes('*', { node: Number, name: String, aeslId: Number, protocolVersion: Number })
.toss();

for (node of [NODE6, NODECLOCK]) {
    frisby.create('Check nodeId not remapped for ' + node)
    .get('http://localhost:3001/nodes/' + node)
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSONTypes({ namedVariables: { running:Number } })
    .expectJSON({
        node: NODECLOCK,
        aeslId: CLOCK,
        name: NODE6,
    })
    .toss();
}

frisby.create('Check conflicting nodeId remapped for ' + NODE5 + ' and ' + NODE7)
.get('http://localhost:3001/nodes/' + NODE5)
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.afterJSON(function (body) {
    var id_5 = body.node;
    frisby.create('Get nodeId remapped for ' + NODE7)
    .get('http://localhost:3001/nodes/' + NODE7)
    .expectStatus(200)
    .afterJSON(function (body) {
        var id_7 = body.node;
        runs(function() {
            expect(id_5).not.toEqual(id_7)
        });
    })
    .toss();
})
.toss();

frisby.create('Check same aeslId but different nodeId for ' + NODE4 + ' and ' + NODE5)
.get('http://localhost:3001/nodes/' + NODE4)
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.afterJSON(function (body) {
    var id_4 = body.node;
    var aesl_4 = body.aeslId;
    frisby.create('Get node for ' + NODE5)
    .get('http://localhost:3001/nodes/' + NODE5)
    .expectStatus(200)
    .afterJSON(function (body) {
        var id_5 = body.node;
        var aesl_5 = body.aeslId;
        runs(function() {
            expect(id_4).not.toEqual(id_5);
            expect(aesl_4).toEqual(aesl_5);
        });
    })
    .toss();
})
.toss();
