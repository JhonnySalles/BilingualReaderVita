#pragma once
#include <string>
#include <vector>
#include <vita2d.h>

class ReaderEPUB {
public:
    ReaderEPUB();
    ~ReaderEPUB();

    bool loadFile(const std::string& path, vita2d_pgf* font);
    void render(vita2d_pgf* font);
    void nextPage();
    void prevPage();
    void close();

    int getCurrentPage() const { return currentPage; }
    int getTotalPages() const { return totalPages; }
    const std::string& getFilename() const { return filename; }

private:
    std::string filename;
    std::vector<std::string> pages;
    int currentPage;
    int totalPages;

    void parseEpubStructure(const std::string& path, vita2d_pgf* font);
};
