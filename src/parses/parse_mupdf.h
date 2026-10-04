#ifndef PARSE_MUPDF_H
#define PARSE_MUPDF_H

#include "parse.h"
#include <string>
#include <vector>
#include <vita2d.h>

struct fz_context;
struct fz_document;

class ParseMuPDF : public Parse {
public:
    ParseMuPDF();
    ~ParseMuPDF() override;

    bool open(const std::string& path) override;
    void close() override;

    const std::vector<std::string>& getPages() const override;
    size_t getPageCount() const override;

    bool extractPage(size_t index, const std::string& outPath) override;
    bool extractCover(const std::string& outPath) override;

    vita2d_texture* loadPageTexture(size_t index) override;
    vita2d_texture* loadPageThumbnail(size_t index, int maxW = 90, int maxH = 130) override;
    bool isDoublePage(size_t index) override;

private:
    std::string m_filePath;
    fz_context* m_ctx;
    fz_document* m_doc;
    std::vector<std::string> m_pageNames;
};

#endif // PARSE_MUPDF_H
