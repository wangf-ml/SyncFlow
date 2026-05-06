#include <gtest/gtest.h>

#include "ring_packet_pool.h"
#include "image_info.h"
#include "status.h"
#include "packet_guard.h"

using namespace syncflow;

// 辅助函数：生成一个简单的 ImageInfo
ImageInfo MakeTestImageInfo() {
    ImageInfo info;
    info.width  = 1920;
    info.height = 1080;
    info.pixel_format = PixelFormat::RGB;
    info.stride = 1920;
    info.size   = 1920 * 1080 * 3;  // RGB 帧大小
    return info;
}

// 测试1：初始化成功
TEST(RingPacketPoolTest, InitSucceeds) {
    RingPacketPool pool;
    auto info = MakeTestImageInfo();
    StatusCode status = pool.init(4, info, 2);   // 4 个槽位，2 个消费者
    EXPECT_EQ(status, StatusCode::OK);
    EXPECT_TRUE(pool.initialized());
}

// 测试2：重复初始化应该返回 OK（或 AlreadyInitialized）
TEST(RingPacketPoolTest, DoubleInit) {
    RingPacketPool pool;
    auto info = MakeTestImageInfo();
    pool.init(4, info, 2);
    StatusCode status = pool.init(4, info, 2);   // 第二次初始化
    EXPECT_EQ(status, StatusCode::OK);            // 根据你的实现，可能是 OK 或其他
}

// 测试3：参数无效（pool_size 为 0）应返回错误
TEST(RingPacketPoolTest, InitFailsWithZeroPoolSize) {
    RingPacketPool pool;
    auto info = MakeTestImageInfo();
    StatusCode status = pool.init(0, info, 2);
    EXPECT_NE(status, StatusCode::OK);
}

// 测试4：生产者获取-发布-消费者获取的基本流程
TEST(RingPacketPoolTest, BasicProducerConsumer) {
    RingPacketPool pool;
    auto info = MakeTestImageInfo();
    ASSERT_EQ(pool.init(4, info, 1), StatusCode::OK);  // 1 个消费者

    // 生产者获取一个 Packet
    Packet* pkt = pool.PAcquire();
    ASSERT_NE(pkt, nullptr);

    // 发布
    pool.PRelease();

    // 消费者获取
    Packet* rcv = pool.CAcquire(0);
    ASSERT_NE(rcv, nullptr);
    // 此时应该拿到了生产者发布的同一个 Packet
    EXPECT_EQ(rcv, pkt);  // 因为是同一个槽位

    // 消费者释放
    pool.CRelease(0);
}

// 测试5：队列满时生产者获取应返回 nullptr
TEST(RingPacketPoolTest, PoolFullReturnsNull) {
    RingPacketPool pool;
    auto info = MakeTestImageInfo();
    const size_t kSlots = 2;
    ASSERT_EQ(pool.init(kSlots, info, 1), StatusCode::OK);

    // 填满池子（生产者写入 kSlots 次）
    for (size_t i = 0; i < kSlots; ++i) {
        Packet* pkt = pool.PAcquire();
        ASSERT_NE(pkt, nullptr);
        pool.PRelease();
    }

    // 现在再尝试获取，应该返回 nullptr（池满）
    Packet* pkt = pool.PAcquire();
    EXPECT_EQ(pkt, nullptr);
}

// 测试6：多消费者并发认领（单线程模拟）
TEST(RingPacketPoolTest, MultiConsumerClaim) {
    RingPacketPool pool;
    auto info = MakeTestImageInfo();
    const size_t kConsumers = 3;
    ASSERT_EQ(pool.init(4, info, kConsumers), StatusCode::OK);

    // 生产者发布一帧
    Packet* pkt = pool.PAcquire();
    ASSERT_NE(pkt, nullptr);
    pool.PRelease();

    // 每个消费者依次获取并处理
    for (size_t i = 0; i < kConsumers; ++i) {
        Packet* rcv = pool.CAcquire(i);
        ASSERT_NE(rcv, nullptr);
        // 使用 PacketGuard 认领
        auto guard = PacketGuard::acquire(rcv, static_cast<uint32_t>(i));
        EXPECT_TRUE(guard) << "Consumer " << i << " should claim successfully";
        // 推进读指针
        pool.CRelease(i);
    }

    // 所有消费者释放
    for (size_t i = 0; i < kConsumers; ++i) {
        EXPECT_EQ(pool.CAcquire(i), nullptr);
    }
}