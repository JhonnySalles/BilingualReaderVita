#include "ui_components.h"
#include "config_manager.h"
#include <algorithm>
#include <cstdio>

vita2d_pgf* UIComponents::pgf = nullptr;
vita2d_texture* UIComponents::appIcon = nullptr;

static bool isRotatedUi = false;
static float screenW = 960.0f;
static float screenH = 544.0f;

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
    screenW = rotated ? 544.0f : 960.0f;
    screenH = rotated ? 960.0f : 544.0f;
}

bool UIComponents::isRotated() {
    return isRotatedUi;
}

float UIComponents::getScreenW() {
    return screenW;
}

float UIComponents::getScreenH() {
    return screenH;
}

UITopBarLayout UIComponents::getTopBarLayout() {
    UITopBarLayout layout;
    layout.search = {isRotatedUi ? 180.0f : 250.0f, 14.0f, isRotatedUi ? 130.0f : 240.0f, 40.0f};
    layout.sort = {isRotatedUi ? 320.0f : 500.0f, 14.0f, isRotatedUi ? 60.0f : 150.0f, 40.0f};
    layout.layout = {isRotatedUi ? 390.0f : 660.0f, 14.0f, isRotatedUi ? 40.0f : 110.0f, 40.0f};
    layout.settings = {isRotatedUi ? 440.0f : 780.0f, 14.0f, isRotatedUi ? 40.0f : 90.0f, 40.0f};
    layout.refresh = {isRotatedUi ? 490.0f : 880.0f, 14.0f, isRotatedUi ? 40.0f : 55.0f, 40.0f};
    return layout;
}

UIReaderChromeLayout UIComponents::getReaderChromeLayout() {
    UIReaderChromeLayout layout;
    layout.screenW = screenW;
    layout.screenH = screenH;
    layout.contentTop = 48.0f;
    layout.contentBottom = screenH - 40.0f;
    layout.rotateBtn = {isRotatedUi ? (screenW - 120.0f) : 840.0f, 8.0f, 100.0f, 32.0f};
    layout.prevBook = {isRotatedUi ? (screenW - 240.0f) : 680.0f, screenH - 36.0f, 110.0f, 32.0f};
    layout.nextBook = {isRotatedUi ? (screenW - 120.0f) : 810.0f, screenH - 36.0f, 110.0f, 32.0f};
    return layout;
}

UIConfirmDialogLayout UIComponents::getConfirmDialogLayout() {
    UIConfirmDialogLayout layout;
    float dlgW = 480.0f;
    float dlgH = 210.0f;
    float dlgX = (screenW - dlgW) / 2.0f;
    float dlgY = (screenH - dlgH) / 2.0f;
    float btnY = dlgY + 145.0f;
    float btnW = 195.0f;
    float btnH = 42.0f;
    layout.dialog = {dlgX, dlgY, dlgW, dlgH};
    layout.yesBtn = {dlgX + 24.0f, btnY, btnW, btnH};
    layout.noBtn = {dlgX + dlgW - 24.0f - btnW, btnY, btnW, btnH};
    return layout;
}

UISettingsLayout UIComponents::getSettingsLayout() {
    UISettingsLayout layout;
    float startY = 88.0f;
    float rowH = 46.0f;
    float boxW = screenW - 64.0f;
    float sec1Y = startY + 26.0f;
    float sec2StartY = sec1Y + rowH * 2.0f + 24.0f;
    float sec2Y = sec2StartY + 26.0f;

    layout.item0 = {32.0f, sec1Y, boxW, rowH};
    layout.item1 = {32.0f, sec1Y + rowH, boxW, rowH};
    layout.item2 = {32.0f, sec2Y, boxW, rowH};
    layout.item3 = {32.0f, sec2Y + rowH, boxW, rowH};
    layout.item4 = {32.0f, sec2Y + rowH * 2.0f, boxW, rowH};
    layout.fontValueSplitX = 32.0f + boxW * 0.55f;
    return layout;
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

    UITopBarLayout bar = getTopBarLayout();

    // Campo de Busca
    unsigned int searchBg = isSearchActive ? UITheme::SearchBarActive : (isSearchSelected ? UITheme::SurfaceActive : UITheme::SearchBarBg);
    drawRoundedBox(bar.search.x, bar.search.y, bar.search.w, bar.search.h, 8.0f, searchBg);

    if (isSearchSelected) {
        vita2d_draw_rectangle(bar.search.x, bar.search.y + 4.0f, 3.0f, bar.search.h - 8.0f, UITheme::Primary);
    }

    if (pgf) {
        if (searchQuery.empty()) {
            vita2d_pgf_draw_text(pgf, bar.search.x + 16.0f, bar.search.y + 26.0f, UITheme::TextSecondary, 0.85f, isRotatedUi ? "Buscar..." : "Buscar... [X]");
        } else {
            std::string displayText = searchQuery;
            if (isRotatedUi && displayText.length() > 8) { displayText = displayText.substr(0, 5) + "..."; } else if (!isRotatedUi && displayText.length() > 16) {
                displayText = displayText.substr(0, 13) + "...";
            }
            vita2d_pgf_draw_text(pgf, bar.search.x + 16.0f, bar.search.y + 26.0f, UITheme::TextPrimary, 0.85f, displayText.c_str());
        }
    }

    // Botão de Ordenação
    unsigned int sortBg = isSortSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(bar.sort.x, bar.sort.y, bar.sort.w, bar.sort.h, 8.0f, sortBg);

    if (isSortSelected) {
        vita2d_draw_rectangle(bar.sort.x, bar.sort.y + 4.0f, 3.0f, bar.sort.h - 8.0f, UITheme::Secondary);
    }

    if (pgf) {
        std::string sortLabel = isRotatedUi ? "Ord" : std::string("Ord: ") + FileBrowser::getSortModeName(currentSort);
        vita2d_pgf_draw_text(pgf, bar.sort.x + 12.0f, bar.sort.y + 26.0f, UITheme::TextPrimary, 0.82f, sortLabel.c_str());
    }

    // Botão de Alternância Lista / Grade
    unsigned int layoutBg = isLayoutSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(bar.layout.x, bar.layout.y, bar.layout.w, bar.layout.h, 8.0f, layoutBg);

    if (isLayoutSelected) {
        vita2d_draw_rectangle(bar.layout.x, bar.layout.y + 4.0f, 3.0f, bar.layout.h - 8.0f, UITheme::Primary);
    }

    if (pgf) {
        const char* modeText = isRotatedUi ? (isGridView ? "[G]" : "[L]") : (isGridView ? "[ Grade ]" : "[ Lista ]");
        vita2d_pgf_draw_text(pgf, bar.layout.x + 14.0f, bar.layout.y + 26.0f, UITheme::TextPrimary, 0.85f, modeText);
    }

    // Botão de Configurações
    unsigned int cfgBg = isSettingsSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(bar.settings.x, bar.settings.y, bar.settings.w, bar.settings.h, 8.0f, cfgBg);

    if (isSettingsSelected) {
        vita2d_draw_rectangle(bar.settings.x, bar.settings.y + 4.0f, 3.0f, bar.settings.h - 8.0f, UITheme::Primary);
    }

    if (pgf) {
        const char* cfgText = isRotatedUi ? "[C]" : "[ Config ]";
        vita2d_pgf_draw_text(pgf, bar.settings.x + 10.0f, bar.settings.y + 26.0f, UITheme::TextPrimary, 0.85f, cfgText);
    }

    // Botão de Refresh
    unsigned int refreshBg = isRefreshSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(bar.refresh.x, bar.refresh.y, bar.refresh.w, bar.refresh.h, 8.0f, refreshBg);

    if (isRefreshSelected) {
        vita2d_draw_rectangle(bar.refresh.x, bar.refresh.y + 4.0f, 3.0f, bar.refresh.h - 8.0f, UITheme::Primary);
    }

    if (pgf) {
        vita2d_pgf_draw_text(pgf, bar.refresh.x + 14.0f, bar.refresh.y + 26.0f, UITheme::TextPrimary, 0.90f, "[R]");
    }
}

void UIComponents::drawConfirmDialog(const std::string& title, const std::string& message, bool isYesSelected) {
    // Backdrop escuro semitransparente
    vita2d_draw_rectangle(0, 0, screenW, screenH, RGBA8(0, 0, 0, 190));

    UIConfirmDialogLayout dlg = getConfirmDialogLayout();

    // Caixa do diálogo
    drawRoundedBox(dlg.dialog.x, dlg.dialog.y, dlg.dialog.w, dlg.dialog.h, 12.0f, UITheme::Surface);
    vita2d_draw_rectangle(dlg.dialog.x, dlg.dialog.y, dlg.dialog.w, 3.0f, RGBA8(239, 68, 68, 255));

    if (pgf) {
        // Título
        vita2d_pgf_draw_text(pgf, dlg.dialog.x + 24.0f, dlg.dialog.y + 42.0f, UITheme::TextPrimary, 1.15f, title.c_str());

        // Mensagem
        std::string displayMsg = message;
        if (displayMsg.length() > 46) {
            displayMsg = displayMsg.substr(0, 43) + "...";
        }
        vita2d_pgf_draw_text(pgf, dlg.dialog.x + 24.0f, dlg.dialog.y + 85.0f, UITheme::TextSecondary, 0.90f, displayMsg.c_str());
        vita2d_pgf_draw_text(pgf, dlg.dialog.x + 24.0f, dlg.dialog.y + 115.0f, UITheme::TextSecondary, 0.85f, "Esta acao nao pode ser desfeita.");
    }

    unsigned int yesBg = isYesSelected ? RGBA8(220, 38, 38, 255) : RGBA8(50, 25, 25, 255);
    drawRoundedBox(dlg.yesBtn.x, dlg.yesBtn.y, dlg.yesBtn.w, dlg.yesBtn.h, 8.0f, yesBg);
    if (isYesSelected) {
        vita2d_draw_rectangle(dlg.yesBtn.x, dlg.yesBtn.y + 4.0f, 3.0f, dlg.yesBtn.h - 8.0f, RGBA8(255, 255, 255, 255));
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, dlg.yesBtn.x + 45.0f, dlg.yesBtn.y + 28.0f, UITheme::TextPrimary, 0.95f, "Excluir (X)");
    }

    unsigned int noBg = !isYesSelected ? UITheme::Primary : RGBA8(40, 46, 68, 255);
    drawRoundedBox(dlg.noBtn.x, dlg.noBtn.y, dlg.noBtn.w, dlg.noBtn.h, 8.0f, noBg);
    if (!isYesSelected) {
        vita2d_draw_rectangle(dlg.noBtn.x, dlg.noBtn.y + 4.0f, 3.0f, dlg.noBtn.h - 8.0f, RGBA8(255, 255, 255, 255));
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, dlg.noBtn.x + 40.0f, dlg.noBtn.y + 28.0f, UITheme::TextPrimary, 0.95f, "Cancelar (O)");
    }
}

void UIComponents::drawProgressPopup(const std::string& title, const std::string& message, float progress) {
    // Override dimension for this popup since it is drawn directly to the physical screen
    float physW = 960.0f;
    float physH = 544.0f;

    // Backdrop escuro semitransparente
    vita2d_draw_rectangle(0, 0, physW, physH, RGBA8(0, 0, 0, 190));

    float dlgW = 480.0f;
    float dlgH = 190.0f;
    float dlgX = (physW - dlgW) / 2.0f;
    float dlgY = (physH - dlgH) / 2.0f;

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
    float trackH = screenH - 132.0f;
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
    UIReaderChromeLayout chrome = getReaderChromeLayout();
    vita2d_draw_rectangle(0, 0, screenW, chrome.contentTop, UITheme::TopBar);
    vita2d_draw_line(0, chrome.contentTop, screenW, chrome.contentTop, RGBA8(42, 48, 70, 255));

    if (pgf) {
        std::string displayTitle = title;
        size_t maxTitleLen = isRotatedUi ? 18 : 34;
        if (displayTitle.length() > maxTitleLen) {
            displayTitle = displayTitle.substr(0, maxTitleLen - 3) + "...";
        }
        vita2d_pgf_draw_text(pgf, 24.0f, 32.0f, badgeColor, 1.0f, displayTitle.c_str());

        if (!extraInfo.empty()) {
            float infoX = chrome.rotateBtn.x - 12.0f;
            int infoW = vita2d_pgf_text_width(pgf, 0.85f, extraInfo.c_str());
            infoX -= static_cast<float>(infoW);
            if (infoX < 24.0f) infoX = 24.0f;
            vita2d_pgf_draw_text(pgf, infoX, 32.0f, UITheme::TextSecondary, 0.85f, extraInfo.c_str());
        }
    }

    unsigned int rotBg = isRotated ? UITheme::Primary : UITheme::Surface;
    drawRoundedBox(chrome.rotateBtn.x, chrome.rotateBtn.y, chrome.rotateBtn.w, chrome.rotateBtn.h, 6.0f, rotBg);

    if (pgf) {
        const char* rotText = isRotated ? "[*] 90 Deg" : "[ ] Normal";
        vita2d_pgf_draw_text(pgf, chrome.rotateBtn.x + 10.0f, chrome.rotateBtn.y + 22.0f, UITheme::TextPrimary, 0.80f, rotText);
    }
}

void UIComponents::drawFooter(const std::string& controlsHint, bool showBookNav) {
    UIReaderChromeLayout chrome = getReaderChromeLayout();
    float footerY = chrome.contentBottom;
    vita2d_draw_rectangle(0, footerY, screenW, 40.0f, UITheme::TopBar);
    vita2d_draw_line(0, footerY, screenW, footerY, RGBA8(42, 48, 70, 255));

    if (pgf) {
        vita2d_pgf_draw_text(pgf, 24.0f, footerY + 24.0f, UITheme::TextSecondary, 0.85f, controlsHint.c_str());
    }

    if (showBookNav) {
        drawRoundedBox(chrome.prevBook.x, chrome.prevBook.y, chrome.prevBook.w, chrome.prevBook.h, 6.0f, UITheme::Surface);
        if (pgf) {
            vita2d_pgf_draw_text(pgf, chrome.prevBook.x + 10.0f, chrome.prevBook.y + 22.0f, UITheme::TextPrimary, 0.78f, "[L] Livro Ant");
        }

        drawRoundedBox(chrome.nextBook.x, chrome.nextBook.y, chrome.nextBook.w, chrome.nextBook.h, 6.0f, UITheme::Surface);
        if (pgf) {
            vita2d_pgf_draw_text(pgf, chrome.nextBook.x + 10.0f, chrome.nextBook.y + 22.0f, UITheme::TextPrimary, 0.78f, "[R] Prox Livro");
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
        float barX = isRotatedUi ? 90.0f : 140.0f;
        float barY = hudY + 18.0f;
        float barW = isRotatedUi ? (screenW - 180.0f) : 680.0f;
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
            vita2d_pgf_draw_text(pgf, 12.0f, hudY + 28.0f, UITheme::TextPrimary, 0.85f, pageStr);

            // Porcentagem
            char percentStr[16];
            snprintf(percentStr, sizeof(percentStr), "%d%%", static_cast<int>(progressRatio * 100.0f));
            int percentW = vita2d_pgf_text_width(pgf, 0.85f, percentStr);
            vita2d_pgf_draw_text(pgf, screenW - 12.0f - static_cast<float>(percentW), hudY + 28.0f, UITheme::TextSecondary, 0.85f, percentStr);
        }
    } else {
        // Barra discreta e fina no fundo da tela
        float barY = screenH - 4.0f;
        float barW = screenW * progressRatio;
        vita2d_draw_rectangle(0, barY, screenW, 4.0f, RGBA8(0, 0, 0, 100));
        if (barW > 0.0f) {
            vita2d_draw_rectangle(0, barY, barW, 4.0f, UITheme::ProgressBarFill);
        }
    }
}

void UIComponents::drawSettingsScreen(int selectedItemIndex, const AppConfig& config) {
    UISettingsLayout layout = getSettingsLayout();
    float boxW = layout.item0.w;
    float rowH = layout.item0.h;
    float sec1Y = layout.item0.y;
    float sec2Y = layout.item2.y;

    // Top Bar de Configurações
    vita2d_draw_rectangle(0, 0, screenW, 68, UITheme::TopBar);
    vita2d_draw_line(0, 68, screenW, 68, RGBA8(42, 48, 70, 255));

    if (pgf) {
        vita2d_pgf_draw_text(pgf, 32.0f, 44.0f, UITheme::TextPrimary, 1.2f, "Configurações");
    }

    // --- SEÇÃO 1: BIBLIOTECA ---
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 40.0f, sec1Y - 10.0f, UITheme::Secondary, 0.9f, "BIBLIOTECA");
    }
    float sec1H = rowH * 2.0f;
    drawRoundedBox(32.0f, sec1Y, boxW, sec1H, 10.0f, UITheme::Surface);

    // Linha divisória interna da seção 1
    vita2d_draw_line(36.0f, sec1Y + rowH, 32.0f + boxW - 4.0f, sec1Y + rowH, RGBA8(42, 48, 70, 255));

    auto drawValueRight = [&](float y, const char* value) {
        if (!pgf || !value) return;
        int valueW = vita2d_pgf_text_width(pgf, 0.95f, value);
        float valueX = 32.0f + boxW - 16.0f - static_cast<float>(valueW);
        if (valueX < layout.fontValueSplitX) valueX = layout.fontValueSplitX;
        vita2d_pgf_draw_text(pgf, valueX, y, UITheme::Primary, 0.95f, value);
    };

    // Item 0: Modo de Exibição (Lista / Grade)
    if (selectedItemIndex == 0) {
        drawRoundedBox(34.0f, layout.item0.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item0.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item0.y + 30.0f, UITheme::TextPrimary, 0.95f,
                             isRotatedUi ? "Modo de Exibição" : "Modo de Exibição Padrão");
        drawValueRight(layout.item0.y + 30.0f, config.isGridView ? "< Grade >" : "< Lista >");
    }

    // Item 1: Ordenação Padrão
    if (selectedItemIndex == 1) {
        drawRoundedBox(34.0f, layout.item1.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item1.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item1.y + 30.0f, UITheme::TextPrimary, 0.95f, "Ordenação Padrão");
        std::string sortStr = std::string("< ") + FileBrowser::getSortModeName(config.sortMode) + " >";
        drawValueRight(layout.item1.y + 30.0f, sortStr.c_str());
    }

    // --- SEÇÃO 2: LEITOR & VISUALIZAÇÃO ---
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 40.0f, sec2Y - 10.0f, UITheme::Secondary, 0.9f, "LEITOR & VISUALIZAÇÃO");
    }
    float sec2H = rowH * 3.0f;
    drawRoundedBox(32.0f, sec2Y, boxW, sec2H, 10.0f, UITheme::Surface);

    // Linhas divisórias internas da seção 2
    vita2d_draw_line(36.0f, sec2Y + rowH, 32.0f + boxW - 4.0f, sec2Y + rowH, RGBA8(42, 48, 70, 255));
    vita2d_draw_line(36.0f, sec2Y + rowH * 2.0f, 32.0f + boxW - 4.0f, sec2Y + rowH * 2.0f, RGBA8(42, 48, 70, 255));

    // Item 2: Orientação do Leitor
    if (selectedItemIndex == 2) {
        drawRoundedBox(34.0f, layout.item2.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item2.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item2.y + 30.0f, UITheme::TextPrimary, 0.95f,
                             isRotatedUi ? "Orientação do Leitor" : "Orientação Inicial do Leitor");
        const char* orientStr = config.readerRotated
            ? (isRotatedUi ? "< Vertical >" : "< Vertical (Girar 90°) >")
            : (isRotatedUi ? "< Horizontal >" : "< Horizontal (Padrão) >");
        drawValueRight(layout.item2.y + 30.0f, orientStr);
    }

    // Item 3: Exibir Número de Páginas
    if (selectedItemIndex == 3) {
        drawRoundedBox(34.0f, layout.item3.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item3.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item3.y + 30.0f, UITheme::TextPrimary, 0.95f,
                             isRotatedUi ? "Barra de Progresso" : "Exibir Barra de Progresso e Páginas");
        drawValueRight(layout.item3.y + 30.0f, config.showPageNumbers ? "< Sim >" : "< Não >");
    }

    // Item 4: Tamanho da Fonte Padrão (EPUB/TXT)
    if (selectedItemIndex == 4) {
        drawRoundedBox(34.0f, layout.item4.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item4.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item4.y + 30.0f, UITheme::TextPrimary, 0.95f,
                             isRotatedUi ? "Fonte EPUB" : "Tamanho de Fonte Padrão (EPUB)");
        char fontStr[32];
        snprintf(fontStr, sizeof(fontStr), "< %d px >", config.epubFontSize);
        drawValueRight(layout.item4.y + 30.0f, fontStr);
    }

    // Rodapé de Navegação
    drawFooter("D-Pad Cima/Baixo: Selecionar | D-Pad Esq/Dir: Alterar | O: Voltar e Salvar");
}
