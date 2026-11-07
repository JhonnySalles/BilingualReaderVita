#include "palmdoc_decompressor.h"
#include <cstdio>
#include <cstring>

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

void PalmDocDecompressor::decompressChunk(const uint8_t* data, size_t size, std::vector<uint8_t>& out) {
    size_t i = 0;
    uint8_t chunkBuf[8192];
    size_t chunkLen = 0;

    while (i < size) {
        uint8_t c = data[i++];
        if (c == 0) {
            if (chunkLen < sizeof(chunkBuf)) chunkBuf[chunkLen++] = 0;
        } else if (c >= 1 && c <= 8) {
            if (i + c <= size && chunkLen + c <= sizeof(chunkBuf)) {
                memcpy(chunkBuf + chunkLen, data + i, c);
                chunkLen += c;
                i += c;
            } else {
                break;
            }
        } else if (c >= 9 && c <= 0x7F) {
            if (chunkLen < sizeof(chunkBuf)) chunkBuf[chunkLen++] = c;
        } else if (c >= 0x80 && c <= 0xBF) {
            if (i < size) {
                uint8_t c2 = data[i++];
                int m = ((c & 0x3F) << 8) | c2;
                int offset = m >> 3;
                int length = (m & 0x07) + 3;

                int start = static_cast<int>(chunkLen) - offset;
                if (start >= 0) {
                    for (int idx = 0; idx < length; ++idx) {
                        if (chunkLen < sizeof(chunkBuf)) {
                            chunkBuf[chunkLen++] = chunkBuf[start++];
                        }
                    }
                }
            }
        } else { // 0xC0..0xFF
            if (chunkLen + 2 <= sizeof(chunkBuf)) {
                chunkBuf[chunkLen++] = 32; // ' '
                chunkBuf[chunkLen++] = c ^ 0x80;
            }
        }
    }

    if (chunkLen > 0) {
        out.insert(out.end(), chunkBuf, chunkBuf + chunkLen);
    }
}

bool PalmDocDecompressor::decompress(const std::string& filePath, std::vector<uint8_t>& outBytes) {
    FILE* f = fopen(filePath.c_str(), "rb");
    if (!f) return false;

    uint8_t header[78];
    if (fread(header, 1, 78, f) != 78) {
        fclose(f);
        return false;
    }

    uint16_t numRecords = readU16BE(header + 76);
    if (numRecords <= 1) {
        fclose(f);
        return false;
    }

    std::vector<uint32_t> recordOffsets(numRecords);
    for (uint16_t i = 0; i < numRecords; ++i) {
        uint8_t entry[8];
        if (fread(entry, 1, 8, f) != 8) {
            fclose(f);
            return false;
        }
        recordOffsets[i] = readU32BE(entry);
    }

    // Record 0: PalmDOC header
    fseek(f, recordOffsets[0], SEEK_SET);
    uint8_t rec0[16];
    if (fread(rec0, 1, 16, f) < 12) {
        fclose(f);
        return false;
    }

    uint16_t compression = readU16BE(rec0);
    uint32_t textLength = readU32BE(rec0 + 4);
    uint16_t numTextRecs = readU16BE(rec0 + 8);

    fseek(f, 0, SEEK_END);
    long fileLength = ftell(f);

    outBytes.reserve(textLength > 0 ? textLength : 65536);

    for (uint16_t i = 1; i <= numTextRecs && i < numRecords; ++i) {
        uint32_t startOffset = recordOffsets[i];
        uint32_t endOffset = (i + 1 < numRecords) ? recordOffsets[i + 1] : static_cast<uint32_t>(fileLength);
        if (endOffset <= startOffset) continue;

        size_t compressedSize = endOffset - startOffset;
        std::vector<uint8_t> compressedData(compressedSize);
        fseek(f, startOffset, SEEK_SET);
        if (fread(compressedData.data(), 1, compressedSize, f) != compressedSize) {
            break;
        }

        if (compression == 2) {
            decompressChunk(compressedData.data(), compressedData.size(), outBytes);
        } else {
            outBytes.insert(outBytes.end(), compressedData.begin(), compressedData.end());
        }
    }

    fclose(f);
    return !outBytes.empty();
}

} // namespace Extractor

