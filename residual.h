#ifndef FLAC_RESIDUAL_H
#define FLAC_RESIDUAL_H
#include <cstdint>
#include <vector>

#include "bitsreader.h"

namespace Residual {
    enum class CodingMethod : std::uint8_t {
        RiceCode4,
        RiceCode5
    };

    struct CodedResidual {
        CodingMethod method{CodingMethod::RiceCode4};
        // width of each partition's Rice parameter: 4 or 5 bits
        std::int32_t numberOfBits{4};
    };

    CodedResidual decodeCodedResidual(std::uint8_t code);

    // reads the coded residual of a FIXED or LPC subframe and appends
    // (blockSize - order) residual samples to `out`
    bool read(BitsReader &reader, std::uint32_t blockSize, std::int32_t order, std::vector<std::int64_t> &out);
}

#endif //FLAC_RESIDUAL_H
