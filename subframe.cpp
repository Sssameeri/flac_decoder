#include "subframe.h"

#include <iostream>
#include <stdexcept>

#include "residual.h"

bool Subframe::readHeader(BitsReader &reader, Header &out) {
    if (reader.readBits(1) != 0) {
        std::cerr << "Subframe is corrupted" << '\n';
        return false;
    }

    out.type = decodeSubframeType(static_cast<std::uint8_t>(reader.readBits(6)));
    out.wastedBits = 0;

    if (reader.readBits(1) == 1) {
        out.wastedBits = reader.readUnary() + 1;
    }

    return true;
}

bool Subframe::read(
    BitsReader &reader,
    const Header &header,
    const std::int32_t sampleSize,
    const std::uint32_t blockSize,
    std::vector<std::int64_t> &out
) {
    out.clear();
    out.reserve(blockSize);

    const std::int32_t order{header.type.predictorOrder};
    bool ok{true};

    switch (header.type.kind) {
        case Kind::Constant: {
            readConstant(reader, sampleSize, blockSize, out);
            break;
        }
        case Kind::Verbatim: {
            readVerbatim(reader, sampleSize, blockSize, out);
            break;
        }
        case Kind::FixedPredictor: {
            ok = readFixed(reader, order, sampleSize, blockSize, out);
            break;
        }
        case Kind::LinearPredictor: {
            ok = readLpc(reader, order, sampleSize, blockSize, out);
            break;
        }
    }

    if (!ok) return false;

    if (out.size() != blockSize) {
        std::cerr << "Subframe has " << out.size() << " samples instead of " << blockSize << '\n';
        return false;
    }

    // wasted bits: restore the low zero bits the encoder removed
    if (header.wastedBits > 0) {
        for (auto &sample: out) {
            sample <<= header.wastedBits;
        }
    }

    return true;
}

void Subframe::readConstant(
    BitsReader &reader,
    const std::int32_t sampleSize,
    const std::uint32_t blockSize,
    std::vector<std::int64_t> &out
) {
    // one value stored, repeated for the whole block
    const std::int64_t sample = reader.readBitsSigned(sampleSize);
    out.assign(blockSize, sample);
}

void Subframe::readVerbatim(
    BitsReader &reader,
    const std::int32_t sampleSize,
    const std::uint32_t blockSize,
    std::vector<std::int64_t> &out
) {
    for (std::uint32_t j = 0; j < blockSize; j++) {
        out.push_back(reader.readBitsSigned(sampleSize));
    }
}

bool Subframe::readFixed(
    BitsReader &reader,
    const std::int32_t order,
    const std::int32_t sampleSize,
    const std::uint32_t blockSize,
    std::vector<std::int64_t> &out
) {
    // warm-up samples are the first `order` samples, stored as they are
    for (std::int32_t j = 0; j < order; j++) {
        out.push_back(reader.readBitsSigned(sampleSize));
    }

    if (!Residual::read(reader, blockSize, order, out)) return false;

    predictFixed(out, order);
    return true;
}

bool Subframe::readLpc(
    BitsReader &reader,
    const std::int32_t order,
    const std::int32_t sampleSize,
    const std::uint32_t blockSize,
    std::vector<std::int64_t> &out
) {
    for (std::int32_t j = 0; j < order; j++) {
        out.push_back(reader.readBitsSigned(sampleSize));
    }

    // coefficient precision: 4 bits, value + 1; 0b1111 is invalid
    const auto precisionCode = reader.readBits(4);
    if (precisionCode == 0b1111) {
        std::cerr << "Invalid LPC coefficient precision" << '\n';
        return false;
    }
    const auto precision = static_cast<std::int32_t>(precisionCode) + 1;

    // prediction right shift: 5-bit signed, negative values are invalid
    const auto shift = reader.readBitsSigned(5);
    if (shift < 0) {
        std::cerr << "Negative LPC shift " << shift << '\n';
        return false;
    }

    std::vector<std::int64_t> coefficients{};
    coefficients.reserve(static_cast<std::size_t>(order));
    for (std::int32_t k = 0; k < order; k++) {
        coefficients.push_back(reader.readBitsSigned(precision));
    }

    if (!Residual::read(reader, blockSize, order, out)) return false;

    predictLpc(out, coefficients, static_cast<std::int32_t>(shift));
    return true;
}

void Subframe::predictFixed(std::vector<std::int64_t> &samples, const std::int32_t order) {
    for (auto j = static_cast<std::size_t>(order); j < samples.size(); j++) {
        switch (order) {
            case 0: {
                // prediction is 0: the sample is the residual
                break;
            }
            case 1: {
                samples[j] += samples[j - 1];
                break;
            }
            case 2: {
                samples[j] += 2 * samples[j - 1] - samples[j - 2];
                break;
            }
            case 3: {
                samples[j] += 3 * samples[j - 1] - 3 * samples[j - 2] + samples[j - 3];
                break;
            }
            case 4: {
                samples[j] += 4 * samples[j - 1] - 6 * samples[j - 2] + 4 * samples[j - 3] - samples[j - 4];
                break;
            }
            default: {
                throw std::invalid_argument("Illegal fixed predictor order");
            }
        }
    }
}

void Subframe::predictLpc(
    std::vector<std::int64_t> &samples,
    const std::vector<std::int64_t> &coefficients,
    const std::int32_t shift
) {
    const std::size_t order{coefficients.size()};

    for (std::size_t j = order; j < samples.size(); j++) {
        std::int64_t sum{0};
        for (std::size_t k = 0; k < order; k++) {
            sum += coefficients[k] * samples[j - 1 - k];
        }
        samples[j] += sum >> shift;
    }
}
