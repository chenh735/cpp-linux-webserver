# cpp-linux-webserver

一个用于学习 Linux C++ 网络编程的轻量级 WebServer 项目。

当前阶段目标不是一次性实现完整 TinyWebServer，而是从最小可运行服务器开始，逐步理解 socket、HTTP、阻塞 IO、非阻塞 IO、`epoll`、线程池和连接管理。

## 当前进度

目前已经完成一个最小 HTTP 服务器：

- 创建 TCP 监听 socket。
- 绑定本地端口并监听连接。
- 支持通过命令行指定端口。
- 使用 `SO_REUSEADDR` 便于开发时快速重启。
- 接收客户端 HTTP 请求并打印到终端。
- 返回固定响应内容：`Hello World`。
- 请求处理结束后关闭客户端连接。
- 对 `recv` 和 `send` 做了基础分支处理。

## 构建与运行

本项目使用 Linux/POSIX 网络接口，例如 `<sys/socket.h>`，因此需要在 Linux 或 WSL 环境中编译运行。

```bash
make build
./server 8080
```

访问：

```text
http://127.0.0.1:8080
```

也可以使用 `curl` 测试：

```bash
curl -v http://127.0.0.1:8080
```

## 当前限制

当前版本仍然是学习用的最小实现，还有不少限制：

- 仍然是阻塞式 `accept/recv/send` 流程。
- 一次只能顺序处理连接。
- 还没有使用非阻塞 socket。
- 还没有使用 `epoll`。
- 还没有解析 HTTP 请求行、请求头和请求体。
- 还不能返回静态文件。
- 错误处理和连接关闭逻辑还需要继续打磨。

## 下一步计划

短期目标：

- 修正 HTTP 响应头和错误处理细节。
- 清理源码编码和错误提示。
- 把单个连接的处理逻辑提取成函数。
- 增加简单的静态文件返回能力。

中期目标：

- 引入非阻塞 socket。
- 使用 `epoll` 管理监听 socket 和客户端 socket。
- 拆分出 `Server`、`HttpConnection`、`Buffer` 等模块。
- 后续再加入线程池、定时器和日志系统。

## 学习方式

本项目采用“先自己实现，再用 AI review”的方式推进：

1. 先理解模块职责和系统调用流程。
2. 自己写第一版代码。
3. 使用 AI 检查 bug、边界情况和优化方向。
4. 将每轮问题整理到 `ai-docs/todo.md`。

参考方向：TinyWebServer、Linux 高性能服务器编程、UNIX 网络编程。
