#pragma once

#include "book_extractor.h"
#include <string>
#include <vector>

namespace Extractor {

class EpubExtractor : public BookExtractor {
public:
    EpubExtractor() = default;
    ~EpubExtractor() override = default;

    std::vector<std::string> getSupportedFormats() const override {
        return { "epub", "kepub" };
    }

    bool extractMetadata(const std::string& path, BookMetadata& outMeta) override;
    bool extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) override;
    std::string extractContent(const std::string& path, const std::string& outputDir) override;
};

} // namespace Extractor

