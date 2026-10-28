#ifndef FLAC_BLOCKSIZE_H
#define FLAC_BLOCKSIZE_H
#include <cstdint>

namespace Frame {
    enum class BlockSizeKind : std::uint8_t {
        Known,
        Uncommon8,
        Uncommon16
    };

    struct BlockSize {
        BlockSizeKind kind{BlockSizeKind::Known};
        std::uint32_t samples{0};

        [[nodiscard]] bool pending() const {
            return kind != BlockSizeKind::Known;
        }
    };

    BlockSize decodeBlockSize(std::uint8_t code);
}

#endif //FLAC_BLOCKSIZE_H
