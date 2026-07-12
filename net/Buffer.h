// Buffer.h — 网络 I/O 缓冲区
//
// 核心设计：双索引 readerIndex_ / writerIndex_ 划分可读空间
//   [ 已读过的 | 还没读的 | 空闲空间 ]
//   ↑           ↑           ↑
//   0      readerIndex_  writerIndex_
//
// retrieve 只移 reader，不搬数据，O(1) 性能。
// readFd 使用 readv + 栈上 extrabuf，一次系统调用清空内核缓冲区。

#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <string.h>

class Buffer {
public:
    static const size_t kInitialSize = 1024;  ///< 初始容量 1KB

    Buffer();

    // === 输入方向（从 fd 读到 Buffer） ===

    /**
     * 从 fd 读数据到 Buffer（readv，高性能）
     * @return 实际读到的字节数，0 = 对端关闭
     */
    ssize_t readFd(int fd);

    // === 取数据（交给上层） ===

    /** @return 可读数据的起始指针 */
    const char* peek() const;
    /** @return 可读数据长度 */
    size_t readableBytes() const;
    /** 取走 len 字节（只移 reader，不删数据） */
    void retrieve(size_t len);
    /** 全部取走（reader/writer 归零） */
    void retrieveAll();
    /** 取走 len 字节并转为 std::string */
    std::string retrieveAsString(size_t len);
    /** 全部取走并转为 std::string */
    std::string retrieveAllAsString();

    // === 协议解析 ===

    /** 在可读数据中查找 \r\n，找不到返回 nullptr（半包处理） */
    const char* findCRLF() const;

    // === 写方向（上层塞进来） ===

    /** 向 writer 位置追加数据，空间不够自动扩容 */
    void append(const char* data, size_t len);
    void append(const std::string& str);

private:
    std::vector<char> buffer_;
    size_t readerIndex_;  ///< "读到哪了"（peek/retrieve 从这里开始）
    size_t writerIndex_;  ///< "写到哪了"（readFd/append 从这里开始写）

    char* begin() { return buffer_.data(); }
    const char* begin() const { return buffer_.data(); }
    char* beginWrite() { return begin() + writerIndex_; }
    const char* beginWrite() const { return begin() + writerIndex_; }
};
