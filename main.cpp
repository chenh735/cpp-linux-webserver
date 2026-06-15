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

        char buf[1024];
        ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if(n == -1){
            close(client_fd);
            perror("revc 错误");
            continue;
        }
        if(n == 0){
            close(client_fd);
            continue;
        }
        if(n > 0){
            buf[n] = '\0';
        }

        const std::string raw(buf, n);
        HttpRequest request;
        ParseResult result = parse_http_request(raw, request);
        std::string path_prefix = "static/";
        HttpResponse resp;
        switch (result.type)
        {
        case ParseErrorType::NoError:{
            if(request.path == "/"){
                request.path = "/index";
            }
            if(request.path == "/index"){
                std::string path = path_prefix + "index.html";
                std::ifstream file(path, std::ios::binary);
                if(!file.is_open()){
                    perror((path + "文件不能打开").c_str());
                    close(client_fd);
                    continue;
                }
                std::stringstream buf;
                buf << file.rdbuf();
                std::string body = buf.str();
                std::unordered_map<std::string, std::string> header;
                header["Content-Length"] = std::to_string(body.size());
                header["Connection"] = "close";
                header["Content-Type"] = "text/html; charset=utf-8";
                resp.body = body;
                resp.status = 200;
                resp.message = status_to_msg[resp.status];
                resp.header = header;
            }else{
                std::string path = path + "/400BadRequest.html";
                std::ifstream file(path, std::ios::binary);
                if(!file.is_open()){
                    perror((path + "文件不能打开").c_str());
                    close(client_fd);
                    continue;
                }
                std::stringstream buf;
                buf << file.rdbuf();
                std::string body = buf.str();
                std::unordered_map<std::string, std::string> header;
                header["Content-Length"] = std::to_string(body.size());
                header["Connection"] = "close";
                header["Content-Type"] = "text/html; charset=utf-8";
                resp.body = body;
                resp.status = 400;
                resp.message = status_to_msg[resp.status];
                resp.header = header;
            }
            break;
        }
        case ParseErrorType::UnsupportedMethod:{
            std::string path = path_prefix + "/405MethodNotAllowed.html";
            std::ifstream file(path, std::ios::binary);
            if(!file.is_open()){
                perror((path + "文件不能打开").c_str());
                close(client_fd);
                continue;
            }
            std::stringstream buf;
            buf << file.rdbuf();
            std::string body = buf.str();
            std::unordered_map<std::string, std::string> header;
            header["Content-Length"] = std::to_string(body.size());
            header["Connection"] = "close";
            header["Content-Type"] = "text/html; charset=utf-8";
            resp.body = body;
            resp.status = 405;
            resp.message = status_to_msg[resp.status];
            resp.header = header;
            break;
        }
        default:{
            std::string path = path_prefix + "/400BadRequest.html";
            std::ifstream file(path, std::ios::binary);
            if(!file.is_open()){
                perror((path + "文件不能打开").c_str());
                close(client_fd);
                continue;
            }
            std::stringstream buf;
            buf << file.rdbuf();
            std::string body = buf.str();
            std::unordered_map<std::string, std::string> header;
            header["Content-Length"] = std::to_string(body.size());
            header["Connection"] = "close";
            header["Content-Type"] = "text/html; charset=utf-8";
            resp.body = body;
            resp.status = 400;
            resp.message = status_to_msg[resp.status];
            resp.header = header;
            break;
        }
        }
        std::string r = resp.builder();
        const char* response = r.data();
        ssize_t total = strlen(response);
        ssize_t send_total = 0;
        while(send_total < total){
            ssize_t n = send(client_fd, (void*)(response + send_total), total - send_total, 0);
            if(n == -1){
                perror("send 失败");
                close(client_fd);
                break;
            }
            if(n == 0){
                close(client_fd);
                break;
            }
            send_total += n;
        }
        close(client_fd);
    }
    
    close(fd);

    return 0;
}
