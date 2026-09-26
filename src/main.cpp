#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ime_dialog.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#include "ui_components.h"
#include "config_manager.h"
#include "file_browser.h"
#include "reader_txt.h"
#include "reader_cbz.h"
#include "reader_epub.h"

enum class AppState {
    MENU,
    SETTINGS,
    READ_TXT,
    READ_CBZ,
    READ_EPUB
};

enum class FocusArea {
    SEARCH,
    SORT,
    LAYOUT,
    SETTINGS,
    REFRESH,
    CONTENT
};

// Variáveis e rotinas para o teclado nativo (SceImeDialog)
static int ime_active = 0;
static uint16_t ime_title_utf16[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
static uint16_t ime_text_utf16[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
static uint16_t ime_initial_utf16[SCE_IME_DIALOG_MAX_TEXT_LENGTH];

static void utf8_to_utf16(const uint8_t *src, uint16_t *dst) {
    int i = 0;
    for (i = 0; src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = 0;
}

static void utf16_to_utf8(const uint16_t *src, uint8_t *dst) {
    int i = 0;
    for (i = 0; src[i]; i++) {
        dst[i] = (uint8_t)src[i];
    }
    dst[i] = 0;
}

static void launch_ime(const char* title, const char* initial_text) {
    SceImeDialogParam param;
    sceImeDialogParamInit(&param);
    param.sdkVersion = 0x03570011;
    param.supportedLanguages = 0x0001FFFF;
    param.languagesForced = SCE_FALSE;
    param.type = SCE_IME_TYPE_DEFAULT;
    param.option = 0;

    utf8_to_utf16((const uint8_t*)title, ime_title_utf16);
    param.title = ime_title_utf16;

    utf8_to_utf16((const uint8_t*)initial_text, ime_initial_utf16);
    param.initialText = ime_initial_utf16;

    param.maxTextLength = 128;
    param.inputTextBuffer = ime_text_utf16;

    sceImeDialogInit(&param);
    ime_active = 1;
}

int main(int argc, char* argv[]) {
    vita2d_init();
    vita2d_set_clear_color(UITheme::Background);

    // Carrega configurações salvas
    ConfigManager::getInstance().load();
    AppConfig& appConfig = ConfigManager::getInstance().getConfig();

    // Habilita amostragem do Touch Frontal
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);

    vita2d_pgf* pgf = vita2d_load_default_pgf();
    UIComponents::init(pgf);

    AppState currentState = AppState::MENU;
    FocusArea currentFocus = FocusArea::CONTENT;
    int settingsSelectedIndex = 0;

    std::vector<LibraryItem> allItems = FileBrowser::scanLibrary();
    SortMode currentSort = appConfig.sortMode;
    bool isGridView = appConfig.isGridView;
    std::string searchQuery = "";

    FileBrowser::sortItems(allItems, currentSort);
    std::vector<LibraryItem> visibleItems = FileBrowser::filterItems(allItems, searchQuery);

    int selectedIndex = 0;
    int scrollOffset = 0;
    float smoothScrollOffset = 0.0f;
    float scrollBarAlpha = 0.0f;
    int scrollBarTimer = 0;

    // Variáveis de controle de Gestos Touch
    bool isTouching = false;
    int touchStartX = 0;
    int touchStartY = 0;
    int dragStartOffset = 0;
    bool isDraggingY = false;
    bool isDraggingX = false;
    int initialTouchItemIndex = -1;

    // Variáveis de Gestos Touch para Leitores
    bool readerIsTouching = false;
    int readerTouchStartX = 0;
    int readerTouchStartY = 0;
    int readerLastTouchX = 0;
    int readerLastTouchY = 0;
    bool readerIsDragging = false;
    bool readerIsPinching = false;
    float readerLastPinchDist = 0.0f;
    bool readerFullscreen = false;
    bool readerRotated = appConfig.readerRotated;

    // Estado do Diálogo de Exclusão (Popup)
    bool showDeleteConfirm = false;
    int itemToDeleteIndex = -1;
    bool deleteConfirmYesSelected = false;

    ReaderTXT readerTxt;
    ReaderCBZ readerCbz;
    ReaderEPUB readerEpub;


    // Texture para rotacao da UI
    vita2d_texture* uiRenderTarget = vita2d_create_empty_texture_rendertarget(544, 960, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR);
    bool analogRotationTriggered = false;
    int epubFontTimer = 0;

    SceCtrlData pad = {0};
    SceCtrlData oldPad = {0};
    SceTouchData touch = {0};
    int oldTouchNum = 0;

    auto openItem = [&](int idx) {
        if (idx >= 0 && idx < static_cast<int>(visibleItems.size())) {
            selectedIndex = idx;
            auto& selected = visibleItems[idx];
            FileBrowser::markAsRead(selected);
            for (auto& orig : allItems) {
                if (orig.filename == selected.filename) {
                    orig.lastReadTime = selected.lastReadTime;
                    break;
                }
            }
            FileBrowser::saveMetadata(allItems);

            if (selected.type == FileType::TXT) {
                if (readerTxt.loadFile(selected.fullPath, pgf)) {
                    readerTxt.setRotated(readerRotated, pgf);
                    currentState = AppState::READ_TXT;
                }
            } else if (FileBrowser::isMangaType(selected.type)) {
                if (readerCbz.loadFile(selected.fullPath)) {
                    readerCbz.setRotated(readerRotated);
                    currentState = AppState::READ_CBZ;
                }
            } else if (selected.type == FileType::EPUB) {
                if (readerEpub.loadFile(selected.fullPath, pgf)) {
                    readerEpub.setRotated(readerRotated);
                    currentState = AppState::READ_EPUB;
                }
            }
        }
    };

    auto openPrevBook = [&]() {
        if (visibleItems.size() <= 1) return;
        int nextIdx = selectedIndex - 1;
        if (nextIdx < 0) nextIdx = static_cast<int>(visibleItems.size()) - 1;
        if (nextIdx != selectedIndex) {
            openItem(nextIdx);
        }
    };

    auto openNextBook = [&]() {
        if (visibleItems.size() <= 1) return;
        int nextIdx = selectedIndex + 1;
        if (nextIdx >= static_cast<int>(visibleItems.size())) nextIdx = 0;
        if (nextIdx != selectedIndex) {
            openItem(nextIdx);
        }
    };


    auto startDrawing = [&]() {
        if (readerRotated && uiRenderTarget) {
            vita2d_start_drawing_advanced(uiRenderTarget, 0);
        } else {
            vita2d_start_drawing();
        }
    };
    
    auto endDrawing = [&]() {
        vita2d_end_drawing();
        if (readerRotated && uiRenderTarget) {
            vita2d_start_drawing();
            vita2d_clear_screen();
            // Desenha a textura girada em 90 graus horario. O centro eh a metade da tela do PS Vita
            vita2d_draw_texture_rotate(uiRenderTarget, 480.0f, 272.0f, 1.57079632679f);
            vita2d_end_drawing();
        }
        vita2d_swap_buffers();
    };

    auto toggleRotation = [&]() {
        readerRotated = !readerRotated;
        UIComponents::setRotated(readerRotated);
        readerTxt.setRotated(readerRotated, pgf);
        readerCbz.setRotated(readerRotated);
        readerEpub.setRotated(readerRotated);
    };

    while (true) {
        // Manipulação do teclado nativo do PS Vita
        if (ime_active) {
            SceCommonDialogStatus status = sceImeDialogGetStatus();
            if (status == SCE_COMMON_DIALOG_STATUS_FINISHED) {
                SceImeDialogResult result;
                sceImeDialogGetResult(&result);
                if (result.button == SCE_IME_DIALOG_BUTTON_ENTER) {
                    uint8_t res_utf8[128];
                    utf16_to_utf8(ime_text_utf16, res_utf8);
                    searchQuery = reinterpret_cast<char*>(res_utf8);
                    visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                    selectedIndex = 0;
                    scrollOffset = 0;
                }
                sceImeDialogTerm();
                ime_active = 0;
            }

            startDrawing();
            vita2d_clear_screen();
            vita2d_common_dialog_update();
            endDrawing();
            continue;
        }

        sceCtrlPeekBufferPositive(0, &pad, 1);

        unsigned int logicalButtons = pad.buttons;
        if (readerRotated) {
            logicalButtons &= ~(SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT | SCE_CTRL_RIGHT);
            if (pad.buttons & SCE_CTRL_UP) logicalButtons |= SCE_CTRL_RIGHT;
            if (pad.buttons & SCE_CTRL_DOWN) logicalButtons |= SCE_CTRL_LEFT;
            if (pad.buttons & SCE_CTRL_LEFT) logicalButtons |= SCE_CTRL_UP;
            if (pad.buttons & SCE_CTRL_RIGHT) logicalButtons |= SCE_CTRL_DOWN;
        }
        unsigned int pressed = logicalButtons & ~oldPad.buttons;


        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
        bool touchDown = (touch.reportNum > 0 && oldTouchNum == 0);
        bool touchHeld = (touch.reportNum > 0 && oldTouchNum > 0);
        bool touchUp   = (touch.reportNum == 0 && oldTouchNum > 0);
        int touchX = 0;
        int touchY = 0;

        if (touch.reportNum > 0) {
            // Converte coordenadas do painel (1920x1088) para a tela (960x544)
            touchX = touch.report[0].x / 2;
            touchY = touch.report[0].y / 2;
            if (readerRotated) {
                // Rotacao 90 graus horario
                int logX = 544 - touchY;
                int logY = touchX;
                touchX = logX;
                touchY = logY;
            }
        }

        oldTouchNum = touch.reportNum;


        int rx = pad.rx;
        int ry = pad.ry;
        if (readerRotated) {
            rx = 255 - pad.ry;
            ry = pad.rx;
        }

        // Rotacao analogico X
        if (rx > 240 || rx < 15) {
            if (!analogRotationTriggered) {
                toggleRotation();
                analogRotationTriggered = true;
            }
        } else if (rx > 64 && rx < 192) {
            analogRotationTriggered = false;
        }

        // Zoom analogico Y
        if (currentState == AppState::READ_CBZ) {
            if (ry < 100) {
                float factor = 1.0f + (100.0f - ry) * 0.0005f;
                readerCbz.addZoom(factor, readerRotated ? 272.0f : 480.0f, readerRotated ? 480.0f : 272.0f);
            } else if (ry > 154) {
                float factor = 1.0f - (ry - 154.0f) * 0.0005f;
                readerCbz.addZoom(factor, readerRotated ? 272.0f : 480.0f, readerRotated ? 480.0f : 272.0f);
            }
        } else if (currentState == AppState::READ_EPUB) {
            if (epubFontTimer > 0) epubFontTimer--;
            if (epubFontTimer == 0) {
                if (ry < 30) {
                    readerEpub.increaseFontSize();
                    epubFontTimer = 15;
                } else if (ry > 225) {
                    readerEpub.decreaseFontSize();
                    epubFontTimer = 15;
                }
            }
        }

        // Processamento de Gestos Touch unificado para os Leitores (TXT, CBZ, EPUB)
        bool readerTapLeft = false;
        bool readerTapRight = false;

        if (currentState != AppState::MENU) {
            if (touch.reportNum >= 2) {
                // Multitoque: Zoom por pinça (Pinch-to-zoom)
                float p0x = touch.report[0].x / 2.0f;
                float p0y = touch.report[0].y / 2.0f;
                float p1x = touch.report[1].x / 2.0f;
                float p1y = touch.report[1].y / 2.0f;
                float dist = std::hypot(p1x - p0x, p1y - p0y);
                float midX = (p0x + p1x) / 2.0f;
                float midY = (p0y + p1y) / 2.0f;

                if (!readerIsPinching) {
                    readerIsPinching = true;
                    readerIsDragging = true;
                    readerLastPinchDist = dist;
                } else if (readerLastPinchDist > 5.0f && dist > 5.0f) {
                    float factor = dist / readerLastPinchDist;
                    if (currentState == AppState::READ_CBZ) {
                        readerCbz.addZoom(factor, midX, midY);
                    }
                    readerLastPinchDist = dist;
                }
            } else if (touch.reportNum == 1) {
                if (touchDown) {
                    readerIsTouching = true;
                    readerTouchStartX = touchX;
                    readerTouchStartY = touchY;
                    readerLastTouchX = touchX;
                    readerLastTouchY = touchY;
                    readerIsDragging = false;
                    readerIsPinching = false;
                    readerLastPinchDist = 0.0f;
                } else if (touchHeld && readerIsTouching && !readerIsPinching) {
                    int totalDx = touchX - readerTouchStartX;
                    int totalDy = touchY - readerTouchStartY;
                    if (std::hypot(totalDx, totalDy) > 25.0f) {
                        readerIsDragging = true;
                    }

                    int dx = touchX - readerLastTouchX;
                    int dy = touchY - readerLastTouchY;
                    if (readerIsDragging && currentState == AppState::READ_CBZ) {
                        readerCbz.addPan(static_cast<float>(dx), static_cast<float>(dy));
                    }
                    readerLastTouchX = touchX;
                    readerLastTouchY = touchY;
                }
            } else if (touchUp && readerIsTouching) {
                // Ao soltar, se foi apenas clique rápido (sem arraste significativo nem pinça)
                if (!readerIsDragging && !readerIsPinching) {
                    // Se não estiver em tela cheia, verifica se tocou nos botões das barras
                    bool handledBarTouch = false;
                    if (!readerFullscreen) {
                        // Barra Superior: Y < 48
                        if (readerTouchStartY < 48) {
                            // Botão de rotação (X: 840..940)
                            if (readerTouchStartX >= 840 && readerTouchStartX <= 940) {
                                toggleRotation();
                                handledBarTouch = true;
                            }
                        }
                        // Barra Inferior (Footer): Y >= 504
                        else if (readerTouchStartY >= 504) {
                            // Botão [L] Livro Ant (X: 680..790)
                            if (readerTouchStartX >= 680 && readerTouchStartX <= 790) {
                                openPrevBook();
                                handledBarTouch = true;
                            }
                            // Botão [R] Prox Livro (X: 810..920)
                            else if (readerTouchStartX >= 810 && readerTouchStartX <= 920) {
                                openNextBook();
                                handledBarTouch = true;
                            }
                        }
                    }

                    if (!handledBarTouch) {
                        // Se não estiver em fullscreen, ignora toques nas bordas das barras (top 48px e bottom 40px)
                        bool canTurnOrToggle = readerFullscreen || (readerTouchStartY >= 48 && readerTouchStartY < 504);
                        if (canTurnOrToggle) {
                            // Tela do PS Vita: 960x544
                            // 25% esquerda: X < 240 -> Página Anterior
                            // 25% direita: X > 720 -> Próxima Página
                            // 50% centro: 240 <= X <= 720 -> Alterna Tela Cheia
                            if (readerTouchStartX < 240) {
                                readerTapLeft = true;
                            } else if (readerTouchStartX > 720) {
                                readerTapRight = true;
                            } else {
                                readerFullscreen = !readerFullscreen;
                            }
                        }
                    }
                }
                readerIsTouching = false;
                readerIsDragging = false;
                readerIsPinching = false;
                readerLastPinchDist = 0.0f;
            }
        }

        if (currentState == AppState::MENU) {
            const int gridCols = 3;
            const int listVisibleCount = 5;
            const int gridVisibleCount = 6; // 3 colunas x 2 linhas

            // GESTÃO DO POPUP DE CONFIRMAÇÃO DE DELEÇÃO
            if (showDeleteConfirm) {
                if (pressed & (SCE_CTRL_LEFT | SCE_CTRL_RIGHT)) {
                    deleteConfirmYesSelected = !deleteConfirmYesSelected;
                }
                if (pressed & SCE_CTRL_CROSS) {
                    if (deleteConfirmYesSelected && itemToDeleteIndex >= 0 && itemToDeleteIndex < static_cast<int>(visibleItems.size())) {
                        const auto item = visibleItems[itemToDeleteIndex];
                        std::string targetPath = item.fullPath;
                        FileBrowser::deleteItem(item);
                        allItems.erase(std::remove_if(allItems.begin(), allItems.end(), [&](const LibraryItem& i) {
                            return i.fullPath == targetPath;
                        }), allItems.end());
                        FileBrowser::saveMetadata(allItems);
                        visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                        if (selectedIndex >= static_cast<int>(visibleItems.size())) {
                            selectedIndex = std::max(0, static_cast<int>(visibleItems.size()) - 1);
                        }
                        int maxOffset = !isGridView ? std::max(0, static_cast<int>(visibleItems.size()) - listVisibleCount)
                                                    : std::max(0, static_cast<int>(visibleItems.size()) - gridVisibleCount);
                        if (scrollOffset > maxOffset) scrollOffset = maxOffset;
                    }
                    showDeleteConfirm = false;
                    itemToDeleteIndex = -1;
                }
                if (pressed & SCE_CTRL_CIRCLE) {
                    showDeleteConfirm = false;
                    itemToDeleteIndex = -1;
                }

                // Toque nos botões do diálogo
                if (touchDown) {
                    float dlgW = 480.0f;
                    float dlgH = 210.0f;
                    float dlgX = (960.0f - dlgW) / 2.0f;
                    float dlgY = (544.0f - dlgH) / 2.0f;
                    float btnY = dlgY + 145.0f;
                    float btnW = 195.0f;
                    float btnH = 42.0f;
                    float btnYesX = dlgX + 24.0f;
                    float btnNoX = dlgX + dlgW - 24.0f - btnW;

                    if (touchX >= btnYesX && touchX <= (btnYesX + btnW) && touchY >= btnY && touchY <= (btnY + btnH)) {
                        if (itemToDeleteIndex >= 0 && itemToDeleteIndex < static_cast<int>(visibleItems.size())) {
                            const auto item = visibleItems[itemToDeleteIndex];
                            std::string targetPath = item.fullPath;
                            FileBrowser::deleteItem(item);
                            allItems.erase(std::remove_if(allItems.begin(), allItems.end(), [&](const LibraryItem& i) {
                                return i.fullPath == targetPath;
                            }), allItems.end());
                            FileBrowser::saveMetadata(allItems);
                            visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                            if (selectedIndex >= static_cast<int>(visibleItems.size())) {
                                selectedIndex = std::max(0, static_cast<int>(visibleItems.size()) - 1);
                            }
                            int maxOffset = !isGridView ? std::max(0, static_cast<int>(visibleItems.size()) - listVisibleCount)
                                                        : std::max(0, static_cast<int>(visibleItems.size()) - gridVisibleCount);
                            if (scrollOffset > maxOffset) scrollOffset = maxOffset;
                        }
                        showDeleteConfirm = false;
                        itemToDeleteIndex = -1;
                    } else if (touchX >= btnNoX && touchX <= (btnNoX + btnW) && touchY >= btnY && touchY <= (btnY + btnH)) {
                        showDeleteConfirm = false;
                        itemToDeleteIndex = -1;
                    }
                }
            } else {
                // PROCESSAMENTO DE TOUCH GESTURES (MENU PRINCIPAL)
                if (touchDown) {
                    isTouching = true;
                    touchStartX = touchX;
                    touchStartY = touchY;
                    dragStartOffset = scrollOffset;
                    isDraggingY = false;
                    isDraggingX = false;
                    initialTouchItemIndex = -1;

                    // Identifica se tocou em algum item
                    if (!visibleItems.empty() && touchStartY >= 70) {
                        if (!isGridView) {
                            float startY = 86.0f;
                            float cardHeight = 64.0f;
                            float cardSpacing = 14.0f;
                            int renderCount = std::min(static_cast<int>(visibleItems.size()) - scrollOffset, listVisibleCount);
                            for (int i = 0; i < renderCount; i++) {
                                float y = startY + i * (cardHeight + cardSpacing);
                                if (touchStartX >= 32 && touchStartX <= 928 && touchStartY >= y && touchStartY <= (y + cardHeight)) {
                                    initialTouchItemIndex = scrollOffset + i;
                                    break;
                                }
                            }
                        } else {
                            float startX = 32.0f;
                            float startY = 86.0f;
                            float cardW = 282.0f;
                            float cardH = 192.0f;
                            float gapX = 25.0f;
                            float gapY = 16.0f;
                            int renderCount = std::min(static_cast<int>(visibleItems.size()) - scrollOffset, gridVisibleCount);
                            for (int i = 0; i < renderCount; i++) {
                                int col = i % gridCols;
                                int row = i / gridCols;
                                float x = startX + col * (cardW + gapX);
                                float y = startY + row * (cardH + gapY);
                                if (touchStartX >= x && touchStartX <= (x + cardW) && touchStartY >= y && touchStartY <= (y + cardH)) {
                                    initialTouchItemIndex = scrollOffset + i;
                                    break;
                                }
                            }
                        }
                    }
                } else if (touchHeld && isTouching) {
                    int deltaX = touchX - touchStartX;
                    int deltaY = touchY - touchStartY;

                    // Arraste Vertical: Rolagem da Lista / Grade
                    if (!isDraggingX && (isDraggingY || std::abs(deltaY) > 12)) {
                        isDraggingY = true;
                        scrollBarTimer = 120;
                        float stepHeight = !isGridView ? 78.0f : 208.0f;
                        int steps = static_cast<int>(deltaY / stepHeight);
                        int newOffset = !isGridView ? (dragStartOffset - steps) : (dragStartOffset - steps * gridCols);
                        
                        int maxOffset = !isGridView ? std::max(0, static_cast<int>(visibleItems.size()) - listVisibleCount)
                                                    : std::max(0, static_cast<int>(visibleItems.size()) - gridVisibleCount);
                        if (newOffset < 0) newOffset = 0;
                        if (newOffset > maxOffset) newOffset = maxOffset;
                        scrollOffset = newOffset;
                    }
                    // Arraste Horizontal: Deleção do item
                    else if (!isDraggingY && initialTouchItemIndex >= 0 && std::abs(deltaX) > 28 && std::abs(deltaX) > std::abs(deltaY) * 1.3f) {
                        isDraggingX = true;
                    }
                } else if (touchUp && isTouching) {
                    if (isDraggingX && initialTouchItemIndex >= 0 && initialTouchItemIndex < static_cast<int>(visibleItems.size())) {
                        // Ativa popup de confirmação de exclusão
                        itemToDeleteIndex = initialTouchItemIndex;
                        showDeleteConfirm = true;
                        deleteConfirmYesSelected = false;
                    } else if (!isDraggingY) {
                        // Clique simples (sem arraste)
                        // 1. Campo de Busca (X: 250..490, Y: 14..54)
                        if (touchStartX >= 250 && touchStartX <= 490 && touchStartY >= 14 && touchStartY <= 54) {
                            currentFocus = FocusArea::SEARCH;
                            launch_ime("Pesquisar", searchQuery.c_str());
                        }
                        // 2. Botão de Ordenação (X: 500..650, Y: 14..54)
                        else if (touchStartX >= 500 && touchStartX <= 650 && touchStartY >= 14 && touchStartY <= 54) {
                            currentFocus = FocusArea::SORT;
                            int nextSort = (static_cast<int>(currentSort) + 1) % static_cast<int>(SortMode::COUNT);
                            currentSort = static_cast<SortMode>(nextSort);
                            appConfig.sortMode = currentSort;
                            ConfigManager::getInstance().save();
                            FileBrowser::sortItems(allItems, currentSort);
                            visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                            selectedIndex = 0;
                            scrollOffset = 0;
                            smoothScrollOffset = 0.0f;
                            scrollBarTimer = 120;
                        }
                        // 3. Botão de Alternância de Layout (X: 660..770, Y: 14..54)
                        else if (touchStartX >= 660 && touchStartX <= 770 && touchStartY >= 14 && touchStartY <= 54) {
                            currentFocus = FocusArea::LAYOUT;
                            isGridView = !isGridView;
                            appConfig.isGridView = isGridView;
                            ConfigManager::getInstance().save();
                            selectedIndex = 0;
                            scrollOffset = 0;
                            smoothScrollOffset = 0.0f;
                            scrollBarTimer = 120;
                        }
                        // 4. Botão de Configurações (X: 780..870, Y: 14..54)
                        else if (touchStartX >= 780 && touchStartX <= 870 && touchStartY >= 14 && touchStartY <= 54) {
                            currentFocus = FocusArea::SETTINGS;
                            currentState = AppState::SETTINGS;
                            settingsSelectedIndex = 0;
                        }
                        // 5. Botão de Refresh (X: 880..935, Y: 14..54)
                        else if (touchStartX >= 880 && touchStartX <= 935 && touchStartY >= 14 && touchStartY <= 54) {
                            currentFocus = FocusArea::REFRESH;
                            allItems = FileBrowser::scanLibrary();
                            FileBrowser::sortItems(allItems, currentSort);
                            visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                            selectedIndex = 0;
                            scrollOffset = 0;
                            smoothScrollOffset = 0.0f;
                            scrollBarTimer = 120;
                        }
                        // 6. Clique em Item: Abre o item imediatamente
                        else if (initialTouchItemIndex >= 0 && initialTouchItemIndex < static_cast<int>(visibleItems.size())) {
                            currentFocus = FocusArea::CONTENT;
                            selectedIndex = initialTouchItemIndex;
                            openItem(initialTouchItemIndex);
                        }
                    }

                    isTouching = false;
                    isDraggingX = false;
                    isDraggingY = false;
                    initialTouchItemIndex = -1;
                }

                // NAVEGAÇÃO ENTRE ÁREAS DE FOCO VIA D-PAD
                if (currentFocus == FocusArea::CONTENT) {
                    if (isGridView) {
                        // Modo Grade
                        if (pressed & SCE_CTRL_UP) {
                            if (selectedIndex >= gridCols) {
                                selectedIndex -= gridCols;
                                if (selectedIndex < scrollOffset) {
                                    scrollOffset -= gridCols;
                                    scrollBarTimer = 120;
                                }
                            } else {
                                currentFocus = FocusArea::SEARCH;
                            }
                        }
                        if (pressed & SCE_CTRL_DOWN) {
                            if (selectedIndex + gridCols < static_cast<int>(visibleItems.size())) {
                                selectedIndex += gridCols;
                                if (selectedIndex >= scrollOffset + gridVisibleCount) {
                                    scrollOffset += gridCols;
                                    scrollBarTimer = 120;
                                }
                            }
                        }
                        if (pressed & SCE_CTRL_LEFT) {
                            if (selectedIndex > 0 && (selectedIndex % gridCols != 0)) {
                                selectedIndex--;
                            }
                        }
                        if (pressed & SCE_CTRL_RIGHT) {
                            if (selectedIndex + 1 < static_cast<int>(visibleItems.size()) && ((selectedIndex + 1) % gridCols != 0)) {
                                selectedIndex++;
                            }
                        }
                    } else {
                        // Modo Lista
                        if (pressed & SCE_CTRL_UP) {
                            if (selectedIndex > 0) {
                                selectedIndex--;
                                if (selectedIndex < scrollOffset) {
                                    scrollOffset = selectedIndex;
                                    scrollBarTimer = 120;
                                }
                            } else {
                                currentFocus = FocusArea::SEARCH;
                            }
                        }
                        if (pressed & SCE_CTRL_DOWN) {
                            if (selectedIndex < static_cast<int>(visibleItems.size()) - 1) {
                                selectedIndex++;
                                if (selectedIndex >= scrollOffset + listVisibleCount) {
                                    scrollOffset = selectedIndex - listVisibleCount + 1;
                                    scrollBarTimer = 120;
                                }
                            }
                        }
                    }

                    // Alternar Favorito com Quadrado
                    if ((pressed & SCE_CTRL_SQUARE) && !visibleItems.empty()) {
                        auto& item = visibleItems[selectedIndex];
                        FileBrowser::toggleFavorite(item);
                        for (auto& orig : allItems) {
                            if (orig.filename == item.filename) {
                                orig.isFavorite = item.isFavorite;
                                break;
                            }
                        }
                        FileBrowser::saveMetadata(allItems);
                    }

                    // Abrir Arquivo com Cruz
                    if ((pressed & SCE_CTRL_CROSS) && !visibleItems.empty()) {
                        openItem(selectedIndex);
                    }
                } else if (currentFocus == FocusArea::SEARCH) {
                    if (pressed & SCE_CTRL_DOWN) currentFocus = FocusArea::CONTENT;
                    if (pressed & SCE_CTRL_RIGHT) currentFocus = FocusArea::SORT;
                    if (pressed & SCE_CTRL_CROSS) {
                        launch_ime("Pesquisar", searchQuery.c_str());
                    }
                    if (pressed & SCE_CTRL_SQUARE) {
                        searchQuery = "";
                        visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                        selectedIndex = 0;
                        scrollOffset = 0;
                        smoothScrollOffset = 0.0f;
                        scrollBarTimer = 120;
                    }
                } else if (currentFocus == FocusArea::SORT) {
                    if (pressed & SCE_CTRL_DOWN) currentFocus = FocusArea::CONTENT;
                    if (pressed & SCE_CTRL_LEFT) currentFocus = FocusArea::SEARCH;
                    if (pressed & SCE_CTRL_RIGHT) currentFocus = FocusArea::LAYOUT;
                    if (pressed & SCE_CTRL_CROSS) {
                        int nextSort = (static_cast<int>(currentSort) + 1) % static_cast<int>(SortMode::COUNT);
                        currentSort = static_cast<SortMode>(nextSort);
                        FileBrowser::sortItems(allItems, currentSort);
                        visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                        selectedIndex = 0;
                        scrollOffset = 0;
                        smoothScrollOffset = 0.0f;
                        scrollBarTimer = 120;
                    }
                } else if (currentFocus == FocusArea::LAYOUT) {
                    if (pressed & SCE_CTRL_DOWN) currentFocus = FocusArea::CONTENT;
                    if (pressed & SCE_CTRL_LEFT) currentFocus = FocusArea::SORT;
                    if (pressed & SCE_CTRL_RIGHT) currentFocus = FocusArea::SETTINGS;
                    if (pressed & SCE_CTRL_CROSS) {
                        isGridView = !isGridView;
                        appConfig.isGridView = isGridView;
                        ConfigManager::getInstance().save();
                        selectedIndex = 0;
                        scrollOffset = 0;
                        smoothScrollOffset = 0.0f;
                        scrollBarTimer = 120;
                    }
                } else if (currentFocus == FocusArea::SETTINGS) {
                    if (pressed & SCE_CTRL_DOWN) currentFocus = FocusArea::CONTENT;
                    if (pressed & SCE_CTRL_LEFT) currentFocus = FocusArea::LAYOUT;
                    if (pressed & SCE_CTRL_RIGHT) currentFocus = FocusArea::REFRESH;
                    if (pressed & SCE_CTRL_CROSS) {
                        currentState = AppState::SETTINGS;
                        settingsSelectedIndex = 0;
                    }
                } else if (currentFocus == FocusArea::REFRESH) {
                    if (pressed & SCE_CTRL_DOWN) currentFocus = FocusArea::CONTENT;
                    if (pressed & SCE_CTRL_LEFT) currentFocus = FocusArea::SETTINGS;
                    if (pressed & SCE_CTRL_CROSS) {
                        allItems = FileBrowser::scanLibrary();
                        FileBrowser::sortItems(allItems, currentSort);
                        visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                        selectedIndex = 0;
                        scrollOffset = 0;
                        smoothScrollOffset = 0.0f;
                        scrollBarTimer = 120;
                    }
                }

                // Atualizar lista geral com Triângulo
                if (pressed & SCE_CTRL_TRIANGLE) {
                    allItems = FileBrowser::scanLibrary();
                    FileBrowser::sortItems(allItems, currentSort);
                    visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                    selectedIndex = 0;
                    scrollOffset = 0;
                    smoothScrollOffset = 0.0f;
                    scrollBarTimer = 120;
                }
            }

            // ATUALIZAÇÃO DA ANIMAÇÃO DE SCROLL (GPU INTERPOLATION) E FADE DA SCROLLBAR
            smoothScrollOffset += (static_cast<float>(scrollOffset) - smoothScrollOffset) * 0.22f;
            if (std::abs(smoothScrollOffset - static_cast<float>(scrollOffset)) < 0.005f) {
                smoothScrollOffset = static_cast<float>(scrollOffset);
            }

            if (scrollBarTimer > 0) {
                scrollBarTimer--;
                scrollBarAlpha += (1.0f - scrollBarAlpha) * 0.15f;
            } else {
                scrollBarAlpha += (0.0f - scrollBarAlpha) * 0.06f;
            }
            if (scrollBarAlpha < 0.005f) scrollBarAlpha = 0.0f;
            if (scrollBarAlpha > 1.0f) scrollBarAlpha = 1.0f;

            // RENDERIZAÇÃO DO MENU
            startDrawing();
            vita2d_clear_screen();

            UIComponents::drawTopBar(
                searchQuery,
                ime_active != 0,
                currentSort,
                isGridView,
                currentFocus == FocusArea::SEARCH,
                currentFocus == FocusArea::SORT,
                currentFocus == FocusArea::LAYOUT,
                currentFocus == FocusArea::SETTINGS,
                currentFocus == FocusArea::REFRESH
            );

            if (visibleItems.empty()) {
                UIComponents::drawRoundedBox(180, 200, 600, 130, 12.0f, UITheme::Surface);
                if (pgf) {
                    vita2d_pgf_draw_text(pgf, 210, 250, UITheme::TextPrimary, 1.1f, "Nenhum arquivo correspondente!");
                    vita2d_pgf_draw_text(pgf, 210, 285, UITheme::TextSecondary, 0.85f, "Tente outra busca ou adicione arquivos em ux0:data/BilingualReaderVita/");
                }
            } else {
                if (!isGridView) {
                    // Renderização em Lista com Animação e Fade nos Extremos
                    float startY = 86.0f;
                    float cardHeight = 64.0f;
                    float cardSpacing = 14.0f;
                    float stepY = cardHeight + cardSpacing; // 78.0f

                    int firstIdx = std::max(0, static_cast<int>(smoothScrollOffset) - 1);
                    int lastIdx = std::min(static_cast<int>(visibleItems.size()), static_cast<int>(smoothScrollOffset) + listVisibleCount + 2);

                    for (int itemIdx = firstIdx; itemIdx < lastIdx; itemIdx++) {
                        const auto& item = visibleItems[itemIdx];
                        bool isSelected = (currentFocus == FocusArea::CONTENT && itemIdx == selectedIndex);
                        float y = startY + (static_cast<float>(itemIdx) - smoothScrollOffset) * stepY;

                        float cardTop = y;
                        float cardBottom = y + cardHeight;
                        if (cardBottom <= 68.0f || cardTop >= 504.0f) {
                            continue;
                        }

                        // Dissolver/Fade ao aproximar dos limites superior e inferior
                        float cardAlpha = 1.0f;
                        if (cardTop < 86.0f) {
                            cardAlpha = (cardBottom - 68.0f) / cardHeight;
                        } else if (cardBottom > 496.0f) {
                            cardAlpha = (504.0f - cardTop) / cardHeight;
                        }
                        cardAlpha = std::max(0.0f, std::min(1.0f, cardAlpha));

                        UIComponents::drawListCard(32.0f, y, 896.0f, cardHeight, isSelected, item, cardAlpha);
                    }

                    // Barra de rolagem animada com fade
                    UIComponents::drawLibraryScrollBar(smoothScrollOffset, static_cast<int>(visibleItems.size()), listVisibleCount, false, scrollBarAlpha);

                } else {
                    // Renderização em Grade com Animação e Fade nos Extremos
                    float startX = 32.0f;
                    float startY = 86.0f;
                    float cardW = 282.0f;
                    float cardH = 192.0f;
                    float gapX = 25.0f;
                    float gapY = 16.0f;
                    float stepY = cardH + gapY; // 208.0f

                    float smoothRowOffset = smoothScrollOffset / static_cast<float>(gridCols);
                    int totalRows = (static_cast<int>(visibleItems.size()) + gridCols - 1) / gridCols;
                    int firstRow = std::max(0, static_cast<int>(smoothRowOffset) - 1);
                    int lastRow = std::min(totalRows, static_cast<int>(smoothRowOffset) + 3);

                    for (int row = firstRow; row < lastRow; row++) {
                        for (int col = 0; col < gridCols; col++) {
                            int itemIdx = row * gridCols + col;
                            if (itemIdx >= static_cast<int>(visibleItems.size())) break;

                            const auto& item = visibleItems[itemIdx];
                            bool isSelected = (currentFocus == FocusArea::CONTENT && itemIdx == selectedIndex);

                            float x = startX + col * (cardW + gapX);
                            float y = startY + (static_cast<float>(row) - smoothRowOffset) * stepY;

                            float cardTop = y;
                            float cardBottom = y + cardH;
                            if (cardBottom <= 68.0f || cardTop >= 504.0f) {
                                continue;
                            }

                            // Dissolver/Fade ao aproximar dos limites superior e inferior
                            float cardAlpha = 1.0f;
                            if (cardTop < 86.0f) {
                                cardAlpha = (cardBottom - 68.0f) / cardH;
                            } else if (cardBottom > 496.0f) {
                                cardAlpha = (504.0f - cardTop) / cardH;
                            }
                            cardAlpha = std::max(0.0f, std::min(1.0f, cardAlpha));

                            UIComponents::drawGridCard(x, y, cardW, cardH, isSelected, item, cardAlpha);
                        }
                    }

                    // Barra de rolagem animada com fade
                    UIComponents::drawLibraryScrollBar(smoothScrollOffset, static_cast<int>(visibleItems.size()), gridVisibleCount, true, scrollBarAlpha);
                }
            }

            UIComponents::drawFooter("D-Pad: Navegar | X: Abrir/Acao | []: Favorito | /\\: Recarregar");

            if (showDeleteConfirm && itemToDeleteIndex >= 0 && itemToDeleteIndex < static_cast<int>(visibleItems.size())) {
                std::string msg = "Deseja excluir: " + visibleItems[itemToDeleteIndex].filename + "?";
                UIComponents::drawConfirmDialog("Excluir Arquivo", msg, deleteConfirmYesSelected);
            }

            endDrawing();


        } else if (currentState == AppState::SETTINGS) {
            const int totalSettingsItems = 5;

            // Navegação Vertical (D-pad Cima / Baixo)
            if (pressed & SCE_CTRL_UP) {
                if (settingsSelectedIndex > 0) settingsSelectedIndex--;
            }
            if (pressed & SCE_CTRL_DOWN) {
                if (settingsSelectedIndex < totalSettingsItems - 1) settingsSelectedIndex++;
            }

            // Alternância de Valores (D-pad Esquerda / Direita ou Cruz)
            bool changePrev = (pressed & SCE_CTRL_LEFT);
            bool changeNext = (pressed & (SCE_CTRL_RIGHT | SCE_CTRL_CROSS));

            if (changePrev || changeNext) {
                switch (settingsSelectedIndex) {
                    case 0: // Modo de Exibição (Lista / Grade)
                        appConfig.isGridView = !appConfig.isGridView;
                        isGridView = appConfig.isGridView;
                        selectedIndex = 0;
                        scrollOffset = 0;
                        smoothScrollOffset = 0.0f;
                        break;
                    case 1: { // Ordenação Padrão
                        int count = static_cast<int>(SortMode::COUNT);
                        int cur = static_cast<int>(appConfig.sortMode);
                        if (changeNext) cur = (cur + 1) % count;
                        else cur = (cur - 1 + count) % count;
                        appConfig.sortMode = static_cast<SortMode>(cur);
                        currentSort = appConfig.sortMode;
                        FileBrowser::sortItems(allItems, currentSort);
                        visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                        selectedIndex = 0;
                        scrollOffset = 0;
                        smoothScrollOffset = 0.0f;
                        break;
                    }
                    case 2: // Orientação do Leitor
                        appConfig.readerRotated = !appConfig.readerRotated;
                        readerRotated = appConfig.readerRotated;
                        UIComponents::setRotated(readerRotated);
                        readerTxt.setRotated(readerRotated, pgf);
                        readerCbz.setRotated(readerRotated);
                        readerEpub.setRotated(readerRotated);
                        break;
                    case 3: // Exibir Número de Páginas
                        appConfig.showPageNumbers = !appConfig.showPageNumbers;
                        break;
                    case 4: // Tamanho de Fonte Padrão (EPUB)
                        if (changeNext) {
                            if (appConfig.epubFontSize < 44) appConfig.epubFontSize += 2;
                        } else {
                            if (appConfig.epubFontSize > 14) appConfig.epubFontSize -= 2;
                        }
                        break;
                }
                ConfigManager::getInstance().save();
            }

            // Touch Gestures na tela de Configurações
            if (touchDown) {
                float sec1Y = 88.0f + 26.0f;
                float rowH = 46.0f;
                float sec2Y = sec1Y + rowH * 2.0f + 50.0f;

                // Item 0
                if (touchStartY >= sec1Y && touchStartY < sec1Y + rowH) {
                    settingsSelectedIndex = 0;
                    appConfig.isGridView = !appConfig.isGridView;
                    isGridView = appConfig.isGridView;
                    selectedIndex = 0; scrollOffset = 0; smoothScrollOffset = 0.0f;
                    ConfigManager::getInstance().save();
                }
                // Item 1
                else if (touchStartY >= sec1Y + rowH && touchStartY < sec1Y + rowH * 2.0f) {
                    settingsSelectedIndex = 1;
                    int count = static_cast<int>(SortMode::COUNT);
                    int cur = (static_cast<int>(appConfig.sortMode) + 1) % count;
                    appConfig.sortMode = static_cast<SortMode>(cur);
                    currentSort = appConfig.sortMode;
                    FileBrowser::sortItems(allItems, currentSort);
                    visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                    selectedIndex = 0; scrollOffset = 0; smoothScrollOffset = 0.0f;
                    ConfigManager::getInstance().save();
                }
                // Item 2
                else if (touchStartY >= sec2Y && touchStartY < sec2Y + rowH) {
                    settingsSelectedIndex = 2;
                    appConfig.readerRotated = !appConfig.readerRotated;
                    readerRotated = appConfig.readerRotated;
                    UIComponents::setRotated(readerRotated);
                    readerTxt.setRotated(readerRotated, pgf);
                    readerCbz.setRotated(readerRotated);
                    readerEpub.setRotated(readerRotated);
                    ConfigManager::getInstance().save();
                }
                // Item 3
                else if (touchStartY >= sec2Y + rowH && touchStartY < sec2Y + rowH * 2.0f) {
                    settingsSelectedIndex = 3;
                    appConfig.showPageNumbers = !appConfig.showPageNumbers;
                    ConfigManager::getInstance().save();
                }
                // Item 4
                else if (touchStartY >= sec2Y + rowH * 2.0f && touchStartY < sec2Y + rowH * 3.0f) {
                    settingsSelectedIndex = 4;
                    if (touchStartX > 600) {
                        if (appConfig.epubFontSize < 44) appConfig.epubFontSize += 2;
                    } else {
                        if (appConfig.epubFontSize > 14) appConfig.epubFontSize -= 2;
                    }
                    ConfigManager::getInstance().save();
                }
            }

            // Voltar para o MENU
            if (pressed & SCE_CTRL_CIRCLE) {
                ConfigManager::getInstance().save();
                currentState = AppState::MENU;
                currentFocus = FocusArea::CONTENT;
            }

            startDrawing();
            vita2d_clear_screen();
            UIComponents::drawSettingsScreen(settingsSelectedIndex, appConfig);
            endDrawing();

        } else if (currentState == AppState::READ_TXT) {
            if (pressed & (SCE_CTRL_RTRIGGER | SCE_CTRL_R1)) {
                if (visibleItems.size() > 1) openNextBook();
                else readerTxt.nextPage();
            } else if (pressed & (SCE_CTRL_LTRIGGER | SCE_CTRL_L1)) {
                if (visibleItems.size() > 1) openPrevBook();
                else readerTxt.prevPage();
            } else {
                if ((pressed & (SCE_CTRL_RIGHT | SCE_CTRL_DOWN)) || readerTapRight) {
                    readerTxt.nextPage();
                }
                if ((pressed & (SCE_CTRL_LEFT | SCE_CTRL_UP)) || readerTapLeft) {
                    readerTxt.prevPage();
                }
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                readerTxt.close();
                currentState = AppState::MENU;
            }

            startDrawing();
            vita2d_clear_screen();
            readerTxt.render(pgf, readerFullscreen);
            endDrawing();

        } else if (currentState == AppState::READ_CBZ) {
            if (pressed & (SCE_CTRL_RTRIGGER | SCE_CTRL_R1)) {
                if (visibleItems.size() > 1) openNextBook();
                else readerCbz.nextPage();
            } else if (pressed & (SCE_CTRL_LTRIGGER | SCE_CTRL_L1)) {
                if (visibleItems.size() > 1) openPrevBook();
                else readerCbz.prevPage();
            } else {
                if ((pressed & (SCE_CTRL_RIGHT | SCE_CTRL_DOWN)) || readerTapRight) {
                    readerCbz.nextPage();
                }
                if ((pressed & (SCE_CTRL_LEFT | SCE_CTRL_UP)) || readerTapLeft) {
                    readerCbz.prevPage();
                }
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                readerCbz.close();
                currentState = AppState::MENU;
            }

            startDrawing();
            vita2d_clear_screen();
            readerCbz.render(pgf, readerFullscreen);
            endDrawing();

        } else if (currentState == AppState::READ_EPUB) {
            if (pressed & (SCE_CTRL_RTRIGGER | SCE_CTRL_R1)) {
                if (visibleItems.size() > 1) openNextBook();
                else readerEpub.nextPage();
            } else if (pressed & (SCE_CTRL_LTRIGGER | SCE_CTRL_L1)) {
                if (visibleItems.size() > 1) openPrevBook();
                else readerEpub.prevPage();
            } else {
                if ((pressed & (SCE_CTRL_RIGHT | SCE_CTRL_DOWN)) || readerTapRight) {
                    readerEpub.nextPage();
                }
                if ((pressed & (SCE_CTRL_LEFT | SCE_CTRL_UP)) || readerTapLeft) {
                    readerEpub.prevPage();
                }
            }
            if (pressed & SCE_CTRL_TRIANGLE) {
                readerEpub.increaseFontSize();
            }
            if (pressed & SCE_CTRL_SQUARE) {
                readerEpub.decreaseFontSize();
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                readerEpub.close();
                currentState = AppState::MENU;
            }

            startDrawing();
            vita2d_clear_screen();
            readerEpub.render(pgf, readerFullscreen);
            endDrawing();
        }

        oldPad.buttons = logicalButtons; // save logical for edge detection
        oldPad.rx = pad.rx; oldPad.ry = pad.ry;
    }

    UIComponents::shutdown();
    vita2d_free_pgf(pgf);
    vita2d_fini();
    sceKernelExitProcess(0);
    return 0;
}
