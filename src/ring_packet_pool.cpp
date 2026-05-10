#include <vector>
#include <memory>
#include <atomic>

#include "ring_packet_pool.h"
#include "status.h"
#include "image_buffer.h"

namespace syncflow {

    RingPacketPool::RingPacketPool()
    {
    }

   StatusCode RingPacketPool::init(size_t pool_size, const ImageInfo &image_attr, 
                                        size_t num_consumers) 
    {
        if (initialized_) return StatusCode::OK;
        if (pool_size == 0 || num_consumers <= 0) return StatusCode::INVALID;

        try {
            // 每帧数据占用的字节数
            const size_t offset = image_attr.size;  

            // 预分配一整块连续内存
            continuous_memory_.resize(pool_size * offset);

            ringpool_.resize(pool_size);

            for (size_t i = 0; i < pool_size; ++i) {
                ringpool_[i].data   = continuous_memory_.data() + i * offset;
                ringpool_[i].offset   = offset;
                ringpool_[i].info   = image_attr;
            }

            consumer_read_indices_ = std::make_unique<std::atomic<unsigned>[]>(num_consumers);
            for (size_t i = 0; i < num_consumers; ++i) {
                consumer_read_indices_[i].store(0, std::memory_order_release);
            }
            consumer_count_ = num_consumers;
        } catch (const std::bad_alloc &) {
            return StatusCode::NO_RESOURCE;
        }

        pool_size_ = pool_size;
        write_index_.store(0, std::memory_order_release);
        initialized_ = true;
        return StatusCode::OK;
    }

    ImageBuffer* RingPacketPool::PAcquire() 
    {
        uint64_t w = write_index_.load(std::memory_order_acquire);
        uint64_t watermark = computeWatermark();
        if (w - watermark >= pool_size_) return nullptr;

        uint64_t idx = w % pool_size_;
        write_index_.store(w + 1, std::memory_order_release);
        return &ringpool_[idx];   // 直接返回池内对象的地址
    }

    void RingPacketPool::PRelease() 
    {
        uint64_t idx = (write_index_.load(std::memory_order_acquire) - 1) % pool_size_;
        uint32_t full_mask = ringpool_[idx].AllConsumersMask(consumer_count());
        ringpool_[idx].consumer_mask.store(full_mask, std::memory_order_release);
    }

    ImageBuffer* RingPacketPool::CAcquire(size_t consumer_id) 
    {
        uint64_t cur_read = consumer_read_indices_[consumer_id].load(std::memory_order_acquire);
        uint64_t cur_write = write_index_.load(std::memory_order_acquire);
        // 1. 判断队列是否为空
        if (cur_read >= cur_write) {
            return nullptr;
        }

        // 2. 计算槽位索引
        uint64_t idx = cur_read % pool_size_;
        ImageBuffer* buf = &ringpool_[idx];


        if (!buf->try_claim(static_cast<uint32_t>(consumer_id))) {
            //如果认领失败就找下一帧就会导致丢帧！！！
            //consumer_read_indices_[consumer_id].fetch_add(1, std::memory_order_release);
            return nullptr;
        }

        return buf;
    }

    void RingPacketPool::CRelease(size_t consumer_id) 
    {
        consumer_read_indices_[consumer_id].fetch_add(1, std::memory_order_release);
    }

    uint64_t RingPacketPool::computeWatermark() const 
    {
        if (consumer_count_ == 0) return write_index_.load(std::memory_order_acquire);
        uint64_t min_read = UINT64_MAX;
        for (size_t i = 0; i < consumer_count_; ++i) {
            uint64_t r = consumer_read_indices_[i].load(std::memory_order_acquire);
            if (r < min_read) min_read = r;
        }
        return min_read;
    }

}