#include "parse_mupdf.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

extern "C" {
#include <mupdf/fitz.h>
}

static vita2d_texture* createTextureFromPixmap(fz_context* ctx, fz_pixmap* pix) {
    if (!ctx || !pix) return nullptr;

    int renderW = fz_pixmap_width(ctx, pix);
    int renderH = fz_pixmap_height(ctx, pix);

    if (renderW <= 0 || renderH <= 0) return nullptr;

    vita2d_texture* tex = vita2d_create_empty_texture_format(renderW, renderH, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR);
    if (!tex) return nullptr;

    unsigned char* texData = reinterpret_cast<unsigned char*>(vita2d_texture_get_datap(tex));
    int texStride = vita2d_texture_get_stride(tex);
    unsigned char* pixData = fz_pixmap_samples(ctx, pix);
    int pixStride = fz_pixmap_stride(ctx, pix);
    int n = fz_pixmap_components(ctx, pix);

    for (int y = 0; y < renderH; y++) {
        unsigned char* srcRow = pixData + (y * pixStride);
        uint32_t* dstRow = reinterpret_cast<uint32_t*>(texData + (y * texStride));

        for (int x = 0; x < renderW; x++) {
            unsigned char r = srcRow[x * n + 0];
            unsigned char g = srcRow[x * n + 1];
            unsigned char b = srcRow[x * n + 2];
            unsigned char a = (n >= 4) ? srcRow[x * n + 3] : 255;
            dstRow[x] = RGBA8(r, g, b, a);
        }
    }

    return tex;
}

ParseMuPDF::ParseMuPDF() : m_ctx(nullptr), m_doc(nullptr) {}

ParseMuPDF::~ParseMuPDF() {
    close();
}

bool ParseMuPDF::open(const std::string& path) {
    close();
    m_filePath = path;

    m_ctx = fz_new_context(NULL, NULL, 12 * 1024 * 1024);
    if (!m_ctx) return false;

    fz_register_document_handlers(m_ctx);

    fz_try(m_ctx) {
        m_doc = fz_open_document(m_ctx, path.c_str());
    }
    fz_catch(m_ctx) {
        close();
        return false;
    }

    if (!m_doc) {
        close();
        return false;
    }

    int count = 0;
    fz_try(m_ctx) {
        count = fz_count_pages(m_ctx, m_doc);
    }
    fz_catch(m_ctx) {
        close();
        return false;
    }

    m_pageNames.clear();
    for (int i = 0; i < count; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Page %d", i + 1);
        m_pageNames.push_back(buf);
    }

    return count > 0;
}

void ParseMuPDF::close() {
    if (m_doc && m_ctx) {
        fz_drop_document(m_ctx, m_doc);
        m_doc = nullptr;
    }
    if (m_ctx) {
        fz_drop_context(m_ctx);
        m_ctx = nullptr;
    }
    m_pageNames.clear();
    m_filePath.clear();
}

const std::vector<std::string>& ParseMuPDF::getPages() const {
    return m_pageNames;
}

size_t ParseMuPDF::getPageCount() const {
    return m_pageNames.size();
}

bool ParseMuPDF::extractPage(size_t index, const std::string& outPath) {
    // MuPDF pode salvar página como imagem se necessário
    return false;
}

bool ParseMuPDF::extractCover(const std::string& outPath) {
    return false;
}

vita2d_texture* ParseMuPDF::loadPageTexture(size_t index) {
    if (!m_ctx || !m_doc || index >= m_pageNames.size()) return nullptr;

    fz_page* page = nullptr;
    fz_pixmap* pix = nullptr;
    vita2d_texture* tex = nullptr;

    fz_try(m_ctx) {
        page = fz_load_page(m_ctx, m_doc, static_cast<int>(index));
        if (page) {
            fz_rect bounds = fz_bound_page(m_ctx, page);
            float pageW = bounds.x1 - bounds.x0;
            float pageH = bounds.y1 - bounds.y0;

            if (pageW > 0.0f && pageH > 0.0f) {
                float maxDimW = 960.0f;
                float maxDimH = 544.0f;
                float scale = std::min(maxDimW / pageW, maxDimH / pageH);
                if (scale > 2.0f) scale = 2.0f;
                if (scale < 0.2f) scale = 0.2f;

                fz_matrix ctm = fz_scale(scale, scale);
                fz_irect ibounds = fz_round_rect(fz_transform_rect(bounds, ctm));

                fz_colorspace* cs = fz_device_rgb(m_ctx);
                pix = fz_new_pixmap_with_bbox(m_ctx, cs, ibounds, NULL, 1);
                fz_clear_pixmap(m_ctx, pix);

                fz_device* dev = fz_new_draw_device(m_ctx, ctm, pix);
                fz_run_page(m_ctx, page, dev, fz_identity, NULL);
                fz_close_device(m_ctx, dev);
                fz_drop_device(m_ctx, dev);

                tex = createTextureFromPixmap(m_ctx, pix);
            }
            fz_drop_page(m_ctx, page);
            page = nullptr;
        }
    }
    fz_catch(m_ctx) {
        if (page) fz_drop_page(m_ctx, page);
        if (pix) fz_drop_pixmap(m_ctx, pix);
        return nullptr;
    }

    if (pix) {
        fz_drop_pixmap(m_ctx, pix);
    }

    return tex;
}

vita2d_texture* ParseMuPDF::loadPageThumbnail(size_t index, int maxW, int maxH) {
    if (!m_ctx || !m_doc || index >= m_pageNames.size()) return nullptr;

    fz_page* page = nullptr;
    fz_pixmap* pix = nullptr;
    vita2d_texture* tex = nullptr;

    fz_try(m_ctx) {
        page = fz_load_page(m_ctx, m_doc, static_cast<int>(index));
        if (page) {
            fz_rect bounds = fz_bound_page(m_ctx, page);
            float pageW = bounds.x1 - bounds.x0;
            float pageH = bounds.y1 - bounds.y0;

            if (pageW > 0.0f && pageH > 0.0f) {
                float scale = std::min(static_cast<float>(maxW) / pageW, static_cast<float>(maxH) / pageH);
                if (scale > 1.0f) scale = 1.0f;
                if (scale < 0.01f) scale = 0.01f;

                fz_matrix ctm = fz_scale(scale, scale);
                fz_irect ibounds = fz_round_rect(fz_transform_rect(bounds, ctm));

                fz_colorspace* cs = fz_device_rgb(m_ctx);
                pix = fz_new_pixmap_with_bbox(m_ctx, cs, ibounds, NULL, 1);
                fz_clear_pixmap(m_ctx, pix);

                fz_device* dev = fz_new_draw_device(m_ctx, ctm, pix);
                fz_run_page(m_ctx, page, dev, fz_identity, NULL);
                fz_close_device(m_ctx, dev);
                fz_drop_device(m_ctx, dev);

                tex = createTextureFromPixmap(m_ctx, pix);
            }
            fz_drop_page(m_ctx, page);
            page = nullptr;
        }
    }
    fz_catch(m_ctx) {
        if (page) fz_drop_page(m_ctx, page);
        if (pix) fz_drop_pixmap(m_ctx, pix);
        return nullptr;
    }

    if (pix) {
        fz_drop_pixmap(m_ctx, pix);
    }

    return tex;
}

bool ParseMuPDF::isDoublePage(size_t index) {
    if (!m_ctx || !m_doc || index >= m_pageNames.size()) return false;

    bool isDouble = false;
    fz_page* page = nullptr;

    fz_try(m_ctx) {
        page = fz_load_page(m_ctx, m_doc, static_cast<int>(index));
        if (page) {
            fz_rect bounds = fz_bound_page(m_ctx, page);
            float w = bounds.x1 - bounds.x0;
            float h = bounds.y1 - bounds.y0;
            if (w > 0.0f && h > 0.0f) {
                // Se a largura for significativamente maior que a altura ou ratio > 1.15
                isDouble = (w / h) > 1.15f;
            }
            fz_drop_page(m_ctx, page);
            page = nullptr;
        }
    }
    fz_catch(m_ctx) {
        if (page) fz_drop_page(m_ctx, page);
        return false;
    }

    return isDouble;
}
