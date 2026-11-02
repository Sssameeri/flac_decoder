#include "player.h"

#include <iostream>

// speaker layouts for 1..8 channels, in FLAC channel order (RFC 9639, section 9.1.3)
constexpr std::array<DWORD, 8> CHANNEL_MASKS{
    0x4, // mono: front center
    0x3, // front left, front right
    0x7, // + front center
    0x33, // front left/right, back left/right
    0x37, // + front center
    0x3F, // 5.1: + LFE
    0x70F, // 6.1
    0x63F // 7.1
};

bool Player::open(const std::uint32_t sampleRate, const std::int32_t channels, const std::int32_t bitsPerSample) {
    bytesPerSample = (bitsPerSample + 7) / 8;
    shiftToContainer = bytesPerSample * 8 - bitsPerSample;

    WAVEFORMATEXTENSIBLE format{};
    format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    format.Format.nChannels = static_cast<WORD>(channels);
    format.Format.nSamplesPerSec = sampleRate;
    format.Format.wBitsPerSample = static_cast<WORD>(bytesPerSample * 8);
    format.Format.nBlockAlign = static_cast<WORD>(channels * bytesPerSample);
    format.Format.nAvgBytesPerSec = sampleRate * format.Format.nBlockAlign;
    format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    format.Samples.wValidBitsPerSample = static_cast<WORD>(bitsPerSample);
    format.dwChannelMask = channels >= 1 && channels <= 8 ? CHANNEL_MASKS[channels - 1] : 0;
    // KSDATAFORMAT_SUBTYPE_PCM, written out so no extra GUID library is needed
    format.SubFormat = GUID{0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};

    // auto-reset event: Windows sets it when a buffer is done, a wait clears it
    bufferDoneEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (bufferDoneEvent == nullptr) {
        std::cerr << "Can't create the playback event" << '\n';
        return false;
    }

    const MMRESULT result = waveOutOpen(
        &device,
        WAVE_MAPPER,
        &format.Format,
        reinterpret_cast<DWORD_PTR>(bufferDoneEvent),
        0,
        CALLBACK_EVENT
    );

    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Can't open the audio device, waveOutOpen error " << result << '\n';
        CloseHandle(bufferDoneEvent);
        bufferDoneEvent = nullptr;
        device = nullptr;
        return false;
    }

    return true;
}

void Player::waitForBuffer(const std::size_t index) {
    WAVEHDR &header = headers[index];

    // never queued (or already released): nothing to wait for
    if ((header.dwFlags & WHDR_PREPARED) == 0) return;

    while ((header.dwFlags & WHDR_DONE) == 0) {
        WaitForSingleObject(bufferDoneEvent, INFINITE);
    }

    waveOutUnprepareHeader(device, &header, sizeof(WAVEHDR));
}

void Player::play(const std::vector<std::vector<std::int64_t> > &samples) {
    if (device == nullptr || samples.empty()) return;

    const std::size_t channels{samples.size()};
    const std::size_t length{samples[0].size()};

    // the slot we are about to fill may still be playing
    waitForBuffer(nextBuffer);

    std::vector<char> &buffer = buffers[nextBuffer];
    buffer.resize(length * channels * static_cast<std::size_t>(bytesPerSample));

    // interleave: L R L R ..., each sample little-endian in bytesPerSample bytes
    std::size_t position{0};
    for (std::size_t j = 0; j < length; j++) {
        for (std::size_t c = 0; c < channels; c++) {
            std::int64_t value = samples[c][j] << shiftToContainer;

            // 8-bit PCM in Windows is unsigned
            if (bytesPerSample == 1) {
                value += 128;
            }

            for (std::int32_t b = 0; b < bytesPerSample; b++) {
                buffer[position] = static_cast<char>((value >> (8 * b)) & 0xFF);
                position++;
            }
        }
    }

    WAVEHDR &header = headers[nextBuffer];
    header = WAVEHDR{};
    header.lpData = buffer.data();
    header.dwBufferLength = static_cast<DWORD>(buffer.size());

    waveOutPrepareHeader(device, &header, sizeof(WAVEHDR));
    waveOutWrite(device, &header, sizeof(WAVEHDR));

    nextBuffer = (nextBuffer + 1) % PLAYER_BUFFERS;
}

void Player::close() {
    if (device == nullptr) return;

    for (std::size_t i = 0; i < PLAYER_BUFFERS; i++) {
        waitForBuffer(i);
    }

    waveOutClose(device);
    device = nullptr;

    CloseHandle(bufferDoneEvent);
    bufferDoneEvent = nullptr;
}
