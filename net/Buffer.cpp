#include "Buffer.h"
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

Buffer::Buffer()
    : buffer_(kInitialSize)
    , readerIndex_(0)
    , writerIndex_(0)
{}

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
