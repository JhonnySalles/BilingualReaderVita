#include "ui_components.h"
#include "config_manager.h"
#include "models/linked_page.h"
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <sstream>

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
    layout.rotateBtn = {isRotatedUi ? (screenW - 110.0f) : 840.0f, 8.0f, 100.0f, 32.0f};
    layout.toggleBtn = {isRotatedUi ? (screenW - 225.0f) : 730.0f, 8.0f, 105.0f, 32.0f};
    layout.linkBtn   = {isRotatedUi ? (screenW - 340.0f) : 615.0f, 8.0f, 105.0f, 32.0f};
    layout.settingsBtn = {isRotatedUi ? (screenW - 390.0f) : 500.0f, 8.0f, isRotatedUi ? 40.0f : 100.0f, 32.0f};
    layout.prevBook  = {isRotatedUi ? (screenW - 240.0f) : 680.0f, screenH - 36.0f, 110.0f, 32.0f};
    layout.nextBook  = {isRotatedUi ? (screenW - 120.0f) : 810.0f, screenH - 36.0f, 110.0f, 32.0f};
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

UISettingsLayout UIComponents::getSettingsLayout(float scrollY) {
    UISettingsLayout layout;
    float startY = 88.0f - scrollY;
    float boxW = screenW - 64.0f;
    float rowH = 46.0f;
    float themeCardRowH = 92.0f;

    // Seção 0: Aparência (Tema)
    float sec0Y = startY + 22.0f;
    layout.themeRow = {32.0f, sec0Y, boxW, themeCardRowH};
    layout.item0 = layout.themeRow;

    // Seção 1: Biblioteca (4 itens: Modo, Ordenação, Gerar Capas, Limpar Cache)
    float sec1StartY = sec0Y + themeCardRowH + 20.0f;
    float sec1Y = sec1StartY + 22.0f;
    layout.item1 = {32.0f, sec1Y, boxW, rowH};
    layout.item2 = {32.0f, sec1Y + rowH, boxW, rowH};
    layout.item3 = {32.0f, sec1Y + rowH * 2.0f, boxW, rowH};
    layout.item4 = {32.0f, sec1Y + rowH * 3.0f, boxW, rowH};

    // Seção 2: Leitor & Visualização (5 itens: itens 5 a 9)
    float sec2StartY = sec1Y + rowH * 4.0f + 20.0f;
    float sec2Y = sec2StartY + 22.0f;
    layout.item5 = {32.0f, sec2Y, boxW, rowH};
    layout.item6 = {32.0f, sec2Y + rowH, boxW, rowH};
    layout.item7 = {32.0f, sec2Y + rowH * 2.0f, boxW, rowH};
    layout.item8 = {32.0f, sec2Y + rowH * 3.0f, boxW, rowH};
    layout.item9 = {32.0f, sec2Y + rowH * 4.0f, boxW, rowH};

    layout.fontValueSplitX = 32.0f + boxW * 0.55f;
    layout.contentTotalH = (sec2Y + rowH * 5.0f) - startY + 24.0f;
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

float UIComponents::getBadgeWidth(const std::string& type, float scale) {
    float textW = 36.0f * scale;
    if (pgf) {
        textW = static_cast<float>(vita2d_pgf_text_width(pgf, 0.8f * scale, type.c_str()));
    }
    float padX = 10.0f * scale;
    return std::max(52.0f * scale, textW + padX * 2.0f);
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
    else if (type == "PDF" || type == "XPS") badgeColor = UITheme::BadgePDF;
    else if (type == "FB2") badgeColor = UITheme::BadgeFB2;
    else if (type == "MOBI") badgeColor = UITheme::BadgeMOBI;
    else if (type == "DOCX") badgeColor = UITheme::BadgeDOCX;
    else if (type == "HTMLZ") badgeColor = UITheme::BadgeHTMLZ;
    else if (type == "TXTZ") badgeColor = UITheme::BadgeTXTZ;
    else if (type == "PMLZ") badgeColor = UITheme::BadgePMLZ;
    else if (type == "TCR") badgeColor = UITheme::BadgeTCR;
    else if (type == "HTML") badgeColor = UITheme::BadgeHTML;
    else if (type == "MD") badgeColor = UITheme::BadgeMD;

    float textW = 0.0f;
    if (pgf) {
        textW = static_cast<float>(vita2d_pgf_text_width(pgf, 0.8f * scale, type.c_str()));
    }
    float padX = 10.0f * scale;
    float w = std::max(52.0f * scale, textW + padX * 2.0f);
    float h = 22.0f * scale;
    drawRoundedBox(x, y, w, h, 5.0f * scale, applyAlpha(badgeColor, alpha));

    if (pgf) {
        float textX = x + (w - textW) / 2.0f;
        vita2d_pgf_draw_text(pgf, textX, y + 16.0f * scale, applyAlpha(RGBA8(255, 255, 255, 255), alpha), 0.8f * scale, type.c_str());
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
    vita2d_draw_line(0, 68, screenW, 68, UITheme::Divider);

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

    unsigned int noBg = !isYesSelected ? UITheme::Primary : UITheme::SurfaceActive;
    unsigned int noTextColor = !isYesSelected ? UITheme::OnPrimary : UITheme::TextPrimary;
    drawRoundedBox(dlg.noBtn.x, dlg.noBtn.y, dlg.noBtn.w, dlg.noBtn.h, 8.0f, noBg);
    if (!isYesSelected) {
        vita2d_draw_rectangle(dlg.noBtn.x, dlg.noBtn.y + 4.0f, 3.0f, dlg.noBtn.h - 8.0f, UITheme::OnPrimary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, dlg.noBtn.x + 40.0f, dlg.noBtn.y + 28.0f, noTextColor, 0.95f, "Cancelar (O)");
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

    drawRoundedBox(barX, barY, barW, barH, 6.0f, UITheme::SurfaceActive);
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

void UIComponents::drawCoverProgressModal(int current, int total, const std::string& currentFileName, int generatedCount) {
    float physW = 960.0f;
    float physH = 544.0f;

    // Fundo escuro semitransparente
    vita2d_draw_rectangle(0, 0, physW, physH, RGBA8(0, 0, 0, 195));

    float modalW = 560.0f;
    float modalH = 220.0f;
    float modalX = (physW - modalW) / 2.0f;
    float modalY = (physH - modalH) / 2.0f;

    // Caixa do Diálogo
    drawRoundedBox(modalX, modalY, modalW, modalH, 12.0f, UITheme::Surface);
    vita2d_draw_rectangle(modalX, modalY, modalW, 3.0f, UITheme::Primary);

    if (pgf) {
        // Título
        vita2d_pgf_draw_text(pgf, modalX + 24.0f, modalY + 38.0f, UITheme::TextPrimary, 1.15f, "Gerando Capas da Biblioteca");

        // Nome do Arquivo Atual
        std::string displayName = currentFileName;
        if (displayName.length() > 42) {
            displayName = displayName.substr(0, 39) + "...";
        }
        std::string fileText = "Processando: " + displayName;
        vita2d_pgf_draw_text(pgf, modalX + 24.0f, modalY + 76.0f, UITheme::TextSecondary, 0.85f, fileText.c_str());

        // Barra de Progresso
        float barX = modalX + 24.0f;
        float barY = modalY + 98.0f;
        float barW = modalW - 48.0f;
        float barH = 16.0f;
        drawRoundedBox(barX, barY, barW, barH, 8.0f, UITheme::SurfaceActive);

        float progress = (total > 0) ? (static_cast<float>(current) / static_cast<float>(total)) : 0.0f;
        if (progress > 1.0f) progress = 1.0f;
        if (progress < 0.0f) progress = 0.0f;

        if (progress > 0.005f) {
            drawRoundedBox(barX, barY, barW * progress, barH, 8.0f, UITheme::Primary);
        }

        // Estatísticas
        char countStr[64];
        int percent = static_cast<int>(progress * 100.0f);
        snprintf(countStr, sizeof(countStr), "%d / %d (%d%%)", current, total, percent);
        vita2d_pgf_draw_text(pgf, barX, barY + 36.0f, UITheme::TextSecondary, 0.85f, countStr);

        char genStr[64];
        snprintf(genStr, sizeof(genStr), "Capas geradas: %d", generatedCount);
        int genW = vita2d_pgf_text_width(pgf, 0.85f, genStr);
        vita2d_pgf_draw_text(pgf, barX + barW - static_cast<float>(genW), barY + 36.0f, UITheme::Secondary, 0.85f, genStr);

        // Rodapé de instruções
        vita2d_draw_line(modalX, modalY + modalH - 40.0f, modalX + modalW, modalY + modalH - 40.0f, UITheme::Divider);
        vita2d_pgf_draw_text(pgf, modalX + 24.0f, modalY + modalH - 16.0f, UITheme::TextSecondary, 0.80f, "Pressione [O] para Cancelar");
    }
}

void UIComponents::drawListCard(float x, float y, float w, float h, bool isSelected, const LibraryItem& item, float alpha, vita2d_texture* coverTex) {
    if (alpha <= 0.01f) return;

    unsigned int bgColor = isSelected ? UITheme::SurfaceActive : UITheme::Surface;
    drawRoundedBox(x, y, w, h, 8.0f, applyAlpha(bgColor, alpha));

    if (isSelected) {
        vita2d_draw_rectangle(x, y + 4.0f, 4.0f, h - 8.0f, applyAlpha(UITheme::Primary, alpha));
    }

    // Slot lateral da capa / miniatura
    float slotX = x + 6.0f;
    float slotY = y + 4.0f;
    float slotW = 54.0f;
    float slotH = h - 8.0f;

    drawRoundedBox(slotX, slotY, slotW, slotH, 6.0f, applyAlpha(UITheme::CoverBg, alpha));

    if (coverTex) {
        float texW = static_cast<float>(vita2d_texture_get_width(coverTex));
        float texH = static_cast<float>(vita2d_texture_get_height(coverTex));
        if (texW > 0.0f && texH > 0.0f) {
            float scale = std::max(slotW / texW, slotH / texH);
            float visibleTexW = std::min(texW, slotW / scale);
            float visibleTexH = std::min(texH, slotH / scale);
            float tex_x = (texW - visibleTexW) * 0.5f;
            float tex_y = 0.0f; // Começa rigorosamente do topo da capa

            float drawX = slotX;
            float drawY = slotY;

            if (alpha >= 0.99f) {
                vita2d_draw_texture_part_scale(coverTex, drawX, drawY, tex_x, tex_y, visibleTexW, visibleTexH, scale, scale);
            } else {
                unsigned int tint = RGBA8(255, 255, 255, static_cast<unsigned int>(alpha * 255.0f));
                vita2d_draw_texture_tint_part_scale(coverTex, drawX, drawY, tex_x, tex_y, visibleTexW, visibleTexH, scale, scale, tint);
            }
        }
    } else {
        // Badge centralizado no mesmo slot com auto-expansão
        float bW = getBadgeWidth(item.typeString, 0.85f);
        float bX = (bW <= slotW) ? (slotX + (slotW - bW) / 2.0f) : (slotX + 2.0f);
        drawBadge(bX, slotY + (slotH - 20.0f) / 2.0f, item.typeString, 0.85f, alpha);
    }

    if (pgf) {
        // 1. Título
        std::string displayTitle = item.filename;
        if (displayTitle.length() > 42) {
            displayTitle = displayTitle.substr(0, 39) + "...";
        }
        unsigned int titleColor = isSelected ? UITheme::TextPrimary : UITheme::TextSecondary;
        vita2d_pgf_draw_text(pgf, x + 68.0f, y + 21.0f, applyAlpha(titleColor, alpha), 0.95f, displayTitle.c_str());

        // Indicador de favorito
        if (item.isFavorite) {
            drawStar(x + w - 160.0f, y + 12.0f, 14.0f, applyAlpha(UITheme::FavoriteGold, alpha));
            vita2d_pgf_draw_text(pgf, x + w - 142.0f, y + 21.0f, applyAlpha(UITheme::FavoriteGold, alpha), 0.75f, "FAV");
        }

        // 2. Linha Intermediária (Progresso de Leitura / Informações do livro ou mangá)
        if (item.totalPages > 0) {
            char progStr[64];
            snprintf(progStr, sizeof(progStr), "Pag %d / %d (%.0f%%)", item.currentPage + 1, item.totalPages, item.progressPercent);
            vita2d_pgf_draw_text(pgf, x + 68.0f, y + 38.0f, applyAlpha(UITheme::TextSecondary, alpha), 0.75f, progStr);

            // Mini barra de progresso horizontal
            float barX = x + 240.0f;
            float barY = y + 31.0f;
            float barW = 110.0f;
            float barH = 4.0f;
            float fillW = std::max(0.0f, std::min(barW, barW * (item.progressPercent / 100.0f)));

            drawRoundedBox(barX, barY, barW, barH, 2.0f, applyAlpha(RGBA8(255, 255, 255, 30), alpha));
            if (fillW > 0.0f) {
                unsigned int barColor = (item.progressPercent >= 100.0f) ? RGBA8(16, 185, 129, 255) : UITheme::Primary;
                drawRoundedBox(barX, barY, fillW, barH, 2.0f, applyAlpha(barColor, alpha));
            }
        } else {
            const char* typeDesc = FileBrowser::isMangaType(item.type) ? "Manga / Quadrinho" : "Livro Digital";
            vita2d_pgf_draw_text(pgf, x + 68.0f, y + 38.0f, applyAlpha(UITheme::TextSecondary, alpha), 0.75f, typeDesc);
        }

        // 3. Linha do Rodapé (Tamanho no início, Extensão no meio, Data da última leitura no final)
        float footerY = y + 54.0f;

        // Tamanho à esquerda
        vita2d_pgf_draw_text(pgf, x + 68.0f, footerY, applyAlpha(UITheme::TextSecondary, alpha), 0.70f, item.formattedSize.c_str());

        // Extensão no meio
        float centerAreaX = x + 240.0f;
        vita2d_pgf_draw_text(pgf, centerAreaX, footerY, applyAlpha(UITheme::Primary, alpha), 0.70f, item.typeString.c_str());

        // Data da última leitura à direita
        std::string dateStr = item.lastReadFormatted.empty() ? "--/--/----" : item.lastReadFormatted;
        float dateW = static_cast<float>(vita2d_pgf_text_width(pgf, 0.70f, dateStr.c_str()));
        float dateX = x + w - 24.0f - dateW;
        vita2d_pgf_draw_text(pgf, dateX, footerY, applyAlpha(UITheme::TextSecondary, alpha), 0.70f, dateStr.c_str());
    }
}

void UIComponents::drawGridCard(float x, float y, float w, float h, bool isSelected, const LibraryItem& item, float alpha, vita2d_texture* coverTex) {
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

    unsigned int coverBg = isSelected ? UITheme::CoverBgActive : UITheme::CoverBg;
    drawRoundedBox(coverX, coverY, coverW, coverH, 8.0f, applyAlpha(coverBg, alpha));

    if (coverTex) {
        float texW = static_cast<float>(vita2d_texture_get_width(coverTex));
        float texH = static_cast<float>(vita2d_texture_get_height(coverTex));
        if (texW > 0.0f && texH > 0.0f) {
            float scale = std::max(coverW / texW, coverH / texH);
            float visibleTexW = std::min(texW, coverW / scale);
            float visibleTexH = std::min(texH, coverH / scale);
            float tex_x = (texW - visibleTexW) * 0.5f;
            float tex_y = 0.0f; // Começa rigorosamente do topo da capa

            float drawX = coverX;
            float drawY = coverY;

            if (alpha >= 0.99f) {
                vita2d_draw_texture_part_scale(coverTex, drawX, drawY, tex_x, tex_y, visibleTexW, visibleTexH, scale, scale);
            } else {
                unsigned int tint = RGBA8(255, 255, 255, static_cast<unsigned int>(alpha * 255.0f));
                vita2d_draw_texture_tint_part_scale(coverTex, drawX, drawY, tex_x, tex_y, visibleTexW, visibleTexH, scale, scale, tint);
            }
        }
    } else {
        // Grande Badge no centro da capa com auto-expansão para HTMLZ e outras extensões
        float bW = getBadgeWidth(item.typeString, 1.2f);
        float bH = 26.0f;
        drawBadge(coverX + (coverW - bW) / 2.0f, coverY + (coverH - bH) / 2.0f, item.typeString, 1.2f, alpha);
    }

    // Indicador de Favorito no canto superior direito da capa
    if (item.isFavorite) {
        drawStar(coverX + coverW - 22.0f, coverY + 6.0f, 14.0f, applyAlpha(UITheme::FavoriteGold, alpha));
    }

    if (pgf) {
        // 1. Título embaixo da capa
        std::string displayTitle = item.filename;
        if (displayTitle.length() > 22) {
            displayTitle = displayTitle.substr(0, 19) + "...";
        }
        unsigned int titleColor = isSelected ? UITheme::TextPrimary : UITheme::TextSecondary;
        vita2d_pgf_draw_text(pgf, x + 10.0f, y + coverH + 24.0f, applyAlpha(titleColor, alpha), 0.85f, displayTitle.c_str());

        // 2. Linha Intermediária (Progresso de Leitura / Informações adicionais)
        if (item.totalPages > 0) {
            char progStr[64];
            snprintf(progStr, sizeof(progStr), "Pag %d / %d (%.0f%%)", item.currentPage + 1, item.totalPages, item.progressPercent);
            vita2d_pgf_draw_text(pgf, x + 10.0f, y + coverH + 42.0f, applyAlpha(UITheme::TextSecondary, alpha), 0.70f, progStr);

            // Mini barra de progresso horizontal
            float barX = x + 10.0f;
            float barY = y + coverH + 48.0f;
            float barW = w - 20.0f;
            float barH = 3.0f;
            float fillW = std::max(0.0f, std::min(barW, barW * (item.progressPercent / 100.0f)));

            drawRoundedBox(barX, barY, barW, barH, 1.5f, applyAlpha(RGBA8(255, 255, 255, 30), alpha));
            if (fillW > 0.0f) {
                unsigned int barColor = (item.progressPercent >= 100.0f) ? RGBA8(16, 185, 129, 255) : UITheme::Primary;
                drawRoundedBox(barX, barY, fillW, barH, 1.5f, applyAlpha(barColor, alpha));
            }
        } else {
            const char* typeDesc = FileBrowser::isMangaType(item.type) ? "Manga / Quadrinho" : "Livro Digital";
            vita2d_pgf_draw_text(pgf, x + 10.0f, y + coverH + 42.0f, applyAlpha(UITheme::TextSecondary, alpha), 0.70f, typeDesc);
        }

        // 3. Linha do Rodapé no final do card: Tamanho (esq) | Extensão (centro) | Data Última Leitura (dir)
        float footerY = y + h - 10.0f;

        // Esquerda: tamanho
        vita2d_pgf_draw_text(pgf, x + 10.0f, footerY, applyAlpha(UITheme::TextSecondary, alpha), 0.68f, item.formattedSize.c_str());

        // Centro: extensão
        float extW = static_cast<float>(vita2d_pgf_text_width(pgf, 0.68f, item.typeString.c_str()));
        float extX = x + (w - extW) / 2.0f;
        vita2d_pgf_draw_text(pgf, extX, footerY, applyAlpha(UITheme::Primary, alpha), 0.68f, item.typeString.c_str());

        // Direita: data da última leitura
        std::string dateStr = item.lastReadFormatted.empty() ? "--/--/----" : item.lastReadFormatted;
        float dateW = static_cast<float>(vita2d_pgf_text_width(pgf, 0.68f, dateStr.c_str()));
        float dateX = x + w - 10.0f - dateW;
        vita2d_pgf_draw_text(pgf, dateX, footerY, applyAlpha(UITheme::TextSecondary, alpha), 0.68f, dateStr.c_str());
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


void UIComponents::drawReaderTopBar(const std::string& title, const std::string& extraInfo, bool isRotated, bool showLinkedPage, unsigned int badgeColor) {
    UIReaderChromeLayout chrome = getReaderChromeLayout();
    vita2d_draw_rectangle(0, 0, screenW, chrome.contentTop, UITheme::TopBar);
    vita2d_draw_line(0, chrome.contentTop, screenW, chrome.contentTop, UITheme::Divider);

    if (pgf) {
        std::string displayTitle = title;
        size_t maxTitleLen = isRotatedUi ? 14 : 26;
        if (displayTitle.length() > maxTitleLen) {
            displayTitle = displayTitle.substr(0, maxTitleLen - 3) + "...";
        }
        vita2d_pgf_draw_text(pgf, 24.0f, 32.0f, badgeColor, 1.0f, displayTitle.c_str());

        if (!extraInfo.empty()) {
            float infoX = chrome.settingsBtn.x - 12.0f;
            int infoW = vita2d_pgf_text_width(pgf, 0.85f, extraInfo.c_str());
            infoX -= static_cast<float>(infoW);
            if (infoX < 24.0f) infoX = 24.0f;
            vita2d_pgf_draw_text(pgf, infoX, 32.0f, UITheme::TextSecondary, 0.85f, extraInfo.c_str());
        }
    }

    // Botão de Configurações
    drawRoundedBox(chrome.settingsBtn.x, chrome.settingsBtn.y, chrome.settingsBtn.w, chrome.settingsBtn.h, 6.0f, UITheme::Surface);
    if (pgf) {
        const char* settingsText = isRotatedUi ? "[C]" : "[ Config ]";
        float settingsTextX = isRotatedUi ? chrome.settingsBtn.x + 8.0f : chrome.settingsBtn.x + 10.0f;
        vita2d_pgf_draw_text(pgf, settingsTextX, chrome.settingsBtn.y + 22.0f, UITheme::TextPrimary, 0.78f, settingsText);
    }

    // Botão de Vínculo de Páginas
    drawRoundedBox(chrome.linkBtn.x, chrome.linkBtn.y, chrome.linkBtn.w, chrome.linkBtn.h, 6.0f, UITheme::Surface);
    if (pgf) {
        vita2d_pgf_draw_text(pgf, chrome.linkBtn.x + 10.0f, chrome.linkBtn.y + 22.0f, UITheme::TextPrimary, 0.78f, "[#] Vincular");
    }

    // Botão de Alternância
    unsigned int toggleBg = showLinkedPage ? UITheme::Primary : UITheme::Surface;
    unsigned int toggleTextCol = showLinkedPage ? UITheme::OnPrimary : UITheme::TextPrimary;
    drawRoundedBox(chrome.toggleBtn.x, chrome.toggleBtn.y, chrome.toggleBtn.w, chrome.toggleBtn.h, 6.0f, toggleBg);
    if (pgf) {
        vita2d_pgf_draw_text(pgf, chrome.toggleBtn.x + 10.0f, chrome.toggleBtn.y + 22.0f, toggleTextCol, 0.78f, showLinkedPage ? "[*] Vínculo" : "[ ] Original");
    }

    // Botão de Rotação 90 Graus
    unsigned int rotBg = isRotated ? UITheme::Primary : UITheme::Surface;
    unsigned int rotTextColor = isRotated ? UITheme::OnPrimary : UITheme::TextPrimary;
    drawRoundedBox(chrome.rotateBtn.x, chrome.rotateBtn.y, chrome.rotateBtn.w, chrome.rotateBtn.h, 6.0f, rotBg);

    if (pgf) {
        const char* rotText = isRotated ? "[*] 90 Deg" : "[ ] Normal";
        vita2d_pgf_draw_text(pgf, chrome.rotateBtn.x + 10.0f, chrome.rotateBtn.y + 22.0f, rotTextColor, 0.80f, rotText);
    }
}

void UIComponents::drawFooter(const std::string& controlsHint, bool showBookNav) {
    UIReaderChromeLayout chrome = getReaderChromeLayout();
    float footerY = chrome.contentBottom;
    vita2d_draw_rectangle(0, footerY, screenW, 40.0f, UITheme::TopBar);
    vita2d_draw_line(0, footerY, screenW, footerY, UITheme::Divider);

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
        vita2d_draw_line(0, hudY, screenW, hudY, applyAlpha(UITheme::Divider, 0.7f));

        // Barra de progresso horizontal
        float barX = isRotatedUi ? 90.0f : 140.0f;
        float barY = hudY + 18.0f;
        float barW = isRotatedUi ? (screenW - 180.0f) : 680.0f;
        float barH = 8.0f;

        drawRoundedBox(barX, barY, barW, barH, 4.0f, UITheme::SurfaceActive);
        if (progressRatio > 0.0f) {
            drawRoundedBox(barX, barY, barW * progressRatio, barH, 4.0f, UITheme::Primary);
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
            vita2d_draw_rectangle(0, barY, barW, 4.0f, UITheme::Primary);
        }
    }
}

void UIComponents::drawSettingsScreen(int selectedItemIndex, const AppConfig& config, float scrollY, float themeScrollX, int themeCursor) {
    UISettingsLayout layout = getSettingsLayout(scrollY);
    float boxW = layout.item1.w;
    float rowH = layout.item1.h;

    // --- SEÇÃO 0: APARÊNCIA (TEMAS) ---
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 40.0f, layout.themeRow.y - 8.0f, UITheme::Secondary, 0.9f, "APARÊNCIA");
    }

    // Desenha a fila de cards horizontais de tema
    int themeCount = static_cast<int>(ThemeId::COUNT);
    for (int i = 0; i < themeCount; i++) {
        ThemeId tid = static_cast<ThemeId>(i);
        UIRect cardRect = layout.getThemeCardRect(i, themeScrollX);

        // Clip simples se estiver fora da tela
        if (cardRect.x + cardRect.w < 0 || cardRect.x > screenW) continue;

        bool isThemeSelected = (config.theme == tid);
        bool isFocused = (selectedItemIndex == 0 && themeCursor == i);

        const ThemePalette& pal = ThemeManager::palette(tid);

        // Fundo do card com a cor de fundo do tema
        drawRoundedBox(cardRect.x, cardRect.y, cardRect.w, cardRect.h, 10.0f, pal.Background);

        // Mini TopBar ilustrativa
        drawRoundedBox(cardRect.x + 4.0f, cardRect.y + 4.0f, cardRect.w - 8.0f, 14.0f, 4.0f, pal.TopBar);

        // Mini Card / Surface ilustrativa com barra Primary
        drawRoundedBox(cardRect.x + 8.0f, cardRect.y + 22.0f, cardRect.w - 16.0f, 26.0f, 6.0f, pal.Surface);
        vita2d_draw_rectangle(cardRect.x + 12.0f, cardRect.y + 28.0f, 4.0f, 14.0f, pal.Primary);
        vita2d_draw_rectangle(cardRect.x + 20.0f, cardRect.y + 31.0f, cardRect.w - 36.0f, 3.0f, pal.Secondary);
        vita2d_draw_rectangle(cardRect.x + 20.0f, cardRect.y + 38.0f, (cardRect.w - 36.0f) * 0.6f, 3.0f, pal.TextSecondary);

        // Nome do tema embaixo
        if (pgf) {
            const char* name = ThemeManager::displayName(tid);
            int nameW = vita2d_pgf_text_width(pgf, 0.72f, name);
            float nameX = cardRect.x + (cardRect.w - static_cast<float>(nameW)) / 2.0f;
            vita2d_pgf_draw_text(pgf, nameX, cardRect.y + cardRect.h - 10.0f, isThemeSelected ? pal.Primary : UITheme::TextSecondary, 0.72f, name);
        }

        // Borda de seleção / foco
        if (isThemeSelected) {
            // Borda destacada em Primary
            vita2d_draw_rectangle(cardRect.x, cardRect.y, cardRect.w, 2.0f, pal.Primary);
            vita2d_draw_rectangle(cardRect.x, cardRect.y + cardRect.h - 2.0f, cardRect.w, 2.0f, pal.Primary);
            vita2d_draw_rectangle(cardRect.x, cardRect.y, 2.0f, cardRect.h, pal.Primary);
            vita2d_draw_rectangle(cardRect.x + cardRect.w - 2.0f, cardRect.y, 2.0f, cardRect.h, pal.Primary);

            // Indicador de Check
            if (pgf) {
                vita2d_pgf_draw_text(pgf, cardRect.x + cardRect.w - 18.0f, cardRect.y + 14.0f, pal.Primary, 0.75f, "[*]");
            }
        }

        if (isFocused) {
            // Realce do cursor D-Pad
            drawRoundedBox(cardRect.x - 3.0f, cardRect.y - 3.0f, cardRect.w + 6.0f, cardRect.h + 6.0f, 12.0f, applyAlpha(UITheme::TextPrimary, 0.35f));
            drawRoundedBox(cardRect.x, cardRect.y, cardRect.w, cardRect.h, 10.0f, pal.Background);
        }
    }

    // --- SEÇÃO 1: BIBLIOTECA ---
    float sec1Y = layout.item1.y;
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 40.0f, sec1Y - 8.0f, UITheme::Secondary, 0.9f, "BIBLIOTECA");
    }
    float sec1H = rowH * 4.0f;
    drawRoundedBox(32.0f, sec1Y, boxW, sec1H, 10.0f, UITheme::Surface);
    vita2d_draw_line(36.0f, sec1Y + rowH, 32.0f + boxW - 4.0f, sec1Y + rowH, UITheme::Divider);
    vita2d_draw_line(36.0f, sec1Y + rowH * 2.0f, 32.0f + boxW - 4.0f, sec1Y + rowH * 2.0f, UITheme::Divider);
    vita2d_draw_line(36.0f, sec1Y + rowH * 3.0f, 32.0f + boxW - 4.0f, sec1Y + rowH * 3.0f, UITheme::Divider);

    auto drawValueRight = [&](float y, const char* value) {
        if (!pgf || !value) return;
        int valueW = vita2d_pgf_text_width(pgf, 0.95f, value);
        float valueX = 32.0f + boxW - 16.0f - static_cast<float>(valueW);
        if (valueX < layout.fontValueSplitX) valueX = layout.fontValueSplitX;
        vita2d_pgf_draw_text(pgf, valueX, y, UITheme::Secondary, 0.95f, value);
    };

    // Item 1: Modo de Exibição (Lista / Grade)
    if (selectedItemIndex == 1) {
        drawRoundedBox(34.0f, layout.item1.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item1.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item1.y + 30.0f, UITheme::TextPrimary, 0.95f,
                             isRotatedUi ? "Modo de Exibição" : "Modo de Exibição Padrão");
        drawValueRight(layout.item1.y + 30.0f, config.isGridView ? "< Grade >" : "< Lista >");
    }

    // Item 2: Ordenação Padrão
    if (selectedItemIndex == 2) {
        drawRoundedBox(34.0f, layout.item2.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item2.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item2.y + 30.0f, UITheme::TextPrimary, 0.95f, "Ordenação Padrão");
        std::string sortStr = std::string("< ") + FileBrowser::getSortModeName(config.sortMode) + " >";
        drawValueRight(layout.item2.y + 30.0f, sortStr.c_str());
    }

    // Item 3: Gerar Miniaturas da Biblioteca
    if (selectedItemIndex == 3) {
        drawRoundedBox(34.0f, layout.item3.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item3.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item3.y + 30.0f, UITheme::TextPrimary, 0.95f, "Gerar Capas da Biblioteca");
        drawValueRight(layout.item3.y + 30.0f, "[ Iniciar (X) ]");
    }

    // Item 4: Limpar Cache de Capas
    if (selectedItemIndex == 4) {
        drawRoundedBox(34.0f, layout.item4.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item4.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item4.y + 30.0f, UITheme::TextPrimary, 0.95f, "Limpar Cache de Capas");
        drawValueRight(layout.item4.y + 30.0f, "[ Excluir (X) ]");
    }

    // --- SEÇÃO 2: LEITOR & VISUALIZAÇÃO ---
    float sec2Y = layout.item5.y;
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 40.0f, sec2Y - 8.0f, UITheme::Secondary, 0.9f, "LEITOR & VISUALIZAÇÃO");
    }
    float sec2H = rowH * 5.0f;
    drawRoundedBox(32.0f, sec2Y, boxW, sec2H, 10.0f, UITheme::Surface);
    vita2d_draw_line(36.0f, sec2Y + rowH, 32.0f + boxW - 4.0f, sec2Y + rowH, UITheme::Divider);
    vita2d_draw_line(36.0f, sec2Y + rowH * 2.0f, 32.0f + boxW - 4.0f, sec2Y + rowH * 2.0f, UITheme::Divider);
    vita2d_draw_line(36.0f, sec2Y + rowH * 3.0f, 32.0f + boxW - 4.0f, sec2Y + rowH * 3.0f, UITheme::Divider);
    vita2d_draw_line(36.0f, sec2Y + rowH * 4.0f, 32.0f + boxW - 4.0f, sec2Y + rowH * 4.0f, UITheme::Divider);

    // Item 5: Orientação do Leitor
    if (selectedItemIndex == 5) {
        drawRoundedBox(34.0f, layout.item5.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item5.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item5.y + 30.0f, UITheme::TextPrimary, 0.95f,
                             isRotatedUi ? "Orientação do Leitor" : "Orientação Inicial do Leitor");
        const char* orientStr = config.readerRotated
            ? (isRotatedUi ? "< Vertical >" : "< Vertical (Girar 90°) >")
            : (isRotatedUi ? "< Horizontal >" : "< Horizontal (Padrão) >");
        drawValueRight(layout.item5.y + 30.0f, orientStr);
    }

    // Item 6: Modo de Rolagem
    if (selectedItemIndex == 6) {
        drawRoundedBox(34.0f, layout.item6.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item6.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item6.y + 30.0f, UITheme::TextPrimary, 0.95f, "Modo de Rolagem");
        drawValueRight(layout.item6.y + 30.0f, readerScrollModeName(config.readerScrollMode, isRotatedUi));
    }

    // Item 7: Transição de Página
    if (selectedItemIndex == 7) {
        drawRoundedBox(34.0f, layout.item7.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item7.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item7.y + 30.0f, UITheme::TextPrimary, 0.95f, "Transição");
        drawValueRight(layout.item7.y + 30.0f, pageTransitionName(config.pageTransition, isRotatedUi));
    }

    // Item 8: Exibir Número de Páginas
    if (selectedItemIndex == 8) {
        drawRoundedBox(34.0f, layout.item8.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item8.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item8.y + 30.0f, UITheme::TextPrimary, 0.95f,
                             isRotatedUi ? "Barra de Progresso" : "Exibir Barra de Progresso e Páginas");
        drawValueRight(layout.item8.y + 30.0f, config.showPageNumbers ? "< Sim >" : "< Não >");
    }

    // Item 9: Tamanho da Fonte Padrão (EPUB/TXT)
    if (selectedItemIndex == 9) {
        drawRoundedBox(34.0f, layout.item9.y + 2.0f, boxW - 4.0f, rowH - 4.0f, 8.0f, UITheme::SurfaceActive);
        vita2d_draw_rectangle(34.0f, layout.item9.y + 6.0f, 4.0f, rowH - 12.0f, UITheme::Primary);
    }
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 52.0f, layout.item9.y + 30.0f, UITheme::TextPrimary, 0.95f,
                             isRotatedUi ? "Fonte EPUB" : "Tamanho de Fonte Padrão (EPUB)");
        char fontStr[32];
        snprintf(fontStr, sizeof(fontStr), "< %d px >", config.epubFontSize);
        drawValueRight(layout.item9.y + 30.0f, fontStr);
    }

    // Top Bar de Configurações sobreposta (Z-Order Top)
    vita2d_draw_rectangle(0, 0, screenW, 68, UITheme::TopBar);
    vita2d_draw_line(0, 68, screenW, 68, UITheme::Divider);
    if (pgf) {
        vita2d_pgf_draw_text(pgf, 32.0f, 44.0f, UITheme::TextPrimary, 1.2f, "Configurações");
    }

    // Rodapé de Navegação sobreposto (Z-Order Top)
    drawFooter("D-Pad: Navegar/Alterar | X: Selecionar Tema | O: Voltar");
}

UIPagesLinkLayout UIComponents::getPagesLinkLayout() {
    UIPagesLinkLayout layout;
    layout.primaryFileHeader = {12.0f, 6.0f, 230.0f, 44.0f};
    layout.selectFileBtn     = {250.0f, 8.0f, 210.0f, 40.0f};
    layout.autoBtn           = {470.0f, 10.0f, 68.0f, 36.0f};
    layout.simpleBtn         = {544.0f, 10.0f, 64.0f, 36.0f};
    layout.doubleBtn         = {614.0f, 10.0f, 64.0f, 36.0f};
    layout.sortBtn           = {684.0f, 10.0f, 64.0f, 36.0f};
    layout.undoBtn           = {754.0f, 10.0f, 68.0f, 36.0f};
    layout.saveBtn           = {828.0f, 10.0f, 120.0f, 36.0f};
    layout.linkedArea        = {0.0f, 56.0f, 960.0f, 324.0f};
    layout.unlinkedArea      = {0.0f, 380.0f, 960.0f, 134.0f};
    layout.linkedScrollBar   = {24.0f, 366.0f, 912.0f, 10.0f};
    layout.unlinkedScrollBar = {24.0f, 506.0f, 912.0f, 10.0f};
    return layout;
}

void UIComponents::drawPagesLinkToolbar(
    const std::string& primaryName,
    int primaryPages,
    const std::string& linkedName,
    int linkedPages,
    int selectedToolbarBtn,
    bool hasBackup,
    bool isLinkedLoaded
) {
    UIPagesLinkLayout layout = getPagesLinkLayout();

    // Fundo da Toolbar
    vita2d_draw_rectangle(0, 0, screenW, 56.0f, UITheme::TopBar);
    vita2d_draw_line(0, 56.0f, screenW, 56.0f, UITheme::Divider);

    // Header do Arquivo Principal
    drawRoundedBox(layout.primaryFileHeader.x, layout.primaryFileHeader.y, layout.primaryFileHeader.w, layout.primaryFileHeader.h, 6.0f, UITheme::Surface);
    if (pgf) {
        std::string nameShort = primaryName;
        if (nameShort.length() > 18) nameShort = nameShort.substr(0, 16) + "..";
        vita2d_pgf_draw_text(pgf, layout.primaryFileHeader.x + 8.0f, layout.primaryFileHeader.y + 20.0f, UITheme::Primary, 0.75f, "ORIGINAL:");
        vita2d_pgf_draw_text(pgf, layout.primaryFileHeader.x + 8.0f, layout.primaryFileHeader.y + 37.0f, UITheme::TextPrimary, 0.85f, nameShort.c_str());

        char pagesBuf[24];
        snprintf(pagesBuf, sizeof(pagesBuf), "%d pág", primaryPages);
        float pW = vita2d_pgf_text_width(pgf, 0.75f, pagesBuf);
        drawRoundedBox(layout.primaryFileHeader.x + layout.primaryFileHeader.w - pW - 12.0f, layout.primaryFileHeader.y + 8.0f, pW + 8.0f, 18.0f, 4.0f, UITheme::Background);
        vita2d_pgf_draw_text(pgf, layout.primaryFileHeader.x + layout.primaryFileHeader.w - pW - 8.0f, layout.primaryFileHeader.y + 22.0f, UITheme::TextSecondary, 0.75f, pagesBuf);
    }

    // Botão Selecionar Arquivo de Vínculo
    bool isSelectFocused = (selectedToolbarBtn == 0);
    unsigned int selectBg = isSelectFocused ? UITheme::SurfaceActive : (isLinkedLoaded ? UITheme::Surface : UITheme::Primary);
    unsigned int selectText = (isLinkedLoaded || isSelectFocused) ? UITheme::TextPrimary : UITheme::OnPrimary;
    drawRoundedBox(layout.selectFileBtn.x, layout.selectFileBtn.y, layout.selectFileBtn.w, layout.selectFileBtn.h, 6.0f, selectBg);
    if (isSelectFocused) {
        vita2d_draw_line(layout.selectFileBtn.x, layout.selectFileBtn.y, layout.selectFileBtn.x + layout.selectFileBtn.w, layout.selectFileBtn.y, UITheme::Primary);
        vita2d_draw_line(layout.selectFileBtn.x, layout.selectFileBtn.y + layout.selectFileBtn.h, layout.selectFileBtn.x + layout.selectFileBtn.w, layout.selectFileBtn.y + layout.selectFileBtn.h, UITheme::Primary);
    }
    if (pgf) {
        if (!isLinkedLoaded) {
            vita2d_pgf_draw_text(pgf, layout.selectFileBtn.x + 14.0f, layout.selectFileBtn.y + 26.0f, selectText, 0.85f, "+ Vincular Arquivo");
        } else {
            std::string linkShort = linkedName;
            if (linkShort.length() > 16) linkShort = linkShort.substr(0, 14) + "..";
            vita2d_pgf_draw_text(pgf, layout.selectFileBtn.x + 8.0f, layout.selectFileBtn.y + 18.0f, UITheme::BadgeTXT, 0.72f, "VINCULADO:");
            vita2d_pgf_draw_text(pgf, layout.selectFileBtn.x + 8.0f, layout.selectFileBtn.y + 34.0f, UITheme::TextPrimary, 0.82f, linkShort.c_str());
        }
    }

    // Botão AUTO DUPLA
    auto drawBtn = [&](const UIRect& r, int btnIdx, const char* label, bool enabled, unsigned int customColor = 0) {
        bool focused = (selectedToolbarBtn == btnIdx);
        unsigned int bg = focused ? UITheme::SurfaceActive : UITheme::Surface;
        if (customColor != 0 && !focused) {
            bg = enabled ? customColor : UITheme::SurfaceActive;
        } else if (!enabled) {
            bg = UITheme::SurfaceActive; // darker theme color instead of transparency
        }
        
        drawRoundedBox(r.x, r.y, r.w, r.h, 6.0f, bg);
        if (focused) {
            vita2d_draw_line(r.x, r.y, r.x + r.w, r.y, UITheme::Primary);
            vita2d_draw_line(r.x, r.y + r.h, r.x + r.w, r.y + r.h, UITheme::Primary);
            vita2d_draw_line(r.x, r.y, r.x, r.y + r.h, UITheme::Primary);
            vita2d_draw_line(r.x + r.w, r.y, r.x + r.w, r.y + r.h, UITheme::Primary);
        }
        if (pgf) {
            float tW = vita2d_pgf_text_width(pgf, 0.80f, label);
            float tx = r.x + (r.w - tW) / 2.0f;
            float ty = r.y + 24.0f;
            unsigned int tc = focused ? UITheme::Primary : UITheme::TextPrimary;
            if (!enabled) tc = UITheme::TextSecondary;
            vita2d_pgf_draw_text(pgf, tx, ty, tc, 0.80f, label);
        }
    };

    drawBtn(layout.autoBtn, 1, "Auto", isLinkedLoaded);
    drawBtn(layout.simpleBtn, 2, "1 : 1", isLinkedLoaded);
    drawBtn(layout.doubleBtn, 3, "2 : 1", isLinkedLoaded);
    drawBtn(layout.sortBtn, 4, "Ord.", isLinkedLoaded);
    drawBtn(layout.undoBtn, 5, "Desfazer", hasBackup);
    drawBtn(layout.saveBtn, 6, "Salvar Vínculo", isLinkedLoaded, UITheme::Primary);
}

void UIComponents::drawLinkedPageCard(
    float x,
    float y,
    float w,
    float h,
    int index,
    const LinkedPage& page,
    vita2d_texture* thumbManga,
    vita2d_texture* thumbLeft,
    vita2d_texture* thumbRight,
    bool isSelected,
    bool isDropTarget,
    bool isGrabbed,
    bool isDualSlotFocused
) {
    // Efeito visual de Card Crescendo quando agarrado/selecionado para mover
    float drawX = x;
    float drawY = y;
    float drawW = w;
    float drawH = h;

    if (isGrabbed) {
        float expandW = 10.0f;
        float expandH = 14.0f;
        drawX = x - expandW / 2.0f;
        drawY = y - expandH / 2.0f;
        drawW = w + expandW;
        drawH = h + expandH;
    }

    // Fundo do Card
    unsigned int cardBg = isSelected ? UITheme::SurfaceActive : UITheme::Surface;
    if (isGrabbed) {
        // Sombra de elevação quando agarrado
        drawRoundedBox(drawX - 4.0f, drawY - 4.0f, drawW + 8.0f, drawH + 8.0f, 12.0f, applyAlpha(UITheme::Primary, 0.50f));
    }
    drawRoundedBox(drawX, drawY, drawW, drawH, 8.0f, cardBg);

    // Borda de Destaque / Foco / Grab
    if (isDropTarget) {
        drawRoundedBox(drawX - 2.0f, drawY - 2.0f, drawW + 4.0f, drawH + 4.0f, 10.0f, UITheme::Primary);
        drawRoundedBox(drawX, drawY, drawW, drawH, 8.0f, UITheme::SurfaceActive);
    } else if (isGrabbed) {
        drawRoundedBox(drawX - 2.0f, drawY - 2.0f, drawW + 4.0f, drawH + 4.0f, 10.0f, UITheme::Primary);
        drawRoundedBox(drawX, drawY, drawW, drawH, 8.0f, UITheme::SurfaceActive);
    } else if (isSelected) {
        vita2d_draw_line(drawX, drawY, drawX + drawW, drawY, UITheme::Primary);
        vita2d_draw_line(drawX, drawY + drawH, drawX + drawW, drawY + drawH, UITheme::Primary);
        vita2d_draw_line(drawX, drawY, drawX, drawY + drawH, UITheme::Primary);
        vita2d_draw_line(drawX + drawW, drawY, drawX + drawW, drawY + drawH, UITheme::Primary);
    }

    float innerW = drawW - 12.0f;
    float halfH = (drawH - 26.0f) / 2.0f;

    // --- Metade Superior: Manga Original ---
    float mangaAreaY = drawY + 6.0f;
    drawRoundedBox(drawX + 6.0f, mangaAreaY, innerW, halfH, 6.0f, UITheme::CoverBg);

    if (thumbManga) {
        float tw = (float)vita2d_texture_get_width(thumbManga);
        float th = (float)vita2d_texture_get_height(thumbManga);
        if (tw > 0 && th > 0) {
            float scale = std::min((innerW - 4.0f) / tw, (halfH - 4.0f) / th);
            float renderW = tw * scale;
            float renderH = th * scale;
            float rx = drawX + 6.0f + (innerW - renderW) / 2.0f;
            float ry = mangaAreaY + (halfH - renderH) / 2.0f;
            vita2d_draw_texture_scale(thumbManga, rx, ry, scale, scale);
        }
    } else if (pgf) {
        float tW = vita2d_pgf_text_width(pgf, 0.75f, "Carregando...");
        vita2d_pgf_draw_text(pgf, drawX + 6.0f + (innerW - tW) / 2.0f, mangaAreaY + halfH / 2.0f + 6.0f, UITheme::TextSecondary, 0.75f, "Carregando...");
    }

    // Badge do número da página original
    if (pgf) {
        char buf[24];
        snprintf(buf, sizeof(buf), "#%d", page.mangaPage + 1);
        float bW = vita2d_pgf_text_width(pgf, 0.75f, buf) + 10.0f;
        drawRoundedBox(drawX + 8.0f, mangaAreaY + 4.0f, bW, 18.0f, 4.0f, applyAlpha(UITheme::Background, 0.85f));
        vita2d_pgf_draw_text(pgf, drawX + 13.0f, mangaAreaY + 17.0f, UITheme::Primary, 0.75f, buf);

        if (page.isMangaDualPage) {
            drawRoundedBox(drawX + innerW - 42.0f, mangaAreaY + 4.0f, 44.0f, 18.0f, 4.0f, UITheme::BadgeCBZ);
            vita2d_pgf_draw_text(pgf, drawX + innerW - 38.0f, mangaAreaY + 17.0f, RGBA8(0,0,0,255), 0.70f, "DUPLA");
        }
    }

    // --- Divisor Central ---
    float divY = mangaAreaY + halfH + 4.0f;
    vita2d_draw_line(drawX + 8.0f, divY + 4.0f, drawX + drawW - 8.0f, divY + 4.0f, UITheme::Divider);
    drawRoundedBox(drawX + (drawW - 20.0f) / 2.0f, divY - 2.0f, 20.0f, 14.0f, 7.0f, UITheme::Background);
    if (pgf) {
        vita2d_pgf_draw_text(pgf, drawX + (drawW - 10.0f) / 2.0f, divY + 9.0f, UITheme::Primary, 0.70f, "v");
    }

    // --- Metade Inferior: Arquivo Vinculado ---
    float linkAreaY = divY + 14.0f;
    drawRoundedBox(drawX + 6.0f, linkAreaY, innerW, halfH, 6.0f, UITheme::CoverBg);

    // Destaca o slot de página dupla quando focado especificamente
    if (isDualSlotFocused) {
        drawRoundedBox(drawX + 4.0f, linkAreaY - 2.0f, innerW + 4.0f, halfH + 4.0f, 8.0f, applyAlpha(UITheme::BadgeCBZ, 0.40f));
        vita2d_draw_line(drawX + 6.0f, linkAreaY, drawX + 6.0f + innerW, linkAreaY, UITheme::BadgeCBZ);
        vita2d_draw_line(drawX + 6.0f, linkAreaY + halfH, drawX + 6.0f + innerW, linkAreaY + halfH, UITheme::BadgeCBZ);
        vita2d_draw_line(drawX + 6.0f, linkAreaY, drawX + 6.0f, linkAreaY + halfH, UITheme::BadgeCBZ);
        vita2d_draw_line(drawX + 6.0f + innerW, linkAreaY, drawX + 6.0f + innerW, linkAreaY + halfH, UITheme::BadgeCBZ);
    }

    if (page.fileLinkLeftPage == PAGE_EMPTY) {
        // Sem Vínculo (Placeholder)
        if (pgf) {
            float tW = vita2d_pgf_text_width(pgf, 0.80f, "Sem Vínculo");
            vita2d_pgf_draw_text(pgf, drawX + 6.0f + (innerW - tW) / 2.0f, linkAreaY + halfH / 2.0f + 6.0f, UITheme::TextSecondary, 0.80f, "Sem Vínculo");
        }
    } else if (page.isDualImage && (page.fileLinkRightPage != PAGE_EMPTY || thumbRight != nullptr)) {
        // Exibição Lado a Lado de Duas Páginas Vinculadas
        float halfThumbW = (innerW - 4.0f) / 2.0f;

        // Página Esquerda
        if (thumbLeft) {
            float tw = (float)vita2d_texture_get_width(thumbLeft);
            float th = (float)vita2d_texture_get_height(thumbLeft);
            if (tw > 0 && th > 0) {
                float scale = std::min((halfThumbW - 2.0f) / tw, (halfH - 4.0f) / th);
                float renderW = tw * scale;
                float renderH = th * scale;
                float rx = drawX + 6.0f + (halfThumbW - renderW) / 2.0f;
                float ry = linkAreaY + (halfH - renderH) / 2.0f;
                vita2d_draw_texture_scale(thumbLeft, rx, ry, scale, scale);
            }
        }

        // Página Direita
        if (thumbRight) {
            float tw = (float)vita2d_texture_get_width(thumbRight);
            float th = (float)vita2d_texture_get_height(thumbRight);
            if (tw > 0 && th > 0) {
                float scale = std::min((halfThumbW - 2.0f) / tw, (halfH - 4.0f) / th);
                float renderW = tw * scale;
                float renderH = th * scale;
                float rx = drawX + 6.0f + halfThumbW + (halfThumbW - renderW) / 2.0f;
                float ry = linkAreaY + (halfH - renderH) / 2.0f;
                vita2d_draw_texture_scale(thumbRight, rx, ry, scale, scale);
            }
        }

        // Badges
        if (pgf) {
            char b1[16], b2[16];
            snprintf(b1, sizeof(b1), "#%d", page.fileLinkLeftPage + 1);
            snprintf(b2, sizeof(b2), "#%d", page.fileLinkRightPage + 1);
            drawRoundedBox(drawX + 8.0f, linkAreaY + 4.0f, 32.0f, 16.0f, 3.0f, applyAlpha(UITheme::Background, 0.85f));
            vita2d_pgf_draw_text(pgf, drawX + 11.0f, linkAreaY + 16.0f, UITheme::BadgeTXT, 0.68f, b1);

            drawRoundedBox(drawX + 6.0f + halfThumbW + 2.0f, linkAreaY + 4.0f, 32.0f, 16.0f, 3.0f, applyAlpha(UITheme::Background, 0.85f));
            vita2d_pgf_draw_text(pgf, drawX + 6.0f + halfThumbW + 5.0f, linkAreaY + 16.0f, UITheme::BadgeTXT, 0.68f, b2);
        }
    } else {
        // Página Simples Vinculada
        if (thumbLeft) {
            float tw = (float)vita2d_texture_get_width(thumbLeft);
            float th = (float)vita2d_texture_get_height(thumbLeft);
            if (tw > 0 && th > 0) {
                float scale = std::min((innerW - 4.0f) / tw, (halfH - 4.0f) / th);
                float renderW = tw * scale;
                float renderH = th * scale;
                float rx = drawX + 6.0f + (innerW - renderW) / 2.0f;
                float ry = linkAreaY + (halfH - renderH) / 2.0f;
                vita2d_draw_texture_scale(thumbLeft, rx, ry, scale, scale);
            }
        }

        // Badge do número
        if (pgf) {
            char buf[24];
            snprintf(buf, sizeof(buf), "#%d", page.fileLinkLeftPage + 1);
            float bW = vita2d_pgf_text_width(pgf, 0.75f, buf) + 10.0f;
            drawRoundedBox(drawX + 8.0f, linkAreaY + 4.0f, bW, 18.0f, 4.0f, applyAlpha(UITheme::Background, 0.85f));
            vita2d_pgf_draw_text(pgf, drawX + 13.0f, linkAreaY + 17.0f, UITheme::BadgeTXT, 0.75f, buf);

            if (page.isFileLeftDualPage) {
                drawRoundedBox(drawX + innerW - 42.0f, linkAreaY + 4.0f, 44.0f, 18.0f, 4.0f, UITheme::BadgeCBZ);
                vita2d_pgf_draw_text(pgf, drawX + innerW - 38.0f, linkAreaY + 17.0f, RGBA8(0,0,0,255), 0.70f, "DUPLA");
            }
        }
    }
}

void UIComponents::drawUnlinkedCard(
    float x,
    float y,
    float w,
    float h,
    int index,
    const LinkedPage& page,
    vita2d_texture* thumb,
    bool isSelected,
    bool isDropTarget,
    bool isGrabbed
) {
    float drawX = x;
    float drawY = y;
    float drawW = w;
    float drawH = h;

    if (isGrabbed) {
        float expandW = 8.0f;
        float expandH = 10.0f;
        drawX = x - expandW / 2.0f;
        drawY = y - expandH / 2.0f;
        drawW = w + expandW;
        drawH = h + expandH;
    }

    unsigned int bg = isSelected ? UITheme::SurfaceActive : UITheme::Surface;
    if (isGrabbed) {
        drawRoundedBox(drawX - 3.0f, drawY - 3.0f, drawW + 6.0f, drawH + 6.0f, 10.0f, applyAlpha(UITheme::Primary, 0.50f));
    }
    drawRoundedBox(drawX, drawY, drawW, drawH, 6.0f, bg);

    if (isDropTarget) {
        drawRoundedBox(drawX - 2.0f, drawY - 2.0f, drawW + 4.0f, drawH + 4.0f, 8.0f, UITheme::Primary);
        drawRoundedBox(drawX, drawY, drawW, drawH, 6.0f, UITheme::SurfaceActive);
    } else if (isGrabbed) {
        drawRoundedBox(drawX - 2.0f, drawY - 2.0f, drawW + 4.0f, drawH + 4.0f, 8.0f, UITheme::Primary);
        drawRoundedBox(drawX, drawY, drawW, drawH, 6.0f, UITheme::SurfaceActive);
    } else if (isSelected) {
        vita2d_draw_line(drawX, drawY, drawX + drawW, drawY, UITheme::Primary);
        vita2d_draw_line(drawX, drawY + drawH, drawX + drawW, drawY + drawH, UITheme::Primary);
        vita2d_draw_line(drawX, drawY, drawX, drawY + drawH, UITheme::Primary);
        vita2d_draw_line(drawX + drawW, drawY, drawX + drawW, drawY + drawH, UITheme::Primary);
    }

    float innerW = drawW - 6.0f;
    float innerH = drawH - 6.0f;
    drawRoundedBox(drawX + 3.0f, drawY + 3.0f, innerW, innerH, 4.0f, UITheme::CoverBg);

    if (thumb) {
        float tw = (float)vita2d_texture_get_width(thumb);
        float th = (float)vita2d_texture_get_height(thumb);
        if (tw > 0 && th > 0) {
            float scale = std::min((innerW - 2.0f) / tw, (innerH - 2.0f) / th);
            float renderW = tw * scale;
            float renderH = th * scale;
            float rx = drawX + 3.0f + (innerW - renderW) / 2.0f;
            float ry = drawY + 3.0f + (innerH - renderH) / 2.0f;
            vita2d_draw_texture_scale(thumb, rx, ry, scale, scale);
        }
    }

    // Badge
    if (pgf) {
        char buf[16];
        snprintf(buf, sizeof(buf), "#%d", page.fileLinkLeftPage + 1);
        float bW = vita2d_pgf_text_width(pgf, 0.70f, buf) + 8.0f;
        drawRoundedBox(drawX + 4.0f, drawY + 4.0f, bW, 16.0f, 3.0f, applyAlpha(UITheme::Background, 0.90f));
        vita2d_pgf_draw_text(pgf, drawX + 8.0f, drawY + 16.0f, UITheme::BadgeTXT, 0.70f, buf);
    }
}

UIDetailLayout UIComponents::getDetailLayout(bool isRotated) {
    UIDetailLayout layout;
    float sW = isRotated ? 544.0f : 960.0f;
    float sH = isRotated ? 960.0f : 544.0f;

    layout.backBtn = {16.0f, 10.0f, 90.0f, 32.0f};

    if (!isRotated) {
        layout.coverCard = {24.0f, 54.0f, 140.0f, 200.0f};

        float btnY = 262.0f;
        float btnW = 145.0f;
        float btnH = 34.0f;
        float gap = 8.0f;
        float startX = 24.0f;

        layout.btnRead         = {startX, btnY, btnW, btnH};
        layout.btnFavorite     = {startX + (btnW + gap), btnY, btnW, btnH};
        layout.btnMarkRead     = {startX + (btnW + gap) * 2.0f, btnY, btnW, btnH};
        layout.btnClearHistory = {startX + (btnW + gap) * 3.0f, btnY, btnW, btnH};
        layout.btnLink         = {startX + (btnW + gap) * 4.0f, btnY, btnW, btnH};
        layout.btnDelete       = {startX + (btnW + gap) * 5.0f, btnY, btnW, btnH};

        float tabY = 306.0f;
        float tabW = 296.0f;
        float tabH = 32.0f;
        layout.tabInfo     = {24.0f, tabY, tabW, tabH};
        layout.tabChapters = {24.0f + tabW + 12.0f, tabY, tabW, tabH};
        layout.tabLink     = {24.0f + (tabW + 12.0f) * 2.0f, tabY, tabW, tabH};

        layout.contentArea = {24.0f, 346.0f, sW - 48.0f, sH - 346.0f - 32.0f};
    } else {
        layout.coverCard = {20.0f, 54.0f, 130.0f, 186.0f};

        float btnY = 250.0f;
        float btnW = 156.0f;
        float btnH = 34.0f;
        float gapX = 12.0f;
        float gapY = 8.0f;
        float startX = 20.0f;

        layout.btnRead         = {startX, btnY, btnW, btnH};
        layout.btnFavorite     = {startX + btnW + gapX, btnY, btnW, btnH};
        layout.btnMarkRead     = {startX + (btnW + gapX) * 2.0f, btnY, btnW, btnH};
        layout.btnClearHistory = {startX, btnY + btnH + gapY, btnW, btnH};
        layout.btnLink         = {startX + btnW + gapX, btnY + btnH + gapY, btnW, btnH};
        layout.btnDelete       = {startX + (btnW + gapX) * 2.0f, btnY + btnH + gapY, btnW, btnH};

        float tabY = 336.0f;
        float tabW = 162.0f;
        float tabH = 32.0f;
        layout.tabInfo     = {20.0f, tabY, tabW, tabH};
        layout.tabChapters = {20.0f + tabW + 9.0f, tabY, tabW, tabH};
        layout.tabLink     = {20.0f + (tabW + 9.0f) * 2.0f, tabY, tabW, tabH};

        layout.contentArea = {20.0f, 376.0f, sW - 40.0f, sH - 376.0f - 32.0f};
    }

    return layout;
}

void UIComponents::drawDetailScreen(
    const LibraryItem& item,
    const DetailMetadata& meta,
    vita2d_texture* coverTex,
    int activeTab,
    int selectedActionBtn,
    int selectedChapterIndex,
    float contentScrollY,
    bool isCoverModalOpen,
    bool isRotated
) {
    float sW = isRotated ? 544.0f : 960.0f;
    float sH = isRotated ? 960.0f : 544.0f;

    UIDetailLayout layout = getDetailLayout(isRotated);

    // 1. Top Bar com Botão Voltar e Título
    drawRoundedBox(layout.backBtn.x, layout.backBtn.y, layout.backBtn.w, layout.backBtn.h, 6.0f, UITheme::Surface);
    if (pgf) {
        vita2d_pgf_draw_text(pgf, layout.backBtn.x + 12.0f, layout.backBtn.y + 22.0f, UITheme::TextPrimary, 0.90f, "< Voltar");
    }

    float badgeX = layout.backBtn.x + layout.backBtn.w + 12.0f;
    drawBadge(badgeX, layout.backBtn.y + 4.0f, item.typeString, 1.0f);

    float titleX = badgeX + getBadgeWidth(item.typeString) + 14.0f;
    if (pgf) {
        std::string dispTitle = !meta.title.empty() ? meta.title : item.filename;
        if (dispTitle.size() > 50) dispTitle = dispTitle.substr(0, 47) + "...";
        vita2d_pgf_draw_text(pgf, titleX, layout.backBtn.y + 22.0f, UITheme::TextPrimary, 0.95f, dispTitle.c_str());
    }

    if (item.isFavorite) {
        drawStar(sW - 44.0f, layout.backBtn.y + 6.0f, 20.0f, UITheme::FavoriteGold);
    }

    // 2. Seção Hero (Capa Grande + Detalhes Principais + Progresso)
    drawRoundedBox(layout.coverCard.x - 2.0f, layout.coverCard.y - 2.0f, layout.coverCard.w + 4.0f, layout.coverCard.h + 4.0f, 8.0f, UITheme::SurfaceActive);
    drawRoundedBox(layout.coverCard.x, layout.coverCard.y, layout.coverCard.w, layout.coverCard.h, 6.0f, UITheme::CoverBg);

    if (coverTex) {
        float tw = (float)vita2d_texture_get_width(coverTex);
        float th = (float)vita2d_texture_get_height(coverTex);
        if (tw > 0 && th > 0) {
            float scale = std::min(layout.coverCard.w / tw, layout.coverCard.h / th);
            float rw = tw * scale;
            float rh = th * scale;
            float rx = layout.coverCard.x + (layout.coverCard.w - rw) / 2.0f;
            float ry = layout.coverCard.y + (layout.coverCard.h - rh) / 2.0f;
            vita2d_draw_texture_scale(coverTex, rx, ry, scale, scale);
        }
    } else {
        drawBadge(layout.coverCard.x + (layout.coverCard.w - getBadgeWidth(item.typeString, 1.4f)) / 2.0f,
                  layout.coverCard.y + layout.coverCard.h / 2.0f - 14.0f,
                  item.typeString, 1.4f);
    }

    // Informações ao lado da Capa
    float infoX = layout.coverCard.x + layout.coverCard.w + 18.0f;
    float infoW = sW - infoX - 24.0f;
    float curY = layout.coverCard.y + 20.0f;

    if (pgf) {
        std::string heroTitle = !meta.title.empty() ? meta.title : item.filename;
        if (heroTitle.size() > 42) heroTitle = heroTitle.substr(0, 39) + "...";
        vita2d_pgf_draw_text(pgf, infoX, curY, UITheme::TextPrimary, 1.15f, heroTitle.c_str());
        curY += 24.0f;

        if (!meta.series.empty()) {
            std::string seriesStr = "Serie: " + meta.series;
            if (meta.volume > 0) seriesStr += " • Vol. " + std::to_string(meta.volume);
            if (!meta.number.empty()) seriesStr += " #" + meta.number;
            if (seriesStr.size() > 48) seriesStr = seriesStr.substr(0, 45) + "...";
            vita2d_pgf_draw_text(pgf, infoX, curY, UITheme::Secondary, 0.95f, seriesStr.c_str());
            curY += 22.0f;
        }

        std::string authorStr = !meta.authors.empty() ? meta.authors : (!meta.writer.empty() ? meta.writer : "");
        if (!authorStr.empty()) {
            if (authorStr.size() > 50) authorStr = authorStr.substr(0, 47) + "...";
            vita2d_pgf_draw_text(pgf, infoX, curY, UITheme::TextSecondary, 0.90f, authorStr.c_str());
            curY += 22.0f;
        }

        char progBuf[128];
        int totPages = (item.totalPages > 0) ? item.totalPages : (meta.count > 0 ? meta.count : 0);
        if (totPages > 0) {
            snprintf(progBuf, sizeof(progBuf), "Progresso: Pagina %d de %d  (%.1f%%)", item.currentPage + 1, totPages, item.progressPercent);
        } else {
            snprintf(progBuf, sizeof(progBuf), "Nao iniciado  (0%%)");
        }
        vita2d_pgf_draw_text(pgf, infoX, curY, UITheme::TextSecondary, 0.85f, progBuf);
        curY += 12.0f;

        // Barra de progresso visual
        float pBarW = std::min(infoW, 360.0f);
        float pBarH = 7.0f;
        drawRoundedBox(infoX, curY, pBarW, pBarH, 3.5f, UITheme::ProgressBarBg);
        if (item.progressPercent > 0.0f) {
            float fillW = std::max(7.0f, (item.progressPercent / 100.0f) * pBarW);
            drawRoundedBox(infoX, curY, fillW, pBarH, 3.5f, UITheme::Primary);
        }
        curY += 20.0f;

        char subInfo[128];
        snprintf(subInfo, sizeof(subInfo), "Tamanho: %s  •  Ultima leitura: %s", item.formattedSize.c_str(), item.lastReadFormatted.c_str());
        vita2d_pgf_draw_text(pgf, infoX, curY, UITheme::TextSecondary, 0.80f, subInfo);
    }

    // 3. Barra de Ações Rápidas
    auto drawActionBtn = [&](const UIRect& r, const char* label, bool isSelected, bool isPrimary = false) {
        unsigned int bg = isPrimary ? UITheme::Primary : (isSelected ? UITheme::SurfaceActive : UITheme::Surface);
        unsigned int fg = isPrimary ? UITheme::OnPrimary : (isSelected ? UITheme::Primary : UITheme::TextPrimary);
        if (isSelected && !isPrimary) {
            drawRoundedBox(r.x - 2.0f, r.y - 2.0f, r.w + 4.0f, r.h + 4.0f, 8.0f, UITheme::Primary);
        }
        drawRoundedBox(r.x, r.y, r.w, r.h, 6.0f, bg);
        if (pgf) {
            float tw = vita2d_pgf_text_width(pgf, 0.85f, label);
            vita2d_pgf_draw_text(pgf, r.x + (r.w - tw) / 2.0f, r.y + 22.0f, fg, 0.85f, label);
        }
    };

    const char* readLabel = (item.currentPage > 0) ? "Continuar" : "Ler";
    drawActionBtn(layout.btnRead, readLabel, selectedActionBtn == 0, true);
    drawActionBtn(layout.btnFavorite, item.isFavorite ? "Favoritado" : "Favoritar", selectedActionBtn == 1);
    drawActionBtn(layout.btnMarkRead, (item.progressPercent >= 99.0f) ? "Lido" : "Marcar Lido", selectedActionBtn == 2);
    drawActionBtn(layout.btnClearHistory, "Limpar Hist.", selectedActionBtn == 3);
    drawActionBtn(layout.btnLink, "Vincular", selectedActionBtn == 4);
    drawActionBtn(layout.btnDelete, "Excluir", selectedActionBtn == 5);

    // 4. Seletor de Abas
    auto drawTab = [&](const UIRect& r, const char* label, bool isActive) {
        unsigned int bg = isActive ? UITheme::SurfaceActive : UITheme::Surface;
        unsigned int fg = isActive ? UITheme::Primary : UITheme::TextSecondary;
        drawRoundedBox(r.x, r.y, r.w, r.h, 6.0f, bg);
        if (isActive) {
            drawRoundedBox(r.x + 8.0f, r.y + r.h - 3.0f, r.w - 16.0f, 3.0f, 1.5f, UITheme::Primary);
        }
        if (pgf) {
            float tw = vita2d_pgf_text_width(pgf, 0.90f, label);
            vita2d_pgf_draw_text(pgf, r.x + (r.w - tw) / 2.0f, r.y + 22.0f, fg, 0.90f, label);
        }
    };

    char chTabTitle[64];
    snprintf(chTabTitle, sizeof(chTabTitle), "Capitulos (%d)", static_cast<int>(meta.chapters.size()));

    drawTab(layout.tabInfo, "Informacoes", activeTab == 0);
    drawTab(layout.tabChapters, chTabTitle, activeTab == 1);
    drawTab(layout.tabLink, "Vinculo Bilingue", activeTab == 2);

    // 5. Área de Conteúdo da Aba Ativa (com rolagem)
    drawRoundedBox(layout.contentArea.x, layout.contentArea.y, layout.contentArea.w, layout.contentArea.h, 8.0f, UITheme::Surface);

    float clipX = layout.contentArea.x + 14.0f;
    float clipY = layout.contentArea.y + 14.0f;
    float clipW = layout.contentArea.w - 28.0f;
    float clipH = layout.contentArea.h - 28.0f;

    if (activeTab == 0) {
        // --- ABA 0: INFORMAÇÕES & SINOPSE & TAGS ---
        float y = clipY - contentScrollY;

        // Tags / Gêneros / Chips
        if (!meta.genres.empty() || !meta.tags.empty()) {
            std::vector<std::string> allChips = meta.genres;
            allChips.insert(allChips.end(), meta.tags.begin(), meta.tags.end());

            float chipX = clipX;
            for (const auto& chip : allChips) {
                if (chip.empty()) continue;
                float chipTextW = vita2d_pgf_text_width(pgf, 0.78f, chip.c_str());
                float chipW = chipTextW + 16.0f;
                float chipH = 22.0f;
                if (chipX + chipW > clipX + clipW) {
                    chipX = clipX;
                    y += 28.0f;
                }
                if (y + chipH >= clipY && y <= clipY + clipH) {
                    drawRoundedBox(chipX, y, chipW, chipH, 11.0f, UITheme::SurfaceActive);
                    vita2d_pgf_draw_text(pgf, chipX + 8.0f, y + 16.0f, UITheme::Secondary, 0.78f, chip.c_str());
                }
                chipX += chipW + 8.0f;
            }
            y += 34.0f;
        }

        // Sinopse
        std::string syn = !meta.summary.empty() ? meta.summary : meta.notes;
        if (!syn.empty() && pgf) {
            if (y + 20.0f >= clipY && y <= clipY + clipH) {
                vita2d_pgf_draw_text(pgf, clipX, y + 16.0f, UITheme::Primary, 0.95f, "Sinopse");
            }
            y += 24.0f;

            // Quebra de linhas aproximada por largura
            std::istringstream stream(syn);
            std::string line;
            while (std::getline(stream, line)) {
                if (line.empty()) { y += 12.0f; continue; }
                std::string word;
                std::istringstream wordStream(line);
                std::string curLine;
                while (wordStream >> word) {
                    std::string testLine = curLine.empty() ? word : (curLine + " " + word);
                    float tw = vita2d_pgf_text_width(pgf, 0.82f, testLine.c_str());
                    if (tw > clipW) {
                        if (y + 18.0f >= clipY && y <= clipY + clipH) {
                            vita2d_pgf_draw_text(pgf, clipX, y + 16.0f, UITheme::TextPrimary, 0.82f, curLine.c_str());
                        }
                        y += 20.0f;
                        curLine = word;
                    } else {
                        curLine = testLine;
                    }
                }
                if (!curLine.empty()) {
                    if (y + 18.0f >= clipY && y <= clipY + clipH) {
                        vita2d_pgf_draw_text(pgf, clipX, y + 16.0f, UITheme::TextPrimary, 0.82f, curLine.c_str());
                    }
                    y += 20.0f;
                }
            }
            y += 14.0f;
        }

        // Ficha Técnica Completa
        if (pgf) {
            if (y + 20.0f >= clipY && y <= clipY + clipH) {
                vita2d_pgf_draw_text(pgf, clipX, y + 16.0f, UITheme::Primary, 0.95f, "Ficha Tecnica");
            }
            y += 24.0f;

            auto drawField = [&](const char* label, const std::string& val) {
                if (val.empty()) return;
                if (y + 18.0f >= clipY && y <= clipY + clipH) {
                    vita2d_pgf_draw_text(pgf, clipX, y + 16.0f, UITheme::TextSecondary, 0.82f, label);
                    vita2d_pgf_draw_text(pgf, clipX + 160.0f, y + 16.0f, UITheme::TextPrimary, 0.82f, val.c_str());
                }
                y += 20.0f;
            };

            drawField("Editora:", meta.publisher);
            drawField("Selo / Imprint:", meta.imprint);
            drawField("Data:", meta.releaseDate);
            drawField("Idioma:", meta.language);
            drawField("ISBN:", meta.isbn);
            drawField("Classificacao:", meta.ageRating);
            drawField("Direcao Manga:", meta.mangaDirection);
            drawField("Arco de Historia:", meta.storyArc);
            drawField("Grupo de Serie:", meta.seriesGroup);
            drawField("Roteiro (Writer):", meta.writer);
            drawField("Desenho (Penciller):", meta.penciller);
            drawField("Arte-Final (Inker):", meta.inker);
            drawField("Cores (Colorist):", meta.colorist);
            drawField("Letras (Letterer):", meta.letterer);
            drawField("Capa (Cover Artist):", meta.coverArtist);
            drawField("Editor:", meta.editor);
            drawField("Traducao:", meta.translator);
            drawField("Caminho do Arquivo:", item.fullPath);
        }
    } else if (activeTab == 1) {
        // --- ABA 1: CAPÍTULOS & MARCADORES ---
        if (meta.chapters.empty()) {
            if (pgf) {
                vita2d_pgf_draw_text(pgf, clipX + 20.0f, clipY + 40.0f, UITheme::TextSecondary, 0.90f, "Nenhum capitulo ou marcador encontrado neste arquivo.");
            }
        } else {
            float rowH = 34.0f;
            for (size_t i = 0; i < meta.chapters.size(); ++i) {
                const auto& ch = meta.chapters[i];
                float rowY = clipY + i * rowH - contentScrollY;

                if (rowY + rowH >= clipY && rowY <= clipY + clipH) {
                    bool isSel = (selectedChapterIndex == static_cast<int>(i));
                    bool isCurPage = (item.currentPage >= ch.pageIndex &&
                                      (i + 1 >= meta.chapters.size() || item.currentPage < meta.chapters[i + 1].pageIndex));

                    unsigned int rowBg = isSel ? UITheme::SurfaceActive : (isCurPage ? applyAlpha(UITheme::Primary, 0.15f) : UITheme::Surface);
                    if (isSel) {
                        drawRoundedBox(clipX - 2.0f, rowY - 1.0f, clipW + 4.0f, rowH - 2.0f, 6.0f, UITheme::Primary);
                    }
                    drawRoundedBox(clipX, rowY, clipW, rowH - 4.0f, 5.0f, rowBg);

                    if (pgf) {
                        char idxBuf[16];
                        snprintf(idxBuf, sizeof(idxBuf), "%02d.", static_cast<int>(i + 1));
                        vita2d_pgf_draw_text(pgf, clipX + 10.0f, rowY + 21.0f, UITheme::Secondary, 0.80f, idxBuf);

                        float textStartX = clipX + 44.0f;
                        if (ch.isBookmark) {
                            drawRoundedBox(textStartX, rowY + 5.0f, 64.0f, 18.0f, 3.0f, UITheme::BadgeCBZ);
                            vita2d_pgf_draw_text(pgf, textStartX + 4.0f, rowY + 18.0f, UITheme::OnPrimary, 0.65f, "Marcador");
                            textStartX += 70.0f;
                        }

                        std::string chTitle = ch.title;
                        if (chTitle.size() > 44) chTitle = chTitle.substr(0, 41) + "...";
                        vita2d_pgf_draw_text(pgf, textStartX, rowY + 21.0f, isSel ? UITheme::Primary : UITheme::TextPrimary, 0.85f, chTitle.c_str());

                        char pBuf[32];
                        snprintf(pBuf, sizeof(pBuf), "Pag. %d", ch.pageIndex + 1);
                        float pw = vita2d_pgf_text_width(pgf, 0.78f, pBuf);
                        vita2d_pgf_draw_text(pgf, clipX + clipW - pw - 14.0f, rowY + 21.0f, UITheme::TextSecondary, 0.78f, pBuf);
                    }
                }
            }
        }
    } else if (activeTab == 2) {
        // --- ABA 2: VÍNCULO BILÍNGUE ---
        float y = clipY + 20.0f - contentScrollY;
        if (pgf) {
            vita2d_pgf_draw_text(pgf, clipX, y, UITheme::Primary, 1.0f, "Status do Vinculo Bilingue (PagesLink)");
            y += 28.0f;

            if (meta.hasLinkedFile) {
                vita2d_pgf_draw_text(pgf, clipX, y, UITheme::BadgeTXT, 0.90f, "[ Vinculado ]");
                y += 24.0f;
                vita2d_pgf_draw_text(pgf, clipX, y, UITheme::TextSecondary, 0.85f, "Arquivo Secundario:");
                vita2d_pgf_draw_text(pgf, clipX + 160.0f, y, UITheme::TextPrimary, 0.85f, meta.linkedFileName.c_str());
                y += 24.0f;
                char pBuf[64];
                snprintf(pBuf, sizeof(pBuf), "%d paginas", meta.linkedPageCount);
                vita2d_pgf_draw_text(pgf, clipX, y, UITheme::TextSecondary, 0.85f, "Total de Paginas:");
                vita2d_pgf_draw_text(pgf, clipX + 160.0f, y, UITheme::TextPrimary, 0.85f, pBuf);
                y += 30.0f;
                vita2d_pgf_draw_text(pgf, clipX, y, UITheme::TextSecondary, 0.82f, "Voce pode alternar entre as versoes durante a leitura usando Quadrado ou Triangulo.");
            } else {
                vita2d_pgf_draw_text(pgf, clipX, y, UITheme::TextSecondary, 0.90f, "Nenhum arquivo secundario vinculado atualmente.");
                y += 26.0f;
                vita2d_pgf_draw_text(pgf, clipX, y, UITheme::TextSecondary, 0.82f, "Clique no botao 'Vincular' acima para associar uma versao em outro idioma e ler simultaneamente.");
            }
        }
    }

    // 6. Modal de Zoom da Capa em Tela Cheia (se ativo)
    if (isCoverModalOpen && coverTex) {
        vita2d_draw_rectangle(0, 0, sW, sH, applyAlpha(UITheme::Background, 0.94f));
        float tw = (float)vita2d_texture_get_width(coverTex);
        float th = (float)vita2d_texture_get_height(coverTex);
        if (tw > 0 && th > 0) {
            float maxW = sW - 40.0f;
            float maxH = sH - 50.0f;
            float scale = std::min(maxW / tw, maxH / th);
            float rw = tw * scale;
            float rh = th * scale;
            float rx = (sW - rw) / 2.0f;
            float ry = (sH - rh) / 2.0f - 10.0f;
            drawRoundedBox(rx - 4.0f, ry - 4.0f, rw + 8.0f, rh + 8.0f, 10.0f, UITheme::Primary);
            vita2d_draw_texture_scale(coverTex, rx, ry, scale, scale);
        }
        if (pgf) {
            const char* tip = "Toque ou pressione Circulo / Triangulo para fechar a visualizacao da capa";
            float tw = vita2d_pgf_text_width(pgf, 0.80f, tip);
            vita2d_pgf_draw_text(pgf, (sW - tw) / 2.0f, sH - 16.0f, UITheme::TextSecondary, 0.80f, tip);
        }
    }

    // 7. Rodapé com Instruções de Controle
    const char* footerHint = isCoverModalOpen
        ? "O/TRI: Fechar Capa"
        : "D-Pad/L/R: Navegar | X: Confirmar | TRI: Ver Capa | O: Voltar";
    drawFooter(footerHint, false);
}

void UIComponents::drawPagesLinkScrollBar(float x, float y, float w, float h, float scrollPos, float totalRange) {
    if (totalRange <= w) return;

    // Fundo / Trilho da barra de rolagem
    drawRoundedBox(x, y, w, h, h / 2.0f, UITheme::ProgressBarBg);

    float thumbW = std::max(32.0f, (w / totalRange) * w);
    float maxScroll = totalRange - w;
    float thumbX = x + (scrollPos / maxScroll) * (w - thumbW);

    if (thumbX < x) thumbX = x;
    if (thumbX + thumbW > x + w) thumbX = x + w - thumbW;

    // Polegar da barra de rolagem
    drawRoundedBox(thumbX, y, thumbW, h, h / 2.0f, UITheme::Primary);
}

void UIComponents::drawDragGhost(float x, float y, float w, float h, vita2d_texture* thumb, const std::string& label) {
    float gx = x - w / 2.0f;
    float gy = y - h / 2.0f;

    // Sombra / Brilho do Card Flutuante
    drawRoundedBox(gx - 4.0f, gy - 4.0f, w + 8.0f, h + 8.0f, 10.0f, applyAlpha(UITheme::Primary, 0.40f));
    drawRoundedBox(gx, gy, w, h, 8.0f, applyAlpha(UITheme::SurfaceActive, 0.85f));

    if (thumb) {
        float tw = (float)vita2d_texture_get_width(thumb);
        float th = (float)vita2d_texture_get_height(thumb);
        if (tw > 0 && th > 0) {
            float scale = std::min((w - 8.0f) / tw, (h - 8.0f) / th);
            float renderW = tw * scale;
            float renderH = th * scale;
            float rx = gx + (w - renderW) / 2.0f;
            float ry = gy + (h - renderH) / 2.0f;
            vita2d_draw_texture_scale(thumb, rx, ry, scale, scale);
        }
    }

    if (pgf && !label.empty()) {
        float tW = vita2d_pgf_text_width(pgf, 0.80f, label.c_str()) + 10.0f;
        drawRoundedBox(gx + (w - tW) / 2.0f, gy + h - 22.0f, tW, 18.0f, 4.0f, applyAlpha(UITheme::Background, 0.90f));
        vita2d_pgf_draw_text(pgf, gx + (w - tW) / 2.0f + 5.0f, gy + h - 8.0f, UITheme::Primary, 0.80f, label.c_str());
    }
}
