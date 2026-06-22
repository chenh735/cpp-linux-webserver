---
title: 从零实现 C++ Linux WebServer：从最小服务器到 HTTP/1.1 解析
date: 2026-06-14 00:00:00
updated: 2026-06-22 00:00:00
tags:
  - C++
  - Linux
  - WebServer
  - Socket
  - HTTP
  - 静态文件
categories:
  - C++ WebServer
---

这篇文章记录我从零实现一个 Linux C++ WebServer 的过程。项目从最小可运行服务器开始，逐步加入 HTTP/1.1 请求解析、响应构造和静态文件返回能力。

这个阶段的目标不是高性能，也不是完整复刻 TinyWebServer，而是先把基础链路、HTTP 处理流程和代码结构真正写明白。

<!-- more -->

## 第一阶段：最小 HTTP 服务器

最开始的目标很简单：

```text
启动服务器：./bin/server 8080
浏览器访问：http://127.0.0.1:8080
返回内容：Hello World
```

这一版虽然功能很小，但它跑通了 Linux 网络服务器最基础的流程：

```text
socket -> bind -> listen -> accept -> recv -> send -> close
```

完成这个阶段后，我理解了几件事：

- HTTP 响应不能只有正文，还必须有状态行、响应头、空行和响应体。
- `send` 不一定一次发送完，应该根据返回值循环发送。
- `accept` 得到的 `client_fd` 在请求处理结束后必须关闭。
- 编译生成的 `server`、`main` 等二进制文件不应该提交到 Git。

## 第二阶段：静态页面和 HTTP 解析

固定返回 `Hello World` 之后，下一步是让服务器根据请求内容返回不同结果。

为此我添加了两个方向的内容。

一是静态页面目录：

```text
static/
  index.html
  400BadRequest.html
  404NotFound.html
  405MethodNotAllowed.html
```

二是 HTTP/1.1 解析模块：

```text
http-1.1/
  request.hpp
  request.cpp
  response.hpp
  response.cpp
```

其中 `HttpRequest` 用于保存请求信息：

- method
- path
- version
- header
- query
- body

`HttpResponse` 用于根据状态码、Header 和 body 构造完整 HTTP 响应。

## 第三阶段：整理代码结构

随着代码增多，问题不再只是“能不能返回页面”，还包括“代码以后能不能继续扩展”。

这一阶段做了几件结构整理：

- `request.hpp` 和 `response.hpp` 只保留类型声明和函数声明。
- 解析实现移动到 `request.cpp`。
- 响应实现和 `status_to_msg` 定义移动到 `response.cpp`。
- Makefile 只编译 `.cpp` 文件，不再把 `.hpp` 当作编译单元。
- 添加 `#pragma once`，避免头文件重复包含。
- 在 `main.cpp` 中提取 `read_file`、`build_html_response`、`send_all`。
- 将单个客户端连接流程封装为 `handle_client(int client_fd)`。
- 引入阻塞式请求缓冲区，避免一次 `recv` 读不完整时直接返回 `400 Bad Request`。
- 将项目拆成 `server/`、`connection/` 和 `util/` 三个模块。

这样整理后，`main()` 的职责更清楚：

```text
解析端口 -> run_server(port)
```

`server/` 负责服务器主流程：

```text
socket -> bind -> listen -> accept -> handle_client -> close
```

`connection/` 负责单个连接中的：

```text
read_http_request -> parse_http_request -> build response -> send_all
```

`util/` 暂时放通用工具函数：

```text
read_file
send_all
```

这一步不是为了追求“高级写法”，而是为了让每个文件的职责更清楚。后续如果继续引入非阻塞 socket 和 `epoll`，`main.cpp` 不需要再继续膨胀。

## 这次实现中遇到的问题

### 1. 调试输出容易污染服务器行为

在实现请求解析时，我加了很多 `cout` 输出 method、path、header 等中间结果。

这对调试有帮助，但放在服务器主流程里会让终端输出变得很乱，也不利于后续测试。因此后面把这些调试输出删掉了。

更合理的做法是后续实现日志系统，用日志级别区分：

- debug
- info
- warn
- error

### 2. 请求解析要区分“没读完整”和“真的错误”

HTTP 请求从 socket 读出来时，不一定一次 `recv` 就完整。

所以解析结果不能只看错误类型，还要区分解析状态：

- Completed：请求完整并解析成功。
- Incompleted：请求还没读完整。
- Error：请求格式确实错误。

当前阻塞版本已经加入最小请求缓冲区：每次 `recv` 后把数据追加到 `raw_request`，再调用 `parse_http_request`。如果解析结果是 `Incompleted`，就继续读取；如果是 `Completed` 或 `Error`，再进入响应构造。

这一版仍然是阻塞模型，还没有处理非阻塞 socket 下的 `EAGAIN/EWOULDBLOCK`，也没有 keep-alive。

### 3. URL 路径和本地文件路径不能直接等同

客户端请求的是 URL：

```text
GET /index.html HTTP/1.1
```

服务器真正读取的是本地文件：

```text
static/index.html
```

这中间需要一个明确的映射步骤。

同时还要避免目录穿越，例如：

```text
GET /../README.md HTTP/1.1
```

学习阶段可以先用简单规则处理：

- `/` 映射到 `static/index.html`。
- `/index.html` 映射到 `static/index.html`。
- 包含 `..` 的路径直接返回 `400 Bad Request`。
- 文件不存在返回 `404 Not Found`。

### 4. Header 解析要注意下标来源

解析 Header 时，如果先把某一行切成 `line`，后续截取 value 就应该继续基于 `line`。

这类 bug 很常见：一旦字符串被切片，后续下标到底是相对于原始字符串，还是相对于当前行，必须统一。

更稳的思路是：

```text
先拿到一整行 header line
在 line 里面找冒号
基于 line 截取 name 和 value
去掉 value 前后的空格
```

## 当前项目状态

当前服务器已经具备阻塞式静态服务器雏形：

- 能监听端口。
- 能接收 HTTP 请求。
- 能初步解析 HTTP/1.1 请求。
- 能构造 HTTP 响应。
- 能在阻塞模式下循环读取不完整请求。
- 能返回 `static/index.html`。
- 能返回 `400 Bad Request` 和 `405 Method Not Allowed` 页面。
- 已经有 `404NotFound.html` 页面，但未知路径的处理逻辑还需要进一步整理。
- 已经有最小手动测试脚本 `scripts/manual_test.sh`。
- `main.cpp` 已经简化为端口解析和 `run_server(port)` 调用。

目前仍然不是高并发服务器，主要限制是：

- 一次只处理一个连接。
- socket 仍然是阻塞模式。
- 连接处理目前还是函数集合，还没有真正封装成 `HttpConnection` 类。
- 不支持 keep-alive。
- 还没有 `epoll`、线程池、定时器和日志系统。

## 下一步计划

下一步目标是把阻塞式版本整理成更稳定的静态文件服务器，并逐步把 `connection/` 演进成真正的 `HttpConnection`：

```text
GET / HTTP/1.1               -> 200 OK + static/index.html
GET /index.html HTTP/1.1     -> 200 OK + static/index.html
GET /not-found.html HTTP/1.1 -> 404 Not Found
POST / HTTP/1.1              -> 405 Method Not Allowed
错误请求行                    -> 400 Bad Request
```

接下来可以按这个顺序推进：

- 修正未知静态资源返回 `404 Not Found`。
- 提取 URL 到本地文件路径的映射函数。
- 明确 `client_fd` 的关闭责任，避免后续模块变多后出现 double close。
- 把 `connection/` 中的函数集合逐步整理成 `HttpConnection` 类。
- 再进入非阻塞 socket 和 `epoll`。

目前先不急着做并发。先把请求解析、文件映射和响应构造写稳，后面的 `epoll` 和线程池才有清楚的落点。

参考项目：https://github.com/qinguoyi/TinyWebServer
