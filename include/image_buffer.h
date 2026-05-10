#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "image_info.h"
#include "config.h"


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

    std::atomic<uint32_t> consumer_mask{0};
    uint64_t epoch{0}; 

    // DMA-BUF 文件描述符，-1 表示无效
    int dma_buf_fd{-1};               

     // 默认构造函数（保留）
    ImageBuffer() = default;

    // 禁止拷贝
    ImageBuffer(const ImageBuffer&) = delete;
    ImageBuffer& operator=(const ImageBuffer&) = delete;

    // 手动移动构造函数
    ImageBuffer(ImageBuffer&& other) noexcept
        : data(other.data),
          offset(other.offset),
          info(std::move(other.info)),
          consumer_mask(other.consumer_mask.load(std::memory_order_relaxed)),
          epoch(other.epoch)
    {
        // 将源对象重置为安全的默认状态
        other.data = nullptr;
        other.offset = 0;
        other.consumer_mask.store(0, std::memory_order_relaxed);
        other.epoch = 0;
    }

    // 手动移动赋值运算符
    ImageBuffer& operator=(ImageBuffer&& other) noexcept {
        if (this != &other) {
            data = other.data;
            offset = other.offset;
            info = std::move(other.info);
            consumer_mask.store(other.consumer_mask.load(std::memory_order_relaxed));
            other.consumer_mask.store(0, std::memory_order_relaxed);
            epoch = other.epoch;
            other.epoch = 0;
            other.data = nullptr;
            other.offset = 0;
        }
        return *this;
    }

    bool try_claim(uint32_t consumer_id) {
        uint32_t my_bit = 1u << consumer_id;
        uint32_t old_mask = consumer_mask.fetch_and(~my_bit, std::memory_order_acq_rel);
        return (old_mask & my_bit) != 0;
    }

    inline uint32_t AllConsumersMask(size_t num_consumers) {
    return (num_consumers < kMaxConsumers) ? (1u << num_consumers) - 1u : ~0u;
}
};
}

