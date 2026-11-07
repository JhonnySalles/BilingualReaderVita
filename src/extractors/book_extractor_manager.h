#pragma once

#include <string>
#include <vector>
#include <memory>
#include "book_extractor.h"

namespace Extractor {

class BookExtractorManager {
public:
    static BookExtractorManager& instance();

    bool isFormatSupported(const std::string& path) const;
    BookExtractor* getExtractor(const std::string& path) const;

    bool extractMetadata(const std::string& path, BookMetadata& outMeta) const;
    bool extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) const;

    /**
     * Prepares the given book for reading by converting it to cached book.epub if needed.
     * If the book is already a native reflowable format (e.g. .epub, .pdf, .xps) it may return the original path.
     * Returns path to the readable file (original or cached book.epub).
     */
    std::string prepareForReading(const std::string& path, const std::string& cacheRoot = "ux0:data/BilingualReaderVita/cache");

private:
    BookExtractorManager();
    std::vector<std::unique_ptr<BookExtractor>> m_extractors;
};

} // namespace Extractor

