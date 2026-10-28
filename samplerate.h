#ifndef FLAC_SAMPLERATE_H
#define FLAC_SAMPLERATE_H
#include <cstdint>

namespace Frame {
    enum class SampleRateKind : std::uint8_t {
        Known,
        UnknownInfo,
        Unknown8,
        Unknown16,
        Unknown16_10
    };

    struct SampleRate {
        SampleRateKind kind{SampleRateKind::Known};
        std::uint32_t sampleRate{0};

        [[nodiscard]] bool pending() const {
            return kind != SampleRateKind::Known;
        }
    };

    SampleRate decodeSampleRate(std::uint8_t code);
}

#endif //FLAC_SAMPLERATE_H
