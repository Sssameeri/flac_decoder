//
// Created by andrii on 28-Oct-26.
//
#include <stdexcept>
#include "channels.h"

Frame::Channels Frame::decodeChannels(const std::uint8_t code) {
    Channels channels{};

    switch (code) {
        case 0b0000:
        case 0b0001:
        case 0b0010:
        case 0b0011:
        case 0b0100:
        case 0b0101:
        case 0b0110:
        case 0b0111: {
            channels = {.assignment = Assignment::Independent, .count = static_cast<std::uint8_t>(code + 1)};
            break;
        }
        case 0b1000: {
            channels = {.assignment = Assignment::LeftSide, .count = 2};
            break;
        }
        case 0b1001: {
            channels = {.assignment = Assignment::SideRight, .count = 2};
            break;
        }
        case 0b1010: {
            channels = {.assignment = Assignment::MidSide, .count = 2};
            break;
        }
        default: {
            throw std::invalid_argument("reserved channel assignment code");
        }
    }

    return channels;
}
