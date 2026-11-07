#include "archive_book_extractor.h"
#include "archive_helper.h"
#include "spine_segmenter.h"
#include "epub_packager.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace Extractor {

static std::string getExtension(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

static bool readFileBytes(const std::string& path, std::vector<uint8_t>& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) {
        fclose(f);
        return false;
    }
    out.resize(sz);
    fread(out.data(), 1, sz, f);
    fclose(f);
    return true;
}

static bool isChapterHeading(const std::string& line) {
    if (line.size() < 4) return false;
    std::string lower = line;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    const char* prefixes[] = {
        "chapter", "capitulo", "capítulo", "part", "parte",
        "book", "livro", "prologue", "prólogo", "epilogue", "epílogo"
    };
    for (const char* p : prefixes) {
        if (lower.rfind(p, 0) == 0) return true;
    }
    return false;
}

bool ArchiveBookExtractor::extractMetadata(const std::string& path, BookMetadata& outMeta) {
    outMeta.filePath = path;
    std::string ext = getExtension(path);

    if (ext == "htmlz" || ext == "txtz" || ext == "pmlz") {
        ArchiveHelper archive;
        if (archive.open(path)) {
            // Check metadata.opf
            std::string opfText;
            if (archive.readEntryString("metadata.opf", opfText)) {
                ArchiveHelper helper;
                fz_xml* root = helper.parseXmlBuffer(reinterpret_cast<const uint8_t*>(opfText.data()), opfText.size());
                if (root) {
                    fz_xml* meta = fz_xml_find_down(root, "metadata");
                    if (meta) {
                        fz_xml* title = fz_xml_find_down(meta, "title");
                        if (title && fz_xml_text(title)) outMeta.title = fz_xml_text(title);
                        fz_xml* creator = fz_xml_find_down(meta, "creator");
                        if (creator && fz_xml_text(creator)) outMeta.author = fz_xml_text(creator);
                        fz_xml* desc = fz_xml_find_down(meta, "description");
                        if (desc && fz_xml_text(desc)) outMeta.annotation = fz_xml_text(desc);
                    }
                    fz_drop_xml(helper.getCtx(), root);
                }
            }
        }
    }

    if (outMeta.title.empty()) {
        size_t slash = path.find_last_of("/\\");
        size_t dot = path.find_last_of('.');
        if (slash != std::string::npos && dot != std::string::npos && dot > slash) {
            outMeta.title = path.substr(slash + 1, dot - slash - 1);
        } else {
            outMeta.title = path;
        }
    }
    return true;
}

bool ArchiveBookExtractor::extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) {
    std::string ext = getExtension(path);
    if (ext == "htmlz" || ext == "txtz" || ext == "pmlz") {
        ArchiveHelper archive;
        if (!archive.open(path)) return false;

        if (archive.hasEntry("cover.jpg") && archive.readEntry("cover.jpg", outCoverBytes)) return true;
        if (archive.hasEntry("cover.jpeg") && archive.readEntry("cover.jpeg", outCoverBytes)) return true;
        if (archive.hasEntry("cover.png") && archive.readEntry("cover.png", outCoverBytes)) return true;

        int total = archive.countEntries();
        for (int i = 0; i < total; ++i) {
            std::string name = archive.getEntryName(i);
            std::string lower = name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.find("cover") != std::string::npos &&
                (lower.find(".jpg") != std::string::npos || lower.find(".jpeg") != std::string::npos || lower.find(".png") != std::string::npos)) {
                if (archive.readEntry(name, outCoverBytes)) return true;
            }
        }
    }
    return false;
}

std::string ArchiveBookExtractor::extractContent(const std::string& path, const std::string& cacheDir) {
    std::string ext = getExtension(path);
    BookMetadata meta;
    extractMetadata(path, meta);

    std::vector<uint8_t> coverBytes;
    extractCover(path, coverBytes);

    std::vector<Block> blocks;
    std::map<std::string, std::vector<uint8_t>> resourceFiles;

    if (ext == "htmlz" || ext == "txtz" || ext == "pmlz") {
        ArchiveHelper archive;
        if (!archive.open(path)) return "";

        int total = archive.countEntries();
        for (int i = 0; i < total; ++i) {
            std::string name = archive.getEntryName(i);
            std::string lower = name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lower.find(".jpg") != std::string::npos || lower.find(".jpeg") != std::string::npos ||
                lower.find(".png") != std::string::npos || lower.find(".gif") != std::string::npos ||
                lower.find(".webp") != std::string::npos) {
                std::vector<uint8_t> imgBytes;
                if (archive.readEntry(name, imgBytes)) {
                    resourceFiles[name] = std::move(imgBytes);
                }
            }
        }

        if (ext == "htmlz") {
            std::string html;
            if (!archive.readEntryString("index.html", html)) {
                for (int i = 0; i < total; ++i) {
                    std::string name = archive.getEntryName(i);
                    if (name.find(".html") != std::string::npos || name.find(".htm") != std::string::npos) {
                        archive.readEntryString(name, html);
                        break;
                    }
                }
            }
            if (!html.empty()) {
                blocks.push_back(Block::makeHtmlText(html, static_cast<int>(html.size())));
            }
        } else if (ext == "txtz") {
            std::string txt;
            if (!archive.readEntryString("index.txt", txt)) {
                for (int i = 0; i < total; ++i) {
                    std::string name = archive.getEntryName(i);
                    if (name.find(".txt") != std::string::npos) {
                        archive.readEntryString(name, txt);
                        break;
                    }
                }
            }
            std::istringstream stream(txt);
            std::string line;
            while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.empty()) continue;

                if (isChapterHeading(line)) {
                    blocks.push_back(Block::makeBoundary(line, "", true));
                    blocks.push_back(Block::makeHtmlText("<h2>" + SpineSegmenter::escapeXml(line) + "</h2>", static_cast<int>(line.size())));
                } else {
                    blocks.push_back(Block::makeHtmlText("<p>" + SpineSegmenter::escapeXml(line) + "</p>", static_cast<int>(line.size())));
                }
            }
        } else if (ext == "pmlz") {
            std::string pml;
            for (int i = 0; i < total; ++i) {
                std::string name = archive.getEntryName(i);
                if (name.find(".pml") != std::string::npos) {
                    archive.readEntryString(name, pml);
                    break;
                }
            }
            std::istringstream stream(pml);
            std::string line;
            while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.empty()) continue;
                blocks.push_back(Block::makeHtmlText("<p>" + SpineSegmenter::escapeXml(line) + "</p>", static_cast<int>(line.size())));
            }
        }
    } else if (ext == "md" || ext == "markdown") {
        std::vector<uint8_t> mdBytes;
        if (!readFileBytes(path, mdBytes)) return "";
        std::string mdText(reinterpret_cast<const char*>(mdBytes.data()), mdBytes.size());

        std::istringstream stream(mdText);
        std::string line;
        bool inFrontmatter = false;
        bool firstLine = true;

        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (firstLine && line == "---") {
                inFrontmatter = true;
                firstLine = false;
                continue;
            }
            firstLine = false;

            if (inFrontmatter) {
                if (line == "---") {
                    inFrontmatter = false;
                }
                continue;
            }

            if (line.empty()) continue;

            if (line.rfind("### ", 0) == 0) {
                blocks.push_back(Block::makeHtmlText("<h3>" + SpineSegmenter::escapeXml(line.substr(4)) + "</h3>"));
            } else if (line.rfind("## ", 0) == 0) {
                std::string h = line.substr(3);
                blocks.push_back(Block::makeBoundary(h, "", true));
                blocks.push_back(Block::makeHtmlText("<h2>" + SpineSegmenter::escapeXml(h) + "</h2>"));
            } else if (line.rfind("# ", 0) == 0) {
                std::string h = line.substr(2);
                blocks.push_back(Block::makeBoundary(h, "", true));
                blocks.push_back(Block::makeHtmlText("<h1>" + SpineSegmenter::escapeXml(h) + "</h1>"));
            } else if (line.rfind("> ", 0) == 0) {
                blocks.push_back(Block::makeHtmlText("<blockquote><p>" + SpineSegmenter::escapeXml(line.substr(2)) + "</p></blockquote>"));
            } else {
                blocks.push_back(Block::makeHtmlText("<p>" + SpineSegmenter::escapeXml(line) + "</p>"));
            }
        }
    } else if (ext == "tcr") {
        // TCR 8-bit text format
        std::vector<uint8_t> tcrBytes;
        if (readFileBytes(path, tcrBytes) && tcrBytes.size() > 9) {
            std::string text(reinterpret_cast<const char*>(tcrBytes.data() + 9), tcrBytes.size() - 9);
            blocks.push_back(Block::makeHtmlText("<p>" + SpineSegmenter::escapeXml(text) + "</p>"));
        }
    }

    std::vector<ChapterItem> chapters = SpineSegmenter::segment(blocks, meta.title);

    std::string outputEpubPath = cacheDir + "/book.epub";
    if (EpubPackager::createEpub(outputEpubPath, meta.title, meta.author, coverBytes, "cover.jpg", {}, chapters, resourceFiles)) {
        return outputEpubPath;
    }

    return "";
}

} // namespace Extractor

