//
// Created by andrii on 24-Oct-26.
//

#include <cstdint>
#include <vector>
#include <fstream>
#include "vorbis.h"

namespace Vorbis {
    Data handleVorbisComment(std::ifstream &file) {
        std::uint32_t vorbisBytes{};
        file.read(reinterpret_cast<char *>(&vorbisBytes), sizeof(vorbisBytes));

        std::vector<char> vendorStringBytes(vorbisBytes);
        file.read(vendorStringBytes.data(), vorbisBytes);
        std::string vendorString(vendorStringBytes.data(), vendorStringBytes.size());

        std::uint32_t numOfFields{};
        file.read(reinterpret_cast<char *>(&numOfFields), sizeof(numOfFields));

        Data data{
            .vendor = vendorString
        };

        if (numOfFields > 0) {
            uint32_t counter{numOfFields};
            std::vector<std::string> fields{};

            while (counter > 0) {
                std::int32_t filedLength{};
                file.read(reinterpret_cast<char *>(&filedLength), sizeof(filedLength));
                std::vector<char> field(filedLength);
                file.read(field.data(), filedLength);
                fields.emplace_back(field.data(), field.size());
                counter--;
            }

            data.fields = fields;
        }

        return data;
    }
}
