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
        // Dimensões úteis do leitor na tela do Vita (960x544, deixando margens para leitura confortável)
        float layoutW = 900.0f;
        float layoutH = 460.0f;
        fz_layout_document(ctx, doc, layoutW, layoutH, currentFontSize);
    }

    totalPages = fz_count_pages(ctx, doc);
    if (totalPages < 1) totalPages = 1;
    if (currentPage >= totalPages) currentPage = totalPages - 1;
}

void ReaderEPUB::increaseFontSize() {
    if (!doc || !ctx) return;
    if (fz_is_document_reflowable(ctx, doc) && currentFontSize < 24.0f) {
        currentFontSize += 1.0f;
        relayout();
        renderCurrentPageToTexture();
    }
}

void ReaderEPUB::decreaseFontSize() {
    if (!doc || !ctx) return;
    if (fz_is_document_reflowable(ctx, doc) && currentFontSize > 7.0f) {
        currentFontSize -= 1.0f;
        relayout();
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

    // Calcula a escala para caber na tela do Vita (920 x 450)
    float targetW = 920.0f;
    float targetH = 450.0f;
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
    // Limpa o fundo com branco/papel
    fz_clear_pixmap_with_value(ctx, pix, 250);

    fz_device* dev = fz_new_draw_device(ctx, ctm, pix);
    fz_run_page(ctx, page, dev, fz_identity, NULL);
    fz_close_device(ctx, dev);
    fz_drop_device(ctx, dev);
    fz_drop_page(ctx, page);

    // Cria textura vita2d compatível (SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR ou similar)
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

void ReaderEPUB::render(vita2d_pgf* font) {
    // Fundo elegante para livro
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(24, 26, 36, 255));

    // Barra superior
    vita2d_draw_rectangle(0, 0, 960, 48, UITheme::TopBar);
    vita2d_draw_line(0, 48, 960, 48, RGBA8(42, 48, 70, 255));

    if (font) {
        std::string title = filename;
        if (title.length() > 40) title = title.substr(0, 37) + "...";
        vita2d_pgf_draw_text(font, 32, 32, UITheme::BadgeEPUB, 1.0f, title.c_str());

        char pageInfo[64];
        snprintf(pageInfo, sizeof(pageInfo), "%d / %d (Fonte: %.0fpt)", currentPage + 1, totalPages > 0 ? totalPages : 1, currentFontSize);
        vita2d_pgf_draw_text(font, 720, 32, UITheme::TextSecondary, 0.85f, pageInfo);
    }

    // Desenha a página renderizada centralizada na tela
    if (pageTexture) {
        float texW = static_cast<float>(vita2d_texture_get_width(pageTexture));
        float texH = static_cast<float>(vita2d_texture_get_height(pageTexture));
        float drawX = (960.0f - texW) / 2.0f;
        float drawY = 48.0f + ((544.0f - 48.0f - 40.0f - texH) / 2.0f);

        // Moldura branca/sombra sutil ao redor da folha
        vita2d_draw_rectangle(drawX - 2.0f, drawY - 2.0f, texW + 4.0f, texH + 4.0f, RGBA8(10, 12, 18, 180));
        vita2d_draw_texture(pageTexture, drawX, drawY);
    }

    UIComponents::drawReaderProgressBar(currentPage, totalPages, false);
    UIComponents::drawFooter("D-Pad Esq/Dir: Páginas | Cima/Baixo: Fonte | O: Voltar");
}
