# F(ast)HTTP
`fhttp` is as library that provides fairly simple interface for building HTTP based servers, it provides "auto" json de/serialization, swagger generation and much more!

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

# todo
- tests
- middlewares
- configurable `Server` header
- compression (for now gzip/deflate)
- try out some basic implementation of websockets (https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers)
- cleanup unused code