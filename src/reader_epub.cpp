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
      scrollMode(ReaderScrollMode::PagedHorizontal),
      pageTransition(PageTransition::Slide),
      scrollY(0.0f),
      zoomScale(1.0f),
      panX(0.0f),
      panY(0.0f),
      ctx(nullptr), 
      doc(nullptr), 
      pageTexture(nullptr),
      nextScrollTexture(nullptr),
      animating(false),
      animType(AnimType::None),
      animT(0.0f),
      animScrollTarget(0.0f),
      animScrollApplied(0.0f),
      animDirection(1),
      animHorizontal(true),
      animToPage(0),
      animToTexture(nullptr) {
}

ReaderEPUB::~ReaderEPUB() {
    close();
}

void ReaderEPUB::freeNextScrollTexture() {
    if (nextScrollTexture) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(nextScrollTexture);
        nextScrollTexture = nullptr;
    }
}

void ReaderEPUB::freeAnimTextures() {
    if (animToTexture) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(animToTexture);
        animToTexture = nullptr;
    }
}

void ReaderEPUB::freeTexture() {
    cancelAnimation(false);
    freeNextScrollTexture();
    if (pageTexture) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(pageTexture);
        pageTexture = nullptr;
    }
}

float ReaderEPUB::fittedPageHeight(vita2d_texture* tex, float screenW) const {
    if (!tex) return 0.0f;
    float texW = static_cast<float>(vita2d_texture_get_width(tex));
    float texH = static_cast<float>(vita2d_texture_get_height(tex));
    if (texW <= 0.0f) return 0.0f;
    return texH * (screenW / texW);
}

void ReaderEPUB::ensureNextScrollTexture() {
    if (scrollMode != ReaderScrollMode::InfiniteScroll) {
        freeNextScrollTexture();
        return;
    }
    if (currentPage + 1 >= totalPages) {
        freeNextScrollTexture();
        return;
    }
    if (nextScrollTexture) return;
    nextScrollTexture = createPageTexture(currentPage + 1);
}

void ReaderEPUB::syncScrollTextures() {
    freeNextScrollTexture();
    if (scrollMode == ReaderScrollMode::InfiniteScroll) {
        ensureNextScrollTexture();
    }
}

void ReaderEPUB::setScrollMode(ReaderScrollMode mode) {
    if (scrollMode == mode) return;
    cancelAnimation(false);
    scrollMode = mode;
    scrollY = 0.0f;
    resetZoom();
    syncScrollTextures();
}

void ReaderEPUB::setPageTransition(PageTransition mode) {
    pageTransition = mode;
}

float ReaderEPUB::easeSmoothstep(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

void ReaderEPUB::cancelScrollAnimation() {
    if (animating && animType == AnimType::Scroll) {
        animating = false;
        animType = AnimType::None;
        animT = 0.0f;
        animScrollTarget = 0.0f;
        animScrollApplied = 0.0f;
    }
}

void ReaderEPUB::cancelAnimation(bool commitPage) {
    if (!animating) {
        freeAnimTextures();
        return;
    }
    if (animType == AnimType::Page && commitPage) {
        finishPageAnimation();
        return;
    }
    freeAnimTextures();
    animating = false;
    animType = AnimType::None;
    animT = 0.0f;
    animScrollTarget = 0.0f;
    animScrollApplied = 0.0f;
}

void ReaderEPUB::startScrollAnimation(float distance) {
    cancelAnimation(false);
    animating = true;
    animType = AnimType::Scroll;
    animT = 0.0f;
    animScrollTarget = distance;
    animScrollApplied = 0.0f;
}

void ReaderEPUB::startPageAnimation(int direction) {
    int target = currentPage + direction;
    if (target < 0 || target >= totalPages) return;

    cancelAnimation(false);
    resetZoom();
    freeNextScrollTexture();

    animToTexture = createPageTexture(target);
    if (!animToTexture) return;

    animating = true;
    animType = AnimType::Page;
    animT = 0.0f;
    animDirection = direction;
    animToPage = target;
    animHorizontal = (scrollMode == ReaderScrollMode::PagedHorizontal);
}

void ReaderEPUB::finishPageAnimation() {
    if (pageTexture) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(pageTexture);
        pageTexture = nullptr;
    }
    currentPage = animToPage;
    pageTexture = animToTexture;
    animToTexture = nullptr;
    animating = false;
    animType = AnimType::None;
    animT = 0.0f;
    syncScrollTextures();
}

void ReaderEPUB::update(float dt) {
    if (!animating) return;
    if (dt > 0.05f) dt = 0.05f;
    if (dt < 0.0f) dt = 0.0f;

    animT += dt / kReaderTransitionDuration;
    if (animT >= 1.0f) animT = 1.0f;
    float eased = easeSmoothstep(animT);

    if (animType == AnimType::Scroll) {
        float desired = animScrollTarget * eased;
        float delta = desired - animScrollApplied;
        animScrollApplied = desired;
        if (delta != 0.0f) {
            addScroll(delta);
        }
        if (animT >= 1.0f) {
            animating = false;
            animType = AnimType::None;
            animScrollTarget = 0.0f;
            animScrollApplied = 0.0f;
        }
    } else if (animType == AnimType::Page) {
        if (animT >= 1.0f) {
            finishPageAnimation();
        }
    }
}

void ReaderEPUB::drawPageTexture(vita2d_texture* tex, float ox, float oy, float alpha) const {
    if (!tex) return;
    float screenW = isRotated ? 544.0f : 960.0f;
    float screenH = isRotated ? 960.0f : 544.0f;
    float texW = static_cast<float>(vita2d_texture_get_width(tex));
    float texH = static_cast<float>(vita2d_texture_get_height(tex));
    float posX = ((screenW - texW) / 2.0f) + ox;
    float posY = ((screenH - texH) / 2.0f) + oy;
    if (alpha >= 0.999f) {
        vita2d_draw_texture_scale(tex, posX, posY, 1.0f, 1.0f);
    } else {
        unsigned int tint = RGBA8(255, 255, 255, static_cast<unsigned int>(alpha * 255.0f + 0.5f));
        vita2d_draw_texture_tint_scale(tex, posX, posY, 1.0f, 1.0f, tint);
    }
}

void ReaderEPUB::addScroll(float dy) {
    if (scrollMode != ReaderScrollMode::InfiniteScroll) return;

    float screenW = isRotated ? 544.0f : 960.0f;
    scrollY += dy;

    while (true) {
        float curH = fittedPageHeight(pageTexture, screenW);
        if (curH <= 1.0f) curH = 1.0f;

        if (scrollY >= curH && currentPage < totalPages - 1) {
            scrollY -= curH;
            currentPage++;
            if (pageTexture && pageTexture != nextScrollTexture) {
                vita2d_wait_rendering_done();
                vita2d_free_texture(pageTexture);
            }
            pageTexture = nextScrollTexture;
            nextScrollTexture = nullptr;
            if (!pageTexture) {
                pageTexture = createPageTexture(currentPage);
            }
            ensureNextScrollTexture();
            continue;
        }
        if (scrollY < 0.0f && currentPage > 0) {
            currentPage--;
            freeNextScrollTexture();
            nextScrollTexture = pageTexture;
            pageTexture = createPageTexture(currentPage);
            float prevH = fittedPageHeight(pageTexture, screenW);
            scrollY += prevH;
            continue;
        }
        if (scrollY < 0.0f) scrollY = 0.0f;
        if (currentPage >= totalPages - 1) {
            float screenH = isRotated ? 960.0f : 544.0f;
            float maxScroll = std::max(0.0f, curH - screenH * 0.35f);
            if (scrollY > maxScroll) scrollY = maxScroll;
        }
        break;
    }
    ensureNextScrollTexture();
}

void ReaderEPUB::resetZoom() {
    zoomScale = 1.0f;
    panX = 0.0f;
    panY = 0.0f;
}

void ReaderEPUB::clampPan() {
    if (!pageTexture) {
        panX = 0.0f;
        panY = 0.0f;
        return;
    }

    float screenW = isRotated ? 544.0f : 960.0f;
    float screenH = isRotated ? 960.0f : 544.0f;
    float renderW = static_cast<float>(vita2d_texture_get_width(pageTexture)) * zoomScale;
    float renderH = static_cast<float>(vita2d_texture_get_height(pageTexture)) * zoomScale;

    if (renderW <= screenW) {
        panX = 0.0f;
    } else {
        float maxPanX = (renderW - screenW) * 0.5f;
        if (panX > maxPanX) panX = maxPanX;
        if (panX < -maxPanX) panX = -maxPanX;
    }

    if (renderH <= screenH) {
        panY = 0.0f;
    } else {
        float maxPanY = (renderH - screenH) * 0.5f;
        if (panY > maxPanY) panY = maxPanY;
        if (panY < -maxPanY) panY = -maxPanY;
    }
}

void ReaderEPUB::addZoom(float factor, float focusX, float focusY) {
    float oldZoom = zoomScale;
    zoomScale *= factor;
    if (zoomScale < 0.8f) zoomScale = 0.8f;
    if (zoomScale > 3.0f) zoomScale = 3.0f;

    float ratio = zoomScale / oldZoom;
    panX = focusX - (focusX - panX) * ratio;
    panY = focusY - (focusY - panY) * ratio;
    clampPan();
}

void ReaderEPUB::addPan(float dx, float dy) {
    panX += dx;
    panY += dy;
    clampPan();
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
    resetZoom();
}

bool ReaderEPUB::loadFile(const std::string& path, vita2d_pgf* font) {
    close();

    filePath = path;
    size_t lastSlash = path.find_last_of("/\\");
    filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

    // Inicializa o contexto MuPDF com limite de cache otimizado para o PS Vita (16MB)
    ctx = fz_new_context(NULL, NULL, 16 * 1024 * 1024);
    if (!ctx) return false;

    fz_register_document_handlers(ctx);
    fz_set_user_css(ctx, 
        "@page { margin: 0; } "
        "html, body { margin: 0; padding: 4px; background: transparent; color: #e2e8f0; } "
        "img, svg { max-width: 100%; max-height: 100%; height: auto; display: block; margin: 0 auto; } "
        "p, div, span, h1, h2, h3, h4, h5, h6, li, a { color: #e2e8f0; }"
    );

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
    resetZoom();
    renderCurrentPageToTexture();

    return totalPages > 0;
}

void ReaderEPUB::relayout() {
    if (!doc || !ctx) return;

    if (fz_is_document_reflowable(ctx, doc)) {
        // Dimensões úteis dependendo da rotação
        float layoutW = isRotated ? 520.0f : 940.0f;
        float layoutH = isRotated ? 920.0f : 500.0f;
        fz_layout_document(ctx, doc, layoutW, layoutH, currentFontSize);
    }

    totalPages = fz_count_pages(ctx, doc);
    if (totalPages < 1) totalPages = 1;
    if (currentPage >= totalPages) currentPage = totalPages - 1;
}

void ReaderEPUB::setRotated(bool rotated) {
    if (isRotated != rotated) {
        cancelAnimation(false);
        isRotated = rotated;
        int savedPage = currentPage;
        resetZoom();
        scrollY = 0.0f;
        relayout();
        currentPage = std::max(0, std::min(savedPage, totalPages - 1));
        renderCurrentPageToTexture();
        syncScrollTextures();
    }
}

void ReaderEPUB::increaseFontSize() {
    if (!doc || !ctx) return;
    if (fz_is_document_reflowable(ctx, doc) && currentFontSize < 24.0f) {
        cancelAnimation(false);
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
        cancelAnimation(false);
        currentFontSize -= 1.0f;
        int savedPage = currentPage;
        relayout();
        currentPage = std::max(0, std::min(savedPage, totalPages - 1));
        renderCurrentPageToTexture();
    }
}

vita2d_texture* ReaderEPUB::createPageTexture(int pageIndex) {
    if (!doc || !ctx || totalPages <= 0) return nullptr;
    if (pageIndex < 0 || pageIndex >= totalPages) return nullptr;

    fz_page* page = nullptr;
    fz_try(ctx) {
        page = fz_load_page(ctx, doc, pageIndex);
    }
    fz_catch(ctx) {
        return nullptr;
    }

    if (!page) return nullptr;

    fz_rect bounds = fz_bound_page(ctx, page);
    float pageW = bounds.x1 - bounds.x0;
    float pageH = bounds.y1 - bounds.y0;
    if (pageW <= 0.0f || pageH <= 0.0f) {
        fz_drop_page(ctx, page);
        return nullptr;
    }

    float targetW = isRotated ? 544.0f : 960.0f;
    float targetH = isRotated ? 960.0f : 544.0f;
    float scale = std::min(targetW / pageW, targetH / pageH);
    if (scale > 2.0f) scale = 2.0f;
    if (scale < 0.2f) scale = 0.2f;

    fz_matrix ctm = fz_scale(scale, scale);
    fz_irect ibounds = fz_round_rect(fz_transform_rect(bounds, ctm));
    int renderW = ibounds.x1 - ibounds.x0;
    int renderH = ibounds.y1 - ibounds.y0;

    if (renderW <= 0 || renderH <= 0) {
        fz_drop_page(ctx, page);
        return nullptr;
    }

    fz_colorspace* cs = fz_device_rgb(ctx);
    fz_pixmap* pix = fz_new_pixmap_with_bbox(ctx, cs, ibounds, NULL, 1);
    fz_clear_pixmap(ctx, pix);

    fz_device* dev = fz_new_draw_device(ctx, ctm, pix);
    fz_run_page(ctx, page, dev, fz_identity, NULL);
    fz_close_device(ctx, dev);
    fz_drop_device(ctx, dev);
    fz_drop_page(ctx, page);

    vita2d_texture* tex = vita2d_create_empty_texture_format(renderW, renderH, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR);
    if (tex) {
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
    }

    fz_drop_pixmap(ctx, pix);
    return tex;
}

void ReaderEPUB::renderCurrentPageToTexture() {
    freeTexture();
    pageTexture = createPageTexture(currentPage);
    syncScrollTextures();
}

void ReaderEPUB::nextPage() {
    if (animating) return;
    if (scrollMode == ReaderScrollMode::InfiniteScroll) {
        float screenH = isRotated ? 960.0f : 544.0f;
        startScrollAnimation(screenH);
        return;
    }
    if (currentPage < totalPages - 1) {
        startPageAnimation(1);
    }
}

void ReaderEPUB::prevPage() {
    if (animating) return;
    if (scrollMode == ReaderScrollMode::InfiniteScroll) {
        float screenH = isRotated ? 960.0f : 544.0f;
        startScrollAnimation(-screenH);
        return;
    }
    if (currentPage > 0) {
        startPageAnimation(-1);
    }
}

void ReaderEPUB::render(vita2d_pgf* font, bool fullscreen) {
    float screenW = isRotated ? 544.0f : 960.0f; 
    float screenH = isRotated ? 960.0f : 544.0f;
    vita2d_draw_rectangle(0, 0, screenW, screenH, UITheme::Background);

    if (animating && animType == AnimType::Page && animToTexture) {
        float eased = easeSmoothstep(animT);
        float axis = animHorizontal ? screenW : screenH;
        float fromOx = 0.0f, fromOy = 0.0f, toOx = 0.0f, toOy = 0.0f;
        float fromA = 1.0f, toA = 1.0f;

        if (pageTransition == PageTransition::Fade) {
            fromA = 1.0f - eased;
            toA = eased;
        } else if (pageTransition == PageTransition::Stack) {
            if (animDirection > 0) {
                if (animHorizontal) toOx = axis * (1.0f - eased);
                else toOy = axis * (1.0f - eased);
            } else {
                if (animHorizontal) fromOx = axis * eased;
                else fromOy = axis * eased;
            }
        } else {
            float dir = static_cast<float>(animDirection);
            if (animHorizontal) {
                fromOx = -dir * axis * eased;
                toOx = dir * axis * (1.0f - eased);
            } else {
                fromOy = -dir * axis * eased;
                toOy = dir * axis * (1.0f - eased);
            }
        }

        if (pageTransition == PageTransition::Stack && animDirection < 0) {
            drawPageTexture(animToTexture, toOx, toOy, toA);
            drawPageTexture(pageTexture, fromOx, fromOy, fromA);
        } else {
            drawPageTexture(pageTexture, fromOx, fromOy, fromA);
            drawPageTexture(animToTexture, toOx, toOy, toA);
        }
    } else if (scrollMode == ReaderScrollMode::InfiniteScroll && pageTexture) {
        ensureNextScrollTexture();
        float texW = static_cast<float>(vita2d_texture_get_width(pageTexture));
        float texH = static_cast<float>(vita2d_texture_get_height(pageTexture));
        float fitScale = (texW > 0.0f) ? (screenW / texW) : 1.0f;
        float drawH = texH * fitScale;
        float top = fullscreen ? 0.0f : 48.0f;
        float posY = top - scrollY;
        vita2d_draw_texture_scale(pageTexture, 0.0f, posY, fitScale, fitScale);
        if (nextScrollTexture) {
            float nW = static_cast<float>(vita2d_texture_get_width(nextScrollTexture));
            float nScale = (nW > 0.0f) ? (screenW / nW) : 1.0f;
            vita2d_draw_texture_scale(nextScrollTexture, 0.0f, posY + drawH, nScale, nScale);
        }
    } else if (pageTexture) {
        float texW = static_cast<float>(vita2d_texture_get_width(pageTexture));
        float texH = static_cast<float>(vita2d_texture_get_height(pageTexture));

        float renderW = texW * zoomScale;
        float renderH = texH * zoomScale;

        float drawX = ((screenW - renderW) / 2.0f) + panX;
        float drawY = ((screenH - renderH) / 2.0f) + panY;

        vita2d_draw_texture_scale(pageTexture, drawX, drawY, zoomScale, zoomScale);
    }

    if (!fullscreen) {
        char pageInfo[64];
        snprintf(pageInfo, sizeof(pageInfo), "%d / %d (%.0fpt)", currentPage + 1, totalPages > 0 ? totalPages : 1, currentFontSize);
        UIComponents::drawReaderTopBar(filename, pageInfo, isRotated, false, UITheme::BadgeEPUB);
        UIComponents::drawReaderProgressBar(currentPage, totalPages, false);
        const char* hint = (scrollMode == ReaderScrollMode::InfiniteScroll)
            ? "D-Pad: Rolar | /\\/[]: Fonte | O: Voltar"
            : "D-Pad: Paginas | /\\/[]: Fonte | SEL: Vinculo | O: Voltar";
        UIComponents::drawFooter(hint, true);
    }
}
