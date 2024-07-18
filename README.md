# F(ast)HTTP
`fhttp` is as library that provides fairly simple interface for building HTTP based servers, it provides "auto" json de/serialization, swagger generation and much more!

For examples check out [basic_http_server](examples/basic_http_server)

# Notice
`fhttp` is currently mostly just a POC and funny personal project

# Features
- Shared configuration
- Shared state between handlers
- Auto JSON de/serialization
- Auto OpenAPI spec generation
- Graceful shutdown
- Regex pattern within URLs
- Currently supports only HTTP version 1.*
- Keep-alive Timeout
- Middlewares using handler base classes that modify `evaluate_request`

# todo
- tests
- add list/optional/union support to data module & swagger
- proper typed query params parsing (add it to swagger generation)
- compression (for now gzip/deflate)
- try out some basic implementation of websockets (https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers)
- cleanup unused code

# Development
- To compile this project you need to have installed boost with `filesystem regex thread chrono date_time json` libs

```sh
mkdir build
cd build
cmake .. && make
```