#include "file.hpp"

#include <fstream>
#include <sstream>

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
