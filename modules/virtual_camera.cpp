#include <iostream>

#include "virtual_camera.h"

namespace syncflow::modules {
    bool VirtualCamera::produce(Packet* pkt) {
        if (current_frame_ >= total_frames_) {
            return false;  // 已经生产完所有帧
        }
        *(int*)(pkt->image->data) = current_frame_;
        //std::cout << "VirtualCamera produced frame: " << current_frame_ << std::endl;
        ++current_frame_;
        return true;
    }
}

