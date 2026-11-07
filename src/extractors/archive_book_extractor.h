#pragma once

#include "book_extractor.h"

namespace Extractor {

class ArchiveBookExtractor : public BookExtractor {
public:
    ArchiveBookExtractor() = default;
    ~ArchiveBookExtractor() override = default;

    std::vector<std::string> getSupportedFormats() const override {
        return {"htmlz", "txtz", "pmlz", "tcr", "md", "markdown"};
    }

    bool extractMetadata(const std::string& path, BookMetadata& outMeta) override;
    bool extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) override;
    std::string extractContent(const std::string& path, const std::string& cacheDir) override;
};

} // namespace Extractor

