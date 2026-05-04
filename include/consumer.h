#pragma once

#include "module_base.h"
#include "ring_packet_pool.h"
#include "packet_guard.h"
#include "packet.h"

namespace syncflow {
class Consumer : public ModuleBase {
public:
    Consumer() = default;
    virtual ~Consumer() override = default;

    void set_pool(RingPacketPool* pool) { pool_ = pool; }
    void set_consumer_id(size_t consumer_id) { consumer_id_ = consumer_id; }

protected:
    //确保帧已认领
    virtual void consume(const PacketGuard& guard) = 0;

    void run() override {
        if (!pool_) {
            // 后续补充日志输出
            return;  // 没有绑定 RingPacketPool，无法生产
        }
        while (is_running()) {
            Packet* pkt = pool_->CAcquire(consumer_id_);
            if (!pkt) {
                //std::this_thread::sleep_for(std::chrono::milliseconds(1));
                std::this_thread::yield();
                continue;
            }
            auto guard = PacketGuard::acquire(pkt, consumer_id_);
            if (guard) {
                consume(*guard);
            }
            pool_->CRelease(consumer_id_);
        }
    }

    size_t consumer_id() const { return consumer_id_; }
private:
    RingPacketPool* pool_{nullptr};
    size_t consumer_id_{0};
};
}