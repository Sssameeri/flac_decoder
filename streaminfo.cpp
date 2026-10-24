//
// Created by andrii on 23-Oct-26.
//

#include <array>
#include <cstdint>
#include <fstream>
#include "streaminfo.h"
#include "endianness.h"

namespace Stream {
    Info handleStreamInfoMetadata(std::ifstream &file) {
        std::array<unsigned char, 2> minBlockSizeBytes{};
        std::array<unsigned char, 2> maxBlockSizeBytes{};
        std::array<unsigned char, 3> minFrameSizeBytes{};
        std::array<unsigned char, 3> maxFrameSizeBytes{};
        std::array<unsigned char, 8> valueBytes{};
        std::array<unsigned char, 16> md5Bytes{};

        file.read(reinterpret_cast<char *>(&minBlockSizeBytes), sizeof(minBlockSizeBytes));
        file.read(reinterpret_cast<char *>(&maxBlockSizeBytes), sizeof(maxBlockSizeBytes));
        file.read(reinterpret_cast<char *>(&minFrameSizeBytes), sizeof(minFrameSizeBytes));
        file.read(reinterpret_cast<char *>(&maxFrameSizeBytes), sizeof(maxFrameSizeBytes));
        file.read(reinterpret_cast<char *>(&valueBytes), sizeof(valueBytes));
        file.read(reinterpret_cast<char *>(&md5Bytes), sizeof(md5Bytes));

        const auto minBlockSize{fromBigEndianness<std::uint16_t>(minBlockSizeBytes)};
        const auto maxBlockSize{fromBigEndianness<std::uint16_t>(maxBlockSizeBytes)};
        const auto minFrameSize{fromBigEndianness<std::uint32_t>(minFrameSizeBytes)};
        const auto maxFrameSize{fromBigEndianness<std::uint32_t>(maxFrameSizeBytes)};
        const auto packedValue{fromBigEndianness<std::uint64_t>(valueBytes)};

        const auto sampleRate = (packedValue >> 44) & 0xFFFFF;
        const auto channels = static_cast<std::uint16_t>(((packedValue >> 41) & 0x7) + 1);
        const auto bitsPerSample = static_cast<std::uint16_t>(((packedValue >> 36) & 0x1F) + 1);
        const auto numberOfSamples = (packedValue & 0xFFFFFFF); // 74 684 643

        return {
            .minBlockSize = minBlockSize,
            .maxBlockSize = maxBlockSize,
            .minFrameSize = minFrameSize,
            .maxFrameSize = maxFrameSize,
            .sampleRate = sampleRate,
            .channels = channels,
            .bitsPerSample = bitsPerSample,
            .numberOfSamples = numberOfSamples,
            .md5 = md5Bytes
        };
    }
}
