# my_muduo

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

轻量级 C++ Reactor 网络库，参考 [muduo](https://github.com/chenshuo/muduo) 架构从零手写。作为学习项目——每一行代码都为了理解 Reactor 模式的底层设计。

A lightweight C++ Reactor network library inspired by muduo. Built from scratch to understand how Reactor pattern works under the hood.

## Features

- **Channel** — fd + 事件 + 回调的抽象，不拥有 fd
- **Poller** — epoll 的 C++ 薄封装，不暴露系统调用
- **EventLoop** — 事件循环，Poll → Dispatch → Poll → Dispatch...
- **Socket** — RAII fd 管理，只移不拷，封装 socket/bind/listen
- **Acceptor** — 监听 socket + Channel 封装，新连接回调通知
- **TcpConnection** — 管理客户端连接，read/write + echo
- **Buffer** — 输入缓冲区，readv 高性能读取 + findCRLF 按行切消息
- **TcpServer** — 顶层封装，组合 Acceptor + TcpConnection，10 行启动 echo server
- **EventLoopThread / Pool** — 多线程支持，one loop per thread，轮转负载均衡
- **TimerQueue** — timerfd 定时器，与 socket fd 统一在 epoll 中管理
- **One loop per thread** — 每个 EventLoop 独占一个 Poller 实例

## Quick Start

### Echo Server

```cpp
#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include <iostream>
#include <arpa/inet.h>

int main() {
    EventLoop loop;

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    Acceptor acceptor(&loop, addr);
    acceptor.setNewConnectionCallback([&](int connfd, const sockaddr_in& peer) {
        auto* conn = new TcpConnection(&loop, connfd);
        conn->setMessageCallback([conn](const char* data, size_t len) {
            std::string msg(data, len);
            std::cout << "收到: " << msg << std::endl;
            conn->send("服务器收到: " + msg);
        });
        conn->connectEstablished();
    });
    acceptor.listen();

    std::cout << "监听 8080..." << std::endl;
    loop.loop();
}
```

```bash
nc localhost 8080
# 敲 hello → 服务器收到: hello
```

## Architecture

```
EventLoop::loop()
  → poller_.poll(activeChannels)       // epoll_wait
    → fillActiveChannels_()            // epoll_event → Channel*
  → for each Channel:
      channel->handleEvent()           // 分发回调
        → if (revents_ & kReadEvent)
            readCallback_()            // 用户设的回调
```

### Core Classes

| 类 | 职责 | 类比 |
|---|---|---|
| `Channel` | 一个 fd + 回调，不拥有 fd | "这个 socket 可读了就叫 A 函数" |
| `Poller` | epoll 封装（create/ctl/wait） | "别直接碰 epoll，通过我来" |
| `EventLoop` | while(true) { poll → dispatch } | "整个程序的发动机" |
| `Socket` | RAII 管理 fd 生命周期 | "这房子我租的，走的时候我关" |
| `Acceptor` | 监听 socket + 新连接回调 | "门童——站门口，来人就通报" |
| `TcpConnection` | 连接 fd 读写 + 回调通知 | "服务员——接了客人，读单上菜" |
| `Buffer` | 输入缓冲 + 按行切消息 | "托盘——攒够了才端上去" |
| `TcpServer` | 组合 Acceptor + TcpConnection | "餐厅——门童接人，服务员服务" |
| `EventLoopThread` | 一个线程跑一个 EventLoop | "新开一家分店" |
| `TimerQueue` | timerfd 定时器，epoll 统一管理 | "闹钟——到点提醒" |

### Key Design

**Channel 不拥有 fd**：fd 由外层 Socket 类管理生命周期，Channel 只是一个"视图"——关注点分离。<br>
**Poller 通过 map 找回 Channel**：epoll 返回的是 fd，Poller 维护 `map<fd, Channel*>` 做映射——epoll 只管通知谁有事件，不管谁处理。<br>
**update() 层层委托**：`Channel::update()` → `EventLoop::updateChannel()` → `Poller::updateChannel()` → `epoll_ctl()`——每层只做一件事。<br>
**Buffer 双索引不搬数据**：`readerIndex_` / `writerIndex_` 双指针标记可读区间，retrieve 只移 reader 不删数据；readv + extrabuf 栈空间一次清空内核缓冲区。

## Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
./muduo_demo
```

依赖：Linux + g++ + cmake + epoll。

## Roadmap

- [x] Channel（fd 抽象 + 回调绑定）
- [x] Poller（epoll 封装）
- [x] EventLoop（事件循环，stdin demo 跑通）
- [x] Socket 封装（RAII fd 管理，只移不拷）
- [x] Acceptor（监听 socket + accept 回调）
- [x] TcpConnection（客户端连接读写，echo server 跑通）
- [x] Buffer（输入缓冲区，readv + findCRLF 粘包处理）
- [x] TcpServer（组合 Acceptor + TcpConnection，单线程 Reactor 闭环）
- [x] EventLoopThread（一个线程跑一个 EventLoop）
- [x] EventLoopThreadPool（线程池 + 轮转调度，多线程 Reactor）
- [x] TimerQueue（timerfd 定时器，epoll 统一管理）

## License

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Licensed under the **glorious, legendary, battle-tested MIT License**.
Free as in freedom. Free as in "do whatever you want, just don't sue me."
