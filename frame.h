#ifndef FLAC_FRAME_H
#define FLAC_FRAME_H
#include <cstdint>
#include <vector>

#include "bitdepth.h"
#include "bitsreader.h"
#include "blocksize.h"
#include "channels.h"
#include "samplerate.h"
#include "streaminfo.h"

namespace Frame {
    struct Header {
        std::uint8_t blockingStrategy{0};
        BlockSize blockSize{};
        SampleRate sampleRate{};
        Channels channels{};
        BitDepth bitDepth{};
    };

    struct Decoded {
        Header header{};
        // samples[channel][position]
        std::vector<std::vector<std::int64_t> > samples{};
    };

    // sync code through CRC-8
    bool readHeader(BitsReader &reader, const Stream::Info &info, Header &out);

    // whole frame: header, subframes, decorrelation, padding, CRC-16
    bool read(BitsReader &reader, const Stream::Info &info, Decoded &out);

    // 1 if subframe `index` is the side channel (it is stored with one extra bit)
    std::int32_t subframeSideBit(const Channels &channels, std::int32_t index);

    void decorrelateChannels(std::vector<std::vector<std::int64_t> > &samples, const Channels &channels);
}

#endif //FLAC_FRAME_H
