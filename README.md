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

# todo
- tests
- add list/optional/union support to data module & swagger
- proper typed query params parsing (add it to swagger generation)
- middlewares - probably using something like pre-handle request or wrap request
```cpp
.. handler code. ..
/* Notes:
    - Allows for custom error handling, possibly even caching
    - Allows modifying headers, also reading them before and validating the input headers (tokens and such) !! might interfere with openapi spec generation !!
 */
void evaluate_request(fhttp::context& ctx, fhttp::request<std::string>& req, fhttp::response& res) {
    /* Before handling */

    // check for some headers, you can even prefill your handler fields, with some useful info

    using super = ...;
    super(ctx, req, res);
    
    /* After handling */
    
    // metrics, duration of handling
    // whole evaluate_request call could be wrapped in try/catch and return statuses based on the kinds of exceptions
    // also the evaluation can be skipped in case of ratelimiting atc
}
.. rest of the handler code ..

.. child handler ..
void evaluate_request(fhttp::context& ctx, fhttp::request<std::string>& req, fhttp::response& res) {
    /* another custom logic */

    using super = ...;
    super(ctx, req, res);
    
}
.. rest of the child handler code ..

```
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