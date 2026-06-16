#pragma once

#include <string>
#include <unordered_map> 

extern std::unordered_map<int, std::string> status_to_msg;

struct HttpResponse{
    int status;
    std::string message;
    std::unordered_map<std::string, std::string> header;
    std::string body;

    HttpResponse();

    HttpResponse(int resp_status, const std::string& resp_message,
    const std::unordered_map<std::string, std::string>& resp_header, const std::string& resp_body);

    std::string builder();
};
