// examples/enter.cpp
#include <iostream>
#include <memory>
#include "ring_packet_pool.h"
#include "image_info.h"
#include "packet_guard.h"
#include "module_base.h"

using namespace syncflow;

//生产者模块
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

// 消费者模块1,正常速度
class FastConsumer : public ModuleBase {
public:
    FastConsumer(RingPacketPool* pool, int frames_num, size_t consumer_id)
        : pool_(pool), frames_num_(frames_num), consumer_id_(consumer_id) {}

protected:
    void run() override {
        int consumed = 0;
        while (consumed < frames_num_) {
            Packet* pkt = pool_->CAcquire(consumer_id_);
            if (!pkt) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                continue;
            }
            auto guard = PacketGuard::acquire(pkt, consumer_id_);
            if (guard) {
                int frame = *(int*)(guard->image()->data);
                //模拟慢处理
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                std::cout << "Consumer[" << consumer_id_ << "] got frame: " << frame << std::endl;
                ++consumed;
            }
            pool_->CRelease();
        }
        std::cout << "Consumer[" << consumer_id_ << "] finished." << std::endl;
    }

private:
    RingPacketPool* pool_;
    int frames_num_;
    size_t consumer_id_;
};

// 消费者模块2,慢速
class SlowConsumer : public ModuleBase {
public:
    SlowConsumer(RingPacketPool* pool, int frames_num, size_t consumer_id, int skip_interval)
        : pool_(pool), frames_num_(frames_num), consumer_id_(consumer_id_), skip_interval_(skip_interval) {}

protected:
    void run() override {
        int consumed = 0;
        //获取当前帧序号
        int frames_count = 0;
        while (consumed < frames_num_ / skip_interval_) { 
            Packet* pkt = pool_->CAcquire(consumer_id_);
            if (!pkt) {
                // 获取失败，等待
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                continue;
            }
            int frame = *(int*)(pkt->image->data);
            if (frame % skip_interval_ == 0) {
                auto guard = PacketGuard::acquire(pkt, consumer_id_);
                if (guard) {
                    std::cout << "SlowConsumer[" << consumer_id_ << "] got frame: " << frame << std::endl;
                    ++consumed;
                }
            } 
           pool_->CRelease(consumer_id_);
        }
        std::cout << "SlowConsumer[" << consumer_id_ << "] finished." << std::endl;
    }

private:
    RingPacketPool* pool_;
    int frames_num_;
    int skip_interval_;
    size_t consumer_id_;
};

int main() {
    ImageInfo info;
    info.width = 1920;
    info.height = 1080;
    info.pixel_format = PixelFormat::RGB;  // 使用你定义的 RGB 格式
    info.size = 1920 * 1080 * 3;           // RGB 帧大小

    int total_frames = 60;
    int num_consumers = 3;
    int slow_skip = 3;

    RingPacketPool pool;
    pool.init(16, info, 1);

    CameraModule camera(&pool, 100);
    FastConsumer consumer1(&pool, total_frames, 0);
    FastConsumer consumer2(&pool, total_frames, 1);
    SlowConsumer consumer3(&pool, total_frames, 2, slow_skip);

    camera.start();
    consumer1.start();
    consumer2.start();
    consumer3.start();
    

    consumer1.stop();
    consumer2.stop();
    consumer3.stop();
    camera.stop();
    

    std::cout << "Module demo done!" << std::endl;
    return 0;
}