#ifndef FLAC_PLAYER_H
#define FLAC_PLAYER_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>

// how many decoded frames can be queued in the sound card at once
constexpr std::size_t PLAYER_BUFFERS = 8;

// plays decoded frames through the Windows waveOut API while decoding continues
class Player {
    HWAVEOUT device{nullptr};

    // signalled by Windows every time a queued buffer finishes playing
    HANDLE bufferDoneEvent{nullptr};

    std::array<WAVEHDR, PLAYER_BUFFERS> headers{};
    std::array<std::vector<char>, PLAYER_BUFFERS> buffers{};
    std::size_t nextBuffer{0};

    // PCM container: bitsPerSample rounded up to whole bytes
    std::int32_t bytesPerSample{0};
    // how far to shift a sample up so its top bit is the container's top bit
    std::int32_t shiftToContainer{0};

    // blocks until buffer `index` is free to be filled again
    void waitForBuffer(std::size_t index);

public:
    bool open(std::uint32_t sampleRate, std::int32_t channels, std::int32_t bitsPerSample);

    // samples[channel][position], as Frame::read produces them
    void play(const std::vector<std::vector<std::int64_t> > &samples);

    // waits until everything queued has played, then closes the device
    void close();
};

#endif //FLAC_PLAYER_H
