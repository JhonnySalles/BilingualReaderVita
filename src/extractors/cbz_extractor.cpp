#include "cbz_extractor.h"
#include "archive_helper.h"
#include <algorithm>
#include <cctype>

namespace Extractor {

static bool isImageExtension(const std::string& name) {
    size_t dot = name.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = name.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "webp" || ext == "bmp");
}

bool CbzExtractor::extractMetadata(const std::string& path, BookMetadata& outMeta) {
    outMeta.filePath = path;
    size_t slash = path.find_last_of("/\\");
    size_t dot = path.find_last_of('.');
    if (slash != std::string::npos && dot != std::string::npos && dot > slash) {
        outMeta.title = path.substr(slash + 1, dot - slash - 1);
    } else {
        outMeta.title = path;
    }
    return true;
}

bool CbzExtractor::extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) {
    ArchiveHelper archive;
    if (!archive.open(path)) return false;

    int total = archive.countEntries();
    std::vector<std::string> images;
    images.reserve(total);

    for (int i = 0; i < total; ++i) {
        std::string name = archive.getEntryName(i);
        if (isImageExtension(name)) {
            images.push_back(name);
        }
    }

    if (images.empty()) return false;

    // Ordenação com prioridade para imagens com "cover" ou "capa" no nome
    std::sort(images.begin(), images.end(), [](const std::string& a, const std::string& b) {
        std::string la = a;
        std::string lb = b;
        std::transform(la.begin(), la.end(), la.begin(), ::tolower);
        std::transform(lb.begin(), lb.end(), lb.begin(), ::tolower);

        bool aCover = (la.find("cover") != std::string::npos || la.find("capa") != std::string::npos);
        bool bCover = (lb.find("cover") != std::string::npos || lb.find("capa") != std::string::npos);

        if (aCover != bCover) {
            return aCover; // aCover vem primeiro
        }
        return a < b; // ordenação alfanumérica padrão
    });

    // Tentar ler a primeira imagem
    for (const auto& imgName : images) {
        if (archive.readEntry(imgName, outCoverBytes) && outCoverBytes.size() > 64) {
            return true;
        }
    }

    return false;
}

std::string CbzExtractor::extractContent(const std::string& path, const std::string& /*outputDir*/) {
    // CBZ/ZIP são lidos nativamente pelo leitor de mangá
    return path;
}

} // namespace Extractor

