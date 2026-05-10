// ring_packet_pool.h
#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>



#include "image_buffer.h"
#include "image_info.h"
#include "status.h"

namespace syncflow {

class RingPacketPool {
public:
    // 无参构造：仅做轻量初始化，不分配资源
    RingPacketPool();

    // 预分配所有 Packet，绑定图像属性与消费者数量
    StatusCode init(size_t pool_size,
                const ImageInfo& image_attr,
                size_t num_consumers);

    // 禁止拷贝和移动
    RingPacketPool(const RingPacketPool&) = delete;
    RingPacketPool& operator=(const RingPacketPool&) = delete;
    RingPacketPool(RingPacketPool&&) = delete;
    RingPacketPool& operator=(RingPacketPool&&) = delete;

    ImageBuffer* PAcquire();
    void PRelease();

    ImageBuffer* CAcquire(size_t consumer_id);
    void CRelease(size_t consumer_id);

    size_t capacity() const { return pool_size_; }
    size_t consumer_count() const { return consumer_count_; }
    bool initialized() const { return initialized_; }

    uint64_t computeWatermark() const;

    unsigned get_write_index() const { return write_index_.load(std::memory_order_acquire); }

    void signal_consumer_ready() { 
        ready_consumers_.fetch_add(1, std::memory_order_release); 
    }

    void wait_all_consumers_ready() {
        while (ready_consumers_.load(std::memory_order_acquire) < consumer_count_) {
            std::this_thread::yield();
        }
    }

private:
    
    size_t pool_size_{0};
    bool initialized_{false};
    std::vector<ImageBuffer> ringpool_;
    std::atomic<unsigned> write_index_{0};
    std::unique_ptr<std::atomic<unsigned>[]> consumer_read_indices_;
    size_t consumer_count_{0};
    std::vector<uint8_t> continuous_memory_;
    std::atomic<size_t> ready_consumers_{0};
    
};

}