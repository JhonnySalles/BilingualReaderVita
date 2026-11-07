#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "book_metadata.h"

namespace Extractor {

class BookExtractor {
public:
    virtual ~BookExtractor() = default;

    virtual std::vector<std::string> getSupportedFormats() const = 0;
    virtual bool extractMetadata(const std::string& path, BookMetadata& outMeta) = 0;
    virtual bool extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) = 0;
    virtual std::string extractContent(const std::string& path, const std::string& cacheDir) = 0;
};

} // namespace Extractor

