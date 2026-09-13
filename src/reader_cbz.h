#pragma once
#include <string>
#include <vector>
#include <vita2d.h>

class ReaderCBZ {
public:
    ReaderCBZ();
    ~ReaderCBZ();

    bool loadFile(const std::string& path);
    void render(vita2d_pgf* font);
    void nextPage();
    void prevPage();
    void close();

    int getCurrentPage() const { return currentPage; }
    int getTotalPages() const { return totalPages; }
    const std::string& getFilename() const { return filename; }

private:
    std::string filename;
    std::string filePath;
    std::vector<std::string> imageEntries;
    int currentPage;
    int totalPages;

    vita2d_texture* currentTexture;

    void loadPageTexture(int pageIndex);
    void freeTexture();
};
