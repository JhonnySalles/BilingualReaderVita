#pragma once
#include <vita2d.h>
#include <string>
#include "file_browser.h"

namespace UITheme {
    constexpr unsigned int Background      = RGBA8(18, 20, 29, 255);
    constexpr unsigned int Surface         = RGBA8(28, 32, 46, 255);
    constexpr unsigned int SurfaceActive   = RGBA8(42, 48, 70, 255);
    constexpr unsigned int Primary         = RGBA8(99, 102, 241, 255);   // Indigo
    constexpr unsigned int Secondary       = RGBA8(139, 92, 246, 255);  // Violeta
    constexpr unsigned int TextPrimary     = RGBA8(243, 244, 246, 255); // Branco suave
    constexpr unsigned int TextSecondary   = RGBA8(156, 163, 175, 255); // Cinza
    
    // Badges por tipo
    constexpr unsigned int BadgeTXT        = RGBA8(16, 185, 129, 255);  // Esmeralda
    constexpr unsigned int BadgeCBZ        = RGBA8(245, 158, 11, 255);  // Âmbar
    constexpr unsigned int BadgeZIP        = RGBA8(217, 119, 6, 255);   // Laranja escuro
    constexpr unsigned int BadgeCBR        = RGBA8(239, 68, 68, 255);   // Vermelho
    constexpr unsigned int BadgeRAR        = RGBA8(185, 28, 28, 255);   // Vinho/Carmim
    constexpr unsigned int BadgeCBT        = RGBA8(14, 165, 233, 255);  // Ciano
    constexpr unsigned int BadgeTAR        = RGBA8(3, 105, 161, 255);   // Azul petróleo
    constexpr unsigned int BadgeCB7        = RGBA8(168, 85, 247, 255);  // Roxo
    constexpr unsigned int Badge7Z         = RGBA8(126, 34, 206, 255);  // Roxo escuro
    constexpr unsigned int BadgeEPUB       = RGBA8(59, 130, 246, 255);  // Azul
    constexpr unsigned int FavoriteGold    = RGBA8(250, 204, 21, 255);  // Dourado
    constexpr unsigned int TopBar          = RGBA8(13, 15, 22, 255);
    constexpr unsigned int SearchBarBg     = RGBA8(30, 35, 52, 255);
    constexpr unsigned int SearchBarActive = RGBA8(45, 52, 78, 255);
    constexpr unsigned int ProgressBarBg   = RGBA8(35, 39, 58, 200);
    constexpr unsigned int ProgressBarFill = RGBA8(99, 102, 241, 255);
}

class UIComponents {
public:
    static void init(vita2d_pgf* defaultPgf);
    static void shutdown();
    static void drawRoundedBox(float x, float y, float w, float h, float radius, unsigned int color);
    
    // Top Bar com Campo de Pesquisa, Ícone de Ordenação, Alternância de Grid/Lista e Botão Refresh
    static void drawTopBar(
        const std::string& searchQuery,
        bool isSearchActive,
        SortMode currentSort,
        bool isGridView,
        bool isSearchSelected,
        bool isSortSelected,
        bool isLayoutSelected,
        bool isRefreshSelected
    );

    // Utilitário para cálculo de transparência (Alpha Blending)
    static inline unsigned int applyAlpha(unsigned int color, float alpha) {
        if (alpha <= 0.0f) return 0;
        if (alpha >= 1.0f) return color;
        unsigned int r = color & 0xFF;
        unsigned int g = (color >> 8) & 0xFF;
        unsigned int b = (color >> 16) & 0xFF;
        unsigned int a = (color >> 24) & 0xFF;
        unsigned int newA = static_cast<unsigned int>(a * alpha);
        if (newA > 255) newA = 255;
        return RGBA8(r, g, b, newA);
    }

    // Renderização dos Itens com suporte a Alpha
    static void drawListCard(float x, float y, float w, float h, bool isSelected, const LibraryItem& item, float alpha = 1.0f);
    static void drawGridCard(float x, float y, float w, float h, bool isSelected, const LibraryItem& item, float alpha = 1.0f);

    // Barra de rolagem da biblioteca
    static void drawLibraryScrollBar(float currentOffset, int totalItems, int visibleCapacity, bool isGridView, float alpha = 1.0f);

    // Diálogo de Confirmação (Popup)
    static void drawConfirmDialog(const std::string& title, const std::string& message, bool isYesSelected);

    // Utilitários
    static void drawFooter(const std::string& controlsHint, bool showBookNav = false);
    static void drawReaderTopBar(const std::string& title, const std::string& extraInfo, bool isRotated, unsigned int badgeColor = UITheme::Primary);
    static void drawBadge(float x, float y, const std::string& type, float scale = 1.0f, float alpha = 1.0f);
    static void drawStar(float x, float y, float size, unsigned int color);

    // Barra de progresso de leitura (HUD inferior do leitor)
    static void drawReaderProgressBar(int currentPage, int totalPages, bool isHudVisible);

private:
    static vita2d_pgf* pgf;
    static vita2d_texture* appIcon;
};

