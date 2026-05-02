#pragma once

#include <atomic>
#include <memory>
#include <functional>

namespace syncflow {

enum class PixelFormat {
        UNKNOWN,
        RGB,
        RGBA,
        BGR,
        BGRA,
        YUV420,
        YUV422,
        YUV444,
};

}
