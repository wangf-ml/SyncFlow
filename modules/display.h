#pragma once

#include <string>

#include "consumer.h"

namespace syncflow::modules {
class Display : public Consumer {
public:
    Display() = default;
    void set_name(const std::string& name) { name_ = name; }

protected:
    void consume(const ImageBuffer* buf);

private:
    std::string name_{"DisplayModule"};
};
}