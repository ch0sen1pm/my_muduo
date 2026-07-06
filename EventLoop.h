// EventLoop.h
// 事件循环 = Poller + while(true)
//
// 它拥有一个 Poller，loop() 里死循环：
//   poller_.poll() → 拿到活跃 Channel → 逐个 handleEvent()

#pragma once
#include <vector>
#include <memory>

class Channel;
class Poller;

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    // 启动循环，一般不退出
    void loop();

    // Channel::update() 最终调到这里 → 转给 Poller
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    std::unique_ptr<Poller> poller_;
    std::vector<Channel*> activeChannels_;
    bool quit_;
};
