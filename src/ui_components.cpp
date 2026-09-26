#include "ui_components.h"
#include "config_manager.h"
#include <algorithm>
#include <cstdio>

vita2d_pgf* UIComponents::pgf = nullptr;
vita2d_texture* UIComponents::appIcon = nullptr;

static bool isRotatedUi = false;
static float screenW = screenW;
static float screenH = screenH;

void UIComponents::init(vita2d_pgf* defaultPgf) {
    pgf = defaultPgf;
    if (!appIcon) {
        appIcon = vita2d_load_PNG_file("app0:assets/images/app_icon_small.png");
        if (!appIcon) appIcon = vita2d_load_PNG_file("app0:app_icon_small.png");
        if (!appIcon) appIcon = vita2d_load_PNG_file("assets/images/app_icon_small.png");
        if (!appIcon) appIcon = vita2d_load_PNG_file("app_icon_small.png");
        if (!appIcon) appIcon = vita2d_load_PNG_file("app0:assets/images/app_icon.png");
        if (!appIcon) appIcon = vita2d_load_PNG_file("assets/images/app_icon.png");
    }
}

void UIComponents::shutdown() {
    if (appIcon) {
        vita2d_free_texture(appIcon);
        appIcon = nullptr;
    }
}

void UIComponents::setRotated(bool rotated) {
    isRotatedUi = rotated;
    screenW = rotated ? screenH : screenW;
    screenH = rotated ? screenW : screenH;
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
    vita2d_draw_fill_circle(x + size / 2.0f, y + size / 2.0f, size / 2.2f, color);
}

void UIComponents::drawBadge(float x, float y, const std::string& type, float scale, float alpha) {
    if (alpha <= 0.01f) return;
    unsigned int badgeColor = UITheme::BadgeTXT;
    if (type == "CBZ") badgeColor = UITheme::BadgeCBZ;
    else if (type == "ZIP") badgeColor = UITheme::BadgeZIP;
    else if (type == "CBR") badgeColor = UITheme::BadgeCBR;
    else if (type == "RAR") badgeColor = UITheme::BadgeRAR;
    else if (type == "CBT") badgeColor = UITheme::BadgeCBT;
    else if (type == "TAR") badgeColor = UITheme::BadgeTAR;
    else if (type == "CB7") badgeColor = UITheme::BadgeCB7;
    else if (type == "7Z") badgeColor = UITheme::Badge7Z;
    else if (type == "EPUB") badgeColor = UITheme::BadgeEPUB;

    float w = 54.0f * scale;
    float h = 22.0f * scale;
    drawRoundedBox(x, y, w, h, 5.0f * scale, applyAlpha(badgeColor, alpha));

    if (pgf) {
        vita2d_pgf_draw_text(pgf, x + 8.0f * scale, y + 16.0f * scale, applyAlpha(RGBA8(255, 255, 255, 255), alpha), 0.8f * scale, type.c_str());
    }
}


void UIComponents::drawTopBar(
    const std::string& searchQuery,
    bool isSearchActive,
    SortMode currentSort,
    bool isGridView,
    bool isSearchSelected,
    bool isSortSelected,
    bool isLayoutSelected,
    bool isSettingsSelected,
    bool isRefreshSelected
) {
    // Fundo da Top Bar
    vita2d_draw_rectangle(0, 0, screenW, 68, UITheme::TopBar);
    vita2d_draw_line(0, 68, screenW, 68, RGBA8(42, 48, 70, 255));

    // Logo / Ícone e Nome do App à esquerda
    if (appIcon) {
        float texW = (float)vita2d_texture_get_width(appIcon);
        float texH = (float)vita2d_texture_get_height(appIcon);
        float targetSize = 38.0f;
        float scaleX = targetSize / (texW > 0 ? texW : targetSize);
        float scaleY = targetSize / (texH > 0 ? texH : targetSize);
        vita2d_draw_texture_scale(appIcon, 20.0f, 15.0f, scaleX, scaleY);

        if (pgf) {
            vita2d_pgf_draw_text(pgf, 68, 43, UITheme::TextPrimary, 1.15f, "Bilingual Reader");
        }
    } else if (pgf) {
        vita2d_pgf_draw_text(pgf, 20, 43, UITheme::TextPrimary, 1.15f, "Bilingual Reader");
    }

    // Campo de Busca
    float searchX = isRotatedUi ? 170.0f : 250.0f;
    float searchY = 14.0f;
    float searchW = isRotatedUi ? 120.0f : 240.0f;
    float searchH = 40.0f;

    unsigned int searchBg = isSearchActive ? UITheme::SearchBarActive : (isSearchSelected ? UITheme::SurfaceActive : UITheme::SearchBarBg);
    drawRoundedBox(searchX, searchY, searchW, searchH, 8.0f, searchBg);

    if (isSearchSelected) {
        vita2d_draw_rectangle(searchX, searchY + 4.0f, 3.0f, searchH - 8.0f, UITheme::Primary);
    }

    if (pgf) {
        if (searchQuery.empty()) {
            vita2d_pgf_draw_text(pgf, searchX + 16.0f, searchY + 26.0f, UITheme::TextSecondary, 0.85f, isRotatedUi ? "Buscar..." : "Buscar... [X]");
        } else {
            std::string displayText = searchQuery;
            if (isRotatedUi && displayText.length() > 8) { displayText = displayText.substr(0, 5) + "..."; } else if (!isRotatedUi && displayText.length() > 16) {
                displayText = displayText.substr(0, 13) + "...";
            }
            vita2d_pgf_draw_text(pgf, searchX + 16.0f, searchY + 26.0f, UITheme::TextPrimary, 0.85f, displayText.c_str());
        }
    }

    // Botão de Ordenação
    float sortX = isRotatedUi ? 295.0f : 500.0f;
    float sortY = 14.0f;
    float sortW = isRotatedUi ? 95.0f : 150.0f;
    float sortH = 40.0f;

    unsigned int sortBg = isSortSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(sortX, sortY, sortW, sortH, 8.0f, sortBg);

    if (isSortSelected) {
        vita2d_draw_rectangle(sortX, sortY + 4.0f, 3.0f, sortH - 8.0f, UITheme::Secondary);
    }

    if (pgf) {
        std::string sortLabel = isRotatedUi ? "Ord" : std::string("Ord: ") + FileBrowser::getSortModeName(currentSort);
        vita2d_pgf_draw_text(pgf, sortX + 12.0f, sortY + 26.0f, UITheme::TextPrimary, 0.82f, sortLabel.c_str());
    }

    // Botão de Alternância Lista / Grade
    float layoutX = isRotatedUi ? 395.0f : 660.0f;
    float layoutY = 14.0f;
    float layoutW = isRotatedUi ? 45.0f : 110.0f;
    float layoutH = 40.0f;

    unsigned int layoutBg = isLayoutSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(layoutX, layoutY, layoutW, layoutH, 8.0f, layoutBg);

    if (isLayoutSelected) {
        vita2d_draw_rectangle(layoutX, layoutY + 4.0f, 3.0f, layoutH - 8.0f, UITheme::Primary);
    }

    if (pgf) {
        const char* modeText = isRotatedUi ? (isGridView ? "[G]" : "[L]") : (isGridView ? "[ Grade ]" : "[ Lista ]");
        vita2d_pgf_draw_text(pgf, layoutX + 14.0f, layoutY + 26.0f, UITheme::TextPrimary, 0.85f, modeText);
    }

    // Botão de Configurações
    float cfgX = isRotatedUi ? 445.0f : 780.0f;
    float cfgY = 14.0f;
    float cfgW = isRotatedUi ? 45.0f : 90.0f;
    float cfgH = 40.0f;

    unsigned int cfgBg = isSettingsSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(cfgX, cfgY, cfgW, cfgH, 8.0f, cfgBg);

    if (isSettingsSelected) {
        vita2d_draw_rectangle(cfgX, cfgY + 4.0f, 3.0f, cfgH - 8.0f, UITheme::Primary);
    }

    if (pgf) {
        const char* cfgText = isRotatedUi ? "[C]" : "[ Config ]";
        vita2d_pgf_draw_text(pgf, cfgX + 10.0f, cfgY + 26.0f, UITheme::TextPrimary, 0.85f, cfgText);
    }

    // Botão de Refresh
    float refreshX = isRotatedUi ? 495.0f : 880.0f;
    float refreshY = 14.0f;
    float refreshW = isRotatedUi ? 40.0f : 55.0f;
    float refreshH = 40.0f;

    unsigned int refreshBg = isRefreshSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(refreshX, refreshY, refreshW, refreshH, 8.0f, refreshBg);

    if (isRefreshSelected) {
        vita2d_draw_rectangle(refreshX, refreshY + 4.0f, 3.0f, refreshH - 8.0f, UITheme::Primary);
    }

    if (pgf) {
        vita2d_pgf_draw_text(pgf, refreshX + 14.0f, refreshY + 26.0f, UITheme::TextPrimary, 0.90f, "[R]");
    }
}

void UIComponents::drawConfirmDialog(const std::string& title, const std::string& message, bool isYesSelected) {
    // Backdrop escuro semitransparente
    vita2d_draw_rectangle(0, 0, screenW, screenH, RGBA8(0, 0, 0, 190));

    float dlgW = 480.0f;
    float dlgH = 210.0f;
    float dlgX = (screenW - dlgW) / 2.0f;
    float dlgY = (screenH - dlgH) / 2.0f;

    // Caixa do diálogo
    drawRoundedBox(dlgX, dlgY, dlgW, dlgH, 12.0f, UITheme::Surface);
    vita2d_draw_rectangle(dlgX, dlgY, dlgW, 3.0f, RGBA8(239, 68, 68, 255));

    if (pgf) {
        // Título
        vita2d_pgf_draw_text(pgf, dlgX + 24.0f, dlgY + 42.0f, UITheme::TextPrimary, 1.15f, title.c_str());

        // Mensagem
        std::string displayMsg = message;
        if (displayMsg.length() > 46) {
            displayMsg = displayMsg.substr(0, 43) + "...";
        }
        vita2d_pgf_draw_text(pgf, dlgX + 24.0f, dlgY + 85.0f, UITheme::TextSecondary, 0.90f, displayMsg.c_str());
        vita2d_pgf_draw_text(pgf, dlgX + 24.0f, dlgY + 115.0f, UITheme::TextSecondary, 0.85f, "Esta acao nao pode ser desfeita.");
    }

    // Botão Excluir (Sim)
    float btnY = dlgY + 145.0f;
    float btnW = 195.0f;
    float btnH = 42.0f;
    float btnYesX = dlgX + 24.0f;
    float btnNoX = dlgX + dlgW - 24.0f - btnW;

    unsigned int yesBg = isYesSelected ? RGBA8(220, 38, 38, 255) : RGBA8(50, 25, 25, 255);
    drawRoundedBox(btnYesX, btnY, btnW, btnH, 8.0f, yesBg);
    if (isYesSelected) {
        vita2d_draw_rectangle(btnYesX, btnY + 4.0f, 3.0f, btnH - 8.0f, RGBA8(255, 255, 255, 255));
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, btnYesX + 45.0f, btnY + 28.0f, UITheme::TextPrimary, 0.95f, "Excluir (X)");
    }

    // Botão Cancelar (Não)
    unsigned int noBg = !isYesSelected ? UITheme::Primary : RGBA8(40, 46, 68, 255);
    drawRoundedBox(btnNoX, btnY, btnW, btnH, 8.0f, noBg);
    if (!isYesSelected) {
        vita2d_draw_rectangle(btnNoX, btnY + 4.0f, 3.0f, btnH - 8.0f, RGBA8(255, 255, 255, 255));
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, btnNoX + 40.0f, btnY + 28.0f, UITheme::TextPrimary, 0.95f, "Cancelar (O)");
    }
}

void UIComponents::drawProgressPopup(const std::string& title, const std::string& message, float progress) {
    // Backdrop escuro semitransparente
    vita2d_draw_rectangle(0, 0, screenW, screenH, RGBA8(0, 0, 0, 190));

    float dlgW = 480.0f;
    float dlgH = 190.0f;
    float dlgX = (screenW - dlgW) / 2.0f;
    float dlgY = (screenH - dlgH) / 2.0f;

    // Caixa do diálogo
    drawRoundedBox(dlgX, dlgY, dlgW, dlgH, 12.0f, UITheme::Surface);
    vita2d_draw_rectangle(dlgX, dlgY, dlgW, 3.0f, UITheme::Primary);

    if (pgf) {
        // Título
        vita2d_pgf_draw_text(pgf, dlgX + 24.0f, dlgY + 42.0f, UITheme::TextPrimary, 1.15f, title.c_str());

        // Mensagem
        std::string displayMsg = message;
        if (displayMsg.length() > 46) {
            displayMsg = displayMsg.substr(0, 43) + "...";
        }
        vita2d_pgf_draw_text(pgf, dlgX + 24.0f, dlgY + 80.0f, UITheme::TextSecondary, 0.90f, displayMsg.c_str());
    }

    // Barra de progresso
    float barX = dlgX + 24.0f;
    float barY = dlgY + 115.0f;
    float barW = dlgW - 48.0f;
    float barH = 12.0f;

    drawRoundedBox(barX, barY, barW, barH, 6.0f, RGBA8(42, 48, 70, 255));
    if (progress >= 0.0f) {
        float p = std::max(0.0f, std::min(1.0f, progress));
        drawRoundedBox(barX, barY, barW * p, barH, 6.0f, UITheme::Primary);
    } else {
        // Indicador de progresso pulsante quando indeterminado
        static float pulse = 0.0f;
        pulse += 0.04f;
        if (pulse > 1.0f) pulse = 0.0f;
        float pulseW = barW * 0.35f;
        float pulseX = barX + (barW - pulseW) * pulse;
        drawRoundedBox(pulseX, barY, pulseW, barH, 6.0f, UITheme::Primary);
    }

    if (pgf && progress >= 0.0f) {
        char percentStr[16];
        snprintf(percentStr, sizeof(percentStr), "%d%%", static_cast<int>(progress * 100.0f));
        vita2d_pgf_draw_text(pgf, dlgX + dlgW - 70.0f, dlgY + 155.0f, UITheme::TextSecondary, 0.85f, percentStr);
    }
}

void UIComponents::drawListCard(float x, float y, float w, float h, bool isSelected, const LibraryItem& item, float alpha) {
    if (alpha <= 0.01f) return;

    unsigned int bgColor = isSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(x, y, w, h, 8.0f, applyAlpha(bgColor, alpha));

    if (isSelected) {
        vita2d_draw_rectangle(x, y + 4.0f, 4.0f, h - 8.0f, applyAlpha(UITheme::Primary, alpha));
    }

    // Badge
    drawBadge(x + 16.0f, y + (h - 22.0f) / 2.0f, item.typeString, 1.0f, alpha);

    // Título
    if (pgf) {
        std::string displayTitle = item.filename;
        if (displayTitle.length() > 40) {
            displayTitle = displayTitle.substr(0, 37) + "...";
        }
        
        unsigned int titleColor = isSelected ? UITheme::TextPrimary : UITheme::TextSecondary;
        vita2d_pgf_draw_text(pgf, x + 82.0f, y + 26.0f, applyAlpha(titleColor, alpha), 1.0f, displayTitle.c_str());

        // Indicador de favorito
        if (item.isFavorite) {
            drawStar(x + w - 160.0f, y + 16.0f, 16.0f, applyAlpha(UITheme::FavoriteGold, alpha));
            vita2d_pgf_draw_text(pgf, x + w - 140.0f, y + 26.0f, applyAlpha(UITheme::FavoriteGold, alpha), 0.8f, "FAV");
        }

        // Tamanho do arquivo
        vita2d_pgf_draw_text(pgf, x + w - 90.0f, y + 26.0f, applyAlpha(UITheme::TextSecondary, alpha), 0.8f, item.formattedSize.c_str());
    }
}

void UIComponents::drawGridCard(float x, float y, float w, float h, bool isSelected, const LibraryItem& item, float alpha) {
    if (alpha <= 0.01f) return;

    unsigned int bgColor = isSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(x, y, w, h, 12.0f, applyAlpha(bgColor, alpha));

    // Borda de seleção
    if (isSelected) {
        drawRoundedBox(x - 2.0f, y - 2.0f, w + 4.0f, h + 4.0f, 14.0f, applyAlpha(UITheme::Primary, alpha));
        drawRoundedBox(x, y, w, h, 12.0f, applyAlpha(bgColor, alpha));
    }

    // Bloco simulando a Capa
    float coverH = h * 0.58f;
    float coverW = w - 16.0f;
    float coverX = x + 8.0f;
    float coverY = y + 8.0f;

    unsigned int coverBg = isSelected ? RGBA8(46, 52, 75, 255) : RGBA8(20, 24, 36, 255);
    drawRoundedBox(coverX, coverY, coverW, coverH, 8.0f, applyAlpha(coverBg, alpha));

    // Grande Badge no centro da capa
    drawBadge(coverX + (coverW - 64.0f) / 2.0f, coverY + (coverH - 26.0f) / 2.0f, item.typeString, 1.2f, alpha);

    // Indicador de Favorito no canto superior direito da capa
    if (item.isFavorite) {
        drawStar(coverX + coverW - 22.0f, coverY + 6.0f, 14.0f, applyAlpha(UITheme::FavoriteGold, alpha));
    }

    // Título abreviado embaixo
    if (pgf) {
        std::string displayTitle = item.filename;
        if (displayTitle.length() > 22) {
            displayTitle = displayTitle.substr(0, 19) + "...";
        }
        unsigned int titleColor = isSelected ? UITheme::TextPrimary : UITheme::TextSecondary;
        vita2d_pgf_draw_text(pgf, x + 10.0f, y + coverH + 28.0f, applyAlpha(titleColor, alpha), 0.9f, displayTitle.c_str());

        // Tamanho do arquivo
        vita2d_pgf_draw_text(pgf, x + 10.0f, y + coverH + 50.0f, applyAlpha(UITheme::TextSecondary, alpha), 0.75f, item.formattedSize.c_str());
    }
}

void UIComponents::drawLibraryScrollBar(float currentOffset, int totalItems, int visibleCapacity, bool isGridView, float alpha) {
    if (totalItems <= visibleCapacity || totalItems <= 0) return;
    if (alpha <= 0.01f) return;

    float trackX = screenW - 12.0f;
    float trackY = 80.0f;
    float trackH = 412.0f;
    float trackW = 5.0f;

    // Fundo do trilho da scrollbar
    drawRoundedBox(trackX, trackY, trackW, trackH, 2.5f, applyAlpha(RGBA8(255, 255, 255, 35), alpha));

    // Altura do thumb proporcional ao número de itens visíveis
    float maxScroll = static_cast<float>(totalItems - visibleCapacity);
    if (maxScroll < 1.0f) maxScroll = 1.0f;

    float thumbRatio = static_cast<float>(visibleCapacity) / static_cast<float>(totalItems);
    float thumbH = std::max(32.0f, trackH * thumbRatio);
    float availableTravel = trackH - thumbH;

    float clampedOffset = std::max(0.0f, std::min(currentOffset, maxScroll));
    float thumbY = trackY + (clampedOffset / maxScroll) * availableTravel;

    // Desenha o thumb
    drawRoundedBox(trackX, thumbY, trackW, thumbH, 2.5f, applyAlpha(UITheme::Primary, alpha));
}


void UIComponents::drawReaderTopBar(const std::string& title, const std::string& extraInfo, bool isRotated, unsigned int badgeColor) {
    vita2d_draw_rectangle(0, 0, screenW, 48, UITheme::TopBar);
    vita2d_draw_line(0, 48, screenW, 48, RGBA8(42, 48, 70, 255));

    if (pgf) {
        std::string displayTitle = title;
        if (displayTitle.length() > 34) {
            displayTitle = displayTitle.substr(0, 31) + "...";
        }
        vita2d_pgf_draw_text(pgf, 24.0f, 32.0f, badgeColor, 1.0f, displayTitle.c_str());

        if (!extraInfo.empty()) {
            vita2d_pgf_draw_text(pgf, 580.0f, 32.0f, UITheme::TextSecondary, 0.85f, extraInfo.c_str());
        }
    }

    // Botão de Rotação de Tela no canto superior direito (X: 840 -> 940, Y: 8 -> 40)
    float rotBtnX = isRotatedUi ? screenW - 120.0f : 840.0f;
    float rotBtnY = 8.0f;
    float rotBtnW = 100.0f;
    float rotBtnH = 32.0f;
    unsigned int rotBg = isRotated ? UITheme::Primary : UITheme::Surface;
    drawRoundedBox(rotBtnX, rotBtnY, rotBtnW, rotBtnH, 6.0f, rotBg);

    if (pgf) {
        const char* rotText = isRotated ? "[*] 90 Deg" : "[ ] Normal";
        vita2d_pgf_draw_text(pgf, rotBtnX + 10.0f, rotBtnY + 22.0f, UITheme::TextPrimary, 0.80f, rotText);
    }
}

void UIComponents::drawFooter(const std::string& controlsHint, bool showBookNav) {
    vita2d_draw_rectangle(0, 504, screenW, 40, UITheme::TopBar);
    vita2d_draw_line(0, 504, screenW, 504, RGBA8(42, 48, 70, 255));

    if (pgf) {
        vita2d_pgf_draw_text(pgf, 24, 528, UITheme::TextSecondary, 0.85f, controlsHint.c_str());
    }

    if (showBookNav) {
        // Botão [L] Livro Ant (X: 680..790, Y: 508..538)
        float btnAntX = isRotatedUi ? screenW - 240.0f : 680.0f;
        float btnAntY = 508.0f;
        float btnW = 110.0f;
        float btnH = 32.0f;
        drawRoundedBox(btnAntX, btnAntY, btnW, btnH, 6.0f, UITheme::Surface);
        if (pgf) {
            vita2d_pgf_draw_text(pgf, btnAntX + 10.0f, btnAntY + 22.0f, UITheme::TextPrimary, 0.78f, "[L] Livro Ant");
        }

        // Botão [R] Prox Livro (X: 810..920, Y: 508..538)
        float btnProxX = isRotatedUi ? screenW - 120.0f : 810.0f;
        float btnProxY = 508.0f;
        drawRoundedBox(btnProxX, btnProxY, btnW, btnH, 6.0f, UITheme::Surface);
        if (pgf) {
            vita2d_pgf_draw_text(pgf, btnProxX + 10.0f, btnProxY + 22.0f, UITheme::TextPrimary, 0.78f, "[R] Prox Livro");
        }
    }
}

void UIComponents::drawReaderProgressBar(int currentPage, int totalPages, bool isHudVisible) {
    if (totalPages <= 0) return;

    float progressRatio = static_cast<float>(currentPage + 1) / static_cast<float>(totalPages);
    if (progressRatio > 1.0f) progressRatio = 1.0f;
    if (progressRatio < 0.0f) progressRatio = 0.0f;

    // Se o HUD estiver visível, desenha uma barra completa com informações no rodapé
    if (isHudVisible) {
        float hudHeight = 44.0f;
        float hudY = screenH - hudHeight;

        // Fundo semitransparente escuro
        vita2d_draw_rectangle(0, hudY, screenW, hudHeight, UITheme::ProgressBarBg);
        vita2d_draw_line(0, hudY, screenW, hudY, RGBA8(60, 66, 92, 180));

        // Barra de progresso horizontal
        float barX = 140.0f;
        float barY = hudY + 18.0f;
        float barW = 680.0f;
        float barH = 8.0f;

        drawRoundedBox(barX, barY, barW, barH, 4.0f, RGBA8(50, 56, 78, 255));
        if (progressRatio > 0.0f) {
            drawRoundedBox(barX, barY, barW * progressRatio, barH, 4.0f, UITheme::ProgressBarFill);
            // Ponto indicador do slider
            vita2d_draw_fill_circle(barX + (barW * progressRatio), barY + (barH / 2.0f), 7.0f, UITheme::TextPrimary);
        }

        if (pgf) {
            // Texto da página (ex: "1 / 45")
            char pageStr[32];
            snprintf(pageStr, sizeof(pageStr), "%d / %d", currentPage + 1, totalPages);
            vita2d_pgf_draw_text(pgf, 20.0f, hudY + 28.0f, UITheme::TextPrimary, 0.85f, pageStr);

            // Porcentagem
            char percentStr[16];
            snprintf(percentStr, sizeof(percentStr), "%d%%", static_cast<int>(progressRatio * 100.0f));
            vita2d_pgf_draw_text(pgf, 840.0f, hudY + 28.0f, UITheme::TextSecondary, 0.85f, percentStr);
        }
    } else {
        // Barra discreta e fina no fundo da tela
        float barW = screenW * progressRatio;
        vita2d_draw_rectangle(0, 540, screenW, 4, RGBA8(0, 0, 0, 100));
        if (barW > 0.0f) {
            vita2d_draw_rectangle(0, 540, barW, 4, UITheme::ProgressBarFill);
        }
    }
}

void UIComponents::drawSettingsScreen(int selectedItemIndex, const AppConfig& config) {
    // Top Bar de Configurações
    vita2d_draw_rectangle(0, 0, screenW, 68, UITheme::TopBar);
    vita2d_draw_line(0, 68, screenW, 68, RGBA8(42, 48, 70, 255));

    if (pgf) {
        vita2d_pgf_draw_text(pgf, 32.0f, 44.0f, UITheme::TextPrimary, 1.2f, "Configurações");
    }

    float startY = 88.0f;
    float boxW = screenW - 64.0f; // 896px
    float rowH = 46.0f;

    // --- SEÇÃO 1: BIBLIOTECA ---
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 40.0f, startY + 16.0f, UITheme::Secondary, 0.9f, "BIBLIOTECA");
    }
    float sec1Y = startY + 26.0f;
    float sec1H = rowH * 2.0f; // 2 itens: Modo de Exibição, Ordenação Padrão
    drawRoundedBox(32.0f, sec1Y, boxW, sec1H, 10.0f, UITheme::Surface);

    // Linha divisória interna da seção 1
    vita2d_draw_line(36.0f, sec1Y + rowH, 32.0f + boxW - 4.0f, sec1Y + rowH, RGBA8(42, 48, 70, 255));

    // Item 0: Modo de Exibição (Lista / Grade)
    if (selectedItemIndex == 0) {
        drawRoundedBox(34.0f, sec1Y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, sec1Y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, sec1Y + 30.0f, UITheme::TextPrimary, 0.95f, "Modo de Exibição Padrão");
        const char* viewModeStr = config.isGridView ? "< Grade >" : "< Lista >";
        vita2d_pgf_draw_text(pgf, boxW - 140.0f, sec1Y + 30.0f, UITheme::Primary, 0.95f, viewModeStr);
    }

    // Item 1: Ordenação Padrão
    float row1Y = sec1Y + rowH;
    if (selectedItemIndex == 1) {
        drawRoundedBox(34.0f, row1Y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, row1Y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, row1Y + 30.0f, UITheme::TextPrimary, 0.95f, "Ordenação Padrão");
        std::string sortStr = std::string("< ") + FileBrowser::getSortModeName(config.sortMode) + " >";
        vita2d_pgf_draw_text(pgf, boxW - 180.0f, row1Y + 30.0f, UITheme::Primary, 0.95f, sortStr.c_str());
    }

    // --- SEÇÃO 2: LEITOR & VISUALIZAÇÃO ---
    float sec2StartY = sec1Y + sec1H + 24.0f;
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 40.0f, sec2StartY + 16.0f, UITheme::Secondary, 0.9f, "LEITOR & VISUALIZAÇÃO");
    }
    float sec2Y = sec2StartY + 26.0f;
    float sec2H = rowH * 3.0f; // 3 itens: Orientação, Números de Página, Tamanho da Fonte EPUB
    drawRoundedBox(32.0f, sec2Y, boxW, sec2H, 10.0f, UITheme::Surface);

    // Linhas divisórias internas da seção 2
    vita2d_draw_line(36.0f, sec2Y + rowH, 32.0f + boxW - 4.0f, sec2Y + rowH, RGBA8(42, 48, 70, 255));
    vita2d_draw_line(36.0f, sec2Y + rowH * 2.0f, 32.0f + boxW - 4.0f, sec2Y + rowH * 2.0f, RGBA8(42, 48, 70, 255));

    // Item 2: Orientação do Leitor
    if (selectedItemIndex == 2) {
        drawRoundedBox(34.0f, sec2Y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, sec2Y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, sec2Y + 30.0f, UITheme::TextPrimary, 0.95f, "Orientação Inicial do Leitor");
        const char* orientStr = config.readerRotated ? "< Vertical (Girar 90°) >" : "< Horizontal (Padrão) >";
        vita2d_pgf_draw_text(pgf, boxW - 250.0f, sec2Y + 30.0f, UITheme::Primary, 0.95f, orientStr);
    }

    // Item 3: Exibir Número de Páginas
    float row3Y = sec2Y + rowH;
    if (selectedItemIndex == 3) {
        drawRoundedBox(34.0f, row3Y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, row3Y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, row3Y + 30.0f, UITheme::TextPrimary, 0.95f, "Exibir Barra de Progresso e Páginas");
        const char* pagesStr = config.showPageNumbers ? "< Sim >" : "< Não >";
        vita2d_pgf_draw_text(pgf, boxW - 140.0f, row3Y + 30.0f, UITheme::Primary, 0.95f, pagesStr);
    }

    // Item 4: Tamanho da Fonte Padrão (EPUB/TXT)
    float row4Y = sec2Y + rowH * 2.0f;
    if (selectedItemIndex == 4) {
        drawRoundedBox(34.0f, row4Y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, row4Y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, row4Y + 30.0f, UITheme::TextPrimary, 0.95f, "Tamanho de Fonte Padrão (EPUB)");
        char fontStr[32];
        snprintf(fontStr, sizeof(fontStr), "< %d px >", config.epubFontSize);
        vita2d_pgf_draw_text(pgf, boxW - 140.0f, row4Y + 30.0f, UITheme::Primary, 0.95f, fontStr);
    }

    // Rodapé de Navegação
    drawFooter("D-Pad Cima/Baixo: Selecionar | D-Pad Esq/Dir: Alterar | O: Voltar e Salvar");
}
