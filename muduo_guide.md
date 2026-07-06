# my_muduo 完全理解指南

> 从零读懂 Reactor 模式，每一行代码拆开讲

---

## 一、先搞清楚要解决什么问题

### 1.1 没有 epoll 的时候

假设你写了一个聊天服务器，有 1000 个客户端连着。你怎么知道哪个客户端发消息了？

**笨办法：轮询**

```cpp
while (true) {
    for (int i = 0; i < 1000; i++) {
        int n = recv(fd[i], buf, 1024, 0);
        if (n > 0) {
            // fd[i] 有数据，处理它
        }
    }
}
```

**问题：** 999 个没数据的连接你也白查了一遍，CPU 浪费。

### 1.2 epoll 的做法

```cpp
int epfd = epoll_create1(0);  // 创建一个 epoll 实例

// 把 1000 个 fd 全部注册进去："帮我盯着"
for (int i = 0; i < 1000; i++) {
    struct epoll_event ev;
    ev.events = EPOLLIN;          // 关心"可读"
    ev.data.fd = fd[i];           // 告诉 epoll 这是哪个 fd
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd[i], &ev);
}

// 等事件：只返回有事件的 fd！
struct epoll_event results[64];
int n = epoll_wait(epfd, results, 64, -1);
// 如果有 3 个 fd 有数据，n=3，results[0..2] 就是它们
// 你只处理这 3 个，另外 997 个完全不用管
```

**epoll 的问题：** 原生 API 只管"通知你哪个 fd 有事件"，不管"这个 fd 有事件后该干什么"。

### 1.3 原生 epoll 的痛点

```cpp
// 你要自己维护：fd → 回调函数 的对应关系
std::map<int, std::function<void()>> callbacks;

// 注册时记住
callbacks[fd] = my_callback;

// epoll 返回后
for (int i = 0; i < n; i++) {
    int fd = results[i].data.fd;
    callbacks[fd]();  // 手动查表，手动调用
}
```

**muduo 要做的就是：把"fd + 事件 + 回调"打包成一个对象（Channel），把 epoll 的三个系统调用包成 Poller，再包一个死循环（EventLoop）。**

---

## 二、三个核心类，一张图讲清

```
┌──────────────────────────────────────────────────────┐
│  main.cpp                                            │
│                                                      │
│  EventLoop loop;           // 发动机                 │
│  Channel chan(&loop, 0);   // fd=0(stdin) + 回调     │
│  chan.setReadCallback(回调);                          │
│  chan.enableReading();     // 注册到 epoll           │
│  loop.loop();              // 点火，死循环           │
└──────────────────────────────────────────────────────┘
         │
         ▼
┌──────────────────────────────────────────────────────┐
│  EventLoop                                           │
│                                                      │
│  while (!quit) {                                     │
│      activeChannels_.clear();                        │
│      poller_.poll(1000, activeChannels_);  ◄── epoll_wait
│      for (auto* ch : activeChannels_) {              │
│          ch->handleEvent();                ◄── 分发   │
│      }                                               │
│  }                                                   │
└──────────────────────────────────────────────────────┘
         │                    │
    ┌────▼────┐          ┌───▼──────┐
    │ Poller  │          │ Channel  │
    │         │          │          │
    │ epollfd │          │ fd_      │
    │ channels│◄─────────│ events_  │
    │   map   │  映射    │ revents_ │
    │ events_ │          │ callback │
    └─────────┘          └──────────┘
```

---

## 三、Channel：fd + 回调 = 一个 Channel

### 3.1 Channel 就是 epoll 的一个条目

原生 epoll 视角：

```
epoll_ctl(epfd, ADD, fd=3, events=EPOLLIN)
→ 内核记下: "fd=3, 关心 EPOLLIN"
```

Channel 视角：

```
Channel ch(loop, 3);            // fd=3
ch.setReadCallback(处理函数);    // 可读时调什么
ch.enableReading();             // → events_ |= kReadEvent → update()
                                // → 最终 epoll_ctl(ADD, 3, EPOLLIN)
```

### 3.2 成员变量，一个一个说

```cpp
int fd_;           // 我管哪个 socket/文件
                   // 就是一个整数，操作系统用它找你要操作的东西
                   // stdin = 0, stdout = 1, stderr = 2
                   // 你创建的 socket 一般从 3 开始

int events_;       // "我关心哪些事件"
                   // bit0=1 关心可读, bit1=1 关心可写, bit0+bit1 同时关心
                   // 用户通过 enableReading/enableWriting 设置

int revents_;      // "epoll 告诉我实际发生了什么"
                   // Poller 从 epoll_wait 拿到结果后填进来
                   // 格式和 events_ 一样，bit0=有可读, bit1=有可写

EventCallback readCallback_;   // 可读时调什么函数
EventCallback writeCallback_;  // 可写时调什么函数
```

### 3.3 events_ 为什么要用位运算？

```cpp
// 一个 int 有 32 个 bit，每个 bit 独立表示一种事件
kReadEvent  = 1 = 0b000...0001   第 0 位 → 可读
kWriteEvent = 2 = 0b000...0010   第 1 位 → 可写

// 设置
events_ |= kReadEvent          // 0b0000 | 0b0001 = 0b0001  关心可读
events_ |= kWriteEvent         // 0b0001 | 0b0010 = 0b0011  同时关心可读+可写

// 检查
if (revents_ & kReadEvent)     // 0b0011 & 0b0001 = 0b0001  有可读事件
if (revents_ & kWriteEvent)    // 0b0011 & 0b0010 = 0b0010  有可写事件

// 清零
events_ = kNoneEvent           // 0b0000  什么都不关心
```

**为什么这样设计？** 因为 Linux epoll 就是这个格式——EPOLLIN=1, EPOLLOUT=2, EPOLLERR=4...

### 3.4 三个核心方法

**enableReading()：**

```cpp
void Channel::enableReading() {
    events_ |= kReadEvent;  // 位运算设 bit0=1
    update();               // 通知 epoll：我改兴趣了
}
```

**update()：**

```cpp
void Channel::update() {
    loop_->updateChannel(this);  // 委派给 EventLoop
    // EventLoop → Poller → updateChannel() → epoll_ctl
    // 每层只做一件事，不越权
}
```

**handleEvent()：**

```cpp
void Channel::handleEvent() {
    if ((revents_ & kReadEvent) && readCallback_) {
        readCallback_();    // revents_ 有可读 AND 设了回调 → 调用
    }
    if ((revents_ & kWriteEvent) && writeCallback_) {
        writeCallback_();
    }
    // 为什么要检查 readCallback_？防止用户忘设回调时崩溃
}
```

---

## 四、Poller：epoll 的封装，三大系统调用

### 4.1 epoll 的三个系统调用

```cpp
// ① 创建 epoll 实例，返回一个 fd（epoll 本身也是文件）
int epfd = epoll_create1(EPOLL_CLOEXEC);
// EPOLL_CLOEXEC: 当进程执行 exec() 时自动关闭 epfd，防止泄漏到子进程

// ② 注册/修改/删除 fd 的监听
// op 可以是 EPOLL_CTL_ADD(添加) / EPOLL_CTL_MOD(修改) / EPOLL_CTL_DEL(删除)
epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &event);
//        ↑epoll实例 ↑做什么      ↑对哪个fd  ↑关心什么事件

// ③ 等待事件发生，阻塞
int n = epoll_wait(epfd, events_array, max_events, timeout_ms);
//                  ↑epoll实例 ↑结果写这里   ↑最多几个  ↑等多久ms(-1=一直等)
// 返回 n = 实际发生的事件数
```

### 4.2 Poller 的成员变量

```cpp
int epollfd_;                          // epoll_create1 返回的 fd
                                       // 后面所有 epoll_ctl/epoll_wait 都用它

std::map<int, Channel*> channels_;     // fd → Channel* 的映射
                                       // epoll_wait 返回的是 fd
                                       // 我们要通过 fd 找到对应的 Channel
                                       // 这就是这个 map 的作用

std::vector<struct epoll_event> events_;  // epoll_wait 的结果数组
                                          // 提前分配好空间
                                          // epoll_wait 把发生的事件写进去
```

### 4.3 poll() —— 核心

```cpp
void Poller::poll(int timeoutMs, vector<Channel*>& activeChannels) {
    // ① 等事件：阻塞在这里
    int n = epoll_wait(epollfd_, events_.data(), events_.size(), timeoutMs);

    // ② 你敲了回车 → n=1，events_[0] 是 fd=0 的事件
    // ③ 把 epoll_event 转成 Channel*
    for (int i = 0; i < n; i++) {
        int fd = events_[i].data.fd;          // epoll 返回的 fd
        Channel* ch = channels_[fd];          // map 查到对应 Channel
        ch->set_revents(events_[i].events);   // 把事件类型写进去
        activeChannels.push_back(ch);         // 加入活跃列表
    }
}
```

### 4.4 updateChannel() — 三种操作

```cpp
void Poller::updateChannel(Channel* channel) {
    int fd = channel->fd();

    if (channels_.count(fd) == 0) {
        // 新来的 fd，ADD
        channels_[fd] = channel;
        update_(EPOLL_CTL_ADD, channel);
    }
    else if (channel->events() == 0) {
        // 事件清零了，DELETE（不再关心这个 fd）
        update_(EPOLL_CTL_DEL, channel);
    }
    else {
        // 已有 fd 但改了事件类型，MOD
        update_(EPOLL_CTL_MOD, channel);
    }
}
```

### 4.5 update_() — 实际调 epoll_ctl

```cpp
void Poller::update_(int op, Channel* channel) {
    struct epoll_event ev;
    ev.data.fd = channel->fd();    // 把 fd 存进 event
                                   // 等 epoll_wait 返回时这个值原样返回
                                   // 我们就知道是哪个 fd 有事件了

    ev.events = 0;
    if (channel->events() & kReadEvent)  ev.events |= EPOLLIN;
    if (channel->events() & kWriteEvent) ev.events |= EPOLLOUT;
    // Channel 的 kReadEvent(1) → epoll 的 EPOLLIN(1)
    // 数值一致但不是必须的，这里是映射关系

    epoll_ctl(epollfd_, op, channel->fd(), &ev);
    // 操作 epollfd_ 这个 epoll 实例
    // op: ADD/MOD/DEL
    // 对 channel->fd() 这个 fd
    // 按 ev 配置的事件监听
}
```

**关键理解 —— ev.data.fd 的作用：**

```
注册时：
  ev.data.fd = 3
  epoll_ctl(epfd, ADD, 3, &ev)      ← 把 fd=3 存进事件结构

epoll_wait 返回时：
  events_[i].data.fd == 3           ← 内核原样返回
  channels_[3] → 找到 Channel       ← map 映射
```

这是 Poller 和 Channel 之间的桥梁——epoll 返回一个整数 fd，Poller 用 map 把它翻译成 Channel*。

---

## 五、EventLoop：发动机

### 5.1 它就是一个死循环

```cpp
void EventLoop::loop() {
    while (!quit_) {
        activeChannels_.clear();                        // 清空上一轮
        poller_->poll(1000, activeChannels_);           // 等事件（阻塞）
        // ↓ 有事件才走到这里
        for (auto* channel : activeChannels_) {
            channel->handleEvent();                     // 分发
        }
        // ↓ 处理完，回到 while 开头，继续等下一轮
    }
}
```

### 5.2 updateChannel() —— 一双手套

```cpp
void EventLoop::updateChannel(Channel* channel) {
    poller_->updateChannel(channel);  // 什么都没做，直接转手
    // 为什么？EventLoop 是 coordinator，不是 executor
    // 只负责协调，不碰 epoll 细节
}
```

---

## 六、一次完整调用，从敲键盘到打印

```
你在键盘敲 "hello\n"

① Linux 内核检测到 fd=0（stdin）有数据可读

② epoll_wait（在 Poller::poll 里）解除阻塞，返回 n=1
   events_[0] = { .data.fd = 0, .events = EPOLLIN }

③ fillActiveChannels_()
   fd=0 → channels_[0] 找到 &stdinChan
   stdinChan.revents_ = EPOLLIN
   activeChannels = [ &stdinChan ]

④ 回到 EventLoop::loop() 的 for 循环
   stdinChan.handleEvent()

⑤ Channel::handleEvent()
   revents_ & kReadEvent → true（EPOLLIN 就是 1，kReadEvent 也是 1）
   readCallback_ 存在 → true
   → readCallback_() 调用！

⑥ 你的 lambda 执行：
   ::read(0, buf, 256) → "hello\n"
   std::cout << "读到: hello\n"

⑦ 回到 while 循环顶部，继续等下一轮事件
```

**全程的数据流：**

```
键盘 → fd=0
  → epoll_wait 返回 EPOLLIN
  → Poller 填 revents_
  → EventLoop 调 handleEvent
  → Channel 检查 revents_ & kReadEvent
  → 调 readCallback_（你的 lambda）
  → 读 stdin → 打印
```

---

## 七、类比你的 logger 项目

| my_logger | my_muduo | 关系 |
|---|---|---|
| `sink` | `Channel` | 一个输出目标 / 一个事件来源 |
| `logger` | `Poller` | 调度写入 / 调度 epoll |
| `main() 循环` | `EventLoop` | 驱动整体 / 驱动整体 |
| `log_msg` | `epoll_event` | 一条日志 / 一个事件 |
| `stdout_sink::log()` | `Channel::handleEvent()` | 具体输出 / 具体回调 |

---

## 八、接下来要写的

- **Socket** — RAII 封装 fd（自动 close）
- **Acceptor** — 监听端口，accept 新连接
- **TcpConnection** — 连接读写管理
- **Buffer** — 输入/输出缓冲区
- **TimerQueue** — 定时器

---

## 附录：完整类关系图

```
EventLoop (发动机)
  │
  ├── owns → Poller (epoll 封装)
  │            │
  │            ├── epollfd_ (epoll_create1 返回)
  │            ├── channels_ (map<int fd, Channel*>)
  │            └── events_ (epoll_wait 结果数组)
  │
  ├── knows → activeChannels_ (这一轮有事件的 Channel)
  │
  └── loop() while(true) { poll → dispatch }
       
Channel (fd 抽象)
  │
  ├── fd_      ← 来自 Socket/Acceptor
  ├── events_  ← 用户设的关心事件
  ├── revents_ ← Poller 填的实际事件
  ├── loop_    → 指向 EventLoop（update 时用）
  └── callbacks ← 用户设的回调函数
```
