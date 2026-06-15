#include <string>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cctype>
#include <cerrno>




enum class ParseStatus{
    Completed,
    Incompleted,
    Error
};

enum class ParseErrorType {
    NoError,

    InvalidRequestLine,    // 请求行格式错误
    UnsupportedMethod,     // 方法不支持
    InvalidPath,           // path 格式错误
    InvalidVersion,        // HTTP 版本错误

    InvalidHeader,         // Header 行格式错误
    InvalidContentLength,  // Content-Length 不是合法数字
    BodyTooLarge           // body 太大，拒绝
};


struct ParseResult{
    ParseStatus status;
    ParseErrorType type;
    std::string message;
};

struct HttpRequest{
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> header;
    std::unordered_map<std::string, std::string> query;
    std::string body;
};
using handler = std::function<ParseResult(const std::string& raw,HttpRequest& request)>;

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
    request.path = path;
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
            request.query.insert(key, value);
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

}

bool parse_version(const std::string verison, HttpRequest& request){
    if(verison != "HTTP/1.1"){
        return false;
    }
}

ParseResult handler_get(const std::string raw, HttpRequest& request){
    std::size_t header_end = raw.find("\r\n\r\n");
    if(header_end == std::string::npos || raw.substr(raw.size() - 2) != "\r\n"){
        return {ParseStatus::Incompleted, ParseErrorType::NoError, "GET 请求不完整"};
    }

    std::size_t pos = raw.find("\r\n", 0);
    const std::string line = raw.substr(pos);
    std::size_t line_pos = line.find(" ");
    if(line_pos == std::string::npos || line_pos + 1 >= line.size() || line[line_pos + 1] != '/'){
        return {ParseStatus::Error, ParseErrorType::InvalidHeader, "无效请求行"};
    }

    request.method = line.substr(0, line_pos);
    if(request.method != "GET"){
        return {ParseStatus::Error, ParseErrorType::UnsupportedMethod, "使用方法不支持"};
    }
    line_pos++;
    std::size_t line_pos1 = line.find(" ");
    if(line_pos1 == std::string::npos || line_pos1 + 1 >= line.size() || line[line_pos1 + 1] != 'H'){
        return {ParseStatus::Error, ParseErrorType::InvalidHeader, "无效请求行"};
    }

    const std::string path = line.substr(line_pos1 - line_pos, line_pos);
    if(!parse_path(path, request)){
        return {ParseStatus::Error, ParseErrorType::InvalidPath, "请求路径不合规" };
    }
    line_pos1++;
    const std::string version = line.substr(line_pos1, line.size() - line_pos1);
    if(!parse_version(version, request)){
        return {ParseStatus::Error, ParseErrorType::InvalidVersion, version + "版本不支持"};
    }

    std::size_t start = pos + 2;
    for(std::size_t end = raw.find("\r\n", start); end - 1 < header_end; start = end + 2, end = raw.find("\r\n", start)){
        const std::string line = raw.substr(start, end - start);
        std::size_t mid = line.find(':');
        if(mid == std::string::npos){
            return {ParseStatus::Error, ParseErrorType::InvalidHeader, "请求头格错误，缺少冒号分割"};
        }
        std::string name = raw.substr(start, mid - start);
        if(name.find('\r') != std::string::npos || 
        name.find('\n') != std::string::npos || name.find(' ') != std::string::npos){
            return {ParseStatus::Error, ParseErrorType::InvalidHeader, "name 中包含特殊字符"};
        }
        mid++;
        while(mid < end && raw[mid] == ' '){
            mid++;
        }
        const std::string value = raw.substr(mid,  end - start);
        to_lower(name);
        request.header.insert(name, value);
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
        unsigned long long length = strtoull(value.data(), &end, 0);
        if(errno > 0 || *end != '\0' || length > max_length){
            return {ParseStatus::Error, ParseErrorType::InvalidHeader, "请求头中content-length格式错误"};
        }
        std::size_t body_start = header_end + 4;
        std::size_t body_end = raw.size() - 2;
        if(body_end - body_start < length){
            return {ParseStatus::Incompleted, ParseErrorType::NoError, "GET 请求体不完整"};
        }
        request.body = raw.substr(body_start, body_end - body_start);
    }
    return {ParseStatus::Completed, ParseErrorType::NoError, "Success"};
}

std::unordered_map<std::string, handler> handlers = {
    {"GET", handler_get}
};

ParseResult parse_http_request(const std::string& raw, HttpRequest& request){
    std::string method = "";
    for(std::size_t i = 0;i < raw.size();i++){
        if(raw[i] == ' '){
            method = raw.substr(0, i);
        }else if(i > 6){
            break;
        }
    }
    auto it = handlers.find(method);
    if(it == handlers.end()){
        return {ParseStatus::Error, ParseErrorType::UnsupportedMethod, "暂不支持" + method + "方法"};
    }
    return it->second(raw, request);
}







