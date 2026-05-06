#include <iostream>

#include "virtual_camera.h"
#include "logger.h"

namespace syncflow::modules {
    bool VirtualCamera::produce(Packet* pkt) {
        int cur = current_frame_.load(std::memory_order_relaxed);

        if (current_frame_ >= total_frames_) {
            return false;  // 已经生产完所有帧
        }

        *(int*)(pkt->image->data) = cur;
        SYNC_LOG("produce frame " << cur);
        current_frame_.store(cur + 1, std::memory_order_relaxed);
        return true;
    }

    
}

