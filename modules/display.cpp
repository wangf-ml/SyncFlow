#include <iostream>

#include "display.h"
#include "logger.h"

namespace syncflow::modules {
    void Display::consume(const PacketGuard& guard) {
        int frame = *(int*)(guard.image()->data);
        SYNC_LOG("[" << name_ << "][consume" << consumer_id() << "] frame: " << frame);
        
    }
}