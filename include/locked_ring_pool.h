#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

namespace syncflow {

struct LockedImageBuffer {
    char* data = nullptr;
    size_t size = 0;
    std::atomic<int> ref_count{0};

    LockedImageBuffer() = default;

    LockedImageBuffer(LockedImageBuffer&& other) noexcept
        : data(other.data),
          size(other.size),
          ref_count(other.ref_count.load(std::memory_order_relaxed)) {
        // 源对象置空，防止析构时误操作
        other.data = nullptr;
        other.size = 0;
        other.ref_count.store(0, std::memory_order_relaxed);
    }

    // 禁止拷贝
    LockedImageBuffer(const LockedImageBuffer&) = delete;
    LockedImageBuffer& operator=(const LockedImageBuffer&) = delete;
};

class LockedRingPool {
public:
    using Buffer = LockedImageBuffer;

    bool init(size_t pool_size, size_t frame_size, int num_consumers) {
        pool_size_ = pool_size;
        num_consumers_ = num_consumers;
        memory_.resize(pool_size * frame_size);
        buffers_.resize(pool_size);
        for (size_t i = 0; i < pool_size; ++i) {
            buffers_[i].data = memory_.data() + i * frame_size;
            buffers_[i].size = frame_size;
            buffers_[i].ref_count.store(0);
        }
        return true;
    }

    Buffer* acquire_producer() {
        std::unique_lock<std::mutex> lock(mutex_);
        // 等待有空闲槽位（引用计数为0）
        cv_not_full_.wait(lock, [this] {
            uint64_t w = write_idx_ % pool_size_;
            return buffers_[w].ref_count.load() == 0;
        });
        uint64_t idx = write_idx_ % pool_size_;
        write_idx_++;
        return &buffers_[idx];
    }

    void release_producer(Buffer* buf, int num_consumers) {
        std::lock_guard<std::mutex> lock(mutex_);
        buf->ref_count.store(num_consumers);         // 设置消费者总数
        cv_not_empty_.notify_all();                   // 唤醒所有消费者
    }

    Buffer* acquire_consumer(int consumer_id) {
        std::unique_lock<std::mutex> lock(mutex_);
        // 等待有新帧（我的读索引落后于写索引，且该帧未被消费完）
        cv_not_empty_.wait(lock, [this, consumer_id] {
            return read_indices_[consumer_id] < write_idx_ &&
                   buffers_[read_indices_[consumer_id] % pool_size_].ref_count.load() > 0;
        });
        return &buffers_[read_indices_[consumer_id] % pool_size_];
    }

    void release_consumer(int consumer_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& buf = buffers_[read_indices_[consumer_id] % pool_size_];
        int prev = buf.ref_count.fetch_sub(1);
        if (prev == 1) {                      // 最后一个消费者
            cv_not_full_.notify_one();        // 生产者可写
        }
        read_indices_[consumer_id]++;
        cv_not_empty_.notify_all();
    }

    void set_consumer_count(int n) {
        read_indices_.resize(n);
        for (auto& v : read_indices_) v = 0;
    }

private:
    size_t pool_size_ = 0;
    int num_consumers_ = 0;
    std::vector<Buffer> buffers_;
    std::vector<char> memory_;
    std::atomic<uint64_t> write_idx_{0};
    std::vector<uint64_t> read_indices_;   // 普通变量，锁保护

    std::mutex mutex_;
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;
};

}