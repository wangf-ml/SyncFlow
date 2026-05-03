int main() {
    ImageInfo info;
    info.width = 1920;
    info.height = 1080;
    info.pixel_format = PixelFormat::RGB;  // 使用你定义的 RGB 格式
    info.size = 1920 * 1080 * 3;           // RGB 帧大小

    Channel channel;
    channel.init(16, info, 3);

    // 创建具体模块（注入依赖由 Channel 负责）
    auto camera = std::make_unique<VirtualCamera>(/* 参数可选 */);
    channel.register_producer(std::move(camera));

    auto display0 = std::make_unique<Display>();
    display0->set_name("Display0");
    channel.register_consumer(std::move(display0), 0);

    auto display1 = std::make_unique<Display>();
    display1->set_name("Display1");
    channel.register_consumer(std::move(display1), 1);

    auto display2 = std::make_unique<Display>();
    display2->set_name("Display2");
    channel.register_consumer(std::move(display2), 2);

    // 启动全部模块
    channel.start_all();

    // 等待业务完成（简化：按固定时长或由 VirtualCamera 内部定时退出）
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 停止全部模块
    channel.stop_all();

    return 0;
}