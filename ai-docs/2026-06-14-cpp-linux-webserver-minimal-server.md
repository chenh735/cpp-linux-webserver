---
title: 从零实现 C++ Linux WebServer：最小 HTTP 服务器
date: 2026-06-14 00:00:00
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

这篇文章记录我从零实现一个 Linux C++ WebServer 的过程。当前已经完成第一阶段：先写出一个最小可运行的 HTTP 服务器。

这个阶段的目标不是高性能，也不是完整复刻 TinyWebServer，而是先把最基础的网络请求链路跑通，然后再逐步升级成静态文件服务器、`epoll` 服务器和多线程服务器。

<!-- more -->

## 当前目标

第一阶段希望达到的效果很简单：

```text
启动服务器：./server 8080
浏览器访问：http://127.0.0.1:8080
返回内容：Hello World
```

虽然功能很小，但它已经包含了 Linux 网络服务器的核心流程：

```text
socket -> bind -> listen -> accept -> recv -> send -> close
```

## 已经完成的内容

当前版本已经实现了这些能力：

- 创建 TCP socket。
- 绑定端口并监听连接。
- 支持命令行传入端口。
- 使用 `SO_REUSEADDR` 方便开发时快速重启。
- 接收浏览器或 `curl` 发来的 HTTP 请求。
- 打印请求内容。
- 返回一个固定的 HTTP 响应。
- 响应头中使用 `Connection: close`。
- 请求处理完成后关闭客户端连接。
- 对 `recv` 和 `send` 的返回值做了初步处理。

## 学到的关键点

### 1. HTTP 响应不只是正文

服务端不能只发送 `Hello World`，而是要返回符合 HTTP 格式的内容。

一个最小响应大致由三部分组成：

```text
状态行
响应头
空行
响应体
```

其中响应头和响应体之间必须用空行分隔，也就是 `\r\n\r\n`。

### 2. `send` 不一定一次发送完

即使响应内容很短，也应该知道：`send` 的返回值表示本次实际发送的字节数。

所以更稳妥的做法是循环发送，直到全部内容发送完成，或者遇到错误。

### 3. 客户端 fd 必须关闭

`accept` 返回的 `client_fd` 代表一次客户端连接。

如果处理完请求后不关闭它，就会产生文件描述符泄漏。请求次数多了以后，服务器可能无法继续接受新连接。

### 4. `.gitignore` 不会影响已经被 Git 跟踪的文件

编译生成的 `server` 是构建产物，不应该提交到仓库。

如果它已经被 Git 跟踪，即使 `.gitignore` 中写了 `server`，Git 仍然会继续跟踪它。需要先从索引中移除，再依赖 `.gitignore` 忽略。

## 下一步：静态文件服务器

最小 HTTP 服务器已经完成了第一阶段目标：浏览器或 `curl` 可以连接服务器，服务器能够读取请求并返回固定的 `Hello World` 响应。

下一步不急着进入 `epoll` 和线程池，而是先把固定响应升级为静态文件响应。这样可以先理解 HTTP 请求路径、响应状态码、文件读取和安全路径处理。

## 为什么下一步做静态文件

当前服务器虽然能返回内容，但无论访问什么路径，响应都是固定的。

这说明我们只验证了网络链路：

```text
socket -> bind -> listen -> accept -> recv -> send -> close
```

但一个 WebServer 还需要根据请求内容做不同处理。静态文件服务器正好是下一阶段最合适的练习目标，因为它会引入三个核心问题：

- 如何解析 HTTP 请求行。
- 如何把 URL 路径映射成本地文件路径。
- 如何根据处理结果返回不同 HTTP 状态码。

## 静态文件阶段的目标效果

下一阶段希望实现这些行为：

```text
GET / HTTP/1.1               -> 返回 www/index.html
GET /index.html HTTP/1.1     -> 返回 www/index.html
GET /not-found.html HTTP/1.1 -> 返回 404 Not Found
POST / HTTP/1.1              -> 返回 405 Method Not Allowed
错误请求行                    -> 返回 400 Bad Request
```

这个阶段仍然使用阻塞 IO。重点不是并发性能，而是把 HTTP 处理流程走通。

## 请求处理流程

可以先按照这个顺序设计：

```text
读取请求
解析请求行
校验 method/path/version
把 path 转换成本地文件路径
读取文件
构造 HTTP 响应
发送响应
关闭连接
```

其中最关键的是请求行。

一个典型请求行长这样：

```text
GET /index.html HTTP/1.1
```

它可以拆成三个字段：

- `method`：请求方法，例如 `GET`。
- `path`：请求路径，例如 `/index.html`。
- `version`：HTTP 版本，例如 `HTTP/1.1`。

## 路径映射

服务器不能直接相信客户端传来的路径。

例如客户端请求：

```text
GET /../README.md HTTP/1.1
```

如果直接拼接路径，就可能读取到 `www/` 目录之外的文件。这就是目录穿越问题。

学习阶段可以先采用简单规则：

- `/` 映射为 `www/index.html`。
- 路径中包含 `..` 直接返回 `400 Bad Request`。
- 所有文件都只能从 `www/` 目录读取。

## 响应状态码

下一阶段至少需要处理四类响应：

- `200 OK`：文件存在并成功读取。
- `400 Bad Request`：请求行格式不合法，或者路径不安全。
- `404 Not Found`：文件不存在。
- `405 Method Not Allowed`：请求方法不是 `GET`。

响应格式仍然保持简单：

```text
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: ...
Connection: close

文件内容
```

## 暂时不做的事

为了保持阶段目标清晰，下面这些内容先不做：

- 非阻塞 socket。
- `epoll`。
- 线程池。
- Keep-Alive。
- 大文件传输优化。
- 完整 MIME 类型识别。
- 完整 HTTP Header 解析。

这些内容后面都很重要，但现在提前加入会分散注意力。

## 完成标准

这个阶段完成后，服务器应该能通过这些测试：

```bash
curl -v http://127.0.0.1:8080/
curl -v http://127.0.0.1:8080/index.html
curl -v http://127.0.0.1:8080/not-found.html
curl -X POST -v http://127.0.0.1:8080/
```

如果这些测试能得到预期状态码和响应内容，就可以进入下一阶段：非阻塞 socket 和 `epoll`。
