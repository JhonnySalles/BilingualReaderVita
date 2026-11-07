#include "book_extractor_manager.h"
#include "epub_extractor.h"
#include "docx_extractor.h"
#include "fb2_extractor.h"
#include "mobi_extractor.h"
#include "archive_book_extractor.h"
#include "cbz_extractor.h"
#include "../file_utils.h"
#include <algorithm>
#include <functional>
#include <sys/stat.h>

#ifdef __vita__
#include <psp2/io/stat.h>
#endif

namespace Extractor {

static std::string getExtension(const std::string& path) {
    std::string lowerPath = path;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
    if (lowerPath.size() >= 8 && lowerPath.substr(lowerPath.size() - 8) == ".fb2.zip") {
        return "fb2.zip";
    }

    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

BookExtractorManager& BookExtractorManager::instance() {
    static BookExtractorManager s_instance;
    return s_instance;
}

BookExtractorManager::BookExtractorManager() {
    m_extractors.push_back(std::make_unique<EpubExtractor>());
    m_extractors.push_back(std::make_unique<DocxExtractor>());
    m_extractors.push_back(std::make_unique<Fb2Extractor>());
    m_extractors.push_back(std::make_unique<MobiExtractor>());
    m_extractors.push_back(std::make_unique<ArchiveBookExtractor>());
    m_extractors.push_back(std::make_unique<CbzExtractor>());
}

BookExtractor* BookExtractorManager::getExtractor(const std::string& path) const {
    std::string ext = getExtension(path);
    for (const auto& extractor : m_extractors) {
        auto formats = extractor->getSupportedFormats();
        for (const auto& f : formats) {
            if (f == ext) {
                return extractor.get();
            }
        }
    }
    return nullptr;
}

bool BookExtractorManager::isFormatSupported(const std::string& path) const {
    return getExtractor(path) != nullptr;
}

bool BookExtractorManager::extractMetadata(const std::string& path, BookMetadata& outMeta) const {
    BookExtractor* ext = getExtractor(path);
    if (!ext) return false;
    return ext->extractMetadata(path, outMeta);
}

bool BookExtractorManager::extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) const {
    BookExtractor* ext = getExtractor(path);
    if (!ext) return false;
    return ext->extractCover(path, outCoverBytes);
}

std::string BookExtractorManager::prepareForReading(const std::string& path, const std::string& cacheRoot) {
    std::string ext = getExtension(path);
    if (ext == "pdf" || ext == "xps" || ext == "cbz" || ext == "cbr" || ext == "cbt" || ext == "rar" || ext == "tar") {
        return path;
    }

    BookExtractor* extractor = getExtractor(path);
    if (!extractor) {
        return path;
    }

    // Hash the path to create a deterministic cache folder name
    size_t hashVal = std::hash<std::string>{}(path);
    char hashStr[32];
    snprintf(hashStr, sizeof(hashStr), "%08lx", static_cast<unsigned long>(hashVal));

    std::string booksCacheDir = cacheRoot + "/books";
    FileUtils::makeDirRecursive(booksCacheDir);

    std::string bookFolder = booksCacheDir + "/book_" + hashStr;
    FileUtils::makeDirRecursive(bookFolder);

    std::string cachedEpub = bookFolder + "/book.epub";
    if (FileUtils::fileExists(cachedEpub)) {
        return cachedEpub;
    }

    std::string result = extractor->extractContent(path, bookFolder);
    if (!result.empty() && FileUtils::fileExists(result)) {
        return result;
    }

    return path;
}

} // namespace Extractor

