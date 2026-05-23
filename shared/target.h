#pragma once

#include <cstdint>

namespace termino_platform
{
    constexpr uint32_t MEMORY_SIZE   = 8 * 1024 * 1024;
    constexpr size_t   ADDRESS_BYTES = 4;
    
    using Address = uint32_t;
}
