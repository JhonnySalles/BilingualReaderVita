#pragma once

#include "book_extractor.h"

namespace Extractor {

class DocxExtractor : public BookExtractor {
public:
    DocxExtractor() = default;
    ~DocxExtractor() override = default;

    std::vector<std::string> getSupportedFormats() const override {
        return {"docx"};
    }

    bool extractMetadata(const std::string& path, BookMetadata& outMeta) override;
    bool extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) override;
    std::string extractContent(const std::string& path, const std::string& cacheDir) override;
};

} // namespace Extractor

