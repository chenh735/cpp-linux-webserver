# TODO：阻塞式静态服务器到 HttpConnection 阶段

## 当前阶段总结

当前项目已经完成最小阻塞式 HTTP 服务器的主体流程：

- [x] 创建 TCP 监听 socket，支持 `./bin/server 8080` 启动。
- [x] 初步解析 HTTP/1.1 请求行、Header、query 和 body。
- [x] 使用 `HttpRequest` 保存请求数据。
- [x] 使用 `HttpResponse` 构造响应数据。
- [x] 将 `request.hpp`、`response.hpp` 中的实现迁移到 `.cpp`。
- [x] Makefile 只编译 `.cpp` 文件。
- [x] 添加 `#pragma once`，避免头文件重复包含。
- [x] 引入阻塞式请求缓冲区，处理一次 `recv` 读不完整的情况。
- [x] 拆分 `server/`、`connection/` 和 `util/` 模块。
- [x] 添加 `scripts/manual_test.sh` 手动测试脚本。

当前代码结构：

```text
main.cpp              解析端口，调用 run_server
server/               监听 socket 和 accept 主循环
connection/           单个客户端连接处理
http-1.1/             HTTP 请求解析和响应构造
util/                 文件读取、socket 发送等通用函数
static/               静态 HTML 页面
```

## 下一步优先任务

- [ ] 暂不扩展静态文件映射。
  - 当前阶段先保留现有行为：主要支持 `/`、`/index` 和 `/index.html`。
  - 未知静态文件返回逻辑暂不继续优化。
  - URL 到本地文件路径的通用映射函数暂不提取。
  - 原因：当前学习重点转向连接处理结构，为后续非阻塞 socket 和 `epoll` 做准备。

- [ ] 明确 fd 关闭责任。
  - 当前：`server/server.cpp` 在 `handle_client(client_fd)` 后统一 `close(client_fd)`。
  - 当前约定是可用的，但后续类化时需要重新确认。
  - 下一步要写清楚：
    - `server` 负责 accept 和最终 close。
    - `connection` 只处理读写，不主动 close。
    - 或者 `HttpConnection` 析构时负责 close。
  - 二选一即可，不要两个模块同时 close。

- [ ] 将 `connection/` 从函数集合演进为 `HttpConnection` 类。
  - 当前：`connection/http_connection.cpp` 还是一组函数。
  - 目标：先做阻塞式 `HttpConnection`，不引入非阻塞和 `epoll`。
  - 类中可以保存：
    - `int client_fd`
    - `std::string read_buffer`
    - `HttpRequest request`
    - `HttpResponse response`
    - `ParseResult parse_result`
  - 初始接口可以设计为：
    - `bool read_request()`
    - `bool build_response()`
    - `bool send_response()`
    - `void handle()`
  - 完成后，`server/server.cpp` 中可以从 `handle_client(client_fd)` 变为创建 `HttpConnection conn(client_fd); conn.handle();`。

## 当前不急着做

- [ ] 非阻塞 socket。
  - 等阻塞式 `HttpConnection` 类稳定后再做。
  - 到时需要处理 `EAGAIN/EWOULDBLOCK`。

- [ ] `epoll`。
  - 先使用 LT 模式。
  - 不要和线程池同时引入。

- [ ] keep-alive。
  - 当前响应头仍然使用 `Connection: close`。
  - 等连接状态对象稳定后再考虑复用连接。

- [ ] chunked body。
  - 当前先依赖 `Content-Length`。
  - 学习阶段可以先不实现。

## 后续路线

参考 TinyWebServer 的学习点，后续建议按这个顺序推进：

- [ ] 第一阶段：HTTP 解析和静态资源返回稳定。
- [ ] 第二阶段：抽取连接处理模块，形成 `HttpConnection` 类。
- [ ] 第三阶段：使用非阻塞 socket。
- [ ] 第四阶段：引入 `epoll`，先使用 LT 模式。
- [ ] 第五阶段：增加定时器，关闭空闲连接。
- [ ] 第六阶段：增加同步日志。
- [ ] 第七阶段：增加线程池。
- [ ] 第八阶段：再做压测和性能优化。

参考仓库：https://github.com/qinguoyi/TinyWebServer
