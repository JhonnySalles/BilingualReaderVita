#include "reader_txt.h"
#include "ui_components.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <algorithm>

ReaderTXT::ReaderTXT()
    : currentPage(0),
      totalPages(0),
      isRotated(false),
      scrollMode(ReaderScrollMode::PagedHorizontal),
      pageTransition(PageTransition::Slide),
      scrollY(0.0f),
      animating(false),
      animType(AnimType::None),
      animT(0.0f),
      animScrollTarget(0.0f),
      animScrollApplied(0.0f),
      animDirection(1),
      animHorizontal(true),
      animToPage(0) {}

ReaderTXT::~ReaderTXT() {
    close();
}

void ReaderTXT::close() {
    cancelAnimation(false);
    pages.clear();
    lines.clear();
    rawContent.clear();
    currentPage = 0;
    totalPages = 0;
    scrollY = 0.0f;
    filename = "";
}

bool ReaderTXT::loadFile(const std::string& path, vita2d_pgf* font) {
    close();

    size_t lastSlash = path.find_last_of("/\\");
    filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    rawContent = buffer.str();
    file.close();

    rebuildLayout(font);
    return !pages.empty() || !lines.empty();
}

void ReaderTXT::setRotated(bool rotated, vita2d_pgf* font) {
    if (isRotated != rotated) {
        cancelAnimation(false);
        isRotated = rotated;
        int savedPage = currentPage;
        float savedScroll = scrollY;
        rebuildLayout(font);
        currentPage = std::max(0, std::min(savedPage, totalPages - 1));
        scrollY = savedScroll;
        clampScroll(false);
    }
}

void ReaderTXT::setScrollMode(ReaderScrollMode mode) {
    if (scrollMode == mode) return;
    cancelAnimation(false);
    scrollMode = mode;
    if (scrollMode == ReaderScrollMode::InfiniteScroll) {
        scrollY = static_cast<float>(currentPage) * viewportHeight(false);
        clampScroll(false);
    } else if (!pages.empty()) {
        float pageH = viewportHeight(false);
        if (pageH > 0.0f) {
            currentPage = std::max(0, std::min(static_cast<int>(scrollY / pageH), totalPages - 1));
        }
        scrollY = 0.0f;
    }
}

void ReaderTXT::setPageTransition(PageTransition mode) {
    pageTransition = mode;
}

float ReaderTXT::easeSmoothstep(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

void ReaderTXT::cancelScrollAnimation() {
    if (animating && animType == AnimType::Scroll) {
        animating = false;
        animType = AnimType::None;
        animT = 0.0f;
        animScrollTarget = 0.0f;
        animScrollApplied = 0.0f;
    }
}

void ReaderTXT::cancelAnimation(bool commitPage) {
    if (!animating) return;
    if (animType == AnimType::Page && commitPage) {
        finishPageAnimation();
        return;
    }
    animating = false;
    animType = AnimType::None;
    animT = 0.0f;
    animScrollTarget = 0.0f;
    animScrollApplied = 0.0f;
}

void ReaderTXT::startScrollAnimation(float distance) {
    cancelAnimation(false);
    animating = true;
    animType = AnimType::Scroll;
    animT = 0.0f;
    animScrollTarget = distance;
    animScrollApplied = 0.0f;
}

void ReaderTXT::startPageAnimation(int direction) {
    int target = currentPage + direction;
    if (target < 0 || target >= totalPages) return;
    if (pages.empty()) return;

    cancelAnimation(false);
    animating = true;
    animType = AnimType::Page;
    animT = 0.0f;
    animDirection = direction;
    animToPage = target;
    animHorizontal = (scrollMode == ReaderScrollMode::PagedHorizontal);
}

void ReaderTXT::finishPageAnimation() {
    currentPage = animToPage;
    animating = false;
    animType = AnimType::None;
    animT = 0.0f;
}

void ReaderTXT::update(float dt) {
    if (!animating) return;
    if (dt > 0.05f) dt = 0.05f;
    if (dt < 0.0f) dt = 0.0f;

    animT += dt / kReaderTransitionDuration;
    if (animT >= 1.0f) animT = 1.0f;
    float eased = easeSmoothstep(animT);

    if (animType == AnimType::Scroll) {
        float desired = animScrollTarget * eased;
        float delta = desired - animScrollApplied;
        animScrollApplied = desired;
        if (delta != 0.0f) {
            addScroll(delta);
        }
        if (animT >= 1.0f) {
            animating = false;
            animType = AnimType::None;
            animScrollTarget = 0.0f;
            animScrollApplied = 0.0f;
        }
    } else if (animType == AnimType::Page) {
        if (animT >= 1.0f) {
            finishPageAnimation();
        }
    }
}

void ReaderTXT::drawTextPage(vita2d_pgf* font, int pageIndex, float ox, float oy, float alpha, float yStart, bool fullscreen) const {
    if (!font || pageIndex < 0 || pageIndex >= static_cast<int>(pages.size())) return;
    float screenW = isRotated ? 544.0f : 960.0f;
    float screenH = isRotated ? 960.0f : 544.0f;
    float top = fullscreen ? 0.0f : 48.0f;
    float bottom = fullscreen ? screenH : (screenH - 40.0f);
    vita2d_draw_rectangle(ox, top + oy, screenW, bottom - top, UIComponents::applyAlpha(UITheme::Background, alpha));

    int xPos = isRotated ? 24 : 48;
    std::istringstream stream(pages[pageIndex]);
    std::string line;
    float yPos = yStart + oy;
    unsigned int color = UIComponents::applyAlpha(UITheme::TextPrimary, alpha);
    while (std::getline(stream, line)) {
        vita2d_pgf_draw_text(font, static_cast<float>(xPos) + ox, yPos, color, 0.95f, line.c_str());
        yPos += kLineHeight;
    }
}

float ReaderTXT::contentHeight() const {
    return static_cast<float>(lines.size()) * kLineHeight;
}

float ReaderTXT::viewportHeight(bool fullscreen) const {
    float screenH = isRotated ? 960.0f : 544.0f;
    float top = fullscreen ? 36.0f : 84.0f;
    float bottom = fullscreen ? 8.0f : 40.0f;
    return std::max(kLineHeight, screenH - top - bottom);
}

void ReaderTXT::clampScroll(bool fullscreen) {
    float maxScroll = std::max(0.0f, contentHeight() - viewportHeight(fullscreen));
    if (scrollY < 0.0f) scrollY = 0.0f;
    if (scrollY > maxScroll) scrollY = maxScroll;
}

void ReaderTXT::addScroll(float dy) {
    if (scrollMode != ReaderScrollMode::InfiniteScroll) return;
    scrollY += dy;
    clampScroll(false);
}

void ReaderTXT::rebuildLayout(vita2d_pgf* font) {
    (void)font;
    pages.clear();
    lines.clear();
    if (rawContent.empty()) {
        totalPages = 0;
        currentPage = 0;
        scrollY = 0.0f;
        return;
    }

    const int maxCharsPerLine = isRotated ? 38 : 70;

    std::istringstream stream(rawContent);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            lines.push_back("");
            continue;
        }
        while (line.length() > static_cast<size_t>(maxCharsPerLine)) {
            size_t splitPos = line.rfind(' ', maxCharsPerLine);
            if (splitPos == std::string::npos) {
                splitPos = static_cast<size_t>(maxCharsPerLine);
            }
            lines.push_back(line.substr(0, splitPos));
            line = line.substr(splitPos + (line[splitPos] == ' ' ? 1 : 0));
        }
        lines.push_back(line);
    }

    paginateFromLines();
    clampScroll(false);
}

void ReaderTXT::paginateFromLines() {
    pages.clear();
    const int maxLinesPerPage = isRotated ? 32 : 16;

    std::string currentPageText;
    int lineCounter = 0;
    for (const auto& l : lines) {
        currentPageText += l + "\n";
        lineCounter++;
        if (lineCounter >= maxLinesPerPage) {
            pages.push_back(currentPageText);
            currentPageText.clear();
            lineCounter = 0;
        }
    }
    if (!currentPageText.empty()) {
        pages.push_back(currentPageText);
    }

    totalPages = static_cast<int>(pages.size());
    if (currentPage >= totalPages) currentPage = std::max(0, totalPages - 1);
}

void ReaderTXT::nextPage() {
    if (animating) return;
    if (scrollMode == ReaderScrollMode::InfiniteScroll) {
        startScrollAnimation(viewportHeight(false));
        return;
    }
    if (currentPage < totalPages - 1) {
        startPageAnimation(1);
    }
}

void ReaderTXT::prevPage() {
    if (animating) return;
    if (scrollMode == ReaderScrollMode::InfiniteScroll) {
        startScrollAnimation(-viewportHeight(false));
        return;
    }
    if (currentPage > 0) {
        startPageAnimation(-1);
    }
}

void ReaderTXT::render(vita2d_pgf* font, bool fullscreen) {
    float screenW = isRotated ? 544.0f : 960.0f;
    float screenH = isRotated ? 960.0f : 544.0f;
    vita2d_draw_rectangle(0, 0, screenW, screenH, UITheme::Background);

    clampScroll(fullscreen);

    int progressCurrent = currentPage;
    int progressTotal = totalPages > 0 ? totalPages : 1;
    if (scrollMode == ReaderScrollMode::InfiniteScroll) {
        float totalH = std::max(contentHeight(), 1.0f);
        progressCurrent = static_cast<int>((scrollY / totalH) * static_cast<float>(std::max(1, static_cast<int>(lines.size()))));
        progressTotal = std::max(1, static_cast<int>(lines.size()));
    }

    if (!fullscreen) {
        char pageInfo[64];
        if (scrollMode == ReaderScrollMode::InfiniteScroll) {
            float pct = contentHeight() > 0.0f ? (scrollY / std::max(contentHeight(), 1.0f)) * 100.0f : 0.0f;
            snprintf(pageInfo, sizeof(pageInfo), "%.0f%%", pct);
        } else {
            snprintf(pageInfo, sizeof(pageInfo), "Pag %d / %d", currentPage + 1, totalPages > 0 ? totalPages : 1);
        }
        UIComponents::drawReaderTopBar(filename, pageInfo, isRotated, false, UITheme::BadgeTXT);
    }

    if (font) {
        int xPos = isRotated ? 24 : 48;
        float yStart = fullscreen ? 36.0f : 84.0f;

        if (animating && animType == AnimType::Page) {
            float eased = easeSmoothstep(animT);
            float axis = animHorizontal ? screenW : screenH;
            float fromOx = 0.0f, fromOy = 0.0f, toOx = 0.0f, toOy = 0.0f;
            float fromA = 1.0f, toA = 1.0f;

            if (pageTransition == PageTransition::Fade) {
                fromA = 1.0f - eased;
                toA = eased;
            } else if (pageTransition == PageTransition::Stack) {
                if (animDirection > 0) {
                    if (animHorizontal) toOx = axis * (1.0f - eased);
                    else toOy = axis * (1.0f - eased);
                } else {
                    if (animHorizontal) fromOx = axis * eased;
                    else fromOy = axis * eased;
                }
            } else {
                float dir = static_cast<float>(animDirection);
                if (animHorizontal) {
                    fromOx = -dir * axis * eased;
                    toOx = dir * axis * (1.0f - eased);
                } else {
                    fromOy = -dir * axis * eased;
                    toOy = dir * axis * (1.0f - eased);
                }
            }

            if (pageTransition == PageTransition::Stack && animDirection < 0) {
                drawTextPage(font, animToPage, toOx, toOy, toA, yStart, fullscreen);
                drawTextPage(font, currentPage, fromOx, fromOy, fromA, yStart, fullscreen);
            } else {
                drawTextPage(font, currentPage, fromOx, fromOy, fromA, yStart, fullscreen);
                drawTextPage(font, animToPage, toOx, toOy, toA, yStart, fullscreen);
            }
        } else if (scrollMode == ReaderScrollMode::InfiniteScroll) {
            float viewH = viewportHeight(fullscreen);
            int firstLine = static_cast<int>(scrollY / kLineHeight);
            float yOffset = -(scrollY - static_cast<float>(firstLine) * kLineHeight);
            float yPos = yStart + yOffset;
            for (int i = firstLine; i < static_cast<int>(lines.size()); i++) {
                if (yPos > yStart + viewH + kLineHeight) break;
                if (yPos >= yStart - kLineHeight) {
                    vita2d_pgf_draw_text(font, static_cast<float>(xPos), yPos, UITheme::TextPrimary, 0.95f, lines[i].c_str());
                }
                yPos += kLineHeight;
            }
        } else if (!pages.empty() && currentPage < static_cast<int>(pages.size())) {
            std::istringstream stream(pages[currentPage]);
            std::string line;
            float yPos = yStart;
            while (std::getline(stream, line)) {
                vita2d_pgf_draw_text(font, static_cast<float>(xPos), yPos, UITheme::TextPrimary, 0.95f, line.c_str());
                yPos += kLineHeight;
            }
        }
    }

    if (!fullscreen) {
        UIComponents::drawReaderProgressBar(progressCurrent, progressTotal, false);
        const char* hint = (scrollMode == ReaderScrollMode::InfiniteScroll)
            ? "D-Pad: Rolar | O: Voltar"
            : "D-Pad: Paginas | O: Voltar";
        UIComponents::drawFooter(hint, true);
    }
}
