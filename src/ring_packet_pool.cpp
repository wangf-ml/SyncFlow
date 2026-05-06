#include <vector>
#include <memory>
#include <atomic>

#include "ring_packet_pool.h"
#include "status.h"
#include "image_buffer.h"
#include "packet.h"

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
                auto* buf = new ImageBuffer();
                buf->data   = continuous_memory_.data() + i * offset;
                buf->offset   = offset;
                buf->info   = image_attr;

                std::shared_ptr<ImageBuffer> img_ptr(
                    buf, [this, i](ImageBuffer* ptr) {
                        ringpool_[i].slot_read_index_.fetch_add(1, std::memory_order_release);
                        cv_.notify_one();
                        delete ptr;
                    }
                );

                ringpool_[i].image = std::move(img_ptr);
            }

            write_index_.store(0, std::memory_order_release); 
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

    Packet* RingPacketPool::PAcquire() 
    {
        uint64_t w = write_index_.fetch_add(1, std::memory_order_acq_rel);
        uint64_t min_read = computeWatermark(); // 最慢消费者进度

        // 队列满判断：写索引领先最慢消费者整整一圈
        if (w - min_read >= pool_size_) {
            return nullptr;
        }

        // 直接返回对应的槽位（此时一定可安全覆盖）
        return &ringpool_[w % pool_size_];
    }

    void RingPacketPool::PRelease() 
    {
        uint64_t idx = (write_index_.load(std::memory_order_acquire) - 1) % pool_size_;
        ringpool_[idx].consumer_mask.store(AllConsumersMask(consumer_count()), std::memory_order_release);
        //后续补充日志打印mask位图信息
    }

    Packet* RingPacketPool::CAcquire(size_t consumer_id) {
        uint64_t cur_read = consumer_read_indices_[consumer_id].load(std::memory_order_acquire);
        uint64_t cur_write = write_index_.load(std::memory_order_acquire);
        // 1. 判断队列是否为空
        if (cur_read >= cur_write) {
            return nullptr;
        }

        // 2. 计算槽位索引
        uint64_t idx = cur_read % pool_size_;
        auto& pkt = ringpool_[idx];

        // if (!pkt.try_claim(static_cast<uint32_t>(consumer_id))) {
        //     //已处理过
        //     return nullptr;
        // }

        return &pkt;
    }

    void RingPacketPool::CRelease(size_t consumer_id) 
    {
        consumer_read_indices_[consumer_id].fetch_add(1, std::memory_order_release);
    }

    uint64_t RingPacketPool::computeWatermark() const 
    {
        uint64_t min_read = UINT64_MAX;
        for (size_t i = 0; i < consumer_count_; ++i) {
            uint64_t r = consumer_read_indices_[i].load(std::memory_order_acquire);
            if (r < min_read) min_read = r;
        }
        return min_read;
    }

    void RingPacketPool::shutdown() 
    {
        std::unique_lock<std::mutex> lock(cv_mutex_);
        cv_.notify_all();
    }

    void RingPacketPool::wait_for_available(uint32_t timeout_ms) 
    {
        std::unique_lock<std::mutex> lock(cv_mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms));
    }

}