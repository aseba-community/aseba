// this test only requires the switch

var frisby = require('frisby');
var port = 3000

frisby.create('Clear all constants')
.delete('http://localhost:' + port + '/constants')
.expectStatus(204)
.toss();

frisby.create('Get an initial list of constants')
.get('http://localhost:' + port + '/constants')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON([])
.toss();

frisby.create('Fail to get an unexisting constant toto')
.get('http://localhost:' + port + '/constants/toto')
.expectStatus(404)
.toss();

frisby.create('Create constant toto')
.post('http://localhost:' + port + '/constants', { name: "toto", value: 0 }, {json: true})
.expectStatus(201)
.expectHeader('Content-Type', 'application/json')
.expectJSON({
	id: 0,
	name: "toto",
	value: 0
})
.toss();

frisby.create('Get constant toto')
.get('http://localhost:' + port + '/constants/toto')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON({
	id: 0,
	name: "toto",
	value: 0
})
.toss();

frisby.create('Fail to create constant toto twice')
.post('http://localhost:' + port + '/constants', { name: "toto", value: 1 }, {json: true})
.expectStatus(409)
.toss();

frisby.create('Fail to create constant titi with too large value')
.post('http://localhost:' + port + '/constants', { name: "titi", value: 32768 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to create constant titi with too small value')
.post('http://localhost:' + port + '/constants', { name: "titi", value: -32769 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to create constant with empty name')
.post('http://localhost:' + port + '/constants', { name: "", value: 0 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to create constant starting with a number')
.post('http://localhost:' + port + '/constants', { name: "42", value: 0 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to create constant starting with a special character')
.post('http://localhost:' + port + '/constants', { name: "#", value: 0 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Create constant titi')
.post('http://localhost:' + port + '/constants', { name: "titi", value: 2 }, {json: true})
.expectStatus(201)
.expectHeader('Content-Type', 'application/json')
.expectJSON({
	id: 1,
	name: "titi",
	value: 2
})
.toss();

frisby.create('Set the value of constant titi to 3')
.put('http://localhost:' + port + '/constants/titi', { value: 3 }, {json: true})
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON({ id: 1, name: "titi", value: 3 })
.toss();

frisby.create('Fail to set the value of unknown constant tutu')
.put('http://localhost:' + port + '/constants/tutu', { value: 0 }, {json: true})
.expectStatus(404)
.toss();

frisby.create('Fail to set constant titi with too large value')
.put('http://localhost:' + port + '/constants/titi', { value: 32768 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to set constant titi with too small value')
.put('http://localhost:' + port + '/constants/titi', { value: -32769 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Get all constants after titi was added')
.get('http://localhost:' + port + '/constants')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON([
	{ id: 0, name: "toto", value: 0 },
	{ id: 1, name: "titi", value: 3 }
])
.toss();

frisby.create('Delete constant toto')
.delete('http://localhost:' + port + '/constants/toto')
.expectStatus(204)
.toss();

frisby.create('Fail to delete constant toto twice')
.delete('http://localhost:' + port + '/constants/toto')
.expectStatus(404)
.toss();

frisby.create('Get all constants after toto was deleted')
.get('http://localhost:' + port + '/constants')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON([
	{ id: 0, name: "titi", value: 3 }
])
.toss();

