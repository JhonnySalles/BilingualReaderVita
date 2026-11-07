#pragma once

#include "book_extractor.h"

namespace Extractor {

class Fb2Extractor : public BookExtractor {
public:
    Fb2Extractor() = default;
    ~Fb2Extractor() override = default;

    std::vector<std::string> getSupportedFormats() const override {
        return {"fb2", "fb2.zip"};
    }

    bool extractMetadata(const std::string& path, BookMetadata& outMeta) override;
    bool extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) override;
    std::string extractContent(const std::string& path, const std::string& cacheDir) override;
};

} // namespace Extractor

