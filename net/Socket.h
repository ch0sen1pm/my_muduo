// Socket.h — RAII socket fd 包装
//
// 核心概念：Socket 拥有 fd（创建/关闭），Channel 观察 fd。
// 只允许移动，不允许拷贝——fd 是独占资源，不能有两个主人。

#pragma once
#include <netinet/in.h>

class Socket {
public:
    /** 接管已有的 socket fd */
    explicit Socket(int sockfd);
    /** RAII 析构：自动 close(fd) */
    ~Socket();

    /** @return socket 文件描述符 */
    int fd() const { return sockfd_; }

    /** 绑定地址和端口 */
    void bindAddress(const sockaddr_in& addr);
    /** 转为监听 socket，开始接受连接 */
    void listen();
    /** 设置 SO_REUSEADDR，允许重用 TIME_WAIT 端口 */
    void setReuseAddr(bool on);

    /** 工厂方法：创建非阻塞 TCP socket（IPv4） */
    static Socket createTcp();

    // 移动语义（独占资源，只移不拷）
    Socket(Socket&&) noexcept;
    Socket& operator=(Socket&&) noexcept;
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

private:
    int sockfd_;
};
