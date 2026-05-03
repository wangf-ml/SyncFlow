#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "image_info.h"


namespace syncflow {
/**
 * @brief 图像数据缓冲区
 * 
 * 参照NVIDIA NvSciBuf：封装DMA内存句柄，为未来支持零拷贝共享。
 */
struct ImageBuffer {
    void* data{nullptr};
    // 物理地址0表示无效
    uint64_t phy_addr{0};
    
    ImageInfo info;
    size_t offset{0};

    // DMA-BUF 文件描述符，-1 表示无效
    int dma_buf_fd{-1};               

    ImageBuffer(const ImageBuffer&) = delete;
    ImageBuffer& operator=(const ImageBuffer&) = delete;
    ImageBuffer(ImageBuffer&&) = default;
    ImageBuffer& operator=(ImageBuffer&&) = default;

    ImageBuffer() = default;
};
}

