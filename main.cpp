#include <iostream>
#include <fstream>
#include <array>
#include <cstdint>
#include <string>

#include "streaminfo.h"
#include "endianness.h"
#include "vorbis.h"
#include "bitsreader.h"
#include "frame.h"
#include "player.h"

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

    const auto bytes = headerPackedValue & 0xFFFFFF;

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

    Stream::Info info{};
    Vorbis::Data vorbisData{};

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

    // std::hex is sticky: switch back so later numbers print in decimal
    std::cout << std::dec << std::endl;

    /*
     * Audio frames: decode one frame, hand it to the player, repeat until the file ends
     */

    Player player{};
    if (!player.open(static_cast<std::uint32_t>(info.sampleRate), info.channels, info.bitsPerSample)) {
        return -1;
    }

    BitsReader reader{file};
    Frame::Decoded frame{};

    std::uint64_t numberOfFrames{0};
    std::uint64_t numberOfDecodedSamples{0};
    bool decodedEverything{true};

    while (!reader.atEnd()) {
        if (!Frame::read(reader, info, frame)) {
            std::cerr << "Decoding stopped at frame " << numberOfFrames << '\n';
            decodedEverything = false;
            break;
        }

        // the player was opened with the STREAMINFO format, so every frame must match it
        if (frame.samples.size() != info.channels) {
            std::cerr << "Frame " << numberOfFrames << " has " << frame.samples.size()
                    << " channels, STREAMINFO says " << info.channels << '\n';
            decodedEverything = false;
            break;
        }

        player.play(frame.samples);

        numberOfFrames++;
        numberOfDecodedSamples += frame.header.blockSize.samples;
    }

    player.close();

    std::cout << "Decoded frames: " << numberOfFrames << '\n';
    std::cout << "Decoded samples: " << numberOfDecodedSamples << " of " << info.numberOfSamples
            << (numberOfDecodedSamples == info.numberOfSamples ? " (all)" : " (MISMATCH)") << '\n';

    return decodedEverything ? 0 : -1;
}
