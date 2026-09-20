#pragma once
#include <string>
#include <vector>
#include <vita2d.h>

struct fz_context;
struct fz_document;

class ReaderCBZ {
public:
    ReaderCBZ();
    ~ReaderCBZ();

    bool loadFile(const std::string& path);
    void render(vita2d_pgf* font, bool fullscreen = false);
    void nextPage();
    void prevPage();
    void close();

    void setRotated(bool rotated);
    bool getRotated() const { return isRotated; }

    int getCurrentPage() const { return currentPage; }
    int getTotalPages() const { return totalPages; }
    const std::string& getFilename() const { return filename; }

    void resetZoom();
    void addZoom(float factor, float focusX = 480.0f, float focusY = 272.0f);
    void addPan(float dx, float dy);
    float getZoom() const { return zoomScale; }

private:
    std::string filename;
    std::string filePath;
    std::string typeString;
    int currentPage;
    int totalPages;
    bool isRotated;

    fz_context* ctx;
    fz_document* doc;
    vita2d_texture* currentTexture;
    float zoomScale;
    float panX;
    float panY;

    class ParseRar* rarParser;

    void loadPageTexture(int pageIndex);
    void freeTexture();
};
