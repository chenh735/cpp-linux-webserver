#include "server.hpp"

#include <cstdint>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../connection/http_connection.hpp"

int run_server(long port){
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
        int client_fd = accept(fd, nullptr, nullptr);
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
