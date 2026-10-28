//
// Created by andrii on 28-Oct-26.
//

#include "bitdepth.h"

#include <stdexcept>

Frame::BitDepth Frame::decodeBitDepth(const std::uint8_t code) {
    BitDepth bitDepth{};

    switch (code) {
        case 0b000: {
            bitDepth = {.state = BitDepthKind::FromInfo, .bitDepth = 0};
            break;
        }
        case 0b001: {
            bitDepth = {.state = BitDepthKind::Known, .bitDepth = 8};
            break;
        }
        case 0b010: {
            bitDepth = {.state = BitDepthKind::Known, .bitDepth = 12};
            break;
        }
        case 0b100: {
            bitDepth = {.state = BitDepthKind::Known, .bitDepth = 16};
            break;
        }
        case 0b101: {
            bitDepth = {.state = BitDepthKind::Known, .bitDepth = 20};
            break;
        }
        case 0b110: {
            bitDepth = {.state = BitDepthKind::Known, .bitDepth = 24};
            break;
        }
        case 0b111: {
            bitDepth = {.state = BitDepthKind::Known, .bitDepth = 32};
            break;
        }
        default: {
            throw std::invalid_argument("reserved bit depth code");
        }
    }

    return bitDepth;
}
