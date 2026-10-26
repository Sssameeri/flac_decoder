//
// Created by andrii on 24-Oct-26.
//

#ifndef FLAC_VORBIS_H
#define FLAC_VORBIS_H
#include <string>
#include <fstream>
#include <vector>

namespace Vorbis {
    struct Data {
        std::string vendor{};
        std::vector<std::string> fields{};
    };

    Data handleVorbisComment(std::ifstream &file);
}

#endif //FLAC_VORBIS_H
