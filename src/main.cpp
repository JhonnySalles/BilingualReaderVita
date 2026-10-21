#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ime_dialog.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "ui_components.h"
#include "config_manager.h"
#include "file_browser.h"
#include "reader_txt.h"
#include "reader_cbz.h"
#include "reader_epub.h"
#include "managers/pages_link_manager.h"
#include "managers/cover_manager.h"

// Matriz ortográfica interna do vita2d (sem setter público).
extern "C" float _vita2d_ortho_matrix[4 * 4];

static void setOrthoProjection(float width, float height) {
    // Mesma fórmula de matrix_init_orthographic do vita2d (Y crescente para baixo).
    float left = 0.0f;
    float right = width;
    float bottom = height;
    float top = 0.0f;
    float nearPlane = 0.0f;
    float farPlane = 1.0f;

    _vita2d_ortho_matrix[0x0] = 2.0f / (right - left);
    _vita2d_ortho_matrix[0x4] = 0.0f;
    _vita2d_ortho_matrix[0x8] = 0.0f;
    _vita2d_ortho_matrix[0xC] = -(right + left) / (right - left);

    _vita2d_ortho_matrix[0x1] = 0.0f;
    _vita2d_ortho_matrix[0x5] = 2.0f / (top - bottom);
    _vita2d_ortho_matrix[0x9] = 0.0f;
    _vita2d_ortho_matrix[0xD] = -(top + bottom) / (top - bottom);

    _vita2d_ortho_matrix[0x2] = 0.0f;
    _vita2d_ortho_matrix[0x6] = 0.0f;
    _vita2d_ortho_matrix[0xA] = -2.0f / (farPlane - nearPlane);
    _vita2d_ortho_matrix[0xE] = (farPlane + nearPlane) / (farPlane - nearPlane);

    _vita2d_ortho_matrix[0x3] = 0.0f;
    _vita2d_ortho_matrix[0x7] = 0.0f;
    _vita2d_ortho_matrix[0xB] = 0.0f;
    _vita2d_ortho_matrix[0xF] = 1.0f;
}

static void setDrawSurfaceSize(float width, float height) {
    setOrthoProjection(width, height);
    // API correta do GXM para casar clip/viewport com o alvo atual.
    // Evita montar sceGxmSetViewport manualmente (ordem xOffset,xScale,yOffset,yScale).
    sceGxmSetDefaultRegionClipAndViewport(
        vita2d_get_context(),
        static_cast<unsigned int>(width) - 1,
        static_cast<unsigned int>(height) - 1
    );
}

static void mapPhysicalTouchToLogical(float physX, float physY, bool rotated, float& outX, float& outY) {
    if (rotated) {
        // Rotação 90° horário: inverte o mapeamento para o espaço lógico 544x960.
        outX = physY;
        outY = 960.0f - physX;
    } else {
        outX = physX;
        outY = physY;
    }
}

enum class AppState {
    MENU,
    SETTINGS,
    READ_TXT,
    READ_CBZ,
    READ_EPUB,
    PAGES_LINK,
    PAGES_LINK_FILE_PICKER
};

enum class PagesLinkFocusArea {
    TOOLBAR,
    LINKED_CARDS,
    LINKED_DUAL_CARDS,
    UNLINKED_CARDS
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
    if (!src || !dst) return;
    int di = 0;
    for (int si = 0; src[si]; ) {
        uint32_t codepoint = 0;
        uint8_t c = src[si++];
        if (c < 0x80) {
            codepoint = c;
        } else if ((c & 0xE0) == 0xC0) {
            if (!src[si]) break;
            codepoint = ((c & 0x1F) << 6) | (src[si++] & 0x3F);
        } else if ((c & 0xF0) == 0xE0) {
            if (!src[si] || !src[si + 1]) break;
            codepoint = ((c & 0x0F) << 12) | ((src[si] & 0x3F) << 6) | (src[si + 1] & 0x3F);
            si += 2;
        } else if ((c & 0xF8) == 0xF0) {
            if (!src[si] || !src[si + 1] || !src[si + 2]) break;
            codepoint = ((c & 0x07) << 18) | ((src[si] & 0x3F) << 12) | ((src[si + 1] & 0x3F) << 6) | (src[si + 2] & 0x3F);
            si += 3;
        }

        if (codepoint <= 0xFFFF) {
            dst[di++] = (uint16_t)codepoint;
        } else {
            codepoint -= 0x10000;
            dst[di++] = (uint16_t)(0xD800 + (codepoint >> 10));
            dst[di++] = (uint16_t)(0xDC00 + (codepoint & 0x3FF));
        }
    }
    dst[di] = 0;
}

static void utf16_to_utf8(const uint16_t *src, uint8_t *dst) {
    if (!src || !dst) return;
    int di = 0;
    for (int si = 0; src[si]; ) {
        uint32_t codepoint = src[si++];
        if (codepoint >= 0xD800 && codepoint <= 0xDBFF && src[si] >= 0xDC00 && src[si] <= 0xDFFF) {
            codepoint = 0x10000 + (((codepoint - 0xD800) << 10) | (src[si++] - 0xDC00));
        }

        if (codepoint < 0x80) {
            dst[di++] = (uint8_t)codepoint;
        } else if (codepoint < 0x800) {
            dst[di++] = (uint8_t)(0xC0 | (codepoint >> 6));
            dst[di++] = (uint8_t)(0x80 | (codepoint & 0x3F));
        } else if (codepoint < 0x10000) {
            dst[di++] = (uint8_t)(0xE0 | (codepoint >> 12));
            dst[di++] = (uint8_t)(0x80 | ((codepoint >> 6) & 0x3F));
            dst[di++] = (uint8_t)(0x80 | (codepoint & 0x3F));
        } else {
            dst[di++] = (uint8_t)(0xF0 | (codepoint >> 18));
            dst[di++] = (uint8_t)(0x80 | ((codepoint >> 12) & 0x3F));
            dst[di++] = (uint8_t)(0x80 | ((codepoint >> 6) & 0x3F));
            dst[di++] = (uint8_t)(0x80 | (codepoint & 0x3F));
        }
    }
    dst[di] = 0;
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

    // Inicializa subsistema de capas e cache
    CoverManager::getInstance().init();

    // Aplica o tema salvo (ou padrão NVIDIA Green)
    ThemeManager::apply(appConfig.theme);

    // Habilita amostragem do Touch Frontal
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);

    vita2d_pgf* pgf = vita2d_load_default_pgf();
    UIComponents::init(pgf);

    AppState currentState = AppState::MENU;
    FocusArea currentFocus = FocusArea::CONTENT;
    int settingsSelectedIndex = 0;
    int settingsThemeCursor = static_cast<int>(appConfig.theme);
    float settingsScrollY = 0.0f;
    float settingsSmoothScrollY = 0.0f;
    float settingsThemeScrollX = 0.0f;
    float settingsSmoothThemeScrollX = 0.0f;

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
    UIComponents::setRotated(readerRotated);

    // Estado do Diálogo de Exclusão (Popup)
    bool showDeleteConfirm = false;
    int itemToDeleteIndex = -1;
    bool deleteConfirmYesSelected = false;

    // Estado do Diálogo de Limpeza de Capas
    bool showClearCoversConfirm = false;
    bool clearCoversConfirmYesSelected = false;

    ReaderTXT readerTxt;
    ReaderCBZ readerCbz;
    ReaderEPUB readerEpub;


    // Texture para rotacao da UI
    vita2d_texture* uiRenderTarget = vita2d_create_empty_texture_rendertarget(544, 960, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR);
    vita2d_texture* uiBgTex = vita2d_create_empty_texture(1, 1);
    if (uiBgTex) {
        unsigned int* data = (unsigned int*)vita2d_texture_get_datap(uiBgTex);
        *data = RGBA8(255, 255, 255, 255);
    }

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

            // Gera miniatura da capa sob demanda caso ainda não tenha sido gerada
            CoverManager::getInstance().generateCover(selected.fullPath);

            // Libera todas as capas em VRAM para dar prioridade de memória total ao leitor
            CoverManager::getInstance().clearAllTextures();

            if (selected.type == FileType::TXT) {
                if (readerTxt.loadFile(selected.fullPath, pgf)) {
                    readerTxt.setRotated(readerRotated, pgf);
                    readerTxt.setScrollMode(appConfig.readerScrollMode);
                    readerTxt.setPageTransition(appConfig.pageTransition);
                    currentState = AppState::READ_TXT;
                }
            } else if (FileBrowser::isMangaType(selected.type)) {
                if (readerCbz.loadFile(selected.fullPath)) {
                    readerCbz.setRotated(readerRotated);
                    readerCbz.setScrollMode(appConfig.readerScrollMode);
                    readerCbz.setPageTransition(appConfig.pageTransition);
                    currentState = AppState::READ_CBZ;
                }
            } else if (selected.type == FileType::EPUB) {
                if (readerEpub.loadFile(selected.fullPath, pgf)) {
                    readerEpub.setRotated(readerRotated);
                    readerEpub.setScrollMode(appConfig.readerScrollMode);
                    readerEpub.setPageTransition(appConfig.pageTransition);
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

    AppState settingsReturnState = AppState::MENU;

    // Variáveis de Estado do Vinculador de Páginas (PagesLink)
    AppState plPreviousState = AppState::MENU;
    PagesLinkFocusArea plFocus = PagesLinkFocusArea::LINKED_CARDS;
    int plSelectedToolbarBtn = 0; // 0: Select, 1: Auto, 2: Simple, 3: Double, 4: Sort, 5: Undo, 6: Save
    int plSelectedLinkedCard = 0;
    int plSelectedUnlinkedCard = 0;

    float plLinkedScrollX = 0.0f;
    float plLinkedSmoothScrollX = 0.0f;
    float plUnlinkedScrollX = 0.0f;
    float plUnlinkedSmoothScrollX = 0.0f;

    bool plIsDragging = false;
    bool plDragFromNotLinked = false;
    int plDragOriginIndex = -1;
    float plDragCurrentX = 0.0f;
    float plDragCurrentY = 0.0f;
    int plHoverDropTargetIndex = -1;
    bool plHoverDropIsUnlinked = false;
    int plToastTimer = 0;
    std::string plToastMessage = "";

    // Controles de Long-press / Touch Scroll / Arraste / Botão X
    bool plIsHoldingCross = false;
    bool plIsLongPressActive = false;
    int plLongPressTimer = 0;
    bool plIsTouchScrolling = false;
    bool plTouchIsValidScrollArea = false;
    float plTouchStartX = 0.0f;
    float plTouchStartY = 0.0f;
    float plTouchInitialScrollX = 0.0f;
    int plCandidateTouchIndex = -1;
    bool plCandidateIsUnlinked = false;
    bool plDraggingLinkedScrollbar = false;
    bool plDraggingUnlinkedScrollbar = false;

    // Estado do Seletor de Arquivo de Vínculo (Picker)
    int plPickerSelectedIndex = 0;
    int plPickerScrollOffset = 0;
    float plPickerSmoothScrollOffset = 0.0f;
    std::vector<LibraryItem> plPickerItems;

    auto openPagesLink = [&](int idx) {
        if (idx >= 0 && idx < static_cast<int>(visibleItems.size())) {
            const auto& selected = visibleItems[idx];
            if (FileBrowser::isMangaType(selected.type) || selected.type == FileType::EPUB) {
                if (PagesLinkManager::getInstance().loadPrimaryFile(selected.fullPath)) {
                    plPreviousState = currentState;
                    plFocus = PagesLinkFocusArea::LINKED_CARDS;
                    plSelectedToolbarBtn = 0;
                    plSelectedLinkedCard = 0;
                    plSelectedUnlinkedCard = 0;
                    plLinkedScrollX = 0.0f;
                    plLinkedSmoothScrollX = 0.0f;
                    plUnlinkedScrollX = 0.0f;
                    plUnlinkedSmoothScrollX = 0.0f;
                    plIsDragging = false;
                    plDragOriginIndex = -1;
                    plIsHoldingCross = false;
                    plIsLongPressActive = false;
                    plLongPressTimer = 0;
                    plIsTouchScrolling = false;
                    plDraggingLinkedScrollbar = false;
                    plDraggingUnlinkedScrollbar = false;
                    CoverManager::getInstance().clearAllTextures();
                    currentState = AppState::PAGES_LINK;
                }
            }
        }
    };

    auto startDrawing = [&]() {
        if (readerRotated && uiRenderTarget) {
            vita2d_start_drawing_advanced(uiRenderTarget, 0);
            setDrawSurfaceSize(544.0f, 960.0f);
            vita2d_clear_screen();
            if (uiBgTex) {
                vita2d_draw_texture_tint_scale(uiBgTex, 0, 0, 544.0f, 960.0f, UITheme::Background);
            }
        } else {
            vita2d_start_drawing();
            setDrawSurfaceSize(960.0f, 544.0f);
        }
    };
    
    auto endDrawing = [&]() {
        vita2d_end_drawing();
        if (readerRotated && uiRenderTarget) {
            vita2d_start_drawing();
            setDrawSurfaceSize(960.0f, 544.0f);
            vita2d_clear_screen();
            // Desenha a textura girada em 90 graus horario. O centro eh a metade da tela do PS Vita
            vita2d_draw_texture_rotate(uiRenderTarget, 480.0f, 272.0f, 1.57079632679f);
            vita2d_end_drawing();
        }
        vita2d_swap_buffers();
        sceDisplayWaitVblankStart();
    };

    auto toggleRotation = [&]() {
        readerRotated = !readerRotated;
        UIComponents::setRotated(readerRotated);
        readerTxt.setRotated(readerRotated, pgf);
        readerCbz.setRotated(readerRotated);
        readerEpub.setRotated(readerRotated);
    };

    uint64_t lastFrameTime = sceKernelGetProcessTimeWide();

    while (true) {
        uint64_t nowFrameTime = sceKernelGetProcessTimeWide();
        float frameDt = static_cast<float>(nowFrameTime - lastFrameTime) / 1000000.0f;
        lastFrameTime = nowFrameTime;
        if (frameDt > 0.05f) frameDt = 0.05f;
        if (frameDt < 0.0f) frameDt = 0.0f;

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
        
        static int lastTouchX = 0;
        static int lastTouchY = 0;
        int touchX = lastTouchX;
        int touchY = lastTouchY;

        if (touch.reportNum > 0) {
            // Converte coordenadas do painel (1920x1088) para a tela (960x544)
            float physX = touch.report[0].x / 2.0f;
            float physY = touch.report[0].y / 2.0f;
            float logX = 0.0f;
            float logY = 0.0f;
            mapPhysicalTouchToLogical(physX, physY, readerRotated, logX, logY);
            touchX = static_cast<int>(logX);
            touchY = static_cast<int>(logY);
            lastTouchX = touchX;
            lastTouchY = touchY;
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
            const float zoomCx = UIComponents::getScreenW() * 0.5f;
            const float zoomCy = UIComponents::getScreenH() * 0.5f;
            if (ry < 100) {
                float factor = 1.0f + (100.0f - ry) * 0.0005f;
                readerCbz.addZoom(factor, zoomCx, zoomCy);
            } else if (ry > 154) {
                float factor = 1.0f - (ry - 154.0f) * 0.0005f;
                readerCbz.addZoom(factor, zoomCx, zoomCy);
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
        bool readerTapPrev = false;
        bool readerTapNext = false;
        const bool inReader =
            currentState == AppState::READ_TXT ||
            currentState == AppState::READ_CBZ ||
            currentState == AppState::READ_EPUB;
        const ReaderScrollMode scrollMode = appConfig.readerScrollMode;
        const bool infiniteScroll = (scrollMode == ReaderScrollMode::InfiniteScroll);
        const bool pagedVertical = (scrollMode == ReaderScrollMode::PagedVertical);

        if (inReader) {
            UIReaderChromeLayout readerChrome = UIComponents::getReaderChromeLayout();

            if (touch.reportNum >= 2 && !infiniteScroll) {
                // Multitoque: Zoom por pinça (apenas nos modos paginados)
                float p0x = 0.0f, p0y = 0.0f, p1x = 0.0f, p1y = 0.0f;
                mapPhysicalTouchToLogical(touch.report[0].x / 2.0f, touch.report[0].y / 2.0f, readerRotated, p0x, p0y);
                mapPhysicalTouchToLogical(touch.report[1].x / 2.0f, touch.report[1].y / 2.0f, readerRotated, p1x, p1y);
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
                    } else if (currentState == AppState::READ_EPUB) {
                        readerEpub.addZoom(factor, midX, midY);
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
                    if (readerIsDragging) {
                        if (infiniteScroll) {
                            if (currentState == AppState::READ_TXT) {
                                readerTxt.cancelScrollAnimation();
                                readerTxt.addScroll(static_cast<float>(-dy));
                            } else if (currentState == AppState::READ_CBZ) {
                                readerCbz.cancelScrollAnimation();
                                readerCbz.addScroll(static_cast<float>(-dy));
                            } else if (currentState == AppState::READ_EPUB) {
                                readerEpub.cancelScrollAnimation();
                                readerEpub.addScroll(static_cast<float>(-dy));
                            }
                        } else if (currentState == AppState::READ_CBZ) {
                            readerCbz.addPan(static_cast<float>(dx), static_cast<float>(dy));
                        } else if (currentState == AppState::READ_EPUB) {
                            readerEpub.addPan(static_cast<float>(dx), static_cast<float>(dy));
                        }
                    }
                    readerLastTouchX = touchX;
                    readerLastTouchY = touchY;
                }
            } else if (touchUp && readerIsTouching) {
                // Ao soltar, se foi apenas clique rápido (sem arraste significativo nem pinça)
                if (!readerIsDragging && !readerIsPinching) {
                    bool handledBarTouch = false;
                    if (!readerFullscreen) {
                        if (readerChrome.settingsBtn.contains(static_cast<float>(readerTouchStartX),
                                                              static_cast<float>(readerTouchStartY))) {
                            settingsReturnState = currentState;
                            currentState = AppState::SETTINGS;
                            settingsSelectedIndex = 0;
                            handledBarTouch = true;
                        } else if (readerChrome.linkBtn.contains(static_cast<float>(readerTouchStartX),
                                                          static_cast<float>(readerTouchStartY))) {
                            openPagesLink(selectedIndex);
                            handledBarTouch = true;
                        } else if (readerChrome.toggleBtn.contains(static_cast<float>(readerTouchStartX),
                                                                   static_cast<float>(readerTouchStartY))) {
                            if (currentState == AppState::READ_CBZ) {
                                readerCbz.toggleLinkedPage();
                            }
                            handledBarTouch = true;
                        } else if (readerChrome.rotateBtn.contains(static_cast<float>(readerTouchStartX),
                                                                   static_cast<float>(readerTouchStartY))) {
                            toggleRotation();
                            handledBarTouch = true;
                        } else if (readerChrome.prevBook.contains(static_cast<float>(readerTouchStartX),
                                                                  static_cast<float>(readerTouchStartY))) {
                            openPrevBook();
                            handledBarTouch = true;
                        } else if (readerChrome.nextBook.contains(static_cast<float>(readerTouchStartX),
                                                                  static_cast<float>(readerTouchStartY))) {
                            openNextBook();
                            handledBarTouch = true;
                        }
                    }

                    if (!handledBarTouch) {
                        bool canTurnOrToggle = readerFullscreen ||
                            (readerTouchStartY >= static_cast<int>(readerChrome.contentTop) &&
                             readerTouchStartY < static_cast<int>(readerChrome.contentBottom));
                        if (canTurnOrToggle) {
                            if (infiniteScroll) {
                                float contentH = readerChrome.contentBottom - readerChrome.contentTop;
                                float midTop = readerChrome.contentTop + contentH * 0.35f;
                                float midBottom = readerChrome.contentTop + contentH * 0.65f;
                                if (readerTouchStartY >= static_cast<int>(midTop) &&
                                    readerTouchStartY <= static_cast<int>(midBottom)) {
                                    readerFullscreen = !readerFullscreen;
                                }
                            } else if (pagedVertical) {
                                float contentH = readerChrome.contentBottom - readerChrome.contentTop;
                                float topEdge = readerChrome.contentTop + contentH * 0.25f;
                                float bottomEdge = readerChrome.contentTop + contentH * 0.75f;
                                if (readerTouchStartY < static_cast<int>(topEdge)) {
                                    readerTapPrev = true;
                                } else if (readerTouchStartY > static_cast<int>(bottomEdge)) {
                                    readerTapNext = true;
                                } else {
                                    readerFullscreen = !readerFullscreen;
                                }
                            } else {
                                float leftEdge = readerChrome.screenW * 0.25f;
                                float rightEdge = readerChrome.screenW * 0.75f;
                                if (readerTouchStartX < static_cast<int>(leftEdge)) {
                                    readerTapLeft = true;
                                } else if (readerTouchStartX > static_cast<int>(rightEdge)) {
                                    readerTapRight = true;
                                } else {
                                    readerFullscreen = !readerFullscreen;
                                }
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

        if (pagedVertical) {
            if (readerTapPrev) readerTapLeft = true;
            if (readerTapNext) readerTapRight = true;
        }

        if (currentState == AppState::MENU) {
            const int gridCols = readerRotated ? 2 : 3;
            const int gridVisibleRows = readerRotated ? 4 : 2;
            const int gridVisibleCount = gridVisibleRows * gridCols;
            const int listVisibleCount = readerRotated ? 10 : 5;

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
                        int maxOffset = 0;
                        if (!isGridView) {
                            maxOffset = std::max(0, static_cast<int>(visibleItems.size()) - listVisibleCount);
                        } else {
                            int totalRows = (static_cast<int>(visibleItems.size()) + gridCols - 1) / gridCols;
                            int maxRowOffset = std::max(0, totalRows - gridVisibleRows);
                            maxOffset = maxRowOffset * gridCols;
                        }
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
                    UIConfirmDialogLayout dlg = UIComponents::getConfirmDialogLayout();
                    float tx = static_cast<float>(touchX);
                    float ty = static_cast<float>(touchY);

                    if (dlg.yesBtn.contains(tx, ty)) {
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
                            int maxOffset = 0;
                            if (!isGridView) {
                                maxOffset = std::max(0, static_cast<int>(visibleItems.size()) - listVisibleCount);
                            } else {
                                int totalRows = (static_cast<int>(visibleItems.size()) + gridCols - 1) / gridCols;
                                int maxRowOffset = std::max(0, totalRows - gridVisibleRows);
                                maxOffset = maxRowOffset * gridCols;
                            }
                            if (scrollOffset > maxOffset) scrollOffset = maxOffset;
                        }
                        showDeleteConfirm = false;
                        itemToDeleteIndex = -1;
                    } else if (dlg.noBtn.contains(tx, ty)) {
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
                                float cardW = readerRotated ? 480.0f : 896.0f;
                                if (touchStartX >= 32 && touchStartX <= (32 + cardW) && touchStartY >= y && touchStartY <= (y + cardHeight)) {
                                    initialTouchItemIndex = scrollOffset + i;
                                    break;
                                }
                            }
                        } else {
                            float startX = 32.0f;
                            float startY = 86.0f;
                            float cardW = readerRotated ? 230.0f : 282.0f;
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
                        
                        int maxOffset = 0;
                        if (!isGridView) {
                            maxOffset = std::max(0, static_cast<int>(visibleItems.size()) - listVisibleCount);
                        } else {
                            int totalRows = (static_cast<int>(visibleItems.size()) + gridCols - 1) / gridCols;
                            int maxRowOffset = std::max(0, totalRows - gridVisibleRows);
                            maxOffset = maxRowOffset * gridCols;
                        }
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
                        UITopBarLayout topBar = UIComponents::getTopBarLayout();
                        float sx = static_cast<float>(touchStartX);
                        float sy = static_cast<float>(touchStartY);

                        // Clique simples (sem arraste)
                        if (topBar.search.contains(sx, sy)) {
                            currentFocus = FocusArea::SEARCH;
                            launch_ime("Pesquisar", searchQuery.c_str());
                        } else if (topBar.sort.contains(sx, sy)) {
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
                        } else if (topBar.layout.contains(sx, sy)) {
                            currentFocus = FocusArea::LAYOUT;
                            isGridView = !isGridView;
                            appConfig.isGridView = isGridView;
                            ConfigManager::getInstance().save();
                            selectedIndex = 0;
                            scrollOffset = 0;
                            smoothScrollOffset = 0.0f;
                            scrollBarTimer = 120;
                        } else if (topBar.settings.contains(sx, sy)) {
                            currentFocus = FocusArea::SETTINGS;
                            settingsReturnState = AppState::MENU;
                            CoverManager::getInstance().clearAllTextures();
                            currentState = AppState::SETTINGS;
                            settingsSelectedIndex = 0;
                        } else if (topBar.refresh.contains(sx, sy)) {
                            currentFocus = FocusArea::REFRESH;
                            CoverManager::getInstance().clearAllTextures();
                            allItems = FileBrowser::scanLibrary();
                            FileBrowser::sortItems(allItems, currentSort);
                            visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                            selectedIndex = 0;
                            scrollOffset = 0;
                            smoothScrollOffset = 0.0f;
                            scrollBarTimer = 120;
                        } else if (initialTouchItemIndex >= 0 && initialTouchItemIndex < static_cast<int>(visibleItems.size())) {
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
                                    if (scrollOffset < 0) scrollOffset = 0;
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
                                    int totalRows = (static_cast<int>(visibleItems.size()) + gridCols - 1) / gridCols;
                                    int maxRowOffset = std::max(0, totalRows - gridVisibleRows);
                                    int maxOffset = maxRowOffset * gridCols;
                                    if (scrollOffset > maxOffset) scrollOffset = maxOffset;
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

                    // Vincular Páginas com Triângulo
                    if ((pressed & SCE_CTRL_TRIANGLE) && !visibleItems.empty()) {
                        openPagesLink(selectedIndex);
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
                        settingsReturnState = AppState::MENU;
                        CoverManager::getInstance().clearAllTextures();
                        currentState = AppState::SETTINGS;
                        settingsSelectedIndex = 0;
                    }
                } else if (currentFocus == FocusArea::REFRESH) {
                    if (pressed & SCE_CTRL_DOWN) currentFocus = FocusArea::CONTENT;
                    if (pressed & SCE_CTRL_LEFT) currentFocus = FocusArea::SETTINGS;
                    if (pressed & SCE_CTRL_CROSS) {
                        CoverManager::getInstance().clearAllTextures();
                        CoverManager::getInstance().refreshDiskIndex();
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
                    CoverManager::getInstance().clearAllTextures();
                    CoverManager::getInstance().refreshDiskIndex();
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
                        float clipBottom = UIComponents::getScreenH() - 40.0f;
                        float clipFadeBottom = clipBottom - 8.0f;
                        if (cardBottom <= 68.0f || cardTop >= clipBottom) {
                            continue;
                        }

                        // Dissolver/Fade ao aproximar dos limites superior e inferior
                        float cardAlpha = 1.0f;
                        if (cardTop < 86.0f) {
                            cardAlpha = (cardBottom - 68.0f) / cardHeight;
                        } else if (cardBottom > clipFadeBottom) {
                            cardAlpha = (clipBottom - cardTop) / cardHeight;
                        }
                        cardAlpha = std::max(0.0f, std::min(1.0f, cardAlpha));

                        vita2d_texture* coverTex = CoverManager::getInstance().getCoverTexture(item.fullPath);

                        float cardListW = readerRotated ? 480.0f : 896.0f;
                        UIComponents::drawListCard(32.0f, y, cardListW, cardHeight, isSelected, item, cardAlpha, coverTex);
                    }

                } else {
                    // Renderização em Grade com Animação e Fade nos Extremos
                    float startX = 32.0f;
                    float startY = 86.0f;
                    float cardW = readerRotated ? 230.0f : 282.0f;
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
                            float clipBottom = UIComponents::getScreenH() - 40.0f;
                            float clipFadeBottom = clipBottom - 8.0f;
                            if (cardBottom <= 68.0f || cardTop >= clipBottom) {
                                continue;
                            }

                            // Dissolver/Fade ao aproximar dos limites superior e inferior
                            float cardAlpha = 1.0f;
                            if (cardTop < 86.0f) {
                                cardAlpha = (cardBottom - 68.0f) / cardH;
                            } else if (cardBottom > clipFadeBottom) {
                                cardAlpha = (clipBottom - cardTop) / cardH;
                            }
                            cardAlpha = std::max(0.0f, std::min(1.0f, cardAlpha));

                            vita2d_texture* coverTex = CoverManager::getInstance().getCoverTexture(item.fullPath);

                            UIComponents::drawGridCard(x, y, cardW, cardH, isSelected, item, cardAlpha, coverTex);
                        }
                    }
                }
            }

            // Barra Superior desenhada por cima do conteúdo (Z-Order Top)
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

            // Barra Inferior / Rodapé desenhado por cima do conteúdo (Z-Order Top)
            UIComponents::drawFooter("D-Pad: Navegar | X: Abrir/Acao | []: Favorito | /\\: Recarregar");

            // Barra de rolagem animada com fade
            if (!visibleItems.empty()) {
                if (!isGridView) {
                    UIComponents::drawLibraryScrollBar(smoothScrollOffset, static_cast<int>(visibleItems.size()), listVisibleCount, false, scrollBarAlpha);
                } else {
                    UIComponents::drawLibraryScrollBar(smoothScrollOffset, static_cast<int>(visibleItems.size()), gridVisibleCount, true, scrollBarAlpha);
                }
            }

            if (showDeleteConfirm && itemToDeleteIndex >= 0 && itemToDeleteIndex < static_cast<int>(visibleItems.size())) {
                std::string msg = "Deseja excluir: " + visibleItems[itemToDeleteIndex].filename + "?";
                UIComponents::drawConfirmDialog("Excluir Arquivo", msg, deleteConfirmYesSelected);
            }

            endDrawing();
            CoverManager::getInstance().trimCache(32);


        } else if (currentState == AppState::SETTINGS) {
            const int totalSettingsItems = 10;
            const int themeCount = static_cast<int>(ThemeId::COUNT);

            if (showClearCoversConfirm) {
                if (pressed & (SCE_CTRL_LEFT | SCE_CTRL_RIGHT)) {
                    clearCoversConfirmYesSelected = !clearCoversConfirmYesSelected;
                }
                if (pressed & SCE_CTRL_CROSS) {
                    if (clearCoversConfirmYesSelected) {
                        CoverManager::getInstance().clearCoverCacheOnDisk();
                    }
                    showClearCoversConfirm = false;
                }
                if (pressed & SCE_CTRL_CIRCLE) {
                    showClearCoversConfirm = false;
                }
                if (touchDown) {
                    UIConfirmDialogLayout dlg = UIComponents::getConfirmDialogLayout();
                    float tx = static_cast<float>(touchX);
                    float ty = static_cast<float>(touchY);
                    if (dlg.yesBtn.contains(tx, ty)) {
                        CoverManager::getInstance().clearCoverCacheOnDisk();
                        showClearCoversConfirm = false;
                    } else if (dlg.noBtn.contains(tx, ty)) {
                        showClearCoversConfirm = false;
                    }
                }
            } else {
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

                if (settingsSelectedIndex == 0) {
                    // Item 0: Seleção de Tema (Cards Horizontais)
                    if (pressed & SCE_CTRL_LEFT) {
                        if (settingsThemeCursor > 0) {
                            settingsThemeCursor--;
                            appConfig.theme = static_cast<ThemeId>(settingsThemeCursor);
                            ThemeManager::apply(appConfig.theme);
                            ConfigManager::getInstance().save();
                        }
                    }
                    if (pressed & SCE_CTRL_RIGHT) {
                        if (settingsThemeCursor < themeCount - 1) {
                            settingsThemeCursor++;
                            appConfig.theme = static_cast<ThemeId>(settingsThemeCursor);
                            ThemeManager::apply(appConfig.theme);
                            ConfigManager::getInstance().save();
                        }
                    }
                    if (pressed & SCE_CTRL_CROSS) {
                        appConfig.theme = static_cast<ThemeId>(settingsThemeCursor);
                        ThemeManager::apply(appConfig.theme);
                        ConfigManager::getInstance().save();
                    }
                } else if (changePrev || changeNext) {
                    switch (settingsSelectedIndex) {
                        case 1: // Modo de Exibição (Lista / Grade)
                            appConfig.isGridView = !appConfig.isGridView;
                            isGridView = appConfig.isGridView;
                            selectedIndex = 0;
                            scrollOffset = 0;
                            smoothScrollOffset = 0.0f;
                            break;
                        case 2: { // Ordenação Padrão
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
                        case 3: { // Gerar Miniaturas da Biblioteca (em lote)
                            if (pressed & (SCE_CTRL_CROSS | SCE_CTRL_RIGHT)) {
                                int totalFiles = static_cast<int>(allItems.size());
                                int generatedCount = 0;
                                bool cancelled = false;

                                for (int i = 0; i < totalFiles; i++) {
                                    sceCtrlPeekBufferPositive(0, &pad, 1);
                                    if (pad.buttons & SCE_CTRL_CIRCLE) {
                                        cancelled = true;
                                        break;
                                    }

                                    const auto& curItem = allItems[i];

                                    startDrawing();
                                    vita2d_clear_screen();
                                    UIComponents::drawSettingsScreen(settingsSelectedIndex, appConfig, settingsSmoothScrollY, settingsSmoothThemeScrollX, settingsThemeCursor);
                                    UIComponents::drawCoverProgressModal(i + 1, totalFiles, curItem.filename, generatedCount);
                                    endDrawing();

                                    if (curItem.type != FileType::TXT && !CoverManager::getInstance().hasCoverOnDisk(curItem.fullPath)) {
                                        if (CoverManager::getInstance().generateCover(curItem.fullPath)) {
                                            generatedCount++;
                                        }
                                    }
                                }

                                if (!cancelled && totalFiles > 0) {
                                    startDrawing();
                                    vita2d_clear_screen();
                                    UIComponents::drawSettingsScreen(settingsSelectedIndex, appConfig, settingsSmoothScrollY, settingsSmoothThemeScrollX, settingsThemeCursor);
                                    UIComponents::drawCoverProgressModal(totalFiles, totalFiles, "Concluído com sucesso!", generatedCount);
                                    endDrawing();
                                    sceKernelDelayThread(800000);
                                }
                                CoverManager::getInstance().refreshDiskIndex();
                                CoverManager::getInstance().clearAllTextures();
                            }
                            break;
                        }
                        case 4: { // Limpar Cache de Capas
                            if (pressed & (SCE_CTRL_CROSS | SCE_CTRL_RIGHT)) {
                                showClearCoversConfirm = true;
                                clearCoversConfirmYesSelected = false;
                            }
                            break;
                        }
                        case 5: // Orientação do Leitor
                            appConfig.readerRotated = !appConfig.readerRotated;
                            readerRotated = appConfig.readerRotated;
                            UIComponents::setRotated(readerRotated);
                            readerTxt.setRotated(readerRotated, pgf);
                            readerCbz.setRotated(readerRotated);
                            readerEpub.setRotated(readerRotated);
                            break;
                        case 6: { // Modo de Rolagem
                            int count = static_cast<int>(ReaderScrollMode::COUNT);
                            int cur = static_cast<int>(appConfig.readerScrollMode);
                            if (changeNext) cur = (cur + 1) % count;
                            else cur = (cur - 1 + count) % count;
                            appConfig.readerScrollMode = static_cast<ReaderScrollMode>(cur);
                            readerTxt.setScrollMode(appConfig.readerScrollMode);
                            readerCbz.setScrollMode(appConfig.readerScrollMode);
                            readerEpub.setScrollMode(appConfig.readerScrollMode);
                            break;
                        }
                        case 7: { // Transição de Página
                            int count = static_cast<int>(PageTransition::COUNT);
                            int cur = static_cast<int>(appConfig.pageTransition);
                            if (changeNext) cur = (cur + 1) % count;
                            else cur = (cur - 1 + count) % count;
                            appConfig.pageTransition = static_cast<PageTransition>(cur);
                            readerTxt.setPageTransition(appConfig.pageTransition);
                            readerCbz.setPageTransition(appConfig.pageTransition);
                            readerEpub.setPageTransition(appConfig.pageTransition);
                            break;
                        }
                        case 8: // Exibir Número de Páginas
                            appConfig.showPageNumbers = !appConfig.showPageNumbers;
                            break;
                        case 9: // Tamanho de Fonte Padrão (EPUB)
                            if (changeNext) {
                                if (appConfig.epubFontSize < 44) appConfig.epubFontSize += 2;
                            } else {
                                if (appConfig.epubFontSize > 14) appConfig.epubFontSize -= 2;
                            }
                            break;
                    }
                    ConfigManager::getInstance().save();
                }
            }

            // Animação de Scroll Vertical na tela de configurações (acompanha o item focado)
            if (settingsSelectedIndex == 0) {
                settingsScrollY = 0.0f;
            } else if (settingsSelectedIndex >= 3) {
                float screenHeight = UIComponents::getScreenH();
                if (screenHeight <= 544.0f) {
                    // No modo horizontal, rola para dar visibilidade aos itens inferiores
                    settingsScrollY = (settingsSelectedIndex - 2) * 52.0f;
                } else if (settingsSelectedIndex >= 6) {
                    settingsScrollY = (settingsSelectedIndex - 5) * 52.0f;
                } else {
                    settingsScrollY = 0.0f;
                }
            } else {
                settingsScrollY = 0.0f;
            }

            // Animação de Scroll Horizontal nos cards de tema (centraliza o card com foco)
            float cardStep = 92.0f + 12.0f; // cardW + gap
            float visibleAreaW = UIComponents::getScreenW() - 72.0f;
            float targetThemeScrollX = std::max(0.0f, (settingsThemeCursor * cardStep) - (visibleAreaW / 2.0f) + (cardStep / 2.0f));
            float maxThemeScrollX = std::max(0.0f, (themeCount * cardStep) - visibleAreaW);
            if (targetThemeScrollX > maxThemeScrollX) targetThemeScrollX = maxThemeScrollX;
            settingsThemeScrollX = targetThemeScrollX;

            settingsSmoothScrollY += (settingsScrollY - settingsSmoothScrollY) * 0.22f;
            settingsSmoothThemeScrollX += (settingsThemeScrollX - settingsSmoothThemeScrollX) * 0.22f;

            // Touch Gestures na tela de Configurações
            if (touchDown) {
                UISettingsLayout settingsLayout = UIComponents::getSettingsLayout(settingsSmoothScrollY);
                float tx = static_cast<float>(touchX);
                float ty = static_cast<float>(touchY);

                // Toque nos Cards de Tema
                bool hitThemeCard = false;
                for (int i = 0; i < themeCount; i++) {
                    UIRect cr = settingsLayout.getThemeCardRect(i, settingsSmoothThemeScrollX);
                    if (cr.contains(tx, ty)) {
                        settingsSelectedIndex = 0;
                        settingsThemeCursor = i;
                        appConfig.theme = static_cast<ThemeId>(i);
                        ThemeManager::apply(appConfig.theme);
                        ConfigManager::getInstance().save();
                        hitThemeCard = true;
                        break;
                    }
                }

                if (!hitThemeCard) {
                    if (settingsLayout.item1.contains(tx, ty)) {
                        settingsSelectedIndex = 1;
                        appConfig.isGridView = !appConfig.isGridView;
                        isGridView = appConfig.isGridView;
                        selectedIndex = 0; scrollOffset = 0; smoothScrollOffset = 0.0f;
                        ConfigManager::getInstance().save();
                    } else if (settingsLayout.item2.contains(tx, ty)) {
                        settingsSelectedIndex = 2;
                        int count = static_cast<int>(SortMode::COUNT);
                        int cur = (static_cast<int>(appConfig.sortMode) + 1) % count;
                        appConfig.sortMode = static_cast<SortMode>(cur);
                        currentSort = appConfig.sortMode;
                        FileBrowser::sortItems(allItems, currentSort);
                        visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                        selectedIndex = 0; scrollOffset = 0; smoothScrollOffset = 0.0f;
                        ConfigManager::getInstance().save();
                    } else if (settingsLayout.item3.contains(tx, ty)) {
                        settingsSelectedIndex = 3;
                        int totalFiles = static_cast<int>(allItems.size());
                        int generatedCount = 0;
                        bool cancelled = false;

                        for (int i = 0; i < totalFiles; i++) {
                            sceCtrlPeekBufferPositive(0, &pad, 1);
                            if (pad.buttons & SCE_CTRL_CIRCLE) {
                                cancelled = true;
                                break;
                            }

                            const auto& curItem = allItems[i];

                            startDrawing();
                            vita2d_clear_screen();
                            UIComponents::drawSettingsScreen(settingsSelectedIndex, appConfig, settingsSmoothScrollY, settingsSmoothThemeScrollX, settingsThemeCursor);
                            UIComponents::drawCoverProgressModal(i + 1, totalFiles, curItem.filename, generatedCount);
                            endDrawing();

                            if (curItem.type != FileType::TXT && !CoverManager::getInstance().hasCoverOnDisk(curItem.fullPath)) {
                                if (CoverManager::getInstance().generateCover(curItem.fullPath)) {
                                    generatedCount++;
                                }
                            }
                        }

                        if (!cancelled && totalFiles > 0) {
                            startDrawing();
                            vita2d_clear_screen();
                            UIComponents::drawSettingsScreen(settingsSelectedIndex, appConfig, settingsSmoothScrollY, settingsSmoothThemeScrollX, settingsThemeCursor);
                            UIComponents::drawCoverProgressModal(totalFiles, totalFiles, "Concluído com sucesso!", generatedCount);
                            endDrawing();
                            sceKernelDelayThread(800000);
                        }
                        CoverManager::getInstance().refreshDiskIndex();
                        CoverManager::getInstance().clearAllTextures();
                    } else if (settingsLayout.item4.contains(tx, ty)) {
                        settingsSelectedIndex = 4;
                        showClearCoversConfirm = true;
                        clearCoversConfirmYesSelected = false;
                    } else if (settingsLayout.item5.contains(tx, ty)) {
                        settingsSelectedIndex = 5;
                        appConfig.readerRotated = !appConfig.readerRotated;
                        readerRotated = appConfig.readerRotated;
                        UIComponents::setRotated(readerRotated);
                        readerTxt.setRotated(readerRotated, pgf);
                        readerCbz.setRotated(readerRotated);
                        readerEpub.setRotated(readerRotated);
                        ConfigManager::getInstance().save();
                    } else if (settingsLayout.item6.contains(tx, ty)) {
                        settingsSelectedIndex = 6;
                        int count = static_cast<int>(ReaderScrollMode::COUNT);
                        int cur = (static_cast<int>(appConfig.readerScrollMode) + 1) % count;
                        appConfig.readerScrollMode = static_cast<ReaderScrollMode>(cur);
                        readerTxt.setScrollMode(appConfig.readerScrollMode);
                        readerCbz.setScrollMode(appConfig.readerScrollMode);
                        readerEpub.setScrollMode(appConfig.readerScrollMode);
                        ConfigManager::getInstance().save();
                    } else if (settingsLayout.item7.contains(tx, ty)) {
                        settingsSelectedIndex = 7;
                        int count = static_cast<int>(PageTransition::COUNT);
                        int cur = (static_cast<int>(appConfig.pageTransition) + 1) % count;
                        appConfig.pageTransition = static_cast<PageTransition>(cur);
                        readerTxt.setPageTransition(appConfig.pageTransition);
                        readerCbz.setPageTransition(appConfig.pageTransition);
                        readerEpub.setPageTransition(appConfig.pageTransition);
                        ConfigManager::getInstance().save();
                    } else if (settingsLayout.item8.contains(tx, ty)) {
                        settingsSelectedIndex = 8;
                        appConfig.showPageNumbers = !appConfig.showPageNumbers;
                        ConfigManager::getInstance().save();
                    } else if (settingsLayout.item9.contains(tx, ty)) {
                        settingsSelectedIndex = 9;
                        if (tx > settingsLayout.fontValueSplitX) {
                            if (appConfig.epubFontSize < 44) appConfig.epubFontSize += 2;
                        } else {
                            if (appConfig.epubFontSize > 14) appConfig.epubFontSize -= 2;
                        }
                        ConfigManager::getInstance().save();
                    }
                }
            }

            // Voltar para o estado de origem (MENU ou leitor)
            if (!showClearCoversConfirm && (pressed & SCE_CTRL_CIRCLE)) {
                ConfigManager::getInstance().save();
                if (settingsReturnState == AppState::MENU) {
                    CoverManager::getInstance().refreshDiskIndex();
                    CoverManager::getInstance().clearAllTextures();
                }
                currentState = settingsReturnState;
                settingsReturnState = AppState::MENU;
                currentFocus = FocusArea::CONTENT;
            }

            startDrawing();
            vita2d_clear_screen();
            UIComponents::drawSettingsScreen(settingsSelectedIndex, appConfig, settingsSmoothScrollY, settingsSmoothThemeScrollX, settingsThemeCursor);
            if (showClearCoversConfirm) {
                UIComponents::drawConfirmDialog("Limpar Cache de Capas", "Deseja excluir todas as capas salvas?", clearCoversConfirmYesSelected);
            }
            endDrawing();

        } else if (currentState == AppState::READ_TXT) {
            if (pressed & (SCE_CTRL_RTRIGGER | SCE_CTRL_R1)) {
                if (visibleItems.size() > 1) openNextBook();
                else readerTxt.nextPage();
            } else if (pressed & (SCE_CTRL_LTRIGGER | SCE_CTRL_L1)) {
                if (visibleItems.size() > 1) openPrevBook();
                else readerTxt.prevPage();
            } else if (appConfig.readerScrollMode == ReaderScrollMode::InfiniteScroll ||
                       appConfig.readerScrollMode == ReaderScrollMode::PagedVertical) {
                if ((pressed & SCE_CTRL_DOWN) || readerTapRight) readerTxt.nextPage();
                if ((pressed & SCE_CTRL_UP) || readerTapLeft) readerTxt.prevPage();
            } else {
                if ((pressed & (SCE_CTRL_RIGHT | SCE_CTRL_DOWN)) || readerTapRight) {
                    readerTxt.nextPage();
                }
                if ((pressed & (SCE_CTRL_LEFT | SCE_CTRL_UP)) || readerTapLeft) {
                    readerTxt.prevPage();
                }
            }
            if (pressed & (SCE_CTRL_TRIANGLE | SCE_CTRL_SELECT)) {
                openPagesLink(selectedIndex);
            }
            if (pressed & SCE_CTRL_START) {
                settingsReturnState = currentState;
                currentState = AppState::SETTINGS;
                settingsSelectedIndex = 0;
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                readerTxt.close();
                currentState = AppState::MENU;
            }

            readerTxt.update(frameDt);
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
            } else if (appConfig.readerScrollMode == ReaderScrollMode::InfiniteScroll ||
                       appConfig.readerScrollMode == ReaderScrollMode::PagedVertical) {
                if ((pressed & SCE_CTRL_DOWN) || readerTapRight) readerCbz.nextPage();
                if ((pressed & SCE_CTRL_UP) || readerTapLeft) readerCbz.prevPage();
            } else {
                if ((pressed & (SCE_CTRL_RIGHT | SCE_CTRL_DOWN)) || readerTapRight) {
                    readerCbz.nextPage();
                }
                if ((pressed & (SCE_CTRL_LEFT | SCE_CTRL_UP)) || readerTapLeft) {
                    readerCbz.prevPage();
                }
            }
            if (pressed & SCE_CTRL_SQUARE) {
                readerCbz.toggleLinkedPage();
            }
            if (pressed & (SCE_CTRL_TRIANGLE | SCE_CTRL_SELECT)) {
                openPagesLink(selectedIndex);
            }
            if (pressed & SCE_CTRL_START) {
                settingsReturnState = currentState;
                currentState = AppState::SETTINGS;
                settingsSelectedIndex = 0;
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                readerCbz.close();
                currentState = AppState::MENU;
            }

            readerCbz.update(frameDt);
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
            } else if (appConfig.readerScrollMode == ReaderScrollMode::InfiniteScroll ||
                       appConfig.readerScrollMode == ReaderScrollMode::PagedVertical) {
                if ((pressed & SCE_CTRL_DOWN) || readerTapRight) readerEpub.nextPage();
                if ((pressed & SCE_CTRL_UP) || readerTapLeft) readerEpub.prevPage();
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
            if (pressed & SCE_CTRL_SELECT) {
                openPagesLink(selectedIndex);
            }
            if (pressed & SCE_CTRL_START) {
                settingsReturnState = currentState;
                currentState = AppState::SETTINGS;
                settingsSelectedIndex = 0;
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                readerEpub.close();
                currentState = AppState::MENU;
            }

            readerEpub.update(frameDt);
            startDrawing();
            vita2d_clear_screen();
            readerEpub.render(pgf, readerFullscreen);
            endDrawing();

        } else if (currentState == AppState::PAGES_LINK) {
            PagesLinkManager& linkMgr = PagesLinkManager::getInstance();
            auto& pagesLink = linkMgr.getPagesLink();
            auto& pagesNotLinked = linkMgr.getPagesNotLinked();
            int totalLinked = static_cast<int>(pagesLink.size());
            int totalNotLinked = static_cast<int>(pagesNotLinked.size());

            UIPagesLinkLayout layout = UIComponents::getPagesLinkLayout();

            // Constantes de layout para cálculo de cards
            const float linkedCardW = 176.0f;
            const float linkedCardH = 300.0f;
            const float linkedCardGap = 16.0f;
            const float linkedCardStep = linkedCardW + linkedCardGap; // 192.0f

            const float unlinkedCardW = 76.0f;
            const float unlinkedCardH = 100.0f;
            const float unlinkedCardGap = 12.0f;
            const float unlinkedCardStep = unlinkedCardW + unlinkedCardGap; // 88.0f

            float maxLinkedScroll = std::max(0.0f, totalLinked * linkedCardStep + 32.0f - 960.0f);
            float maxUnlinkedScroll = std::max(0.0f, totalNotLinked * unlinkedCardStep + 32.0f - 960.0f);

            // Verifica se o botão X (Cross) está sendo mantido pressionado
            plIsHoldingCross = (pad.buttons & SCE_CTRL_CROSS) != 0;

            // GESTÃO DE FOCO E TROCA RÁPIDA DE ZONAS COM OS GATILHOS (L1/R1)
            PagesLinkFocusArea plOldFocus = plFocus;
            if (pressed & (SCE_CTRL_LTRIGGER | SCE_CTRL_L1)) {
                if (plFocus == PagesLinkFocusArea::TOOLBAR) plFocus = (totalNotLinked > 0) ? PagesLinkFocusArea::UNLINKED_CARDS : PagesLinkFocusArea::LINKED_DUAL_CARDS;
                else if (plFocus == PagesLinkFocusArea::UNLINKED_CARDS) plFocus = PagesLinkFocusArea::LINKED_DUAL_CARDS;
                else if (plFocus == PagesLinkFocusArea::LINKED_DUAL_CARDS) plFocus = PagesLinkFocusArea::LINKED_CARDS;
                else plFocus = PagesLinkFocusArea::TOOLBAR;
            } else if (pressed & (SCE_CTRL_RTRIGGER | SCE_CTRL_R1)) {
                if (plFocus == PagesLinkFocusArea::TOOLBAR) plFocus = PagesLinkFocusArea::LINKED_CARDS;
                else if (plFocus == PagesLinkFocusArea::LINKED_CARDS) plFocus = PagesLinkFocusArea::LINKED_DUAL_CARDS;
                else if (plFocus == PagesLinkFocusArea::LINKED_DUAL_CARDS) plFocus = (totalNotLinked > 0) ? PagesLinkFocusArea::UNLINKED_CARDS : PagesLinkFocusArea::TOOLBAR;
                else plFocus = PagesLinkFocusArea::TOOLBAR;
            }

            // CONTROLE POR BOTÕES / GAMEPAD
            if (plFocus != plOldFocus && plFocus == PagesLinkFocusArea::TOOLBAR) {
                plSelectedToolbarBtn = 0;
            }
            if (plFocus == PagesLinkFocusArea::TOOLBAR) {
                if (pressed & SCE_CTRL_LEFT) {
                    if (plSelectedToolbarBtn > 0) plSelectedToolbarBtn--;
                }
                if (pressed & SCE_CTRL_RIGHT) {
                    if (plSelectedToolbarBtn < 6) plSelectedToolbarBtn++;
                }
                if (pressed & SCE_CTRL_DOWN) {
                    plFocus = PagesLinkFocusArea::LINKED_CARDS;
                }
                if (pressed & SCE_CTRL_CROSS) {
                    switch (plSelectedToolbarBtn) {
                        case 0:
                            plPickerItems = FileBrowser::scanLibrary();
                            plPickerSelectedIndex = 0;
                            plPickerScrollOffset = 0;
                            plPickerSmoothScrollOffset = 0.0f;
                            currentState = AppState::PAGES_LINK_FILE_PICKER;
                            break;
                        case 1: linkMgr.autoReorderDoublePages(); break;
                        case 2: linkMgr.reorderSimplePages(); break;
                        case 3: linkMgr.reorderDoublePages(); break;
                        case 4: linkMgr.reorderBySortPages(); break;
                        case 5: linkMgr.returnBackup(); break;
                        case 6:
                            linkMgr.save();
                            plToastMessage = "Vínculo salvo com sucesso!";
                            plToastTimer = 90;
                            break;
                    }
                }
            } else if (plFocus == PagesLinkFocusArea::LINKED_CARDS) {
                if (plIsHoldingCross) {
                    // Ao segurar X, mover para direita/esquerda na grid move o item selecionado de posição
                    if (pressed & SCE_CTRL_LEFT) {
                        if (plSelectedLinkedCard > 0) {
                            linkMgr.onMove(plSelectedLinkedCard, plSelectedLinkedCard - 1);
                            plSelectedLinkedCard--;
                            float targetX = (plSelectedLinkedCard * linkedCardStep) - (960.0f - linkedCardW) / 2.0f;
                            plLinkedScrollX = std::max(0.0f, std::min(maxLinkedScroll, targetX));
                        }
                    }
                    if (pressed & SCE_CTRL_RIGHT) {
                        if (plSelectedLinkedCard < totalLinked - 1) {
                            linkMgr.onMove(plSelectedLinkedCard, plSelectedLinkedCard + 1);
                            plSelectedLinkedCard++;
                            float targetX = (plSelectedLinkedCard * linkedCardStep) - (960.0f - linkedCardW) / 2.0f;
                            plLinkedScrollX = std::max(0.0f, std::min(maxLinkedScroll, targetX));
                        }
                    }
                } else {
                    // Navegação normal do cursor
                    if (pressed & SCE_CTRL_UP) {
                        plFocus = PagesLinkFocusArea::TOOLBAR;
                        plSelectedToolbarBtn = 0;
                    }
                    if (pressed & SCE_CTRL_DOWN) {
                        // Pula para a linha de página dupla
                        plFocus = PagesLinkFocusArea::LINKED_DUAL_CARDS;
                    }
                    if (pressed & SCE_CTRL_LEFT) {
                        if (plSelectedLinkedCard > 0) {
                            plSelectedLinkedCard--;
                            float targetX = (plSelectedLinkedCard * linkedCardStep) - (960.0f - linkedCardW) / 2.0f;
                            plLinkedScrollX = std::max(0.0f, std::min(maxLinkedScroll, targetX));
                        }
                    }
                    if (pressed & SCE_CTRL_RIGHT) {
                        if (plSelectedLinkedCard < totalLinked - 1) {
                            plSelectedLinkedCard++;
                            float targetX = (plSelectedLinkedCard * linkedCardStep) - (960.0f - linkedCardW) / 2.0f;
                            plLinkedScrollX = std::max(0.0f, std::min(maxLinkedScroll, targetX));
                        }
                    }
                }

                // Desvincular com Quadrado
                if (pressed & SCE_CTRL_SQUARE) {
                    linkMgr.onNotLinked(plSelectedLinkedCard);
                }
            } else if (plFocus == PagesLinkFocusArea::LINKED_DUAL_CARDS) {
                if (plIsHoldingCross) {
                    // Ao segurar X na linha de página dupla, move a página secundária/dupla
                    if (pressed & SCE_CTRL_LEFT) {
                        if (plSelectedLinkedCard > 0) {
                            linkMgr.onMoveDualPage(true, plSelectedLinkedCard, true, plSelectedLinkedCard - 1);
                            plSelectedLinkedCard--;
                            float targetX = (plSelectedLinkedCard * linkedCardStep) - (960.0f - linkedCardW) / 2.0f;
                            plLinkedScrollX = std::max(0.0f, std::min(maxLinkedScroll, targetX));
                        }
                    }
                    if (pressed & SCE_CTRL_RIGHT) {
                        if (plSelectedLinkedCard < totalLinked - 1) {
                            linkMgr.onMoveDualPage(true, plSelectedLinkedCard, true, plSelectedLinkedCard + 1);
                            plSelectedLinkedCard++;
                            float targetX = (plSelectedLinkedCard * linkedCardStep) - (960.0f - linkedCardW) / 2.0f;
                            plLinkedScrollX = std::max(0.0f, std::min(maxLinkedScroll, targetX));
                        }
                    }
                } else {
                    if (pressed & SCE_CTRL_UP) {
                        // Sobe para a linha de páginas vinculadas principal
                        plFocus = PagesLinkFocusArea::LINKED_CARDS;
                    }
                    if (pressed & SCE_CTRL_DOWN) {
                        // Desce para a lista de não vinculados
                        if (totalNotLinked > 0) {
                            plFocus = PagesLinkFocusArea::UNLINKED_CARDS;
                        }
                    }
                    if (pressed & SCE_CTRL_LEFT) {
                        if (plSelectedLinkedCard > 0) {
                            plSelectedLinkedCard--;
                            float targetX = (plSelectedLinkedCard * linkedCardStep) - (960.0f - linkedCardW) / 2.0f;
                            plLinkedScrollX = std::max(0.0f, std::min(maxLinkedScroll, targetX));
                        }
                    }
                    if (pressed & SCE_CTRL_RIGHT) {
                        if (plSelectedLinkedCard < totalLinked - 1) {
                            plSelectedLinkedCard++;
                            float targetX = (plSelectedLinkedCard * linkedCardStep) - (960.0f - linkedCardW) / 2.0f;
                            plLinkedScrollX = std::max(0.0f, std::min(maxLinkedScroll, targetX));
                        }
                    }
                }

                // Desvincular com Quadrado
                if (pressed & SCE_CTRL_SQUARE) {
                    linkMgr.onNotLinked(plSelectedLinkedCard);
                }
            } else if (plFocus == PagesLinkFocusArea::UNLINKED_CARDS) {
                if (plIsHoldingCross) {
                    // Ao segurar X na lista de não vinculados, reordena o item não vinculado
                    if (pressed & SCE_CTRL_LEFT) {
                        if (plSelectedUnlinkedCard > 0) {
                            std::swap(pagesNotLinked[plSelectedUnlinkedCard], pagesNotLinked[plSelectedUnlinkedCard - 1]);
                            plSelectedUnlinkedCard--;
                            float targetX = (plSelectedUnlinkedCard * unlinkedCardStep) - (960.0f - unlinkedCardW) / 2.0f;
                            plUnlinkedScrollX = std::max(0.0f, std::min(maxUnlinkedScroll, targetX));
                        }
                    }
                    if (pressed & SCE_CTRL_RIGHT) {
                        if (plSelectedUnlinkedCard < totalNotLinked - 1) {
                            std::swap(pagesNotLinked[plSelectedUnlinkedCard], pagesNotLinked[plSelectedUnlinkedCard + 1]);
                            plSelectedUnlinkedCard++;
                            float targetX = (plSelectedUnlinkedCard * unlinkedCardStep) - (960.0f - unlinkedCardW) / 2.0f;
                            plUnlinkedScrollX = std::max(0.0f, std::min(maxUnlinkedScroll, targetX));
                        }
                    }
                } else {
                    if (pressed & SCE_CTRL_UP) {
                        // Sobe para a linha de página dupla
                        plFocus = PagesLinkFocusArea::LINKED_DUAL_CARDS;
                    }
                    if (pressed & SCE_CTRL_LEFT) {
                        if (plSelectedUnlinkedCard > 0) {
                            plSelectedUnlinkedCard--;
                            float targetX = (plSelectedUnlinkedCard * unlinkedCardStep) - (960.0f - unlinkedCardW) / 2.0f;
                            plUnlinkedScrollX = std::max(0.0f, std::min(maxUnlinkedScroll, targetX));
                        }
                    }
                    if (pressed & SCE_CTRL_RIGHT) {
                        if (plSelectedUnlinkedCard < totalNotLinked - 1) {
                            plSelectedUnlinkedCard++;
                            float targetX = (plSelectedUnlinkedCard * unlinkedCardStep) - (960.0f - unlinkedCardW) / 2.0f;
                            plUnlinkedScrollX = std::max(0.0f, std::min(maxUnlinkedScroll, targetX));
                        }
                    }
                }
            }

            // Auto Dupla com Triângulo
            if (pressed & SCE_CTRL_TRIANGLE) {
                linkMgr.autoReorderDoublePages();
            }

            // Desfazer com Select
            if (pressed & SCE_CTRL_SELECT) {
                linkMgr.returnBackup();
            }

            // Salvar com Start
            if (pressed & SCE_CTRL_START) {
                linkMgr.save();
                plToastMessage = "Vínculo salvo com sucesso!";
                plToastTimer = 90;
            }

            // Voltar / Cancelar com Círculo
            if (pressed & SCE_CTRL_CIRCLE) {
                if (plIsDragging) {
                    plIsDragging = false;
                    plIsLongPressActive = false;
                    plDragOriginIndex = -1;
                    plHoverDropTargetIndex = -1;
                } else {
                    linkMgr.close();
                    currentState = plPreviousState;
                }
            }

            // GESTÃO AVANÇADA DE TOUCH, SCROLL E LONG-PRESS PARA REORGANIZAÇÃO
            if (touchDown) {
                float tx = static_cast<float>(touchX);
                float ty = static_cast<float>(touchY);

                plTouchStartX = tx;
                plTouchStartY = ty;
                plLongPressTimer = 0;
                plIsLongPressActive = false;
                plIsTouchScrolling = false;
                plTouchIsValidScrollArea = false;
                plCandidateTouchIndex = -1;
                plDraggingLinkedScrollbar = false;
                plDraggingUnlinkedScrollbar = false;

                // 1. Interação Direta com a Barra de Progresso / Rolagem Superior (Vinculados)
                if (ty >= 355.0f && ty <= 382.0f && tx >= 20.0f && tx <= 940.0f) {
                    plDraggingLinkedScrollbar = true;
                    if (maxLinkedScroll > 0.0f) {
                        float ratio = std::max(0.0f, std::min(1.0f, (tx - 24.0f) / 912.0f));
                        plLinkedScrollX = ratio * maxLinkedScroll;
                    }
                }
                // 2. Interação Direta com a Barra de Progresso / Rolagem Inferior (Não Vinculados)
                else if (ty >= 495.0f && ty <= 535.0f && tx >= 20.0f && tx <= 940.0f) {
                    plDraggingUnlinkedScrollbar = true;
                    if (maxUnlinkedScroll > 0.0f) {
                        float ratio = std::max(0.0f, std::min(1.0f, (tx - 24.0f) / 912.0f));
                        plUnlinkedScrollX = ratio * maxUnlinkedScroll;
                    }
                }
                // 3. Área dos Cards Vinculados
                else if (ty > 56.0f && ty < 380.0f) {
                    plTouchInitialScrollX = plLinkedScrollX;
                    plCandidateIsUnlinked = false;
                    plTouchIsValidScrollArea = true;
                    int clickedIdx = static_cast<int>((tx + plLinkedSmoothScrollX - 24.0f) / linkedCardStep);
                    if (clickedIdx >= 0 && clickedIdx < totalLinked) {
                        plCandidateTouchIndex = clickedIdx;
                    }
                }
                // 4. Área dos Cards Não Vinculados
                else if (ty >= 380.0f && ty < 544.0f) {
                    plTouchInitialScrollX = plUnlinkedScrollX;
                    plCandidateIsUnlinked = true;
                    plTouchIsValidScrollArea = true;
                    int clickedIdx = static_cast<int>((tx + plUnlinkedSmoothScrollX - 24.0f) / unlinkedCardStep);
                    if (clickedIdx >= 0 && clickedIdx < totalNotLinked) {
                        plCandidateTouchIndex = clickedIdx;
                    }
                }
            } else if (touch.reportNum > 0) {
                float tx = static_cast<float>(touchX);
                float ty = static_cast<float>(touchY);

                if (plDraggingLinkedScrollbar) {
                    if (maxLinkedScroll > 0.0f) {
                        float ratio = std::max(0.0f, std::min(1.0f, (tx - 24.0f) / 912.0f));
                        plLinkedScrollX = ratio * maxLinkedScroll;
                    }
                } else if (plDraggingUnlinkedScrollbar) {
                    if (maxUnlinkedScroll > 0.0f) {
                        float ratio = std::max(0.0f, std::min(1.0f, (tx - 24.0f) / 912.0f));
                        plUnlinkedScrollX = ratio * maxUnlinkedScroll;
                    }
                } else {
                    float distX = tx - plTouchStartX;
                    float distY = ty - plTouchStartY;
                    float totalDist = std::sqrt(distX * distX + distY * distY);

                    // Se ainda não ativou Long-Press e não está rolando
                    if (!plIsLongPressActive && !plIsTouchScrolling) {
                        if (totalDist > 14.0f && plTouchIsValidScrollArea) {
                            // Dedo moveu antes de 1 segundo: inicia ROLAGEM PURO (Touch Scroll)
                            plIsTouchScrolling = true;
                        } else if (plCandidateTouchIndex >= 0) {
                            bool canDrag = true;
                            // Bloqueia arraste se for card vinculado (top half) -> mangaArea
                            if (!plCandidateIsUnlinked && plTouchStartY < 211.0f) {
                                canDrag = false;
                            }
                            if (canDrag) {
                                // Dedo parado no mesmo ponto: incrementa temporizador de Long Click
                                plLongPressTimer++;
                                if (plLongPressTimer >= 55) {
                                    plIsLongPressActive = true;
                                    plIsDragging = true;
                                    plDragOriginIndex = plCandidateTouchIndex;
                                    plDragFromNotLinked = plCandidateIsUnlinked;
                                    plDragCurrentX = tx;
                                    plDragCurrentY = ty;
                                }
                            }
                        }
                    }

                    // Rolagem Suave com o Dedo
                    if (plIsTouchScrolling) {
                        if (!plCandidateIsUnlinked) {
                            plLinkedScrollX = std::max(0.0f, std::min(maxLinkedScroll, plTouchInitialScrollX - distX));
                        } else {
                            plUnlinkedScrollX = std::max(0.0f, std::min(maxUnlinkedScroll, plTouchInitialScrollX - distX));
                        }
                    }

                    // Arraste de Card após Long Click
                    if (plIsLongPressActive && plIsDragging) {
                        plDragCurrentX = tx;
                        plDragCurrentY = ty;

                        // Auto-rolagem suave nas bordas da tela durante o arraste
                        if (tx > 880.0f) {
                            if (!plDragFromNotLinked) plLinkedScrollX = std::min(maxLinkedScroll, plLinkedScrollX + 8.0f);
                            else plUnlinkedScrollX = std::min(maxUnlinkedScroll, plUnlinkedScrollX + 8.0f);
                        } else if (tx < 80.0f) {
                            if (!plDragFromNotLinked) plLinkedScrollX = std::max(0.0f, plLinkedScrollX - 8.0f);
                            else plUnlinkedScrollX = std::max(0.0f, plUnlinkedScrollX - 8.0f);
                        }

                        // Cálculo do alvo de soltura
                        if (ty < 380.0f) {
                            plHoverDropIsUnlinked = false;
                            int targetIdx = static_cast<int>((tx + plLinkedSmoothScrollX - 24.0f) / linkedCardStep);
                            plHoverDropTargetIndex = (targetIdx >= 0 && targetIdx < totalLinked) ? targetIdx : -1;
                        } else {
                            plHoverDropIsUnlinked = true;
                            int targetIdx = static_cast<int>((tx + plUnlinkedSmoothScrollX - 24.0f) / unlinkedCardStep);
                            plHoverDropTargetIndex = (targetIdx >= 0 && targetIdx < totalNotLinked) ? targetIdx : -1;
                        }
                    }
                }
            } else if (touchUp) {
                float tx = static_cast<float>(touchX);
                float ty = static_cast<float>(touchY);

                if (plDraggingLinkedScrollbar || plDraggingUnlinkedScrollbar) {
                    plDraggingLinkedScrollbar = false;
                    plDraggingUnlinkedScrollbar = false;
                } else if (plIsLongPressActive && plIsDragging) {
                    // Soltura do Card após Reorganização por Long Click
                    if (!plDragFromNotLinked) {
                        if (!plHoverDropIsUnlinked && plHoverDropTargetIndex >= 0 && plHoverDropTargetIndex != plDragOriginIndex) {
                            linkMgr.onMove(plDragOriginIndex, plHoverDropTargetIndex);
                            plSelectedLinkedCard = plHoverDropTargetIndex;
                            plFocus = PagesLinkFocusArea::LINKED_CARDS;
                        } else if (plHoverDropIsUnlinked) {
                            linkMgr.onNotLinked(plDragOriginIndex);
                        }
                    } else {
                        if (!plHoverDropIsUnlinked && plHoverDropTargetIndex >= 0) {
                            linkMgr.fromNotLinked(plDragOriginIndex, plHoverDropTargetIndex);
                            plSelectedLinkedCard = plHoverDropTargetIndex;
                            plFocus = PagesLinkFocusArea::LINKED_CARDS;
                        }
                    }
                    plIsDragging = false;
                    plIsLongPressActive = false;
                    plDragOriginIndex = -1;
                    plHoverDropTargetIndex = -1;
                } else if (!plIsTouchScrolling) {
                    // Clique Simples (Tap)
                    if (ty <= 56.0f) {
                        if (layout.selectFileBtn.contains(tx, ty)) {
                            plSelectedToolbarBtn = 0;
                            plPickerItems = FileBrowser::scanLibrary();
                            plPickerSelectedIndex = 0;
                            plPickerScrollOffset = 0;
                            plPickerSmoothScrollOffset = 0.0f;
                            currentState = AppState::PAGES_LINK_FILE_PICKER;
                        } else if (layout.autoBtn.contains(tx, ty)) {
                            plSelectedToolbarBtn = 1;
                            linkMgr.autoReorderDoublePages();
                        } else if (layout.simpleBtn.contains(tx, ty)) {
                            plSelectedToolbarBtn = 2;
                            linkMgr.reorderSimplePages();
                        } else if (layout.doubleBtn.contains(tx, ty)) {
                            plSelectedToolbarBtn = 3;
                            linkMgr.reorderDoublePages();
                        } else if (layout.sortBtn.contains(tx, ty)) {
                            plSelectedToolbarBtn = 4;
                            linkMgr.reorderBySortPages();
                        } else if (layout.undoBtn.contains(tx, ty)) {
                            plSelectedToolbarBtn = 5;
                            linkMgr.returnBackup();
                        } else if (layout.saveBtn.contains(tx, ty)) {
                            plSelectedToolbarBtn = 6;
                            linkMgr.save();
                            plToastMessage = "Vínculo salvo com sucesso!";
                            plToastTimer = 90;
                        }
                    } else if (ty > 56.0f && ty < 380.0f) {
                        if (plCandidateTouchIndex >= 0 && plCandidateTouchIndex < totalLinked) {
                            plSelectedLinkedCard = plCandidateTouchIndex;
                            plFocus = PagesLinkFocusArea::LINKED_CARDS;

                            // Verifica se tocou no botão de desvincular do card
                            float cardX = 24.0f + plCandidateTouchIndex * linkedCardStep - plLinkedSmoothScrollX;
                            if (tx > cardX + linkedCardW - 40.0f && ty > 64.0f + linkedCardH - 40.0f) {
                                linkMgr.onNotLinked(plCandidateTouchIndex);
                            }
                        }
                    } else if (ty >= 380.0f && ty < 544.0f) {
                        if (plCandidateTouchIndex >= 0 && plCandidateTouchIndex < totalNotLinked) {
                            plSelectedUnlinkedCard = plCandidateTouchIndex;
                            plFocus = PagesLinkFocusArea::UNLINKED_CARDS;
                        }
                    }
                }

                // Reset das variáveis temporárias do touch
                plIsTouchScrolling = false;
                plLongPressTimer = 0;
                plCandidateTouchIndex = -1;
            }

            // Suavização da Rolagem
            plLinkedSmoothScrollX += (plLinkedScrollX - plLinkedSmoothScrollX) * 0.25f;
            if (std::abs(plLinkedSmoothScrollX - plLinkedScrollX) < 0.01f) {
                plLinkedSmoothScrollX = plLinkedScrollX;
            }

            plUnlinkedSmoothScrollX += (plUnlinkedScrollX - plUnlinkedSmoothScrollX) * 0.25f;
            if (std::abs(plUnlinkedSmoothScrollX - plUnlinkedScrollX) < 0.01f) {
                plUnlinkedSmoothScrollX = plUnlinkedScrollX;
            }

            // Cards de Páginas Vinculadas (Área Principal) - Bounds de Visibilidade
            int firstVisibleLinked = std::max(0, static_cast<int>((plLinkedSmoothScrollX - 250.0f) / linkedCardStep));
            int lastVisibleLinked = std::min(totalLinked, static_cast<int>((plLinkedSmoothScrollX + 1210.0f) / linkedCardStep));

            int firstVisibleUnlinked = 0;
            int lastVisibleUnlinked = 0;
            if (totalNotLinked > 0) {
                firstVisibleUnlinked = std::max(0, static_cast<int>((plUnlinkedSmoothScrollX - 150.0f) / unlinkedCardStep));
                lastVisibleUnlinked = std::min(totalNotLinked, static_cast<int>((plUnlinkedSmoothScrollX + 1110.0f) / unlinkedCardStep));
            }

            // Coleta páginas ativas e pré-carregadas para o gerenciador de cache
            std::vector<int> keepPrimary;
            std::vector<int> keepLinked;
            keepPrimary.reserve((lastVisibleLinked - firstVisibleLinked) + 8);
            keepLinked.reserve((lastVisibleLinked - firstVisibleLinked) * 2 + (lastVisibleUnlinked - firstVisibleUnlinked) + 8);

            for (int i = firstVisibleLinked; i < lastVisibleLinked; i++) {
                const auto& page = pagesLink[i];
                if (page.mangaPage != PAGE_EMPTY) keepPrimary.push_back(page.mangaPage);
                if (page.fileLinkLeftPage != PAGE_EMPTY) keepLinked.push_back(page.fileLinkLeftPage);
                if (page.fileLinkRightPage != PAGE_EMPTY) keepLinked.push_back(page.fileLinkRightPage);
            }

            for (int i = firstVisibleUnlinked; i < lastVisibleUnlinked; i++) {
                const auto& page = pagesNotLinked[i];
                if (page.fileLinkLeftPage != PAGE_EMPTY) keepLinked.push_back(page.fileLinkLeftPage);
            }

            // Limpeza inteligente de cache de texturas não visíveis para economia de RAM
            linkMgr.purgeUnusedThumbnails(keepPrimary, keepLinked);

            // RENDERIZAÇÃO DA TELA DE VÍNCULO
            startDrawing();
            vita2d_clear_screen();

            // Toolbar superior
            UIComponents::drawPagesLinkToolbar(
                linkMgr.getPrimaryName(),
                linkMgr.getPrimaryPageCount(),
                linkMgr.getLinkedName(),
                linkMgr.getLinkedPageCount(),
                (plFocus == PagesLinkFocusArea::TOOLBAR) ? plSelectedToolbarBtn : -1,
                linkMgr.hasBackup(),
                linkMgr.isLinkedLoaded()
            );

            // Cards de Páginas Vinculadas (Área Principal)
            for (int i = firstVisibleLinked; i < lastVisibleLinked; i++) {
                const auto& page = pagesLink[i];
                float cardX = 24.0f + i * linkedCardStep - plLinkedSmoothScrollX;

                vita2d_texture* thumbManga = linkMgr.getPrimaryThumbnail(page.mangaPage);
                vita2d_texture* thumbLeft = (page.fileLinkLeftPage != PAGE_EMPTY) ? linkMgr.getLinkedThumbnail(page.fileLinkLeftPage) : nullptr;
                vita2d_texture* thumbRight = (page.fileLinkRightPage != PAGE_EMPTY) ? linkMgr.getLinkedThumbnail(page.fileLinkRightPage) : nullptr;

                bool isSelected = (plFocus == PagesLinkFocusArea::LINKED_CARDS || plFocus == PagesLinkFocusArea::LINKED_DUAL_CARDS) && (i == plSelectedLinkedCard);
                bool isDropTarget = (plIsDragging && !plHoverDropIsUnlinked && i == plHoverDropTargetIndex);
                bool isGrabbed = (isSelected && plIsHoldingCross) || (plIsDragging && !plDragFromNotLinked && i == plDragOriginIndex);
                bool isDualSlotFocused = (plFocus == PagesLinkFocusArea::LINKED_DUAL_CARDS && i == plSelectedLinkedCard);

                UIComponents::drawLinkedPageCard(cardX, 64.0f, linkedCardW, linkedCardH, i, page, thumbManga, thumbLeft, thumbRight, isSelected, isDropTarget, isGrabbed, isDualSlotFocused);
            }

            // Barra de Rolagem da Área Vinculada
            UIComponents::drawPagesLinkScrollBar(24.0f, 366.0f, 912.0f, 8.0f, plLinkedSmoothScrollX, totalLinked * linkedCardStep);

            // Área de Páginas Não Vinculadas (Rodapé Rolável)
            vita2d_draw_rectangle(0, 380.0f, UIComponents::getScreenW(), 134.0f, UITheme::TopBar);
            vita2d_draw_line(0, 380.0f, UIComponents::getScreenW(), 380.0f, UITheme::Divider);

            if (pgf) {
                char unlinkedTitle[48];
                snprintf(unlinkedTitle, sizeof(unlinkedTitle), "PÁGINAS NÃO VINCULADAS (%d)", totalNotLinked);
                vita2d_pgf_draw_text(pgf, 24.0f, 394.0f, UITheme::TextSecondary, 0.72f, unlinkedTitle);
            }

            if (totalNotLinked == 0) {
                if (pgf) {
                    vita2d_pgf_draw_text(pgf, 320.0f, 440.0f, UITheme::TextSecondary, 0.85f, "Nenhuma página não vinculada.");
                }
            } else {
                for (int i = firstVisibleUnlinked; i < lastVisibleUnlinked; i++) {
                    const auto& page = pagesNotLinked[i];
                    float cardX = 24.0f + i * unlinkedCardStep - plUnlinkedSmoothScrollX;

                    vita2d_texture* thumb = (page.fileLinkLeftPage != PAGE_EMPTY) ? linkMgr.getLinkedThumbnail(page.fileLinkLeftPage) : nullptr;
                    bool isSelected = (plFocus == PagesLinkFocusArea::UNLINKED_CARDS && i == plSelectedUnlinkedCard);
                    bool isDropTarget = (plIsDragging && plHoverDropIsUnlinked && i == plHoverDropTargetIndex);
                    bool isGrabbed = (isSelected && plIsHoldingCross) || (plIsDragging && plDragFromNotLinked && i == plDragOriginIndex);

                    UIComponents::drawUnlinkedCard(cardX, 400.0f, unlinkedCardW, unlinkedCardH, i, page, thumb, isSelected, isDropTarget, isGrabbed);
                }

                // Barra de Rolagem da Área Não Vinculada
                UIComponents::drawPagesLinkScrollBar(24.0f, 506.0f, 912.0f, 8.0f, plUnlinkedSmoothScrollX, totalNotLinked * unlinkedCardStep);
            }

            // Desenha Fantasma Flutuante se Arraste Ativo
            if (plIsDragging && plDragOriginIndex >= 0) {
                vita2d_texture* ghostThumb = nullptr;
                std::string ghostLabel = "";

                if (!plDragFromNotLinked && plDragOriginIndex < totalLinked) {
                    int p = pagesLink[plDragOriginIndex].fileLinkLeftPage;
                    if (p != PAGE_EMPTY) ghostThumb = linkMgr.getLinkedThumbnail(p);
                    char buf[16]; snprintf(buf, sizeof(buf), "#%d", p + 1); ghostLabel = buf;
                } else if (plDragFromNotLinked && plDragOriginIndex < totalNotLinked) {
                    int p = pagesNotLinked[plDragOriginIndex].fileLinkLeftPage;
                    if (p != PAGE_EMPTY) ghostThumb = linkMgr.getLinkedThumbnail(p);
                    char buf[16]; snprintf(buf, sizeof(buf), "#%d", p + 1); ghostLabel = buf;
                }

                UIComponents::drawDragGhost(plDragCurrentX, plDragCurrentY, 110.0f, 150.0f, ghostThumb, ghostLabel);
            }

            // Banner Toast de Notificação
            if (plToastTimer > 0) {
                plToastTimer--;
                float toastW = 320.0f;
                float toastH = 40.0f;
                float toastX = (UIComponents::getScreenW() - toastW) / 2.0f;
                float toastY = 70.0f;
                UIComponents::drawRoundedBox(toastX, toastY, toastW, toastH, 8.0f, UITheme::Primary);
                if (pgf) {
                    float tW = vita2d_pgf_text_width(pgf, 0.85f, plToastMessage.c_str());
                    vita2d_pgf_draw_text(pgf, toastX + (toastW - tW) / 2.0f, toastY + 26.0f, UITheme::OnPrimary, 0.85f, plToastMessage.c_str());
                }
            }

            // Rodapé com dicas de navegação dinâmicas
            if (plIsHoldingCross) {
                UIComponents::drawFooter("Segurando [X]: D-Pad Dir/Esq move posição do card | Solte [X] para fixar");
            } else if (plFocus == PagesLinkFocusArea::LINKED_DUAL_CARDS) {
                UIComponents::drawFooter("Linha de Página Dupla | D-Pad: Cima/Baixo/Esq/Dir | Segure [X] + Dir/Esq: Mover | []: Desvincular | O: Voltar");
            } else {
                UIComponents::drawFooter("D-Pad: Navegar Zonas | Segure [X] + Dir/Esq: Mover Card | Long Touch: Arrastar | []: Desvincular | TRI: Auto | O: Voltar");
            }

            endDrawing();

        } else if (currentState == AppState::PAGES_LINK_FILE_PICKER) {
            const int listVisibleCount = 5;
            int totalItems = static_cast<int>(plPickerItems.size());

            if (pressed & SCE_CTRL_UP) {
                if (plPickerSelectedIndex > 0) {
                    plPickerSelectedIndex--;
                    if (plPickerSelectedIndex < plPickerScrollOffset) {
                        plPickerScrollOffset = plPickerSelectedIndex;
                    }
                }
            }
            if (pressed & SCE_CTRL_DOWN) {
                if (plPickerSelectedIndex < totalItems - 1) {
                    plPickerSelectedIndex++;
                    if (plPickerSelectedIndex >= plPickerScrollOffset + listVisibleCount) {
                        plPickerScrollOffset = plPickerSelectedIndex - listVisibleCount + 1;
                    }
                }
            }
            if (pressed & SCE_CTRL_CROSS) {
                if (!plPickerItems.empty() && plPickerSelectedIndex >= 0 && plPickerSelectedIndex < totalItems) {
                    PagesLinkManager::getInstance().loadLinkedFile(plPickerItems[plPickerSelectedIndex].fullPath);
                    currentState = AppState::PAGES_LINK;
                }
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                currentState = AppState::PAGES_LINK;
            }

            // Touch selection
            if (touchDown) {
                float ty = static_cast<float>(touchY);
                if (ty >= 86.0f && ty <= 460.0f) {
                    int clickedIdx = plPickerScrollOffset + static_cast<int>((ty - 86.0f) / 78.0f);
                    if (clickedIdx >= 0 && clickedIdx < totalItems) {
                        plPickerSelectedIndex = clickedIdx;
                        PagesLinkManager::getInstance().loadLinkedFile(plPickerItems[clickedIdx].fullPath);
                        currentState = AppState::PAGES_LINK;
                    }
                }
            }

            plPickerSmoothScrollOffset += (static_cast<float>(plPickerScrollOffset) - plPickerSmoothScrollOffset) * 0.22f;

            startDrawing();
            vita2d_clear_screen();

            // Header do Picker
            vita2d_draw_rectangle(0, 0, UIComponents::getScreenW(), 68.0f, UITheme::TopBar);
            vita2d_draw_line(0, 68.0f, UIComponents::getScreenW(), 68.0f, UITheme::Divider);
            if (pgf) {
                vita2d_pgf_draw_text(pgf, 32.0f, 42.0f, UITheme::TextPrimary, 1.15f, "Selecione o Arquivo para Vínculo (ex: Traduzido / Português)");
            }

            // Lista de Livros / Mangás
            float startY = 86.0f;
            float cardHeight = 64.0f;
            float cardSpacing = 14.0f;
            float stepY = cardHeight + cardSpacing;

            for (int i = 0; i < totalItems; i++) {
                float y = startY + (static_cast<float>(i) - plPickerSmoothScrollOffset) * stepY;
                if (y + cardHeight <= 68.0f || y >= UIComponents::getScreenH() - 40.0f) continue;
                bool isSelected = (i == plPickerSelectedIndex);
                UIComponents::drawListCard(32.0f, y, 896.0f, cardHeight, isSelected, plPickerItems[i], 1.0f);
            }

            // Scrollbar do Picker
            if (totalItems > listVisibleCount) {
                UIComponents::drawLibraryScrollBar(plPickerSmoothScrollOffset, totalItems, listVisibleCount, false, 1.0f);
            }

            UIComponents::drawFooter("D-Pad / Toque: Selecionar Arquivo | X: Confirmar | O: Cancelar");

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
