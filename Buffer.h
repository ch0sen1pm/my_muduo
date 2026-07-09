#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <string.h>

class Buffer {
public:
    static const size_t kInitialSize = 1024;

    Buffer();

    ssize_t readFd(int fd);

    const char* peek() const;
    size_t readableBytes() const;
    void retrieve(size_t len);
    void retrieveAll();
    std::string retrieveAsString(size_t len);
    std::string retrieveAllAsString();

    const char* findCRLF() const;

    void append(const char* data, size_t len);
    void append(const std::string& str);

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    char* begin() { return buffer_.data(); }
    const char* begin() const { return buffer_.data(); }

    char* beginWrite() { return begin() + writerIndex_; }
    const char* beginWrite() const { return begin() + writerIndex_; }
};