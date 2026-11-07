#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>

namespace Extractor {

class ZipWriter {
public:
    ZipWriter();
    ~ZipWriter();

    bool open(const std::string& outputPath);
    bool addEntry(const std::string& entryName, const uint8_t* data, size_t size, bool compress = false);
    bool addEntry(const std::string& entryName, const std::string& text, bool compress = false);
    bool addEntry(const std::string& entryName, const std::vector<uint8_t>& data, bool compress = false);
    bool close();

private:
    struct EntryInfo {
        std::string name;
        uint32_t crc = 0;
        uint32_t uncompressedSize = 0;
        uint32_t compressedSize = 0;
        uint32_t localHeaderOffset = 0;
        uint16_t compressionMethod = 0; // 0 = STORE, 8 = DEFLATE
    };

    FILE* m_file = nullptr;
    std::vector<EntryInfo> m_entries;
    uint32_t m_currentOffset = 0;
};

} // namespace Extractor

