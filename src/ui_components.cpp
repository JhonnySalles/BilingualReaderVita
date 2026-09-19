#include "ui_components.h"
#include <algorithm>

vita2d_pgf* UIComponents::pgf = nullptr;
vita2d_texture* UIComponents::appIcon = nullptr;

void UIComponents::init(vita2d_pgf* defaultPgf) {
    pgf = defaultPgf;
    if (!appIcon) {
        appIcon = vita2d_load_PNG_file("app0:assets/images/app_icon.png");
        if (!appIcon) appIcon = vita2d_load_PNG_file("app0:app_icon.png");
        if (!appIcon) appIcon = vita2d_load_PNG_file("assets/images/app_icon.png");
        if (!appIcon) appIcon = vita2d_load_PNG_file("app_icon.png");
    }
}

void UIComponents::shutdown() {
    if (appIcon) {
        vita2d_free_texture(appIcon);
        appIcon = nullptr;
    }
}

void UIComponents::drawRoundedBox(float x, float y, float w, float h, float radius, unsigned int color) {
    vita2d_draw_rectangle(x + radius, y, w - 2 * radius, h, color);
    vita2d_draw_rectangle(x, y + radius, radius, h - 2 * radius, color);
    vita2d_draw_rectangle(x + w - radius, y + radius, radius, h - 2 * radius, color);
    
    vita2d_draw_fill_circle(x + radius, y + radius, radius, color);
    vita2d_draw_fill_circle(x + w - radius, y + radius, radius, color);
    vita2d_draw_fill_circle(x + radius, y + h - radius, radius, color);
    vita2d_draw_fill_circle(x + w - radius, y + h - radius, radius, color);
}

void UIComponents::drawStar(float x, float y, float size, unsigned int color) {
    // Desenha uma estrela estilizada usando círculos/formas geométricas
    vita2d_draw_fill_circle(x + size / 2.0f, y + size / 2.0f, size / 2.2f, color);
}

void UIComponents::drawBadge(float x, float y, const std::string& type, float scale) {
    unsigned int badgeColor = UITheme::BadgeTXT;
    if (type == "CBZ") badgeColor = UITheme::BadgeCBZ;
    else if (type == "EPUB") badgeColor = UITheme::BadgeEPUB;

    float w = 54.0f * scale;
    float h = 22.0f * scale;
    drawRoundedBox(x, y, w, h, 5.0f * scale, badgeColor);

    if (pgf) {
        vita2d_pgf_draw_text(pgf, x + 8.0f * scale, y + 16.0f * scale, RGBA8(255, 255, 255, 255), 0.8f * scale, type.c_str());
    }
}

void UIComponents::drawTopBar(
    const std::string& searchQuery,
    bool isSearchActive,
    SortMode currentSort,
    bool isGridView,
    bool isSearchSelected,
    bool isSortSelected,
    bool isLayoutSelected
) {
    // Fundo da Top Bar
    vita2d_draw_rectangle(0, 0, 960, 68, UITheme::TopBar);
    vita2d_draw_line(0, 68, 960, 68, RGBA8(42, 48, 70, 255));

    // Logo / Ícone e Nome do App à esquerda
    if (appIcon) {
        float texW = (float)vita2d_texture_get_width(appIcon);
        float texH = (float)vita2d_texture_get_height(appIcon);
        float targetSize = 40.0f;
        float scaleX = targetSize / (texW > 0 ? texW : 1.0f);
        float scaleY = targetSize / (texH > 0 ? texH : 1.0f);
        float iconX = 18.0f;
        float iconY = 14.0f;
        vita2d_draw_texture_scale(appIcon, iconX, iconY, scaleX, scaleY);

        if (pgf) {
            vita2d_pgf_draw_text(pgf, iconX + targetSize + 10.0f, 44, UITheme::Primary, 1.15f, "Bilingual Reader");
        }
    } else if (pgf) {
        vita2d_pgf_draw_text(pgf, 20, 44, UITheme::Primary, 1.15f, "Bilingual Reader");
    }

    // Campo de Busca
    float searchX = 275.0f;
    float searchY = 14.0f;
    float searchW = 325.0f;
    float searchH = 40.0f;

    unsigned int searchBg = isSearchSelected ? UITheme::SearchBarActive : UITheme::SearchBarBg;
    drawRoundedBox(searchX, searchY, searchW, searchH, 8.0f, searchBg);

    if (isSearchSelected) {
        vita2d_draw_rectangle(searchX, searchY + 4.0f, 3.0f, searchH - 8.0f, UITheme::Primary);
    }

    if (pgf) {
        std::string displayText = searchQuery.empty() ? "Pesquisar livros e mangas..." : searchQuery;
        unsigned int textColor = searchQuery.empty() ? UITheme::TextSecondary : UITheme::TextPrimary;
        vita2d_pgf_draw_text(pgf, searchX + 16.0f, searchY + 26.0f, textColor, 0.85f, displayText.c_str());
    }

    // Botão de Ordenação
    float sortX = 620.0f;
    float sortY = 14.0f;
    float sortW = 180.0f;
    float sortH = 40.0f;

    unsigned int sortBg = isSortSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(sortX, sortY, sortW, sortH, 8.0f, sortBg);

    if (isSortSelected) {
        vita2d_draw_rectangle(sortX, sortY + 4.0f, 3.0f, sortH - 8.0f, UITheme::Secondary);
    }

    if (pgf) {
        std::string sortLabel = std::string("Ord: ") + FileBrowser::getSortModeName(currentSort);
        vita2d_pgf_draw_text(pgf, sortX + 14.0f, sortY + 26.0f, UITheme::TextPrimary, 0.85f, sortLabel.c_str());
    }

    // Botão de Alternância Lista / Grade
    float layoutX = 816.0f;
    float layoutY = 14.0f;
    float layoutW = 120.0f;
    float layoutH = 40.0f;

    unsigned int layoutBg = isLayoutSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(layoutX, layoutY, layoutW, layoutH, 8.0f, layoutBg);

    if (isLayoutSelected) {
        vita2d_draw_rectangle(layoutX, layoutY + 4.0f, 3.0f, layoutH - 8.0f, UITheme::Primary);
    }

    if (pgf) {
        const char* modeText = isGridView ? "[ Grade ]" : "[ Lista ]";
        vita2d_pgf_draw_text(pgf, layoutX + 18.0f, layoutY + 26.0f, UITheme::TextPrimary, 0.85f, modeText);
    }
}

void UIComponents::drawListCard(float x, float y, float w, float h, bool isSelected, const LibraryItem& item) {
    unsigned int bgColor = isSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(x, y, w, h, 8.0f, bgColor);

    if (isSelected) {
        vita2d_draw_rectangle(x, y + 4.0f, 4.0f, h - 8.0f, UITheme::Primary);
    }

    // Badge
    drawBadge(x + 16.0f, y + (h - 22.0f) / 2.0f, item.typeString);

    // Título
    if (pgf) {
        std::string displayTitle = item.filename;
        if (displayTitle.length() > 40) {
            displayTitle = displayTitle.substr(0, 37) + "...";
        }
        
        unsigned int titleColor = isSelected ? UITheme::TextPrimary : UITheme::TextSecondary;
        vita2d_pgf_draw_text(pgf, x + 82.0f, y + 26.0f, titleColor, 1.0f, displayTitle.c_str());

        // Indicador de favorito
        if (item.isFavorite) {
            drawStar(x + w - 160.0f, y + 16.0f, 16.0f, UITheme::FavoriteGold);
            vita2d_pgf_draw_text(pgf, x + w - 140.0f, y + 26.0f, UITheme::FavoriteGold, 0.8f, "FAV");
        }

        // Tamanho do arquivo
        vita2d_pgf_draw_text(pgf, x + w - 90.0f, y + 26.0f, UITheme::TextSecondary, 0.8f, item.formattedSize.c_str());
    }
}

void UIComponents::drawGridCard(float x, float y, float w, float h, bool isSelected, const LibraryItem& item) {
    unsigned int bgColor = isSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(x, y, w, h, 12.0f, bgColor);

    // Borda de seleção
    if (isSelected) {
        drawRoundedBox(x - 2.0f, y - 2.0f, w + 4.0f, h + 4.0f, 14.0f, UITheme::Primary);
        drawRoundedBox(x, y, w, h, 12.0f, bgColor);
    }

    // Bloco simulando a Capa
    float coverH = h * 0.58f;
    float coverW = w - 16.0f;
    float coverX = x + 8.0f;
    float coverY = y + 8.0f;

    unsigned int coverBg = isSelected ? RGBA8(46, 52, 75, 255) : RGBA8(20, 24, 36, 255);
    drawRoundedBox(coverX, coverY, coverW, coverH, 8.0f, coverBg);

    // Grande Badge no centro da capa
    drawBadge(coverX + (coverW - 64.0f) / 2.0f, coverY + (coverH - 26.0f) / 2.0f, item.typeString, 1.2f);

    // Indicador de Favorito no canto superior direito da capa
    if (item.isFavorite) {
        drawStar(coverX + coverW - 22.0f, coverY + 6.0f, 14.0f, UITheme::FavoriteGold);
    }

    // Título abreviado embaixo
    if (pgf) {
        std::string displayTitle = item.filename;
        if (displayTitle.length() > 22) {
            displayTitle = displayTitle.substr(0, 19) + "...";
        }
        unsigned int titleColor = isSelected ? UITheme::TextPrimary : UITheme::TextSecondary;
        vita2d_pgf_draw_text(pgf, x + 10.0f, y + coverH + 28.0f, titleColor, 0.9f, displayTitle.c_str());

        // Tamanho do arquivo
        vita2d_pgf_draw_text(pgf, x + 10.0f, y + coverH + 50.0f, UITheme::TextSecondary, 0.75f, item.formattedSize.c_str());
    }
}

void UIComponents::drawFooter(const std::string& controlsHint) {
    vita2d_draw_rectangle(0, 504, 960, 40, UITheme::TopBar);
    vita2d_draw_line(0, 504, 960, 504, RGBA8(42, 48, 70, 255));

    if (pgf) {
        vita2d_pgf_draw_text(pgf, 24, 528, UITheme::TextSecondary, 0.85f, controlsHint.c_str());
    }
}
