#pragma once

#include "producer.h"

namespace syncflow::modules {
class VirtualCamera : public Producer {
public:
    VirtualCamera() = default;
    void set_total_frames(int total_frames) { total_frames_ = total_frames; }

    bool is_done() const {
        return current_frame_.load(std::memory_order_relaxed) >= total_frames_;
    }

protected:
    bool produce(Packet* pkt);

private:
    int total_frames_{0};
    std::atomic<int> current_frame_{0};
};
}