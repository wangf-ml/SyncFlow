#pragma once

#include <string>

#include "pixel_format.h"

namespace syncflow {

struct ImageInfo {    
    ImageInfo(const ImageInfo&) = default;
    ImageInfo& operator=(const ImageInfo&) = default;
    ImageInfo(ImageInfo&&) = default;
    ImageInfo& operator=(ImageInfo&&) = default;
    
    ImageInfo() = default;
    ImageInfo(PixelFormat pixel_format, uint32_t width, uint32_t height);

    uint32_t size = 0;
    PixelFormat pixel_format{PixelFormat::UNKNOWN};
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0; 
};
}
