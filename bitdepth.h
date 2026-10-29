//
// Created by andrii on 28-Oct-26.
//

#ifndef FLAC_BITDEPTH_H
#define FLAC_BITDEPTH_H
#include <cstdint>

namespace Frame {
    enum class BitDepthKind : std::uint8_t {
        Known,
        FromInfo
    };

    struct BitDepth {
        BitDepthKind state{BitDepthKind::Known};
        std::int32_t bitDepth{0};

        [[nodiscard]] bool pending() const {
            return state != BitDepthKind::Known;
        }
    };

    BitDepth decodeBitDepth(std::uint8_t code);
}


#endif //FLAC_BITDEPTH_H
