//
// Created by andrii on 28-Oct-26.
//

#ifndef FLAC_SUBFRAMETYPE_H
#define FLAC_SUBFRAMETYPE_H
#include <cstdint>

namespace Subframe {
    constexpr std::int16_t UNNECESSARY_ORDER = -1;

    enum class Kind : std::uint8_t {
        Constant,
        Verbatim,
        FixedPredictor,
        LinearPredictor
    };

    struct Type {
        Kind kind{Kind::Constant};
        std::int16_t predictorOrder{UNNECESSARY_ORDER};

        [[nodiscard]] bool hasPredictorOrder() const {
            return kind == Kind::FixedPredictor || kind == Kind::LinearPredictor;
        }
    };

    Type decodeSubframeType(std::uint8_t code);
}

#endif //FLAC_SUBFRAMETYPE_H
