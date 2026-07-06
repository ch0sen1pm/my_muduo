# my_muduo

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

轻量级 C++ Reactor 网络库，参考 [muduo](https://github.com/chenshuo/muduo) 架构从零手写。作为学习项目——每一行代码都为了理解 Reactor 模式的底层设计。

A lightweight C++ Reactor network library inspired by muduo. Built from scratch to understand how Reactor pattern works under the hood.

## Features

- **Channel** — fd + 事件 + 回调的抽象，不拥有 fd
- **Poller** — epoll 的 C++ 薄封装，不暴露系统调用
- **EventLoop** — 事件循环，Poll → Dispatch → Poll → Dispatch...
- **One loop per thread** — 每个 EventLoop 独占一个 Poller 实例

## Quick Start

```cpp
#include "EventLoop.h"
#include "Channel.h"
#include <iostream>
#include <unistd.h>

int main() {
    EventLoop loop;

    // fd=0 是 stdin（标准输入）
    Channel stdinChan(&loop, 0);

    // 设回调：stdin 可读时调用
    stdinChan.setReadCallback([&]() {
        char buf[256];
        ssize_t n = ::read(0, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            std::cout << "读到: " << buf;
        }
    });

    stdinChan.enableReading();
    loop.loop();  // 死循环，等待事件
}
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

### Key Design

**Channel 不拥有 fd**：fd 由外层 Socket 类管理生命周期，Channel 只是一个"视图"——关注点分离。<br>
**Poller 通过 map 找回 Channel**：epoll 返回的是 fd，Poller 维护 `map<fd, Channel*>` 做映射——epoll 只管通知谁有事件，不管谁处理。<br>
**update() 层层委托**：`Channel::update()` → `EventLoop::updateChannel()` → `Poller::updateChannel()` → `epoll_ctl()`——每层只做一件事。

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
- [ ] Socket 封装（RAII fd 管理）
- [ ] Acceptor（监听 socket + accept）
- [ ] TcpConnection（客户端连接读写）
- [ ] Buffer（输入输出缓冲区）
- [ ] TimerQueue（定时器）

## License

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Licensed under the **glorious, legendary, battle-tested MIT License**.
Free as in freedom. Free as in "do whatever you want, just don't sue me."
