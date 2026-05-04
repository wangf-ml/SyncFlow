#include <iostream>
#include <memory>
#include "ring_packet_pool.h"
#include "image_info.h"
#include "packet_guard.h"
#include "virtual_camera.h"
#include "display.h"
#include "channel.h"
 
using namespace syncflow;
int main() {
    ImageInfo info;
    info.width = 1920;
    info.height = 1080;
    info.pixel_format = PixelFormat::RGB;  // 使用你定义的 RGB 格式
    info.size = 1920 * 1080 * 3;           // RGB 帧大小

    Channel channel;
    channel.init(5, info, 3);

    // 创建具体模块（注入依赖由 Channel 负责）
    auto camera = std::make_unique<syncflow::modules::VirtualCamera>();
    camera->set_total_frames(30);
    auto* cam_raw = camera.get();
    channel.register_producer(std::move(camera));
    

    auto display0 = std::make_unique<syncflow::modules::Display>();
    display0->set_name("Display0");
    channel.register_consumer(std::move(display0), 0);

    auto display1 = std::make_unique<syncflow::modules::Display>();
    display1->set_name("Display1");
    channel.register_consumer(std::move(display1), 1);

    auto display2 = std::make_unique<syncflow::modules::Display>();
    display2->set_name("Display2");
    channel.register_consumer(std::move(display2), 2);

    // 启动全部模块
    channel.start_all();

    int cur_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    while (!cam_raw->is_done()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    //给消费者一点时间消费完残帧
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - cur_time;

    std::cout << "Total time: " << total_time << " ms" << std::endl;
    // 停止全部模块
    channel.stop_all();

    return 0;
}