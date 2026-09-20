#include "reader_epub.h"
#include "ui_components.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

extern "C" {
#include <mupdf/fitz.h>
}

ReaderEPUB::ReaderEPUB() 
    : currentPage(0), 
      totalPages(0), 
      currentFontSize(11.0f), 
      isRotated(false),
      ctx(nullptr), 
      doc(nullptr), 
      pageTexture(nullptr) {
}

ReaderEPUB::~ReaderEPUB() {
    close();
}

void ReaderEPUB::freeTexture() {
    if (pageTexture) {
        vita2d_free_texture(pageTexture);
        pageTexture = nullptr;
    }
}

void ReaderEPUB::close() {
    freeTexture();

    if (doc && ctx) {
        fz_drop_document(ctx, doc);
        doc = nullptr;
    }

    if (ctx) {
        fz_drop_context(ctx);
        ctx = nullptr;
    }

    currentPage = 0;
    totalPages = 0;
    filename = "";
    filePath = "";
}

bool ReaderEPUB::loadFile(const std::string& path, vita2d_pgf* font) {
    close();

    filePath = path;
    size_t lastSlash = path.find_last_of("/\\");
    filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

    // Inicializa o contexto MuPDF com limite de cache estrito para a memória do PS Vita (16MB)
    ctx = fz_new_context(NULL, NULL, 16 * 1024 * 1024);
    if (!ctx) return false;

    fz_register_document_handlers(ctx);
    fz_set_user_css(ctx, "body { color: #e2e8f0; background: transparent; } p, div, span, h1, h2, h3, h4, h5, h6, li, a { color: #e2e8f0; }");

    fz_try(ctx) {
        doc = fz_open_document(ctx, path.c_str());
    }
    fz_catch(ctx) {
        close();
        return false;
    }

    if (!doc) {
        close();
        return false;
    }

    relayout();
    currentPage = 0;
    renderCurrentPageToTexture();

    return totalPages > 0;
}

void ReaderEPUB::relayout() {
    if (!doc || !ctx) return;

    if (fz_is_document_reflowable(ctx, doc)) {
        // Dimensões úteis dependendo da rotação
        float layoutW = isRotated ? 480.0f : 900.0f;
        float layoutH = isRotated ? 880.0f : 460.0f;
        fz_layout_document(ctx, doc, layoutW, layoutH, currentFontSize);
    }

    totalPages = fz_count_pages(ctx, doc);
    if (totalPages < 1) totalPages = 1;
    if (currentPage >= totalPages) currentPage = totalPages - 1;
}

void ReaderEPUB::setRotated(bool rotated) {
    if (isRotated != rotated) {
        isRotated = rotated;
        int savedPage = currentPage;
        relayout();
        currentPage = std::max(0, std::min(savedPage, totalPages - 1));
        renderCurrentPageToTexture();
    }
}

void ReaderEPUB::increaseFontSize() {
    if (!doc || !ctx) return;
    if (fz_is_document_reflowable(ctx, doc) && currentFontSize < 24.0f) {
        currentFontSize += 1.0f;
        int savedPage = currentPage;
        relayout();
        currentPage = std::max(0, std::min(savedPage, totalPages - 1));
        renderCurrentPageToTexture();
    }
}

void ReaderEPUB::decreaseFontSize() {
    if (!doc || !ctx) return;
    if (fz_is_document_reflowable(ctx, doc) && currentFontSize > 7.0f) {
        currentFontSize -= 1.0f;
        int savedPage = currentPage;
        relayout();
        currentPage = std::max(0, std::min(savedPage, totalPages - 1));
        renderCurrentPageToTexture();
    }
}

void ReaderEPUB::renderCurrentPageToTexture() {
    freeTexture();
    if (!doc || !ctx || totalPages <= 0) return;

    fz_page* page = nullptr;
    fz_try(ctx) {
        page = fz_load_page(ctx, doc, currentPage);
    }
    fz_catch(ctx) {
        return;
    }

    if (!page) return;

    fz_rect bounds = fz_bound_page(ctx, page);
    float pageW = bounds.x1 - bounds.x0;
    float pageH = bounds.y1 - bounds.y0;
    if (pageW <= 0.0f || pageH <= 0.0f) {
        fz_drop_page(ctx, page);
        return;
    }

    // Calcula a escala para caber no formato desejado
    float targetW = isRotated ? 500.0f : 920.0f;
    float targetH = isRotated ? 900.0f : 450.0f;
    float scale = std::min(targetW / pageW, targetH / pageH);
    if (scale > 2.0f) scale = 2.0f;

    fz_matrix ctm = fz_scale(scale, scale);
    fz_irect ibounds = fz_round_rect(fz_transform_rect(bounds, ctm));
    int renderW = ibounds.x1 - ibounds.x0;
    int renderH = ibounds.y1 - ibounds.y0;

    if (renderW <= 0 || renderH <= 0) {
        fz_drop_page(ctx, page);
        return;
    }

    fz_colorspace* cs = fz_device_rgb(ctx);
    fz_pixmap* pix = fz_new_pixmap_with_bbox(ctx, cs, ibounds, NULL, 1);
    // Limpa o fundo com total transparência
    fz_clear_pixmap(ctx, pix);

    fz_device* dev = fz_new_draw_device(ctx, ctm, pix);
    fz_run_page(ctx, page, dev, fz_identity, NULL);
    fz_close_device(ctx, dev);
    fz_drop_device(ctx, dev);
    fz_drop_page(ctx, page);

    // Cria textura vita2d compatível
    pageTexture = vita2d_create_empty_texture_format(renderW, renderH, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR);
    if (pageTexture) {
        unsigned char* texData = reinterpret_cast<unsigned char*>(vita2d_texture_get_datap(pageTexture));
        int texStride = vita2d_texture_get_stride(pageTexture);
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
    }

    fz_drop_pixmap(ctx, pix);
}

void ReaderEPUB::nextPage() {
    if (currentPage < totalPages - 1) {
        currentPage++;
        renderCurrentPageToTexture();
    }
}

void ReaderEPUB::prevPage() {
    if (currentPage > 0) {
        currentPage--;
        renderCurrentPageToTexture();
    }
}

void ReaderEPUB::render(vita2d_pgf* font, bool fullscreen) {
    // Fundo elegante para livro
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(24, 26, 36, 255));

    if (!fullscreen) {
        char pageInfo[64];
        snprintf(pageInfo, sizeof(pageInfo), "%d / %d (%.0fpt)", currentPage + 1, totalPages > 0 ? totalPages : 1, currentFontSize);
        UIComponents::drawReaderTopBar(filename, pageInfo, isRotated, UITheme::BadgeEPUB);
    }

    // Desenha a página renderizada centralizada na tela
    if (pageTexture) {
        float texW = static_cast<float>(vita2d_texture_get_width(pageTexture));
        float texH = static_cast<float>(vita2d_texture_get_height(pageTexture));

        if (isRotated) {
            // Em rotação 90 graus (retrato na tela paisagem do Vita)
            // O centro da tela do PS Vita é (480, 272)
            float rad = 1.57079632679f; // 90 graus em radianos
            vita2d_draw_texture_rotate(pageTexture, 480.0f, 272.0f, rad);
        } else {
            float drawX = (960.0f - texW) / 2.0f;
            float topOffset = fullscreen ? 0.0f : 48.0f;
            float bottomOffset = fullscreen ? 0.0f : 40.0f;
            float availableH = 544.0f - topOffset - bottomOffset;
            float drawY = topOffset + ((availableH - texH) / 2.0f);

            vita2d_draw_texture(pageTexture, drawX, drawY);
        }
    }

    if (!fullscreen) {
        UIComponents::drawReaderProgressBar(currentPage, totalPages, false);
        UIComponents::drawFooter("D-Pad: Paginas | /\\/[]: Fonte | O: Voltar", true);
    }
}
