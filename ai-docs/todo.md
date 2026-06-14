# TODO：最小 WebServer 代码 Review

## 必须修复

- [o] 每次请求处理完成后关闭 `client_fd`。
  - 问题：`accept` 得到的 `client_fd` 没有被关闭。
  - 风险：文件描述符泄漏；多次请求后可能出现 `too many open files`。
  - 位置：`main.cpp` 中 `send` 循环结束后。

- [o] HTTP 响应头中添加 `Connection: close`。
  - 问题：当前服务器没有实现 Keep-Alive，但响应头没有明确告诉客户端连接会关闭。
  - 风险：浏览器或客户端对连接生命周期的判断不够清晰。
  - 位置：`main.cpp` 中 HTTP 响应字符串。

- [o] 在 `bind` 前添加 `SO_REUSEADDR`。
  - 问题：服务器停止后马上重启时，端口可能还处于旧的 TCP 状态。
  - 风险：开发过程中可能出现 `bind` 失败。
  - 位置：`socket` 创建成功之后，`bind` 之前。

## 建议优化

- [o] 支持命令行传入端口。
  - 当前行为：端口写死为 `8080`。
  - 目标行为：支持 `./server 8080` 这种启动方式。
  - 细节：检查 `argc`，解析 `argv[1]`，拒绝非法端口值。

- [o] 更清楚地处理 `recv` 的返回值。
  - `n > 0`：读取到了请求，可以打印或处理请求内容。
  - `n == 0`：客户端主动关闭了连接。
  - `n == -1`：读取失败，应打印错误并关闭客户端 fd。

- [o] 改进 `send` 的错误处理。
  - 当前优点：已经处理了“部分发送”的情况。
  - 缺失情况：`send == 0`、被信号中断的 `EINTR`、客户端提前断开连接。
  - 后续知识点：向已关闭的 socket 发送数据时，注意避免 `SIGPIPE` 导致进程退出。

- [ ] 清理错误提示和编码问题。
  - 问题：当前中文错误信息在终端里显示为乱码。
  - 建议：源码统一保存为 UTF-8；`perror` 的提示字符串不要带换行。
  - 示例风格：`perror("socket")`、`perror("bind")`。

## 项目整理

- [o] 不要把编译产物提交到 Git。
  - 问题：`server` 当前被 Git 跟踪，编译后会显示为已修改。
  - 建议：从 Git 跟踪中移除 `server`，并把它作为构建产物忽略。

- [ ] 在 WSL/Linux 环境中编译和运行。
  - 问题：Windows 原生 g++ 找不到 Linux/POSIX 头文件，例如 `<sys/socket.h>`。
  - 建议：进入 WSL 后，在 `/mnt/c/Project/cpp/cpp-linux-webserver` 下编译运行。

- [ ] 后续可以考虑调整可执行文件名。
  - 当前名称：`server`。
  - 可选名称：`webserver`。
  - 原因：更贴合目标启动方式，例如 `./webserver 8080`。

## 后续学习步骤

- [ ] 将重复的连接处理逻辑提取成辅助函数。
- [ ] 添加非阻塞 socket 设置。
- [ ] 用 `epoll` 事件循环替换当前阻塞式 `accept/recv/send` 流程。
- [ ] 从 `www/` 目录返回静态文件。
- [ ] 最小版本稳定后，再拆分出 `Server`、`HttpConnection`、`Buffer` 等模块。
