#include <cstdint>
#include <stdexcept>
#include "blocksize.h"

namespace Frame {
    BlockSize decodeBlockSize(const std::uint8_t code) {
        BlockSize size{};

        switch (code) {
            case 0b0001: {
                size = {.kind = BlockSizeKind::Known, .samples = 192};
                break;
            }
            case 0b0010:
            case 0b0011:
            case 0b0100:
            case 0b0101: {
                size = {.kind = BlockSizeKind::Known, .samples = 144u << code};
                break;
            }
            case 0b0110: {
                size = {.kind = BlockSizeKind::Uncommon8, .samples = 0};
                break;
            }
            case 0b0111: {
                size = {.kind = BlockSizeKind::Uncommon16, .samples = 0};
                break;
            }
            case 0b1000:
            case 0b1001:
            case 0b1010:
            case 0b1011:
            case 0b1100:
            case 0b1101:
            case 0b1110:
            case 0b1111: {
                size = {.kind = BlockSizeKind::Known, .samples = 1u << code};
                break;
            }
            default: {
                throw std::invalid_argument("block size is 0");
            }
        }

        return size;
    }
}
