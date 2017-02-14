var frisby = require('frisby');
var port = 3000

frisby.create('Clear all evenst')
.delete('http://localhost:' + port + '/events')
.expectStatus(204)
.toss();

frisby.create('Get an initial list of events')
.get('http://localhost:' + port + '/events')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON([])
.toss();

frisby.create('Fail to get an unexisting event toto')
.get('http://localhost:' + port + '/events/toto')
.expectStatus(404)
.toss();

frisby.create('Create event toto')
.post('http://localhost:' + port + '/events', { name: "toto", size: 0 }, {json: true})
.expectStatus(201)
.expectHeader('Content-Type', 'application/json')
.expectJSON({
	id: 0,
	name: "toto",
	size: 0
})
.toss();

frisby.create('Get event toto')
.get('http://localhost:' + port + '/events/toto')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON({
	id: 0,
	name: "toto",
	size: 0
})
.toss();

frisby.create('Fail to create event toto twice')
.post('http://localhost:' + port + '/events', { name: "toto", size: 1 }, {json: true})
.expectStatus(409)
.toss();

frisby.create('Fail to create event titi with too large size')
.post('http://localhost:' + port + '/events', { name: "titi", size: 259 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to create event titi with negative size')
.post('http://localhost:' + port + '/events', { name: "titi", size: -1 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to create event with empty name')
.post('http://localhost:' + port + '/events', { name: "", size: 0 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to create event starting with a number')
.post('http://localhost:' + port + '/events', { name: "42", size: 0 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to create event starting with a special character')
.post('http://localhost:' + port + '/events', { name: "#", size: 0 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Create event titi')
.post('http://localhost:' + port + '/events', { name: "titi", size: 258 }, {json: true})
.expectStatus(201)
.expectHeader('Content-Type', 'application/json')
.expectJSON({
	id: 1,
	name: "titi",
	size: 258
})
.toss();

frisby.create('Set the size of event titi to 3')
.put('http://localhost:' + port + '/events/titi', { size: 3 }, {json: true})
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON({ id: 1, name: "titi", size: 3 })
.toss();

frisby.create('Fail to set the size of unknown event tutu')
.put('http://localhost:' + port + '/events/tutu', { size: 0 }, {json: true})
.expectStatus(404)
.toss();

frisby.create('Fail to set event titi with too large size')
.put('http://localhost:' + port + '/events/titi', { size: 259 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Fail to set event titi with negative size')
.put('http://localhost:' + port + '/events/titi', { size: -1 }, {json: true})
.expectStatus(400)
.toss();

frisby.create('Get all events after titi was added')
.get('http://localhost:' + port + '/events')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON([
	{ id: 0, name: "toto", size: 0 },
	{ id: 1, name: "titi", size: 3 }
])
.toss();

frisby.create('Delete event toto')
.delete('http://localhost:' + port + '/events/toto')
.expectStatus(204)
.toss();

frisby.create('Fail to delete event toto twice')
.delete('http://localhost:' + port + '/events/toto')
.expectStatus(404)
.toss();

frisby.create('Get all events after toto was deleted')
.get('http://localhost:' + port + '/events')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSON([
	{ id: 0, name: "titi", size: 3 }
])
.toss();

