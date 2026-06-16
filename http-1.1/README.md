# http-1.1 模块说明

这个目录用于存放当前项目的 HTTP/1.1 请求解析和响应构造代码。

## 文件职责

- `request.hpp`：定义 `HttpRequest`、`ParseResult`、`ParseStatus`、`ParseErrorType`，并声明 `parse_http_request`。
- `request.cpp`：实现 HTTP 请求行、Header、query 和 body 的解析逻辑。
- `response.hpp`：声明 `HttpResponse` 和状态码到原因短语的映射表。
- `response.cpp`：定义状态码映射表，并实现 `HttpResponse::builder()`。

## 当前能力

当前模块主要服务于学习阶段的阻塞式服务器：

- 支持基本 HTTP/1.1 请求解析。
- 支持 GET 请求处理入口。
- 能区分解析完成、请求不完整和解析错误。
- 能构造带状态行、响应头、空行和响应体的 HTTP 响应。

## 当前边界

这个模块还不是完整 HTTP 协议库：

- 暂未支持 chunked body。
- 暂未支持 keep-alive 连接复用。
- 暂未实现完整 MIME 类型判断。
- 请求不完整时还需要配合后续 Buffer/HttpConnection 继续读取。

后续可以在保持该模块职责清晰的前提下，把连接读写状态交给 `HttpConnection`，让 `http-1.1` 目录继续专注于协议解析和响应构造。
