#include <cstdint>
#include <cstddef>
#include <iostream>

struct Layout
{
    size_t size;
    size_t align;

    Layout(size_t size, size_t align)
        : size(size)
        , align(align)
    {
    }
};

struct FlatBuf
{
    uint8_t *data;

    FlatBuf();

    /**
     * `realloc()` the contents of this buffer, without intializing the extra
     * space. throw OOM on realloc fail. 
     */
    void grow(Layout, Layout);

    void dealloc();
};