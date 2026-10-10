#pragma once
#include <string>
#include <vector>
#include <vita2d.h>
#include "config_manager.h"

class ReaderTXT {
public:
    ReaderTXT();
    ~ReaderTXT();

    bool loadFile(const std::string& path, vita2d_pgf* font);
    void render(vita2d_pgf* font, bool fullscreen = false);
    void update(float dt);
    void nextPage();
    void prevPage();
    void close();

    void setRotated(bool rotated, vita2d_pgf* font);
    bool getRotated() const { return isRotated; }

    void setScrollMode(ReaderScrollMode mode);
    ReaderScrollMode getScrollMode() const { return scrollMode; }
    void setPageTransition(PageTransition mode);
    PageTransition getPageTransition() const { return pageTransition; }
    void addScroll(float dy);
    void cancelScrollAnimation();
    bool isAnimating() const { return animating; }

    int getCurrentPage() const { return currentPage; }
    int getTotalPages() const { return totalPages; }
    const std::string& getFilename() const { return filename; }

private:
    enum class AnimType {
        None,
        Scroll,
        Page
    };

    std::string filename;
    std::string rawContent;
    std::vector<std::string> pages;
    std::vector<std::string> lines;
    int currentPage;
    int totalPages;
    bool isRotated;
    ReaderScrollMode scrollMode;
    PageTransition pageTransition;
    float scrollY;
    static constexpr float kLineHeight = 24.0f;

    bool animating;
    AnimType animType;
    float animT;
    float animScrollTarget;
    float animScrollApplied;
    int animDirection;
    bool animHorizontal;
    int animToPage;

    void rebuildLayout(vita2d_pgf* font);
    void paginateFromLines();
    float contentHeight() const;
    float viewportHeight(bool fullscreen) const;
    void clampScroll(bool fullscreen);
    void cancelAnimation(bool commitPage);
    void startScrollAnimation(float distance);
    void startPageAnimation(int direction);
    void finishPageAnimation();
    void drawTextPage(vita2d_pgf* font, int pageIndex, float ox, float oy, float alpha, float yStart, bool fullscreen) const;
    static float easeSmoothstep(float t);
};
