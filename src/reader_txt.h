#pragma once
#include <string>
#include <vector>
#include <vita2d.h>

class ReaderTXT {
public:
    ReaderTXT();
    ~ReaderTXT();

    bool loadFile(const std::string& path, vita2d_pgf* font);
    void render(vita2d_pgf* font, bool fullscreen = false);
    void nextPage();
    void prevPage();
    void close();

    void setRotated(bool rotated, vita2d_pgf* font);
    bool getRotated() const { return isRotated; }

    int getCurrentPage() const { return currentPage; }
    int getTotalPages() const { return totalPages; }
    const std::string& getFilename() const { return filename; }

private:
    std::string filename;
    std::string rawContent;
    std::vector<std::string> pages;
    int currentPage;
    int totalPages;
    bool isRotated;

    void paginateText(const std::string& fullText, vita2d_pgf* font);
};
