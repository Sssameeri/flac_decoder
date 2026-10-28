#include <cstdint>
#include <iostream>
#include <stdexcept>
#include "samplerate.h"

namespace Frame {
    SampleRate decodeSampleRate(const std::uint8_t code) {
        SampleRate sampleRate{};

        switch (code) {
            case 0b0000: {
                sampleRate = {.kind = SampleRateKind::UnknownInfo, .sampleRate = 0};
                break;
            }
            case 0b0001: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 88200};
                break;
            }
            case 0b0010: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 176400};
                break;
            }
            case 0b0011: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 192000};
                break;
            }
            case 0b0100: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 8000};
                break;
            }
            case 0b0101: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 16000};
                break;
            }
            case 0b0110: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 22050};
                break;
            }
            case 0b0111: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 24000};
                break;
            }
            case 0b1000: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 32000};
                break;
            }
            case 0b1001: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 44100};
                break;
            }
            case 0b1010: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 48000};
                break;
            }
            case 0b1011: {
                sampleRate = {.kind = SampleRateKind::Known, .sampleRate = 96000};
                break;
            }
            case 0b1100: {
                sampleRate = {.kind = SampleRateKind::Unknown8, .sampleRate = 0};
                break;
            }
            case 0b1101: {
                sampleRate = {.kind = SampleRateKind::Unknown16, .sampleRate = 0};
                break;
            }
            case 0b1110: {
                sampleRate = {.kind = SampleRateKind::Unknown16_10, .sampleRate = 0};
                break;
            }
            default: {
                std::cerr << "Sample rate " << code << " is forbidden" << '\n';
                throw std::invalid_argument("Illegal sample rate");
                break;
            }
        }

        return sampleRate;
    }
}
