#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Extractor {

class PalmDocDecompressor {
public:
    static bool decompress(const std::string& filePath, std::vector<uint8_t>& outBytes);
    static void decompressChunk(const uint8_t* data, size_t size, std::vector<uint8_t>& out);
};

} // namespace Extractor

