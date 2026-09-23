#pragma once
#include <cstdint>
#include <span>

namespace scanme {

void rs_generate_ec(
    std::span<const uint8_t> data,
    std::span<uint8_t> ecc
);

} // namespace scanme
