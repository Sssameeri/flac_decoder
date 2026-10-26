//
// Created by andrii on 23-Oct-26.
//

#ifndef FLAC_ENDIANNESS_H
#define FLAC_ENDIANNESS_H
#include <array>
#include <cstddef>

template<typename R, typename T, std::size_t S>
R fromBigEndianness(std::array<T, S> &array) {
    std::uint64_t value{};
    for (const auto &i: array) {
        value = (value << 8) | i;
    }
    return value;
}

#endif //FLAC_ENDIANNESS_H
