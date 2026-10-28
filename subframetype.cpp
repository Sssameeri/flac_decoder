//
// Created by andrii on 28-Oct-26.
//

#include "subframetype.h"

#include <stdexcept>

Subframe::Type Subframe::decodeSubframeType(const std::uint8_t code) {
    Type type{};

    switch (code) {
        case 0b000000: {
            type = {.kind = Kind::Constant};
            break;
        }
        case 0b000001: {
            type = {.kind = Kind::Verbatim};
            break;
        }
        case 0b001000:
        case 0b001001:
        case 0b001010:
        case 0b001011:
        case 0b001100: {
            type = {.kind = Kind::FixedPredictor, .predictorOrder = static_cast<std::int16_t>(code - 8)};
            break;
        }
        case 0b100000:
        case 0b100001:
        case 0b100010:
        case 0b100011:
        case 0b100100:
        case 0b100101:
        case 0b100110:
        case 0b100111:
        case 0b101000:
        case 0b101001:
        case 0b101010:
        case 0b101011:
        case 0b101100:
        case 0b101101:
        case 0b101110:
        case 0b101111:
        case 0b110000:
        case 0b110001:
        case 0b110010:
        case 0b110011:
        case 0b110100:
        case 0b110101:
        case 0b110110:
        case 0b110111:
        case 0b111000:
        case 0b111001:
        case 0b111010:
        case 0b111011:
        case 0b111100:
        case 0b111101:
        case 0b111110:
        case 0b111111: {
            type = {.kind = Kind::LinearPredictor, .predictorOrder = static_cast<std::int16_t>(code - 31)};
            break;
        }
        default: {
            throw std::invalid_argument("Illegal subframe type");
        }
    }

    return type;
}
