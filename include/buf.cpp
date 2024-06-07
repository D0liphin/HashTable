#include "buf.hpp"
#include <cstring>
#include <cstdlib>

FlatBuf::FlatBuf()
    : data(nullptr)
{
}

void FlatBuf::grow(Layout old_layout, Layout new_layout)
{
    if (new_layout.align > alignof(max_align_t)) {
        uint8_t *resized_buf;
        if (!posix_memalign((void **)&resized_buf, new_layout.align, new_layout.size)) {
            throw std::runtime_error("OOM");
        }
        memcpy(resized_buf, data, old_layout.size);
        data = resized_buf;
    } else {
        data = (uint8_t *)realloc(data, new_layout.size);
    }
}

void FlatBuf::dealloc()
{
    if (data) free(data);
}
