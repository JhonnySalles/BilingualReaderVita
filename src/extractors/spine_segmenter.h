#pragma once

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "image_size_utils.h"

namespace Extractor {

struct ChapterItem {
    std::string title;
    std::string contentHtml;
    std::string fileName = "chapter.xhtml";
    std::string anchorId;
    std::vector<std::string> cssLinks;
};

struct GalleryItem {
    std::string imageFileName;
    std::string title = "Illustration";
};

enum class BlockType {
    BOUNDARY,
    HTML_TEXT,
    IMAGE
};

struct Block {
    BlockType type;
    std::string title;
    std::string textOrHtml;
    std::string src;
    std::string alt;
    std::string anchorId;
    bool forceBreak = false;
    int textLength = -1;
    ImageSizeUtils::ImageRole role = ImageSizeUtils::ImageRole::INLINE;
    std::vector<uint8_t> imageBytes;
    std::pair<int, int> dims = {-1, -1};

    static Block makeBoundary(const std::string& title = "", const std::string& anchorId = "", bool forceBreak = false) {
        Block b;
        b.type = BlockType::BOUNDARY;
        b.title = title;
        b.anchorId = anchorId;
        b.forceBreak = forceBreak;
        return b;
    }

    static Block makeHtmlText(const std::string& html, int textLength = -1, const std::string& anchorId = "") {
        Block b;
        b.type = BlockType::HTML_TEXT;
        b.textOrHtml = html;
        b.textLength = textLength;
        b.anchorId = anchorId;
        return b;
    }

    static Block makeImage(const std::string& src, const std::string& alt = "",
                           ImageSizeUtils::ImageRole role = ImageSizeUtils::ImageRole::INLINE,
                           const std::vector<uint8_t>& bytes = {},
                           const std::pair<int, int>& dims = {-1, -1}) {
        Block b;
        b.type = BlockType::IMAGE;
        b.src = src;
        b.alt = alt;
        b.role = role;
        b.imageBytes = bytes;
        b.dims = dims;
        return b;
    }
};

class SpineSegmenter {
public:
    static constexpr int DEFAULT_MIN_TEXT_CHARS = 200;

    static std::string escapeXml(const std::string& text);
    static std::string toValidXhtml(const std::string& bodyHtml);

    static std::vector<ChapterItem> segment(
        const std::vector<Block>& blocks,
        const std::string& bookTitle,
        int minTextChars = DEFAULT_MIN_TEXT_CHARS,
        int screenW = ImageSizeUtils::DEFAULT_SCREEN_WIDTH,
        int screenH = ImageSizeUtils::DEFAULT_SCREEN_HEIGHT,
        int minSide = ImageSizeUtils::DEFAULT_MIN_SIDE,
        const std::vector<std::string>& defaultCssLinks = {}
    );
};

} // namespace Extractor

