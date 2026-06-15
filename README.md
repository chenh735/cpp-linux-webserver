# cpp-linux-webserver

一个用于学习 Linux C++ 网络编程的轻量级 WebServer 项目。

项目目标不是一次性复刻完整 TinyWebServer，而是从最小可运行服务器开始，逐步理解 socket、HTTP、阻塞 IO、非阻塞 IO、`epoll`、线程池和连接管理。

## 当前进度

当前已经从“固定返回 Hello World”的最小服务器，推进到 HTTP/1.1 请求解析和响应构造阶段。

已完成能力：

- 创建 TCP 监听 socket。
- 绑定本地端口并监听连接。
- 支持通过命令行指定端口，例如 `./server 8080`。
- 使用 `SO_REUSEADDR`，便于开发时快速重启。
- 接收客户端 HTTP 请求。
- 初步解析 HTTP/1.1 请求行、Header 和可选 body。
- 使用 `HttpRequest` 保存 method、path、version、header、query 和 body。
- 使用 `HttpResponse` 构造 HTTP 响应字符串。
- 已添加 `static/` 静态页面目录。
- 支持返回 `index.html`、`400BadRequest.html`、`405MethodNotAllowed.html` 等错误页面。
- 响应头中使用 `Connection: close`。
- 请求处理结束后关闭客户端连接。

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

## 当前目录

```text
.
├── main.cpp
├── Makefile
├── http-1.1/
│   ├── request.hpp
│   ├── request.cpp
│   ├── response.hpp
│   └── response.cpp
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
- 请求读取只调用一次 `recv`，还没有处理完整的请求缓冲。
- 静态文件路径映射还不完整。
- 还没有真正支持任意静态文件和 `404 Not Found`。
- HTTP 解析器仍需要修复 Header、query、body 等边界问题。
- 还没有使用非阻塞 socket 和 `epoll`。

## 下一阶段目标

下一阶段重点是稳定 HTTP/1.1 解析和静态文件返回：

- 修复当前 HTTP 解析和响应构造中的 bug。
- 完成 `/`、`/index.html`、不存在文件、非法请求、非 GET 请求的完整返回逻辑。
- 把重复的“读取页面 + 构造响应”逻辑提取为辅助函数。
- 整理 `request.hpp/response.hpp` 的头文件结构，避免后续多源文件链接问题。
- 调整 Makefile，只编译 `.cpp` 文件，不把 `.hpp` 当作编译单元。

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
