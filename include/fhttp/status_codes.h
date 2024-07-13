#pragma once

namespace fhttp {
    // Status codes
    inline constexpr int STATUS_CODE_OK = 200;
    inline constexpr int STATUS_CODE_CREATED = 201;
    inline constexpr int STATUS_CODE_ACCEPTED = 202;    
    inline constexpr int STATUS_CODE_NO_CONTENT = 204;
    inline constexpr int STATUS_CODE_MOVED_PERMANENTLY = 301;
    inline constexpr int STATUS_CODE_FOUND = 302;
    inline constexpr int STATUS_CODE_BAD_REQUEST = 400;
    inline constexpr int STATUS_CODE_UNAUTHORIZED = 401;
    inline constexpr int STATUS_CODE_FORBIDDEN = 403;
    inline constexpr int STATUS_CODE_NOT_FOUND = 404;
    inline constexpr int STATUS_CODE_METHOD_NOT_ALLOWED = 405;
    inline constexpr int STATUS_CODE_INTERNAL_SERVER_ERROR = 500;
    inline constexpr int STATUS_CODE_NOT_IMPLEMENTED = 501;
    inline constexpr int STATUS_CODE_SERVICE_UNAVAILABLE = 503;
    inline constexpr int STATUS_CODE_HTTP_VERSION_NOT_SUPPORTED = 505;
    inline constexpr int STATUS_CODE_TOO_MANY_REQUESTS = 429;
    inline constexpr int STATUS_CODE_REQUEST_TIMEOUT = 408;
    inline constexpr int STATUS_CODE_CONFLICT = 409;
    inline constexpr int STATUS_CODE_PRECONDITION_FAILED = 412;
    inline constexpr int STATUS_CODE_PAYLOAD_TOO_LARGE = 413;
    inline constexpr int STATUS_CODE_UNSUPPORTED_MEDIA_TYPE = 415;
    inline constexpr int STATUS_CODE_EXPECTATION_FAILED = 417;
    inline constexpr int STATUS_CODE_UPGRADE_REQUIRED = 426;
    inline constexpr int STATUS_CODE_PRECONDITION_REQUIRED = 428;
    inline constexpr int STATUS_CODE_TOO_EARLY = 425;
    inline constexpr int STATUS_CODE_UNPROCESSABLE_ENTITY = 422;
    inline constexpr int STATUS_CODE_LOCKED = 423;
    inline constexpr int STATUS_CODE_FAILED_DEPENDENCY = 424;
}