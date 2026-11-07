#pragma once

#include "book_extractor.h"

namespace Extractor {

class MobiExtractor : public BookExtractor {
public:
    MobiExtractor() = default;
    ~MobiExtractor() override = default;

    std::vector<std::string> getSupportedFormats() const override {
        return {"mobi", "azw", "azw3", "azw4", "prc", "pdb"};
    }

    bool extractMetadata(const std::string& path, BookMetadata& outMeta) override;
    bool extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) override;
    std::string extractContent(const std::string& path, const std::string& cacheDir) override;
};

} // namespace Extractor

