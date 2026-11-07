#pragma once

#include "book_extractor.h"
#include <string>
#include <vector>

namespace Extractor {

class CbzExtractor : public BookExtractor {
public:
    CbzExtractor() = default;
    ~CbzExtractor() override = default;

    std::vector<std::string> getSupportedFormats() const override {
        return { "cbz", "zip" };
    }

    bool extractMetadata(const std::string& path, BookMetadata& outMeta) override;
    bool extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) override;
    std::string extractContent(const std::string& path, const std::string& outputDir) override;
};

} // namespace Extractor

