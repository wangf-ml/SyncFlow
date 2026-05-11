#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include "ring_packet_pool.h"
#include "locked_ring_pool.h"
#include "image_info.h"

using namespace syncflow;

constexpr int TOTAL_FRAMES = 50;
constexpr int POOL_SIZE = 5;
constexpr int NUM_CONSUMERS = 3;

// ==================== SyncFlow 基准 ====================
double bench_syncflow() {
    RingPacketPool pool;
    ImageInfo info;
    info.size = 1920 * 1080 * 3;
    info.width = 1920; info.height = 1080;
    info.pixel_format = PixelFormat::RGB;
    pool.init(POOL_SIZE, info, NUM_CONSUMERS);

    auto start = std::chrono::steady_clock::now();

    std::thread producer([&] {
        for (int i = 0; i < TOTAL_FRAMES; ++i) {
            ImageBuffer* buf = pool.PAcquire();
            while (!buf) { buf = pool.PAcquire(); std::this_thread::yield(); }
            *(int*)(buf->data) = i;
            pool.PRelease();
        }
    });

    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; ++c) {
        consumers.emplace_back([&pool, c] {
            for (int i = 0; i < TOTAL_FRAMES; ++i) {
                ImageBuffer* buf = pool.CAcquire(c);
                while (!buf) { buf = pool.CAcquire(c); std::this_thread::yield(); }
                pool.CRelease(c);
            }
        });
    }

    producer.join();
    for (auto& th : consumers) th.join();

    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// ==================== 带锁对照组 ====================
double bench_locked() {
    LockedRingPool pool;
    pool.init(POOL_SIZE, 1920 * 1080 * 3, NUM_CONSUMERS);
    pool.set_consumer_count(NUM_CONSUMERS);

    auto start = std::chrono::steady_clock::now();

    std::thread producer([&] {
        for (int i = 0; i < TOTAL_FRAMES; ++i) {
            auto buf = pool.acquire_producer();
            *(int*)(buf->data) = i;
            pool.release_producer(buf, NUM_CONSUMERS);
        }
    });

    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; ++c) {
        consumers.emplace_back([&pool, c] {
            for (int i = 0; i < TOTAL_FRAMES; ++i) {
                auto buf = pool.acquire_consumer(c);
                pool.release_consumer(c);
            }
        });
    }

    producer.join();
    for (auto& th : consumers) th.join();

    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    double t1 = bench_syncflow();
    double t2 = bench_locked();
    std::cout << "SyncFlow (lock-free): " << t1 << " ms\n";
    std::cout << "LockedRingPool:       " << t2 << " ms\n";
    std::cout << "Speedup: " << t2 / t1 << "x\n";
    return 0;
}