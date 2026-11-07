#include "mobi_extractor.h"
#include "palmdoc_decompressor.h"
#include "spine_segmenter.h"
#include "epub_packager.h"
#include <cstdio>
#include <cstring>
#include <map>

namespace Extractor {

static uint16_t readU16BE(const uint8_t* p) {
    return (static_cast<uint16_t>(p[0]) << 8) | p[1];
}

static uint32_t readU32BE(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8)  |
           p[3];
}

struct MobiInfo {
    std::string title;
    std::string author;
    std::string description;
    std::string publisher;
    std::string genre;
    std::string date;
    uint32_t firstImageIndex = 0;
    int32_t coverRecordIndex = -1;
    bool isAzw4Pdf = false;
    std::vector<uint32_t> recordOffsets;
};

static bool parseMobiRecords(FILE* f, MobiInfo& outInfo) {
    fseek(f, 0, SEEK_SET);
    uint8_t header[78];
    if (fread(header, 1, 78, f) != 78) return false;

    uint16_t numRecords = readU16BE(header + 76);
    if (numRecords <= 1) return false;

    outInfo.recordOffsets.resize(numRecords);
    for (uint16_t i = 0; i < numRecords; ++i) {
        uint8_t entry[8];
        if (fread(entry, 1, 8, f) != 8) return false;
        outInfo.recordOffsets[i] = readU32BE(entry);
    }

    // Read full Record 0
    uint32_t rec0Size = (numRecords > 1 && outInfo.recordOffsets[1] > outInfo.recordOffsets[0]) ?
                        (outInfo.recordOffsets[1] - outInfo.recordOffsets[0]) : 4096;
    if (rec0Size > 131072) rec0Size = 131072; // cap to 128KB max for record 0
    std::vector<uint8_t> rec0(rec0Size);

    fseek(f, outInfo.recordOffsets[0], SEEK_SET);
    size_t rec0Read = fread(rec0.data(), 1, rec0Size, f);
    if (rec0Read < 132) return false;

    // Check "MOBI" magic at offset 16 of record 0
    if (memcmp(rec0.data() + 16, "MOBI", 4) == 0) {
        uint32_t fullNameOffset = readU32BE(rec0.data() + 84);
        uint32_t fullNameLength = readU32BE(rec0.data() + 88);

        if (fullNameLength > 0 && fullNameLength < 1024) {
            if (fullNameOffset + fullNameLength <= rec0Read) {
                outInfo.title = std::string(reinterpret_cast<char*>(rec0.data() + fullNameOffset), fullNameLength);
            } else {
                uint32_t absNameOffset = outInfo.recordOffsets[0] + fullNameOffset;
                fseek(f, absNameOffset, SEEK_SET);
                std::vector<char> nameBuf(fullNameLength + 1, 0);
                if (fread(nameBuf.data(), 1, fullNameLength, f) == fullNameLength) {
                    outInfo.title = nameBuf.data();
                }
            }
        }

        if (rec0Read >= 112) {
            outInfo.firstImageIndex = readU32BE(rec0.data() + 108);
        }

        // Dynamic search for EXTH header inside Record 0 (matching Android MobiParser)
        size_t exthPos = std::string::npos;
        for (size_t i = 16; i + 12 <= rec0Read; ++i) {
            if (rec0[i] == 'E' && rec0[i+1] == 'X' && rec0[i+2] == 'T' && rec0[i+3] == 'H') {
                exthPos = i;
                break;
            }
        }

        if (exthPos != std::string::npos) {
            uint32_t count = readU32BE(rec0.data() + exthPos + 8);
            size_t curr = exthPos + 12;

            for (uint32_t i = 0; i < count && curr + 8 <= rec0Read; ++i) {
                uint32_t recType = readU32BE(rec0.data() + curr);
                uint32_t recLen = readU32BE(rec0.data() + curr + 4);
                if (recLen < 8 || curr + recLen > rec0Read) break;

                uint32_t dataLen = recLen - 8;
                const uint8_t* data = rec0.data() + curr + 8;

                if (recType == 100) {
                    outInfo.author = std::string(reinterpret_cast<const char*>(data), dataLen);
                } else if (recType == 101) {
                    outInfo.publisher = std::string(reinterpret_cast<const char*>(data), dataLen);
                } else if (recType == 103) {
                    outInfo.description = std::string(reinterpret_cast<const char*>(data), dataLen);
                } else if (recType == 105) {
                    outInfo.genre = std::string(reinterpret_cast<const char*>(data), dataLen);
                } else if (recType == 106) {
                    outInfo.date = std::string(reinterpret_cast<const char*>(data), dataLen);
                } else if (recType == 201 && dataLen == 4) {
                    uint32_t coverOffset = readU32BE(data);
                    if (outInfo.firstImageIndex + coverOffset < numRecords) {
                        outInfo.coverRecordIndex = outInfo.firstImageIndex + coverOffset;
                    }
                } else if (recType == 202 && dataLen == 4 && outInfo.coverRecordIndex == 0) {
                    uint32_t thumbOffset = readU32BE(data);
                    if (outInfo.firstImageIndex + thumbOffset < numRecords) {
                        outInfo.coverRecordIndex = outInfo.firstImageIndex + thumbOffset;
                    }
                } else if (recType == 503 && dataLen > 0) {
                    outInfo.title = std::string(reinterpret_cast<const char*>(data), dataLen);
                }

                curr += recLen;
            }
        }

        // Fallback: If no explicit EXTH cover tag, try firstImageIndex as cover
        if (outInfo.coverRecordIndex == 0 && outInfo.firstImageIndex > 0 && outInfo.firstImageIndex < numRecords) {
            outInfo.coverRecordIndex = outInfo.firstImageIndex;
        }
    }

    return true;
}

bool MobiExtractor::extractMetadata(const std::string& path, BookMetadata& outMeta) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;

    MobiInfo info;
    bool ok = parseMobiRecords(f, info);
    fclose(f);
    if (!ok) return false;

    outMeta.filePath = path;
    outMeta.title = info.title;
    outMeta.author = info.author;
    outMeta.annotation = info.description;
    outMeta.publisher = info.publisher;
    outMeta.genre = info.genre;
    outMeta.releaseDate = info.date;

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

bool MobiExtractor::extractCover(const std::string& path, std::vector<uint8_t>& outCoverBytes) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;

    MobiInfo info;
    bool ok = parseMobiRecords(f, info);
    if (!ok) {
        fclose(f);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long fileLength = ftell(f);

    auto readRecordBytes = [&](uint32_t recIdx, std::vector<uint8_t>& out) -> bool {
        if (recIdx >= info.recordOffsets.size()) return false;
        uint32_t start = info.recordOffsets[recIdx];
        uint32_t end = (recIdx + 1 < info.recordOffsets.size()) ? info.recordOffsets[recIdx + 1] : static_cast<uint32_t>(fileLength);
        if (end <= start) return false;
        size_t sz = end - start;
        out.resize(sz);
        fseek(f, start, SEEK_SET);
        return fread(out.data(), 1, sz, f) == sz;
    };

    // If explicit coverRecordIndex found from EXTH 201
    if (info.coverRecordIndex >= 0 && static_cast<size_t>(info.coverRecordIndex) < info.recordOffsets.size()) {
        if (readRecordBytes(info.coverRecordIndex, outCoverBytes)) {
            fclose(f);
            return true;
        }
    }

    // Fallback: check first image record starting from firstImageIndex
    if (info.firstImageIndex > 0 && info.firstImageIndex < info.recordOffsets.size()) {
        for (size_t idx = info.firstImageIndex; idx < info.recordOffsets.size() && idx < info.firstImageIndex + 5; ++idx) {
            std::vector<uint8_t> rec;
            if (readRecordBytes(idx, rec) && rec.size() > 4) {
                // Check JPEG or PNG
                if ((rec[0] == 0xFF && rec[1] == 0xD8) || (rec[0] == 0x89 && rec[1] == 0x50)) {
                    outCoverBytes = std::move(rec);
                    fclose(f);
                    return true;
                }
            }
        }
    }

    fclose(f);
    return false;
}

std::string MobiExtractor::extractContent(const std::string& path, const std::string& cacheDir) {
    std::vector<uint8_t> rawText;
    if (!PalmDocDecompressor::decompress(path, rawText)) {
        return "";
    }

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return "";

    MobiInfo info;
    parseMobiRecords(f, info);

    fseek(f, 0, SEEK_END);
    long fileLength = ftell(f);

    std::map<std::string, std::vector<uint8_t>> resourceFiles;
    if (info.firstImageIndex > 0 && info.firstImageIndex < info.recordOffsets.size()) {
        int imgNum = 1;
        for (size_t idx = info.firstImageIndex; idx < info.recordOffsets.size(); ++idx) {
            uint32_t start = info.recordOffsets[idx];
            uint32_t end = (idx + 1 < info.recordOffsets.size()) ? info.recordOffsets[idx + 1] : static_cast<uint32_t>(fileLength);
            if (end <= start) continue;

            size_t sz = end - start;
            std::vector<uint8_t> rec(sz);
            fseek(f, start, SEEK_SET);
            if (fread(rec.data(), 1, sz, f) == sz && sz > 4) {
                if (rec[0] == 0xFF && rec[1] == 0xD8) {
                    char nameBuf[64];
                    snprintf(nameBuf, sizeof(nameBuf), "images/image_%04d.jpg", imgNum++);
                    resourceFiles[nameBuf] = std::move(rec);
                } else if (rec[0] == 0x89 && rec[1] == 0x50) {
                    char nameBuf[64];
                    snprintf(nameBuf, sizeof(nameBuf), "images/image_%04d.png", imgNum++);
                    resourceFiles[nameBuf] = std::move(rec);
                }
            }
        }
    }
    fclose(f);

    BookMetadata meta;
    extractMetadata(path, meta);

    std::vector<uint8_t> coverBytes;
    extractCover(path, coverBytes);

    // Convert raw text into blocks
    std::string htmlContent(reinterpret_cast<const char*>(rawText.data()), rawText.size());
    std::vector<Block> blocks;
    blocks.push_back(Block::makeHtmlText(htmlContent, static_cast<int>(htmlContent.size())));

    std::vector<ChapterItem> chapters = SpineSegmenter::segment(blocks, meta.title);

    std::string outputEpubPath = cacheDir + "/book.epub";
    if (EpubPackager::createEpub(outputEpubPath, meta.title, meta.author, coverBytes, "cover.jpg", {}, chapters, resourceFiles)) {
        return outputEpubPath;
    }

    return "";
}

} // namespace Extractor

