//
// Created by andrii on 26-Oct-26.
//

#ifndef FLAC_BITSREADER_H
#define FLAC_BITSREADER_H

#include <cstddef>
#include <cstdint>
#include <fstream>

std::uint64_t calculateMask(std::size_t fieldSize);
std::uint8_t crc8Update(std::uint8_t crc, const std::uint8_t &c);
std::uint16_t crc16Update(std::uint16_t crc, const std::uint8_t &c);

class BitsReader {
    // буфер бітів, котрі я ще не зчитав
    std::uint64_t buffer{};

    // к-ть бітів у буфері, валідних, котрі треба обробити
    std::int32_t bitCount{};

    // файл для читання
    std::ifstream *file;

    std::uint8_t  crc8{};
    std::uint16_t crc16{};

public:
    explicit BitsReader(std::ifstream &file);

    // n - скільки я хочу зчитати
    std::uint64_t readBits(std::int32_t n);
    std::int32_t readUnary();

    void resetCrc();
    [[nodiscard]] std::uint8_t getCrc8() const;
    [[nodiscard]] std::uint16_t getCrc16() const;
};

#endif //FLAC_BITSREADER_H
