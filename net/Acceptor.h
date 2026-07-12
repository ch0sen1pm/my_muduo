// Acceptor.h — 监听 socket + accept 封装
//
// 把 Socket/Channel 粘成单一职责的门童：
//   创建监听 socket → bind → listen → epoll 注册 → accept → 回调通知

#pragma once
#include "Socket.h"
#include "../core/Channel.h"
#include <functional>
#include <netinet/in.h>

class EventLoop;

class Acceptor {
public:
    /// @param connfd  accept 返回的新连接 fd
    /// @param peerAddr 客户端的 IP 和端口
    using NewConnectionCallback =
        std::function<void(int connfd, const sockaddr_in& peerAddr)>;

    /**
     * @param loop       所属 EventLoop
     * @param listenAddr 要绑定的地址（IP + 端口）
     */
    Acceptor(EventLoop* loop, const sockaddr_in& listenAddr);
    ~Acceptor();

    /** 设置新连接到来时的回调 */
    void setNewConnectionCallback(NewConnectionCallback cb);
    /** 开始监听：::listen() + Channel::enableReading() */
    void listen();

private:
    /** 监听 fd 可读 → accept → 调 newConnectionCallback_ */
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;          ///< 监听 socket（RAII 持有 fd）
    Channel acceptChannel_;        ///< 盯监听 fd 的可读事件
    NewConnectionCallback newConnectionCallback_;
};
