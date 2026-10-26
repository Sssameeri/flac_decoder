#include <iostream>
#include <fstream>
#include <array>
#include <cstdint>
#include <bitset>
#include <vector>
#include <bit>
#include "streaminfo.h"
#include "endianness.h"
#include "vorbis.h"
#include "bitsreader.h"

constexpr std::array<unsigned char, 4> flac{'f', 'L', 'a', 'C'};

enum MetadataType {
    streamInfo,
    padding,
    application,
    seekTable,
    vorbisComment,
    cuesheet,
    picture
};

struct FlacSignature {
    std::array<unsigned char, 4> id{};
};

struct Metadata {
    bool isLastMetadataBlock{};
    std::uint32_t nextMetadataBlock{};
    std::uint32_t size{};
};

Metadata readNextMetadataBlock(std::ifstream &file) {
    std::array<unsigned char, 4> metadataBytes{};

    file.read(reinterpret_cast<char *>(&metadataBytes), sizeof(metadataBytes));
    const auto headerPackedValue{fromBigEndianness<std::uint32_t>(metadataBytes)};

    const bool isLastMetadataBlock = (headerPackedValue >> 31) & 0x1;
    const std::uint32_t nextMetadataBlock = (headerPackedValue >> 24) & 0x7F;

    [[maybe_unused]] const auto bytes = headerPackedValue & 0xFFFFFF;

    return {
        .isLastMetadataBlock = isLastMetadataBlock,
        .nextMetadataBlock = nextMetadataBlock,
        .size = bytes
    };
}

int main(const int argc, char *argv[]) {
    if (argc == 1) {
        std::cout << "There is no audio path in args" << '\n';
        return -1;
    }

    const std::string path{argv[1]};
    std::cout << path << '\n';

    std::ifstream file(path, std::ios::binary);

    FlacSignature signature{};

    file.read(reinterpret_cast<char *>(&signature), sizeof(FlacSignature));

    if (signature.id != flac) {
        std::cerr << "Non valid flac file" << std::endl;
        return -1;
    }

    [[maybe_unused]] Stream::Info info{};
    [[maybe_unused]] Vorbis::Data vorbisData{};

    Metadata currentMetadata{readNextMetadataBlock(file)};
    Metadata prevMetadata{currentMetadata};

    while (!prevMetadata.isLastMetadataBlock) {
        switch (currentMetadata.nextMetadataBlock) {
            case streamInfo: {
                info = Stream::handleStreamInfoMetadata(file);
                break;
            }
            case padding: {
                file.seekg(currentMetadata.size, std::ios::cur);
                break;
            }
            case application: {
                file.seekg(currentMetadata.size, std::ios::cur);
                break;
            }
            case seekTable: {
                file.seekg(currentMetadata.size, std::ios::cur);
                break;
            }
            case vorbisComment: {
                vorbisData = Vorbis::handleVorbisComment(file);
                break;
            }
            case cuesheet: {
                file.seekg(currentMetadata.size, std::ios::cur);
                break;
            }
            case picture: {
                file.seekg(currentMetadata.size, std::ios::cur);
                break;
            }
            default: {
                std::cerr << "unknown metadata block: " << currentMetadata.nextMetadataBlock << std::endl;
                return -1;
            }
        }
        if (currentMetadata.isLastMetadataBlock) break;
        prevMetadata = currentMetadata;
        currentMetadata = readNextMetadataBlock(file);
    }

    std::cout << vorbisData.vendor << "\n";

    for (const auto &s: vorbisData.fields) {
        std::cout << s << '\n';
    }

    std::cout << "Sample rate: " << info.sampleRate << '\n';
    std::cout << "Num of channels: " << info.channels << '\n';
    std::cout << "Num of samples: " << info.numberOfSamples << '\n';
    std::cout << "Bits per samples: " << info.bitsPerSample << '\n';
    std::cout << "Min/max bloc sizes: " << info.minBlockSize << "/" << info.maxBlockSize << '\n';
    std::cout << "Min/max frame sizes: " << info.minFrameSize << "/" << info.maxFrameSize << '\n';
    std::cout << "md5: ";

    for (const auto &c: info.md5) {
        std::cout << std::hex << static_cast<std::int32_t>(c) << " ";
    }

    std::cout << std::endl;

    BitsReader reader{file};

    reader.resetCrc();

    if (const auto frameSyncFromReader = reader.readBits(15); frameSyncFromReader != 0x7FFC) {
        std::cerr << "Sync code is corrupted" << '\n';
        return -1;
    }
    [[maybe_unused]] const auto strategyFromReader = reader.readBits(1);

    [[maybe_unused]] const auto blockSize = reader.readBits(4);
    if (blockSize == 6 || blockSize == 7) {
        // six seven (-_-) heheheh
        std::cerr << "Can't handle blockSize " << blockSize << '\n';
        return -1;
    }

    [[maybe_unused]] const auto sampleRate = reader.readBits(4);
    if (sampleRate == 12 || sampleRate == 13 || sampleRate == 14) {
        std::cerr << "Can't handle sampleRate " << sampleRate << '\n';
        return -1;
    }

    [[maybe_unused]] const auto channels = reader.readBits(4);
    [[maybe_unused]] const auto bitDepth = reader.readBits(3);

    if (const auto reservedBit = reader.readBits(1); reservedBit != 0) {
        std::cerr << "Frame header reserved bit equals to " << reservedBit << '\n';
        return -1;
    }

    // first byte of the coded number: count of leading 1s = total byte count (0 means a single byte)
    const auto frameNumberFirstByte = static_cast<std::uint8_t>(reader.readBits(8));
    const std::int32_t codedNumbersSize{std::countl_one(frameNumberFirstByte)};

    if (codedNumbersSize == 1 || codedNumbersSize > 7) {
        std::cerr << "frame number is corrupted" << '\n';
        return -1;
    }

    // continuation bytes (10xxxxxx each), max 6 bytes = 48 bits
    if (codedNumbersSize > 1) {
        reader.readBits((codedNumbersSize - 1) * 8);
    }

    const auto crc = reader.getCrc8();

    if (auto crcChecksum = reader.readBits(8); crcChecksum != crc) {
        std::cerr << "frame crc " << static_cast<std::uint16_t>(crc) << " is not matches with calculated " <<
                static_cast<std::uint16_t>(crcChecksum) << '\n';
    }

    if (const auto subframeMandatoryBit = reader.readBits(1); subframeMandatoryBit != 0) {
        std::cerr << "Subframe is corrupted " << '\n';
        return -1;
    }

    [[maybe_unused]] const auto subframeType = reader.readBits(6);

    const auto subframeUsesWastedBits = reader.readBits(1);

    [[maybe_unused]] std::uint32_t wastedBits{0};

    if (subframeUsesWastedBits != 0) {
        wastedBits = reader.readUnary() + 1;
        std::cerr << "Subframe reported wasted bits, handle it " << '\n';
    }

    


    // std::uint8_t subframeHeaderBeg{};
    // file.read(reinterpret_cast<char *>(&subframeHeaderBeg), sizeof(subframeHeaderBeg));
    //
    // [[maybe_unused]] auto const subframeHeaderStart = (subframeHeaderBeg >> 31) & calculateMask(1);
    // [[maybe_unused]] auto const subframeType = (subframeHeaderBeg >> 25) & calculateMask(6);

    return 0;
}
