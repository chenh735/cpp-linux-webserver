#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include "server/server.hpp"

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

    return run_server(port);
}
