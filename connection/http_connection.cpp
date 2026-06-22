#include "http_connection.hpp"

#include <cstdio>
#include <string>
#include <unordered_map>
#include <sys/socket.h>
#include <unistd.h>

#include "../http-1.1/request.hpp"
#include "../http-1.1/response.hpp"
#include "../util/file.hpp"
#include "../util/socket.hpp"

namespace {

// 根据状态码和 HTML 文件构造完整 HTTP 响应。
bool build_html_response(int status, const std::string& file_path, HttpResponse& resp){
    std::string body;
    if(!read_file(file_path, body)){
        return false;
    }

    std::unordered_map<std::string, std::string> header;
    header["Content-Length"] = std::to_string(body.size());
    header["Connection"] = "close";
    header["Content-Type"] = "text/html; charset=utf-8";
    resp.body = body;
    resp.status = status;
    resp.message = status_to_msg[resp.status];
    resp.header = header;
    return true;
}

// 阻塞读取一个完整 HTTP 请求，直到解析完成或发生错误。
bool read_http_request(int client_fd, std::string& raw_request, HttpRequest& request, ParseResult& result){
    constexpr std::size_t max_request_size = 1024 * 1024;
    char buf[1024];

    while(true){
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
        if(n == -1){
            perror("recv 错误");
            return false;
        }
        if(n == 0){
            return false;
        }

        raw_request.append(buf, n);
        if(raw_request.size() > max_request_size){
            result = {ParseStatus::Error, ParseErrorType::BodyTooLarge, "请求数据过大"};
            return true;
        }

        HttpRequest parsed_request;
        result = parse_http_request(raw_request, parsed_request);
        if(result.status == ParseStatus::Completed || result.status == ParseStatus::Error){
            request = parsed_request;
            return true;
        }
    }
}

// 根据状态码构造错误页面响应。
bool build_error_response(int status, HttpResponse& resp){
    std::string path_prefix = "static/";
    std::string path = path_prefix + "400BadRequest.html";

    if(status == 404){
        path = path_prefix + "404NotFound.html";
    }else if(status == 405){
        path = path_prefix + "405MethodNotAllowed.html";
    }

    if(!build_html_response(status, path, resp)){
        perror((path + "文件不能打开").c_str());
        return false;
    }

    return true;
}

// 根据请求路径构造静态资源响应。
bool resolve_static_response(const HttpRequest& request, HttpResponse& resp){
    std::string path_prefix = "static/";
    std::string request_path = request.path;

    if(request_path == "/"){
        request_path = "/index";
    }

    if(request_path == "/index" || request_path == "/index.html"){
        std::string path = path_prefix + "index.html";
        if(!build_html_response(200, path, resp)){
            perror((path + "文件不能打开").c_str());
            return false;
        }
        return true;
    }

    return build_error_response(400, resp);
}

// 根据解析结果分发到正常响应或错误响应。
bool build_response_from_request(const ParseResult& result, const HttpRequest& request, HttpResponse& resp){
    switch (result.status)
    {
    case ParseStatus::Completed:{
        if(result.type == ParseErrorType::NoError){
            return resolve_static_response(request, resp);
        }
        return build_error_response(400, resp);
    }
    case ParseStatus::Incompleted:{
        return build_error_response(400, resp);
    }
    case ParseStatus::Error:{
        if(result.type == ParseErrorType::UnsupportedMethod){
            return build_error_response(405, resp);
        }
        return build_error_response(400, resp);
    }
    default:{
        return build_error_response(400, resp);
    }
    }
}

}

void handle_client(int client_fd){
    std::string raw_request;
    HttpRequest request;
    ParseResult result;
    HttpResponse resp;

    if(!read_http_request(client_fd, raw_request, request, result)){
        return;
    }

    if(!build_response_from_request(result, request, resp)){
        return;
    }

    std::string r = resp.builder();
    send_all(client_fd, r);
    close(client_fd);
}
