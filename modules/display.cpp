#include <iostream>

#include "display.h"
#include "logger.h"

namespace syncflow::modules {
    void Display::consume(const ImageBuffer* buf) {
        int frame = *(int*)(buf->data);
        SYNC_LOG("[" << name_ << "][consume" << consumer_id() << "] frame: " << frame);
        
    }
}