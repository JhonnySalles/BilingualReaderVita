#include "reader_cbz.h"
#include "ui_components.h"
#include "file_utils.h"
#include "parses/parse_rar.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

extern "C" {
#include <mupdf/fitz.h>
}

ReaderCBZ::ReaderCBZ() 
    : currentPage(0), 
      totalPages(0), 
      isRotated(false), 
      ctx(nullptr), 
      doc(nullptr), 
      currentTexture(nullptr), 
      zoomScale(1.0f), 
      panX(0.0f), 
      panY(0.0f),
      rarParser(nullptr) {
}

ReaderCBZ::~ReaderCBZ() {
    close();
}

void ReaderCBZ::freeTexture() {
    if (currentTexture) {
        vita2d_free_texture(currentTexture);
        currentTexture = nullptr;
    }
}

void ReaderCBZ::resetZoom() {
    zoomScale = 1.0f;
    panX = 0.0f;
    panY = 0.0f;
}

void ReaderCBZ::addZoom(float factor, float focusX, float focusY) {
    float oldZoom = zoomScale;
    zoomScale *= factor;
    if (zoomScale < 0.8f) zoomScale = 0.8f;
    if (zoomScale > 3.0f) zoomScale = 3.0f;

    float ratio = zoomScale / oldZoom;
    panX = focusX - (focusX - panX) * ratio;
    panY = focusY - (focusY - panY) * ratio;
    loadPageTexture(currentPage);
}

void ReaderCBZ::addPan(float dx, float dy) {
    panX += dx;
    panY += dy;
}

void ReaderCBZ::close() {
    freeTexture();

    if (doc && ctx) {
        fz_drop_document(ctx, doc);
        doc = nullptr;
    }

    if (ctx) {
        fz_drop_context(ctx);
        ctx = nullptr;
    }

    if (rarParser) {
        delete rarParser;
        rarParser = nullptr;
    }

    currentPage = 0;
    totalPages = 0;
    filename = "";
    filePath = "";
    typeString = "CBZ";
    resetZoom();
}

bool ReaderCBZ::loadFile(const std::string& path) {
    close();
    filePath = path;

    size_t lastSlash = path.find_last_of("/\\");
    filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

    std::string ext = FileUtils::getExtension(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
    typeString = ext.empty() ? "CBZ" : ext;

    if (typeString == "RAR" || typeString == "CBR") {
        rarParser = new ParseRar();
        if (rarParser->open(path)) {
            totalPages = rarParser->getPageCount();
            currentPage = 0;
            loadPageTexture(0);
            resetZoom();
            return true;
        } else {
            close();
            return false;
        }
    }

    // Inicializa o contexto MuPDF com limite de cache estrito (16MB)
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

    totalPages = fz_count_pages(ctx, doc);
    if (totalPages <= 0) {
        close();
        return false;
    }

    currentPage = 0;
    loadPageTexture(0);
    resetZoom();
    return true;
}

void ReaderCBZ::setRotated(bool rotated) {
    if (isRotated != rotated) {
        isRotated = rotated;
        resetZoom();
        loadPageTexture(currentPage);
    }
}

void ReaderCBZ::loadPageTexture(int pageIndex) {
    freeTexture();
    if (pageIndex < 0 || pageIndex >= totalPages) return;

    if (rarParser) {
        currentTexture = rarParser->loadPageTexture(pageIndex);
        return;
    }

    if (!doc || !ctx || totalPages <= 0) return;

    fz_page* page = nullptr;
    fz_try(ctx) {
        page = fz_load_page(ctx, doc, pageIndex);
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

    // Calcula resolução otimizada para o PS Vita
    // Aplica o zoomScale diretamente para manter nitidez
    float maxDimW = isRotated ? 544.0f : 960.0f;
    float maxDimH = isRotated ? 960.0f : 544.0f;
    float baseScale = std::min(maxDimW / pageW, maxDimH / pageH);
    float scale = baseScale * zoomScale;
    if (scale > 3.0f) scale = 3.0f;
    if (scale < 0.2f) scale = 0.2f;

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
    fz_clear_pixmap(ctx, pix);

    fz_device* dev = fz_new_draw_device(ctx, ctm, pix);
    fz_run_page(ctx, page, dev, fz_identity, NULL);
    fz_close_device(ctx, dev);
    fz_drop_device(ctx, dev);
    fz_drop_page(ctx, page);

    currentTexture = vita2d_create_empty_texture_format(renderW, renderH, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR);
    if (currentTexture) {
        unsigned char* texData = reinterpret_cast<unsigned char*>(vita2d_texture_get_datap(currentTexture));
        int texStride = vita2d_texture_get_stride(currentTexture);
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

void ReaderCBZ::nextPage() {
    if (currentPage < totalPages - 1) {
        currentPage++;
        loadPageTexture(currentPage);
        resetZoom();
    }
}

void ReaderCBZ::prevPage() {
    if (currentPage > 0) {
        currentPage--;
        loadPageTexture(currentPage);
        resetZoom();
    }
}

void ReaderCBZ::render(vita2d_pgf* font, bool fullscreen) {
    // Fundo escuro focado para leitura de mangá
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(10, 10, 12, 255));

    if (currentTexture) {
        unsigned int texW = vita2d_texture_get_width(currentTexture);
        unsigned int texH = vita2d_texture_get_height(currentTexture);

        if (isRotated) {
            float rad = 1.57079632679f; // 90 graus
            float centerX = 480.0f + panX;
            float centerY = 272.0f + panY;
            vita2d_draw_texture_rotate(currentTexture, centerX, centerY, rad);
        } else {
            float renderW = static_cast<float>(texW);
            float renderH = static_cast<float>(texH);
            float topOffset = fullscreen ? 0.0f : 48.0f;
            float bottomOffset = fullscreen ? 0.0f : 40.0f;
            float availableH = 544.0f - topOffset - bottomOffset;

            float posX = ((960.0f - renderW) / 2.0f) + panX;
            float posY = topOffset + ((availableH - renderH) / 2.0f) + panY;
            vita2d_draw_texture(currentTexture, posX, posY);
        }
    } else {
        // Fallback visual com indicador moderno
        UIComponents::drawRoundedBox(330, 220, 300, 100, 12.0f, UITheme::Surface);
        if (font) {
            std::string label = "[ Manga / " + typeString + " ]";
            vita2d_pgf_draw_text(font, 360, 260, UITheme::TextPrimary, 1.0f, label.c_str());
            vita2d_pgf_draw_text(font, 380, 290, UITheme::TextSecondary, 0.85f, "Carregando pagina...");
        }
    }

    if (!fullscreen) {
        char pageInfo[64];
        if (zoomScale > 1.01f || zoomScale < 0.99f) {
            snprintf(pageInfo, sizeof(pageInfo), "Pag %d / %d (Zoom: %.0f%%)", currentPage + 1, totalPages > 0 ? totalPages : 1, zoomScale * 100.0f);
        } else {
            snprintf(pageInfo, sizeof(pageInfo), "Pag %d / %d", currentPage + 1, totalPages > 0 ? totalPages : 1);
        }
        
        unsigned int badgeColor = UITheme::BadgeCBZ;
        if (typeString == "ZIP") badgeColor = UITheme::BadgeZIP;
        else if (typeString == "CBR") badgeColor = UITheme::BadgeCBR;
        else if (typeString == "RAR") badgeColor = UITheme::BadgeRAR;
        else if (typeString == "CBT") badgeColor = UITheme::BadgeCBT;
        else if (typeString == "TAR") badgeColor = UITheme::BadgeTAR;
        else if (typeString == "CB7") badgeColor = UITheme::BadgeCB7;
        else if (typeString == "7Z") badgeColor = UITheme::Badge7Z;

        UIComponents::drawReaderTopBar(filename, pageInfo, isRotated, badgeColor);
        UIComponents::drawReaderProgressBar(currentPage, totalPages, false);
        UIComponents::drawFooter("D-Pad: Paginas | Pinca: Zoom | Arraste: Mover | O: Voltar", true);
    }
}
