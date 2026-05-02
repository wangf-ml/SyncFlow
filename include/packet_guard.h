#pragma once

#include <memory>

#include "packet.h"

namespace syncflow { 

class PacketGuard {
public:
    static std::unique_ptr<PacketGuard> acquire(Packet* pkt, uint32_t consumer_id) {
        if (!pkt) {
            return nullptr;
        }
        auto img = pkt->image;

        if (!pkt->try_claim(consumer_id)) {
            return nullptr;
        }
        return std::unique_ptr<PacketGuard>(new PacketGuard(pkt, std::move(img)));
    }

    const ImageBuffer* image() const {
        return img_.get();
    }

    const std::shared_ptr<ImageBuffer>& image_ptr() const { 
        return img_;
    }
    
    PacketGuard(const PacketGuard&) = delete;
    PacketGuard& operator=(const PacketGuard&) = delete;
    
private:
    PacketGuard(Packet* packet, std::shared_ptr<ImageBuffer> img)
        : pkt_(packet), img_(std::move(img)) {}
    Packet* pkt_;
    std::shared_ptr<ImageBuffer> img_;
};
}