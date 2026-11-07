#pragma once

#include <cstdint>
#include <vector>
#include <utility>

class ImageSizeUtils {
public:
    enum class ImageRole {
        FULL_PAGE,
        INLINE
    };

    static constexpr int DEFAULT_SCREEN_WIDTH = 960;  // PS Vita screen width
    static constexpr int DEFAULT_SCREEN_HEIGHT = 544; // PS Vita screen height
    static constexpr int DEFAULT_MIN_SIDE = 64;

    /**
     * Reads the dimensions (width, height) of PNG, JPEG, GIF, or WEBP image bytes.
     * Lightweight header parsing without decoding the whole bitmap.
     * Returns std::pair<int, int>(width, height), or {-1, -1} if failed.
     */
    static std::pair<int, int> readDimensions(const uint8_t* data, size_t size);
    static std::pair<int, int> readDimensions(const std::vector<uint8_t>& bytes);

    /**
     * Classifies an image into FULL_PAGE or INLINE based on its dimensions.
     * Rule: FULL_PAGE if (w > screenW / 3 || h > screenH / 3) && min(w, h) >= minSide.
     * Otherwise INLINE.
     */
    static ImageRole classify(
        const std::pair<int, int>& dims,
        int screenW = DEFAULT_SCREEN_WIDTH,
        int screenH = DEFAULT_SCREEN_HEIGHT,
        int minSide = DEFAULT_MIN_SIDE
    );

    static ImageRole classify(
        const uint8_t* data,
        size_t size,
        int screenW = DEFAULT_SCREEN_WIDTH,
        int screenH = DEFAULT_SCREEN_HEIGHT,
        int minSide = DEFAULT_MIN_SIDE
    );

    static ImageRole classify(
        const std::vector<uint8_t>& bytes,
        int screenW = DEFAULT_SCREEN_WIDTH,
        int screenH = DEFAULT_SCREEN_HEIGHT,
        int minSide = DEFAULT_MIN_SIDE
    );
};

