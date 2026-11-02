#include "frame.h"

#include <bit>
#include <iostream>
#include <stdexcept>

#include "subframe.h"

bool Frame::readHeader(BitsReader &reader, const Stream::Info &info, Header &out) {
    if (reader.readBits(15) != 0x7FFC) {
        std::cerr << "Sync code is corrupted" << '\n';
        return false;
    }

    out.blockingStrategy = static_cast<std::uint8_t>(reader.readBits(1));
    out.blockSize = decodeBlockSize(static_cast<std::uint8_t>(reader.readBits(4)));
    out.sampleRate = decodeSampleRate(static_cast<std::uint8_t>(reader.readBits(4)));
    out.channels = decodeChannels(static_cast<std::uint8_t>(reader.readBits(4)));
    out.bitDepth = decodeBitDepth(static_cast<std::uint8_t>(reader.readBits(3)));

    if (out.bitDepth.pending()) {
        // code 0: bit depth comes from STREAMINFO, no extra bits to read
        out.bitDepth.bitDepth = info.bitsPerSample;
        out.bitDepth.state = BitDepthKind::Known;
    }

    if (const auto reservedBit = reader.readBits(1); reservedBit != 0) {
        std::cerr << "Frame header reserved bit equals to " << reservedBit << '\n';
        return false;
    }

    // first byte of the coded number: count of leading 1s = total byte count (0 means a single byte)
    const auto frameNumberFirstByte = static_cast<std::uint8_t>(reader.readBits(8));
    const std::int32_t codedNumbersSize{std::countl_one(frameNumberFirstByte)};

    if (codedNumbersSize == 1 || codedNumbersSize > 7) {
        std::cerr << "Frame number is corrupted" << '\n';
        return false;
    }

    // continuation bytes (10xxxxxx each), max 6 bytes = 48 bits
    if (codedNumbersSize > 1) {
        reader.readBits((codedNumbersSize - 1) * 8);
    }

    // uncommon block size comes after the coded number
    if (out.blockSize.pending()) {
        std::uint32_t uncommonBlockSize{0};

        switch (out.blockSize.kind) {
            case BlockSizeKind::Uncommon8: {
                uncommonBlockSize = static_cast<std::uint32_t>(reader.readBits(8));
                break;
            }
            case BlockSizeKind::Uncommon16: {
                uncommonBlockSize = static_cast<std::uint32_t>(reader.readBits(16));
                break;
            }
            default: {
                throw std::invalid_argument("Illegal block size kind");
            }
        }

        out.blockSize.samples = uncommonBlockSize + 1;
        out.blockSize.kind = BlockSizeKind::Known;
    }

    // then the uncommon sample rate
    if (out.sampleRate.pending()) {
        std::uint32_t uncommonSampleRate{0};

        switch (out.sampleRate.kind) {
            case SampleRateKind::UnknownInfo: {
                uncommonSampleRate = static_cast<std::uint32_t>(info.sampleRate);
                break;
            }
            case SampleRateKind::Unknown8: {
                uncommonSampleRate = static_cast<std::uint32_t>(reader.readBits(8)) * 1000;
                break;
            }
            case SampleRateKind::Unknown16: {
                uncommonSampleRate = static_cast<std::uint32_t>(reader.readBits(16));
                break;
            }
            case SampleRateKind::Unknown16_10: {
                uncommonSampleRate = static_cast<std::uint32_t>(reader.readBits(16)) * 10;
                break;
            }
            default: {
                throw std::invalid_argument("Illegal sample rate");
            }
        }

        out.sampleRate.sampleRate = uncommonSampleRate;
        out.sampleRate.kind = SampleRateKind::Known;
    }

    // read the computed CRC before reading the stored one: reading feeds bytes into the CRC
    const auto calculatedCrc8 = reader.getCrc8();
    if (const auto storedCrc8 = reader.readBits(8); storedCrc8 != calculatedCrc8) {
        std::cerr << "Frame header CRC-8 " << storedCrc8 << " does not match calculated "
                << static_cast<std::uint32_t>(calculatedCrc8) << '\n';
        return false;
    }

    return true;
}

bool Frame::read(BitsReader &reader, const Stream::Info &info, Decoded &out) {
    reader.resetCrc();

    if (!readHeader(reader, info, out.header)) return false;

    const Channels &channels = out.header.channels;
    const std::uint32_t blockSize{out.header.blockSize.samples};

    // vectors are reused between frames; Subframe::read clears each one
    out.samples.resize(channels.count);

    for (std::int32_t i = 0; i < channels.count; i++) {
        Subframe::Header subframeHeader{};
        if (!Subframe::readHeader(reader, subframeHeader)) return false;

        const std::int32_t sampleSize{
            out.header.bitDepth.bitDepth + subframeSideBit(channels, i) - subframeHeader.wastedBits
        };

        if (sampleSize < 1 || sampleSize > 33) {
            std::cerr << "Invalid subframe bit depth " << sampleSize << '\n';
            return false;
        }

        if (!Subframe::read(reader, subframeHeader, sampleSize, blockSize, out.samples[i])) return false;
    }

    decorrelateChannels(out.samples, channels);

    // frame footer: zero padding to the byte boundary, then CRC-16 over the whole frame
    reader.alignToByte();

    const auto calculatedCrc16 = reader.getCrc16();
    if (const auto storedCrc16 = reader.readBits(16); storedCrc16 != calculatedCrc16) {
        std::cerr << "Frame CRC-16 " << storedCrc16 << " does not match calculated " << calculatedCrc16 << '\n';
        return false;
    }

    return true;
}

std::int32_t Frame::subframeSideBit(const Channels &channels, const std::int32_t index) {
    std::int32_t sideBit{0};

    switch (channels.assignment) {
        case Assignment::LeftSide:
        case Assignment::MidSide: {
            constexpr std::int32_t secondSubframe{1};
            sideBit = index == secondSubframe ? 1 : 0;
            break;
        }
        case Assignment::SideRight: {
            constexpr std::int32_t firstSubframe{0};
            sideBit = index == firstSubframe ? 1 : 0;
            break;
        }
        case Assignment::Independent: {
            sideBit = 0;
            break;
        }
    }

    return sideBit;
}

void Frame::decorrelateChannels(std::vector<std::vector<std::int64_t> > &samples, const Channels &channels) {
    if (!channels.needDecorrelation()) return;

    for (std::size_t i = 0; i < samples[0].size(); i++) {
        switch (channels.assignment) {
            case Assignment::LeftSide: {
                const auto left = samples[0][i];
                samples[1][i] = left - samples[1][i];
                break;
            }
            case Assignment::SideRight: {
                const auto right = samples[1][i];
                samples[0][i] = right + samples[0][i];
                break;
            }
            case Assignment::MidSide: {
                auto mid = samples[0][i] << 1;
                const auto side = samples[1][i];

                // the bit lost when the encoder halved mid is the lowest bit of side
                if (side % 2 != 0) {
                    mid = mid + 1;
                }

                samples[0][i] = (mid + side) >> 1;
                samples[1][i] = (mid - side) >> 1;
                break;
            }
            case Assignment::Independent: {
                break;
            }
        }
    }
}
