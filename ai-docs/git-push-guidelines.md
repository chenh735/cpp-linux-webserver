# Git Push 规范

本文档记录本项目在 Windows 宿主机和 Linux 虚拟机之间通过 GitHub 同步代码时的基本规范。

## 基本原则

- 只提交源码、文档、配置和必要的静态资源。
- 不提交编译产物、临时文件、日志文件和本地 IDE 缓存。
- 每次 push 前先确认工作区状态。
- 提交信息要说明本次变更的目的。
- Windows 宿主机和 Linux 虚拟机同步前都先 `pull`，避免分叉提交。

## 推荐流程

### 1. 开始开发前拉取远端更新

```bash
git pull
```

如果是在 Linux 虚拟机开发，先拉取宿主机或 GitHub 上的最新文档和代码。

如果是在 Windows 宿主机使用 Codex 修改文档，也先拉取虚拟机推送过来的最新代码。

### 2. 查看当前改动

```bash
git status
```

重点确认：

- 哪些文件被修改。
- 哪些文件是新增的。
- 是否有不应该提交的编译产物。

### 3. 查看具体差异

```bash
git diff
```

提交前至少快速扫一遍 diff，确认没有误改无关内容。

### 4. 暂存需要提交的文件

推荐明确指定文件，不建议无脑使用 `git add .`。

```bash
git add main.cpp README.md ai-docs/todo.md
```

如果新增静态资源，可以明确添加：

```bash
git add static/index.html
```

### 5. 提交

```bash
git commit -m "docs: update next step plan"
```

提交信息建议使用简单前缀：

```text
docs: 文档变更
feat: 新功能
fix: bug 修复
refactor: 代码重构
chore: 项目配置或杂项
test: 测试相关
```

示例：

```bash
git commit -m "feat: serve static index page"
git commit -m "fix: close client fd after response"
git commit -m "docs: add static server plan"
```

### 6. 推送到 GitHub

```bash
git push
```

推送后，另一台机器需要同步时执行：

```bash
git pull
```

## 不应该提交的内容

不要提交这些文件：

```text
server
*.o
*.out
build/
CMakeFiles/
CMakeCache.txt
*.log
```

原因：

- `server` 是 Linux 编译出来的二进制文件，不适合在 Windows 和 Linux 之间同步。
- 编译产物每次构建都可能变化，会污染 Git 历史。
- 不同系统或不同编译器生成的二进制文件可能无法通用。

## 如果误提交了编译产物

如果文件已经被 Git 跟踪，`.gitignore` 不会让它自动失效。

例如 `server` 已经进入 Git 索引，需要执行：

```bash
git rm --cached server
git commit -m "chore: stop tracking server binary"
git push
```

这个命令只会让 Git 不再跟踪 `server`，不会删除本地文件。

## Windows 和 Linux 同步建议

推荐分工：

```text
Windows 宿主机：
- 使用 Codex
- 修改 README、ai-docs、博客文档
- 做代码 review 和文档整理

Linux 虚拟机：
- 编译运行
- 测试 socket、HTTP、epoll 等 Linux 相关功能
- 提交源码改动
```

推荐同步节奏：

```text
开始前：git pull
开发后：git status
提交前：git diff
提交后：git push
另一端：git pull
```

## Push 前检查清单

- [ ] 已执行 `git status`。
- [ ] 已查看关键 `git diff`。
- [ ] 没有提交 `server` 等编译产物。
- [ ] 提交信息能说明本次改动。
- [ ] 如果另一台机器也改过代码，已经先执行 `git pull`。
- [ ] push 后另一台机器会执行 `git pull` 同步。
