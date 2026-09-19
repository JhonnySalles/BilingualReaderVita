#ifndef PARSE_TAR_H
#define PARSE_TAR_H

#include "parse.h"
#include <cstdio>
#include <map>

class ParseTar : public Parse {
public:
    ParseTar();
    virtual ~ParseTar();

    bool open(const std::string& path) override;
    void close() override;

    const std::vector<std::string>& getPages() const override;
    size_t getPageCount() const override;

    bool extractPage(size_t index, const std::string& outPath) override;
    bool extractCover(const std::string& outPath) override;
    vita2d_texture* loadPageTexture(size_t index) override;

private:
    struct TarEntry {
        std::string name;
        long dataOffset;
        size_t size;
    };

    std::string m_filePath;
    FILE* m_file;
    std::vector<std::string> m_pageNames;
    std::map<std::string, TarEntry> m_entries;
};

#endif // PARSE_TAR_H
