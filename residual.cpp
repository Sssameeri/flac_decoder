#include "residual.h"

#include <iostream>
#include <stdexcept>

Residual::CodedResidual Residual::decodeCodedResidual(const std::uint8_t code) {
    CodedResidual codedResidual{};

    switch (code) {
        case 0b00: {
            codedResidual = {.method = CodingMethod::RiceCode4, .numberOfBits = 4};
            break;
        }
        case 0b01: {
            codedResidual = {.method = CodingMethod::RiceCode5, .numberOfBits = 5};
            break;
        }
        default: {
            throw std::invalid_argument("Reserved coded residual");
        }
    }

    return codedResidual;
}

bool Residual::read(
    BitsReader &reader,
    const std::uint32_t blockSize,
    const std::int32_t order,
    std::vector<std::int64_t> &out
) {
    const auto codedResidualCode = static_cast<std::uint8_t>(reader.readBits(2));
    const CodedResidual codedResidual{decodeCodedResidual(codedResidualCode)};

    const auto partitionOrder = static_cast<std::int32_t>(reader.readBits(4));
    const std::uint32_t numberOfPartitions{1u << partitionOrder};
    const std::uint32_t partitionSize{blockSize >> partitionOrder};
    const auto unsignedOrder = static_cast<std::uint32_t>(order);

    if (blockSize % numberOfPartitions != 0) {
        std::cerr << "Block size is not evenly divisible by the number of partitions" << '\n';
        return false;
    }

    const bool tooSmall = partitionOrder > 0 ? partitionSize <= unsignedOrder : blockSize < unsignedOrder;
    if (tooSmall) {
        std::cerr << "Partition is too small for the predictor order" << '\n';
        return false;
    }

    // all ones in the parameter width means "escaped partition"
    const std::int32_t escapeCode{(1 << codedResidual.numberOfBits) - 1};

    for (std::uint32_t p = 0; p < numberOfPartitions; p++) {
        const std::uint32_t numberOfResidualSamples{p == 0 ? partitionSize - unsignedOrder : partitionSize};
        const auto riceParameter = static_cast<std::int32_t>(reader.readBits(codedResidual.numberOfBits));

        if (riceParameter == escapeCode) {
            // escaped partition: plain signed values, all of the same width
            const auto residualSampleWidth = static_cast<std::int32_t>(reader.readBits(5));
            for (std::uint32_t s = 0; s < numberOfResidualSamples; s++) {
                out.push_back(reader.readBitsSigned(residualSampleWidth));
            }
        } else {
            // Rice code: unary quotient, then riceParameter bits of remainder, then zig-zag
            for (std::uint32_t s = 0; s < numberOfResidualSamples; s++) {
                const auto quotient = static_cast<std::uint64_t>(reader.readUnary()) << riceParameter;
                const auto remainder = reader.readBits(riceParameter);
                out.push_back(zigZagDecode(quotient | remainder));
            }
        }
    }

    return true;
}
