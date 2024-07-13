#pragma once

namespace fhttp {
    // Well known http headers
    inline static constexpr const char* HEADER_COOKIES = "Cookies";
    inline static constexpr const char* HEADER_CONTENT_TYPE = "Content-Type";
    inline static constexpr const char* HEADER_CONTENT_LENGTH = "Content-Length";
    inline static constexpr const char* HEADER_SET_COOKIE = "Set-Cookie";
    inline static constexpr const char* HEADER_LOCATION = "Location";
    inline static constexpr const char* HEADER_USER_AGENT = "User-Agent";
    inline static constexpr const char* HEADER_ACCEPT = "Accept";
    inline static constexpr const char* HEADER_ACCEPT_LANGUAGE = "Accept-Language";
    inline static constexpr const char* HEADER_ACCEPT_ENCODING = "Accept-Encoding";
    inline static constexpr const char* HEADER_ACCEPT_CHARSET = "Accept-Charset";
    inline static constexpr const char* HEADER_HOST = "Host";
    inline static constexpr const char* HEADER_CONNECTION = "Connection";
    inline static constexpr const char* HEADER_CACHE_CONTROL = "Cache-Control";
    inline static constexpr const char* HEADER_ORIGIN = "Origin";
    inline static constexpr const char* HEADER_REFERER = "Referer";
}