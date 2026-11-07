#include "epub_extractor.h"
#include "archive_helper.h"
#include "zip_writer.h"
#include "../file_utils.h"
#include <algorithm>
#include <cstring>
#include <strings.h>
#include <cstdio>
#include <cctype>
#include <set>
#include <map>

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
            if (strcasecmp(tag, tagName) == 0) return child;
            const char* colon = strchr(tag, ':');
            if (colon && strcasecmp(colon + 1, tagName) == 0) return child;
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
            if (strcasecmp(tag, tagName) == 0) {
                outList.push_back(child);
            } else {
                const char* colon = strchr(tag, ':');
                if (colon && strcasecmp(colon + 1, tagName) == 0) {
                    outList.push_back(child);
                }
            }
        }
        collectAllTags(child, tagName, outList);
    }
}

static std::string resolveRelativePath(const std::string& basePath, const std::string& relativePath) {
    if (relativePath.empty()) return "";
    if (relativePath[0] == '/') return relativePath.substr(1);
    size_t slash = basePath.find_last_of('/');
    if (slash != std::string::npos) {
        return basePath.substr(0, slash + 1) + relativePath;
    }
    return relativePath;
}

static std::string findOpfPath(ArchiveHelper& archive) {
    // 1. Tentar ler META-INF/container.xml
    std::vector<uint8_t> containerBytes;
    if (archive.readEntry("META-INF/container.xml", containerBytes)) {
        fz_xml* cRoot = archive.parseXmlBuffer(containerBytes.data(), containerBytes.size());
        if (cRoot) {
            std::vector<fz_xml*> rootfiles;
            collectAllTags(cRoot, "rootfile", rootfiles);
            for (fz_xml* rf : rootfiles) {
                char* fullPath = fz_xml_att(rf, "full-path");
                if (fullPath && strlen(fullPath) > 0) {
                    return fullPath;
                }
            }
        }
    }

    // 2. Fallback: procurar por qualquer arquivo .opf no arquivo compactado
    int count = archive.countEntries();
    for (int i = 0; i < count; ++i) {
        std::string name = archive.getEntryName(i);
        if (name.size() > 4) {
            std::string ext = name.substr(name.size() - 4);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".opf") {
                return name;
            }
        }
    }

    return "";
}

std::string EpubExtractor::stripHtmlTagsAndEntities(const std::string& input) {
    std::string result;
    result.reserve(input.size());

    bool inTag = false;
    std::string curTag;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '<') {
            inTag = true;
            curTag.clear();
        } else if (c == '>') {
            inTag = false;
            std::string tagLower = curTag;
            std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::tolower);
            if (tagLower == "p" || tagLower == "/p" || tagLower == "br" || tagLower == "br/" ||
                tagLower == "div" || tagLower == "/div") {
                if (result.empty() || result.back() != '\n') {
                    result += '\n';
                }
            }
        } else if (inTag) {
            if (curTag.size() < 16) curTag += c;
        } else if (c == '&') {
            size_t semi = input.find(';', i);
            if (semi != std::string::npos && (semi - i) <= 10) {
                std::string ent = input.substr(i + 1, semi - i - 1);
                if (ent == "nbsp") result += ' ';
                else if (ent == "amp") result += '&';
                else if (ent == "lt") result += '<';
                else if (ent == "gt") result += '>';
                else if (ent == "quot") result += '"';
                else if (ent == "apos") result += '\'';
                else result += ' ';
                i = semi;
            } else {
                result += c;
            }
        } else {
            result += c;
        }
    }

    // Limpeza de quebras de linha múltiplas e espaços excessivos
    std::string clean;
    clean.reserve(result.size());
    int consecutiveNewlines = 0;
    for (char c : result) {
        if (c == '\r') continue;
        if (c == '\n') {
            consecutiveNewlines++;
            if (consecutiveNewlines <= 2) clean += c;
        } else {
            consecutiveNewlines = 0;
            clean += c;
        }
    }

    // Trim inicial e final
    size_t first = clean.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = clean.find_last_not_of(" \t\n\r");
    return clean.substr(first, last - first + 1);
}

static void extractChaptersFromNcx(ArchiveHelper& archive, const std::string& ncxPath, const std::map<std::string, int>& hrefToPage, std::vector<ChapterItem>& outChapters) {
    std::vector<uint8_t> ncxBytes;
    if (!archive.readEntry(ncxPath, ncxBytes)) return;

    fz_xml* root = archive.parseXmlBuffer(ncxBytes.data(), ncxBytes.size());
    if (!root) return;

    std::vector<fz_xml*> navPoints;
    collectAllTags(root, "navPoint", navPoints);

    for (fz_xml* np : navPoints) {
        fz_xml* navLabel = findChildTag(np, "navLabel");
        std::string title;
        if (navLabel) {
            fz_xml* textNode = findChildTag(navLabel, "text");
            if (textNode) title = getNodeTextRecursive(textNode);
        }
        fz_xml* contentNode = findChildTag(np, "content");
        std::string src;
        if (contentNode) {
            char* s = fz_xml_att(contentNode, "src");
            if (s) src = s;
        }

        if (!title.empty()) {
            size_t hash = src.find('#');
            std::string srcClean = (hash != std::string::npos) ? src.substr(0, hash) : src;
            size_t slash = srcClean.find_last_of('/');
            if (slash != std::string::npos) srcClean = srcClean.substr(slash + 1);

            int page = 0;
            auto it = hrefToPage.find(srcClean);
            if (it != hrefToPage.end()) {
                page = it->second;
            }

            ChapterItem ch;
            ch.title = title;
            ch.pageIndex = page;
            ch.isBookmark = false;
            outChapters.push_back(ch);
        }
    }
}

bool EpubExtractor::extractMetadata(const std::string& path, BookMetadata& outMeta) {
    ArchiveHelper archive;
    if (!archive.open(path)) return false;

    std::string opfPath = findOpfPath(archive);
    if (opfPath.empty()) return false;

    std::vector<uint8_t> opfBytes;
    if (!archive.readEntry(opfPath, opfBytes)) return false;

    fz_xml* root = archive.parseXmlBuffer(opfBytes.data(), opfBytes.size());
    if (!root) return false;

    outMeta.filePath = path;

    fz_xml* metadataNode = findChildTag(root, "metadata");
    if (metadataNode) {
        fz_xml* titleNode = findChildTag(metadataNode, "title");
        if (titleNode) outMeta.title = getNodeTextRecursive(titleNode);

        // Múltiplos Autores
        std::vector<fz_xml*> creators;
        collectAllTags(metadataNode, "creator", creators);
        std::string authorsJoined;
        for (size_t i = 0; i < creators.size(); ++i) {
            std::string a = getNodeTextRecursive(creators[i]);
            if (!a.empty()) {
                if (!authorsJoined.empty()) authorsJoined += ", ";
                authorsJoined += a;
            }
        }
        outMeta.author = authorsJoined;

        fz_xml* descNode = findChildTag(metadataNode, "description");
        if (descNode) {
            std::string rawDesc = getNodeTextRecursive(descNode);
            outMeta.annotation = stripHtmlTagsAndEntities(rawDesc);
        }

        fz_xml* pubNode = findChildTag(metadataNode, "publisher");
        if (pubNode) outMeta.publisher = getNodeTextRecursive(pubNode);

        fz_xml* dateNode = findChildTag(metadataNode, "date");
        if (dateNode) {
            std::string d = getNodeTextRecursive(dateNode);
            if (d.size() >= 10 && d[4] == '-' && d[7] == '-') {
                // Formato YYYY-MM-DD para DD/MM/YYYY
                char buf[32];
                snprintf(buf, sizeof(buf), "%s/%s/%s", d.substr(8, 2).c_str(), d.substr(5, 2).c_str(), d.substr(0, 4).c_str());
                outMeta.releaseDate = buf;
            } else {
                outMeta.releaseDate = d;
            }
        }

        fz_xml* langNode = findChildTag(metadataNode, "language");
        if (langNode) outMeta.language = getNodeTextRecursive(langNode);

        // Múltiplos Assuntos/Gêneros
        std::vector<fz_xml*> subjects;
        collectAllTags(metadataNode, "subject", subjects);
        std::string genresJoined;
        for (size_t i = 0; i < subjects.size(); ++i) {
            std::string s = getNodeTextRecursive(subjects[i]);
            if (!s.empty()) {
                outMeta.subjects.push_back(s);
                if (!genresJoined.empty()) genresJoined += ", ";
                genresJoined += s;
            }
        }
        outMeta.genre = genresJoined;

        // Identificadores (ISBN)
        std::vector<fz_xml*> identifiers;
        collectAllTags(metadataNode, "identifier", identifiers);
        for (fz_xml* idNode : identifiers) {
            std::string idText = getNodeTextRecursive(idNode);
            char* scheme = fz_xml_att(idNode, "scheme");
            char* idAttr = fz_xml_att(idNode, "id");
            if ((scheme && strcasecmp(scheme, "isbn") == 0) ||
                (idAttr && strstr(idAttr, "isbn") != nullptr) ||
                idText.find("978-") != std::string::npos || idText.find("979-") != std::string::npos) {
                outMeta.isbn = idText;
                break;
            }
        }

        // Série e Volume (Calibre meta e EPUB 3 meta)
        std::vector<fz_xml*> metas;
        collectAllTags(metadataNode, "meta", metas);
        for (fz_xml* m : metas) {
            char* name = fz_xml_att(m, "name");
            char* content = fz_xml_att(m, "content");
            char* prop = fz_xml_att(m, "property");

            if (name && content) {
                if (strcmp(name, "calibre:series") == 0) {
                    outMeta.series = content;
                } else if (strcmp(name, "calibre:series_index") == 0) {
                    try { outMeta.seriesIndex = static_cast<int>(std::stof(content)); } catch (...) {}
                }
            }

            if (prop) {
                if (strcmp(prop, "belongs-to-collection") == 0) {
                    outMeta.series = getNodeTextRecursive(m);
                } else if (strcmp(prop, "group-position") == 0) {
                    std::string pos = getNodeTextRecursive(m);
                    try { outMeta.seriesIndex = std::stoi(pos); } catch (...) {}
                }
            }
        }
    }

    // Mapeamento do Spine para índices de página dos capítulos
    std::map<std::string, std::string> idToHref;
    std::string ncxHref;
    std::vector<fz_xml*> items;
    collectAllTags(root, "item", items);
    for (fz_xml* it : items) {
        char* id = fz_xml_att(it, "id");
        char* href = fz_xml_att(it, "href");
        char* media = fz_xml_att(it, "media-type");
        if (id && href) {
            idToHref[id] = href;
        }
        if (media && strcmp(media, "application/x-dtbncx+xml") == 0 && href) {
            ncxHref = href;
        }
    }

    std::map<std::string, int> hrefToPage;
    std::vector<fz_xml*> itemrefs;
    collectAllTags(root, "itemref", itemrefs);
    for (size_t i = 0; i < itemrefs.size(); ++i) {
        char* idref = fz_xml_att(itemrefs[i], "idref");
        if (idref) {
            auto it = idToHref.find(idref);
            if (it != idToHref.end()) {
                std::string href = it->second;
                size_t slash = href.find_last_of('/');
                if (slash != std::string::npos) href = href.substr(slash + 1);
                hrefToPage[href] = static_cast<int>(i);
            }
        }
    }

    if (!ncxHref.empty()) {
        std::string fullNcxPath = resolveRelativePath(opfPath, ncxHref);
        extractChaptersFromNcx(archive, fullNcxPath, hrefToPage, outMeta.chapters);
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

bool EpubExtractor::extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) {
    ArchiveHelper archive;
    if (!archive.open(path)) return false;

    std::string opfPath = findOpfPath(archive);
    if (!opfPath.empty()) {
        std::vector<uint8_t> opfBytes;
        if (archive.readEntry(opfPath, opfBytes)) {
            fz_xml* root = archive.parseXmlBuffer(opfBytes.data(), opfBytes.size());
            if (root) {
                std::string coverId;
                std::string coverHref;

                // 1. Procura <meta name="cover" content="ID"/>
                std::vector<fz_xml*> metaTags;
                collectAllTags(root, "meta", metaTags);
                for (fz_xml* m : metaTags) {
                    char* name = fz_xml_att(m, "name");
                    char* content = fz_xml_att(m, "content");
                    if (name && content && strcmp(name, "cover") == 0) {
                        coverId = content;
                        break;
                    }
                }

                // 2. Coleta itens do manifest
                std::vector<fz_xml*> itemTags;
                collectAllTags(root, "item", itemTags);

                // Busca por ID identificado na meta cover
                if (!coverId.empty()) {
                    for (fz_xml* it : itemTags) {
                        char* id = fz_xml_att(it, "id");
                        char* href = fz_xml_att(it, "href");
                        if (id && href && strcmp(id, coverId.c_str()) == 0) {
                            coverHref = href;
                            break;
                        }
                    }
                }

                // 3. Se não achou, procura item com properties="cover-image" (EPUB 3)
                if (coverHref.empty()) {
                    for (fz_xml* it : itemTags) {
                        char* props = fz_xml_att(it, "properties");
                        char* href = fz_xml_att(it, "href");
                        if (props && href && strstr(props, "cover-image") != nullptr) {
                            coverHref = href;
                            break;
                        }
                    }
                }

                // 4. Se ainda não achou, procura item cujo id ou href contenha "cover" e media-type="image/..."
                if (coverHref.empty()) {
                    for (fz_xml* it : itemTags) {
                        char* href = fz_xml_att(it, "href");
                        char* id = fz_xml_att(it, "id");
                        char* media = fz_xml_att(it, "media-type");
                        if (media && strncmp(media, "image/", 6) == 0 && href) {
                            std::string hLower = href;
                            std::transform(hLower.begin(), hLower.end(), hLower.begin(), ::tolower);
                            std::string idLower = id ? id : "";
                            std::transform(idLower.begin(), idLower.end(), idLower.begin(), ::tolower);
                            if (hLower.find("cover") != std::string::npos || idLower.find("cover") != std::string::npos) {
                                coverHref = href;
                                break;
                            }
                        }
                    }
                }

                if (!coverHref.empty()) {
                    std::string fullCoverPath = resolveRelativePath(opfPath, coverHref);
                    if (archive.readEntry(fullCoverPath, outCoverBytes) && outCoverBytes.size() > 64) {
                        return true;
                    }
                }
            }
        }
    }

    // 5. Fallback: procurar no archive qualquer entrada cujo nome contenha "cover"
    int count = archive.countEntries();
    for (int i = 0; i < count; ++i) {
        std::string name = archive.getEntryName(i);
        std::string nLower = name;
        std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);

        if (nLower.find("cover") != std::string::npos) {
            if (nLower.find(".jpg") != std::string::npos ||
                nLower.find(".jpeg") != std::string::npos ||
                nLower.find(".png") != std::string::npos ||
                nLower.find(".webp") != std::string::npos) {
                if (archive.readEntry(name, outCoverBytes) && outCoverBytes.size() > 64) {
                    return true;
                }
            }
        }
    }

    // 6. Fallback final: primeira imagem válida com tamanho > 2KB
    for (int i = 0; i < count; ++i) {
        std::string name = archive.getEntryName(i);
        std::string nLower = name;
        std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);

        if (nLower.find(".jpg") != std::string::npos ||
            nLower.find(".jpeg") != std::string::npos ||
            nLower.find(".png") != std::string::npos) {
            std::vector<uint8_t> testBytes;
            if (archive.readEntry(name, testBytes) && testBytes.size() > 2048) {
                outCoverBytes = std::move(testBytes);
                return true;
            }
        }
    }

    return false;
}

std::string EpubExtractor::extractContent(const std::string& path, const std::string& outputDir) {
    ArchiveHelper archive;
    if (!archive.open(path)) {
        return path;
    }

    std::string opfPath = findOpfPath(archive);
    if (opfPath.empty()) {
        return path;
    }

    std::vector<uint8_t> opfBytes;
    if (!archive.readEntry(opfPath, opfBytes)) {
        return path;
    }

    std::string opfContent(reinterpret_cast<char*>(opfBytes.data()), opfBytes.size());
    if (opfContent.find("bilingual-cover-page") != std::string::npos) {
        return path; // Já possui injeção de capa
    }

    std::vector<uint8_t> coverBytes;
    if (!extractCover(path, coverBytes) || coverBytes.empty()) {
        return path; // Sem imagem de capa para injetar
    }

    std::string targetEpub = outputDir + "/book.epub";
    ZipWriter writer;
    if (!writer.open(targetEpub)) {
        return path;
    }

    std::string parentDir;
    size_t lastSlash = opfPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        parentDir = opfPath.substr(0, lastSlash + 1);
    }

    const std::string coverImgName = "bilingual-cover-image.jpg";
    const std::string coverPageName = "bilingual-cover-page.xhtml";

    std::string modifiedOpf = opfContent;
    size_t manifestPos = modifiedOpf.find("<manifest>");
    if (manifestPos != std::string::npos) {
        size_t ins = manifestPos + 10;
        std::string items = "\n    <item id=\"bilingual-cover-image\" href=\"" + coverImgName + "\" media-type=\"image/jpeg\" />" +
                            "\n    <item id=\"bilingual-cover-page\" href=\"" + coverPageName + "\" media-type=\"application/xhtml+xml\" />";
        modifiedOpf.insert(ins, items);
    }

    size_t spinePos = modifiedOpf.find("<spine");
    if (spinePos != std::string::npos) {
        size_t closeTag = modifiedOpf.find('>', spinePos);
        if (closeTag != std::string::npos) {
            size_t ins = closeTag + 1;
            std::string itemref = "\n    <itemref idref=\"bilingual-cover-page\" />";
            modifiedOpf.insert(ins, itemref);
        }
    }

    int entryCount = archive.countEntries();
    for (int i = 0; i < entryCount; ++i) {
        std::string entryName = archive.getEntryName(i);
        if (entryName == opfPath) {
            writer.addEntry(entryName, modifiedOpf);
        } else {
            std::vector<uint8_t> data;
            if (archive.readEntry(entryName, data)) {
                writer.addEntry(entryName, data.data(), data.size());
            }
        }
    }

    writer.addEntry(parentDir + coverImgName, coverBytes.data(), coverBytes.size());

    std::string xhtml =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<!DOCTYPE html>\n"
        "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
        "<head>\n"
        "  <title>Cover</title>\n"
        "  <style type=\"text/css\">\n"
        "    body { margin: 0; padding: 0; text-align: center; background-color: #ffffff; }\n"
        "    img { max-width: 100%; max-height: 100%; height: auto; width: auto; margin: 0 auto; display: block; }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <div>\n"
        "    <img src=\"" + coverImgName + "\" alt=\"Cover\" />\n"
        "  </div>\n"
        "</body>\n"
        "</html>\n";
    writer.addEntry(parentDir + coverPageName, xhtml);

    writer.close();
    return targetEpub;
}

} // namespace Extractor
