#pragma once

#include <string>
#include <unordered_map>

enum class ParseStatus{
    Completed,
    Incompleted,
    Error
};

enum class ParseErrorType {
    NoError,

    InvalidRequestLine,    // 请求行格式错误
    UnsupportedMethod,     // 方法不支持
    InvalidPath,           // path 格式错误
    InvalidVersion,        // HTTP 版本错误

    InvalidHeader,         // Header 行格式错误
    InvalidContentLength,  // Content-Length 不是合法数字
    BodyTooLarge           // body 太大，拒绝
};

struct ParseResult{
    ParseStatus status;
    ParseErrorType type;
    std::string message;
};

struct HttpRequest{
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> header;
    std::unordered_map<std::string, std::string> query;
    std::string body;
};

ParseResult parse_http_request(const std::string& raw, HttpRequest& request);
