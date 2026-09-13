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
    constexpr unsigned int BadgeTXT        = RGBA8(16, 185, 129, 255);  // Esmeralda
    constexpr unsigned int BadgeCBZ        = RGBA8(245, 158, 11, 255);  // Âmbar
    constexpr unsigned int BadgeEPUB       = RGBA8(59, 130, 246, 255);  // Azul
    constexpr unsigned int FavoriteGold    = RGBA8(250, 204, 21, 255);  // Dourado
    constexpr unsigned int TopBar          = RGBA8(13, 15, 22, 255);
    constexpr unsigned int SearchBarBg     = RGBA8(30, 35, 52, 255);
    constexpr unsigned int SearchBarActive = RGBA8(45, 52, 78, 255);
}

class UIComponents {
public:
    static void init(vita2d_pgf* defaultPgf);
    static void drawRoundedBox(float x, float y, float w, float h, float radius, unsigned int color);
    
    // Top Bar com Campo de Pesquisa, Ícone de Ordenação e Alternância de Grid/Lista
    static void drawTopBar(
        const std::string& searchQuery,
        bool isSearchActive,
        SortMode currentSort,
        bool isGridView,
        bool isSearchSelected,
        bool isSortSelected,
        bool isLayoutSelected
    );

    // Renderização dos Itens
    static void drawListCard(float x, float y, float w, float h, bool isSelected, const LibraryItem& item);
    static void drawGridCard(float x, float y, float w, float h, bool isSelected, const LibraryItem& item);

    // Utilitários
    static void drawFooter(const std::string& controlsHint);
    static void drawBadge(float x, float y, const std::string& type, float scale = 1.0f);
    static void drawStar(float x, float y, float size, unsigned int color);

private:
    static vita2d_pgf* pgf;
};
