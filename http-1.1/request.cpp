#include "request.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>

namespace {

void to_lower(std::string& s){
    std::transform(s.begin(), s.end(),s.begin(), [](unsigned char c){
        return std::tolower(c);
    });
}

bool parse_path(const std::string& path, HttpRequest& request){
    const std::size_t npos = std::string::npos;
    std::size_t pos = path.find('?');
    const std::string ret_path = path.substr(0, pos);
    if(ret_path.empty() || ret_path[0] != '/' || path.size() > 2048 || path.find(' ') != npos
    || path.find('\r') != npos || path.find('\n') != npos ){
        return false;
    }
    request.path = path.substr(0, pos);
    if(pos != npos){
        std::string query = path.substr(pos + 1);
        if(query.size() > 2048 || query.find(' ') != npos || query.find('\r') != npos
        || query.find('\n') != npos || query.find('?') != npos){
            return false;
        }

        std::size_t start = 0;
        std::size_t end = query.find('&');
        bool first = true;
        while(end != npos){
            std::size_t mid = start;
            while(mid < end && query[mid] != '='){
                mid++;
            }
            
            if(mid == start){
                return false;
            }

            std::string key = query.substr(start, mid - start);
            if(mid == end){
                mid--;
            }

            std::string value = query.substr(mid + 1, end - mid);
            request.query.emplace(key, value);
            start = end + 1;
            end = query.find('=', start);
            if(end == npos){
                if(first){
                    end = query.size();
                    first = false;
                }
            }
        }
    }
    return true;
}

bool parse_version(const std::string& verison){
    if(verison != "HTTP/1.1"){
        return false;
    }
    return true;
}

bool is_supported_method(const std::string& method){
    return method == "GET";
}

ParseResult parse_http_message(const std::string& raw, HttpRequest& request){
    std::size_t header_end = raw.find("\r\n\r\n");
    if(header_end == std::string::npos){
        return {ParseStatus::Incompleted, ParseErrorType::NoError, "HTTP 请求头不完整"};
    }

    std::size_t pos = raw.find("\r\n", 0);
    if(pos == std::string::npos){
        return {ParseStatus::Incompleted, ParseErrorType::NoError, "HTTP 请求行不完整"};
    }

    const std::string line = raw.substr(0, pos);
    std::size_t line_pos = line.find(' ');
    if(line_pos == std::string::npos || line_pos + 1 >= line.size() || line[line_pos + 1] != '/'){
        return {ParseStatus::Error, ParseErrorType::InvalidRequestLine, "无效请求行1"};
    }

    request.method = line.substr(0, line_pos);
    line_pos++;
    std::size_t line_pos1 = line.find(' ', line_pos);
    if(line_pos1 == std::string::npos || line_pos1 + 1 >= line.size() || line[line_pos1 + 1] != 'H'){
        return {ParseStatus::Error, ParseErrorType::InvalidRequestLine, "无效请求行2"};
    }

    const std::string path = line.substr(line_pos, line_pos1 - line_pos);
    if(!parse_path(path, request)){
        return {ParseStatus::Error, ParseErrorType::InvalidPath, "请求路径不合规" };
    }
    line_pos1++;
    const std::string version = line.substr(line_pos1, line.size() - line_pos1);
    if(!parse_version(version)){
        return {ParseStatus::Error, ParseErrorType::InvalidVersion, version + "版本不支持"};
    }

    std::size_t start = pos + 2;
    while(start < header_end){
        std::size_t end = raw.find("\r\n", start);
        if(end == std::string::npos || end > header_end){
            return {ParseStatus::Incompleted, ParseErrorType::NoError, "HTTP 请求头不完整"};
        }

        const std::string line = raw.substr(start, end - start);
        std::size_t mid = line.find(':');
        
        if(mid == std::string::npos){
            return {ParseStatus::Error, ParseErrorType::InvalidHeader, "请求头格错误，缺少冒号分割"};
        }
        std::string name = line.substr(0, mid);
        if(name.find('\r') != std::string::npos || 
        name.find('\n') != std::string::npos || name.find(' ') != std::string::npos){
            return {ParseStatus::Error, ParseErrorType::InvalidHeader, "name 中包含特殊字符"};
        }
        mid++;
        while(mid < line.size() && line[mid] == ' '){
            mid++;
        }
        const std::string value = line.substr(mid);
        to_lower(name);
        request.header.emplace(name, value);

        start = end + 2;
    }
    if(request.header.find("host") == request.header.end()){
        return {ParseStatus::Error, ParseErrorType::InvalidHeader, "请求头没有包含 host 信息"};
    }

    auto it = request.header.find("content-length");
    if(it != request.header.end()){
        constexpr unsigned long long max_length = 1024 * 1024;
        std::string value = it->second;
        errno = 0;
        char* end = nullptr;
        unsigned long long length = strtoull(value.c_str(), &end, 0);
        if(errno > 0 || *end != '\0' || length > max_length){
            return {ParseStatus::Error, ParseErrorType::InvalidContentLength, "请求头中content-length格式错误"};
        }
        std::size_t body_start = header_end + 4;
        if(raw.size() - body_start < length){
            return {ParseStatus::Incompleted, ParseErrorType::NoError, "HTTP 请求体不完整"};
        }
        request.body = raw.substr(body_start, length);
    }
    return {ParseStatus::Completed, ParseErrorType::NoError, "Success"};
}

}

ParseResult parse_http_request(const std::string& raw, HttpRequest& request){
    ParseResult result = parse_http_message(raw, request);
    if(result.status != ParseStatus::Completed){
        return result;
    }

    if(!is_supported_method(request.method)){
        return {ParseStatus::Error, ParseErrorType::UnsupportedMethod, "暂不支持" + request.method + "方法"};
    }

    return result;
}
