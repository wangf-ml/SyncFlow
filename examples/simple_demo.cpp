#include <iostream>
#include <thread>
#include "ring_packet_pool.h"
#include "image_info.h"
#include "packet_guard.h"

using namespace syncflow;
int main() {
    ImageInfo info;
    info.width = 1920; info.height = 1080;
    info.pixel_format = PixelFormat::RGB;
    info.size = 1920 * 1080 * 3;

    RingPacketPool pool;
    pool.init(8, info, 1);

    // 生产者线程（不变）
    std::thread producer([&] {
        for (int i = 0; i < 10; ++i) {
            Packet* pkt = pool.PAcquire();
            if (!pkt) {
                std::this_thread::yield();
                --i;
                continue;
            }
            *(int*)(pkt->image->data) = i;
            pool.PRelease();
            std::cout << "Produced: " << i << std::endl;
        }
    });

    // 消费者线程（使用 PacketGuard，极大简化）
    std::thread consumer([&] {
        for (int i = 0; i < 10; ++i) {
            Packet* pkt = pool.CAcquire(0);
            if (!pkt) {
                std::this_thread::yield();
                --i;
                continue;
            }
            
            // 使用 PacketGuard 自动管理同步
            auto guard = PacketGuard::acquire(pkt, 0);
            if (!guard) {
                // 已被处理过，跳过
                pool.CRelease(0);
                --i;
                continue;
            }
            
            int frame = *(int*)(guard->image()->data);
            pool.CRelease(0);
            std::cout << "Consumed: " << frame << std::endl;
            
            // guard 析构自动释放
        }
    });

    producer.join();
    consumer.join();
    std::cout << "Demo with PacketGuard done!" << std::endl;
    return 0;
}