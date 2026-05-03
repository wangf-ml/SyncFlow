#pragma once

#include "producer.h"

using namespace syncflow::modules {
class VirtualCamera : public Producer {
public:
    VirtualCamera() = default;
    void set_total_frames(int total_frames) { total_frames_ = total_frames; }

protected:
    bool produce(Packet* pkt) override;

private:
    int total_frames_{0};
    int current_frame_{0};
};
}