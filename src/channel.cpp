#include "channel.h"
#include "consumer.h"
#include "producer.h"

namespace syncflow {
    StatusCode Channel::init(size_t pool_size, const ImageInfo& info, size_t num_consumers) {
        return ring_pool_.init(pool_size, info, num_consumers);
    }

    void Channel::register_producer(std::unique_ptr<Producer> producer) {
        producer->set_pool(&ring_pool_);
        producer_ = std::move(producer);
    }

    void Channel::register_consumer(std::unique_ptr<Consumer> consumer, uint32_t consumer_id) {
        consumer->set_pool(&ring_pool_);
        consumer->set_consumer_id(consumer_id);
        consumers_.push_back(std::move(consumer));
    }

    void Channel::start_all() {
        for (auto& consumer : consumers_) {
            consumer->start();
        }
        if (producer_) {
            producer_->start();
        }
    }

    void Channel::stop_all() {
        if (producer_) {
            producer_->stop();
        }
        for (auto& consumer : consumers_) {
            consumer->stop();
        }
    }

    RingPacketPool* Channel::internal_pool() {
        return &ring_pool_;
    }
}