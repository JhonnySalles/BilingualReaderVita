#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "spine_segmenter.h"

namespace Extractor {

class EpubPackager {
public:
    static std::string getMimeType(const std::string& fileName);

    static bool createEpub(
        const std::string& outputFilePath,
        const std::string& title,
        const std::string& author = "",
        const std::vector<uint8_t>& coverBytes = {},
        const std::string& coverFileName = "cover.jpg",
        const std::vector<GalleryItem>& galleryItems = {},
        const std::vector<ChapterItem>& chapters = {},
        const std::map<std::string, std::vector<uint8_t>>& resourceFiles = {},
        const std::string& customCss = ""
    );
};

} // namespace Extractor

