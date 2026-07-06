#pragma once
#include <vector>
#include <memory>

class Channel;
class Poller;

class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    
    void loop();

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    std::unique_ptr<Poller> poller_;
    std:vector<Channel*> activeChannels_;
    bool quit_;
};