# TODO：下一阶段实现静态文件服务器

## 当前阶段总结

第一阶段“最小 HTTP 服务器”已经完成。当前服务器已经能够：

- [x] 监听指定端口。
- [x] 接收客户端连接。
- [x] 读取并打印 HTTP 请求。
- [x] 返回固定 HTTP 响应。
- [x] 在响应头中使用 `Connection: close`。
- [x] 请求结束后关闭 `client_fd`。
- [x] 对 `recv` 和 `send` 做基础返回值处理。

一些细节优化暂时不继续展开，当前阶段到此收束。下一步重点转向 HTTP 请求解析和静态文件返回。

## 下一阶段目标

实现一个最小静态文件服务器。

目标效果：

```text
访问 /              -> 返回 www/index.html
访问 /index.html    -> 返回 www/index.html
访问不存在的文件    -> 返回 404 Not Found
发送非 GET 请求     -> 返回 405 Method Not Allowed
请求格式不合法      -> 返回 400 Bad Request
```

## 实现思路

### 1. 准备静态资源目录

- [ ] 新增 `www/` 目录。
- [ ] 新增 `www/index.html`。
- [ ] 暂时只允许返回 `www/` 目录下的文件。

关键点：

- URL `/` 映射到 `www/index.html`。
- URL `/index.html` 映射到 `www/index.html`。
- 不允许请求路径逃出 `www/` 目录。

### 2. 解析 HTTP 请求行

- [ ] 从 `recv` 读取到的请求中取第一行。
- [ ] 按空格拆出三个部分：`method`、`path`、`version`。
- [ ] 暂时只支持 `GET` 方法。
- [ ] 暂时只接受 `HTTP/1.0` 和 `HTTP/1.1`。

请求行示例：

```text
GET /index.html HTTP/1.1
```

这个阶段不需要完整解析所有请求头。

### 3. 构造文件路径

- [ ] 将请求路径转换为本地文件路径。
- [ ] `/` 默认转换为 `/index.html`。
- [ ] 拒绝包含 `..` 的路径，避免目录穿越。
- [ ] 拼接为 `www` 目录下的相对路径。

示例：

```text
/              -> www/index.html
/index.html    -> www/index.html
/hello.html    -> www/hello.html
```

### 4. 读取文件内容

- [ ] 判断文件是否存在。
- [ ] 文件存在则读取完整内容。
- [ ] 文件不存在则返回 404 响应。
- [ ] 暂时只处理普通小文件，不处理大文件和分块发送。

这个阶段可以先使用 C++ 文件流读取，后续再学习 `sendfile`、`mmap` 或 `writev`。

### 5. 根据结果返回 HTTP 响应

- [ ] 成功返回 `200 OK`。
- [ ] 文件不存在返回 `404 Not Found`。
- [ ] 非 GET 方法返回 `405 Method Not Allowed`。
- [ ] 请求行解析失败返回 `400 Bad Request`。

响应仍然保持简单结构：

```text
HTTP/1.1 状态码 状态说明
Content-Type: ...
Content-Length: ...
Connection: close

响应体
```

### 6. 补充手动测试

- [ ] `curl -v http://127.0.0.1:8080/`
- [ ] `curl -v http://127.0.0.1:8080/index.html`
- [ ] `curl -v http://127.0.0.1:8080/not-found.html`
- [ ] `curl -X POST -v http://127.0.0.1:8080/`

## 暂不处理

下面这些点先不进入下一阶段，避免目标过大：

- 非阻塞 IO。
- `epoll`。
- 线程池。
- 长连接 Keep-Alive。
- 完整 HTTP Header 解析。
- 大文件传输优化。
- MIME 类型完整映射。

## 完成标准

下一阶段完成时，服务器应该具备这些能力：

- 浏览器访问 `/` 能看到 `www/index.html` 页面。
- 请求不存在的文件时能得到明确的 404 响应。
- `curl -v` 能看到正确的状态码、`Content-Length` 和 `Connection: close`。
- 代码仍然保持单文件或少量函数，不急着做复杂模块拆分。
