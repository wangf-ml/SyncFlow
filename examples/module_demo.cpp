// examples/enter.cpp
#include <iostream>
#include <memory>
#include "ring_packet_pool.h"
#include "image_info.h"
#include "packet_guard.h"
#include "module_base.h"

using namespace syncflow;

// ---------- Camera 生产者模块 ----------
class CameraModule : public ModuleBase {
public:
    CameraModule(RingPacketPool* pool, int frames_num)
        : pool_(pool), frames_num_(frames_num) {}

protected:
    void run() override {
        for (int i = 0; i < frames_num_; ) {
            Packet* pkt = pool_->PAcquire();
            if (!pkt) {
                std::this_thread::yield();
                continue;
            }
            *(int*)(pkt->image->data) = i;
            pool_->PRelease();
            std::cout << "Produced: " << i << std::endl;
            ++i;  // 只在获取成功时才递增
        }
    }

private:
    RingPacketPool* pool_;
    int frames_num_;
};

// ---------- Display 消费者模块 ----------
class DisplayModule : public ModuleBase {
public:
    DisplayModule(RingPacketPool* pool, int frames_num)
        : pool_(pool), frames_num_(frames_num) {}

protected:
    void run() override {
        int consumed = 0;
        while (consumed < frames_num_) {
            Packet* pkt = pool_->CAcquire(0);
            if (!pkt) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                continue;
            }
            auto guard = PacketGuard::acquire(pkt, 0);
            if (guard) {
                int frame = *(int*)(guard->image()->data);
                std::cout << "Consumed: " << frame << std::endl;
                ++consumed;
            }
            pool_->CRelease(0);
        }
    }

private:
    RingPacketPool* pool_;
    int frames_num_;
};


int main() {
    ImageInfo info;
    info.width = 1920;
    info.height = 1080;
    info.pixel_format = PixelFormat::RGB;  // 使用你定义的 RGB 格式
    info.size = 1920 * 1080 * 3;           // RGB 帧大小

    RingPacketPool pool;
    pool.init(8, info, 1);

    CameraModule camera(&pool, 100);
    DisplayModule display(&pool, 100);

    camera.start();
    display.start();

    camera.stop();
    display.stop();

    std::cout << "Module demo done!" << std::endl;
    return 0;
}