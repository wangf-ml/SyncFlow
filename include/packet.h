#pragma once

#include <atomic>

#include <image_buffer.h>
#include <memory>


namespace syncflow {

constexpr uint32_t kMaxConsumers = 6;

// 根据消费者数量生成“所有消费者均需处理”的掩码
inline uint32_t AllConsumersMask(size_t num_consumers) {
    return (num_consumers < kMaxConsumers) ? (1u << num_consumers) - 1u : ~0u;
}

/**
 * @brief 环形池中流转的数据包
 *
 * 每个槽位固定包含一个 ImageBuffer（帧数据载体）和同步元数据。
 * ImageBuffer::data 即模拟的连续 DMA 内存虚拟地址。
 */
struct Packet {

    Packet() = default;
    // ---------- 移动构造函数 ----------
    Packet(Packet&& other) noexcept
        : image(std::move(other.image)),
          consumer_mask(other.consumer_mask.load(std::memory_order_relaxed)),
          epoch(other.epoch),
          slot_read_index_(other.slot_read_index_.load(std::memory_order_relaxed))
    {
        // 重置源对象的原子变量
        other.consumer_mask.store(0, std::memory_order_relaxed);
        other.slot_read_index_.store(0, std::memory_order_relaxed);
    }

    // ---------- 移动赋值运算符 ----------
    Packet& operator=(Packet&& other) noexcept {
        if (this != &other) {
            image = std::move(other.image);
            consumer_mask.store(other.consumer_mask.load(std::memory_order_relaxed));
            other.consumer_mask.store(0, std::memory_order_relaxed);
            epoch = other.epoch;
            slot_read_index_.store(other.slot_read_index_.load(std::memory_order_relaxed));
            other.slot_read_index_.store(0, std::memory_order_relaxed);
        }
        return *this;
    }

    // 禁止拷贝
    Packet(const Packet&) = delete;
    Packet& operator=(const Packet&) = delete;

    // ------ 数据成员 ------
    std::shared_ptr<ImageBuffer> image;
    uint64_t epoch{0};
    std::atomic<unsigned> consumer_mask{0};
    std::atomic<unsigned> slot_read_index_{0};

    // ------ 已有方法 ------
    bool try_claim(uint32_t consumer_id) {
        uint32_t my_bit = 1u << consumer_id;
        uint32_t old_mask = consumer_mask.fetch_and(~my_bit, std::memory_order_acq_rel);
        return (old_mask & my_bit) != 0;
    }
};
} // namespace syncflow