var frisby = require('frisby');
var port = 3000

frisby.create('Get API DOCS')
.get('http://localhost:' + port + '/apidocs')
.expectStatus(200)
.expectHeader('Content-Type', 'application/json')
.expectJSONTypes({})
.toss();


