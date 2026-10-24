//
// Created by andrii on 23-Oct-26.
//

#ifndef FLAC_STREAMINFO_H
#define FLAC_STREAMINFO_H
#include <cstdint>
#include <fstream>
#include <array>

#endif //FLAC_STREAMINFO_H

namespace Stream {
    struct Info {
        std::uint16_t minBlockSize{};
        std::uint16_t maxBlockSize{};
        std::uint32_t minFrameSize{};
        std::uint32_t maxFrameSize{};
        std::uint64_t sampleRate{};
        std::uint16_t channels{};
        std::uint16_t bitsPerSample{};
        std::uint64_t numberOfSamples{};
        std::array<unsigned char, 16> md5{};
    };

    Info handleStreamInfoMetadata(std::ifstream &file);
}