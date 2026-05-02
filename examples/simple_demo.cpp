#include <iostream>
#include <thread>
#include "ring_packet_pool.h"
#include "image_info.h"

using namespace syncflow;

int main() {
    // 1. 准备图像信息
    ImageInfo info;
    info.width = 1920; info.height = 1080;
    info.pixel_format_ = syncflow::PixelFormat::RGB;
    info.size = 1920 * 1080 * 3;

    // 2. 初始化环形池：4 个槽位，1 个消费者
    RingPacketPool pool;
    pool.init(4, info, 1);

    // 3. 生产者线程
    std::thread producer([&] {
        for (int i = 0; i < 10; ++i) {
            Packet* pkt = pool.PAcquire();
            if (!pkt) {
                std::this_thread::yield();
                --i;
                continue;
            }
            // 写入模拟帧号
            *(int*)(pkt->image->data) = i;
            pool.PRelease();
            std::cout << "Produced: " << i << std::endl;
        }
    });

    // 4. 消费者线程
    std::thread consumer([&] {
        for (int i = 0; i < 10; ++i) {
            Packet* pkt = pool.CAcquire(0);
            if (!pkt) {
                std::this_thread::yield();
                --i;
                continue;
            }
            int frame = *(int*)(pkt->image->data);
            pool.CRelease(0);
            std::cout << "Consumed: " << frame << std::endl;
        }
    });

    producer.join();
    consumer.join();
    std::cout << "Demo done!" << std::endl;
    return 0;
}