#include "fb2_extractor.h"
#include "archive_helper.h"
#include "spine_segmenter.h"
#include "epub_packager.h"
#include "image_size_utils.h"
#include <map>
#include <cstring>
#include <cstdio>
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

static bool readFileBytes(const std::string& path, std::vector<uint8_t>& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;

    uint8_t magic[4] = {0};
    size_t magicRead = fread(magic, 1, 4, f);
    fclose(f);

    // Se for um arquivo ZIP (magic PK\x03\x04 ou .fb2.zip)
    if (magicRead == 4 && magic[0] == 0x50 && magic[1] == 0x4B && magic[2] == 0x03 && magic[3] == 0x04) {
        ArchiveHelper zipArchive;
        if (zipArchive.open(path)) {
            int count = zipArchive.countEntries();
            for (int i = 0; i < count; ++i) {
                std::string name = zipArchive.getEntryName(i);
                std::string nLower = name;
                std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);
                if (nLower.size() > 4 && nLower.substr(nLower.size() - 4) == ".fb2") {
                    return zipArchive.readEntry(name, out);
                }
            }
        }
    }

    // Arquivo FB2 plano descompactado
    f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) {
        fclose(f);
        return false;
    }
    out.resize(sz);
    size_t rd = fread(out.data(), 1, sz, f);
    fclose(f);
    return rd == static_cast<size_t>(sz);
}

bool Fb2Extractor::extractMetadata(const std::string& path, BookMetadata& outMeta) {
    std::vector<uint8_t> xmlBytes;
    if (!readFileBytes(path, xmlBytes)) return false;

    ArchiveHelper archive;
    fz_xml* root = archive.parseXmlBuffer(xmlBytes.data(), xmlBytes.size());
    if (!root) return false;

    outMeta.filePath = path;

    fz_xml* desc = findChildTag(root, "description");
    if (desc) {
        fz_xml* tInfo = findChildTag(desc, "title-info");
        if (tInfo) {
            fz_xml* bTitle = findChildTag(tInfo, "book-title");
            if (bTitle) outMeta.title = getNodeTextRecursive(bTitle);

            fz_xml* author = findChildTag(tInfo, "author");
            if (author) {
                std::string first = getNodeTextRecursive(findChildTag(author, "first-name"));
                std::string middle = getNodeTextRecursive(findChildTag(author, "middle-name"));
                std::string last = getNodeTextRecursive(findChildTag(author, "last-name"));
                outMeta.author = first;
                if (!middle.empty()) outMeta.author += " " + middle;
                if (!last.empty()) outMeta.author += " " + last;
            }

            fz_xml* annot = findChildTag(tInfo, "annotation");
            if (annot) outMeta.annotation = getNodeTextRecursive(annot);

            fz_xml* genre = findChildTag(tInfo, "genre");
            if (genre) outMeta.genre = getNodeTextRecursive(genre);

            fz_xml* date = findChildTag(tInfo, "date");
            if (date) outMeta.releaseDate = getNodeTextRecursive(date);

            fz_xml* seq = findChildTag(tInfo, "sequence");
            if (seq) {
                char* sName = fz_xml_att(seq, "name");
                if (sName) outMeta.series = sName;
                char* sNum = fz_xml_att(seq, "number");
                if (sNum) outMeta.seriesIndex = atoi(sNum);
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

    fz_drop_xml(archive.getCtx(), root);
    return true;
}

bool Fb2Extractor::extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) {
    std::vector<uint8_t> xmlBytes;
    if (!readFileBytes(path, xmlBytes)) return false;

    ArchiveHelper archive;
    fz_xml* root = archive.parseXmlBuffer(xmlBytes.data(), xmlBytes.size());
    if (!root) return false;

    std::string coverId;
    fz_xml* coverpage = findChildTag(root, "coverpage");
    if (coverpage) {
        fz_xml* img = findChildTag(coverpage, "image");
        if (img) {
            char* href = fz_xml_att(img, "l:href");
            if (!href) href = fz_xml_att(img, "href");
            if (href) {
                if (href[0] == '#') href++;
                coverId = href;
            }
        }
    }

    std::vector<fz_xml*> binaries;
    collectAllTags(root, "binary", binaries);

    fz_xml* targetBin = nullptr;
    for (fz_xml* bin : binaries) {
        char* id = fz_xml_att(bin, "id");
        if (id) {
            if (!coverId.empty() && strcmp(id, coverId.c_str()) == 0) {
                targetBin = bin;
                break;
            } else if (coverId.empty()) {
                targetBin = bin;
                break;
            }
        }
    }

    bool success = false;
    if (targetBin) {
        std::string base64Text = getNodeTextRecursive(targetBin);
        if (!base64Text.empty()) {
            fz_buffer* buf = fz_new_buffer_from_base64(archive.getCtx(), base64Text.data(), base64Text.size());
            if (buf) {
                unsigned char* data = nullptr;
                size_t len = fz_buffer_storage(archive.getCtx(), buf, &data);
                if (len > 0 && data) {
                    outCoverBytes.assign(data, data + len);
                    success = true;
                }
                fz_drop_buffer(archive.getCtx(), buf);
            }
        }
    }

    fz_drop_xml(archive.getCtx(), root);
    return success;
}

std::string Fb2Extractor::extractContent(const std::string& path, const std::string& cacheDir) {
    std::vector<uint8_t> xmlBytes;
    if (!readFileBytes(path, xmlBytes)) return "";

    ArchiveHelper archive;
    fz_xml* root = archive.parseXmlBuffer(xmlBytes.data(), xmlBytes.size());
    if (!root) return "";

    BookMetadata meta;
    extractMetadata(path, meta);

    std::vector<uint8_t> coverBytes;
    extractCover(path, coverBytes);

    // Decode all <binary> tags into resourceFiles map
    std::map<std::string, std::vector<uint8_t>> resourceFiles;
    std::vector<fz_xml*> binaries;
    collectAllTags(root, "binary", binaries);

    for (fz_xml* bin : binaries) {
        char* id = fz_xml_att(bin, "id");
        if (id) {
            std::string base64Text = getNodeTextRecursive(bin);
            if (!base64Text.empty()) {
                fz_buffer* buf = fz_new_buffer_from_base64(archive.getCtx(), base64Text.data(), base64Text.size());
                if (buf) {
                    unsigned char* data = nullptr;
                    size_t len = fz_buffer_storage(archive.getCtx(), buf, &data);
                    if (len > 0 && data) {
                        resourceFiles[id] = std::vector<uint8_t>(data, data + len);
                    }
                    fz_drop_buffer(archive.getCtx(), buf);
                }
            }
        }
    }

    std::vector<Block> blocks;
    std::vector<fz_xml*> bodies;
    collectAllTags(root, "body", bodies);

    for (fz_xml* body : bodies) {
        char* name = fz_xml_att(body, "name");
        bool isNotes = name && strcmp(name, "notes") == 0;

        std::vector<fz_xml*> sections;
        collectAllTags(body, "section", sections);
        if (sections.empty()) sections.push_back(body);

        for (fz_xml* sec : sections) {
            char* secId = fz_xml_att(sec, "id");
            fz_xml* titleNode = findChildTag(sec, "title");
            std::string titleText = titleNode ? getNodeTextRecursive(titleNode) : "";

            blocks.push_back(Block::makeBoundary(titleText, secId ? secId : "", true));

            if (!titleText.empty()) {
                blocks.push_back(Block::makeHtmlText("<h2>" + SpineSegmenter::escapeXml(titleText) + "</h2>",
                                                    static_cast<int>(titleText.size())));
            }

            // Paragraphs and images
            for (fz_xml* child = fz_xml_down(sec); child; child = fz_xml_next(child)) {
                char* tag = fz_xml_tag(child);
                if (!tag) continue;

                if (strcmp(tag, "p") == 0) {
                    std::string pText = getNodeTextRecursive(child);
                    if (!pText.empty()) {
                        blocks.push_back(Block::makeHtmlText("<p>" + SpineSegmenter::escapeXml(pText) + "</p>",
                                                            static_cast<int>(pText.size())));
                    }
                } else if (strcmp(tag, "image") == 0) {
                    char* href = fz_xml_att(child, "l:href");
                    if (!href) href = fz_xml_att(child, "href");
                    if (href) {
                        if (href[0] == '#') href++;
                        auto it = resourceFiles.find(href);
                        if (it != resourceFiles.end()) {
                            auto dims = ImageSizeUtils::readDimensions(it->second);
                            auto role = ImageSizeUtils::classify(dims);
                            blocks.push_back(Block::makeImage(href, "Image", role, it->second, dims));
                        }
                    }
                } else if (strcmp(tag, "epigraph") == 0 || strcmp(tag, "cite") == 0 || strcmp(tag, "poem") == 0) {
                    std::string qText = getNodeTextRecursive(child);
                    if (!qText.empty()) {
                        blocks.push_back(Block::makeHtmlText("<blockquote><p>" + SpineSegmenter::escapeXml(qText) + "</p></blockquote>",
                                                            static_cast<int>(qText.size())));
                    }
                }
            }
        }
    }

    fz_drop_xml(archive.getCtx(), root);

    std::vector<ChapterItem> chapters = SpineSegmenter::segment(blocks, meta.title);

    std::string outputEpubPath = cacheDir + "/book.epub";
    if (EpubPackager::createEpub(outputEpubPath, meta.title, meta.author, coverBytes, "cover.jpg", {}, chapters, resourceFiles)) {
        return outputEpubPath;
    }

    return "";
}

} // namespace Extractor

