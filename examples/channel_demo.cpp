#include <memory>
#include<chrono>

#include "ring_packet_pool.h"
#include "image_info.h"
#include "virtual_camera.h"
#include "display.h"
#include "channel.h"
#include "logger.h"
 
using namespace syncflow;
int main() {
    syncflow::init_log("./syncflow.log");

    ImageInfo info;
    info.width = 1920;
    info.height = 1080;
    info.pixel_format = PixelFormat::RGB;  // 使用你定义的 RGB 格式
    info.size = 1920 * 1080 * 3;           // RGB 帧大小

    Channel channel;
    channel.init(5, info, 3);

    // 创建具体模块（注入依赖由 Channel 负责）

    auto display0 = std::make_unique<syncflow::modules::Display>();
    display0->set_name("Display0");
    channel.register_consumer(std::move(display0), 0);

    auto display1 = std::make_unique<syncflow::modules::Display>();
    display1->set_name("Display1");
    channel.register_consumer(std::move(display1), 1);

    auto display2 = std::make_unique<syncflow::modules::Display>();
    display2->set_name("Display2");
    channel.register_consumer(std::move(display2), 2);

    auto camera = std::make_unique<syncflow::modules::VirtualCamera>();
    camera->set_total_frames(30);
    auto* cam_raw = camera.get();
    channel.register_producer(std::move(camera));

    auto start_time = std::chrono::steady_clock::now();

    // 启动全部模块
    channel.start_all();

    while (!cam_raw->is_done()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    //给消费者一点时间消费完残帧
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    
    // 停止全部模块
    channel.stop_all();

    auto end_time = std::chrono::steady_clock::now();
    int total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    SYNC_LOG("Total time: " << total_time << " ms");

    syncflow::close_log();
    return 0;
}