#include "reader_cbz.h"
#include "ui_components.h"
#include "file_utils.h"
#include "parses/parse_rar.h"
#include "managers/pages_link_manager.h"
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
      showLinkedPage(false),
      scrollMode(ReaderScrollMode::PagedHorizontal),
      pageTransition(PageTransition::Slide),
      scrollY(0.0f),
      ctx(nullptr), 
      doc(nullptr), 
      currentTexture(nullptr), 
      currentTextureRight(nullptr),
      nextScrollTexture(nullptr),
      zoomScale(1.0f), 
      panX(0.0f), 
      panY(0.0f),
      rarParser(nullptr),
      animating(false),
      animType(AnimType::None),
      animT(0.0f),
      animScrollTarget(0.0f),
      animScrollApplied(0.0f),
      animDirection(1),
      animHorizontal(true),
      animToPage(0),
      animToTexture(nullptr),
      animToTextureRight(nullptr) {
}

ReaderCBZ::~ReaderCBZ() {
    close();
}

void ReaderCBZ::freeNextScrollTexture() {
    if (nextScrollTexture) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(nextScrollTexture);
        nextScrollTexture = nullptr;
    }
}

void ReaderCBZ::freeAnimTextures() {
    if (animToTexture) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(animToTexture);
        animToTexture = nullptr;
    }
    if (animToTextureRight) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(animToTextureRight);
        animToTextureRight = nullptr;
    }
}

void ReaderCBZ::freeTexture() {
    cancelAnimation(false);
    freeNextScrollTexture();
    if (currentTexture) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(currentTexture);
        currentTexture = nullptr;
    }
    if (currentTextureRight) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(currentTextureRight);
        currentTextureRight = nullptr;
    }
}

float ReaderCBZ::fittedPageHeight(vita2d_texture* tex, float screenW) const {
    if (!tex) return 0.0f;
    float texW = static_cast<float>(vita2d_texture_get_width(tex));
    float texH = static_cast<float>(vita2d_texture_get_height(tex));
    if (texW <= 0.0f) return 0.0f;
    float scale = screenW / texW;
    return texH * scale;
}

void ReaderCBZ::ensureNextScrollTexture() {
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

void ReaderCBZ::syncScrollTextures() {
    freeNextScrollTexture();
    if (scrollMode == ReaderScrollMode::InfiniteScroll) {
        ensureNextScrollTexture();
    }
}

void ReaderCBZ::setScrollMode(ReaderScrollMode mode) {
    if (scrollMode == mode) return;
    cancelAnimation(false);
    scrollMode = mode;
    scrollY = 0.0f;
    if (scrollMode != ReaderScrollMode::InfiniteScroll) {
        freeNextScrollTexture();
        resetZoom();
    } else {
        resetZoom();
        syncScrollTextures();
    }
}

void ReaderCBZ::setPageTransition(PageTransition mode) {
    pageTransition = mode;
}

float ReaderCBZ::easeSmoothstep(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

void ReaderCBZ::cancelScrollAnimation() {
    if (animating && animType == AnimType::Scroll) {
        animating = false;
        animType = AnimType::None;
        animT = 0.0f;
        animScrollTarget = 0.0f;
        animScrollApplied = 0.0f;
    }
}

void ReaderCBZ::cancelAnimation(bool commitPage) {
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

void ReaderCBZ::startScrollAnimation(float distance) {
    cancelAnimation(false);
    animating = true;
    animType = AnimType::Scroll;
    animT = 0.0f;
    animScrollTarget = distance;
    animScrollApplied = 0.0f;
}

void ReaderCBZ::loadPagePair(int pageIndex, vita2d_texture*& left, vita2d_texture*& right) {
    left = nullptr;
    right = nullptr;
    if (pageIndex < 0 || pageIndex >= totalPages) return;

    if (showLinkedPage) {
        auto& linkMgr = PagesLinkManager::getInstance();
        if (linkMgr.isLinkedLoaded()) {
            auto& pages = linkMgr.getPagesLink();
            if (pageIndex >= 0 && pageIndex < static_cast<int>(pages.size())) {
                auto& link = pages[pageIndex];
                if (link.fileLinkLeftPage != -1) {
                    Parse* lParse = linkMgr.getLinkedParse();
                    if (lParse) {
                        left = lParse->loadPageTexture(link.fileLinkLeftPage);
                        if (link.fileLinkRightPage != -1) {
                            right = lParse->loadPageTexture(link.fileLinkRightPage);
                        }
                        return;
                    }
                }
            }
        }
    }
    left = createPageTexture(pageIndex);
}

void ReaderCBZ::startPageAnimation(int direction) {
    int target = currentPage + direction;
    if (target < 0 || target >= totalPages) return;

    cancelAnimation(false);
    resetZoom();
    freeNextScrollTexture();

    loadPagePair(target, animToTexture, animToTextureRight);
    if (!animToTexture) return;

    animating = true;
    animType = AnimType::Page;
    animT = 0.0f;
    animDirection = direction;
    animToPage = target;
    animHorizontal = (scrollMode == ReaderScrollMode::PagedHorizontal);
}

void ReaderCBZ::finishPageAnimation() {
    if (currentTexture) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(currentTexture);
        currentTexture = nullptr;
    }
    if (currentTextureRight) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(currentTextureRight);
        currentTextureRight = nullptr;
    }

    currentPage = animToPage;
    currentTexture = animToTexture;
    currentTextureRight = animToTextureRight;
    animToTexture = nullptr;
    animToTextureRight = nullptr;

    animating = false;
    animType = AnimType::None;
    animT = 0.0f;
    syncScrollTextures();
}

void ReaderCBZ::update(float dt) {
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

void ReaderCBZ::drawPagePair(vita2d_texture* left, vita2d_texture* right, float ox, float oy, float alpha) const {
    if (!left) return;
    float screenW = isRotated ? 544.0f : 960.0f;
    float screenH = isRotated ? 960.0f : 544.0f;
    unsigned int tint = RGBA8(255, 255, 255, static_cast<unsigned int>(alpha * 255.0f + 0.5f));

    if (right) {
        float renderWL = static_cast<float>(vita2d_texture_get_width(left));
        float renderHL = static_cast<float>(vita2d_texture_get_height(left));
        float renderWR = static_cast<float>(vita2d_texture_get_width(right));
        float renderHR = static_cast<float>(vita2d_texture_get_height(right));
        float totalW = renderWL + renderWR;
        float posX = ((screenW - totalW) / 2.0f) + ox;
        float posY_L = ((screenH - renderHL) / 2.0f) + oy;
        float posY_R = ((screenH - renderHR) / 2.0f) + oy;
        if (alpha >= 0.999f) {
            vita2d_draw_texture_scale(left, posX, posY_L, 1.0f, 1.0f);
            vita2d_draw_texture_scale(right, posX + renderWL, posY_R, 1.0f, 1.0f);
        } else {
            vita2d_draw_texture_tint_scale(left, posX, posY_L, 1.0f, 1.0f, tint);
            vita2d_draw_texture_tint_scale(right, posX + renderWL, posY_R, 1.0f, 1.0f, tint);
        }
    } else {
        float renderW = static_cast<float>(vita2d_texture_get_width(left));
        float renderH = static_cast<float>(vita2d_texture_get_height(left));
        float posX = ((screenW - renderW) / 2.0f) + ox;
        float posY = ((screenH - renderH) / 2.0f) + oy;
        if (alpha >= 0.999f) {
            vita2d_draw_texture_scale(left, posX, posY, 1.0f, 1.0f);
        } else {
            vita2d_draw_texture_tint_scale(left, posX, posY, 1.0f, 1.0f, tint);
        }
    }
}

void ReaderCBZ::addScroll(float dy) {
    if (scrollMode != ReaderScrollMode::InfiniteScroll) return;

    float screenW = isRotated ? 544.0f : 960.0f;
    scrollY += dy;

    while (true) {
        float curH = fittedPageHeight(currentTexture, screenW);
        if (curH <= 1.0f) curH = 1.0f;

        if (scrollY >= curH && currentPage < totalPages - 1) {
            scrollY -= curH;
            currentPage++;
            if (currentTextureRight) {
                vita2d_wait_rendering_done();
                vita2d_free_texture(currentTextureRight);
                currentTextureRight = nullptr;
            }
            if (currentTexture && currentTexture != nextScrollTexture) {
                vita2d_wait_rendering_done();
                vita2d_free_texture(currentTexture);
            }
            currentTexture = nextScrollTexture;
            nextScrollTexture = nullptr;
            if (!currentTexture) {
                currentTexture = createPageTexture(currentPage);
            }
            ensureNextScrollTexture();
            continue;
        }
        if (scrollY < 0.0f && currentPage > 0) {
            currentPage--;
            if (currentTextureRight) {
                vita2d_wait_rendering_done();
                vita2d_free_texture(currentTextureRight);
                currentTextureRight = nullptr;
            }
            freeNextScrollTexture();
            nextScrollTexture = currentTexture;
            currentTexture = createPageTexture(currentPage);
            float prevH = fittedPageHeight(currentTexture, screenW);
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

void ReaderCBZ::toggleLinkedPage() {
    cancelAnimation(false);
    showLinkedPage = !showLinkedPage;
    loadPageTexture(currentPage);
}

void ReaderCBZ::resetZoom() {
    zoomScale = 1.0f;
    panX = 0.0f;
    panY = 0.0f;
}

void ReaderCBZ::clampPan() {
    if (!currentTexture) {
        panX = 0.0f;
        panY = 0.0f;
        return;
    }

    float screenW = isRotated ? 544.0f : 960.0f;
    float screenH = isRotated ? 960.0f : 544.0f;

    float renderW = 0.0f;
    float renderH = 0.0f;
    if (currentTextureRight) {
        float renderWL = static_cast<float>(vita2d_texture_get_width(currentTexture)) * zoomScale;
        float renderHL = static_cast<float>(vita2d_texture_get_height(currentTexture)) * zoomScale;
        float renderWR = static_cast<float>(vita2d_texture_get_width(currentTextureRight)) * zoomScale;
        float renderHR = static_cast<float>(vita2d_texture_get_height(currentTextureRight)) * zoomScale;
        renderW = renderWL + renderWR;
        renderH = std::max(renderHL, renderHR);
    } else {
        renderW = static_cast<float>(vita2d_texture_get_width(currentTexture)) * zoomScale;
        renderH = static_cast<float>(vita2d_texture_get_height(currentTexture)) * zoomScale;
    }

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

void ReaderCBZ::addZoom(float factor, float focusX, float focusY) {
    float oldZoom = zoomScale;
    zoomScale *= factor;
    if (zoomScale < 0.8f) zoomScale = 0.8f;
    if (zoomScale > 3.0f) zoomScale = 3.0f;

    float ratio = zoomScale / oldZoom;
    panX = focusX - (focusX - panX) * ratio;
    panY = focusY - (focusY - panY) * ratio;
    clampPan();
}

void ReaderCBZ::addPan(float dx, float dy) {
    panX += dx;
    panY += dy;
    clampPan();
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
    showLinkedPage = false;
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
        cancelAnimation(false);
        isRotated = rotated;
        resetZoom();
        scrollY = 0.0f;
        loadPageTexture(currentPage);
        syncScrollTextures();
    }
}

vita2d_texture* ReaderCBZ::createPageTexture(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= totalPages) return nullptr;

    if (showLinkedPage) {
        auto& linkMgr = PagesLinkManager::getInstance();
        if (linkMgr.isLinkedLoaded()) {
            auto& pages = linkMgr.getPagesLink();
            if (pageIndex >= 0 && pageIndex < static_cast<int>(pages.size())) {
                auto& link = pages[pageIndex];
                if (link.fileLinkLeftPage != -1) {
                    Parse* lParse = linkMgr.getLinkedParse();
                    if (lParse) {
                        return lParse->loadPageTexture(link.fileLinkLeftPage);
                    }
                }
            }
        }
    }

    if (rarParser) {
        return rarParser->loadPageTexture(pageIndex);
    }

    if (!doc || !ctx || totalPages <= 0) return nullptr;

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

    float maxDimW = isRotated ? 544.0f : 960.0f;
    float maxDimH = isRotated ? 960.0f : 544.0f;
    float scale = std::min(maxDimW / pageW, maxDimH / pageH);
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

void ReaderCBZ::loadPageTexture(int pageIndex) {
    freeTexture();
    if (pageIndex < 0 || pageIndex >= totalPages) return;

    if (showLinkedPage) {
        auto& linkMgr = PagesLinkManager::getInstance();
        if (linkMgr.isLinkedLoaded()) {
            auto& pages = linkMgr.getPagesLink();
            if (pageIndex >= 0 && pageIndex < static_cast<int>(pages.size())) {
                auto& link = pages[pageIndex];
                if (link.fileLinkLeftPage != -1) {
                    Parse* lParse = linkMgr.getLinkedParse();
                    if (lParse) {
                        if (link.fileLinkRightPage != -1) {
                            currentTexture = lParse->loadPageTexture(link.fileLinkLeftPage);
                            currentTextureRight = lParse->loadPageTexture(link.fileLinkRightPage);
                            syncScrollTextures();
                            return;
                        } else {
                            currentTexture = lParse->loadPageTexture(link.fileLinkLeftPage);
                            syncScrollTextures();
                            return;
                        }
                    }
                }
            }
        }
    }

    currentTexture = createPageTexture(pageIndex);
    syncScrollTextures();
}

void ReaderCBZ::nextPage() {
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

void ReaderCBZ::prevPage() {
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

void ReaderCBZ::render(vita2d_pgf* font, bool fullscreen) {
    float screenW = isRotated ? 544.0f : 960.0f; 
    float screenH = isRotated ? 960.0f : 544.0f;
    vita2d_draw_rectangle(0, 0, screenW, screenH, RGBA8(10, 10, 12, 255));

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
            drawPagePair(animToTexture, animToTextureRight, toOx, toOy, toA);
            drawPagePair(currentTexture, currentTextureRight, fromOx, fromOy, fromA);
        } else {
            drawPagePair(currentTexture, currentTextureRight, fromOx, fromOy, fromA);
            drawPagePair(animToTexture, animToTextureRight, toOx, toOy, toA);
        }
    } else if (scrollMode == ReaderScrollMode::InfiniteScroll && currentTexture) {
        ensureNextScrollTexture();
        float texW = static_cast<float>(vita2d_texture_get_width(currentTexture));
        float texH = static_cast<float>(vita2d_texture_get_height(currentTexture));
        float fitScale = (texW > 0.0f) ? (screenW / texW) : 1.0f;
        float drawH = texH * fitScale;
        float top = fullscreen ? 0.0f : 48.0f;
        float posY = top - scrollY;
        vita2d_draw_texture_scale(currentTexture, 0.0f, posY, fitScale, fitScale);

        if (nextScrollTexture) {
            float nW = static_cast<float>(vita2d_texture_get_width(nextScrollTexture));
            float nScale = (nW > 0.0f) ? (screenW / nW) : 1.0f;
            vita2d_draw_texture_scale(nextScrollTexture, 0.0f, posY + drawH, nScale, nScale);
        }
    } else if (currentTexture && currentTextureRight) {
        unsigned int texWL = vita2d_texture_get_width(currentTexture);
        unsigned int texHL = vita2d_texture_get_height(currentTexture);
        unsigned int texWR = vita2d_texture_get_width(currentTextureRight);
        unsigned int texHR = vita2d_texture_get_height(currentTextureRight);

        float renderWL = static_cast<float>(texWL) * zoomScale;
        float renderHL = static_cast<float>(texHL) * zoomScale;
        float renderWR = static_cast<float>(texWR) * zoomScale;
        float renderHR = static_cast<float>(texHR) * zoomScale;

        float totalW = renderWL + renderWR;

        float posX = ((screenW - totalW) / 2.0f) + panX;
        float posY_L = ((screenH - renderHL) / 2.0f) + panY;
        float posY_R = ((screenH - renderHR) / 2.0f) + panY;

        vita2d_draw_texture_scale(currentTexture, posX, posY_L, zoomScale, zoomScale);
        vita2d_draw_texture_scale(currentTextureRight, posX + renderWL, posY_R, zoomScale, zoomScale);
    } else if (currentTexture) {
        unsigned int texW = vita2d_texture_get_width(currentTexture);
        unsigned int texH = vita2d_texture_get_height(currentTexture);

        float renderW = static_cast<float>(texW) * zoomScale;
        float renderH = static_cast<float>(texH) * zoomScale;

        float posX = ((screenW - renderW) / 2.0f) + panX;
        float posY = ((screenH - renderH) / 2.0f) + panY;
        vita2d_draw_texture_scale(currentTexture, posX, posY, zoomScale, zoomScale);
    } else {
        UIComponents::drawRoundedBox(screenW/2.0f - 150.0f, screenH/2.0f - 50.0f, 300, 100, 12.0f, UITheme::Surface);
        if (font) {
            std::string label = "[ Manga / " + typeString + " ]";
            vita2d_pgf_draw_text(font, screenW/2.0f - 120.0f, screenH/2.0f - 10.0f, UITheme::TextPrimary, 1.0f, label.c_str());
            vita2d_pgf_draw_text(font, screenW/2.0f - 100.0f, screenH/2.0f + 20.0f, UITheme::TextSecondary, 0.85f, "Carregando pagina...");
        }
    }

    if (!fullscreen) {
        char pageInfo[64];
        if (scrollMode == ReaderScrollMode::InfiniteScroll) {
            snprintf(pageInfo, sizeof(pageInfo), "Pag %d / %d", currentPage + 1, totalPages > 0 ? totalPages : 1);
        } else if (zoomScale > 1.01f || zoomScale < 0.99f) {
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

        UIComponents::drawReaderTopBar(filename, pageInfo, isRotated, showLinkedPage, badgeColor);
        UIComponents::drawReaderProgressBar(currentPage, totalPages, false);
        const char* hint = (scrollMode == ReaderScrollMode::InfiniteScroll)
            ? "D-Pad: Rolar | TRI: Vinculo | O: Voltar"
            : "D-Pad: Paginas | Pinca: Zoom | TRI: Vinculo | O: Voltar | QUAD: Trad";
        UIComponents::drawFooter(hint, true);
    }
}
