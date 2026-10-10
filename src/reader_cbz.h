#pragma once
#include <string>
#include <vector>
#include <vita2d.h>
#include "config_manager.h"

struct fz_context;
struct fz_document;

class ReaderCBZ {
public:
    ReaderCBZ();
    ~ReaderCBZ();

    bool loadFile(const std::string& path);
    void render(vita2d_pgf* font, bool fullscreen = false);
    void update(float dt);
    void nextPage();
    void prevPage();
    void close();

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
    void toggleLinkedPage();
    bool isShowingLinkedPage() const { return showLinkedPage; }

    void resetZoom();
    void addZoom(float factor, float focusX = 480.0f, float focusY = 272.0f);
    void addPan(float dx, float dy);
    float getZoom() const { return zoomScale; }

private:
    enum class AnimType {
        None,
        Scroll,
        Page
    };

    std::string filename;
    std::string filePath;
    std::string typeString;
    int currentPage;
    int totalPages;
    bool isRotated;
    bool showLinkedPage;
    ReaderScrollMode scrollMode;
    PageTransition pageTransition;
    float scrollY;

    fz_context* ctx;
    fz_document* doc;
    vita2d_texture* currentTexture;
    vita2d_texture* currentTextureRight;
    vita2d_texture* nextScrollTexture;
    float zoomScale;
    float panX;
    float panY;

    class ParseRar* rarParser;

    bool animating;
    AnimType animType;
    float animT;
    float animScrollTarget;
    float animScrollApplied;
    int animDirection;
    bool animHorizontal;
    int animToPage;
    vita2d_texture* animToTexture;
    vita2d_texture* animToTextureRight;

    void loadPageTexture(int pageIndex);
    vita2d_texture* createPageTexture(int pageIndex);
    void loadPagePair(int pageIndex, vita2d_texture*& left, vita2d_texture*& right);
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
    void drawPagePair(vita2d_texture* left, vita2d_texture* right, float ox, float oy, float alpha) const;
    static float easeSmoothstep(float t);
};
