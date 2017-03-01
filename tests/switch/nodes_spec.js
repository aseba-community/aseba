// this test only requires the switch and two dummies, on ports 33333 and 33334

var frisby = require('frisby');
var port = 3000

frisby.create('Get the list of nodes')
.get('http://localhost:' + port + '/nodes')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON([
	{ id: 1, name: "dummynode-0", pid: -1 },
	{ id: 2, name: "dummynode-1", pid: -1 }
])
.toss();

frisby.create('Fail to get the description of a node given by name')
.get('http://localhost:' + port + '/nodes/dummynode-0')
.expectStatus(404)
.toss()

frisby.create('Fail to get the description of unexisting node')
.get('http://localhost:' + port + '/nodes/3')
.expectStatus(404)
.toss()

frisby.create('Get the description of node 1')
.get('http://localhost:' + port + '/nodes/1')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes({
	properties: Object,
	program: Object,
	variables: Object
})
.toss();

// TODO: add check for properties, program, and variables
// TODO: write program and get compilation result
// TODO: set and get variables
