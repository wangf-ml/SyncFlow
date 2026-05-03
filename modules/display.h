#pragma once

#include <string>

#include "consumer.h"

using namespace syncflow::modules {
class DisplayModule : public Consumer {
public:
    DisplayModule() = default;
    void set_name(const std::string& name) { name_ = name; }

protected:
    void consume(PacketGuard guard) override;

private:
    std::string name_{"DisplayModule"};
};
}