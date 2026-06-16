# cpp-linux-webserver

一个用于学习 Linux C++ 网络编程的轻量级 WebServer 项目。

项目目标不是一次性复刻完整 TinyWebServer，而是从最小可运行服务器开始，逐步理解 socket、HTTP、阻塞 IO、非阻塞 IO、`epoll`、线程池和连接管理。

## 当前进度

当前项目已经从固定返回 `Hello World` 的最小服务器，推进到阻塞式 HTTP/1.1 静态页面服务器雏形。

已完成能力：

- 创建 TCP 监听 socket。
- 绑定本地端口并监听连接。
- 支持通过命令行指定端口，例如 `./server 8080`。
- 使用 `SO_REUSEADDR`，便于开发时快速重启。
- 接收客户端 HTTP 请求。
- 初步解析 HTTP/1.1 请求行、Header、query 和可选 body。
- 使用 `HttpRequest` 保存 method、path、version、header、query 和 body。
- 使用 `HttpResponse` 构造 HTTP 响应字符串。
- 将 `request.hpp/response.hpp` 中的实现迁移到 `.cpp` 文件，降低重复定义风险。
- 提取 `read_file`、`build_html_response`、`send_all` 和 `handle_client`，让主循环更清晰。
- 添加 `static/` 静态页面目录。
- 添加 `scripts/manual_test.sh` 手动测试脚本。

## 构建与运行

本项目使用 Linux/POSIX 网络接口，例如 `<sys/socket.h>`，需要在 Linux 或 WSL 环境中编译运行。

```bash
make build
./server 8080
```

访问：

```text
http://127.0.0.1:8080
```

使用 `curl` 查看响应：

```bash
curl -v http://127.0.0.1:8080/
```

运行手动测试脚本：

```bash
bash scripts/manual_test.sh
```

## 当前目录

```text
.
├── main.cpp
├── Makefile
├── http-1.1/
│   ├── README.md
│   ├── request.hpp
│   ├── request.cpp
│   ├── response.hpp
│   └── response.cpp
├── scripts/
│   └── manual_test.sh
├── static/
│   ├── index.html
│   ├── 400BadRequest.html
│   ├── 404NotFound.html
│   └── 405MethodNotAllowed.html
└── ai-docs/
```

## 当前阶段边界

当前版本仍然是学习阶段实现，还有这些限制：

- 仍然是阻塞式 `accept/recv/send` 流程。
- 一次只顺序处理一个连接。
- 请求读取只调用一次 `recv`，还没有请求缓冲区。
- 请求不完整时当前按 `400 Bad Request` 返回，后续需要配合 Buffer/HttpConnection 继续读取。
- 当前静态文件映射仍然较简单，主要支持 `/` 和 `/index.html`。
- 未知静态文件路径当前还需要进一步整理为 `404 Not Found`。
- 还没有使用非阻塞 socket 和 `epoll`。

## 下一阶段目标

下一阶段重点是把阻塞式版本整理成更清晰的连接处理模块，为后续 `epoll` 做准备：

- 完善静态文件映射，让不存在的资源返回 `404 Not Found`。
- 把 `handle_client` 进一步拆成更清晰的请求处理函数。
- 抽取 `HttpConnection` 雏形，管理单个连接的读、解析、写流程。
- 引入请求缓冲区，处理一次 `recv` 读不完整的情况。
- 在保持阻塞版本可运行的基础上，再引入非阻塞 socket。

## 后续路线

参考 TinyWebServer 的学习路线，后续按这个顺序推进：

1. 稳定 HTTP 请求解析和静态资源返回。
2. 抽取 `Server`、`HttpConnection`、`HttpRequest`、`HttpResponse` 等模块。
3. 引入非阻塞 socket。
4. 使用 `epoll` 管理监听 socket 和客户端 socket。
5. 增加定时器，关闭超时空闲连接。
6. 增加日志系统。
7. 增加线程池。
8. 最后再考虑压测和更完整的 HTTP 功能。

TinyWebServer 参考仓库：https://github.com/qinguoyi/TinyWebServer

## 学习方式

本项目采用“先自己实现，再用 AI review”的方式推进：

1. 先理解模块职责和系统调用流程。
2. 自己写第一版代码。
3. 使用 AI 检查 bug、边界情况和优化方向。
4. 将每轮问题整理到 `ai-docs/todo.md`。
