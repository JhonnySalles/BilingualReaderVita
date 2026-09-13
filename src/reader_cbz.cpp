#include "reader_cbz.h"
#include "ui_components.h"
#include <algorithm>
#include <cstdio>

ReaderCBZ::ReaderCBZ() : currentPage(0), totalPages(0), currentTexture(nullptr) {}

ReaderCBZ::~ReaderCBZ() {
    close();
}

void ReaderCBZ::freeTexture() {
    if (currentTexture) {
        vita2d_free_texture(currentTexture);
        currentTexture = nullptr;
    }
}

void ReaderCBZ::close() {
    freeTexture();
    imageEntries.clear();
    currentPage = 0;
    totalPages = 0;
    filename = "";
    filePath = "";
}

bool ReaderCBZ::loadFile(const std::string& path) {
    close();
    filePath = path;

    size_t lastSlash = path.find_last_of("/\\");
    filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

    // Em implementações com libzip / minizip, escaneia as entradas .jpg/.png
    // Adicionamos fallback estrutural seguro de páginas
    totalPages = 1; // Inicializa com ao menos 1 página
    loadPageTexture(0);
    return true;
}

void ReaderCBZ::loadPageTexture(int pageIndex) {
    freeTexture();
    // Exemplo: carrega imagem com vita2d_load_PNG_file ou vita2d_load_JPEG_file
    // Se a textura for carregada diretamente na VRAM, o escalonamento é instantâneo via GPU
}

void ReaderCBZ::nextPage() {
    if (currentPage < totalPages - 1) {
        currentPage++;
        loadPageTexture(currentPage);
    }
}

void ReaderCBZ::prevPage() {
    if (currentPage > 0) {
        currentPage--;
        loadPageTexture(currentPage);
    }
}

void ReaderCBZ::render(vita2d_pgf* font) {
    // Fundo escuro focado para leitura de mangá
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(10, 10, 12, 255));

    if (currentTexture) {
        unsigned int texW = vita2d_texture_get_width(currentTexture);
        unsigned int texH = vita2d_texture_get_height(currentTexture);

        // Ajusta escala mantendo aspect ratio
        float scale = std::min(960.0f / texW, 544.0f / texH);
        float renderW = texW * scale;
        float renderH = texH * scale;
        float posX = (960.0f - renderW) / 2.0f;
        float posY = (544.0f - renderH) / 2.0f;

        vita2d_draw_texture_scale(currentTexture, posX, posY, scale, scale);
    } else {
        // Fallback visual com indicador moderno
        UIComponents::drawRoundedBox(330, 220, 300, 100, 12.0f, UITheme::Surface);
        if (font) {
            vita2d_pgf_draw_text(font, 360, 260, UITheme::TextPrimary, 1.0f, "[ Mangá / CBZ ]");
            vita2d_pgf_draw_text(font, 380, 290, UITheme::TextSecondary, 0.85f, "Carregando página...");
        }
    }

    // Overlay sutil de status
    if (font) {
        char pageInfo[64];
        snprintf(pageInfo, sizeof(pageInfo), "Pág %d / %d", currentPage + 1, totalPages > 0 ? totalPages : 1);
        vita2d_pgf_draw_text(font, 32, 524, UITheme::TextSecondary, 0.85f, pageInfo);
    }

    UIComponents::drawFooter("D-Pad Esq/Dir: Páginas | O: Voltar");
}
