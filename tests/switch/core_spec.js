// this test only requires the switch

var frisby = require('frisby');
var port = 3000

frisby.create('Verify no root endpoint')
.get('http://localhost:' + port + '/')
.expectStatus(404)
.toss();

