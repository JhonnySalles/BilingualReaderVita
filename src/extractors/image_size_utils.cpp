#include "image_size_utils.h"
#include <algorithm>
#include <cstring>

std::pair<int, int> ImageSizeUtils::readDimensions(const uint8_t* bytes, size_t size) {
    if (!bytes || size < 8) return {-1, -1};

    // 1. PNG: 89 50 4E 47 0D 0A 1A 0A
    if (bytes[0] == 0x89 && bytes[1] == 0x50 &&
        bytes[2] == 0x4E && bytes[3] == 0x47 &&
        bytes[4] == 0x0D && bytes[5] == 0x0A &&
        bytes[6] == 0x1A && bytes[7] == 0x0A) {
        if (size >= 24) {
            int w = (bytes[16] << 24) | (bytes[17] << 16) | (bytes[18] << 8) | bytes[19];
            int h = (bytes[20] << 24) | (bytes[21] << 16) | (bytes[22] << 8) | bytes[23];
            if (w > 0 && h > 0) return {w, h};
        }
        return {-1, -1};
    }

    // 2. GIF: GIF87a or GIF89a
    if (bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F') {
        if (size >= 10) {
            int w = bytes[6] | (bytes[7] << 8);
            int h = bytes[8] | (bytes[9] << 8);
            if (w > 0 && h > 0) return {w, h};
        }
        return {-1, -1};
    }

    // 3. WEBP: RIFF....WEBP
    if (size >= 16 &&
        bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
        bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B' && bytes[11] == 'P') {
        if (size >= 30 && memcmp(bytes + 12, "VP8 ", 4) == 0) {
            int w = (bytes[26] | (bytes[27] << 8)) & 0x3FFF;
            int h = (bytes[28] | (bytes[29] << 8)) & 0x3FFF;
            if (w > 0 && h > 0) return {w, h};
        } else if (size >= 25 && memcmp(bytes + 12, "VP8L", 4) == 0 && bytes[20] == 0x2F) {
            uint32_t b21 = bytes[21], b22 = bytes[22], b23 = bytes[23], b24 = bytes[24];
            int w = 1 + (b21 | ((b22 & 0x3F) << 8));
            int h = 1 + (((b22 >> 6) | (b23 << 2) | ((b24 & 0x0F) << 10)));
            if (w > 0 && h > 0) return {w, h};
        } else if (size >= 30 && memcmp(bytes + 12, "VP8X", 4) == 0) {
            int w = 1 + (bytes[24] | (bytes[25] << 8) | (bytes[26] << 16));
            int h = 1 + (bytes[27] | (bytes[28] << 8) | (bytes[29] << 16));
            if (w > 0 && h > 0) return {w, h};
        }
        return {-1, -1};
    }

    // 4. JPEG: FF D8
    if (bytes[0] == 0xFF && bytes[1] == 0xD8) {
        size_t offset = 2;
        while (offset < size - 8) {
            if (bytes[offset] != 0xFF) {
                offset++;
                continue;
            }
            while (offset < size && bytes[offset] == 0xFF) {
                offset++;
            }
            if (offset >= size) break;
            uint8_t marker = bytes[offset++];

            // SOF markers: Baseline (C0), Extended Sequential (C1), Progressive (C2), Lossless (C3),
            // Differential (C5-C7), Extended (C9-CB), etc. Excluding DHT (C4) and DAC (CC).
            if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2 || marker == 0xC3 ||
                marker == 0xC5 || marker == 0xC6 || marker == 0xC7 || marker == 0xC9 ||
                marker == 0xCA || marker == 0xCB || marker == 0xCD || marker == 0xCE || marker == 0xCF) {
                if (offset + 7 <= size) {
                    int h = (bytes[offset + 3] << 8) | bytes[offset + 4];
                    int w = (bytes[offset + 5] << 8) | bytes[offset + 6];
                    if (w > 0 && h > 0) return {w, h};
                }
                break;
            } else if (marker == 0xDA || marker == 0xD9) {
                // SOS or EOI
                break;
            } else if ((marker < 0xD0 || marker > 0xD7) && marker != 0x01) {
                if (offset + 2 <= size) {
                    size_t length = (bytes[offset] << 8) | bytes[offset + 1];
                    offset += length;
                } else {
                    break;
                }
            }
        }
        return {-1, -1};
    }

    return {-1, -1};
}

std::pair<int, int> ImageSizeUtils::readDimensions(const std::vector<uint8_t>& bytes) {
    return readDimensions(bytes.data(), bytes.size());
}

ImageSizeUtils::ImageRole ImageSizeUtils::classify(
    const std::pair<int, int>& dims,
    int screenW,
    int screenH,
    int minSide
) {
    if (dims.first <= 0 || dims.second <= 0) return ImageRole::INLINE;
    int w = dims.first;
    int h = dims.second;
    int minDim = std::min(w, h);
    bool isLargeEnough = (w > screenW / 3 || h > screenH / 3);
    bool hasSufficientThickness = (minDim >= minSide);

    if (isLargeEnough && hasSufficientThickness) {
        return ImageRole::FULL_PAGE;
    }
    return ImageRole::INLINE;
}

ImageSizeUtils::ImageRole ImageSizeUtils::classify(
    const uint8_t* data,
    size_t size,
    int screenW,
    int screenH,
    int minSide
) {
    auto dims = readDimensions(data, size);
    return classify(dims, screenW, screenH, minSide);
}

ImageSizeUtils::ImageRole ImageSizeUtils::classify(
    const std::vector<uint8_t>& bytes,
    int screenW,
    int screenH,
    int minSide
) {
    return classify(bytes.data(), bytes.size(), screenW, screenH, minSide);
}

