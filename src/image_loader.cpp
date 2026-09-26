#include "image_loader.h"
#include <cstdio>
#include <vector>

extern "C" {
#include <mupdf/fitz.h>
}

namespace ImageLoader {

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

vita2d_texture* loadTextureFromFile(const std::string& filePath) {
    if (filePath.empty()) return nullptr;

    size_t dotPos = filePath.find_last_of('.');
    std::string ext = (dotPos != std::string::npos) ? filePath.substr(dotPos + 1) : "";
    for (auto& c : ext) c = tolower(c);

    if (ext == "jpg" || ext == "jpeg") {
        vita2d_texture* tex = vita2d_load_JPEG_file(filePath.c_str());
        if (tex) return tex;
    } else if (ext == "png") {
        vita2d_texture* tex = vita2d_load_PNG_file(filePath.c_str());
        if (tex) return tex;
    } else if (ext == "bmp") {
        vita2d_texture* tex = vita2d_load_BMP_file(filePath.c_str());
        if (tex) return tex;
    }

    fz_context* ctx = fz_new_context(NULL, NULL, 16 * 1024 * 1024);
    if (!ctx) return nullptr;

    fz_register_document_handlers(ctx);

    fz_document* doc = nullptr;
    fz_page* page = nullptr;
    fz_pixmap* pix = nullptr;
    vita2d_texture* tex = nullptr;

    fz_try(ctx) {
        doc = fz_open_document(ctx, filePath.c_str());
        if (doc) {
            int pageCount = fz_count_pages(ctx, doc);
            if (pageCount > 0) {
                page = fz_load_page(ctx, doc, 0);
                if (page) {
                    fz_rect bounds = fz_bound_page(ctx, page);
                    float pageW = bounds.x1 - bounds.x0;
                    float pageH = bounds.y1 - bounds.y0;

                    if (pageW > 0.0f && pageH > 0.0f) {
                        float maxDimW = 960.0f;
                        float maxDimH = 960.0f;
                        float scale = std::min(maxDimW / pageW, maxDimH / pageH);
                        if (scale > 2.0f) scale = 2.0f;
                        if (scale < 0.2f) scale = 0.2f;

                        fz_matrix ctm = fz_scale(scale, scale);
                        fz_irect ibounds = fz_round_rect(fz_transform_rect(bounds, ctm));

                        fz_colorspace* cs = fz_device_rgb(ctx);
                        pix = fz_new_pixmap_with_bbox(ctx, cs, ibounds, NULL, 1);
                        fz_clear_pixmap(ctx, pix);

                        fz_device* dev = fz_new_draw_device(ctx, ctm, pix);
                        fz_run_page(ctx, page, dev, fz_identity, NULL);
                        fz_close_device(ctx, dev);
                        fz_drop_device(ctx, dev);

                        tex = createTextureFromPixmap(ctx, pix);
                    }
                    fz_drop_page(ctx, page);
                    page = nullptr;
                }
            }
            fz_drop_document(ctx, doc);
            doc = nullptr;
        }
    }
    fz_catch(ctx) {
        if (page) fz_drop_page(ctx, page);
        if (doc) fz_drop_document(ctx, doc);
        if (pix) fz_drop_pixmap(ctx, pix);
        fz_drop_context(ctx);
        return nullptr;
    }

    if (pix) {
        fz_drop_pixmap(ctx, pix);
    }
    fz_drop_context(ctx);

    return tex;
}

vita2d_texture* loadTextureFromBuffer(const unsigned char* buffer, size_t size) {
    if (!buffer || size == 0) return nullptr;

    fz_context* ctx = fz_new_context(NULL, NULL, 16 * 1024 * 1024);
    if (!ctx) return nullptr;

    fz_register_document_handlers(ctx);

    fz_buffer* fzbuf = nullptr;
    fz_stream* stm = nullptr;
    fz_document* doc = nullptr;
    fz_page* page = nullptr;
    fz_pixmap* pix = nullptr;
    vita2d_texture* tex = nullptr;

    fz_try(ctx) {
        fzbuf = fz_new_buffer_from_copied_data(ctx, buffer, size);
        stm = fz_open_buffer(ctx, fzbuf);
        doc = fz_open_document_with_stream(ctx, ".jpg", stm); // Reconhece automaticamente imagem pelo header

        if (doc) {
            int pageCount = fz_count_pages(ctx, doc);
            if (pageCount > 0) {
                page = fz_load_page(ctx, doc, 0);
                if (page) {
                    fz_rect bounds = fz_bound_page(ctx, page);
                    float pageW = bounds.x1 - bounds.x0;
                    float pageH = bounds.y1 - bounds.y0;

                    if (pageW > 0.0f && pageH > 0.0f) {
                        float maxDimW = 960.0f;
                        float maxDimH = 960.0f;
                        float scale = std::min(maxDimW / pageW, maxDimH / pageH);
                        if (scale > 2.0f) scale = 2.0f;
                        if (scale < 0.2f) scale = 0.2f;

                        fz_matrix ctm = fz_scale(scale, scale);
                        fz_irect ibounds = fz_round_rect(fz_transform_rect(bounds, ctm));

                        fz_colorspace* cs = fz_device_rgb(ctx);
                        pix = fz_new_pixmap_with_bbox(ctx, cs, ibounds, NULL, 1);
                        fz_clear_pixmap(ctx, pix);

                        fz_device* dev = fz_new_draw_device(ctx, ctm, pix);
                        fz_run_page(ctx, page, dev, fz_identity, NULL);
                        fz_close_device(ctx, dev);
                        fz_drop_device(ctx, dev);

                        tex = createTextureFromPixmap(ctx, pix);
                    }
                    fz_drop_page(ctx, page);
                    page = nullptr;
                }
            }
            fz_drop_document(ctx, doc);
            doc = nullptr;
        }
        fz_drop_stream(ctx, stm);
        stm = nullptr;
        fz_drop_buffer(ctx, fzbuf);
        fzbuf = nullptr;
    }
    fz_catch(ctx) {
        if (page) fz_drop_page(ctx, page);
        if (doc) fz_drop_document(ctx, doc);
        if (stm) fz_drop_stream(ctx, stm);
        if (fzbuf) fz_drop_buffer(ctx, fzbuf);
        if (pix) fz_drop_pixmap(ctx, pix);
        fz_drop_context(ctx);
        return nullptr;
    }

    if (pix) {
        fz_drop_pixmap(ctx, pix);
    }
    fz_drop_context(ctx);

    return tex;
}

}
