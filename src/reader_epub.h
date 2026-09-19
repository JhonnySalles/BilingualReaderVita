#pragma once
#include <string>
#include <vector>
#include <vita2d.h>

struct fz_context;
struct fz_document;

class ReaderEPUB {
public:
    ReaderEPUB();
    ~ReaderEPUB();

    bool loadFile(const std::string& path, vita2d_pgf* font);
    void render(vita2d_pgf* font);
    void nextPage();
    void prevPage();
    void close();

    void increaseFontSize();
    void decreaseFontSize();

    int getCurrentPage() const { return currentPage; }
    int getTotalPages() const { return totalPages; }
    const std::string& getFilename() const { return filename; }

private:
    std::string filename;
    std::string filePath;
    int currentPage;
    int totalPages;
    float currentFontSize;

    fz_context* ctx;
    fz_document* doc;
    vita2d_texture* pageTexture;

    void relayout();
    void renderCurrentPageToTexture();
    void freeTexture();
};
