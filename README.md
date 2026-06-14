# cpp-linux-webserver

一个用于学习 Linux C++ 网络编程的轻量级 WebServer 项目。

项目目标不是一次性复刻完整 TinyWebServer，而是从最小可运行服务器开始，逐步理解 socket、HTTP、阻塞 IO、非阻塞 IO、`epoll`、线程池和连接管理。

## 当前进度

当前已经完成第一阶段：最小 HTTP 服务器。

已完成能力：

- 创建 TCP 监听 socket。
- 绑定本地端口并监听连接。
- 支持通过命令行指定端口，例如 `./server 8080`。
- 使用 `SO_REUSEADDR`，便于开发时快速重启。
- 接收客户端 HTTP 请求并打印到终端。
- 返回固定 HTTP 响应：`Hello World`。
- 响应头中明确使用 `Connection: close`。
- 请求处理结束后关闭客户端连接。
- 对 `recv` 和 `send` 做了基础分支处理。

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
curl -v http://127.0.0.1:8080
```

## 当前阶段边界

第一阶段只验证最基础的请求链路：

```text
socket -> bind -> listen -> accept -> recv -> send -> close
```

暂时还没有处理这些内容：

- 不解析 HTTP 请求行和请求头。
- 不根据 URL 路径返回不同内容。
- 不读取静态文件。
- 不支持非阻塞 IO。
- 不使用 `epoll`。
- 不处理并发连接。

## 下一阶段目标

下一阶段进入“静态文件服务器”：

- 新增 `www/index.html` 作为默认页面。
- 解析最小 HTTP 请求行，例如 `GET /index.html HTTP/1.1`。
- 支持访问 `/` 时返回 `/index.html`。
- 根据请求路径读取 `www/` 目录下的文件。
- 文件存在时返回 `200 OK`。
- 文件不存在时返回 `404 Not Found`。
- 非 `GET` 请求暂时返回 `405 Method Not Allowed`。
- 非法请求行返回 `400 Bad Request`。

这个阶段仍然先使用阻塞 IO，不急着引入 `epoll`。目标是先把 HTTP 请求解析、文件路径处理和响应构造这三件事理解清楚。

## 后续路线

完成静态文件服务器后，再进入：

- 抽取 `handle_client`、`create_listen_socket` 等函数。
- 引入非阻塞 socket。
- 使用 `epoll` 管理监听 socket 和客户端 socket。
- 拆分 `Server`、`HttpConnection`、`Buffer`、`HttpRequest`、`HttpResponse` 等模块。
- 最后再考虑线程池、定时器和日志系统。

## 学习方式

本项目采用“先自己实现，再用 AI review”的方式推进：

1. 先理解模块职责和系统调用流程。
2. 自己写第一版代码。
3. 使用 AI 检查 bug、边界情况和优化方向。
4. 将每轮问题整理到 `ai-docs/todo.md`。

参考方向：TinyWebServer、Linux 高性能服务器编程、UNIX 网络编程。
