#include <iostream>

#include "display.h"
#include "logger.h"

namespace syncflow::modules {
    void Display::consume(const PacketGuard& guard) {
        int frame = *(int*)(guard.image()->data);
        auto start_time = std::chrono::steady_clock::now();
        SYNC_LOG("[" << name_ << "][consume" << consumer_id() << "] frame: " << frame);
    }
}