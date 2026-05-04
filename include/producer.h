#pragma once

#include "module_base.h"
#include "ring_packet_pool.h"
#include "packet.h"

namespace syncflow { 
class Producer : public ModuleBase {
public:
    Producer() = default;
    virtual ~Producer() override = default;

    void set_pool(RingPacketPool* pool) { pool_ = pool; }
    
protected:
    virtual bool produce(Packet* pkt) = 0;  // 纯虚函数，具体生产逻辑由子类实现
    void run() override {
        if (!pool_) {
            return;  // 没有绑定 RingPacketPool，无法生产
        }
        while (is_running()) {
            Packet* pkt = pool_->PAcquire();
            if (!pkt) {
                std::this_thread::yield();  // 没有可用槽位，稍后重试
                continue;
            }
            if (!produce(pkt)) {
                pkt->consumer_mask.store(0, std::memory_order_release);
                break;
            }
            pool_->PRelease();
        }
    }
private:
    RingPacketPool* pool_{nullptr};
};
}