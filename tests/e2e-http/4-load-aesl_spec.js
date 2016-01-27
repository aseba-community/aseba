var frisby = require('frisby');

var NODE = 'dummynode-8';

var fs = require('fs');
var stream = require('stream');
var path = require('path');

var filePath = path.resolve(__dirname, 'dice.aesl');
var fileSize = fs.statSync(filePath).size;

frisby.create('PUT new program to ' + NODE)
.put('http://localhost:3001/nodes/' + NODE,
    fs.createReadStream(filePath),
    {
        json: false,
        headers: {
            "Content-Type": "application/octet-stream",
            "Content-Length": fileSize
        }
    })
.expectStatus(204)
.after(function(err, res, body) {
    frisby.create('Inspect new program in ' + NODE)
    .get('http://localhost:3001/nodes/' + NODE)
    .expectStatus(200)
    .expectHeader('Content-Type', 'application/json')
    .expectJSON({
        namedVariables: { dice:2 },
        events: { roll: 0 }
    })
    .afterJSON(function (body) {
        frisby.create('Get dice from ' + NODE)
        .get('http://localhost:3001/nodes/' + NODE + '/dice')
        .expectStatus(200)
        .expectJSONTypes([Number,Number])
        .afterJSON(function (body) {
            frisby.create('Roll dice on ' + NODE)
            .get('http://localhost:3001/nodes/' + NODE + '/roll')
            .expectStatus(204)
            .after(function (err, res, body) {
                frisby.create('Get dice from ' + NODE)
                .get('http://localhost:3001/nodes/' + NODE + '/dice')
                .expectStatus(200)
                .expectJSONTypes([Number,Number])
                .toss();
            })
            .toss();
        })
        .toss();
    })
    .toss();
})
.toss();

frisby.create('PUT a malformed program to ' + NODE)
.put('http://localhost:3001/nodes/' + NODE,
    fs.createReadStream(filePath),
    {
        json: false,
        headers: {
            "Content-Type": "application/octet-stream",
            "Content-Length": fileSize - 100
        }
    })
.expectStatus(500)
.toss();
