#pragma once

#include "module_base.h"
#include "ring_packet_pool.h"
#include "image_buffer.h"

namespace syncflow {
class Consumer : public ModuleBase {
public:
    Consumer() = default;
    virtual ~Consumer() override = default;

    void set_pool(RingPacketPool* pool) { pool_ = pool; }
    void set_consumer_id(size_t consumer_id) { consumer_id_ = consumer_id; }

protected:
    //确保帧已认领
    virtual void consume(const ImageBuffer* buf) = 0;

    void run() override {
        if (!pool_) {
            // 后续补充日志输出
            return;  // 没有绑定 RingPacketPool，无法生产
        }
        bool reported  = false;
        while (is_running()) {
            
            if (!reported) {
                pool_->signal_consumer_ready();   // 原子通知：我已进入主循环
                reported = true;
            }

            ImageBuffer* buf = pool_->CAcquire(consumer_id_);
            if (!buf) {
                std::this_thread::yield();  // 无新帧或认领失败，稍后重试
                continue;
            }

           consume(buf);

            // 处理完后释放槽位，推进读索引，通知生产者
            pool_->CRelease(consumer_id_);
        }
    }

    size_t consumer_id() const { return consumer_id_; }
private:
    RingPacketPool* pool_{nullptr};
    size_t consumer_id_{0};
};
}