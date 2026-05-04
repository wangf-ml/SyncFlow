#include <iostream>

#include "display.h"

namespace syncflow::modules {
    void Display::consume(const PacketGuard& guard) {
        int frame = *(int*)(guard.image()->data);
        std::cout << "[" << name_ << "][consumer-" << consumer_id() << "] frame: " << frame << std::endl;
    }
}