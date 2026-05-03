#pragma once

#include <iostream>

#include "display.h"

using namespace syncflow::modules {
    void DisplayModule::consume(PacketGuard guard) {
        int frame = *(int*)(guard.image()->data);
        std::cout << "[" << name_ << "][consumer-" << consumer_id() << "] frame: " << frame << std::endl;
    }
}