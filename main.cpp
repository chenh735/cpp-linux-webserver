#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <errno.h>
#include <iostream>



int main(int argc, char* argv[]){
    long port = 8080;
    if(argc == 2){
        errno = 0;
        char* end = nullptr;
        port = strtol(argv[1], &end, 0);
        if(errno !=0 || *end != '\0' || port < 0 || port > 65535){
            perror("port 格式错误\n");
            return 1;
        }
    }


    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == fd){
        perror("socket 创建失败\n");
        return 1;
    }

    int opt = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        close(fd);
        perror("setsockopt 失败\n");
        return 1;
    }
    
    sockaddr_in in_addr{};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(8080);
    in_addr.sin_addr.s_addr = INADDR_ANY;

    if(-1 == bind(fd, (sockaddr*)(&in_addr), sizeof(in_addr))){
        perror("bind 创建失败\n");
        close(fd);
        return -1;
    }

    if(-1 == listen(fd, SOMAXCONN)){
        perror("listen 失败\n");
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
            perror("revc 错误\n");
            break;
        }
        if(n == 0){
            close(client_fd);
            perror("客户端关闭连接\n");
            continue;
        }
        if(n > 0){
            buf[n] = '\0';
            std::cout << buf << std::endl;
        }

        const char* response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 12\r\n"
            "Connect: close\r\n"
            "\r\n"
            "Hello World\n";
        
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
                perror("客户端关闭连接");
                continue;
            }
            send_total += n;
        }
        close(client_fd);
    }
    
    close(fd);
    std::cout << "success" << std::endl;

    return 0;
}