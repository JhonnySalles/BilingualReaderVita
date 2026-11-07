#include "spine_segmenter.h"
#include <sstream>

namespace Extractor {

std::string SpineSegmenter::escapeXml(const std::string& text) {
    std::string out;
    out.reserve(text.size() * 1.1);
    for (char c : text) {
        switch (c) {
            case '&':  out += "&amp;"; break;
            case '<':  out += "&lt;"; break;
            case '>':  out += "&gt;"; break;
            case '\"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default:   out += c; break;
        }
    }
    return out;
}

std::string SpineSegmenter::toValidXhtml(const std::string& bodyHtml) {
    // Basic cleanup: ensure empty body returns empty
    if (bodyHtml.empty()) return "";
    return bodyHtml;
}

std::vector<ChapterItem> SpineSegmenter::segment(
    const std::vector<Block>& blocks,
    const std::string& bookTitle,
    int minTextChars,
    int screenW,
    int screenH,
    int minSide,
    const std::vector<std::string>& defaultCssLinks
) {
    std::vector<ChapterItem> chapters;
    std::string currentHtml;
    int currentTextChars = 0;
    std::string currentTitle;
    std::string currentAnchorId;
    int chapterIndex = 0;

    auto flushCurrentChapter = [&]() {
        if (!currentHtml.empty()) {
            std::string chapTitle = currentTitle.empty() ? bookTitle : currentTitle;
            std::string chapFile = "chap_" + std::to_string(chapterIndex++) + ".xhtml";

            ChapterItem item;
            item.title = chapTitle;
            item.contentHtml = currentHtml;
            item.fileName = chapFile;
            item.anchorId = currentAnchorId;
            item.cssLinks = defaultCssLinks;
            chapters.push_back(std::move(item));

            currentHtml.clear();
            currentTextChars = 0;
            currentTitle.clear();
            currentAnchorId.clear();
        }
    };

    for (const auto& block : blocks) {
        switch (block.type) {
            case BlockType::BOUNDARY: {
                if (block.forceBreak || currentTextChars >= minTextChars) {
                    flushCurrentChapter();
                    currentTitle = block.title;
                    if (!block.anchorId.empty()) {
                        currentAnchorId = block.anchorId;
                    }
                } else {
                    if (currentTitle.empty() && !block.title.empty()) {
                        currentTitle = block.title;
                    }
                    if (currentAnchorId.empty() && !block.anchorId.empty()) {
                        currentAnchorId = block.anchorId;
                    }
                }
                break;
            }
            case BlockType::HTML_TEXT: {
                if (!block.textOrHtml.empty()) {
                    currentHtml += block.textOrHtml;
                    currentHtml += "\n";
                    int len = block.textLength >= 0 ? block.textLength : (int)block.textOrHtml.size();
                    currentTextChars += len;
                    if (currentAnchorId.empty() && !block.anchorId.empty()) {
                        currentAnchorId = block.anchorId;
                    }
                }
                break;
            }
            case BlockType::IMAGE: {
                ImageSizeUtils::ImageRole role = block.role;
                if (!block.imageBytes.empty()) {
                    role = ImageSizeUtils::classify(block.imageBytes, screenW, screenH, minSide);
                } else if (block.dims.first > 0 && block.dims.second > 0) {
                    role = ImageSizeUtils::classify(block.dims, screenW, screenH, minSide);
                }

                if (role == ImageSizeUtils::ImageRole::FULL_PAGE) {
                    flushCurrentChapter();

                    std::string fullPageHtml = "<div class=\"br-full-page\"><img src=\"" +
                        escapeXml(block.src) + "\" alt=\"" + escapeXml(block.alt) + "\" /></div>";
                    std::string imgFile = "img_page_" + std::to_string(chapterIndex++) + ".xhtml";
                    std::string imgTitle = block.alt.empty() ? "Illustration" : block.alt;

                    ChapterItem item;
                    item.title = imgTitle;
                    item.contentHtml = fullPageHtml;
                    item.fileName = imgFile;
                    item.anchorId = currentAnchorId;
                    item.cssLinks = defaultCssLinks;
                    chapters.push_back(std::move(item));

                    currentAnchorId.clear();
                } else {
                    currentHtml += "<div class=\"br-inline-wrapper\" style=\"text-align: center; margin: 0.8em 0;\">";
                    currentHtml += "<img class=\"br-inline\" src=\"" + escapeXml(block.src) + "\" alt=\"" + escapeXml(block.alt) + "\" /></div>\n";
                }
                break;
            }
        }
    }

    flushCurrentChapter();
    return chapters;
}

} // namespace Extractor

