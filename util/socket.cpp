#include "socket.hpp"

#include <cstdio>
#include <sys/socket.h>

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
