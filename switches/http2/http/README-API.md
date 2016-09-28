The asebaswitch REST API provides access to the core functions in asebacommon and asebacompiler for managing
robot targets and their programs. The current version is [aseba-v1.json](aseba-v1.json).

The API is specified using the [OAI OpenAPI Specification](https://github.com/OAI/OpenAPI-Specification). The
specification, in JSON or equivalently YAML, can be edited using [Swagger Editor](http://swagger.io/swagger-editor/)
or for example [Stoplight.io](http://stoplight.io). It can be explored and tested using an OAS documentation UI
such as [swagger-ui](http://swagger.io/swagger-ui/), [ReDoc](https://github.com/Rebilly/ReDoc/) or through the
hosted documentation at https://aseba.api-docs.io/v1. See http://swagger.io/open-source-integrations/ for
tools and open source integrations.

C++ stubs for HTTP handlers can be generated here from the OAS apidocs using
```
./apidoc-repair.perl aseba-v1.json | ./apidoc-to-stubs.perl > stubs.cpp
```

The program `apidoc-repair.perl` removes optional elements such as examples and moves response descriptions 
temporarily attached to the schema instead of the response.
The program `apidoc-to-stubs.perl` maps the corrected endpoints to asebaswitch HTTP handlers.
