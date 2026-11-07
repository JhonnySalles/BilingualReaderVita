#include "comicinfo_parser.h"
#include "archive_helper.h"
#include "../file_utils.h"

#define _UNIX
#include "../../libs/unrar/dll.hpp"

#include <algorithm>
#include <cstring>
#include <strings.h>
#include <cctype>
#include <cstdio>
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

std::string ComicInfoParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

void ComicInfoParser::splitString(const std::string& str, char delimiter, std::vector<std::string>& outList) {
    size_t start = 0;
    while (start < str.size()) {
        size_t end = str.find(delimiter, start);
        if (end == std::string::npos) end = str.size();
        std::string token = trim(str.substr(start, end - start));
        if (!token.empty()) {
            outList.push_back(token);
        }
        start = end + 1;
    }
}

static void buildChaptersFromImages(const std::vector<std::string>& images, std::vector<ChapterItem>& outChapters) {
    std::string lastFolder = "";
    for (size_t i = 0; i < images.size(); ++i) {
        const std::string& img = images[i];
        size_t slash = img.find_last_of("/\\");
        if (slash != std::string::npos) {
            std::string folder = img.substr(0, slash);
            size_t subSlash = folder.find_last_of("/\\");
            std::string chapterName = (subSlash != std::string::npos) ? folder.substr(subSlash + 1) : folder;
            if (chapterName != lastFolder && !chapterName.empty()) {
                lastFolder = chapterName;
                ChapterItem ch;
                ch.title = chapterName;
                ch.pageIndex = static_cast<int>(i);
                ch.isBookmark = false;
                outChapters.push_back(ch);
            }
        }
    }
}

bool ComicInfoParser::parseXmlData(const uint8_t* xmlData, size_t xmlSize, DetailMetadata& outDetail) {
    if (!xmlData || xmlSize == 0) return false;

    fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!ctx) return false;

    fz_xml* root = nullptr;
    fz_buffer* buf = nullptr;
    fz_try(ctx) {
        buf = fz_new_buffer_from_copied_data(ctx, xmlData, xmlSize);
        root = fz_parse_xml(ctx, buf, 0);
    }
    fz_always(ctx) {
        fz_drop_buffer(ctx, buf);
    }
    fz_catch(ctx) {
        root = nullptr;
    }

    if (!root) {
        fz_drop_context(ctx);
        return false;
    }

    auto getField = [&](const char* tag) -> std::string {
        fz_xml* n = findChildTag(root, tag);
        return n ? trim(getNodeTextRecursive(n)) : "";
    };

    std::string title = getField("Title");
    if (!title.empty()) outDetail.title = title;

    std::string series = getField("Series");
    if (!series.empty()) outDetail.series = series;

    outDetail.number = getField("Number");
    
    std::string volStr = getField("Volume");
    if (!volStr.empty()) {
        try { outDetail.volume = std::stoi(volStr); } catch (...) {}
    }

    std::string cntStr = getField("Count");
    if (!cntStr.empty()) {
        try { outDetail.count = std::stoi(cntStr); } catch (...) {}
    }

    outDetail.summary = getField("Summary");
    outDetail.notes = getField("Notes");
    outDetail.publisher = getField("Publisher");
    outDetail.imprint = getField("Imprint");
    outDetail.genre = getField("Genre");
    outDetail.language = getField("LanguageISO");
    outDetail.ageRating = getField("AgeRating");
    outDetail.mangaDirection = getField("Manga");
    outDetail.storyArc = getField("StoryArc");
    outDetail.seriesGroup = getField("SeriesGroup");
    outDetail.web = getField("Web");

    outDetail.writer = getField("Writer");
    outDetail.penciller = getField("Penciller");
    outDetail.inker = getField("Inker");
    outDetail.colorist = getField("Colorist");
    outDetail.letterer = getField("Letterer");
    outDetail.coverArtist = getField("CoverArtist");
    outDetail.editor = getField("Editor");
    outDetail.translator = getField("Translator");

    // Construção da string concatenada de autores/equipe criativa
    std::vector<std::string> authorList;
    if (!outDetail.writer.empty()) authorList.push_back(outDetail.writer + " (Roteiro)");
    if (!outDetail.penciller.empty()) authorList.push_back(outDetail.penciller + " (Desenho)");
    if (!outDetail.inker.empty() && outDetail.inker != outDetail.penciller) authorList.push_back(outDetail.inker + " (Arte-Final)");
    if (!outDetail.colorist.empty()) authorList.push_back(outDetail.colorist + " (Cores)");
    if (!outDetail.letterer.empty()) authorList.push_back(outDetail.letterer + " (Letras)");
    if (!outDetail.translator.empty()) authorList.push_back(outDetail.translator + " (Tradução)");

    if (!authorList.empty()) {
        std::string joined;
        for (size_t i = 0; i < authorList.size(); ++i) {
            if (i > 0) joined += ", ";
            joined += authorList[i];
        }
        outDetail.authors = joined;
    }

    // Montagem da data de lançamento
    std::string year = getField("Year");
    std::string month = getField("Month");
    std::string day = getField("Day");
    if (!year.empty()) {
        if (!month.empty() && !day.empty()) {
            char dateBuf[32];
            snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%s", std::atoi(day.c_str()), std::atoi(month.c_str()), year.c_str());
            outDetail.releaseDate = dateBuf;
        } else if (!month.empty()) {
            char dateBuf[32];
            snprintf(dateBuf, sizeof(dateBuf), "%02d/%s", std::atoi(month.c_str()), year.c_str());
            outDetail.releaseDate = dateBuf;
        } else {
            outDetail.releaseDate = year;
        }
    }

    // Divisão de Tags, Gêneros, Personagens, Equipes, Locais
    std::string genreRaw = getField("Genre");
    if (!genreRaw.empty()) {
        splitString(genreRaw, ',', outDetail.genres);
        if (outDetail.genres.empty()) splitString(genreRaw, ';', outDetail.genres);
    }

    std::string tagsRaw = getField("Tags");
    if (!tagsRaw.empty()) {
        splitString(tagsRaw, ',', outDetail.tags);
        if (outDetail.tags.empty()) splitString(tagsRaw, ';', outDetail.tags);
    }

    std::string charsRaw = getField("Characters");
    if (!charsRaw.empty()) {
        splitString(charsRaw, ',', outDetail.characters);
        if (outDetail.characters.empty()) splitString(charsRaw, ';', outDetail.characters);
    }

    std::string teamsRaw = getField("Teams");
    if (!teamsRaw.empty()) {
        splitString(teamsRaw, ',', outDetail.teams);
        if (outDetail.teams.empty()) splitString(teamsRaw, ';', outDetail.teams);
    }

    std::string locsRaw = getField("Locations");
    if (!locsRaw.empty()) {
        splitString(locsRaw, ',', outDetail.locations);
        if (outDetail.locations.empty()) splitString(locsRaw, ';', outDetail.locations);
    }

    // Leitura dos Bookmarks de Páginas: <Pages><Page Image="0" Bookmark="Capítulo 1"/></Pages>
    fz_xml* pagesNode = findChildTag(root, "Pages");
    if (pagesNode) {
        std::vector<fz_xml*> pageTags;
        collectAllTags(pagesNode, "Page", pageTags);
        for (fz_xml* p : pageTags) {
            char* bookmark = fz_xml_att(p, "Bookmark");
            char* imageIdx = fz_xml_att(p, "Image");
            if (bookmark && strlen(bookmark) > 0) {
                ChapterItem ch;
                ch.title = bookmark;
                ch.isBookmark = true;
                ch.pageIndex = imageIdx ? std::atoi(imageIdx) : 0;
                outDetail.chapters.push_back(ch);
            }
        }
    }

    fz_drop_xml(ctx, root);
    fz_drop_context(ctx);
    return true;
}

bool ComicInfoParser::parseZip(const std::string& filePath, DetailMetadata& outDetail) {
    ArchiveHelper archive;
    if (!archive.open(filePath)) return false;

    int totalEntries = archive.countEntries();
    std::vector<std::string> images;
    std::string comicInfoEntry = "";

    for (int i = 0; i < totalEntries; ++i) {
        std::string name = archive.getEntryName(i);
        if (FileUtils::isImageFile(name)) {
            images.push_back(name);
        } else {
            std::string nLower = name;
            std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);
            if (nLower == "comicinfo.xml" || (nLower.size() > 14 && nLower.substr(nLower.size() - 14) == "/comicinfo.xml")) {
                comicInfoEntry = name;
            }
        }
    }

    std::sort(images.begin(), images.end(), FileUtils::naturalSortCompare);

    if (!comicInfoEntry.empty()) {
        std::vector<uint8_t> xmlBytes;
        if (archive.readEntry(comicInfoEntry, xmlBytes)) {
            parseXmlData(xmlBytes.data(), xmlBytes.size(), outDetail);
        }
    }

    // Adiciona capítulos baseados na estrutura de subpastas caso não haja marcadores no ComicInfo
    if (outDetail.chapters.empty()) {
        buildChaptersFromImages(images, outDetail.chapters);
    }

    return true;
}

struct RarMemCallbackData {
    std::vector<uint8_t> buffer;
};

static int CALLBACK RarExtractCallback(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2) {
    if (msg == UCM_PROCESSDATA && UserData != 0) {
        RarMemCallbackData* data = reinterpret_cast<RarMemCallbackData*>(UserData);
        uint8_t* p = reinterpret_cast<uint8_t*>(P1);
        size_t size = static_cast<size_t>(P2);
        if (p && size > 0) {
            data->buffer.insert(data->buffer.end(), p, p + size);
        }
    }
    return 1;
}

bool ComicInfoParser::parseRar(const std::string& filePath, DetailMetadata& outDetail) {
    RAROpenArchiveDataEx arcData = {0};
    arcData.ArcName = const_cast<char*>(filePath.c_str());
    arcData.OpenMode = RAR_OM_EXTRACT;

    HANDLE hArc = RAROpenArchiveEx(&arcData);
    if (!hArc || arcData.OpenResult != ERAR_SUCCESS) return false;

    RarMemCallbackData memData;
    RARSetCallback(hArc, RarExtractCallback, reinterpret_cast<LPARAM>(&memData));

    std::vector<std::string> images;
    RARHeaderDataEx headerData = {0};

    while (RARReadHeaderEx(hArc, &headerData) == ERAR_SUCCESS) {
        std::string name = headerData.FileName;
        if (FileUtils::isImageFile(name)) {
            images.push_back(name);
            RARProcessFile(hArc, RAR_SKIP, NULL, NULL);
        } else {
            std::string nLower = name;
            std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);
            if (nLower == "comicinfo.xml" || (nLower.size() > 14 && nLower.substr(nLower.size() - 14) == "/comicinfo.xml")) {
                memData.buffer.clear();
                RARProcessFile(hArc, RAR_TEST, NULL, NULL);
            } else {
                RARProcessFile(hArc, RAR_SKIP, NULL, NULL);
            }
        }
    }
    RARCloseArchive(hArc);

    std::sort(images.begin(), images.end(), FileUtils::naturalSortCompare);

    if (!memData.buffer.empty()) {
        parseXmlData(memData.buffer.data(), memData.buffer.size(), outDetail);
    }

    if (outDetail.chapters.empty()) {
        buildChaptersFromImages(images, outDetail.chapters);
    }

    return true;
}

struct TarHeaderBlock {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
};

static size_t parseOctalTar(const char* str, size_t maxLen) {
    size_t val = 0;
    while (maxLen > 0 && *str == ' ') {
        str++;
        maxLen--;
    }
    while (maxLen > 0 && *str >= '0' && *str <= '7') {
        val = (val << 3) | (*str - '0');
        str++;
        maxLen--;
    }
    return val;
}

bool ComicInfoParser::parseTar(const std::string& filePath, DetailMetadata& outDetail) {
    FILE* f = fopen(filePath.c_str(), "rb");
    if (!f) return false;

    std::vector<std::string> images;
    std::vector<uint8_t> xmlBytes;

    TarHeaderBlock header;
    while (fread(&header, 1, 512, f) == 512) {
        if (header.name[0] == '\0') break;

        size_t fileSize = parseOctalTar(header.size, sizeof(header.size));
        long dataOffset = ftell(f);

        std::string entryName(header.name);
        if (header.prefix[0] != '\0') {
            entryName = std::string(header.prefix) + "/" + entryName;
        }

        if (header.typeflag == '0' || header.typeflag == '\0') {
            if (FileUtils::isImageFile(entryName)) {
                images.push_back(entryName);
            } else {
                std::string nLower = entryName;
                std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);
                if (nLower == "comicinfo.xml" || (nLower.size() > 14 && nLower.substr(nLower.size() - 14) == "/comicinfo.xml")) {
                    xmlBytes.resize(fileSize);
                    if (fread(xmlBytes.data(), 1, fileSize, f) == fileSize) {
                        // lido
                    }
                }
            }
        }

        size_t blocks = (fileSize + 511) / 512;
        fseek(f, dataOffset + (blocks * 512), SEEK_SET);
    }
    fclose(f);

    std::sort(images.begin(), images.end(), FileUtils::naturalSortCompare);

    if (!xmlBytes.empty()) {
        parseXmlData(xmlBytes.data(), xmlBytes.size(), outDetail);
    }

    if (outDetail.chapters.empty()) {
        buildChaptersFromImages(images, outDetail.chapters);
    }

    return true;
}

bool ComicInfoParser::parse(const std::string& filePath, DetailMetadata& outDetail) {
    outDetail.isManga = true;
    outDetail.isLoaded = false;

    std::string ext = FileUtils::getExtension(filePath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    bool ok = false;
    if (ext == "cbz" || ext == "zip") {
        ok = parseZip(filePath, outDetail);
    } else if (ext == "cbr" || ext == "rar") {
        ok = parseRar(filePath, outDetail);
    } else if (ext == "cbt" || ext == "tar") {
        ok = parseTar(filePath, outDetail);
    }

    if (ok) {
        outDetail.isLoaded = true;
        if (outDetail.title.empty()) {
            size_t slash = filePath.find_last_of("/\\");
            size_t dot = filePath.find_last_of('.');
            if (slash != std::string::npos && dot != std::string::npos && dot > slash) {
                outDetail.title = filePath.substr(slash + 1, dot - slash - 1);
            } else {
                outDetail.title = filePath;
            }
        }
    }

    return ok;
}

} // namespace Extractor

