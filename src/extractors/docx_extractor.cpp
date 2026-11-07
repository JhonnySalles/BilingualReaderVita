#include "docx_extractor.h"
#include "archive_helper.h"
#include "spine_segmenter.h"
#include "epub_packager.h"
#include "image_size_utils.h"
#include <map>
#include <cstring>
#include <algorithm>

namespace Extractor {

static std::string getNodeTextRecursive(fz_xml* node) {
    if (!node) return "";
    std::string text;
    for (fz_xml* child = fz_xml_down(node); child; child = fz_xml_next(child)) {
        char* t = fz_xml_text(child);
        if (t) text += t;
        text += getNodeTextRecursive(child);
    }
    return text;
}

static fz_xml* findChildTag(fz_xml* node, const char* tagName) {
    if (!node) return nullptr;
    for (fz_xml* child = fz_xml_down(node); child; child = fz_xml_next(child)) {
        char* tag = fz_xml_tag(child);
        if (tag) {
            // Match tag directly or ignoring prefix like "dc:title" vs "title"
            if (strcmp(tag, tagName) == 0) return child;
            const char* colon = strchr(tag, ':');
            if (colon && strcmp(colon + 1, tagName) == 0) return child;
        }
        fz_xml* sub = findChildTag(child, tagName);
        if (sub) return sub;
    }
    return nullptr;
}

static void collectAllTags(fz_xml* node, const char* tagName, std::vector<fz_xml*>& outList) {
    if (!node) return;
    for (fz_xml* child = fz_xml_down(node); child; child = fz_xml_next(child)) {
        char* tag = fz_xml_tag(child);
        if (tag) {
            if (strcmp(tag, tagName) == 0) {
                outList.push_back(child);
            } else {
                const char* colon = strchr(tag, ':');
                if (colon && strcmp(colon + 1, tagName) == 0) {
                    outList.push_back(child);
                }
            }
        }
        collectAllTags(child, tagName, outList);
    }
}

bool DocxExtractor::extractMetadata(const std::string& path, BookMetadata& outMeta) {
    ArchiveHelper archive;
    if (!archive.open(path)) return false;

    outMeta.filePath = path;

    // 1. docProps/core.xml
    fz_xml* coreXml = archive.parseXmlEntry("docProps/core.xml");
    if (coreXml) {
        fz_xml* tNode = findChildTag(coreXml, "title");
        if (tNode) outMeta.title = getNodeTextRecursive(tNode);

        fz_xml* cNode = findChildTag(coreXml, "creator");
        if (cNode) outMeta.author = getNodeTextRecursive(cNode);

        fz_xml* dNode = findChildTag(coreXml, "description");
        if (dNode) outMeta.annotation = getNodeTextRecursive(dNode);

        fz_xml* sNode = findChildTag(coreXml, "subject");
        if (sNode) outMeta.genre = getNodeTextRecursive(sNode);

        fz_xml* dateNode = findChildTag(coreXml, "created");
        if (dateNode) outMeta.releaseDate = getNodeTextRecursive(dateNode);

        fz_drop_xml(archive.getCtx(), coreXml);
    }

    // 2. docProps/app.xml (Company = publisher)
    fz_xml* appXml = archive.parseXmlEntry("docProps/app.xml");
    if (appXml) {
        fz_xml* compNode = findChildTag(appXml, "Company");
        if (compNode) outMeta.publisher = getNodeTextRecursive(compNode);
        fz_drop_xml(archive.getCtx(), appXml);
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

bool DocxExtractor::extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) {
    ArchiveHelper archive;
    if (!archive.open(path)) return false;

    // 1. Fast path: check docProps/thumbnail.jpeg or docProps/thumbnail.png (standard Word/Calibre cover)
    if (archive.hasEntry("docProps/thumbnail.jpeg") && archive.readEntry("docProps/thumbnail.jpeg", outCoverBytes)) {
        return true;
    }
    if (archive.hasEntry("docProps/thumbnail.png") && archive.readEntry("docProps/thumbnail.png", outCoverBytes)) {
        return true;
    }

    // 2. Check if any file in word/media/ contains "cover"
    int total = archive.countEntries();
    for (int i = 0; i < total; ++i) {
        std::string name = archive.getEntryName(i);
        std::string nLower = name;
        std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);
        if (nLower.rfind("word/media/", 0) == 0 && nLower.find("cover") != std::string::npos) {
            size_t dot = name.find_last_of('.');
            if (dot != std::string::npos) {
                std::string ext = nLower.substr(dot);
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".webp") {
                    if (archive.readEntry(name, outCoverBytes) && outCoverBytes.size() > 64) {
                        return true;
                    }
                }
            }
        }
    }

    // 3. Map relationships word/_rels/document.xml.rels
    std::map<std::string, std::string> relations;
    fz_xml* relsXml = archive.parseXmlEntry("word/_rels/document.xml.rels");
    if (relsXml) {
        std::vector<fz_xml*> relNodes;
        collectAllTags(relsXml, "Relationship", relNodes);
        for (fz_xml* r : relNodes) {
            char* id = fz_xml_att(r, "Id");
            char* target = fz_xml_att(r, "Target");
            char* type = fz_xml_att(r, "Type");
            if (id && target && type && strstr(type, "image")) {
                relations[id] = target;
            }
        }
        fz_drop_xml(archive.getCtx(), relsXml);
    }

    // 4. Check first image reference in word/document.xml
    std::string firstImageTarget;
    fz_xml* docXml = archive.parseXmlEntry("word/document.xml");
    if (docXml) {
        fz_xml* blip = findChildTag(docXml, "blip");
        if (blip) {
            char* embed = fz_xml_att(blip, "r:embed");
            if (!embed) embed = fz_xml_att(blip, "embed");
            if (embed && relations.find(embed) != relations.end()) {
                firstImageTarget = relations[embed];
            }
        }
        if (firstImageTarget.empty()) {
            fz_xml* imgData = findChildTag(docXml, "imagedata");
            if (imgData) {
                char* rId = fz_xml_att(imgData, "r:id");
                if (!rId) rId = fz_xml_att(imgData, "id");
                if (rId && relations.find(rId) != relations.end()) {
                    firstImageTarget = relations[rId];
                }
            }
        }
        fz_drop_xml(archive.getCtx(), docXml);
    }

    if (!firstImageTarget.empty()) {
        std::string zipPath = "word/" + firstImageTarget;
        if (archive.hasEntry(zipPath) && archive.readEntry(zipPath, outCoverBytes) && outCoverBytes.size() > 64) {
            return true;
        }
        if (archive.hasEntry(firstImageTarget) && archive.readEntry(firstImageTarget, outCoverBytes) && outCoverBytes.size() > 64) {
            return true;
        }
    }

    // 5. Fallback: first image in word/media/
    for (int i = 0; i < total; ++i) {
        std::string name = archive.getEntryName(i);
        if (name.rfind("word/media/", 0) == 0) {
            size_t dot = name.find_last_of('.');
            if (dot != std::string::npos) {
                std::string ext = name.substr(dot);
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
                    if (archive.readEntry(name, outCoverBytes) && outCoverBytes.size() > 64) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

std::string DocxExtractor::extractContent(const std::string& path, const std::string& cacheDir) {
    ArchiveHelper archive;
    if (!archive.open(path)) return "";

    std::map<std::string, std::string> relations;
    fz_xml* relsXml = archive.parseXmlEntry("word/_rels/document.xml.rels");
    if (relsXml) {
        std::vector<fz_xml*> relNodes;
        collectAllTags(relsXml, "Relationship", relNodes);
        for (fz_xml* r : relNodes) {
            char* id = fz_xml_att(r, "Id");
            char* target = fz_xml_att(r, "Target");
            char* type = fz_xml_att(r, "Type");
            if (id && target && type && strstr(type, "image")) {
                relations[id] = target;
            }
        }
        fz_drop_xml(archive.getCtx(), relsXml);
    }

    std::map<std::string, std::vector<uint8_t>> resourceFiles;
    for (const auto& pair : relations) {
        const std::string& target = pair.second;
        std::string zipPath = "word/" + target;
        std::vector<uint8_t> data;
        if (archive.readEntry(zipPath, data)) {
            resourceFiles[target] = std::move(data);
        } else if (archive.readEntry(target, data)) {
            resourceFiles[target] = std::move(data);
        }
    }

    BookMetadata meta;
    extractMetadata(path, meta);

    std::vector<uint8_t> coverBytes;
    extractCover(path, coverBytes);

    fz_xml* docXml = archive.parseXmlEntry("word/document.xml");
    if (!docXml) return "";

    std::vector<Block> blocks;
    std::vector<fz_xml*> paragraphs;
    collectAllTags(docXml, "p", paragraphs);

    int headingCount = 0;

    for (fz_xml* p : paragraphs) {
        // Page break?
        if (findChildTag(p, "pageBreakBefore") || findChildTag(p, "lastRenderedPageBreak")) {
            blocks.push_back(Block::makeBoundary("", "", true));
        }

        // Bookmark anchor?
        fz_xml* bm = findChildTag(p, "bookmarkStart");
        if (bm) {
            char* name = fz_xml_att(bm, "w:name");
            if (!name) name = fz_xml_att(bm, "name");
            if (name && name[0] != '_') {
                blocks.push_back(Block::makeHtmlText("<a id=\"" + std::string(name) + "\"></a>", 0, name));
            }
        }

        // Images?
        std::vector<fz_xml*> blips;
        collectAllTags(p, "blip", blips);
        collectAllTags(p, "imagedata", blips);

        for (fz_xml* blip : blips) {
            char* embed = fz_xml_att(blip, "r:embed");
            if (!embed) embed = fz_xml_att(blip, "embed");
            if (!embed) embed = fz_xml_att(blip, "r:id");
            if (!embed) embed = fz_xml_att(blip, "id");

            if (embed && relations.find(embed) != relations.end()) {
                const std::string& target = relations[embed];
                auto it = resourceFiles.find(target);
                if (it != resourceFiles.end()) {
                    auto dims = ImageSizeUtils::readDimensions(it->second);
                    auto role = ImageSizeUtils::classify(dims);
                    blocks.push_back(Block::makeImage(target, "Image", role, it->second, dims));
                }
            }
        }

        // Paragraph text
        std::string pText;
        std::vector<fz_xml*> texts;
        collectAllTags(p, "t", texts);
        for (fz_xml* tNode : texts) {
            pText += getNodeTextRecursive(tNode);
        }

        if (!pText.empty()) {
            fz_xml* pStyle = findChildTag(p, "pStyle");
            char* styleVal = pStyle ? fz_xml_att(pStyle, "w:val") : nullptr;
            if (!styleVal && pStyle) styleVal = fz_xml_att(pStyle, "val");

            bool isHeading = styleVal && (strstr(styleVal, "Heading") || strstr(styleVal, "heading") || strstr(styleVal, "Titulo") || strstr(styleVal, "titulo"));

            if (isHeading) {
                headingCount++;
                std::string anchor = "heading_" + std::to_string(headingCount);
                std::string hHtml = "<h2 id=\"" + anchor + "\">" + SpineSegmenter::escapeXml(pText) + "</h2>";
                blocks.push_back(Block::makeHtmlText(hHtml, static_cast<int>(pText.size()), anchor));
            } else {
                std::string pHtml = "<p>" + SpineSegmenter::escapeXml(pText) + "</p>";
                blocks.push_back(Block::makeHtmlText(pHtml, static_cast<int>(pText.size())));
            }
        }
    }

    fz_drop_xml(archive.getCtx(), docXml);

    std::vector<ChapterItem> chapters = SpineSegmenter::segment(blocks, meta.title);

    std::string outputEpubPath = cacheDir + "/book.epub";
    if (EpubPackager::createEpub(outputEpubPath, meta.title, meta.author, coverBytes, "cover.jpg", {}, chapters, resourceFiles)) {
        return outputEpubPath;
    }

    return "";
}

} // namespace Extractor

