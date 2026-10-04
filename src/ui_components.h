#pragma once
#include <vita2d.h>
#include <string>
#include "file_browser.h"

struct UIRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool contains(float px, float py) const {
        return px >= x && px <= (x + w) && py >= y && py <= (y + h);
    }
};

struct UITopBarLayout {
    UIRect search;
    UIRect sort;
    UIRect layout;
    UIRect settings;
    UIRect refresh;
};

struct UIReaderChromeLayout {
    UIRect rotateBtn;
    UIRect linkBtn;
    UIRect prevBook;
    UIRect nextBook;
    float contentTop = 48.0f;
    float contentBottom = 504.0f;
    float screenW = 960.0f;
    float screenH = 544.0f;
};

struct UIConfirmDialogLayout {
    UIRect dialog;
    UIRect yesBtn;
    UIRect noBtn;
};

struct UIPagesLinkLayout {
    UIRect primaryFileHeader;
    UIRect selectFileBtn;
    UIRect autoBtn;
    UIRect simpleBtn;
    UIRect doubleBtn;
    UIRect sortBtn;
    UIRect undoBtn;
    UIRect saveBtn;
    UIRect linkedArea;
    UIRect unlinkedArea;
};

struct UISettingsLayout {
    UIRect themeRow;
    UIRect item0;   // Tema (Horizontal Cards container)
    UIRect item1;   // Modo de Exibição
    UIRect item2;   // Ordenação Padrão
    UIRect item3;   // Orientação do Leitor
    UIRect item4;   // Exibir Número de Páginas
    UIRect item5;   // Tamanho de Fonte Padrão
    float fontValueSplitX = 600.0f;
    float contentTotalH = 500.0f;

    UIRect getThemeCardRect(int themeIndex, float scrollX = 0.0f) const {
        float cardW = 92.0f;
        float cardH = 80.0f;
        float gap = 12.0f;
        float startX = 36.0f - scrollX;
        return {startX + themeIndex * (cardW + gap), item0.y + 4.0f, cardW, cardH};
    }
};

namespace UITheme {
    // Cores do Tema ativo (atualizadas via ThemeManager::apply)
    extern unsigned int Background;
    extern unsigned int TopBar;
    extern unsigned int Surface;
    extern unsigned int SurfaceActive;
    extern unsigned int Divider;
    extern unsigned int Primary;
    extern unsigned int OnPrimary;
    extern unsigned int Secondary;
    extern unsigned int TextPrimary;
    extern unsigned int TextSecondary;
    extern unsigned int SearchBarBg;
    extern unsigned int SearchBarActive;
    extern unsigned int ProgressBarBg;
    extern unsigned int CoverBg;
    extern unsigned int CoverBgActive;

    // Badges por tipo (Fixas para manter identidade de cada extensão)
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
}

class UIComponents {
public:
    static void init(vita2d_pgf* defaultPgf);
    static void shutdown();
    static void setRotated(bool rotated);
    static bool isRotated();
    static float getScreenW();
    static float getScreenH();

    static UITopBarLayout getTopBarLayout();
    static UIReaderChromeLayout getReaderChromeLayout();
    static UIConfirmDialogLayout getConfirmDialogLayout();
    static UISettingsLayout getSettingsLayout(float scrollY = 0.0f);

    static void drawRoundedBox(float x, float y, float w, float h, float radius, unsigned int color);
    
    // Top Bar com Campo de Pesquisa, Ícone de Ordenação, Alternância de Grid/Lista, Botão Configurações e Refresh
    static void drawTopBar(
        const std::string& searchQuery,
        bool isSearchActive,
        SortMode currentSort,
        bool isGridView,
        bool isSearchSelected,
        bool isSortSelected,
        bool isLayoutSelected,
        bool isSettingsSelected,
        bool isRefreshSelected
    );

    // Tela de Configurações (Estilo PS Vita Nativo) com rolagem vertical e horizontal nos temas
    static void drawSettingsScreen(int selectedItemIndex, const struct AppConfig& config, float scrollY = 0.0f, float themeScrollX = 0.0f, int themeCursor = 0);

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

    // Diálogo de Progresso / Carregamento (Popup)
    static void drawProgressPopup(const std::string& title, const std::string& message, float progress = -1.0f);

    // Utilitários
    static void drawFooter(const std::string& controlsHint, bool showBookNav = false);
    static void drawReaderTopBar(const std::string& title, const std::string& extraInfo, bool isRotated, unsigned int badgeColor = UITheme::Primary);
    static void drawBadge(float x, float y, const std::string& type, float scale = 1.0f, float alpha = 1.0f);
    static void drawStar(float x, float y, float size, unsigned int color);

    // Barra de progresso de leitura (HUD inferior do leitor)
    static void drawReaderProgressBar(int currentPage, int totalPages, bool isHudVisible);

    // Layout e Renderização da Tela de Vínculo de Páginas
    static UIPagesLinkLayout getPagesLinkLayout();
    static void drawPagesLinkToolbar(
        const std::string& primaryName,
        int primaryPages,
        const std::string& linkedName,
        int linkedPages,
        int selectedToolbarBtn,
        bool hasBackup,
        bool isLinkedLoaded
    );
    static void drawLinkedPageCard(
        float x,
        float y,
        float w,
        float h,
        int index,
        const struct LinkedPage& page,
        vita2d_texture* thumbManga,
        vita2d_texture* thumbLeft,
        vita2d_texture* thumbRight,
        bool isSelected,
        bool isDropTarget
    );
    static void drawUnlinkedCard(
        float x,
        float y,
        float w,
        float h,
        int index,
        const struct LinkedPage& page,
        vita2d_texture* thumb,
        bool isSelected,
        bool isDropTarget
    );
    static void drawPagesLinkScrollBar(float x, float y, float w, float h, float scrollPos, float totalRange);
    static void drawDragGhost(float x, float y, float w, float h, vita2d_texture* thumb, const std::string& label);

private:
    static vita2d_pgf* pgf;
    static vita2d_texture* appIcon;
};

