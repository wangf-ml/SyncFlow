#include <iostream>

#include "virtual_camera.h"
#include "logger.h"

namespace syncflow::modules {
    bool VirtualCamera::produce(Packet* pkt) {
        if (current_frame_ >= total_frames_) {
            return false;  // 已经生产完所有帧
        }
        *(int*)(pkt->image->data) = current_frame_;
        SYNC_LOG("produce frame " << current_frame_);
        ++current_frame_;
        return true;
    }
}

