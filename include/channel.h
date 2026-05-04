#pragma once

#include <memory>
#include <vector>

#include "ring_packet_pool.h"
#include "image_info.h"
#include "status.h"


namespace syncflow {

class Producer;
class Consumer;

class Channel {
public:
    Channel() = default;

    StatusCode init(size_t pool_size, const ImageInfo& info, size_t num_consumers);
    
    void register_producer(std::unique_ptr<Producer> producer);
    void register_consumer(std::unique_ptr<Consumer> consumer, uint32_t consumer_id);
    void start_all();
    void stop_all();

    RingPacketPool* internal_pool();
    

    template<typename T, typename... Args>
    std::unique_ptr<T> create_producer(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    std::unique_ptr<T> create_consumer(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

private:
    RingPacketPool ring_pool_;
    std::unique_ptr<Producer> producer_;
    std::vector<std::unique_ptr<Consumer>> consumers_;
};

}