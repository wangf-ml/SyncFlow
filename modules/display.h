#pragma once

#include <string>

#include "consumer.h"

namespace syncflow::modules {
class Display : public Consumer {
public:
    Display() = default;
    void set_name(const std::string& name) { name_ = name; }

protected:
    void consume(const PacketGuard& guard);

private:
    std::string name_{"DisplayModule"};
};
}