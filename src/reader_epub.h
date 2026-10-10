#pragma once
#include <string>
#include <vector>
#include <vita2d.h>
#include "config_manager.h"

struct fz_context;
struct fz_document;

class ReaderEPUB {
public:
    ReaderEPUB();
    ~ReaderEPUB();

    bool loadFile(const std::string& path, vita2d_pgf* font);
    void render(vita2d_pgf* font, bool fullscreen = false);
    void update(float dt);
    void nextPage();
    void prevPage();
    void close();

    void increaseFontSize();
    void decreaseFontSize();

    void resetZoom();
    void addZoom(float factor, float focusX, float focusY);
    void addPan(float dx, float dy);

    void setRotated(bool rotated);
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
    std::string filePath;
    int currentPage;
    int totalPages;
    float currentFontSize;
    bool isRotated;
    ReaderScrollMode scrollMode;
    PageTransition pageTransition;
    float scrollY;
    float zoomScale;
    float panX;
    float panY;

    fz_context* ctx;
    fz_document* doc;
    vita2d_texture* pageTexture;
    vita2d_texture* nextScrollTexture;

    bool animating;
    AnimType animType;
    float animT;
    float animScrollTarget;
    float animScrollApplied;
    int animDirection;
    bool animHorizontal;
    int animToPage;
    vita2d_texture* animToTexture;

    void relayout();
    void renderCurrentPageToTexture();
    vita2d_texture* createPageTexture(int pageIndex);
    void freeTexture();
    void freeNextScrollTexture();
    void freeAnimTextures();
    void ensureNextScrollTexture();
    void syncScrollTextures();
    float fittedPageHeight(vita2d_texture* tex, float screenW) const;
    void clampPan();
    void cancelAnimation(bool commitPage);
    void startScrollAnimation(float distance);
    void startPageAnimation(int direction);
    void finishPageAnimation();
    void drawPageTexture(vita2d_texture* tex, float ox, float oy, float alpha) const;
    static float easeSmoothstep(float t);
};
