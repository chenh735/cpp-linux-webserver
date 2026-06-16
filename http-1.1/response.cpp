#include "response.hpp"

std::unordered_map<int, std::string> status_to_msg{
    {200, "OK"},
    {400, "Bad Request"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {500, "Internal Server Error"}
};

HttpResponse::HttpResponse(){}

HttpResponse::HttpResponse(int resp_status, const std::string& resp_message,
const std::unordered_map<std::string, std::string>& resp_header, const std::string& resp_body)
:status(resp_status), message(resp_message), header(resp_header),body(resp_body){
    if(status_to_msg.find(status) != status_to_msg.end()){
        message = status_to_msg[status];
    }
}

std::string HttpResponse::builder(){
    std::string resp;
    resp += "HTTP/1.1 " + std::to_string(status) + " " + message + "\r\n";
    for(const auto&[key, value]: header){
        resp += key + ": " + value + "\r\n";
    }
    resp += "\r\n" + body + "\r\n";
    return resp;
}
