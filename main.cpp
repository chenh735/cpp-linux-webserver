#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <errno.h>
#include <sstream>
#include <fstream>
#include "http-1.1/response.hpp"
#include "http-1.1/request.hpp"


// 读取本地文件内容到字符串中。
bool read_file(const std::string& path, std::string& body){
    std::ifstream file(path, std::ios::binary);
    if(!file.is_open()){
        return false;
    }

    std::stringstream buf;
    buf << file.rdbuf();
    body = buf.str();
    return true;
}

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

// 循环发送响应，直到全部内容写入 socket。
bool send_all(int client_fd, const std::string& response){
    const char* data = response.data();
    ssize_t total = response.size();
    ssize_t send_total = 0;
    while(send_total < total){
        ssize_t n = send(client_fd, (void*)(data + send_total), total - send_total, 0);
        if(n == -1){
            perror("send 失败");
            return false;
        }
        if(n == 0){
            return false;
        }
        send_total += n;
    }
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

// 处理单个客户端连接的完整请求响应流程。
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
}


int main(int argc, char* argv[]){
    long port = 8080;
    if(argc == 2){
        errno = 0;
        char* end = nullptr;
        port = strtol(argv[1], &end, 0);
        if(errno !=0 || *end != '\0' || port <= 0 || port > 65535){
            perror("port 格式错误");
            return 1;
        }
    }


    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == fd){
        perror("socket 创建失败");
        return 1;
    }

    int opt = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        close(fd);
        perror("setsockopt 失败");
        return 1;
    }
    
    sockaddr_in in_addr{};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(static_cast<uint16_t>(port));
    in_addr.sin_addr.s_addr = INADDR_ANY;

    if(-1 == bind(fd, (sockaddr*)(&in_addr), sizeof(in_addr))){
        perror("bind 创建失败");
        close(fd);
        return -1;
    }

    if(-1 == listen(fd, SOMAXCONN)){
        perror("listen 失败");
        close(fd);
        return -1;
    }

    while (true)
    {
        int client_fd = accept(fd, nullptr, NULL);
        if(client_fd == -1){
            sleep(1);
            continue;
        }

        handle_client(client_fd);
        close(client_fd);
    }
    
    close(fd);

    return 0;
}
