#pragma once

#include "module_base.h"
#include "ring_packet_pool.h"

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
            // 后续补充日志输出
            return;  // 没有绑定 RingPacketPool，无法生产
        }
        while (is_running()) {
            Packet* pkt = pool_->PAcquire();
            if (!pkt) {
                std::this_thread::yield();  // 没有可用槽位，稍后重试
                continue;
            }
            if (produce(pkt)) {
                pool_->PRelease();  // 成功生产，发布
            } else {
                // 失败，不发布
                //后续补充打印日志逻辑
                pool_->PRelease();  // 释放占用的槽位
            }
        }
    }
private:
    RingPacketPool* pool_{nullptr};
};
}