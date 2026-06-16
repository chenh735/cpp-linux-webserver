# TODO：HTTP/1.1 解析与静态文件阶段

## 当前阶段总结

当前项目已经从最小 HTTP 服务器推进到 HTTP/1.1 协议解析阶段。

已完成：

- [x] 使用 `http-1.1/request.hpp` 定义 `HttpRequest`、`ParseResult` 和解析入口。
- [x] 初步解析请求方法、路径、HTTP 版本。
- [x] 初步解析 Header。
- [x] 初步处理 query 和 body。
- [x] 使用 `http-1.1/response.hpp` 定义 `HttpResponse` 响应构造逻辑。
- [x] 添加 `static/` 静态页面目录。
- [x] 删除 `main.cpp` 和 `http-1.1` 中用于调试的 `cout`。

当前阶段的重点已经不是继续堆功能，而是先把 HTTP 解析、静态文件返回和工程结构打稳。

## 优先修复

- [x] 修复非 `/index` 路径分支的文件路径构造。
  - 位置：`main.cpp` 中 `std::string path = path + "/400BadRequest.html";`
  - 问题：局部变量在初始化时引用自身，逻辑错误。
  - 建议：如果请求文件不存在，应返回 `static/404NotFound.html`，不是 `400BadRequest.html`。

- [x] 区分 `ParseStatus` 和 `ParseErrorType`。
  - 位置：`main.cpp` 中 `switch (result.type)`。
  - 问题：当前只根据错误类型分支，没有先判断 `Completed/Incompleted/Error`。
  - 风险：请求不完整时 `type` 仍可能是 `NoError`，容易被当成成功请求处理。

- [x] 修复静态文件路径映射。
  - 当前只支持 `/` 和 `/index`。
  - 需要支持 `/index.html`。
  - 后续再支持更多 `static/` 下的普通文件。
  - 路径中包含 `..` 时应直接返回 `400 Bad Request`，避免目录穿越。

- [x] 修复 query 解析逻辑。
  - 位置：`request.hpp` 的 `parse_path`。
  - 问题：`request.path` 当前保存的是带 query 的原始 path，而不是去掉 `?` 后的路径。
  - 问题：query 循环中后续查找分隔符时使用了 `query.find('=', start)`，不符合按 `&` 分割参数的目标。
  - 建议：先把 path 和 query 分离，再单独解析 `a=b&c=d`。

- [x] 修复 Header value 截取逻辑。
  - 位置：`request.hpp` 中解析 Header 的循环。
  - 问题：`mid` 是当前 header 行内的下标，但代码中用 `raw[mid]` 和 `raw.substr(mid, ...)`，容易错用全局 raw 下标。
  - 建议：Header 行已经放在 `line` 变量中，后续 trim 和 substr 都应基于 `line`。

- [x] 修复 body 长度判断。
  - 当前用 `raw.size() - 2` 作为 body 结束位置。
  - HTTP body 不要求以 `\r\n` 结束。
  - 建议：body 起点为 `header_end + 4`，后续根据 `Content-Length` 精确读取。

- [x] 修复发送长度计算。
  - 位置：`main.cpp` 中 `strlen(response)`。
  - 问题：`response` 来自 `std::string`，以后返回二进制文件时可能包含 `\0`。
  - 建议：使用 `r.size()` 作为发送长度。

- [x] 避免重复关闭 `client_fd`。
  - 当前 `send` 出错或返回 0 时在循环内 `close(client_fd)`，循环外还会再次 `close(client_fd)`。
  - 建议：统一在单一出口关闭，或者用标志位避免 double close。

## 工程结构优化

- [x] 调整 Makefile。
  - 当前 `SRC` 中包含 `.hpp` 文件。
  - 建议只编译 `.cpp`：`main.cpp http-1.1/request.cpp http-1.1/response.cpp`。
  - Header 由 `#include` 引入，不作为独立编译单元。

- [x] 给头文件添加 include guard 或 `#pragma once`。
  - 涉及：`request.hpp`、`response.hpp`。
  - 目的：避免重复包含。

- [x] 将头文件中的函数定义迁移到 `.cpp`。
  - 当前 `request.hpp` 中包含大量函数实现。
  - 当前 `response.hpp` 中定义了全局变量 `status_to_msg`。
  - 后续多个 `.cpp` 同时 include 时，容易出现重复定义问题。

- [x] 把重复的响应构造逻辑提取成函数。
  - 当前 `main.cpp` 中读取 HTML 文件、设置 Header、设置状态码的逻辑重复较多。
  - 可先提取：
    - `read_file(path)`
    - `build_html_response(status, file_path)`
    - `send_all(client_fd, response)`

- [x] 把单个连接处理逻辑提取成 `handle_client(int client_fd)`。
  - 让主循环只保留 `accept -> handle_client -> close` 的主流程。
  - 为后续 `HttpConnection` 类做准备。

## 功能完善

- [x] 完成静态文件返回。
  - `/` -> `static/index.html`
  - `/index.html` -> `static/index.html`
  - 文件存在 -> `200 OK`
  - 文件不存在 -> `404 Not Found`

- [x] 补齐错误响应。
  - 请求格式错误 -> `400 Bad Request`
  - 方法不支持 -> `405 Method Not Allowed`
  - 文件打开失败或服务器内部错误 -> `500 Internal Server Error`

- [x] 添加最小手动测试清单。
  - `curl -v http://127.0.0.1:8080/`
  - `curl -v http://127.0.0.1:8080/index.html`
  - `curl -v http://127.0.0.1:8080/not-found.html`
  - `curl -X POST -v http://127.0.0.1:8080/`
  - `printf 'BAD\\r\\n\\r\\n' | nc 127.0.0.1 8080`

## 参考 TinyWebServer 的后续路线

TinyWebServer 的主要学习点包括：线程池、非阻塞 socket、`epoll`、Reactor/Proactor、HTTP 状态机、日志、定时器、数据库连接池和压测。

当前项目不建议马上进入数据库和登录注册。更合适的顺序是：

- [ ] 第一阶段：HTTP 解析和静态资源返回稳定。
- [ ] 第二阶段：抽取连接处理模块，形成 `HttpConnection` 雏形。
- [ ] 第三阶段：使用非阻塞 socket。
- [ ] 第四阶段：引入 `epoll`，先 LT 模式。
- [ ] 第五阶段：增加定时器，关闭空闲连接。
- [ ] 第六阶段：增加同步日志。
- [ ] 第七阶段：增加线程池。
- [ ] 第八阶段：再做压测和性能优化。

参考仓库：https://github.com/qinguoyi/TinyWebServer
