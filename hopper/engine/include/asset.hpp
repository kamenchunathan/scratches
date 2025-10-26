#pragma once

#ifndef __EMSCRIPTEN__

    #include "png.h"

namespace asset {

class PngReader {
public:
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

    PngReader() {
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (png_ptr) {
            info_ptr = png_create_info_struct(png_ptr);
        }
    }

    ~PngReader() {
        if (png_ptr) {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        }
    }

    PngReader(const PngReader&) = delete;
    PngReader& operator=(const PngReader&) = delete;
    PngReader(PngReader&&) = delete;
    PngReader& operator=(PngReader&&) = delete;

    operator bool() const {
        return png_ptr && info_ptr;
    }
};

#endif // !__EMSCRIPTEN__

} // namespace asset
