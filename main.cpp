// 第一个 muduo demo：监听 stdin（fd=0）
// 键盘敲回车 → epoll 返回 → channel.handleEvent() → 打印信息
//
// 流程：
//   EventLoop loop
//   Channel stdinChan(&loop, 0)         // fd=0 是标准输入
//   stdinChan.setReadCallback(回调)      // 设好：可读时干嘛
//   stdinChan.enableReading()           // 告诉 epoll：监听 fd=0
//   loop.loop()                         // 死循环，等事件

#include "EventLoop.h"
#include "Channel.h"
#include <iostream>
#include <unistd.h>

int main() {
    EventLoop loop;

    // fd=0 就是 stdin（标准输入）
    Channel stdinChan(&loop, 0);

    // 设回调：stdin 可读时调这个
    stdinChan.setReadCallback([&]() {
        char buf[256];
        ssize_t n = ::read(0, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            std::cout << "[main] stdin 触发！读到了: " << buf;
        } else {
            // n <= 0 说明 EOF（Ctrl+D），退出
            std::cout << "[main] stdin 关闭，退出" << std::endl;
            exit(0);
        }
    });

    stdinChan.enableReading();  // 注册到 epoll

    std::cout << "输入文字然后回车，Ctrl+D 退出..." << std::endl;
    loop.loop();  // 死循环，等事件
}
