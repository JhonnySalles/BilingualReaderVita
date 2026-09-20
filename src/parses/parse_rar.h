#ifndef PARSE_RAR_H
#define PARSE_RAR_H

#include "parse.h"
#include <string>
#include <vector>
#include <vita2d.h>

class ParseRar : public Parse {
public:
    ParseRar();
    ~ParseRar() override;

    bool open(const std::string& path) override;
    void close() override;

    const std::vector<std::string>& getPages() const override;
    size_t getPageCount() const override;

    bool extractPage(size_t index, const std::string& outPath) override;
    bool extractCover(const std::string& outPath) override;
    vita2d_texture* loadPageTexture(size_t index) override;

private:
    std::string m_filePath;
    std::string m_cacheDir;
    std::vector<std::string> m_pageNames; // Caminhos absolutos para as imagens extraídas

    bool extractAllWithProgress();
};

#endif // PARSE_RAR_H
