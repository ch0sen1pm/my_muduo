#include "Buffer.h"
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

Buffer::Buffer()
    : buffer_(kInitialSize)
    , readerIndex_(0)
    , writerIndex_(0)
{}

// readv: 一次系统调用读到多块内存
// vec[0] → buffer 空闲区，vec[1] → 栈上 64KB 溢出区
// 先填 buffer，满了溢出到 extrabuf，再 append 回来
// 好处：一次 readv 清空内核缓冲区，不用等下一次 epoll
ssize_t Buffer::readFd(int fd) {
    char extrabuf[65536];
    struct iovec vec[2];

    size_t writable = buffer_.size() - writerIndex_;
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    ssize_t n = ::readv(fd, vec, 2);

    if (n < 0) {
        return n;
    } else if (static_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    } else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}

// 双索引设计（核心！）：reader 和 writer 都是整数索引
//   [ 已读过的 | 还没读的 | 空闲空间 ]
//   ↑           ↑           ↑
//   0      readerIndex_  writerIndex_
// retrieve 只移 reader 不删数据，O(1) 无需搬内存
const char* Buffer::peek() const {
    return begin() + readerIndex_;
}

size_t Buffer::readableBytes() const {
    return writerIndex_ - readerIndex_;
}

void Buffer::retrieve(size_t len) {
    if (len < readableBytes()) {
        readerIndex_ += len;
    } else {
        retrieveAll();
    }
}

void Buffer::retrieveAll() {
    readerIndex_ = 0;
    writerIndex_ = 0;
}

std::string Buffer::retrieveAsString(size_t len) {
    std::string result(peek(), len);
    retrieve(len);
    return result;
}

std::string Buffer::retrieveAllAsString() {
    return retrieveAsString(readableBytes());
}

// 在可读数据中找 \r\n，用于按行切消息（Telnet/HTTP 协议）
// 返回 nullptr = 还没攒够一行，继续等
const char* Buffer::findCRLF() const {
    const char* crlf = std::search(
        peek(),
        beginWrite(),
        "\r\n",
        "\r\n" + 2);
    return crlf == beginWrite() ? nullptr : crlf;
}

void Buffer::append(const char* data, size_t len) {
    if (buffer_.size() - writerIndex_ < len) {
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_,
                  begin() + writerIndex_,
                  begin());
        readerIndex_ = 0;
        writerIndex_ = readable;
    }

    if (buffer_.size() - writerIndex_ < len) {
        buffer_.resize(writerIndex_ + len);
    }

    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
}

void Buffer::append(const std::string& str) {
    append(str.data(), str.size());
}
