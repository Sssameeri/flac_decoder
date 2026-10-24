#include <iostream>
#include <fstream>
#include <array>
#include <cstdint>
#include <bitset>
#include <vector>

#include "streaminfo.h"
#include "endianness.h"
#include "vorbis.h"

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

struct Frame {
    std::array<unsigned char, 8> header{};
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
                break;
            }
            case seekTable: {
                break;
            }
            case vorbisComment: {
                vorbisData = Vorbis::handleVorbisComment(file);
                break;
            }
            case cuesheet: {
                break;
            }
            case picture: {
                break;
            }
            default: {
                std::cerr << "unknown metadata block: " << currentMetadata.nextMetadataBlock << std::endl;
                break;
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

    std::array<unsigned char, 4> header{};

    file.read(reinterpret_cast<char *>(&header), sizeof(header));

    [[maybe_unused]] auto packedValues = fromBigEndianness<std::uint32_t>(header);

    [[maybe_unused]] auto frameSync = (packedValues >> 17) & 0x7FFF;
    [[maybe_unused]] auto strategy = (packedValues >> 16) & 0x0001;
    [[maybe_unused]] auto blockSize = (packedValues >> 12) & 0xF;
    [[maybe_unused]] auto sampleRate = (packedValues >> 8) & 0xF;
    [[maybe_unused]] auto channels = (packedValues >> 4) & 0xF;
    [[maybe_unused]] auto bitDepth = (packedValues >> 1) & 0x7;



    // file.read(reinterpret_cast<char *>(&blockSizeSampleRate), sizeof(blockSizeSampleRate));


    return 0;
}
