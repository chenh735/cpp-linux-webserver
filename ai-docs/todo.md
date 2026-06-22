# TODO：阻塞式静态服务器到 HttpConnection 阶段

## 当前阶段总结

当前项目已经完成最小阻塞式 HTTP 服务器的主体流程：

- [x] 创建 TCP 监听 socket，支持 `./server 8080` 启动。
- [x] 初步解析 HTTP/1.1 请求行、Header、query 和 body。
- [x] 使用 `HttpRequest` 保存请求数据。
- [x] 使用 `HttpResponse` 构造响应数据。
- [x] 将 `request.hpp`、`response.hpp` 中的实现迁移到 `.cpp`。
- [x] Makefile 只编译 `.cpp` 文件。
- [x] 添加 `#pragma once`，避免头文件重复包含。
- [x] 提取 `read_file(path)`。
- [x] 提取 `build_html_response(status, file_path, resp)`。
- [x] 提取 `send_all(client_fd, response)`。
- [x] 提取 `handle_client(int client_fd)`，让主循环只保留 `accept -> handle_client -> close`。
- [x] 添加 `scripts/manual_test.sh` 手动测试脚本。

这一阶段的重点已经从“先跑起来”转为“把连接处理边界理清楚”。

## 下一步优先修复

- [ ] 修正未知静态文件的返回逻辑。
  - 当前：非 `/`、`/index`、`/index.html` 的成功 GET 请求仍可能走 `400 Bad Request`。
  - 期望：请求路径格式合法但文件不存在时，返回 `404 Not Found` 和 `static/404NotFound.html`。
  - 思路：先判断 URL path 是否非法，再判断本地文件是否存在。

- [ ] 整理 URL 路径到本地文件路径的映射函数。
  - 可提取：`map_url_to_static_path(request.path, file_path)`。
  - `/` 映射到 `static/index.html`。
  - `/index.html` 映射到 `static/index.html`。
  - 包含 `..` 的路径直接返回 `400 Bad Request`。
  - 文件不存在返回 `404 Not Found`。

- [x] 让 `handle_client` 的分支更清晰。
  - 当前 `handle_client` 仍然同时负责 `recv`、解析、路由、构造响应和发送。
  - 可以继续提取：
    - `build_response_from_request(result, request, resp)`
    - `build_error_response(status, resp)`
    - `resolve_static_response(request, resp)`

- [x] 处理请求不完整的情况。
  - 当前只有一次 `recv`，遇到 `ParseStatus::Incompleted` 只能返回 `400 Bad Request`。
  - 目标：先在阻塞版本中引入最小请求缓冲区，让服务器可以处理一次 `recv` 没读完整的请求。
  - 第一版实现边界：
    - 在 `handle_client` 中使用 `std::string read_buffer` 保存已读数据。
    - 每次 `recv` 后把新数据追加到 `read_buffer`。
    - 每次追加后调用 `parse_http_request(read_buffer, request)`。
    - `ParseStatus::Completed`：停止读取，进入响应构造。
    - `ParseStatus::Error`：停止读取，构造 `400` 或 `405` 响应。
    - `ParseStatus::Incompleted`：继续读取，而不是立即返回 `400`。
  - 需要加的保护：
    - 设置最大请求大小，例如 `MAX_REQUEST_SIZE = 8192` 或 `1024 * 1024`。
    - 超过最大请求大小仍未解析完成时，返回 `400 Bad Request` 或后续扩展 `413 Payload Too Large`。
    - `recv` 返回 `0` 时表示客户端关闭连接，应结束当前连接处理。
    - `recv` 返回 `-1` 时先按错误处理，后续非阻塞版本再考虑 `EAGAIN/EWOULDBLOCK`。
  - 阻塞版本推荐顺序：
    - 第一步：循环读取到 `\r\n\r\n`，让 GET 请求头完整。
    - 第二步：依赖当前 parser 的 `Content-Length` 判断 body 是否完整。
    - 第三步：把读取逻辑提取成 `read_http_request(client_fd, raw_request)`，为后续 `HttpConnection` 做准备。
  - 手动验证：
    - `printf 'GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n' | nc 127.0.0.1 8080`
    - 分段发送请求时，服务器不应因为第一次读不完整就返回 `400`。
  - 暂不处理：
    - 非阻塞 socket。
    - `epoll`。
    - keep-alive。
    - chunked body。

## HttpConnection 雏形

- [ ] 新增 `HttpConnection` 类或模块。
  - 目标：把单个连接的状态从 `main.cpp` 中抽出来。
  - 初始职责：
    - 保存 `client_fd`。
    - 保存读缓冲区。
    - 调用 HTTP 解析器。
    - 构造并发送响应。

- [ ] 明确 fd 关闭责任。
  - 当前：`main()` 在 `handle_client(client_fd)` 后统一 `close(client_fd)`。
  - 后续如果引入 `HttpConnection`，需要明确由谁负责关闭连接，避免 double close。

- [ ] 保留阻塞版本作为基线。
  - 先让 `HttpConnection` 在阻塞模型下工作。
  - 等行为稳定后，再切换到非阻塞 socket 和 `epoll`。

## 后续路线

参考 TinyWebServer 的学习点，后续建议按这个顺序推进：

- [ ] 第一阶段：HTTP 解析和静态资源返回稳定。
- [ ] 第二阶段：抽取连接处理模块，形成 `HttpConnection` 雏形。
- [ ] 第三阶段：使用非阻塞 socket。
- [ ] 第四阶段：引入 `epoll`，先使用 LT 模式。
- [ ] 第五阶段：增加定时器，关闭空闲连接。
- [ ] 第六阶段：增加同步日志。
- [ ] 第七阶段：增加线程池。
- [ ] 第八阶段：再做压测和性能优化。

参考仓库：https://github.com/qinguoyi/TinyWebServer
