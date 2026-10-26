//
// Created by andrii on 26-Oct-26.
//

#include "bitsreader.h"

#include <assert.h>
#include <iostream>

std::uint64_t calculateMask(const std::size_t fieldSize) {
    return ((1ULL << fieldSize) - 1ULL);
}

std::uint8_t crc8Update(std::uint8_t crc, const std::uint8_t &c) {
    crc = crc ^ c;
    for (uint32_t b = 0; b < 8; b++) {
        // if first is 1
        if (crc & 0b10000000) {
            constexpr std::uint16_t polynomialBits{0x07};
            crc = (crc << 1) ^ polynomialBits;
        } else {
            crc = crc << 1;
        }
    }

    return crc;
}

std::uint16_t crc16Update(std::uint16_t crc, const std::uint8_t &c) {
    // the byte goes into the top 8 bits of the 16-bit register
    crc = crc ^ (c << 8);
    for (uint32_t b = 0; b < 8; b++) {
        // if first (bit 15) is 1
        if (crc & 0x8000) {
            constexpr std::uint16_t polynomialBits{0x8005};
            crc = (crc << 1) ^ polynomialBits;
        } else {
            crc = crc << 1;
        }
    }

    return crc;
}

BitsReader::BitsReader(std::ifstream &file) : file(&file) {
}

std::uint64_t BitsReader::readBits(const std::int32_t n) {
    assert(0 < n && n <= 56);

    std::uint64_t rawBits{};

    // якщо к-ть бітів у буфері, менше, ніж ті, що запитав, то зчитай
    while (bitCount < n) {
        // читаємо мінімально доступну к-ть інфи - 1 байт
        std::uint8_t byteBuffer{};
        // якщо немає помилки
        if (file->read(reinterpret_cast<char *>(&byteBuffer), sizeof(byteBuffer))) {
            //зчитуємо один байт і закидаємо у буфер
            buffer = (buffer << 8) | byteBuffer;

            crc8 = crc8Update(crc8, byteBuffer);
            crc16 = crc16Update(crc16, byteBuffer);
            // додаки к-ть зчитаних біт
            bitCount += 8;
        } else {
            std::cerr << "Can't read bits" << '\n';
            break;
        }
    }

    // додати у rawBits к-ть
    rawBits = (buffer >> (bitCount - n)) & calculateMask(n);
    bitCount -= n;

    return rawBits;
}

std::int32_t BitsReader::readUnary() {
    std::int32_t value{0};
    while (const auto bit = readBits(1)) {
        if (bit == 0) value++;
    }

    return value;
}

void BitsReader::resetCrc() {
    crc8 = 0;
    crc16 = 0;
}

std::uint8_t BitsReader::getCrc8() const {
    return crc8;
}

std::uint16_t BitsReader::getCrc16() const {
    return crc16;
}
