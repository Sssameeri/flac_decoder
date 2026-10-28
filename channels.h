//
// Created by andrii on 28-Oct-26.
//

#ifndef FLAC_CHANNELS_H
#define FLAC_CHANNELS_H
#include <cstdint>

namespace Frame {
    enum class Assignment : std::uint8_t {
        Independent,
        LeftSide,
        SideRight,
        MidSide
    };

    struct Channels {
        Assignment assignment{Assignment::Independent};
        std::uint8_t count{0};
    };

    Channels decodeChannels(std::uint8_t code);
}

#endif //FLAC_CHANNELS_H
