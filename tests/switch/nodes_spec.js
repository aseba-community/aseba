// this test only requires the switch and two dummies, on ports 33333 and 33334

var frisby = require('frisby');
var port = 3000

// node listing

frisby.create('Get the list of nodes')
.get('http://localhost:' + port + '/nodes')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON([
	{ id: 1, name: "dummynode-0", pid: -1 },
	{ id: 2, name: "dummynode-1", pid: -1 }
])
.toss();

// node descriptions

frisby.create('Fail to get the description of a node given by name')
.get('http://localhost:' + port + '/nodes/dummynode-0')
.expectStatus(404)
.toss()

frisby.create('Fail to get the description of unexisting node')
.get('http://localhost:' + port + '/nodes/3')
.expectStatus(404)
.toss()

frisby.create('Get the informations for node 1')
.get('http://localhost:' + port + '/nodes/1')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
// Note: it seems frisby does not make an error if some of these entries are not present, why?
.expectJSONTypes({
	description: Object,
	program: {
		source: String,
		compilationResult: Object
	}
})
.toss();

frisby.create('Get the description for node 1')
.get('http://localhost:' + port + '/nodes/1/description')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON({
	id: 1,
	name: "dummynode-0"
})
.expectJSONTypes({
	id: Number,
	name: String,
	protocolVersion: Number,
	vmProperties: {
		bytecodeSize: Number,
		stackSize: Number,
		variablesSize: Number
	},
	nativeEvents: Array,
	nativeFunctions: Array,
	nativeVariables: Array
})
.expectJSONTypes('nativeEvents.*', {
	name: String,
	description: String
})
.expectJSONTypes('nativeFunctions.*', {
	name: String,
	description: String,
	parameters: Array
})
// Note: frisby does not support two * in its path, only one as last character
.expectJSONTypes('nativeFunctions.0.parameters.*', {
	name: String,
	size: Number
})
.expectJSONTypes('nativeVariables.*', {
	name: String,
	size: Number
})
.toss();

// variables

frisby.create('Get variable id from node 1')
.get('http://localhost:' + port + '/nodes/1/variables/id')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON([1])
.toss();

frisby.create('Fail to get unexisting variable doesnotexist from node 1')
.get('http://localhost:' + port + '/nodes/1/variables/doesnotexist')
.expectStatus(404)
.toss();

frisby.create('Fail to put unexisting variable doesnotexist to node 1')
.put('http://localhost:' + port + '/nodes/1/variables/doesnotexist', [0], {json: true})
.expectStatus(404)
.toss();

var valsForArgsTest = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 ,15 ,16 ,17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32];
frisby.create('Put [1, .. , 32] into the variable args for node 1')
.put('http://localhost:' + port + '/nodes/1/variables/args', valsForArgsTest, {json: true})
.expectStatus(204)
.after(function(err, res, body) {
	frisby.create('Get the variable args from node 1')
	.get('http://localhost:' + port + '/nodes/1/variables/args')
	.expectStatus(200)
	.expectJSON(valsForArgsTest)
	.toss();
})
.toss();

frisby.create('Fail to put too long vector of length 33 into the variable args for node 1')
.put('http://localhost:' + port + '/nodes/1/variables/args', valsForArgsTest.concat([33]), {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to put wrong type 1 into the variable args for node 1')
.put('http://localhost:' + port + '/nodes/1/variables/args', ['toto'], {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to put wrong type 2 into the variable args for node 1')
.put('http://localhost:' + port + '/nodes/1/variables/args', [ [0] ], {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to put wrong type 3 into the variable args for node 1')
.put('http://localhost:' + port + '/nodes/1/variables/args', {}, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to put out of bound value 32768 into the variable args for node 1')
.put('http://localhost:' + port + '/nodes/1/variables/args', [32768], {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to put out of bound value -32769 into the variable args for node 1')
.put('http://localhost:' + port + '/nodes/1/variables/args', [-32769], {json: true})
.expectStatus(400)
.toss();

// program

frisby.create('Fail to compile an erroneous program')
.put('http://localhost:' + port + '/nodes/1/program', new Buffer('var_typo a'), {json: false, headers: {'content-type': 'text/plain'}})
.expectStatus(400)
.expectJSON({
	success: false
})
.toss();

frisby.create('Compile a valid program program on node 1')
.put('http://localhost:' + port + '/nodes/1/program', new Buffer('var a = 0\nvar b = 0\nvar c = a + b'), {json: false, headers: {'content-type': 'text/plain'}})
.expectStatus(200)
.expectJSON({
	success: true
})
.after(function(err, res, body) {
	frisby.create('Get variable a from node 1')
	.get('http://localhost:' + port + '/nodes/1/variables/a')
	.expectStatus(200)
	.expectHeader('Content-Type', 'application/json')
	.toss();
})
.toss();
