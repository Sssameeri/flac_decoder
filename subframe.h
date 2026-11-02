#ifndef FLAC_SUBFRAME_H
#define FLAC_SUBFRAME_H
#include <cstdint>
#include <vector>

#include "bitsreader.h"
#include "subframetype.h"

namespace Subframe {
    struct Header {
        Type type{};
        std::int32_t wastedBits{0};
    };

    // zero bit, 6-bit type, wasted bits flag (+ unary count)
    bool readHeader(BitsReader &reader, Header &out);

    // decodes one subframe into `out` (blockSize samples, wasted bits already restored)
    bool read(
        BitsReader &reader,
        const Header &header,
        std::int32_t sampleSize,
        std::uint32_t blockSize,
        std::vector<std::int64_t> &out
    );

    void readConstant(BitsReader &reader, std::int32_t sampleSize, std::uint32_t blockSize,
                      std::vector<std::int64_t> &out);

    void readVerbatim(BitsReader &reader, std::int32_t sampleSize, std::uint32_t blockSize,
                      std::vector<std::int64_t> &out);

    bool readFixed(BitsReader &reader, std::int32_t order, std::int32_t sampleSize, std::uint32_t blockSize,
                   std::vector<std::int64_t> &out);

    bool readLpc(BitsReader &reader, std::int32_t order, std::int32_t sampleSize, std::uint32_t blockSize,
                 std::vector<std::int64_t> &out);

    // turns residuals (after the warm-up samples) into samples, in place
    void predictFixed(std::vector<std::int64_t> &samples, std::int32_t order);

    void predictLpc(std::vector<std::int64_t> &samples, const std::vector<std::int64_t> &coefficients,
                    std::int32_t shift);
}

#endif //FLAC_SUBFRAME_H
